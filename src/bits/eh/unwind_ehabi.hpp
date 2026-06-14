//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh_ehabi)

#include "../../exit.hpp"
#include "../../types.hpp"
#include "cxa_eh_globals.hpp"
#include "cxa_exception.hpp"
#include "dwarf_enc.hpp"
#include "eh_debug.hpp"
#include "terminate.hpp"
#include "typeinfo.hpp"
#include "unwind.hpp"

// WARNING: arm32 does not use DWARF .eh_frame; instead it uses the ARM Exception Handling ABI
// (IHI0038) a sorted .ARM.exidx index that maps a PC to a .ARM.extab entry,
// which carries compact unwind opcodes
// therefore register unwinding is EHABI specific but catch matching reuses the same
// LSDA format as the DWARF backend

extern "C" const byte __ehdr_start[] __attribute__((visibility("hidden")));

// the unwinder context = the Virtual Register Set (core registers r0-r15)
struct _Unwind_Context {
  usize core[16];
};

typedef int _Unwind_State;

enum {
  _US_VIRTUAL_UNWIND_FRAME = 0,
  _US_UNWIND_FRAME_STARTING = 1,
  _US_UNWIND_FRAME_RESUME = 2,
  _US_ACTION_MASK = 3,
  _US_FORCE_UNWIND = 8,
  _US_END_OF_STACK = 16
};

