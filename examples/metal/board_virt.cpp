//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BOARD PORT: qemu's `-M virt`, for BOTH armv7-a and aarch64
//
// One file serves both because the machine is the same machine: a PL011 UART at 0x09000000, RAM at
// 0x40000000 -- which is what metal_arm32.ld and metal_arm64.ld already assume -- and the
// architected generic timer. Only the two privileged instruction sequences differ, and each is four
// lines.
//
// THIS IS A QEMU BOARD, AND SAYS SO. mc_mport_halt goes out through SEMIHOSTING, which is a
// debugger protocol, not a machine feature: the call is trapped by the host only when qemu is
// started with -semihosting-config enable=on. Without that flag it is an undefined instruction, and
// with no vector table installed that is a silent lockup rather than a message.
// examples/metal/Makefile always passes the flag; a board port for real hardware replaces this
// hook, which is the entire point of the hooks being weak.
//
// AND THE IMMEDIATE IS NOT THE SAME IN BOTH INSTRUCTION SETS. ARM state takes `svc 0x00123456`,
// Thumb state takes `svc 0xAB` -- the Thumb encoding has an 8-bit field and cannot hold the other
// one. This matters here and not in theory: the Linaro toolchain duck drives for --arm defaults to
// THUMB, so the arm32 metal build assembles this file in Thumb state, and the ARM-state spelling is
// a hard assembler error ("value of 00123456 too large for field of 2 bytes"). It is the same
// reason duck has a --marm flag at all.
//
// Semihosting rather than PSCI SYSTEM_OFF, which would also work and needs no flag, because PSCI
// cannot carry an exit code and micron's PASS sentinel is a number. A test that can only report
// "stopped" is not a test.

#include "../../src/port/backends/__mport_abi.hpp"

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the PL011 at qemu virt's fixed address
//
// qemu's model transmits on any write to UARTDR with no initialisation at all, but the FR poll is
// kept: it is what real silicon requires, and a board file that only works because the model is
// forgiving teaches the wrong thing.

constexpr mc_mport_addr kPl011 = 0x09000000u;
constexpr unsigned kUartDr = 0x000;      // data
constexpr unsigned kUartFr = 0x018;      // flag; bit 5 is TXFF
constexpr unsigned kTxFull = 1u << 5;

inline volatile mc_mport_u32 *
pl011(unsigned __off) noexcept
{
  return reinterpret_cast<volatile mc_mport_u32 *>(kPl011 + __off);
}

inline void
uart_put(char __c) noexcept
{
  while ( (*pl011(kUartFr) & kTxFull) != 0 ) { }
  *pl011(kUartDr) = static_cast<mc_mport_u32>(static_cast<unsigned char>(__c));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the architected generic timer
//
// This is the whole reason to override mc_mport_mono_ns: the weak default in start/metal/mc_mport.cpp
// returns ++counter, one nanosecond per call, which makes port::sleep_ns() a spin of literally that
// many iterations. Both arches have a real counter and it costs two instructions to read.

[[gnu::always_inline]] inline mc_mport_i64
cnt_ticks(void) noexcept
{
#if defined(__aarch64__)
  mc_mport_i64 __v;
  __asm__ __volatile__("isb\n\tmrs %0, cntvct_el0" : "=r"(__v) : : "memory");
  return __v;
#else
  mc_mport_u32 __lo, __hi;
  __asm__ __volatile__("isb\n\tmrrc p15, 1, %0, %1, c14" : "=r"(__lo), "=r"(__hi) : : "memory");
  return static_cast<mc_mport_i64>((static_cast<unsigned long long>(__hi) << 32) | __lo);
#endif
}

[[gnu::always_inline]] inline mc_mport_u32
cnt_freq(void) noexcept
{
#if defined(__aarch64__)
  mc_mport_i64 __v;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(__v));
  return static_cast<mc_mport_u32>(__v);
#else
  mc_mport_u32 __v;
  __asm__ __volatile__("mrc p15, 0, %0, c14, c0, 0" : "=r"(__v));
  return __v;
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// semihosting

constexpr unsigned kSysExitExtended = 0x20;
constexpr unsigned long kAdpStoppedApplicationExit = 0x20026ul;

[[noreturn]] inline void
semihost_exit(int __code) noexcept
{
  volatile unsigned long __blk[2] = { kAdpStoppedApplicationExit, static_cast<unsigned long>(__code) };
#if defined(__aarch64__)
  register unsigned long __x0 asm("x0") = kSysExitExtended;
  register const volatile unsigned long *__x1 asm("x1") = __blk;
  __asm__ __volatile__("hlt #0xF000" : : "r"(__x0), "r"(__x1) : "memory");
#else
  register unsigned long __r0 asm("r0") = kSysExitExtended;
  register const volatile unsigned long *__r1 asm("r1") = __blk;
#if defined(__thumb__)
  __asm__ __volatile__("svc 0xAB" : : "r"(__r0), "r"(__r1) : "memory");
#else
  __asm__ __volatile__("svc 0x00123456" : : "r"(__r0), "r"(__r1) : "memory");
#endif
#endif
  // qemu does not return from that. If semihosting was not enabled it is an undefined instruction
  // and control never arrives here either -- park regardless, so a returning host cannot fall into
  // whatever follows in .text.
  for ( ;; ) __asm__ __volatile__("wfi" ::: "memory");
}

};      // namespace

extern "C" {

void
mc_mport_write_diag(const char *__s, mc_mport_usize __n)
{
  for ( mc_mport_usize __i = 0; __i < __n; ++__i ) {
    // a bare LF is what a terminal expects from a UART; qemu's -serial does not translate
    if ( __s[__i] == '\n' ) uart_put('\r');
    uart_put(__s[__i]);
  }
}

[[noreturn]] void
mc_mport_halt(int __code)
{
  semihost_exit(__code);
}

mc_mport_i64
mc_mport_mono_ns(void)
{
  const mc_mport_u32 __f = cnt_freq();
  if ( __f == 0 ) return 0;
  const mc_mport_i64 __t = cnt_ticks();
  // split rather than (t * 1000000000) / f: qemu virt counts at 62.5 MHz, and the naive form
  // overflows a signed 64-bit product after about 150 seconds of uptime. This form is exact and
  // cannot overflow for any counter value the hardware can hold.
  const mc_mport_i64 __whole = __t / static_cast<mc_mport_i64>(__f);
  const mc_mport_i64 __part = __t % static_cast<mc_mport_i64>(__f);
  return __whole * 1000000000LL + (__part * 1000000000LL) / static_cast<mc_mport_i64>(__f);
}

};      // extern "C"
