//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../errno.hpp"
#include "../linux/sys/prctl.hpp"
#include "../syscall.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../linux/sys/bpf.hpp"
#include "../linux/sys/ioctl.hpp"
#include "../linux/sys/seccomp.hpp"

#include "bits.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// seccomp-bpf porcelain

namespace micron
{
namespace sec
{
namespace seccomp
{

constexpr static const u32 audit_arch_64bit = 0x80000000u;
constexpr static const u32 audit_arch_le = 0x40000000u;

enum class arch : u32 {
  // LE 32-bit
  x86 = audit_arch_le | 0x03u,      // EM_386=3
  arm = audit_arch_le | 0x28u,      // EM_ARM=40
  // BE 32-bit
  m68k = 0x04u,         // EM_68K=4
  mips_be = 0x08u,      // EM_MIPS=8, big-endian
  ppc = 0x14u,          // EM_PPC=20
  s390 = 0x16u,         // EM_S390=22
  // LE 64-bit
  x86_64 = audit_arch_64bit | audit_arch_le | 0x3Eu,            // EM_X86_64=62
  x32 = audit_arch_le | 0x3Eu,                                  // x32 ABI (64-bit EM, 32-bit pointers)
  aarch64 = audit_arch_64bit | audit_arch_le | 0xB7u,           // EM_AARCH64=183
  loongarch64 = audit_arch_64bit | audit_arch_le | 0x102u,      // EM_LOONGARCH=258
  mipsel = audit_arch_le | 0x08u,
  mipsel64 = audit_arch_64bit | audit_arch_le | 0x08u,
  ppc64le = audit_arch_64bit | audit_arch_le | 0x15u,      // EM_PPC64=21
  riscv64 = audit_arch_64bit | audit_arch_le | 0xF3u,      // EM_RISCV=243
  // BE 64-bit
  mips64_be = audit_arch_64bit | 0x08u,
  ppc64 = audit_arch_64bit | 0x15u,
  s390x = audit_arch_64bit | 0x16u,
};

#if defined(__micron_arch_amd64)
constexpr static const arch native_arch = arch::x86_64;
#elif defined(__micron_arch_x86)
constexpr static const arch native_arch = arch::x86;
#elif defined(__micron_arch_arm64)
constexpr static const arch native_arch = arch::aarch64;
#elif defined(__micron_arch_arm32)
constexpr static const arch native_arch = arch::arm;
#else
#error "Unsupported architecture. Is your compiler working properly?"
#endif

constexpr u32
act_kill_process() noexcept
{
  return posix::seccomp_ret_kill_process;
}

constexpr u32
act_kill_thread() noexcept
{
  return posix::seccomp_ret_kill_thread;
}

constexpr u32
act_kill() noexcept
{
  return posix::seccomp_ret_kill_process;
}

constexpr u32
act_trap() noexcept
{
  return posix::seccomp_ret_trap;
}

constexpr u32
act_log() noexcept
{
  return posix::seccomp_ret_log;
}

constexpr u32
act_allow() noexcept
{
  return posix::seccomp_ret_allow;
}

constexpr u32
act_notify() noexcept
{
  return posix::seccomp_ret_user_notif;
}

constexpr u32
act_errno(u16 err) noexcept
{
  return posix::seccomp_ret_errno | (static_cast<u32>(err) & posix::seccomp_ret_data);
}

constexpr u32
act_trace(u16 msg) noexcept
{
  return posix::seccomp_ret_trace | (static_cast<u32>(msg) & posix::seccomp_ret_data);
}

enum class cmp : u32 {
  ne = 1u,             // arg != datum_a
  lt = 2u,             // arg <  datum_a  (unsigned)
  le = 3u,             // arg <= datum_a  (unsigned)
  eq = 4u,             // arg == datum_a
  ge = 5u,             // arg >= datum_a  (unsigned)
  gt = 6u,             // arg >  datum_a  (unsigned)
  masked_eq = 7u,      // (arg & datum_b) == (datum_a & datum_b)
};

struct arg_cmp_t {
  u32 arg;      // argument index 0..5
  cmp op;
  u64 datum_a;      // comparison value  (or target value for masked_eq)
  u64 datum_b;      // mask (only used for masked_eq)
};

constexpr arg_cmp_t
arg_eq(u32 n, u64 val) noexcept
{
  return { n, cmp::eq, val, 0 };
}

constexpr arg_cmp_t
arg_ne(u32 n, u64 val) noexcept
{
  return { n, cmp::ne, val, 0 };
}

constexpr arg_cmp_t
arg_lt(u32 n, u64 val) noexcept
{
  return { n, cmp::lt, val, 0 };
}

constexpr arg_cmp_t
arg_le(u32 n, u64 val) noexcept
{
  return { n, cmp::le, val, 0 };
}

constexpr arg_cmp_t
arg_gt(u32 n, u64 val) noexcept
{
  return { n, cmp::gt, val, 0 };
}

constexpr arg_cmp_t
arg_ge(u32 n, u64 val) noexcept
{
  return { n, cmp::ge, val, 0 };
}

constexpr arg_cmp_t
arg_masked(u32 n, u64 mask, u64 val) noexcept
{
  return { n, cmp::masked_eq, val, mask };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NARROWED ARGUMENTS -- read this before writing arg_eq() on a selector
//
// seccomp compares the full 64-bit register the argument arrived in. Many syscalls do not: their C
// prototype narrows the parameter to `int` or `unsigned int`, and the high 32 bits are discarded
// before any handler sees them. For those, a 64-bit compare and the kernel DISAGREE about what the
// argument is, and the disagreement is an argument the attacker controls:
//
//     deny_if(SYS_ioctl, arg_eq(1, tiocsti))            <- ioctl(fd, TIOCSTI | 1ull<<32) walks past
//     deny_if(SYS_ioctl, arg_eq32(1, tiocsti))          <- cannot
//
// That is CVE-2019-10063 exactly: Flatpak blocked TIOCSTI with a 64-bit equality and the block was
// bypassable by setting any bit above 31. The fix upstream, and the fix here, is to compare only the
// width the kernel reads.
//
// The arguments that narrow to 32 bits, and which therefore want the *32 forms:
//
//     ioctl        arg 1  (unsigned int cmd)          fcntl      arg 1  (unsigned int cmd)
//     prctl        arg 0  (int option)                personality arg 0 (unsigned int)
//     socket       args 0,1,2 (int family/type/proto) socketpair args 0,1,2
//     openat       arg 2  (int flags)                 keyctl     arg 0  (int operation)
//     madvise      arg 2  (int advice)                mmap       args 2,3 (int prot/flags)
//     setsockopt   args 1,2 (int level/optname)       shmget/semget/msgget  flag args
//     seccomp      args 0,1 (unsigned int op/flags)   ptrace     arg 0  (long request -- NOT narrowed)
//
// The ones that genuinely are 64 bits -- clone's flags, mmap's addr and length, every pointer -- want
// plain arg_eq/arg_masked, and using a *32 form on them would be the mirror-image mistake: it would
// ignore the high half of a value the kernel reads in full.
//
// tests/adv/adv_cve_2019_10063_tiocsti.cpp drives both spellings against the oracle and requires
// them to disagree on the decorated value; that disagreement is what makes this comment load-bearing
// rather than advice.

constexpr u64 narrow32_mask = 0xFFFFFFFFull;

// arg == val, comparing only the low 32 bits -- the width the kernel reads for a narrowed selector
constexpr arg_cmp_t
arg_eq32(u32 n, u32 val) noexcept
{
  return { n, cmp::masked_eq, static_cast<u64>(val), narrow32_mask };
}

// (arg & mask) == (val & mask), with the mask clamped to the low word so a caller cannot accidentally
// reintroduce a high-half comparison on an argument the kernel truncates
constexpr arg_cmp_t
arg_masked32(u32 n, u32 mask, u32 val) noexcept
{
  return { n, cmp::masked_eq, static_cast<u64>(val), static_cast<u64>(mask) };
}

// (arg & bits) != 0, over the low word. There is no cmp:: for "any of these bits set", so this is
// the negative form: it matches when NONE are set, and the caller pairs it with a following rule for
// the syscall. See policy.hpp's no_new_namespaces for the pattern.
constexpr arg_cmp_t
arg_no_bits32(u32 n, u32 bits) noexcept
{
  return { n, cmp::masked_eq, 0ull, static_cast<u64>(bits) };
}

// the same, at full width -- for clone(2), whose flags really are an unsigned long
constexpr arg_cmp_t
arg_no_bits(u32 n, u64 bits) noexcept
{
  return { n, cmp::masked_eq, 0ull, bits };
}

constexpr u32
msw(u64 v) noexcept
{
  return static_cast<u32>((v >> 32) & 0xFFFFFFFFu);
}

constexpr u32
lsw(u64 v) noexcept
{
  return static_cast<u32>(v & 0xFFFFFFFFu);
}

// NOTE: seccomp_data.args[n] is a u64 at byte offset 16 + n*8; the MSW/LSW split is endian
// dependent: little-endian stores the low word first (base+0) and the high word at base+4;
// big-endian is the reverse
constexpr bpf::insn_t
load_arg_msw(u32 n) noexcept
{
  const u32 base = 16u + n * 8u;
#if defined(__micron_endian_big)
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, base);      // big-endian: MSW at base+0
#else
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, base + 4u);      // little-endian: MSW at base+4
#endif
}

constexpr bpf::insn_t
load_arg_lsw(u32 n) noexcept
{
  const u32 base = 16u + n * 8u;
#if defined(__micron_endian_big)
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, base + 4u);      // big-endian: LSW at base+4
#else
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, base);      // little-endian: LSW at base+0
#endif
}

