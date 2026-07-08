//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../memory/allocation/abcmalloc/tapi.hpp"

#include "../../bits/__arch.hpp"      // __micron_arch_*, __micron_no_ssp
#include "../../errno.hpp"
#include "../../type_traits.hpp"
#include "../__includes.hpp"

#include "sched.hpp"

#include "signal.hpp"
#include "system.hpp"

#include "types.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

// raw arch specific asm clone trampolines

#if defined(__micron_arch_amd64)
[[maybe_unused]] static __attribute__((naked, noinline)) __micron_no_ssp long
__micron_clone_entry(int (*)(void *), void *, unsigned long, void *, int *, void *, int *)
{
  // entry: rdi=fn rsi=stack rdx=flags rcx=arg r8=ptid r9=tls 8(%rsp)=ctid
  asm volatile("xorl %eax, %eax\n\t"
               "movb $56, %al\n\t"           // SYS_clone
               "movq %rdi, %r11\n\t"         // save fn
               "movq %rdx, %rdi\n\t"         // flags -> a1
               "movq %r8, %rdx\n\t"          // ptid  -> a3
               "movq %r9, %r8\n\t"           // tls   -> a5
               "movq 8(%rsp), %r10\n\t"      // ctid  -> a4
               "movq %r11, %r9\n\t"          // fn    -> r9 (preserved across syscall)
               "andq $-16, %rsi\n\t"         // align child stack top
               "subq $8, %rsi\n\t"
               "movq %rcx, (%rsi)\n\t"      // seed arg on child stack
               "syscall\n\t"
               "testq %rax, %rax\n\t"
               "jnz 1f\n\t"               // parent -> return pid in rax
               "xorl %ebp, %ebp\n\t"      // child: terminate unwind chain
               "popq %rdi\n\t"            // arg -> a1 of fn
               "callq *%r9\n\t"           // fn(arg)
               "movl %eax, %edi\n\t"      // status
               "xorl %eax, %eax\n\t"
               "movb $60, %al\n\t"      // SYS_exit
               "syscall\n\t"
               "hlt\n\t"
               "1:\n\t"
               "ret\n\t");
}
#elif defined(__micron_arch_x86)
[[maybe_unused]] static __attribute__((naked, noinline)) __micron_no_ssp long
__micron_clone_entry(int (*)(void *), void *, unsigned long, void *, int *, void *, int *)
{
  // all args on stack; +16 after pushing the 4 callee-saved regs
  asm volatile("pushl %ebp\n\t"
               "pushl %edi\n\t"
               "pushl %esi\n\t"
               "pushl %ebx\n\t"
               "movl 20(%esp), %eax\n\t"      // fn
               "movl 24(%esp), %ecx\n\t"      // stack top
               "movl 28(%esp), %ebx\n\t"      // flags -> a1
               "movl 36(%esp), %edx\n\t"      // ptid  -> a3
               "movl 40(%esp), %esi\n\t"      // tls   -> a4 (CLONE_BACKWARDS)
               "movl 44(%esp), %edi\n\t"      // ctid  -> a5
               "andl $-16, %ecx\n\t"          // align top
               "subl $16, %ecx\n\t"           // reserve seed area below newsp
               "movl %eax, (%ecx)\n\t"        // fn  at newsp+0
               "movl 32(%esp), %eax\n\t"      // arg
               "movl %eax, 4(%ecx)\n\t"       // arg at newsp+4
               "movl $120, %eax\n\t"          // SYS_clone
               "int $0x80\n\t"
               "testl %eax, %eax\n\t"
               "jnz 1f\n\t"                  // parent
               "movl (%esp), %eax\n\t"       // child: fn
               "movl 4(%esp), %edx\n\t"      // arg
               "xorl %ebp, %ebp\n\t"
               "subl $16, %esp\n\t"         // align outgoing frame
               "movl %edx, (%esp)\n\t"      // arg -> outgoing slot
               "calll *%eax\n\t"            // fn(arg)
               "movl %eax, %ebx\n\t"        // status
               "movl $1, %eax\n\t"          // SYS_exit
               "int $0x80\n\t"
               "hlt\n\t"
               "1:\n\t"
               "popl %ebx\n\t"
               "popl %esi\n\t"
               "popl %edi\n\t"
               "popl %ebp\n\t"
               "ret\n\t");
}
#elif defined(__micron_arch_arm64)
// NOTE: aarch64 GCC __IGNORES__ __attribute__((naked)) (?!?!) which would wrap the body in a prologue/epilogue
// and corrupt the child SP pivot
extern "C" long __micron_clone_entry(int (*)(void *), void *, unsigned long, void *, int *, void *, int *);
asm(".text\n\t"
    ".weak __micron_clone_entry\n\t"
    ".type __micron_clone_entry, %function\n\t"
    ".p2align 2\n"
    "__micron_clone_entry:\n\t"
    // entry: x0=fn x1=stack x2=flags x3=arg x4=ptid x5=tls x6=ctid
    "and x1, x1, #-16\n\t"             // align child stack top
    "stp x0, x3, [x1, #-16]!\n\t"      // seed {fn, arg}; x1 becomes newsp
    "mov x0, x2\n\t"                   // flags -> a0
    "mov x2, x4\n\t"                   // ptid  -> a2
    "mov x3, x5\n\t"                   // tls   -> a3
    "mov x4, x6\n\t"                   // ctid  -> a4
    "mov x8, #220\n\t"                 // SYS_clone
    "svc #0\n\t"
    "cbz x0, 1f\n\t"      // child -> 1
    "ret\n\t"             // parent -> return pid in x0
    "1:\n\t"
    "ldp x1, x0, [sp], #16\n\t"      // x1=fn, x0=arg
    "blr x1\n\t"                     // fn(arg)
    "mov x8, #93\n\t"                // SYS_exit
    "svc #0\n\t");