namespace micron::eh
{

constexpr u32 EXIDX_CANTUNWIND = 1;

// .ARM.exidx discovery via PT_ARM_EXIDX
#if defined(__micron_arch_width_64)
struct elf_ehdr {
  byte e_ident[16];
  u16 e_type, e_machine;
  u32 e_version;
  u64 e_entry, e_phoff, e_shoff;
  u32 e_flags;
  u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
};

struct elf_phdr {
  u32 p_type, p_flags;
  u64 p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align;
};
#else
struct elf_ehdr {
  byte e_ident[16];
  u16 e_type, e_machine;
  u32 e_version, e_entry, e_phoff, e_shoff, e_flags;
  u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
};

struct elf_phdr {
  u32 p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
};
#endif

constexpr u32 PT_ARM_EXIDX = 0x70000001;

struct exidx_state {
  const u32 *table = nullptr;      // sorted array of {prel31 fn, data} pairs
  usize count = 0;
  bool initialized = false;
  bool valid = false;
};

inline exidx_state &
__exidx_state() noexcept
{
  static exidx_state s;
  return s;
}

// decode a 31-bit place-relative (prel31) offset at *p into an absolute address
inline usize
prel31(const u32 *p) noexcept
{
  i32 off = static_cast<i32>(*p << 1) >> 1;      // sign-extend bit30 -> 31-bit signed
  return reinterpret_cast<usize>(p) + static_cast<usize>(off);
}

inline void
register_exidx() noexcept
{
  exidx_state &s = __exidx_state();
  if ( s.initialized ) return;
  s.initialized = true;

  const elf_ehdr *eh = reinterpret_cast<const elf_ehdr *>(__ehdr_start);
  const byte *phbase = __ehdr_start + eh->e_phoff;
  usize bias = 0;
  for ( u16 i = 0; i < eh->e_phnum; ++i ) {
    const elf_phdr *ph = reinterpret_cast<const elf_phdr *>(phbase + static_cast<usize>(i) * eh->e_phentsize);
    if ( ph->p_type == 6 /*PT_PHDR*/ ) {
      bias = reinterpret_cast<usize>(phbase) - static_cast<usize>(ph->p_vaddr);
      break;
    }
  }
  for ( u16 i = 0; i < eh->e_phnum; ++i ) {
    const elf_phdr *ph = reinterpret_cast<const elf_phdr *>(phbase + static_cast<usize>(i) * eh->e_phentsize);
    if ( ph->p_type == PT_ARM_EXIDX ) {
      s.table = reinterpret_cast<const u32 *>(static_cast<usize>(ph->p_vaddr) + bias);
      s.count = static_cast<usize>(ph->p_memsz) / 8;      // 8 bytes per entry
      s.valid = true;
      return;
    }
  }
}

inline const u32 *
find_exidx(usize pc) noexcept
{
  exidx_state &s = __exidx_state();
  if ( !s.initialized ) register_exidx();
  if ( !s.valid || s.count == 0 ) return nullptr;

  usize lo = 0, hi = s.count;
  if ( pc < prel31(&s.table[0]) ) return nullptr;
  while ( hi - lo > 1 ) {
    const usize mid = lo + (hi - lo) / 2;
    if ( prel31(&s.table[mid * 2]) <= pc )
      lo = mid;
    else
      hi = mid;
  }
  return &s.table[lo * 2];
}

struct op_stream {
  const u32 *words;
  usize total;      // total opcode bytes
  usize idx;
  int first_n;      // opcode bytes carried in words[0] (3 or 2)
};

inline byte
op_at(const op_stream &s, usize i) noexcept
{
  if ( i < static_cast<usize>(s.first_n) ) return static_cast<byte>(s.words[0] >> (8 * (s.first_n - 1 - static_cast<int>(i))));
  const usize j = i - static_cast<usize>(s.first_n);
  const u32 w = s.words[1 + j / 4];
  return static_cast<byte>(w >> (24 - 8 * (j % 4)));
}

inline bool
exec_opcodes(op_stream &st, _Unwind_Context *ctx) noexcept
{
  usize vsp = ctx->core[13];
  while ( st.idx < st.total ) {
    const byte op = op_at(st, st.idx++);
    if ( (op & 0xc0) == 0x00 ) {      // 00xxxxxx: vsp += (xxxxxx<<2)+4
      vsp += (static_cast<usize>(op & 0x3f) << 2) + 4;
    } else if ( (op & 0xc0) == 0x40 ) {      // 01xxxxxx: vsp -= (xxxxxx<<2)+4
      vsp -= (static_cast<usize>(op & 0x3f) << 2) + 4;
    } else if ( (op & 0xf0) == 0x80 ) {      // 1000iiii iiiiiiii: pop {r4-r15} by mask
      const byte op2 = op_at(st, st.idx++);
      const u32 mask = (static_cast<u32>(op & 0x0f) << 8) | op2;
      if ( mask == 0 ) return false;      // refuse to unwind
      for ( int r = 0; r < 12; ++r )
        if ( mask & (1u << r) ) {
          ctx->core[4 + r] = read_unaligned<usize>(reinterpret_cast<const byte *>(vsp));
          vsp += 4;
        }
    } else if ( (op & 0xf0) == 0x90 && op != 0x9d && op != 0x9f ) {      // 1001nnnn: vsp = r[nnnn]
      vsp = ctx->core[op & 0x0f];
    } else if ( (op & 0xf8) == 0xa0 ) {      // 10100nnn: pop r4-r[4+n]
      const int n = op & 0x07;
      for ( int r = 0; r <= n; ++r ) {
        ctx->core[4 + r] = read_unaligned<usize>(reinterpret_cast<const byte *>(vsp));
        vsp += 4;
      }
    } else if ( (op & 0xf8) == 0xa8 ) {      // 10101nnn: pop r4-r[4+n] and r14
      const int n = op & 0x07;
      for ( int r = 0; r <= n; ++r ) {
        ctx->core[4 + r] = read_unaligned<usize>(reinterpret_cast<const byte *>(vsp));
        vsp += 4;
      }
      ctx->core[14] = read_unaligned<usize>(reinterpret_cast<const byte *>(vsp));
      vsp += 4;
    } else if ( op == 0xb0 ) {      // finish
      ctx->core[13] = vsp;
      if ( ctx->core[15] == 0 ) ctx->core[15] = ctx->core[14];      // pc not set -> use lr
      return true;
    } else if ( op == 0xb1 ) {      // 10110001 0000iiii: pop {r0-r3} by mask
      const byte op2 = op_at(st, st.idx++);
      if ( op2 == 0 || (op2 & 0xf0) ) { /* spare / reserved */
      } else
        for ( int r = 0; r < 4; ++r )
          if ( op2 & (1u << r) ) {
            ctx->core[r] = read_unaligned<usize>(reinterpret_cast<const byte *>(vsp));
            vsp += 4;
          }
    } else if ( op == 0xb2 ) {      // vsp += 0x204 + (uleb<<2)
      u64 shift = 0, v = 0;
      byte b;
      do {
        b = op_at(st, st.idx++);
        v |= static_cast<u64>(b & 0x7f) << shift;
        shift += 7;
      } while ( b & 0x80 );
      vsp += 0x204 + (static_cast<usize>(v) << 2);
    } else if ( op == 0xb3 ) {      // FSTMFDX: operand sssscccc -> (cccc+1) doubles + 1 alignment word
      const byte d = op_at(st, st.idx++);
      vsp += (static_cast<usize>(d & 0x0f) + 1) * 8 + 4;
    } else if ( op == 0xc8 || op == 0xc9 || op == 0xc6 ) {      // VFP/iWMMXt pop: operand sssscccc -> (cccc+1) regs
      const byte d = op_at(st, st.idx++);
      vsp += (static_cast<usize>(d & 0x0f) + 1) * 8;
    } else if ( (op & 0xf8) == 0xb8 ) {      // pop d8-d[8+nnn] (FSTMFDX): (nnn+1) doubles + 1 word
      vsp += (static_cast<usize>(op & 0x07) + 1) * 8 + 4;
    } else if ( (op & 0xf8) == 0xc0 ) {      // pop wR[10]-wR[10+nnn] / d-regs: (nnn+1) regs
      vsp += (static_cast<usize>(op & 0x07) + 1) * 8;
    } else if ( op == 0xc7 ) {      // iWMMXt wCGR pop: operand 0000iiii (no vsp change tracked here)
      ++st.idx;
    } else {
      // NOTE: VFP/iWMMXt register vals are not restored
      ctx->core[13] = vsp;
      return true;
    }
  }
  ctx->core[13] = vsp;
  if ( ctx->core[15] == 0 ) ctx->core[15] = ctx->core[14];
  return true;
}

};      // namespace micron::eh

