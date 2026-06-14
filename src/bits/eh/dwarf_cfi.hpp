//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh_dwarf)

#include "../../types.hpp"
#include "dwarf_enc.hpp"
#include "find_fde.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// DWARF CFI interpreter

namespace micron::eh
{

// arch register model
#if defined(__micron_arch_amd64)
constexpr int DWARF_REG_COUNT = 17;      // rax..r15 (0..15) + RA (16)
constexpr int DWARF_RA_COLUMN = 16;
constexpr int DWARF_SP_COLUMN = 7;
#elif defined(__micron_arch_x86)
constexpr int DWARF_REG_COUNT = 9;      // eax..edi (0..7) + RA (8)
constexpr int DWARF_RA_COLUMN = 8;
constexpr int DWARF_SP_COLUMN = 4;
#elif defined(__micron_arch_arm64)
constexpr int DWARF_REG_COUNT = 33;      // x0..x30 (0..30) + SP (31); FP/SIMD V-regs (64..95) not restored
constexpr int DWARF_RA_COLUMN = 30;      // LR
constexpr int DWARF_SP_COLUMN = 31;
#endif

constexpr int DWARF_MAX_REG = DWARF_REG_COUNT;

struct reg_context {
  usize reg[DWARF_MAX_REG];
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// capture / install trampolines
extern "C" {
void __micron_eh_capture(reg_context *) noexcept;
[[noreturn]] void __micron_eh_install_context(reg_context *) noexcept;
}

// DWARF loc expression evaluator
enum : u8 {
  DW_OP_addr = 0x03,
  DW_OP_deref = 0x06,
  DW_OP_const1u = 0x08,
  DW_OP_const1s = 0x09,
  DW_OP_const2u = 0x0a,
  DW_OP_const2s = 0x0b,
  DW_OP_const4u = 0x0c,
  DW_OP_const4s = 0x0d,
  DW_OP_const8u = 0x0e,
  DW_OP_const8s = 0x0f,
  DW_OP_constu = 0x10,
  DW_OP_consts = 0x11,
  DW_OP_dup = 0x12,
  DW_OP_drop = 0x13,
  DW_OP_over = 0x14,
  DW_OP_swap = 0x16,
  DW_OP_plus = 0x22,
  DW_OP_plus_uconst = 0x23,
  DW_OP_minus = 0x1c,
  DW_OP_and = 0x1a,
  DW_OP_or = 0x21,
  DW_OP_shl = 0x24,
  DW_OP_shr = 0x25,
  DW_OP_lit0 = 0x30,
  DW_OP_lit31 = 0x4f,
  DW_OP_reg0 = 0x50,
  DW_OP_reg31 = 0x6f,
  DW_OP_breg0 = 0x70,
  DW_OP_breg31 = 0x8f,
  DW_OP_regx = 0x90,
  DW_OP_bregx = 0x92,
  DW_OP_nop = 0x96
};

inline usize
eval_dwarf_expr(const byte *p, const byte *end, const reg_context &ctx, usize initial, bool push_initial) noexcept
{
  usize stk[64];
  int sp = 0;
  if ( push_initial ) stk[sp++] = initial;

  while ( p < end ) {
    const u8 op = *p++;
    if ( op >= DW_OP_lit0 && op <= DW_OP_lit31 ) {
      stk[sp++] = op - DW_OP_lit0;
    } else if ( op >= DW_OP_breg0 && op <= DW_OP_breg31 ) {
      const i64 off = read_sleb128(p);
      const unsigned r = op - DW_OP_breg0;
      stk[sp++] = ctx.reg[r < DWARF_MAX_REG ? r : 0] + static_cast<usize>(off);
    } else if ( op >= DW_OP_reg0 && op <= DW_OP_reg31 ) {
      const unsigned r = op - DW_OP_reg0;
      stk[sp++] = ctx.reg[r < DWARF_MAX_REG ? r : 0];
    } else {
      switch ( op ) {
      case DW_OP_addr:
        stk[sp++] = read_unaligned<usize>(p);
        p += sizeof(usize);
        break;
      case DW_OP_const1u:
        stk[sp++] = *p++;
        break;
      case DW_OP_const1s:
        stk[sp++] = static_cast<usize>(static_cast<intptr_t>(static_cast<i8>(*p++)));
        break;
      case DW_OP_const2u:
        stk[sp++] = read_unaligned<u16>(p);
        p += 2;
        break;
      case DW_OP_const2s:
        stk[sp++] = static_cast<usize>(static_cast<intptr_t>(read_unaligned<i16>(p)));
        p += 2;
        break;
      case DW_OP_const4u:
        stk[sp++] = read_unaligned<u32>(p);
        p += 4;
        break;
      case DW_OP_const4s:
        stk[sp++] = static_cast<usize>(static_cast<intptr_t>(read_unaligned<i32>(p)));
        p += 4;
        break;
      case DW_OP_const8u:
      case DW_OP_const8s:
        stk[sp++] = static_cast<usize>(read_unaligned<u64>(p));
        p += 8;
        break;
      case DW_OP_constu:
        stk[sp++] = static_cast<usize>(read_uleb128(p));
        break;
      case DW_OP_consts:
        stk[sp++] = static_cast<usize>(read_sleb128(p));
        break;
      case DW_OP_dup:
        stk[sp] = stk[sp - 1];
        ++sp;
        break;
      case DW_OP_drop:
        --sp;
        break;
      case DW_OP_over:
        stk[sp] = stk[sp - 2];
        ++sp;
        break;
      case DW_OP_swap: {
        usize t = stk[sp - 1];
        stk[sp - 1] = stk[sp - 2];
        stk[sp - 2] = t;
        break;
      }
      case DW_OP_deref:
        stk[sp - 1] = read_unaligned<usize>(reinterpret_cast<const byte *>(stk[sp - 1]));
        break;
      case DW_OP_plus:
        stk[sp - 2] += stk[sp - 1];
        --sp;
        break;
      case DW_OP_minus:
        stk[sp - 2] -= stk[sp - 1];
        --sp;
        break;
      case DW_OP_and:
        stk[sp - 2] &= stk[sp - 1];
        --sp;
        break;
      case DW_OP_or:
        stk[sp - 2] |= stk[sp - 1];
        --sp;
        break;
      case DW_OP_shl:
        stk[sp - 2] <<= stk[sp - 1];
        --sp;
        break;
      case DW_OP_shr:
        stk[sp - 2] >>= stk[sp - 1];
        --sp;
        break;
      case DW_OP_plus_uconst:
        stk[sp - 1] += static_cast<usize>(read_uleb128(p));
        break;
      case DW_OP_regx:
        stk[sp++] = ctx.reg[read_uleb128(p) % DWARF_MAX_REG];
        break;
      case DW_OP_bregx: {
        const unsigned r = static_cast<unsigned>(read_uleb128(p));
        const i64 off = read_sleb128(p);
        stk[sp++] = ctx.reg[r < DWARF_MAX_REG ? r : 0] + static_cast<usize>(off);
        break;
      }
      case DW_OP_nop:
      default:
        break;
      }
    }
    if ( sp <= 0 || sp >= 63 ) break;      // guard
  }
  return sp > 0 ? stk[sp - 1] : 0;
}

// CFI rule table
enum rule_kind : u8 {
  rule_undefined = 0,      // unknown -> keep current value (callee preserved it)
  rule_same,
  rule_offset,             // value at *(CFA + off)
  rule_val_offset,         // value is CFA + off
  rule_register,           // value is reg[which]
  rule_expression,         // value at *(eval(expr, initial=CFA))
  rule_val_expression      // value is eval(expr, initial=CFA)
};

struct reg_rule {
  rule_kind kind;
  i64 off;
  u32 which;
  const byte *expr;
  const byte *expr_end;
};

struct cfi_row {
  bool cfa_is_expr;
  u32 cfa_reg;
  i64 cfa_off;
  const byte *cfa_expr;
  const byte *cfa_expr_end;
  reg_rule rules[DWARF_MAX_REG];
};

enum : u8 {
  DW_CFA_nop = 0x00,
  DW_CFA_set_loc = 0x01,
  DW_CFA_advance_loc1 = 0x02,
  DW_CFA_advance_loc2 = 0x03,
  DW_CFA_advance_loc4 = 0x04,
  DW_CFA_offset_extended = 0x05,
  DW_CFA_restore_extended = 0x06,
  DW_CFA_undefined = 0x07,
  DW_CFA_same_value = 0x08,
  DW_CFA_register = 0x09,
  DW_CFA_remember_state = 0x0a,
  DW_CFA_restore_state = 0x0b,
  DW_CFA_def_cfa = 0x0c,
  DW_CFA_def_cfa_register = 0x0d,
  DW_CFA_def_cfa_offset = 0x0e,
  DW_CFA_def_cfa_expression = 0x0f,
  DW_CFA_expression = 0x10,
  DW_CFA_offset_extended_sf = 0x11,
  DW_CFA_def_cfa_sf = 0x12,
  DW_CFA_def_cfa_offset_sf = 0x13,
  DW_CFA_val_offset = 0x14,
  DW_CFA_val_offset_sf = 0x15,
  DW_CFA_val_expression = 0x16,
  DW_CFA_GNU_args_size = 0x2e,
  DW_CFA_GNU_negative_offset_extended = 0x2f
};

// run the CIE+FDE CFI program up to pc_target
inline void
run_cfi(const fde_info &fde, usize pc_target, cfi_row &row) noexcept
{
  for ( int i = 0; i < DWARF_MAX_REG; ++i ) row.rules[i] = reg_rule{ rule_undefined, 0, 0, nullptr, nullptr };
  row.cfa_is_expr = false;
  row.cfa_reg = 0;
  row.cfa_off = 0;
  row.cfa_expr = nullptr;
  row.cfa_expr_end = nullptr;

  const u64 caf = fde.cie.code_align;
  const i64 daf = fde.cie.data_align;
  const decode_ctx ctx{ 0, __eh_state().text_base, __eh_state().data_base, fde.func_start };

  // remember/restore stack
  cfi_row stack[8];
  int stack_sp = 0;
  cfi_row initial;      // snapshot of rules after the CIE program (for DW_CFA_restore)
  bool have_initial = false;

  auto run = [&](const byte *p, const byte *end, usize start_loc) {
    usize loc = start_loc;
    while ( p < end ) {
      const u8 op = *p++;
      const u8 hi = op & 0xc0;
      if ( hi == 0x40 ) {      // DW_CFA_advance_loc
        loc += static_cast<usize>(op & 0x3f) * caf;
        if ( loc > pc_target ) return;
        continue;
      }
      if ( hi == 0x80 ) {      // DW_CFA_offset
        const u32 r = op & 0x3f;
        const u64 o = read_uleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_offset, static_cast<i64>(o) * daf, 0, nullptr, nullptr };
        continue;
      }
      if ( hi == 0xc0 ) {      // DW_CFA_restore
        const u32 r = op & 0x3f;
        if ( r < DWARF_MAX_REG ) row.rules[r] = have_initial ? initial.rules[r] : reg_rule{ rule_undefined, 0, 0, nullptr, nullptr };
        continue;
      }
      switch ( op ) {
      case DW_CFA_nop:
        break;
      case DW_CFA_set_loc: {
        const byte *q = p;
        loc = read_encoded(fde.cie.fde_enc, q, ctx);
        p = q;
        if ( loc > pc_target ) return;
        break;
      }
      case DW_CFA_advance_loc1:
        loc += static_cast<usize>(*p++) * caf;
        if ( loc > pc_target ) return;
        break;
      case DW_CFA_advance_loc2:
        loc += static_cast<usize>(read_unaligned<u16>(p)) * caf;
        p += 2;
        if ( loc > pc_target ) return;
        break;
      case DW_CFA_advance_loc4:
        loc += static_cast<usize>(read_unaligned<u32>(p)) * caf;
        p += 4;
        if ( loc > pc_target ) return;
        break;
      case DW_CFA_offset_extended: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const u64 o = read_uleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_offset, static_cast<i64>(o) * daf, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_offset_extended_sf: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const i64 o = read_sleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_offset, o * daf, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_GNU_negative_offset_extended: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const u64 o = read_uleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_offset, -static_cast<i64>(o) * daf, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_val_offset: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const u64 o = read_uleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_val_offset, static_cast<i64>(o) * daf, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_val_offset_sf: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const i64 o = read_sleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_val_offset, o * daf, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_restore_extended: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        if ( r < DWARF_MAX_REG ) row.rules[r] = have_initial ? initial.rules[r] : reg_rule{ rule_undefined, 0, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_undefined: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_undefined, 0, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_same_value: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_same, 0, 0, nullptr, nullptr };
        break;
      }
      case DW_CFA_register: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const u32 r2 = static_cast<u32>(read_uleb128(p));
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_register, 0, r2, nullptr, nullptr };
        break;
      }
      case DW_CFA_remember_state:
        if ( stack_sp < 8 ) stack[stack_sp++] = row;
        break;
      case DW_CFA_restore_state:
        if ( stack_sp > 0 ) {
          cfi_row saved = stack[--stack_sp];
          // restore only the register rules + CFA
          row.cfa_is_expr = saved.cfa_is_expr;
          row.cfa_reg = saved.cfa_reg;
          row.cfa_off = saved.cfa_off;
          row.cfa_expr = saved.cfa_expr;
          row.cfa_expr_end = saved.cfa_expr_end;
          for ( int i = 0; i < DWARF_MAX_REG; ++i ) row.rules[i] = saved.rules[i];
        }
        break;
      case DW_CFA_def_cfa: {
        row.cfa_is_expr = false;
        row.cfa_reg = static_cast<u32>(read_uleb128(p));
        row.cfa_off = static_cast<i64>(read_uleb128(p));
        break;
      }
      case DW_CFA_def_cfa_sf: {
        row.cfa_is_expr = false;
        row.cfa_reg = static_cast<u32>(read_uleb128(p));
        row.cfa_off = read_sleb128(p) * daf;
        break;
      }
      case DW_CFA_def_cfa_register:
        row.cfa_reg = static_cast<u32>(read_uleb128(p));
        row.cfa_is_expr = false;
        break;
      case DW_CFA_def_cfa_offset:
        row.cfa_off = static_cast<i64>(read_uleb128(p));
        row.cfa_is_expr = false;
        break;
      case DW_CFA_def_cfa_offset_sf:
        row.cfa_off = read_sleb128(p) * daf;
        row.cfa_is_expr = false;
        break;
      case DW_CFA_def_cfa_expression: {
        const u64 len = read_uleb128(p);
        row.cfa_is_expr = true;
        row.cfa_expr = p;
        row.cfa_expr_end = p + len;
        p += len;
        break;
      }
      case DW_CFA_expression: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const u64 len = read_uleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_expression, 0, 0, p, p + len };
        p += len;
        break;
      }
      case DW_CFA_val_expression: {
        const u32 r = static_cast<u32>(read_uleb128(p));
        const u64 len = read_uleb128(p);
        if ( r < DWARF_MAX_REG ) row.rules[r] = reg_rule{ rule_val_expression, 0, 0, p, p + len };
        p += len;
        break;
      }
      case DW_CFA_GNU_args_size:
        (void)read_uleb128(p);
        break;
      default:
        return;
      }
    }
  };

  run(fde.cie.cfi_begin, fde.cie.cfi_end, fde.func_start);
  initial = row;
  have_initial = true;
  run(fde.cfi_begin, fde.cfi_end, fde.func_start);
}