#elif defined(__micron_arch_arm32)
[[maybe_unused]] static __attribute__((naked, noinline)) __micron_no_ssp long
__micron_clone_entry(int (*)(void *), void *, unsigned long, void *, int *, void *, int *)
{
  // entry: r0=fn r1=stack r2=flags r3=arg; ptid@[sp,#0] tls@[sp,#4] ctid@[sp,#8] (+16 after push)
  asm volatile("stmfd sp!, {r4, r5, r6, r7}\n\t"
               "mov r7, #120\n\t"           // SYS_clone
               "mov r5, r0\n\t"             // fn  -> r5 (callee-saved, survives into child)
               "mov r6, r3\n\t"             // arg -> r6 (callee-saved, survives into child)
               "mov r0, r2\n\t"             // flags -> a0
               "and r1, r1, #-16\n\t"       // align newsp (a1)
               "ldr r2, [sp, #16]\n\t"      // ptid -> a2
               "ldr r3, [sp, #20]\n\t"      // tls  -> a3 (CLONE_BACKWARDS)
               "ldr r4, [sp, #24]\n\t"      // ctid -> a4
               "svc #0\n\t"
               "tst r0, r0\n\t"
               "bne 1f\n\t"          // parent
               "mov r0, r6\n\t"      // child: arg -> a0
               "blx r5\n\t"          // fn(arg)
               "mov r7, #1\n\t"      // SYS_exit
               "svc #0\n\t"
               "1:\n\t"
               "ldmfd sp!, {r4, r5, r6, r7}\n\t"
               "bx lr\n\t");
}
#endif

