//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"

#include "../attributes.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%5
// activation record control
//
// in dev, NOTE:
//   .. frame_pointer()/return_address<L>()/caller() and frame-chain walking assume the target was
//     built with frame pointers; -Ofast OMITS them, so chain walking is best effort attempt
//   .. thread_pointer() assumes a TCB is installed (not available via freestanding; pthread/glibc pulls it in hosted)
//   .. set_stack_pointer()/call_on_stack() are dangerous

namespace micron
{
namespace ar
{

[[nodiscard, gnu::always_inline]] inline void *
stack_pointer() noexcept
{
  void *sp;
#if defined(__micron_arch_amd64)
  asm volatile("movq %%rsp, %0" : "=r"(sp));
#elif defined(__micron_arch_x86)
  asm volatile("movl %%esp, %0" : "=r"(sp));
#elif defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
  asm volatile("mov %0, sp" : "=r"(sp));
#endif
  return sp;
}

[[nodiscard, gnu::always_inline]] inline void *
frame_pointer() noexcept
{
  return __builtin_frame_address(0);
}

[[nodiscard, gnu::always_inline]] inline void *
program_counter() noexcept
{
  void *pc;
#if defined(__micron_arch_amd64)
  asm volatile("leaq (%%rip), %0" : "=r"(pc));
#elif defined(__micron_arch_x86)
  asm volatile("call 1f\n\t"      // i386 has no PC-relative LEA; read EIP via call/pop
               "1: popl %0"
               : "=r"(pc));
#elif defined(__micron_arch_arm64)
  asm volatile("adr %0, ." : "=r"(pc));
#elif defined(__micron_arch_arm32)
  asm volatile("mov %0, pc" : "=r"(pc));
#endif
  return pc;
}

template<unsigned Level = 0>
[[nodiscard, gnu::always_inline]] inline void *
return_address() noexcept
{
  return __builtin_return_address(Level);
}

template<unsigned Level = 0>
[[nodiscard, gnu::always_inline]] inline void *
frame_address() noexcept
{
  return __builtin_frame_address(Level);
}

[[nodiscard, gnu::always_inline]] inline void *
thread_pointer() noexcept
{
  void *tp;
#if defined(__micron_arch_amd64)
#if defined(__micron_x86_fsgsbase)
  asm volatile("rdfsbase %0" : "=r"(tp));
#else
  asm volatile("movq %%fs:0, %0" : "=r"(tp));
#endif
#elif defined(__micron_arch_x86)
  asm volatile("movl %%gs:0, %0" : "=r"(tp));
#elif defined(__micron_arch_arm64)
  asm volatile("mrs %0, tpidr_el0" : "=r"(tp));
#elif defined(__micron_arch_arm32)
  asm volatile("mrc p15, 0, %0, c13, c0, 3" : "=r"(tp));
#endif
  return tp;
}

[[gnu::always_inline]] inline bool
set_thread_pointer([[maybe_unused]] void *tp) noexcept
{
#if defined(__micron_arch_arm64)
  asm volatile("msr tpidr_el0, %0" ::"r"(tp));
  return true;
#elif defined(__micron_arch_amd64) && defined(__micron_x86_fsgsbase)
  asm volatile("wrfsbase %0" ::"r"(tp));
  return true;
#else
  return false;
#endif
}

#if defined(__micron_arch_x86_any)
[[nodiscard, gnu::always_inline]] inline u16
code_segment() noexcept
{
  u16 s;
  asm volatile("mov %%cs, %0" : "=r"(s));
  return s;
}

[[nodiscard, gnu::always_inline]] inline u16
data_segment() noexcept
{
  u16 s;
  asm volatile("mov %%ds, %0" : "=r"(s));
  return s;
}

[[nodiscard, gnu::always_inline]] inline u16
extra_segment() noexcept
{
  u16 s;
  asm volatile("mov %%es, %0" : "=r"(s));
  return s;
}

[[nodiscard, gnu::always_inline]] inline u16
stack_segment() noexcept
{
  u16 s;
  asm volatile("mov %%ss, %0" : "=r"(s));
  return s;
}

[[nodiscard, gnu::always_inline]] inline u16
fs_segment() noexcept
{
  u16 s;
  asm volatile("mov %%fs, %0" : "=r"(s));
  return s;
}

[[nodiscard, gnu::always_inline]] inline u16
gs_segment() noexcept
{
  u16 s;
  asm volatile("mov %%gs, %0" : "=r"(s));
  return s;
}
#endif

struct record {
  void *sp;      // stack pointer
  void *fp;      // frame (base) pointer
  void *pc;      // program counter / return address
};

[[nodiscard, gnu::always_inline]] inline record
here() noexcept
{
  return record{ stack_pointer(), frame_pointer(), program_counter() };
}

// NOTE: REQUIRES the walked code be built with -fno-omit-frame-pointer
[[gnu::always_inline]] inline bool
caller(record &r) noexcept
{
  if ( r.fp == nullptr ) return false;
  void **fp = static_cast<void **>(r.fp);
  void *saved_fp = fp[0];      // caller's frame pointer
  void *ret = fp[1];           // return address
  // frames grow toward higher addresses up the chain; reject garbage / cycles
  if ( saved_fp == nullptr || saved_fp <= r.fp ) return false;
  r.sp = static_cast<void *>(fp + 2);
  r.fp = saved_fp;
  r.pc = ret;
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dangerous code below

// mutate the live stack pointer; only sound/safe in SPECIFIC use cases (coroutines/fibers/not returing from old frames); calling fn must be
// naked or pure asm almost always
[[gnu::always_inline]] inline void
set_stack_pointer(void *sp) noexcept
{
#if defined(__micron_arch_amd64)
  asm volatile("movq %0, %%rsp" ::"r"(sp) : "memory");
#elif defined(__micron_arch_x86)
  asm volatile("movl %0, %%esp" ::"r"(sp) : "memory");
#elif defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
  asm volatile("mov sp, %0" ::"r"(sp) : "memory");
#endif
}

inline __attribute__((noinline)) void
call_on_stack([[maybe_unused]] void *stack_top, [[maybe_unused]] void (*fn)(void *), [[maybe_unused]] void *arg) noexcept
{
#if defined(__micron_arch_amd64)
  asm volatile("movq %%rsp, %%r15\n\t"      // save old sp (callee-saved, preserved by fn)
               "andq $-16, %0\n\t"
               "movq %0, %%rsp\n\t"         // pivot
               "movq %2, %%rdi\n\t"         // arg -> a1
               "callq *%1\n\t"              // fn(arg)
               "movq %%r15, %%rsp\n\t"      // restore
               : "+r"(stack_top)
               : "r"(fn), "r"(arg)
               : "r15", "rdi", "rax", "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory", "cc");
#elif defined(__micron_arch_x86)
  asm volatile("movl %%esp, %%edi\n\t"      // save old sp (edi callee-saved on i386)
               "andl $-16, %0\n\t"
               "movl %0, %%esp\n\t"
               "subl $12, %%esp\n\t"      // keep 16-byte alignment after the arg push
               "pushl %2\n\t"             // arg
               "calll *%1\n\t"
               "movl %%edi, %%esp\n\t"      // restore
               : "+r"(stack_top)
               : "r"(fn), "r"(arg)
               : "edi", "eax", "ecx", "edx", "memory", "cc");
#elif defined(__micron_arch_arm64)
  asm volatile("mov x19, sp\n\t"      // save old sp (callee-saved)
               "and %0, %0, #-16\n\t"
               "mov sp, %0\n\t"
               "mov x0, %2\n\t"       // arg
               "blr %1\n\t"           // fn(arg)
               "mov sp, x19\n\t"      // restore
               : "+r"(stack_top)
               : "r"(fn), "r"(arg)
               : "x19", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17",
                 "x18", "x30", "memory", "cc");
#elif defined(__micron_arch_arm32)
  asm volatile("mov r4, sp\n\t"      // save old sp (callee-saved)
               "and %0, %0, #-8\n\t"
               "mov sp, %0\n\t"
               "mov r0, %2\n\t"      // arg
               "blx %1\n\t"          // fn(arg)
               "mov sp, r4\n\t"      // restore
               : "+r"(stack_top)
               : "r"(fn), "r"(arg)
               : "r4", "r0", "r1", "r2", "r3", "r12", "lr", "memory", "cc");
#endif
}

// fiber CPU contexts

#if defined(__micron_arch_amd64)

struct __fiber_ctx {
  u64 rbx;        // 0x00
  u64 rbp;        // 0x08
  u64 r12;        // 0x10  carries fn-arg into the trampoline (make_context)
  u64 r13;        // 0x18  carries fn-ptr into the trampoline (make_context)
  u64 r14;        // 0x20
  u64 r15;        // 0x28
  u64 rsp;        // 0x30
  u64 rip;        // 0x38  resume PC (trampoline for a fresh ctx; return addr for a suspended one)
  u32 mxcsr;      // 0x40  SSE control/status (callee-saved bits)
  u16 fcw;        // 0x44  x87 control word
};

static_assert(__builtin_offsetof(__fiber_ctx, rsp) == 0x30, "amd64 __fiber_ctx rsp offset");
static_assert(__builtin_offsetof(__fiber_ctx, rip) == 0x38, "amd64 __fiber_ctx rip offset");
static_assert(__builtin_offsetof(__fiber_ctx, mxcsr) == 0x40, "amd64 __fiber_ctx mxcsr offset");
static_assert(__builtin_offsetof(__fiber_ctx, fcw) == 0x44, "amd64 __fiber_ctx fcw offset");

[[maybe_unused]] static __attribute__((naked, noinline)) void
__micron_swap_context(__fiber_ctx * /*rdi from*/, __fiber_ctx * /*rsi to*/) noexcept
{
  asm volatile("movq %rbx, 0x00(%rdi)\n\t"
               "movq %rbp, 0x08(%rdi)\n\t"
               "movq %r12, 0x10(%rdi)\n\t"
               "movq %r13, 0x18(%rdi)\n\t"
               "movq %r14, 0x20(%rdi)\n\t"
               "movq %r15, 0x28(%rdi)\n\t"
               "movq (%rsp), %rax\n\t"      // return address -> from->rip
               "movq %rax, 0x38(%rdi)\n\t"
               "leaq 8(%rsp), %rax\n\t"      // caller SP past the return addr (16-aligned) -> from->rsp
               "movq %rax, 0x30(%rdi)\n\t"
               "stmxcsr 0x40(%rdi)\n\t"
               "fnstcw 0x44(%rdi)\n\t"
               "ldmxcsr 0x40(%rsi)\n\t"      // restore *to
               "fldcw 0x44(%rsi)\n\t"
               "movq 0x00(%rsi), %rbx\n\t"
               "movq 0x08(%rsi), %rbp\n\t"
               "movq 0x10(%rsi), %r12\n\t"
               "movq 0x18(%rsi), %r13\n\t"
               "movq 0x20(%rsi), %r14\n\t"
               "movq 0x28(%rsi), %r15\n\t"
               "movq 0x30(%rsi), %rsp\n\t"
               "jmp *0x38(%rsi)\n\t");
}

[[maybe_unused]] static __attribute__((naked, noinline, noreturn)) void
__micron_fiber_trampoline() noexcept
{
  asm volatile("andq $-16, %rsp\n\t"      // defensive 16-align
               "xorl %ebp, %ebp\n\t"      // terminate the unwind/backtrace chain
               "movq %r12, %rdi\n\t"      // arg -> a0
               "callq *%r13\n\t"          // fn(arg)
               "ud2\n\t");                // fn must not return
}

[[gnu::always_inline]] inline void
make_context(__fiber_ctx *ctx, void *stack_top, void (*fn)(void *), void *arg) noexcept
{
  *ctx = __fiber_ctx{};
  ctx->rsp = reinterpret_cast<u64>(stack_top) & ~static_cast<u64>(15);
  ctx->rip = reinterpret_cast<u64>(&__micron_fiber_trampoline);
  ctx->r12 = reinterpret_cast<u64>(arg);
  ctx->r13 = reinterpret_cast<u64>(fn);
  ctx->mxcsr = 0x1F80;      // default: all SSE exceptions masked, round-to-nearest
  ctx->fcw = 0x037F;        // default x87 control word
}

#elif defined(__micron_arch_x86)

struct __fiber_ctx {
  u32 ebx;        // 0x00
  u32 esi;        // 0x04  carries fn-arg into the trampoline
  u32 edi;        // 0x08  carries fn-ptr into the trampoline
  u32 ebp;        // 0x0c
  u32 esp;        // 0x10
  u32 eip;        // 0x14  resume PC
  u32 mxcsr;      // 0x18  (only meaningful with SSE)
  u16 fcw;        // 0x1c  x87 control word
};

[[maybe_unused]] static __attribute__((naked, noinline)) void
__micron_swap_context(__fiber_ctx *, __fiber_ctx *) noexcept
{
  // cdecl: from @ 4(%esp), to @ 8(%esp)
  asm volatile("movl 4(%esp), %eax\n\t"      // from
               "movl %ebx, 0x00(%eax)\n\t"
               "movl %esi, 0x04(%eax)\n\t"
               "movl %edi, 0x08(%eax)\n\t"
               "movl %ebp, 0x0c(%eax)\n\t"
               "movl (%esp), %ecx\n\t"      // return addr -> from->eip
               "movl %ecx, 0x14(%eax)\n\t"
               "leal 4(%esp), %ecx\n\t"      // caller esp past ret -> from->esp
               "movl %ecx, 0x10(%eax)\n\t"
#if defined(__SSE__)
               "stmxcsr 0x18(%eax)\n\t"
#endif
               "fnstcw 0x1c(%eax)\n\t"
               "movl 8(%esp), %eax\n\t"      // to (esp still original here)
#if defined(__SSE__)
               "ldmxcsr 0x18(%eax)\n\t"
#endif
               "fldcw 0x1c(%eax)\n\t"
               "movl 0x00(%eax), %ebx\n\t"
               "movl 0x04(%eax), %esi\n\t"
               "movl 0x08(%eax), %edi\n\t"
               "movl 0x0c(%eax), %ebp\n\t"
               "movl 0x10(%eax), %esp\n\t"
               "jmp *0x14(%eax)\n\t");
}

[[maybe_unused]] static __attribute__((naked, noinline, noreturn)) void
__micron_fiber_trampoline() noexcept
{
  asm volatile("andl $-16, %esp\n\t"
               "xorl %ebp, %ebp\n\t"
               "subl $12, %esp\n\t"      // keep 16-byte alignment after the arg push
               "pushl %esi\n\t"          // arg
               "calll *%edi\n\t"         // fn(arg)
               "ud2\n\t");
}

[[gnu::always_inline]] inline void
make_context(__fiber_ctx *ctx, void *stack_top, void (*fn)(void *), void *arg) noexcept
{
  *ctx = __fiber_ctx{};
  ctx->esp = reinterpret_cast<u32>(stack_top) & ~static_cast<u32>(15);
  ctx->eip = reinterpret_cast<u32>(&__micron_fiber_trampoline);
  ctx->esi = reinterpret_cast<u32>(arg);
  ctx->edi = reinterpret_cast<u32>(fn);
  ctx->mxcsr = 0x1F80;
  ctx->fcw = 0x037F;
}

#elif defined(__micron_arch_arm64)

struct __fiber_ctx {
  u64 x[12];      // 0x00  x19..x30 ; x[10]=x29(fp) x[11]=x30(lr/resume) ; x[0]=x19 arg, x[1]=x20 fn (make_context)
  u64 sp;         // 0x60
  u64 d[8];       // 0x68  d8..d15 (callee-saved low 64 bits, AAPCS64)
};

static_assert(__builtin_offsetof(__fiber_ctx, sp) == 0x60, "arm64 __fiber_ctx sp offset");
static_assert(__builtin_offsetof(__fiber_ctx, d) == 0x68, "arm64 __fiber_ctx d offset");

// aarch64 GCC IGNORES __attribute__((naked)); emit both routines as top-level asm (see clone.hpp:99)
extern "C" void __micron_swap_context(__fiber_ctx *from, __fiber_ctx *to) noexcept;
extern "C" void __micron_fiber_trampoline() noexcept;

}      // namespace ar
}      // namespace micron

asm(".text\n\t"
    ".weak __micron_swap_context\n\t"
    ".type __micron_swap_context, %function\n\t"
    ".p2align 2\n"
    "__micron_swap_context:\n\t"
    // x0 = from, x1 = to
    "stp x19, x20, [x0, #0x00]\n\t"
    "stp x21, x22, [x0, #0x10]\n\t"
    "stp x23, x24, [x0, #0x20]\n\t"
    "stp x25, x26, [x0, #0x30]\n\t"
    "stp x27, x28, [x0, #0x40]\n\t"
    "stp x29, x30, [x0, #0x50]\n\t"      // fp, lr (lr = resume pc)
    "mov x2, sp\n\t"
    "str x2, [x0, #0x60]\n\t"
    "stp d8, d9,   [x0, #0x68]\n\t"
    "stp d10, d11, [x0, #0x78]\n\t"
    "stp d12, d13, [x0, #0x88]\n\t"
    "stp d14, d15, [x0, #0x98]\n\t"
    "ldp x19, x20, [x1, #0x00]\n\t"      // restore *to
    "ldp x21, x22, [x1, #0x10]\n\t"
    "ldp x23, x24, [x1, #0x20]\n\t"
    "ldp x25, x26, [x1, #0x30]\n\t"
    "ldp x27, x28, [x1, #0x40]\n\t"
    "ldp x29, x30, [x1, #0x50]\n\t"
    "ldr x2, [x1, #0x60]\n\t"
    "mov sp, x2\n\t"
    "ldp d8, d9,   [x1, #0x68]\n\t"
    "ldp d10, d11, [x1, #0x78]\n\t"
    "ldp d12, d13, [x1, #0x88]\n\t"
    "ldp d14, d15, [x1, #0x98]\n\t"
    "ret\n\t");      // branch to x30 (resume pc)

asm(".text\n\t"
    ".weak __micron_fiber_trampoline\n\t"
    ".type __micron_fiber_trampoline, %function\n\t"
    ".p2align 2\n"
    "__micron_fiber_trampoline:\n\t"
    "mov x29, #0\n\t"      // terminate unwind chain
    "mov x0, x19\n\t"      // arg -> a0
    "blr x20\n\t"          // fn(arg)
    "brk #0\n\t");

namespace micron
{
namespace ar
{

[[gnu::always_inline]] inline void
make_context(__fiber_ctx *ctx, void *stack_top, void (*fn)(void *), void *arg) noexcept
{
  *ctx = __fiber_ctx{};
  ctx->sp = reinterpret_cast<u64>(stack_top) & ~static_cast<u64>(15);
  ctx->x[11] = reinterpret_cast<u64>(&__micron_fiber_trampoline);      // x30 = entry
  ctx->x[0] = reinterpret_cast<u64>(arg);                              // x19
  ctx->x[1] = reinterpret_cast<u64>(fn);                               // x20
}

#elif defined(__micron_arch_arm32)

struct __fiber_ctx {
  u32 r[8];      // 0x00  r4..r11 ; r[0]=r4 arg, r[1]=r5 fn (make_context)
  u32 sp;        // 0x20
  u32 lr;        // 0x24  resume pc
  u64 d[8];      // 0x28  d8..d15 (guarded by __ARM_FP)
};

[[maybe_unused]] static __attribute__((naked, noinline)) void
__micron_swap_context(__fiber_ctx *, __fiber_ctx *) noexcept
{
  // r0 = from, r1 = to
  asm volatile("stm r0, {r4-r11}\n\t"      // r4..r11 at 0x00..0x1c
               "str sp, [r0, #0x20]\n\t"
               "str lr, [r0, #0x24]\n\t"
#if defined(__ARM_FP)
               "add r2, r0, #0x28\n\t"
               "vstm r2, {d8-d15}\n\t"      // callee-saved VFP regs
               "add r2, r1, #0x28\n\t"
               "vldm r2, {d8-d15}\n\t"
#endif
               "ldr sp, [r1, #0x20]\n\t"
               "ldr lr, [r1, #0x24]\n\t"
               "ldm r1, {r4-r11}\n\t"
               "bx lr\n\t");
}

[[maybe_unused]] static __attribute__((naked, noinline, noreturn)) void
__micron_fiber_trampoline() noexcept
{
  asm volatile("mov r11, #0\n\t"      // terminate unwind chain
               "mov r0, r4\n\t"       // arg -> a0
               "blx r5\n\t"           // fn(arg)
               "udf #0\n\t");
}

[[gnu::always_inline]] inline void
make_context(__fiber_ctx *ctx, void *stack_top, void (*fn)(void *), void *arg) noexcept
{
  *ctx = __fiber_ctx{};
  ctx->sp = reinterpret_cast<u32>(stack_top) & ~static_cast<u32>(7);
  ctx->lr = reinterpret_cast<u32>(&__micron_fiber_trampoline);
  ctx->r[0] = reinterpret_cast<u32>(arg);      // r4
  ctx->r[1] = reinterpret_cast<u32>(fn);       // r5
}

#endif

};      // namespace ar
};      // namespace micron