inline usize
compute_cfa(const cfi_row &row, const reg_context &reg) noexcept
{
  if ( row.cfa_is_expr ) return eval_dwarf_expr(row.cfa_expr, row.cfa_expr_end, reg, 0, false);
  return reg.reg[row.cfa_reg < DWARF_MAX_REG ? row.cfa_reg : 0] + static_cast<usize>(row.cfa_off);
}

inline usize
apply_rule(const reg_rule &r, const reg_context &reg, usize cfa, usize cur) noexcept
{
  switch ( r.kind ) {
  case rule_offset:
    return read_unaligned<usize>(reinterpret_cast<const byte *>(cfa + static_cast<usize>(r.off)));
  case rule_val_offset:
    return cfa + static_cast<usize>(r.off);
  case rule_register:
    return reg.reg[r.which < DWARF_MAX_REG ? r.which : 0];
  case rule_expression:
    return read_unaligned<usize>(reinterpret_cast<const byte *>(eval_dwarf_expr(r.expr, r.expr_end, reg, cfa, true)));
  case rule_val_expression:
    return eval_dwarf_expr(r.expr, r.expr_end, reg, cfa, true);
  case rule_same:
  case rule_undefined:
  default:
    return cur;
  }
}

inline bool
cfi_advance(reg_context &reg, const cfi_row &row, usize cfa, usize &pc) noexcept
{
  reg_context nx = reg;
  for ( int i = 0; i < DWARF_MAX_REG; ++i ) {
    if ( i == DWARF_SP_COLUMN ) continue;
    nx.reg[i] = apply_rule(row.rules[i], reg, cfa, reg.reg[i]);
  }
  const usize ra = apply_rule(row.rules[DWARF_RA_COLUMN], reg, cfa, 0);
  nx.reg[DWARF_SP_COLUMN] = cfa;
  nx.reg[DWARF_RA_COLUMN] = ra;

  reg = nx;
  pc = ra;
  return ra != 0;
}

