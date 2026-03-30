//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "prctl.hpp"

namespace micron
{

namespace bpf
{

// instruction classes (BPF_CLASS)
constexpr static const u16 ld = 0x00;
constexpr static const u16 ldx = 0x01;
constexpr static const u16 st = 0x02;
constexpr static const u16 stx = 0x03;
constexpr static const u16 alu = 0x04;
constexpr static const u16 jmp = 0x05;
constexpr static const u16 ret = 0x06;
constexpr static const u16 misc = 0x07;

// size modifiers (BPF_SIZE)
constexpr static const u16 w = 0x00;     // 32-bit word
constexpr static const u16 h = 0x08;     // 16-bit halfword
constexpr static const u16 b = 0x10;     // 8-bit byte

// addressing modes (BPF_MODE)
constexpr static const u16 imm = 0x00;
constexpr static const u16 abs = 0x20;
constexpr static const u16 ind = 0x40;
constexpr static const u16 mem = 0x60;
constexpr static const u16 len = 0x80;
constexpr static const u16 msh = 0xa0;

// ALU operations (BPF_OP for BPF_ALU)
constexpr static const u16 op_add = 0x00;
constexpr static const u16 op_sub = 0x10;
constexpr static const u16 op_mul = 0x20;
constexpr static const u16 op_div = 0x30;
constexpr static const u16 op_or = 0x40;
constexpr static const u16 op_and = 0x50;
constexpr static const u16 op_lsh = 0x60;
constexpr static const u16 op_rsh = 0x70;
constexpr static const u16 op_neg = 0x80;
constexpr static const u16 op_mod = 0x90;
constexpr static const u16 op_xor = 0xa0;

// jump operations (BPF_OP for BPF_JMP)
constexpr static const u16 op_ja = 0x00;       // unconditional jump
constexpr static const u16 op_jeq = 0x10;      // jump if A == K
constexpr static const u16 op_jgt = 0x20;      // jump if A >  K  (unsigned)
constexpr static const u16 op_jge = 0x30;      // jump if A >= K  (unsigned)
constexpr static const u16 op_jset = 0x40;     // jump if A &  K

// source operands
constexpr static const u16 src_k = 0x00;     // immediate constant
constexpr static const u16 src_x = 0x08;     // index register X

// return operand sources
constexpr static const u16 retsrc_k = 0x00;     // return K field
constexpr static const u16 retsrc_a = 0x10;     // return A register

// kernel-imposed maximum program length (BPF_MAXINSNS)
constexpr static const usize max_instructions = 4096u;

struct insn_t {
  u16 code;
  u8 jt;     // instructions to skip on true branch
  u8 jf;     // instructions to skip on false branch
  u32 k;     // immediate / offset operand
};

// BPF_STMT equivalent: instruction with no jump
[[nodiscard]] constexpr insn_t
stmt(u16 code, u32 kval) noexcept
{
  return insn_t{ code, 0, 0, kval };
}

// BPF_JUMP equivalent: conditional jump
[[nodiscard]] constexpr insn_t
jump(u16 code, u32 kval, u8 jt, u8 jf) noexcept
{
  return insn_t{ code, jt, jf, kval };
}

struct fprog_t {
  u16 len;
  insn_t *filter;
};

};     // namespace bpf

};     // namespace micron
