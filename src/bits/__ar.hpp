//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"

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

};      // namespace ar
};      // namespace micron