// clone3 child trampolines (for micthread et al.)
#if defined(__micron_arch_amd64)
[[maybe_unused]] static __attribute__((naked, noinline)) __micron_no_ssp long
__micron_clone3_entry(int (*)(void *), void *, void *, unsigned long)
{
  // rdi=thunk rsi=payload rdx=&clone_args rcx=args_size
  asm volatile("pushq %r12\n\t"      // preserve callee-saved (parent path restores)
               "pushq %r13\n\t"
               "movq %rdi, %r12\n\t"      // thunk -> r12 (survives into child)
               "movq %rsi, %r13\n\t"      // payload -> r13
               "movq %rdx, %rdi\n\t"      // &clone_args -> a1
               "movq %rcx, %rsi\n\t"      // args_size  -> a2
               "movl $435, %eax\n\t"      // SYS_clone3
               "syscall\n\t"
               "testq %rax, %rax\n\t"
               "jnz 1f\n\t"               // parent -> return tid in rax
               "xorl %ebp, %ebp\n\t"      // child: terminate unwind chain
               "andq $-16, %rsp\n\t"      // 16-byte align for the call ABI
               "movq %r13, %rdi\n\t"      // payload -> a1
               "callq *%r12\n\t"          // thunk(payload)
               "movl %eax, %edi\n\t"      // status
               "movl $60, %eax\n\t"       // SYS_exit
               "syscall\n\t"
               "hlt\n\t"
               "1:\n\t"
               "popq %r13\n\t"
               "popq %r12\n\t"
               "ret\n\t");
}
#elif defined(__micron_arch_x86)
[[maybe_unused]] static __attribute__((naked, noinline)) __micron_no_ssp long
__micron_clone3_entry(int (*)(void *), void *, void *, unsigned long)
{
  // cdecl stack args; +16 after pushing the 4 callee-saved regs
  asm volatile("pushl %ebp\n\t"
               "pushl %edi\n\t"
               "pushl %esi\n\t"
               "pushl %ebx\n\t"
               "movl 28(%esp), %ebx\n\t"      // &clone_args -> a1
               "movl 32(%esp), %ecx\n\t"      // args_size  -> a2
               "movl 20(%esp), %esi\n\t"      // thunk -> esi (callee-saved, survives into child)
               "movl 24(%esp), %edi\n\t"      // payload -> edi
               "movl $435, %eax\n\t"          // SYS_clone3
               "int $0x80\n\t"
               "testl %eax, %eax\n\t"
               "jnz 1f\n\t"               // parent
               "xorl %ebp, %ebp\n\t"      // child
               "andl $-16, %esp\n\t"      // align
               "subl $12, %esp\n\t"       // keep 16-byte alignment after the arg push
               "pushl %edi\n\t"           // payload arg
               "calll *%esi\n\t"          // thunk(payload)
               "movl %eax, %ebx\n\t"      // status
               "movl $1, %eax\n\t"        // SYS_exit
               "int $0x80\n\t"
               "hlt\n\t"
               "1:\n\t"
               "popl %ebx\n\t"
               "popl %esi\n\t"
               "popl %edi\n\t"
               "popl %ebp\n\t"
               "ret\n\t");
}
#elif defined(__micron_arch_arm64)
extern "C" long __micron_clone3_entry(int (*)(void *), void *, void *, unsigned long);
asm(".text\n\t"
    ".weak __micron_clone3_entry\n\t"
    ".type __micron_clone3_entry, %function\n\t"
    ".p2align 2\n"
    "__micron_clone3_entry:\n\t"
    // x0=thunk x1=payload x2=&clone_args x3=args_size
    "stp x19, x20, [sp, #-16]!\n\t"      // preserve callee-saved
    "mov x19, x0\n\t"                    // thunk (survives into child)
    "mov x20, x1\n\t"                    // payload
    "mov x0, x2\n\t"                     // &clone_args -> a0
    "mov x1, x3\n\t"                     // args_size  -> a1
    "mov x8, #435\n\t"                   // SYS_clone3
    "svc #0\n\t"
    "cbz x0, 1f\n\t"                   // child -> 1
    "ldp x19, x20, [sp], #16\n\t"      // parent: restore + return tid
    "ret\n\t"
    "1:\n\t"
    "mov x0, x20\n\t"      // payload -> a0
    "blr x19\n\t"          // thunk(payload)
    "mov x8, #93\n\t"      // SYS_exit
    "svc #0\n\t");
