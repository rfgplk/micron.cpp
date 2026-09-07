//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BOARD PORT: a PC-class target under qemu-system-x86_64 -kernel

#include "../../src/port/backends/__mport_abi.hpp"

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the 16550 UART at the PC's traditional COM1

constexpr unsigned short kCom1 = 0x3F8;

constexpr unsigned short kDebugExit = 0xF4;

inline void
outb(unsigned short __port, unsigned char __v) noexcept
{
  __asm__ __volatile__("outb %0, %1" : : "a"(__v), "Nd"(__port));
}

inline unsigned char
inb(unsigned short __port) noexcept
{
  unsigned char __r;
  __asm__ __volatile__("inb %1, %0" : "=a"(__r) : "Nd"(__port));
  return __r;
}

bool __uart_ready = false;

void
uart_init(void) noexcept
{
  if ( __uart_ready ) return;
  __uart_ready = true;
  outb(kCom1 + 1, 0x00);      // interrupts off -- there is no handler
  outb(kCom1 + 3, 0x80);      // DLAB: the next two writes are the divisor
  outb(kCom1 + 0, 0x03);      // 115200 / 3 = 38400 baud
  outb(kCom1 + 1, 0x00);
  outb(kCom1 + 3, 0x03);      // 8N1, DLAB off
  outb(kCom1 + 2, 0xC7);      // FIFO on, cleared, 14-byte threshold
  outb(kCom1 + 4, 0x0B);      // RTS/DSR set
}

};      // namespace

extern "C" {

void
mc_mport_write_diag(const char *__s, mc_mport_usize __n)
{
  uart_init();
  for ( mc_mport_usize __i = 0; __i < __n; ++__i ) {
    // a bare LF is what a terminal expects from a UART; qemu's -serial does not translate
    if ( __s[__i] == '\n' ) {
      while ( (inb(kCom1 + 5) & 0x20) == 0 ) { }
      outb(kCom1, '\r');
    }
    while ( (inb(kCom1 + 5) & 0x20) == 0 ) { }
    outb(kCom1, static_cast<unsigned char>(__s[__i]));
  }
}

[[noreturn]] void
mc_mport_halt(int __code)
{
  outb(kDebugExit, static_cast<unsigned char>(__code == 1 ? 0 : (__code & 0x7F)));
  for ( ;; ) __asm__ __volatile__("cli; hlt" ::: "memory");
}

};      // extern "C"
