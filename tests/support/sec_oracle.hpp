//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Oracle kit for micron::sec::seccomp.
//
// The filter builder emits classic BPF: a straight-line program of 8-byte instructions whose only
// control flow is a forward skip count. Its hard part is emit_pred(), which lowers a 64-bit
// comparison on a syscall argument into a ladder of up to 8 instructions that compare the two
// 32-bit halves in sequence. An off-by-one in ANY skip count there produces a filter that still
// loads, still looks plausible, and silently classifies some inputs wrong.
//
// Nothing about the emitted array reveals that. The only way to know what a program means is to
// execute it, so this header carries:
//
//   verify()  -- the kernel's own structural checks (jump targets in range, no out-of-bounds
//                packet load, program ends in a return). This is what seccomp(2) enforces at load
//                time, so a program failing it would be a bare EINVAL at runtime.
//   run()     -- a cBPF interpreter over the subset the builder emits, returning the action a real
//                kernel would take for a given syscall.
//
// A test then asks the same question twice: once by executing the compiled filter, once by
// evaluating the predicate directly in C++. They must agree for every input.

#include "../../src/linux/sys/bpf.hpp"
#include "../../src/linux/sys/seccomp.hpp"
#include "../../src/types.hpp"

namespace sec_oracle
{

using micron::bpf::insn_t;

// what the filter sees: the kernel's struct seccomp_data, which is the interpreter's "packet"
struct probe {
  i32 nr = 0;
  u32 arch = 0;
  u64 ip = 0;
  u64 args[6]{};
};

enum class fault : u8 {
  none = 0,
  bad_jump,          // a skip count lands past the end of the program
  oob_load,          // a load reaches outside the 64-byte seccomp_data
  fell_off_end,      // control ran past the last instruction without returning
  bad_opcode,        // an instruction the builder should never emit
};

struct outcome {
  u32 action = 0;
  fault err = fault::none;

