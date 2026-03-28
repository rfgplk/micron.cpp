//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../syscall.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../sys/prctl.hpp"

namespace micron
{

namespace seccomp
{

constexpr static const u32 audit_arch_64bit = 0x80000000u;
constexpr static const u32 audit_arch_le = 0x40000000u;

enum class arch : u32 {
  // LE 32-bit
  x86 = audit_arch_le | 0x03u,     // EM_386=3
  arm = audit_arch_le | 0x28u,     // EM_ARM=40
  // BE 32-bit
  m68k = 0x04u,        // EM_68K=4
  mips_be = 0x08u,     // EM_MIPS=8, big-endian
  ppc = 0x14u,         // EM_PPC=20
  s390 = 0x16u,        // EM_S390=22
  // LE 64-bit
  x86_64 = audit_arch_64bit | audit_arch_le | 0x3Eu,           // EM_X86_64=62
  x32 = audit_arch_le | 0x3Eu,                                 // x32 ABI (64-bit EM, 32-bit pointers)
  aarch64 = audit_arch_64bit | audit_arch_le | 0xB7u,          // EM_AARCH64=183
  loongarch64 = audit_arch_64bit | audit_arch_le | 0x102u,     // EM_LOONGARCH=258
  mipsel = audit_arch_le | 0x08u,
  mipsel64 = audit_arch_64bit | audit_arch_le | 0x08u,
  ppc64le = audit_arch_64bit | audit_arch_le | 0x15u,     // EM_PPC64=21
  riscv64 = audit_arch_64bit | audit_arch_le | 0xF3u,     // EM_RISCV=243
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
  ne = 1u,            // arg != datum_a
  lt = 2u,            // arg <  datum_a  (unsigned)
  le = 3u,            // arg <= datum_a  (unsigned)
  eq = 4u,            // arg == datum_a
  ge = 5u,            // arg >= datum_a  (unsigned)
  gt = 6u,            // arg >  datum_a  (unsigned)
  masked_eq = 7u,     // (arg & datum_b) == (datum_a & datum_b)
};

struct arg_cmp_t {
  u32 arg;     // argument index 0..5
  cmp op;
  u64 datum_a;     // comparison value  (or target value for masked_eq)
  u64 datum_b;     // mask (only used for masked_eq)
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

constexpr bpf::insn_t
load_arg_msw(u32 n) noexcept
{
#if defined(__micron_endian_big)

  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, posix::seccomp_data_arg_first_off(n));
#else
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, posix::seccomp_data_arg_second_off(n));
#endif
}

constexpr bpf::insn_t
load_arg_lsw(u32 n) noexcept
{
#if defined(__micron_endian_big)
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, posix::seccomp_data_arg_second_off(n));
#else
  return bpf::stmt(bpf::ld | bpf::w | bpf::abs, posix::seccomp_data_arg_first_off(n));
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
  case cmp::eq :
  case cmp::ne :
    return 5u;
  case cmp::gt :
  case cmp::ge :
    return 6u;
  case cmp::lt :
  case cmp::le :
  case cmp::masked_eq :
    return 7u;
  }
  return 0u;
}

template <usize Max = 128>
  requires(Max > 0 and Max <= bpf::max_instructions)     // guard against kernel limit
struct filter_builder {

  bpf::insn_t insns[Max]{};
  usize count = 0;
  bool arch_ok = false;     // set by require_arch / require_native_arch
  bool sealed = false;      // set by default_* methods