inline bool
cfi_step(reg_context &reg, usize &pc, usize &out_cfa) noexcept
{
  fde_info fde;
  if ( !find_fde(pc - 1, fde) ) return false;
  cfi_row row;
  run_cfi(fde, pc - 1, row);
  out_cfa = compute_cfa(row, reg);
  return cfi_advance(reg, row, out_cfa, pc);
}

};      // namespace micron::eh

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// capture / install trampolines

#if defined(__micron_arch_amd64)
// reg[i] @ 8i: rbx=3(0x18) rbp=6(0x30) rsp=7(0x38) r12..r15=12..15(0x60..0x78)
// rax=0 rdx=1(0x08) rsi=4(0x20) RA=16(0x80). eh_return_data_regno 0=rax,1=rdx.
asm(R"asm(
	.text
	.weak __micron_eh_capture
	.type __micron_eh_capture, @function
__micron_eh_capture:
	movq %rbx, 0x18(%rdi)
	movq %rbp, 0x30(%rdi)
	leaq 8(%rsp), %rax
	movq %rax, 0x38(%rdi)
	movq %r12, 0x60(%rdi)
	movq %r13, 0x68(%rdi)
	movq %r14, 0x70(%rdi)
	movq %r15, 0x78(%rdi)
	movq (%rsp), %rax
	movq %rax, 0x80(%rdi)
	ret
	.weak __micron_eh_install_context
	.type __micron_eh_install_context, @function
__micron_eh_install_context:
	movq 0x80(%rdi), %rax
	movq 0x38(%rdi), %rcx
	subq $8, %rcx
	movq %rax, (%rcx)
	movq 0x18(%rdi), %rbx
	movq 0x30(%rdi), %rbp
	movq 0x60(%rdi), %r12
	movq 0x68(%rdi), %r13
	movq 0x70(%rdi), %r14
	movq 0x78(%rdi), %r15
	movq 0x20(%rdi), %rsi
	movq 0x08(%rdi), %rdx
	movq 0x00(%rdi), %rax
	movq %rcx, %rsp
	ret
)asm");