  [[nodiscard]] constexpr bool
  ok() const noexcept
  {
    return err == fault::none;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the packet: struct seccomp_data is 64 bytes and every field is little-endian on our targets

constexpr usize data_size = 64;

[[nodiscard]] constexpr u32
load_word(const probe &p, u32 off) noexcept
{
  // nr @0, arch @4, ip @8, args[n] @ 16 + 8n -- each 64-bit field split lo/hi
  if ( off == 0 ) return static_cast<u32>(p.nr);
  if ( off == 4 ) return p.arch;
  if ( off == 8 ) return static_cast<u32>(p.ip & 0xffffffffu);
  if ( off == 12 ) return static_cast<u32>(p.ip >> 32);
  const u32 rel = off - 16;
  const u32 idx = rel / 8;
  return (rel % 8 == 0) ? static_cast<u32>(p.args[idx] & 0xffffffffu) : static_cast<u32>(p.args[idx] >> 32);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the structural pass the kernel runs before it will accept a filter

[[nodiscard]] constexpr fault
verify(const insn_t *prog, usize n) noexcept
{
  namespace b = micron::bpf;
  if ( n == 0 || n > b::max_instructions ) return fault::bad_jump;

  for ( usize i = 0; i < n; ++i ) {
    const u16 code = prog[i].code;
    const u16 cls = static_cast<u16>(code & 0x07);

    if ( cls == b::ld ) {
      if ( (code & 0xe0) != b::abs || (code & 0x18) != b::w ) return fault::bad_opcode;
      if ( static_cast<usize>(prog[i].k) + 4 > data_size ) return fault::oob_load;
    } else if ( cls == b::alu ) {
      if ( (code & 0x08) != b::src_k ) return fault::bad_opcode;
    } else if ( cls == b::jmp ) {
      const u16 op = static_cast<u16>(code & 0xf0);
      if ( op == b::op_ja ) {
        // an unconditional jump carries its distance in k, not jt/jf
        if ( i + 1 + static_cast<usize>(prog[i].k) >= n ) return fault::bad_jump;
      } else if ( op == b::op_jeq || op == b::op_jgt || op == b::op_jge || op == b::op_jset ) {
        if ( i + 1 + static_cast<usize>(prog[i].jt) >= n ) return fault::bad_jump;
        if ( i + 1 + static_cast<usize>(prog[i].jf) >= n ) return fault::bad_jump;
      } else {
        return fault::bad_opcode;
      }
    } else if ( cls == b::ret ) {
      continue;
    } else {
      return fault::bad_opcode;
    }
  }

  // a program must not be able to run off the end; the kernel requires the last insn be a return
  if ( (prog[n - 1].code & 0x07) != b::ret ) return fault::fell_off_end;
  return fault::none;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the interpreter

[[nodiscard]] constexpr outcome
run(const insn_t *prog, usize n, const probe &p) noexcept
{
  namespace b = micron::bpf;
  if ( const fault f = verify(prog, n); f != fault::none ) return outcome{ 0, f };

  u32 acc = 0;
  usize pc = 0;

  for ( usize steps = 0; steps <= b::max_instructions; ++steps ) {
    if ( pc >= n ) return outcome{ 0, fault::fell_off_end };
    const insn_t in = prog[pc];
    const u16 cls = static_cast<u16>(in.code & 0x07);

    if ( cls == b::ret ) {
      // the builder only ever emits BPF_RET|BPF_K
      return outcome{ in.k, fault::none };
    }

    if ( cls == b::ld ) {
      acc = load_word(p, in.k);
      ++pc;
      continue;
    }

    if ( cls == b::alu ) {
      switch ( in.code & 0xf0 ) {
      case b::op_and: acc &= in.k; break;
      case b::op_or: acc |= in.k; break;
      case b::op_xor: acc ^= in.k; break;
      case b::op_add: acc += in.k; break;
      case b::op_sub: acc -= in.k; break;
      case b::op_lsh: acc <<= (in.k & 31); break;
      case b::op_rsh: acc >>= (in.k & 31); break;
      default: return outcome{ 0, fault::bad_opcode };
      }
      ++pc;
      continue;
    }

    // jmp: all comparisons against k are UNSIGNED, which is why a signed syscall number still
    // orders correctly in a range check
    const u16 op = static_cast<u16>(in.code & 0xf0);
    if ( op == b::op_ja ) {
      pc += 1 + static_cast<usize>(in.k);
      continue;
    }

    bool taken = false;
    switch ( op ) {
    case b::op_jeq: taken = (acc == in.k); break;
    case b::op_jgt: taken = (acc > in.k); break;
    case b::op_jge: taken = (acc >= in.k); break;
    case b::op_jset: taken = ((acc & in.k) != 0); break;
    default: return outcome{ 0, fault::bad_opcode };
    }
    pc += 1 + static_cast<usize>(taken ? in.jt : in.jf);
  }

  return outcome{ 0, fault::fell_off_end };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the independent predicate. this is the "what SHOULD happen" half -- deliberately written in
// plain 64-bit C++ with no reference to the lowering, so it cannot share a bug with it

enum class cmp_kind : u8 { eq, ne, lt, le, gt, ge, masked };

[[nodiscard]] constexpr bool
predicate(cmp_kind k, u64 arg, u64 val, u64 mask = 0) noexcept
{
  switch ( k ) {
  case cmp_kind::eq: return arg == val;
  case cmp_kind::ne: return arg != val;
  case cmp_kind::lt: return arg < val;
  case cmp_kind::le: return arg <= val;
  case cmp_kind::gt: return arg > val;
  case cmp_kind::ge: return arg >= val;
  case cmp_kind::masked: return (arg & mask) == (val & mask);
  }
  return false;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a fixed-seed xorshift. NEVER seed a rigor test from the clock

struct rng {
  u64 s;

  constexpr explicit rng(u64 seed) noexcept : s(seed ? seed : 0x9E3779B97F4A7C15ull) { }

  constexpr u64
  next() noexcept
  {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  }

  constexpr u32
  below(u32 n) noexcept
  {
    return n ? static_cast<u32>(next() % n) : 0u;
  }

  // a value drawn to sit ON the interesting boundaries, not uniformly in 2^64: half-word edges,
  // sign flips, and neighbours of the comparand are where a lo/hi ladder actually breaks
  constexpr u64
  edgy(u64 around) noexcept
  {
    switch ( below(10) ) {
    case 0: return 0;
    case 1: return ~0ull;
    case 2: return 0xffffffffull;
    case 3: return 0x100000000ull;
    case 4: return around;
    case 5: return around ? around - 1 : 0;
    case 6: return around + 1;
    case 7: return around ^ 0xffffffffull;          // flip the low half only
    case 8: return around ^ 0xffffffff00000000ull;  // flip the high half only
    default: return next();
    }
  }
};

};      // namespace sec_oracle
