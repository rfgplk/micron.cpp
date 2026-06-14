//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh_dwarf)

#include "../../types.hpp"
#include "cxa_exception.hpp"
#include "dwarf_enc.hpp"
#include "eh_debug.hpp"
#include "typeinfo.hpp"
#include "unwind.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __gxx_personality_v0

namespace micron::eh
{

// fetch the index-th (1-based, backward) entry from the TType table
inline std::type_info *
get_ttype_entry(const byte *ttype_base, u8 ttype_enc, u64 index, const decode_ctx &ctx) noexcept
{
  if ( ttype_enc == DW_EH_PE_omit ) return nullptr;
  usize sz = encoded_size(ttype_enc);
  if ( sz == 0 ) sz = sizeof(void *);      // leb-encoded ttype tables are not emitted by gcc
  const byte *entry = ttype_base - index * sz;
  const byte *q = entry;
  const usize v = read_encoded(ttype_enc, q, ctx);
  return reinterpret_cast<std::type_info *>(v);
}

};      // namespace micron::eh

extern "C" inline _Unwind_Reason_Code
__gxx_personality_v0(int version, _Unwind_Action actions, _Unwind_Exception_Class exception_class, _Unwind_Exception *ue,
                     _Unwind_Context *context)
{
  using namespace micron::eh;
  using __cxxabiv1::__cxa_exception;

  if ( version != 1 || ue == nullptr || context == nullptr ) return _URC_FATAL_PHASE1_ERROR;

  __dbg_kv("[pers] actions=", static_cast<usize>(actions));

  const byte *lsda = reinterpret_cast<const byte *>(_Unwind_GetLanguageSpecificData(context));
  if ( !lsda ) return _URC_CONTINUE_UNWIND;      // no actions in this frame

  const usize region_start = _Unwind_GetRegionStart(context);
  int ip_before = 0;
  usize ip = _Unwind_GetIPInfo(context, &ip_before);
  if ( !ip_before && ip != 0 ) ip -= 1;      // call return address -> step back into the call
  const usize pc_offset = ip - region_start;

  const decode_ctx dctx{ 0, _Unwind_GetTextRelBase(context), _Unwind_GetDataRelBase(context), region_start };

  // %%% parse the LSDA header %%%
  const byte *p = lsda;
  const u8 lpstart_enc = *p++;
  usize lpstart = region_start;
  if ( lpstart_enc != DW_EH_PE_omit ) lpstart = read_encoded(lpstart_enc, p, dctx);

  const u8 ttype_enc = *p++;
  const byte *ttype_base = nullptr;
  if ( ttype_enc != DW_EH_PE_omit ) {
    const u64 ttype_off = read_uleb128(p);
    ttype_base = p + ttype_off;
  }

  const u8 cs_enc = *p++;
  const u64 cs_table_len = read_uleb128(p);
  const byte *cs_ptr = p;
  const byte *cs_end = p + cs_table_len;
  const byte *action_table = cs_end;

  // scan the call site table for the entry covering pc_offset
  usize landing_pad = 0;
  const byte *action_record = nullptr;
  bool found_cs = false;

  while ( cs_ptr < cs_end ) {
    const usize cs_start = read_encoded(cs_enc, cs_ptr, dctx);
    const usize cs_len = read_encoded(cs_enc, cs_ptr, dctx);
    const usize cs_lp = read_encoded(cs_enc, cs_ptr, dctx);
    const u64 cs_action = read_uleb128(cs_ptr);

    if ( pc_offset < cs_start ) break;      // table is sorted; we have passed it
    if ( pc_offset < cs_start + cs_len ) {
      if ( cs_lp ) landing_pad = lpstart + cs_lp;
      if ( cs_action ) action_record = action_table + (cs_action - 1);
      found_cs = true;
      break;
    }
  }

  __dbg_kv("[pers] pc_offset=", pc_offset);
  __dbg_kv("  found_cs=", found_cs);
  __dbg_kv("  landing_pad=", landing_pad);
  if ( !found_cs ) return _URC_CONTINUE_UNWIND;             // PC has no EH region here
  if ( landing_pad == 0 ) return _URC_CONTINUE_UNWIND;      // region with no landing pad

  // recover the thrown type / object from the exception header
  __cxa_exception *xh = __cxxabiv1::__get_exception_header_from_ue(ue);
  const bool native = (exception_class & 0xffffffffffffff00ULL) == (__cxxabiv1::__gxx_exception_class & 0xffffffffffffff00ULL);
  std::type_info *throw_type = native ? xh->exceptionType : nullptr;
  void *thrown_ptr = __cxxabiv1::__get_object_from_ue(ue);

  // no action chain -> cleanup-only landing pad
  if ( action_record == nullptr ) {
    if ( actions & _UA_SEARCH_PHASE ) return _URC_CONTINUE_UNWIND;      // cleanups don't stop phase 1
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(0), reinterpret_cast<_Unwind_Word>(ue));
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(1), 0);
    _Unwind_SetIP(context, landing_pad);
    return _URC_INSTALL_CONTEXT;
  }

  // walk the action chain looking for a matching catch
  int handler_switch = 0;
  bool saw_cleanup = false;
  bool saw_handler = false;
  void *handler_ptr = thrown_ptr;

  const byte *a = action_record;
  for ( ;; ) {
    const byte *ap = a;
    const i64 ar_filter = read_sleb128(ap);
    const byte *next_field = ap;
    const i64 ar_next = read_sleb128(ap);

    if ( ar_filter == 0 ) {
      saw_cleanup = true;
    } else if ( ar_filter > 0 ) {
      std::type_info *catch_type = get_ttype_entry(ttype_base, ttype_enc, static_cast<u64>(ar_filter), dctx);
      if ( catch_type == nullptr ) {
        // catch(...) -- matches anything
        saw_handler = true;
        handler_switch = static_cast<int>(ar_filter);
        handler_ptr = thrown_ptr;
        break;
      }
      if ( throw_type != nullptr ) {
        void *adj = thrown_ptr;
        if ( catch_type->__do_catch(throw_type, &adj, 1) ) {
          saw_handler = true;
          handler_switch = static_cast<int>(ar_filter);
          handler_ptr = adj;
          break;
        }
      }
    } else {
      saw_handler = true;
      handler_switch = static_cast<int>(ar_filter);
      handler_ptr = thrown_ptr;
      break;
    }

    if ( ar_next == 0 ) break;
    a = next_field + ar_next;      // ar_next is self-relative to its own field
  }

  if ( actions & _UA_SEARCH_PHASE ) {
    if ( saw_handler ) {
      // stash for phase 2 / __cxa_begin_catch
      xh->handlerSwitchValue = handler_switch;
      xh->actionRecord = reinterpret_cast<const unsigned char *>(action_record);
      xh->languageSpecificData = reinterpret_cast<const unsigned char *>(lsda);
      xh->catchTemp = landing_pad;
      xh->adjustedPtr = handler_ptr;
      return _URC_HANDLER_FOUND;
    }
    return _URC_CONTINUE_UNWIND;      // only cleanups here
  }

  // phase 2 / cleanup phase
  const bool is_handler_frame = (actions & _UA_HANDLER_FRAME) != 0;
  if ( is_handler_frame && saw_handler ) {
    xh->handlerSwitchValue = handler_switch;
    xh->adjustedPtr = handler_ptr;
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(0), reinterpret_cast<_Unwind_Word>(ue));
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(1), static_cast<_Unwind_Word>(static_cast<intptr_t>(handler_switch)));
    _Unwind_SetIP(context, landing_pad);
    return _URC_INSTALL_CONTEXT;
  }
  if ( saw_cleanup ) {
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(0), reinterpret_cast<_Unwind_Word>(ue));
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(1), 0);
    _Unwind_SetIP(context, landing_pad);
    return _URC_INSTALL_CONTEXT;
  }
  return _URC_CONTINUE_UNWIND;
}

#endif      // __micron_eh_dwarf