#elif defined(__micron_arch_x86)
// i386 cdecl: arg at [esp+4]. reg[i] @ 4i: ebx=3(12) esp=4(16) ebp=5(20)
// esi=6(24) edi=7(28) RA=8(32). eax=0 edx=2(8). eh_return_data_regno 0=eax,1=edx.
asm(R"asm(
	.text
	.weak __micron_eh_capture
	.type __micron_eh_capture, @function
__micron_eh_capture:
	mov 4(%esp), %eax
	mov %ebx, 12(%eax)
	mov %ebp, 20(%eax)
	mov %esi, 24(%eax)
	mov %edi, 28(%eax)
	lea 4(%esp), %ecx
	mov %ecx, 16(%eax)
	mov (%esp), %ecx
	mov %ecx, 32(%eax)
	ret
	.weak __micron_eh_install_context
	.type __micron_eh_install_context, @function
__micron_eh_install_context:
	mov 4(%esp), %ecx
	mov 16(%ecx), %eax
	sub $4, %eax
	mov 32(%ecx), %edx
	mov %edx, (%eax)
	mov 12(%ecx), %ebx
	mov 20(%ecx), %ebp
	mov 24(%ecx), %esi
	mov 28(%ecx), %edi
	mov %eax, %esp
	mov 0(%ecx), %eax
	mov 8(%ecx), %edx
	ret
)asm");

