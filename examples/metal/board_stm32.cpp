//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BOARD PORT: STM32F4-class, under `qemu-system-arm -M netduinoplus2`
//
// An STM32F405: Cortex-M4, 192 KiB of SRAM, 1 MiB of flash. `-M netduino2` is an STM32F205 (M3,
// 128 KiB) and the same three hooks serve it -- USART1 is at the same address on both.
//
// WHAT A REAL BOARD HAS TO ADD, and it is not in here on purpose. Bringing USART1 up on silicon
// means enabling its clock in RCC, putting the TX pin into its alternate function through GPIOA,
// and programming BRR from the actual APB2 clock -- three peripherals whose register layout differs
// across the F1/F2/F4/F7/H7 families, in a file that is meant to show the seam rather than be a
// vendor HAL. qemu's model transmits on a write to DR regardless, so the poll on TXE below is the
// part that is real and the setup is the part a board fills in.
//
// mc_mport_halt goes out through SEMIHOSTING (`bkpt 0xAB`), which needs
// -semihosting-config enable=on. See board_virt.cpp for why semihosting and not something the
// hardware provides: it is the only exit that can carry micron's PASS sentinel as a number.

#include "../../src/port/backends/__mport_abi.hpp"

#if !defined(__ARM_ARCH_PROFILE) || (__ARM_ARCH_PROFILE != 'M')
#error "board_stm32.cpp is an ARMv7-M board port -- build it with duck --cortex-m"
#endif

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// USART1

constexpr mc_mport_addr kUsart1 = 0x40011000u;
constexpr unsigned kUsartSr = 0x00;      // status
constexpr unsigned kUsartDr = 0x04;      // data
constexpr unsigned kTxe = 1u << 7;       // transmit register empty

inline volatile mc_mport_u32 *
usart(unsigned __off) noexcept
{
  return reinterpret_cast<volatile mc_mport_u32 *>(kUsart1 + __off);
}

inline void
usart_put(char __c) noexcept
{
  while ( (*usart(kUsartSr) & kTxe) == 0 ) { }
  *usart(kUsartDr) = static_cast<mc_mport_u32>(static_cast<unsigned char>(__c));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the monotonic counter -- SysTick, NOT DWT_CYCCNT
//
// M-profile has no architected generic timer, so there are two candidates and the obvious one is
// wrong. DWT_CYCCNT is a free-running 32-bit count of core cycles and is what every bring-up guide
// reaches for -- but the DWT is an OPTIONAL debug unit. qemu's Cortex-M models do not implement it,
// and the failure is silent: DEMCR.TRCENA and DWT_CTRL.CYCCNTENA both accept the write, the counter
// reads 0 forever, and port::mono_ticks() becomes a constant. Measured here as
//
//     micron-metal: clock delta=0ns real=false
//
// which the demo only noticed because it asks whether the clock MOVED, not whether it reads.
//
// SysTick is part of the System Control Space and is architecturally present on every ARMv7-M. It
// is a 24-BIT DOWN-counter, so it needs both a wrap extension and a direction flip; that is the
// whole reason this is not the two instructions the A-profile board's CNTVCT read is.
//
// A board with an RTOS wants SysTick for its scheduler tick and should point this at one of the
// general-purpose timers instead. It is claimed here because a demo has no scheduler and SysTick is
// the one timer guaranteed to exist.

constexpr mc_mport_addr kSystCsr = 0xE000E010u;      // ENABLE|TICKINT|CLKSOURCE, COUNTFLAG at bit 16
constexpr mc_mport_addr kSystRvr = 0xE000E014u;      // reload, 24-bit
constexpr mc_mport_addr kSystCvr = 0xE000E018u;      // current
constexpr mc_mport_u32 kSystReload = 0x00FFFFFFu;
constexpr mc_mport_u32 kSystEnable = (1u << 0) | (1u << 2);      // ENABLE | CLKSOURCE=processor
constexpr mc_mport_u32 kSystCountFlag = 1u << 16;

// the default from the reset stub's point of view; a board that programs the PLL updates it
constexpr mc_mport_i64 kCoreHz = 168000000LL;

bool __syst_ready = false;
mc_mport_u32 __syst_last = 0;
mc_mport_i64 __syst_acc = 0;

inline volatile mc_mport_u32 *
reg(mc_mport_addr __a) noexcept
{
  return reinterpret_cast<volatile mc_mport_u32 *>(__a);
}

};      // namespace

extern "C" {

void
mc_mport_write_diag(const char *__s, mc_mport_usize __n)
{
  for ( mc_mport_usize __i = 0; __i < __n; ++__i ) {
    // a bare LF is what a terminal expects from a UART; qemu's -serial does not translate
    if ( __s[__i] == '\n' ) usart_put('\r');
    usart_put(__s[__i]);
  }
}

[[noreturn]] void
mc_mport_halt(int __code)
{
  volatile unsigned long __blk[2] = { 0x20026ul /* ADP_Stopped_ApplicationExit */, static_cast<unsigned long>(__code) };
  register unsigned long __r0 asm("r0") = 0x20;      // SYS_EXIT_EXTENDED
  register const volatile unsigned long *__r1 asm("r1") = __blk;
  // M-profile semihosting is BKPT 0xAB, not SVC -- an SVC here would take the SVCall vector and
  // land in Default_Handler instead of reaching the host
  __asm__ __volatile__("bkpt 0xAB" : : "r"(__r0), "r"(__r1) : "memory");
  for ( ;; ) __asm__ __volatile__("wfi" ::: "memory");
}

mc_mport_i64
mc_mport_mono_ns(void)
{
  if ( !__syst_ready ) {
    __syst_ready = true;
    *reg(kSystRvr) = kSystReload;
    *reg(kSystCvr) = 0;      // any write clears the counter AND clears COUNTFLAG
    *reg(kSystCsr) = kSystEnable;
    __syst_last = kSystReload;
    __syst_acc = 0;
  }

  // COUNTFLAG is read-to-clear, so it has to be read on EVERY call or a wrap is lost. It says "at
  // least one wrap since you last looked", and one is all this accounts for -- which holds because
  // mono_ticks() sits on the spin path of sleep_ns() and wait() and is called far faster than the
  // 24-bit period. A caller that slept a whole period and then asked once would undercount; a board
  // that needs that arms a 32-bit general-purpose timer instead.
  const bool __wrapped = (*reg(kSystCsr) & kSystCountFlag) != 0;
  const mc_mport_u32 __now = *reg(kSystCvr) & kSystReload;

  // it counts DOWN, so elapsed is last - now, and a wrap adds one full period
  mc_mport_u32 __elapsed = (__syst_last >= __now) ? (__syst_last - __now) : 0;
  if ( __wrapped || __syst_last < __now ) __elapsed = __syst_last + (kSystReload + 1u - __now);
  __syst_last = __now;
  __syst_acc += static_cast<mc_mport_i64>(__elapsed);

  // split rather than (cyc * 1000000000) / hz, which overflows a signed 64-bit product after about
  // 9 seconds at this clock. Exact, and it cannot overflow for any count the accumulator holds.
  return (__syst_acc / kCoreHz) * 1000000000LL + ((__syst_acc % kCoreHz) * 1000000000LL) / kCoreHz;
}

};      // extern "C"