constexpr bpf::insn_t
load_syscall_nr() noexcept
{
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, posix::seccomp_data_nr_off);
}

constexpr bpf::insn_t
load_arch() noexcept
{
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, posix::seccomp_data_arch_off);
}

constexpr bpf::insn_t
ret_k(u32 action) noexcept
{
  return bpf::stmt(bpf::ret | bpf::src_k, action);
}

constexpr bpf::insn_t
jeq_k(u32 val, u8 jt, u8 jf = 0) noexcept
{
  return bpf::jump(bpf::jmp | bpf::op_jeq | bpf::src_k, val, jt, jf);
}

constexpr bpf::insn_t
jgt_k(u32 val, u8 jt, u8 jf = 0) noexcept
{
  return bpf::jump(bpf::jmp | bpf::op_jgt | bpf::src_k, val, jt, jf);
}

constexpr bpf::insn_t
jge_k(u32 val, u8 jt, u8 jf = 0) noexcept
{
  return bpf::jump(bpf::jmp | bpf::op_jge | bpf::src_k, val, jt, jf);
}

constexpr bpf::insn_t
jset_k(u32 mask, u8 jt, u8 jf = 0) noexcept
{
  return bpf::jump(bpf::jmp | bpf::op_jset | bpf::src_k, mask, jt, jf);
}