#elif defined(__micron_arch_arm32)
[[maybe_unused]] static __attribute__((naked, noinline)) __micron_no_ssp long
__micron_clone3_entry(int (*)(void *), void *, void *, unsigned long)
{
  // r0=thunk r1=payload r2=&clone_args r3=args_size
  asm volatile("stmfd sp!, {r4, r5, r7}\n\t"      // preserve callee-saved
               "mov r4, r0\n\t"                   // thunk (survives into child)
               "mov r5, r1\n\t"                   // payload
               "mov r0, r2\n\t"                   // &clone_args -> a1
               "mov r1, r3\n\t"                   // args_size  -> a2
               "mov r7, #435\n\t"                 // SYS_clone3
               "svc #0\n\t"
               "tst r0, r0\n\t"
               "bne 1f\n\t"          // parent
               "mov r0, r5\n\t"      // child: payload -> a1
               "blx r4\n\t"          // thunk(payload)
               "mov r7, #1\n\t"      // SYS_exit
               "svc #0\n\t"
               "1:\n\t"
               "ldmfd sp!, {r4, r5, r7}\n\t"
               "bx lr\n\t");
}
#endif

#pragma GCC diagnostic pop

namespace micron
{

/*
 *           long clone(unsigned long flags, void *stack,
 *                      int *parent_tid, int *child_tid,
 *                     unsigned long tls);
 */

namespace posix
{

enum class clone_flags : u64 {      // u64: clone_io is 0x80000000 and clone3 admits 64-bit flags
  clear_tid = clone_child_cleartid,
  set_tid = clone_child_settid,
  detached_obsolete = clone_detached,
  new_cgroup = clone_newcgroup,
  copy_files = clone_files,
  same_io = clone_io,
  newcgroup = clone_newcgroup,
  newipc = clone_newipc,
  newnet = clone_newnet,
  newns = clone_newns,
  newpid = clone_newpid,
  newuser = clone_newuser,
  parent_copy = clone_parent,
  parent_settid = clone_parent_settid,
  pid_pointer = clone_pidfd,
  ptrace = clone_ptrace,
  enable_tls = clone_settls,
  share_signals = clone_sighand,
  start_stopped_removed = 0,
  share_semaphores = clone_sysvsem,
  as_thread = clone_thread,
  no_trace = clone_untraced,
  vfork_obsolete = clone_vfork,
  same_memory = clone_vm
};

enum class posix_process_flags {
  reset_ids = 0x01,
  set_pgroup = 0x02,
  sig_def = 0x04,
  sig_mask = 0x08,
  sched_param = 0x10,
  set_scheduler = 0x20,
  use_vfork = 0x40,
  new_session = 0x80,
  set_cgroup = 0x100
};

auto
clone_kernel(unsigned long flags, void *stack, int *parent_tid, int *child_tid, unsigned long tls)
{
#if defined(__micron_arch_arm32) || defined(__micron_arch_x86)
  // WARNING: arm32 (and i386) are CLONE_BACKWARDS; tls and child_tid are swapped vs arm64/x86_64
  return micron::syscall(SYS_clone, flags, stack, parent_tid, tls, child_tid);
#else
  return micron::syscall(SYS_clone, flags, stack, parent_tid, child_tid, tls);
#endif
}

// NOTE: Linux 5.3+(5.7) req

struct clone_args {
  u64 flags;        /* Flags bit mask */
  u64 pidfd;        /* Where to store PID file descriptor
                       (int *) */
  u64 child_tid;    /* Where to store child TID,
                       in child's memory (posix::pid_t *) */
  u64 parent_tid;   /* Where to store child TID,
                       in parent's memory (posix::pid_t *) */
  u64 exit_signal;  /* Signal to deliver to parent on
                       child termination */
  u64 stack;        /* Pointer to lowest byte of stack */
  u64 stack_size;   /* Size of stack */
  u64 tls;          /* Location of new TLS */
  u64 set_tid;      /* Pointer to a posix::pid_t array
                       (since Linux 5.5) */
  u64 set_tid_size; /* Number of elements in set_tid
                       (since Linux 5.5) */
  u64 cgroup;       /* File descriptor for target cgroup
                       of child (since Linux 5.7) */
};

auto
clone3_kernel(clone_args &args)
{
  return micron::syscall(SYS_clone3, &args, sizeof(args));
}

inline constexpr unsigned long micthread_flags
    = clone_vm | clone_fs | clone_files | clone_sighand | clone_thread | clone_sysvsem | clone_settls | clone_child_cleartid;

// the value the spawner seeds into the CHILD_CLEARTID word
inline constexpr int __micthread_ctid_alive = 1;

// main clone3 spawn
inline pid_t
clone3_spawn(int (*thunk)(void *), void *payload, void *stack_low, usize stack_size, unsigned long tls, pid_t *parent_tid, int *child_tid,
             unsigned long flags = micthread_flags)
{
  if ( !thunk || !stack_low || stack_size == 0 ) return -EINVAL;
  if ( child_tid ) *child_tid = __micthread_ctid_alive;
  clone_args args{};
  args.flags = flags;
  args.exit_signal = 0;      // CLONE_THREAD: no exit signal to the parent (kernel rejects otherwise)
  args.stack = reinterpret_cast<u64>(stack_low);
  args.stack_size = stack_size;
  args.tls = tls;      // amd64/arm64/arm32: CLONE_SETTLS takes the raw TP (FSBASE / TPIDR / TPIDRURO)
  args.parent_tid = reinterpret_cast<u64>(parent_tid);
  args.child_tid = reinterpret_cast<u64>(child_tid);
#if defined(__micron_arch_x86)
  // i386 is the exception (WHY?!): CLONE_SETTLS expects a struct user_desc * not a raw TP (?!?!?!?!)
  struct __i386_clone_user_desc {
    unsigned int entry_number, base_addr, limit, flags;
  };

  __i386_clone_user_desc __i386_tls_desc{ 0xffffffffu, static_cast<unsigned int>(tls), 0xfffffu, 0x51u };
  args.tls = reinterpret_cast<u64>(&__i386_tls_desc);
#endif
  long raw = ::__micron_clone3_entry(thunk, payload, &args, sizeof(args));
  if ( micron::syscall_failed(raw) && child_tid ) *child_tid = 0;      // no child ran
  return static_cast<pid_t>(raw);
}

auto
fork_kernel(void)
{
#if defined(__micron_syscall_generic)
  // arm64 has no fork
  return micron::syscall(SYS_clone, static_cast<unsigned long>(sig_chld), 0, 0, 0, 0);
#else
  return micron::syscall(SYS_fork);
#endif
}

inline __attribute__((always_inline)) unsigned long
__micron_fold_exit_signal(unsigned long flags, int exit_signal) noexcept
{
  return (flags & ~0xffUL) | (static_cast<unsigned long>(exit_signal) & 0xffUL);
}

template<auto Fn, typename... Args> struct __clone_payload {
  void *args[sizeof...(Args)];

  static int
  thunk(void *p)
  {
    auto *self = static_cast<__clone_payload *>(p);
    int rc = __invoke(self, micron::make_index_sequence<sizeof...(Args)>{});
    destroy(self);
    return rc;
  }

  static void
  destroy(__clone_payload *self)
  {
    __cleanup(self, micron::make_index_sequence<sizeof...(Args)>{});
    delete self;
  }

private:
  template<usize... I>
  static int
  __invoke(__clone_payload *self, micron::index_sequence<I...>)
  {
    using ret_t = decltype(Fn(static_cast<Args &&>(*static_cast<Args *>(self->args[I]))...));
    if constexpr ( micron::is_same_v<ret_t, int> )
      return Fn(static_cast<Args &&>(*static_cast<Args *>(self->args[I]))...);
    else {
      Fn(static_cast<Args &&>(*static_cast<Args *>(self->args[I]))...);
      return 0;
    }
  }

  template<usize... I>
  static void
  __cleanup([[maybe_unused]] __clone_payload *self, micron::index_sequence<I...>)
  {
    (delete static_cast<Args *>(self->args[I]), ...);
  }
};

template<usize Sz, auto Fn, typename... Args>
pid_t
clone(addr_t *stack, int flags, Args &&...args)
{
  if ( !stack ) return -EINVAL;

  using payload_t = __clone_payload<Fn, micron::decay_t<Args>...>;
  auto *payload = new payload_t{ { reinterpret_cast<void *>(new micron::decay_t<Args>(static_cast<Args &&>(args)))... } };

  unsigned long f = static_cast<unsigned long>(static_cast<unsigned>(flags));
  int exit_signal = sig_chld;
  if ( (f & clone_parent) or ((f & clone_sighand) and (f & clone_thread)) ) exit_signal = 0;      // kernel constraint otherwise -22
  f = __micron_fold_exit_signal(f, exit_signal);

  uintptr_t top = (reinterpret_cast<uintptr_t>(stack) + Sz) & ~uintptr_t(0xF);

  long raw = ::__micron_clone_entry(&payload_t::thunk, reinterpret_cast<void *>(top), f, reinterpret_cast<void *>(payload), nullptr,
                                    nullptr, nullptr);

  if ( micron::syscall_failed(raw) ) {
    payload_t::destroy(payload);      // clone failed, no child ran
    return static_cast<pid_t>(raw);
  }
  if ( !(f & clone_vm) ) payload_t::destroy(payload);      // parent owns its independent (COW) copy
  return static_cast<pid_t>(raw);
}

// special clone func
pid_t
__fork_clone(int exit_signal)
{
  // these should be exactly like this
  posix::clone_args clone_args{};
  clone_args.flags = 0;
  clone_args.exit_signal = exit_signal;      // should be sig_chld
  clone_args.stack = 0;
  clone_args.stack_size = 0;
  clone_args.parent_tid = 0;
  clone_args.child_tid = 0;
  clone_args.tls = 0;

  long raw = posix::clone3_kernel(clone_args);
  if ( micron::syscall_failed(raw) && micron::syscall_errno(raw) == ENOSYS ) {
    // clone3 unavailable (pre-5.3 kernel, or under qemu-user) -> legacy fork/clone fallback
    raw = posix::fork_kernel();
  }
  return static_cast<pid_t>(raw);
}

// c-style entry
pid_t
clone(int (*fn)(void *), void *arg, unsigned long flags, void *stack, usize stack_size, int *parent_tid = nullptr, int *child_tid = nullptr,
      unsigned long tls = 0, int exit_signal = sig_chld)
{
  if ( !stack or !fn or stack_size == 0 ) return -EINVAL;

  if ( (flags & clone_parent) or ((flags & clone_sighand) and (flags & clone_thread)) )
    exit_signal = 0;      // kernel constraint otherwise -22
  unsigned long f = __micron_fold_exit_signal(flags, exit_signal);

  uintptr_t top = (reinterpret_cast<uintptr_t>(stack) + stack_size) & ~uintptr_t(0xF);

  long raw = ::__micron_clone_entry(fn, reinterpret_cast<void *>(top), f, arg, parent_tid, reinterpret_cast<void *>(tls), child_tid);
  return static_cast<pid_t>(raw);
}
};      // namespace posix
};      // namespace micron
