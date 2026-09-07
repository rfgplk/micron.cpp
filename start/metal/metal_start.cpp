//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __micron_metalc

#include "../__crt.hpp"
#include <micron/port/init.hpp>
#include <micron/port/panic.hpp>

#if !defined(MICRON_METAL_ENTRY)
#define MICRON_METAL_ENTRY "metal_main"
#endif

extern "C" int __micron_metal_user_entry(void) __asm__(MICRON_METAL_ENTRY);

namespace
{

void
__mc_hex(unsigned long __v, unsigned __digits) noexcept
{
  char __b[19];
  __b[0] = '0';
  __b[1] = 'x';
  for ( unsigned __i = 0; __i < __digits; ++__i ) {
    const unsigned __nib = static_cast<unsigned>((__v >> ((__digits - 1 - __i) * 4)) & 0xF);
    __b[2 + __i] = static_cast<char>(__nib < 10 ? '0' + __nib : 'a' + (__nib - 10));
  }
  micron::port::write_diag(__b, 2 + __digits);
}

};      // namespace

extern "C" void
__mc_metal_fault_report(const char *__n0, unsigned long __v0, const char *__n1, unsigned long __v1, const char *__n2,
                        unsigned long __v2) noexcept
{
  const unsigned __d = sizeof(unsigned long) * 2;
  micron::port::write_diag("micron-metal: FAULT ");
  micron::port::write_diag(__n0);
  micron::port::write_diag("=");
  __mc_hex(__v0, __d);
  micron::port::write_diag(" ");
  micron::port::write_diag(__n1);
  micron::port::write_diag("=");
  __mc_hex(__v1, __d);
  micron::port::write_diag(" ");
  micron::port::write_diag(__n2);
  micron::port::write_diag("=");
  __mc_hex(__v2, __d);
  micron::port::write_diag("\n");
}

extern "C" [[noreturn]] void
__micron_metalc(void)
{
  micron::port::run_init_array(__preinit_array_start, __preinit_array_end);
  micron::port::run_init_array(__init_array_start, __init_array_end);

  const int __rc = __micron_metal_user_entry();

  micron::port::run_fini_array(__fini_array_start, __fini_array_end);

  micron::port::halt(__rc);
}