constexpr bpf::insn_t
and_k(u32 mask) noexcept
{
  return bpf::stmt(bpf::alu | bpf::op_and | bpf::src_k, mask);
}

constexpr usize
pred_insns(cmp op) noexcept
{
  // NOTE: valid for now, must be fixed if additional inst added later
  switch ( op ) {
  case cmp::eq:
  case cmp::ne:
    return 5u;
  case cmp::gt:
  case cmp::ge:
    return 6u;
  case cmp::lt:
  case cmp::le:
  case cmp::masked_eq:
    return 7u;
  }
  return 0u;
}

template<usize Max = 128>
  requires(Max > 0 and Max <= bpf::max_instructions)      // guard against kernel limit
struct filter_builder {

  bpf::insn_t insns[Max]{};
  usize count = 0;
  bool arch_ok = false;           // set by require_arch / require_native_arch
  bool native_gated = false;      // set ONLY by require_native_arch: the gate INCLUDING the x32 deny
  bool sealed = false;            // set by default_* methods
  bool overflowed = false;

  constexpr usize
  remaining() const noexcept
  {
    return Max - count;
  }

  // the room a RULE may claim. one slot is always held back for the default action while the
  // builder is unsealed.
  //
  // WARNING: this reserve is the whole reason `overflowed` can be trusted. A rule that consumed
  // the last slot would leave the seal unable to push -- which is the ONLY condition the old
  // push()-side flag could see. Truncations that stopped 1 or 2 slots short still sealed, still
  // ended in a return, still passed valid(), and installed a policy missing rules the author
  // wrote: fail-OPEN wherever the surviving default allows.
  constexpr usize
  rule_room() const noexcept
  {
    const usize r = Max - count;
    if ( sealed ) return r;
    return r >= 1 ? r - 1 : 0;
  }