extern "C" {

inline _Unwind_VRS_Result
_Unwind_VRS_Get(_Unwind_Context *context, _Unwind_VRS_RegClass regclass, u32 regno, _Unwind_VRS_DataRepresentation, void *valuep)
{
  if ( regclass != _UVRSC_CORE || regno > 15 ) return _UVRSR_FAILED;
  *static_cast<usize *>(valuep) = context->core[regno];
  return _UVRSR_OK;
}

inline _Unwind_VRS_Result
_Unwind_VRS_Set(_Unwind_Context *context, _Unwind_VRS_RegClass regclass, u32 regno, _Unwind_VRS_DataRepresentation, void *valuep)
{
  if ( regclass != _UVRSC_CORE || regno > 15 ) return _UVRSR_FAILED;
  context->core[regno] = *static_cast<usize *>(valuep);
  return _UVRSR_OK;
}

inline _Unwind_Word
_Unwind_GetGR(_Unwind_Context *context, int index)
{
  return context->core[index & 15];
}

inline void
_Unwind_SetGR(_Unwind_Context *context, int index, _Unwind_Word value)
{
  context->core[index & 15] = value;
}

inline _Unwind_Ptr
_Unwind_GetIP(_Unwind_Context *context)
{
  return context->core[15] & ~static_cast<usize>(1);      // strip Thumb bit
}

inline _Unwind_Ptr
_Unwind_GetIPInfo(_Unwind_Context *context, int *ip_before_insn)
{
  *ip_before_insn = 0;
  return context->core[15] & ~static_cast<usize>(1);
}

inline void
_Unwind_SetIP(_Unwind_Context *context, _Unwind_Ptr value)
{
  context->core[15] = value | (context->core[15] & 1);      // preserve Thumb bit
}

}      // extern "C"