#elif defined(__micron_arch_arm64)
// reg[i] @ 8i: x19..x28=19..28(152..224) x29=29(232) x30/RA=30(240) sp=31(248)
// x0=0 x1=8. eh_return_data_regno 0=x0,1=x1. `bl` puts RA in lr (not on stack).
asm(R"asm(
	.text
	.weak __micron_eh_capture
	.type __micron_eh_capture, %function
__micron_eh_capture:
	stp x19, x20, [x0, #152]
	stp x21, x22, [x0, #168]
	stp x23, x24, [x0, #184]
	stp x25, x26, [x0, #200]
	stp x27, x28, [x0, #216]
	str x29, [x0, #232]
	str x30, [x0, #240]
	mov x9, sp
	str x9, [x0, #248]
	ret
	.weak __micron_eh_install_context
	.type __micron_eh_install_context, %function
__micron_eh_install_context:
	ldp x19, x20, [x0, #152]
	ldp x21, x22, [x0, #168]
	ldp x23, x24, [x0, #184]
	ldp x25, x26, [x0, #200]
	ldp x27, x28, [x0, #216]
	ldr x29, [x0, #232]
	ldr x30, [x0, #240]
	ldr x9, [x0, #248]
	mov sp, x9
	ldr x1, [x0, #8]
	ldr x0, [x0, #0]
	br x30
)asm");
#endif

#endif      // __micron_eh_dwarf