  constexpr bool
  full() const noexcept
  {
    return count >= Max;
  }

private:
  constexpr filter_builder &
  push(const bpf::insn_t &i) noexcept
  {
    if ( count < Max )
      insns[count++] = i;
    else
      overflowed = true;
    return *this;
  }

  // [I] = instruction at offset I from block start (0-based)
  // PC  = next instruction after a jump from [I] with skip S: I + 1 + S
  constexpr void
  emit_pred(const arg_cmp_t &ac, u32 action) noexcept
  {
    const u32 val_msw = msw(ac.datum_a);
    const u32 val_lsw = lsw(ac.datum_a);

    switch ( ac.op ) {

    case cmp::eq:
      push(load_arg_lsw(ac.arg));
      push(jeq_k(val_lsw, 0, 3));
      push(load_arg_msw(ac.arg));
      push(jeq_k(val_msw, 0, 1));
      push(ret_k(action));
      break;

    case cmp::ne:
      push(load_arg_lsw(ac.arg));      // [0]
      push(jeq_k(val_lsw, 0, 2));      // [1] lsw==val -> check msw [2]; lsw!=val -> action [4]
      push(load_arg_msw(ac.arg));      // [2]
      push(jeq_k(val_msw, 1, 0));      // [3] msw==val -> skip [5] (no action); msw!=val -> action [4]
      push(ret_k(action));             // [4]
      break;

    case cmp::lt:
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 5, 0));
      push(jeq_k(val_msw, 1, 0));
      push(ret_k(action));
      push(load_arg_lsw(ac.arg));
      push(jge_k(val_lsw, 1, 0));
      push(ret_k(action));
      break;

    case cmp::le:
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 5, 0));
      push(jeq_k(val_msw, 1, 0));
      push(ret_k(action));
      push(load_arg_lsw(ac.arg));
      push(jgt_k(val_lsw, 1, 0));
      push(ret_k(action));
      break;

    case cmp::gt:
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 3, 0));
      push(jeq_k(val_msw, 0, 3));
      push(load_arg_lsw(ac.arg));
      push(jgt_k(val_lsw, 0, 1));
      push(ret_k(action));
      break;

    case cmp::ge:
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 3, 0));
      push(jeq_k(val_msw, 0, 3));
      push(load_arg_lsw(ac.arg));
      push(jge_k(val_lsw, 0, 1));
      push(ret_k(action));
      break;

    case cmp::masked_eq: {
      const u32 mask_msw = msw(ac.datum_b);
      const u32 mask_lsw = lsw(ac.datum_b);
      const u32 cmp_lsw = val_lsw & mask_lsw;      // (datum_a & datum_b) low half
      const u32 cmp_msw = val_msw & mask_msw;      // (datum_a & datum_b) high half
      push(load_arg_lsw(ac.arg));
      push(and_k(mask_lsw));
      push(jeq_k(cmp_lsw, 0, 4));
      push(load_arg_msw(ac.arg));
      push(and_k(mask_msw));
      push(jeq_k(cmp_msw, 0, 1));
      push(ret_k(action));
      break;
    }
    }
  }