  constexpr usize
  remaining() const noexcept
  {
    return Max - count;
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

    case cmp::eq :
      push(load_arg_lsw(ac.arg));
      push(jeq_k(val_lsw, 0, 3));
      push(load_arg_msw(ac.arg));
      push(jeq_k(val_msw, 0, 1));
      push(ret_k(action));
      break;

    case cmp::ne :
      push(load_arg_lsw(ac.arg));
      push(jeq_k(val_lsw, 0, 2));
      push(ret_k(action));
      push(load_arg_msw(ac.arg));
      push(jeq_k(val_msw, 1, 0));
      push(ret_k(action));
      break;

    case cmp::lt :
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 5, 0));
      push(jeq_k(val_msw, 1, 0));
      push(ret_k(action));
      push(load_arg_lsw(ac.arg));
      push(jge_k(val_lsw, 1, 0));
      push(ret_k(action));
      break;

    case cmp::le :
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 5, 0));
      push(jeq_k(val_msw, 1, 0));
      push(ret_k(action));
      push(load_arg_lsw(ac.arg));
      push(jgt_k(val_lsw, 1, 0));
      push(ret_k(action));
      break;

    case cmp::gt :
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 3, 0));
      push(jeq_k(val_msw, 0, 3));
      push(load_arg_lsw(ac.arg));
      push(jgt_k(val_lsw, 0, 1));
      push(ret_k(action));
      break;

    case cmp::ge :
      push(load_arg_msw(ac.arg));
      push(jgt_k(val_msw, 3, 0));
      push(jeq_k(val_msw, 0, 3));
      push(load_arg_lsw(ac.arg));
      push(jge_k(val_lsw, 0, 1));
      push(ret_k(action));
      break;

    case cmp::masked_eq : {
      const u32 mask_msw = msw(ac.datum_b);
      const u32 mask_lsw = lsw(ac.datum_b);
      const u32 cmp_lsw = val_lsw & mask_lsw;     // (datum_a & datum_b) low half
      const u32 cmp_msw = val_msw & mask_msw;     // (datum_a & datum_b) high half
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
  constexpr filter_builder &
  require_arch(arch a) noexcept
  {
    if ( remaining() < 3 )
      return *this;
    push(load_arch());
    push(jeq_k(static_cast<u32>(a), 1, 0));
    push(ret_k(posix::seccomp_ret_kill_process));
    arch_ok = true;
    return *this;
  }

  constexpr filter_builder &
  require_native_arch() noexcept
  {
    return require_arch(native_arch);
  }

  constexpr filter_builder &
  allow(i32 nr) noexcept
  {
    if ( sealed )
      return *this;
    if ( remaining() < 3 )
      return *this;
    push(load_syscall_nr());
    push(jeq_k(static_cast<u32>(nr), 0, 1));
    push(ret_k(posix::seccomp_ret_allow));
    return *this;
  }

  constexpr filter_builder &
  deny(i32 nr, u32 action = posix::seccomp_ret_kill_process) noexcept
  {
    if ( sealed )
      return *this;
    if ( remaining() < 3 )
      return *this;
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

  template <i32... Nrs>
    requires(sizeof...(Nrs) >= 1 and sizeof...(Nrs) <= 256)
  constexpr filter_builder &
  allow_batch() noexcept
  {
    if ( sealed )
      return *this;
    constexpr usize N = sizeof...(Nrs);
    if ( remaining() < N + 2 )
      return *this;

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
    if ( sealed )
      return *this;
    if ( remaining() < 4 )
      return *this;
    push(load_syscall_nr());
    push(jge_k(static_cast<u32>(lo_nr), 0, 2));
    push(jgt_k(static_cast<u32>(hi_nr), 1, 0));
    push(ret_k(posix::seccomp_ret_allow));
    return *this;
  }

  constexpr filter_builder &
  deny_range(i32 lo_nr, i32 hi_nr, u32 action = posix::seccomp_ret_kill_process) noexcept
  {
    if ( sealed )
      return *this;
    if ( remaining() < 4 )
      return *this;
    push(load_syscall_nr());
    push(jge_k(static_cast<u32>(lo_nr), 0, 2));
    push(jgt_k(static_cast<u32>(hi_nr), 1, 0));
    push(ret_k(action));
    return *this;
  }

  constexpr filter_builder &
  action_if(i32 nr, const arg_cmp_t &ac, u32 action) noexcept
  {
    if ( sealed )
      return *this;
    const usize psize = pred_insns(ac.op);
    if ( psize == 0 || remaining() < psize + 2 )
      return *this;
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
  finalize() noexcept
  {
    if ( !sealed )
      default_kill();
    return *this;
  }

  constexpr bool
  valid() const noexcept
  {
    if ( !arch_ok )
      return false;
    if ( count == 0 || !sealed )
      return false;
    const u16 last_class = insns[count - 1].code & 0x07u;
    return last_class == bpf::ret;
  }

  bpf::fprog_t
  prog() noexcept
  {
    if ( !arch_ok )
      finalize();
    if ( !sealed )
      finalize();
    return bpf::fprog_t{ static_cast<u16>(count), insns };
  }
};

template <usize N>
inline int
load(filter_builder<N> &fb, bool set_no_new_privs = true, u32 extra_flags = 0)
{
  if ( set_no_new_privs )
    micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
  auto p = fb.prog();
  return posix::seccomp_load_filter(p, extra_flags);
}

template <usize N>
inline int
load_tsync(filter_builder<N> &fb, bool set_no_new_privs = true)
{
  return load(fb, set_no_new_privs, posix::seccomp_filter_flag_tsync);
}

template <usize N>
inline int
load_notif(filter_builder<N> &fb, bool set_no_new_privs = true)
{
  if ( set_no_new_privs )
    micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
  auto p = fb.prog();
  return posix::seccomp_load_filter_notif(p);
}

inline int
load_raw(bpf::fprog_t &prog, bool set_no_new_privs = true, u32 flags = 0)
{
  if ( set_no_new_privs )
    micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
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
  return posix::seccomp_notif_receive(listener_fd, req);
}

inline int
notif_continue(int listener_fd, u64 id)
{
  auto resp = posix::seccomp_notif_continue(id);
  return posix::seccomp_notif_respond(listener_fd, resp);
}

inline int
notif_inject_error(int listener_fd, u64 id, i32 err)
{
  auto resp = posix::seccomp_notif_error(id, err);
  return posix::seccomp_notif_respond(listener_fd, resp);
}

inline int
notif_inject_success(int listener_fd, u64 id, i64 retval)
{
  auto resp = posix::seccomp_notif_success(id, retval);
  return posix::seccomp_notif_respond(listener_fd, resp);
}

inline bool
notif_id_valid(int listener_fd, u64 id)
{
  return posix::seccomp_notif_id_valid(listener_fd, id) == 0;
}

/*
 example
template <usize N = 64>
constexpr filter_builder<N>
minimal_base(arch a = native_arch) noexcept
{
  filter_builder<N> f;
  f.require_arch(a);
  f.allow(SYS_brk).allow(SYS_mmap).allow(SYS_munmap).allow(SYS_exit).allow(SYS_exit_group).allow(SYS_rt_sigreturn);
  return f;
}
*/
};     // namespace seccomp
};     // namespace micron