namespace micron::eh
{

// thread the exidx/extab lookup into an op_stream + (for C++ frames) the LSDA
struct frame_unwind {
  int pr_kind;
  op_stream ops;
  const byte *lsda;      // nullptr if none
  usize func_start;
};

inline bool
decode_frame(usize pc, frame_unwind &out) noexcept
{
  if ( pc == 0 ) return false;
  const u32 *entry = find_exidx(pc - 1);
  if ( !entry ) return false;
  out.func_start = prel31(&entry[0]);
  const u32 data = entry[1];
  if ( data == EXIDX_CANTUNWIND ) return false;
  out.lsda = nullptr;      // only the generic (C++) form carries an LSDA

  if ( data & 0x80000000u ) {
    out.pr_kind = static_cast<int>((data >> 24) & 0x0f);
    out.ops = op_stream{ &entry[1], 3, 0, 3 };
    return true;
  }
  // long entry: prel31 -> .ARM.extab
  const u32 *extab = reinterpret_cast<const u32 *>(prel31(&entry[1]));
  const u32 w0 = extab[0];
  if ( w0 & 0x80000000u ) {
    // compact model in extab
    out.pr_kind = static_cast<int>((w0 >> 24) & 0x0f);
    if ( out.pr_kind == 0 ) {
      out.ops = op_stream{ extab, 3, 0, 3 };
    } else {
      const u32 n = (w0 >> 16) & 0xff;      // pr1/pr2: byte 2 = extra-word count, bytes 1-0 = 2 opcodes
      out.ops = op_stream{ extab, static_cast<usize>(2 + 4 * n), 0, 2 };
    }
    return true;
  }
  out.pr_kind = 3;
  const u32 *ehtp = &extab[1];
  const u32 count = ehtp[0] >> 24;
  out.ops = op_stream{ ehtp, static_cast<usize>(3 + 4 * count), 0, 3 };
  out.lsda = reinterpret_cast<const byte *>(&ehtp[1 + count]);
  return true;
}

};      // namespace micron::eh

extern "C" {

inline _Unwind_Reason_Code
__gnu_unwind_frame(_Unwind_Exception *, _Unwind_Context *context)
{
  using namespace micron::eh;
  frame_unwind fu;
  if ( !decode_frame(_Unwind_GetIP(context), fu) ) return _URC_FAILURE;
  context->core[15] = 0;      // clear pc; opcodes/finish will set it (or lr)
  exec_opcodes(fu.ops, context);
  return _URC_OK;
}

inline _Unwind_Reason_Code
__aeabi_unwind_cpp_pr0(_Unwind_State state, _Unwind_Exception *ucbp, _Unwind_Context *context)
{
  using namespace micron::eh;
  (void)state;
  return __gnu_unwind_frame(ucbp, context);
}

inline _Unwind_Reason_Code
__aeabi_unwind_cpp_pr1(_Unwind_State state, _Unwind_Exception *ucbp, _Unwind_Context *context)
{
  return __aeabi_unwind_cpp_pr0(state, ucbp, context);
}

inline _Unwind_Reason_Code
__aeabi_unwind_cpp_pr2(_Unwind_State state, _Unwind_Exception *ucbp, _Unwind_Context *context)
{
  return __aeabi_unwind_cpp_pr0(state, ucbp, context);
}

};      // extern "C"