public:
  // NOTE: if arch does not match the running kernel the filter kills the entire process immediately
  //
  // WARNING: the gate MUST be the first thing the program does. Emitted after any other rule it is
  // reached only when nothing above it returned, and emitted after a seal it is never reached at
  // all -- either way the filter carries no effective arch check while arch_ok would claim one, so
  // a foreign-ABI task walks straight through a syscall ladder written for numbers it does not
  // use. `count != 0` is refused for exactly that reason, and it is refused LOUDLY: a silently
  // ungated filter is the fail-open case.
  constexpr filter_builder &
  require_arch(arch a) noexcept
  {
    if ( arch_ok ) return *this;      // already gated at [0]; a second gate is unreachable anyway
    if ( sealed || count != 0 || rule_room() < 3 ) {
      overflowed = true;
      return *this;
    }
    push(load_arch());
    push(jeq_k(static_cast<u32>(a), 1, 0));
    push(ret_k(posix::seccomp_ret_kill_process));
    arch_ok = true;
    return *this;
  }

  // NOTE: no x32 guard. A filter built with this is bypassable on amd64 through the x32 entry
  // (nr | 0x40000000); it exists to test that difference, not to be used
  constexpr filter_builder &
  require_native_arch_raw() noexcept
  {
    return require_arch(native_arch);
  }

  constexpr filter_builder &
  require_native_arch() noexcept
  {
    // WARNING: a gate is already here, and it is NOT this one. require_arch() sets arch_ok without
    // the x32 deny, so returning quietly would hand back a builder whose gate is weaker than the one
    // the caller just asked for by name -- and valid() would say yes, because the structural check
    // only looks at insns[0]. Refuse loudly instead. A second call to require_native_arch itself is
    // still a no-op, which is what native_gated distinguishes.
    if ( arch_ok ) {
      if ( !native_gated ) overflowed = true;
      return *this;
    }
#if defined(__micron_arch_amd64)
    // the x32 guard is PART of the gate, not a decoration: AUDIT_ARCH_X86_64 is what an x32 task
    // reports too, so without the negative-number range deny the compare passes and the syscall
    // ladder is evaluated against x32 numbers. Both halves land or neither does -- 3 + 4 slots.
    if ( sealed || count != 0 || rule_room() < 7 ) {
      overflowed = true;
      return *this;
    }
    require_arch(native_arch);
    deny_range(static_cast<i32>(0x40000000), static_cast<i32>(0x7FFFFFFF), posix::seccomp_ret_kill_process);
#else
    require_arch(native_arch);
#endif
    native_gated = true;
    return *this;
  }

  constexpr filter_builder &
  allow(i32 nr) noexcept
  {
    if ( sealed ) return *this;      // fail-CLOSED: a rule after the default is only ever a widening
    if ( rule_room() < 3 ) {
      overflowed = true;
      return *this;
    }
    push(load_syscall_nr());
    push(jeq_k(static_cast<u32>(nr), 0, 1));
    push(ret_k(posix::seccomp_ret_allow));
    return *this;
  }

  constexpr filter_builder &
  deny(i32 nr, u32 action = posix::seccomp_ret_kill_process) noexcept
  {
    if ( sealed ) return *this;
    if ( rule_room() < 3 ) {
      overflowed = true;
      return *this;
    }
    push(load_syscall_nr());
    push(jeq_k(static_cast<u32>(nr), 0, 1));
    push(ret_k(action));
    return *this;
  }

  constexpr filter_builder &
  deny_errno(i32 nr, u16 err) noexcept
  {
    return deny(nr, act_errno(err));
  }

  constexpr filter_builder &
  trap_syscall(i32 nr) noexcept
  {
    return deny(nr, posix::seccomp_ret_trap);
  }

  template<i32... Nrs>
    requires(sizeof...(Nrs) >= 1 and sizeof...(Nrs) <= 256)
  constexpr filter_builder &
  allow_batch() noexcept
  {
    if ( sealed ) return *this;
    constexpr usize N = sizeof...(Nrs);
    if ( rule_room() < N + 2 ) {
      overflowed = true;
      return *this;
    }

    push(load_syscall_nr());

    usize i = 0;
    (
        [&] {
          const u8 jt = static_cast<u8>(N - 1 - i);
          const u8 jf = static_cast<u8>((i == N - 1) ? 1u : 0u);
          push(jeq_k(static_cast<u32>(Nrs), jt, jf));
          ++i;
        }(),
        ...);

    push(ret_k(posix::seccomp_ret_allow));
    return *this;
  }

  constexpr filter_builder &
  allow_range(i32 lo_nr, i32 hi_nr) noexcept
  {
    if ( sealed ) return *this;
    if ( rule_room() < 4 ) {
      overflowed = true;
      return *this;
    }
    push(load_syscall_nr());
    push(jge_k(static_cast<u32>(lo_nr), 0, 2));
    push(jgt_k(static_cast<u32>(hi_nr), 1, 0));
    push(ret_k(posix::seccomp_ret_allow));
    return *this;
  }

  constexpr filter_builder &
  deny_range(i32 lo_nr, i32 hi_nr, u32 action = posix::seccomp_ret_kill_process) noexcept
  {
    if ( sealed ) return *this;
    if ( rule_room() < 4 ) {
      overflowed = true;
      return *this;
    }
    push(load_syscall_nr());
    push(jge_k(static_cast<u32>(lo_nr), 0, 2));
    push(jgt_k(static_cast<u32>(hi_nr), 1, 0));
    push(ret_k(action));
    return *this;
  }

  constexpr filter_builder &
  action_if(i32 nr, const arg_cmp_t &ac, u32 action) noexcept
  {
    if ( sealed ) return *this;
    const usize psize = pred_insns(ac.op);
    // psize == 0 means an op outside the enum; emitting nothing for it would drop an argument
    // predicate the author wrote, which is the same fail-open as a capacity drop
    if ( psize == 0 || rule_room() < psize + 2 ) {
      overflowed = true;
      return *this;
    }
    push(load_syscall_nr());
    push(jeq_k(static_cast<u32>(nr), 0, static_cast<u8>(psize)));
    emit_pred(ac, action);
    return *this;
  }

  constexpr filter_builder &
  allow_if(i32 nr, const arg_cmp_t &ac) noexcept
  {
    return action_if(nr, ac, posix::seccomp_ret_allow);
  }

  constexpr filter_builder &
  deny_if(i32 nr, const arg_cmp_t &ac, u32 action = posix::seccomp_ret_kill_process) noexcept
  {
    return action_if(nr, ac, action);
  }

  constexpr filter_builder &
  deny_if_errno(i32 nr, const arg_cmp_t &ac, u16 err) noexcept
  {
    return action_if(nr, ac, act_errno(err));
  }

  constexpr filter_builder &
  default_kill() noexcept
  {
    push(ret_k(posix::seccomp_ret_kill_process));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  default_kill_thread() noexcept
  {
    push(ret_k(posix::seccomp_ret_kill_thread));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  default_trap() noexcept
  {
    push(ret_k(posix::seccomp_ret_trap));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  default_errno(u16 err) noexcept
  {
    push(ret_k(act_errno(err)));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  default_log() noexcept
  {
    push(ret_k(posix::seccomp_ret_log));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  default_allow() noexcept
  {
    push(ret_k(posix::seccomp_ret_allow));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  default_notify() noexcept
  {
    push(ret_k(posix::seccomp_ret_user_notif));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  __seal(u32 action) noexcept
  {
    push(ret_k(action));
    sealed = true;
    return *this;
  }

  constexpr filter_builder &
  finalize() noexcept
  {
    if ( !sealed ) default_kill();
    return *this;
  }

  constexpr bool
  valid() const noexcept
  {
    if ( !arch_ok ) return false;
    if ( overflowed ) return false;
    if ( count == 0 || !sealed ) return false;
    // structural, not a flag: the program's FIRST act must be the load of seccomp_data.arch. This
    // is what actually pins the gate to [0]; arch_ok alone is a claim about the builder, and a
    // claim is not what the kernel executes
    if ( insns[0].code != static_cast<u16>(bpf::ld | bpf::w | bpf::abs) ) return false;
    if ( insns[0].k != posix::seccomp_data_arch_off ) return false;
    const u16 last_class = insns[count - 1].code & 0x07u;
    return last_class == bpf::ret;
  }

  bpf::fprog_t
  prog() noexcept
  {
    // NOTE: seals an unsealed builder, and does NOT and CANNOT retrofit an arch gate -- an
    // AUDIT_ARCH compare has to sit at [0], long before prog() is reached. Callers gate on
    // valid(); prog() is not a sanitizer
    if ( !sealed ) finalize();
    return bpf::fprog_t{ static_cast<u16>(count), insns };
  }
};

template<usize N>
inline int
load(filter_builder<N> &fb, bool set_no_new_privs = true, u32 extra_flags = 0)
{
  if ( set_no_new_privs ) {
    i32 __nnp = micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
    if ( __nnp < 0 ) return __nnp;      // NNP must succeed before SET_MODE_FILTER (else EACCES); was discarded
  }
  auto p = fb.prog();
  // WARNING: refuse to install a filter without a native-arch guard (require_native_arch())
  if ( !fb.valid() ) return -error::invalid_arg;
  return posix::seccomp_load_filter(p, extra_flags);
}

// SECCOMP_SET_MODE_FILTER confines the CALLING THREAD. In a process that has already started a
// worker, a coroutine runtime or an io_uring reactor, that is one thread of several -- and the
// siblings share the address space, so an unconfined sibling is not a partial bypass but a total one.
// TSYNC makes the install atomic across the thread group.
//
// WARNING: A PARTIAL TSYNC RETURNS A POSITIVE NUMBER, NOT AN ERROR. When the kernel cannot
// synchronise a thread it answers with that thread's TID, so a caller testing `r < 0` reads a
// half-applied filter as a clean install and runs on believing every thread is covered.
// SECCOMP_FILTER_FLAG_TSYNC_ESRCH turns that into -ESRCH; it is requested where the kernel has it
// (5.19+), and the positive return is mapped anyway so an older kernel cannot report it as success.
template<usize N>
inline int
load_tsync(filter_builder<N> &fb, bool set_no_new_privs = true)
{
  const int r = load(fb, set_no_new_privs, posix::seccomp_filter_flag_tsync | posix::seccomp_filter_flag_tsync_esrch);
  if ( r == -error::invalid_arg ) {
    // TSYNC_ESRCH is 5.19+; an older kernel rejects the unknown flag. Retry without it and map the
    // positive answer ourselves -- the diagnosis is coarser (we do not learn WHICH thread) but the
    // verdict is the same, and the verdict is the part that must not be lost.
    const int plain = load(fb, false, posix::seccomp_filter_flag_tsync);
    return plain > 0 ? -error::no_process : plain;
  }
  return r > 0 ? -error::no_process : r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tty injection
//
// TIOCSTI pushes a byte into the input queue of a terminal the caller has a descriptor to -- so a
// sandbox holding the launching shell's tty on fd 0 (which sandbox::stdio() exists to arrange) can
// type into that shell, outside the sandbox, with the launcher's privileges. TIOCLINUX does the same
// through a console selection paste, and TIOCSETD swaps the line discipline out from under the
// terminal. This is CVE-2019-10063.
//
// Every rule here is a *32 comparison, for the reason the banner above gives: ioctl's cmd is an
// `unsigned int` and a 64-bit compare is bypassable by decorating the high half.
//
// NOTE: dev.tty.legacy_tiocsti defaults to 0 on 6.2+, which disables TIOCSTI in the kernel. That is
// the host's choice and can be turned back on; a library cannot rely on it.
template<usize N>
constexpr filter_builder<N> &
deny_tty_injection(filter_builder<N> &fb, u16 err = static_cast<u16>(error::permissions)) noexcept
{
  fb.deny_if_errno(SYS_ioctl, arg_eq32(1, static_cast<u32>(posix::tiocsti)), err);
  fb.deny_if_errno(SYS_ioctl, arg_eq32(1, static_cast<u32>(posix::tioclinux)), err);
  fb.deny_if_errno(SYS_ioctl, arg_eq32(1, static_cast<u32>(posix::tiocsetd)), err);
  return fb;
}

// AF_NETLINK is the kernel's configuration interface and AF_PACKET is raw link-layer access; neither
// is "networking" in the sense anybody means when they allow a group called network. Denying them is
// what keeps a sandbox away from RTM_NEWLINK and the tunnel-device code behind it (CVE-2026-63921).
// Masked for the same reason: socket(2)'s family is an `int`.
template<usize N>
constexpr filter_builder<N> &
deny_raw_socket_families(filter_builder<N> &fb, u16 err = static_cast<u16>(error::permissions)) noexcept
{
  constexpr u32 af_netlink = 16;
  constexpr u32 af_packet = 17;
  fb.deny_if_errno(SYS_socket, arg_eq32(0, af_netlink), err);
  fb.deny_if_errno(SYS_socket, arg_eq32(0, af_packet), err);
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
  fb.deny_if_errno(SYS_socketpair, arg_eq32(0, af_netlink), err);
  fb.deny_if_errno(SYS_socketpair, arg_eq32(0, af_packet), err);
#endif
  return fb;
}

template<usize N>
inline int
load_notif(filter_builder<N> &fb, bool set_no_new_privs = true)
{
  if ( set_no_new_privs ) {
    i32 __nnp = micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
    if ( __nnp < 0 ) return __nnp;      // NNP must succeed before SET_MODE_FILTER (else EACCES); was discarded
  }
  auto p = fb.prog();
  // WARNING: the SAME gate as load(). This one used to be missing, and USER_NOTIF is the mode
  // where its absence bites hardest: a filter with no arch guard answers SECCOMP_RET_ALLOW to a
  // compat-ABI syscall directly out of the BPF, so the supervisor never sees the call and cannot
  // compensate for it
  if ( !fb.valid() ) return -error::invalid_arg;
  return posix::seccomp_load_filter_notif(p);
}

// the structural half of filter_builder::valid(), for a program that did not come from a builder:
// non-empty, ends in a return, and opens with the load of seccomp_data.arch that the gate needs
[[nodiscard]] inline bool
prog_is_arch_gated(const bpf::fprog_t &prog) noexcept
{
  if ( prog.len == 0 || prog.filter == nullptr ) return false;
  if ( prog.filter[0].code != static_cast<u16>(bpf::ld | bpf::w | bpf::abs) ) return false;
  if ( prog.filter[0].k != posix::seccomp_data_arch_off ) return false;
  return (prog.filter[prog.len - 1].code & 0x07u) == bpf::ret;
}

// WARNING: `unchecked` is the escape hatch for a program deliberately built without an arch gate.
// It is never the right default -- an ungated ladder is evaluated against whatever ABI the caller
// entered by -- so it has to be asked for by name
inline int
load_raw(bpf::fprog_t &prog, bool set_no_new_privs = true, u32 flags = 0, bool unchecked = false)
{
  if ( !unchecked && !prog_is_arch_gated(prog) ) return -error::invalid_arg;
  if ( set_no_new_privs ) {
    i32 __nnp = micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
    if ( __nnp < 0 ) return __nnp;      // NNP must succeed before SET_MODE_FILTER (else EACCES); was discarded
  }
  return posix::seccomp_load_filter(prog, flags);
}

inline int
strict(void)
{
  return posix::seccomp_strict();
}

inline bool
action_avail_kill_process() noexcept
{
  return posix::seccomp_action_avail(posix::seccomp_ret_kill_process) == 0;
}

inline bool
action_avail_log() noexcept
{
  return posix::seccomp_action_avail(posix::seccomp_ret_log) == 0;
}

inline bool
action_avail_notify() noexcept
{
  return posix::seccomp_action_avail(posix::seccomp_ret_user_notif) == 0;
}

inline int
notif_receive(int listener_fd, posix::seccomp_notif_t &req)
{
  return posix::seccomp_notify_receive(listener_fd, req);
}
};      // namespace seccomp
};      // namespace sec
};      // namespace micron
