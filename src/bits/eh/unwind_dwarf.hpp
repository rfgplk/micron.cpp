//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh_dwarf)

#include "../../exit.hpp"
#include "../../types.hpp"
#include "dwarf_cfi.hpp"
#include "unwind.hpp"

// concrete definition of the context the personality is handed
struct _Unwind_Context {
  micron::eh::reg_context *reg;
  usize cfa;
  usize ra;      // PC in this frame (a return address)
  const micron::eh::fde_info *fde;
  int ip_before_insn;
};

namespace micron::eh
{

inline _Unwind_Reason_Code
unwind_phase1(_Unwind_Exception *exc, reg_context work) noexcept
{
  usize pc = work.reg[DWARF_RA_COLUMN];
  for ( ;; ) {
    fde_info fde;
    if ( !find_fde(pc - 1, fde) ) return _URC_END_OF_STACK;
    cfi_row row;
    run_cfi(fde, pc - 1, row);
    const usize cfa = compute_cfa(row, work);

    if ( fde.cie.personality ) {
      _Unwind_Context ctx{ &work, cfa, pc, &fde, fde.cie.signal_frame ? 1 : 0 };
      const _Unwind_Reason_Code r = fde.cie.personality(1, _UA_SEARCH_PHASE, exc->exception_class, exc, &ctx);
      if ( r == _URC_HANDLER_FOUND ) {
        exc->private_2 = cfa;
        return _URC_NO_REASON;
      }
      if ( r != _URC_CONTINUE_UNWIND ) return r;
    }
    if ( !cfi_advance(work, row, cfa, pc) ) return _URC_END_OF_STACK;
  }
}

[[noreturn]] inline void
unwind_phase2(_Unwind_Exception *exc, reg_context work) noexcept
{
  usize pc = work.reg[DWARF_RA_COLUMN];
  for ( ;; ) {
    fde_info fde;
    if ( !find_fde(pc - 1, fde) ) break;
    cfi_row row;
    run_cfi(fde, pc - 1, row);
    const usize cfa = compute_cfa(row, work);

    if ( fde.cie.personality ) {
      _Unwind_Action actions = _UA_CLEANUP_PHASE;
      if ( cfa == exc->private_2 ) actions |= _UA_HANDLER_FRAME;
      _Unwind_Context ctx{ &work, cfa, pc, &fde, fde.cie.signal_frame ? 1 : 0 };
      const _Unwind_Reason_Code r = fde.cie.personality(1, actions, exc->exception_class, exc, &ctx);
      if ( r == _URC_INSTALL_CONTEXT ) __micron_eh_install_context(&work);      // jumps to landing pad
      if ( r != _URC_CONTINUE_UNWIND ) break;
    }
    if ( !cfi_advance(work, row, cfa, pc) ) break;
  }
  micron::abort(0xff);
}

};      // namespace micron::eh

extern "C" {

inline _Unwind_Reason_Code
_Unwind_RaiseException(_Unwind_Exception *exc)
{
  using namespace micron::eh;
  reg_context start;
  __micron_eh_capture(&start);      // capture this frame; phase loops walk copies

  const _Unwind_Reason_Code r = unwind_phase1(exc, start);
  if ( r != _URC_NO_REASON ) return r;

  unwind_phase2(exc, start);      // installs (no return) on success
  __builtin_unreachable();
}

inline void
_Unwind_Resume(_Unwind_Exception *exc)
{
  using namespace micron::eh;
  reg_context start;
  __micron_eh_capture(&start);      // capture the cleanup frame; continue phase 2
  unwind_phase2(exc, start);
  __builtin_unreachable();
}

inline _Unwind_Reason_Code
_Unwind_Resume_or_Rethrow(_Unwind_Exception *exc)
{
  return _Unwind_RaiseException(exc);
}

inline void
_Unwind_DeleteException(_Unwind_Exception *exc)
{
  if ( exc->exception_cleanup != nullptr ) exc->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, exc);
}

inline _Unwind_Reason_Code
_Unwind_ForcedUnwind(_Unwind_Exception *exc, _Unwind_Stop_Fn stop, void *stop_parameter)
{
  using namespace micron::eh;
  reg_context start;
  __micron_eh_capture(&start);
  reg_context work = start;
  usize pc = work.reg[DWARF_RA_COLUMN];
  for ( ;; ) {
    fde_info fde;
    const bool found = find_fde(pc - 1, fde);
    cfi_row row;
    usize cfa = 0;
    if ( found ) {
      run_cfi(fde, pc - 1, row);
      cfa = compute_cfa(row, work);
    }
    _Unwind_Context ctx{ &work, cfa, pc, found ? &fde : nullptr, (found && fde.cie.signal_frame) ? 1 : 0 };
    const _Unwind_Action act = _UA_FORCE_UNWIND | _UA_CLEANUP_PHASE | (found ? 0 : _UA_END_OF_STACK);
    const _Unwind_Reason_Code sr = stop(1, act, exc->exception_class, exc, &ctx, stop_parameter);
    if ( sr != _URC_NO_REASON ) return sr;
    if ( found && fde.cie.personality ) {
      const _Unwind_Reason_Code r = fde.cie.personality(1, act, exc->exception_class, exc, &ctx);
      if ( r == _URC_INSTALL_CONTEXT ) __micron_eh_install_context(&work);
    }
    if ( !found || !cfi_advance(work, row, cfa, pc) ) return _URC_END_OF_STACK;
  }
}

inline _Unwind_Word
_Unwind_GetGR(_Unwind_Context *context, int index)
{
  return context->reg->reg[index];
}

inline void
_Unwind_SetGR(_Unwind_Context *context, int index, _Unwind_Word value)
{
  context->reg->reg[index] = value;
}

inline _Unwind_Ptr
_Unwind_GetIP(_Unwind_Context *context)
{
  return context->ra;
}

inline _Unwind_Ptr
_Unwind_GetIPInfo(_Unwind_Context *context, int *ip_before_insn)
{
  *ip_before_insn = context->ip_before_insn;
  return context->ra;
}

inline void
_Unwind_SetIP(_Unwind_Context *context, _Unwind_Ptr value)
{
  context->ra = value;
  context->reg->reg[micron::eh::DWARF_RA_COLUMN] = value;      // install jumps here
}

inline _Unwind_Word
_Unwind_GetCFA(_Unwind_Context *context)
{
  return context->cfa;
}

inline _Unwind_Ptr
_Unwind_GetLanguageSpecificData(_Unwind_Context *context)
{
  return context->fde ? reinterpret_cast<_Unwind_Ptr>(context->fde->lsda) : 0;
}

inline _Unwind_Ptr
_Unwind_GetRegionStart(_Unwind_Context *context)
{
  return context->fde ? context->fde->func_start : 0;
}

inline _Unwind_Ptr
_Unwind_GetDataRelBase(_Unwind_Context *)
{
  return micron::eh::__eh_state().data_base;
}

inline _Unwind_Ptr
_Unwind_GetTextRelBase(_Unwind_Context *)
{
  return micron::eh::__eh_state().text_base;
}

};      // extern "C"

#endif      // __micron_eh_dwarf