// C++ personality (ARM signature)
namespace micron::eh
{

inline bool
ehabi_scan_lsda(const byte *lsda, usize ip, usize func_start, _Unwind_Exception *ue, bool search_phase, bool &is_handler,
                usize &landing_pad, int &selector, void *&adjusted) noexcept
{
  using __cxxabiv1::__cxa_exception;
  const decode_ctx dctx{ 0, 0, 0, func_start };

  const byte *p = lsda;
  const u8 lpstart_enc = *p++;
  usize lpstart = func_start;
  if ( lpstart_enc != DW_EH_PE_omit ) lpstart = read_encoded(lpstart_enc, p, dctx);

  const u8 ttype_enc = *p++;
  const byte *ttype_base = nullptr;
  if ( ttype_enc != DW_EH_PE_omit ) {
    const u64 off = read_uleb128(p);
    ttype_base = p + off;
  }
  const u8 cs_enc = *p++;
  const u64 cs_len = read_uleb128(p);
  const byte *cs_ptr = p;
  const byte *cs_end = p + cs_len;
  const byte *action_table = cs_end;

  const usize pc_off = (ip - 1) - func_start;      // ip is a return address; step into the call site
  usize lp = 0;
  const byte *action_record = nullptr;
  bool found = false;
  while ( cs_ptr < cs_end ) {
    const usize cs_start = read_encoded(cs_enc, cs_ptr, dctx);
    const usize cs_clen = read_encoded(cs_enc, cs_ptr, dctx);
    const usize cs_lp = read_encoded(cs_enc, cs_ptr, dctx);
    const u64 cs_action = read_uleb128(cs_ptr);
    if ( pc_off < cs_start ) break;
    if ( pc_off < cs_start + cs_clen ) {
      if ( cs_lp ) lp = lpstart + cs_lp;
      if ( cs_action ) action_record = action_table + (cs_action - 1);
      found = true;
      break;
    }
  }
  if ( !found || lp == 0 ) return false;

  __cxa_exception *xh = __cxxabiv1::__get_exception_header_from_ue(ue);
  std::type_info *throw_type = xh->exceptionType;
  void *thrown_ptr = __cxxabiv1::__get_object_from_ue(ue);

  if ( action_record == nullptr ) {      // cleanup-only
    if ( search_phase ) return false;
    is_handler = false;
    landing_pad = lp;
    selector = 0;
    adjusted = thrown_ptr;
    return true;
  }

  const byte *a = action_record;
  for ( ;; ) {
    const byte *ap = a;
    const i64 ar_filter = read_sleb128(ap);
    const byte *next_field = ap;
    const i64 ar_next = read_sleb128(ap);
    if ( ar_filter == 0 ) {
      // cleanup
    } else if ( ar_filter > 0 ) {
      usize sz = encoded_size(ttype_enc);
      if ( sz == 0 ) sz = sizeof(void *);
      const byte *ent = ttype_base - static_cast<u64>(ar_filter) * sz;
      const byte *q = ent;
      std::type_info *catch_type = reinterpret_cast<std::type_info *>(read_encoded(ttype_enc, q, dctx));
      if ( catch_type == nullptr ) {
        is_handler = true;
        landing_pad = lp;
        selector = static_cast<int>(ar_filter);
        adjusted = thrown_ptr;
        return true;
      }
      void *adj = thrown_ptr;
      if ( catch_type->__do_catch(throw_type, &adj, 1) ) {
        is_handler = true;
        landing_pad = lp;
        selector = static_cast<int>(ar_filter);
        adjusted = adj;
        return true;
      }
    }
    if ( ar_next == 0 ) break;
    a = next_field + ar_next;
  }

  if ( search_phase ) return false;      // only cleanups matched here
  is_handler = false;
  landing_pad = lp;
  selector = 0;
  adjusted = thrown_ptr;
  return true;
}

};      // namespace micron::eh

extern "C" {

inline _Unwind_Reason_Code
__gxx_personality_v0(_Unwind_State state, _Unwind_Exception *ucbp, _Unwind_Context *context)
{
  using namespace micron::eh;
  using __cxxabiv1::__cxa_exception;

  frame_unwind fu;
  if ( !decode_frame(_Unwind_GetIP(context), fu) ) return _URC_FAILURE;

  const int action = state & _US_ACTION_MASK;
  __cxa_exception *xh = __cxxabiv1::__get_exception_header_from_ue(ucbp);

  if ( action == _US_VIRTUAL_UNWIND_FRAME ) {
    // phase 1: search for a handler in this frame
    if ( fu.lsda ) {
      bool is_handler = false;
      usize lp = 0;
      int sel = 0;
      void *adj = nullptr;
      if ( ehabi_scan_lsda(fu.lsda, _Unwind_GetIP(context), fu.func_start, ucbp, true, is_handler, lp, sel, adj) && is_handler ) {
        // stash for phase 2 in the barrier cache
        ucbp->barrier_cache.sp = context->core[13];
        ucbp->barrier_cache.bitpattern[0] = static_cast<_Unwind_Word>(reinterpret_cast<usize>(adj));
        ucbp->barrier_cache.bitpattern[1] = static_cast<_Unwind_Word>(static_cast<usize>(sel));
        ucbp->barrier_cache.bitpattern[2] = static_cast<_Unwind_Word>(lp);
        xh->adjustedPtr = adj;
        xh->handlerSwitchValue = sel;
        return _URC_HANDLER_FOUND;
      }
    }
    // not a handler frame: unwind one frame and continue
    context->core[15] = 0;
    exec_opcodes(fu.ops, context);
    return _URC_CONTINUE_UNWIND;
  }

  const bool is_handler_frame = (state & _US_ACTION_MASK) == _US_UNWIND_FRAME_STARTING && context->core[13] == ucbp->barrier_cache.sp;

  if ( fu.lsda ) {
    bool is_handler = false;
    usize lp = 0;
    int sel = 0;
    void *adj = nullptr;
    if ( ehabi_scan_lsda(fu.lsda, _Unwind_GetIP(context), fu.func_start, ucbp, false, is_handler, lp, sel, adj) ) {
      if ( is_handler && !is_handler_frame ) {
        // a handler match but not THE recorded handler frame -> treat as continue
      } else {
        if ( is_handler ) {
          xh->adjustedPtr = adj;
          xh->handlerSwitchValue = sel;
        }
        _Unwind_SetGR(context, 0, reinterpret_cast<_Unwind_Word>(ucbp));
        _Unwind_SetGR(context, 1, static_cast<_Unwind_Word>(static_cast<intptr_t>(is_handler ? sel : 0)));
        _Unwind_SetIP(context, lp);
        return _URC_INSTALL_CONTEXT;
      }
    }
  }
  // nothing to do here: unwind and continue
  context->core[15] = 0;
  exec_opcodes(fu.ops, context);
  return _URC_CONTINUE_UNWIND;
}

};      // extern "C"

extern "C" {
void __micron_eh_capture(_Unwind_Context *) noexcept;
[[noreturn]] void __micron_eh_install_context(_Unwind_Context *) noexcept;
}

namespace micron::eh
{

[[noreturn]] inline void
ehabi_phase2(_Unwind_Exception *ue, _Unwind_Context ctx) noexcept
{
  for ( ;; ) {
    frame_unwind fu;
    if ( !decode_frame(_Unwind_GetIP(&ctx), fu) ) break;
    _Unwind_Reason_Code r;
    if ( fu.pr_kind == 3 )
      r = __gxx_personality_v0(_US_UNWIND_FRAME_STARTING, ue, &ctx);
    else {
      // pure-unwind frame: just step
      ctx.core[15] = 0;
      exec_opcodes(fu.ops, &ctx);
      continue;
    }
    if ( r == _URC_INSTALL_CONTEXT ) __micron_eh_install_context(&ctx);
    if ( r != _URC_CONTINUE_UNWIND ) break;
  }
  micron::abort(0xff);
}

};      // namespace micron::eh

extern "C" {

inline _Unwind_Reason_Code
_Unwind_RaiseException(_Unwind_Exception *ucbp)
{
  using namespace micron::eh;
  __cxa_get_globals()->propagatingExceptions = __cxxabiv1::__get_exception_header_from_ue(ucbp);
  _Unwind_Context ctx;
  __micron_eh_capture(&ctx);

  _Unwind_Context work = ctx;
  for ( ;; ) {
    frame_unwind fu;
    if ( !decode_frame(_Unwind_GetIP(&work), fu) ) return _URC_END_OF_STACK;
    if ( fu.pr_kind == 3 ) {
      const _Unwind_Reason_Code r = __gxx_personality_v0(_US_VIRTUAL_UNWIND_FRAME, ucbp, &work);
      if ( r == _URC_HANDLER_FOUND ) break;
      if ( r != _URC_CONTINUE_UNWIND ) return r;
    } else {
      work.core[15] = 0;
      exec_opcodes(fu.ops, &work);
    }
  }
  ehabi_phase2(ucbp, ctx);
  __builtin_unreachable();
}

inline void
_Unwind_Resume(_Unwind_Exception *ucbp)
{
  using namespace micron::eh;
  __cxa_get_globals()->propagatingExceptions = __cxxabiv1::__get_exception_header_from_ue(ucbp);
  _Unwind_Context ctx;
  __micron_eh_capture(&ctx);
  ehabi_phase2(ucbp, ctx);
  __builtin_unreachable();
}

inline _Unwind_Reason_Code
_Unwind_Resume_or_Rethrow(_Unwind_Exception *ucbp)
{
  return _Unwind_RaiseException(ucbp);
}

inline void
_Unwind_Complete(_Unwind_Exception *)
{
}

inline void
_Unwind_DeleteException(_Unwind_Exception *ucbp)
{
  if ( ucbp->exception_cleanup ) ucbp->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, ucbp);
}

inline _Unwind_Reason_Code
_Unwind_ForcedUnwind(_Unwind_Exception *ucbp, _Unwind_Stop_Fn, void *)
{
  return _Unwind_RaiseException(ucbp);
}

inline void
__cxa_begin_cleanup(_Unwind_Exception *)
{
}

[[noreturn]] inline void
__cxa_end_cleanup()
{
  __cxxabiv1::__cxa_eh_globals *g = __cxa_get_globals();
  __cxxabiv1::__cxa_exception *h = reinterpret_cast<__cxxabiv1::__cxa_exception *>(g->propagatingExceptions);
  if ( h ) _Unwind_Resume(&h->unwindHeader);
  micron::eh::__terminate();
}

};      // extern "C"

// capture / install asm (arm32)
asm(R"asm(
	.text
	.weak __micron_eh_capture
	.type __micron_eh_capture, %function
__micron_eh_capture:
	str r4, [r0, #16]
	str r5, [r0, #20]
	str r6, [r0, #24]
	str r7, [r0, #28]
	str r8, [r0, #32]
	str r9, [r0, #36]
	str r10, [r0, #40]
	str r11, [r0, #44]
	str r12, [r0, #48]
	str sp, [r0, #52]
	str lr, [r0, #56]
	str lr, [r0, #60]
	bx lr
	.weak __micron_eh_install_context
	.type __micron_eh_install_context, %function
__micron_eh_install_context:
	mov r12, r0
	ldr r1, [r12, #4]
	ldr r4, [r12, #16]
	ldr r5, [r12, #20]
	ldr r6, [r12, #24]
	ldr r7, [r12, #28]
	ldr r8, [r12, #32]
	ldr r9, [r12, #36]
	ldr r10, [r12, #40]
	ldr r11, [r12, #44]
	ldr sp, [r12, #52]
	ldr lr, [r12, #60]
	ldr r0, [r12, #0]
	bx lr
)asm");

#endif      // __micron_eh_ehabi
