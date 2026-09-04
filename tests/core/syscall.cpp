//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::syscall -- the raw entry, exercised through mmap/munmap.
//
// The mmap bit constants are micron::{prot_read,prot_write,map_private,map_anonymous}
// (src/memory/mmap_bits.hpp:13,14,24,35), NOT the libc uppercase spellings.

#include "../../src/syscall.hpp"
#include "../../src/io/console.hpp"
#include "../../src/memory/mman.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  test_case("raw SYS_mmap round-trips through micron::syscall");
  {
    const long raw = mc::syscall(SYS_mmap, nullptr, 4096, mc::prot_read | mc::prot_write, mc::map_private | mc::map_anonymous, -1, 0);
    // the kernel answers -errno in the return register, never MAP_FAILED
    require_true(raw > 0);

    byte *ptr = reinterpret_cast<byte *>(raw);
    require_true(ptr != reinterpret_cast<const byte *>(mc::map_failed));
    require_true((reinterpret_cast<uintptr_t>(ptr) & 0xfff) == 0);      // page aligned

    ptr[0] = 'H';
    ptr[1] = 'e';
    ptr[2] = 'l';
    ptr[3] = 'l';
    ptr[4] = 'o';
    ptr[5] = 0x0;
    require_true(mc::strcmp(reinterpret_cast<const char *>(ptr), "Hello") == 0);

    // the tail of a fresh anonymous page is zero-filled by the kernel
    require_true(ptr[6] == 0 && ptr[4095] == 0);

    require_true(mc::munmap(mc::ptr_cast<addr_t *>(ptr), 4096) == 0);
  }
  end_test_case();

  test_case("a bad syscall answers -errno rather than trapping");
  {
    // length 0 is EINVAL for mmap; the raw entry must surface it as a negative return
    const long bad = mc::syscall(SYS_mmap, nullptr, 0, mc::prot_read, mc::map_private | mc::map_anonymous, -1, 0);
    require_true(bad < 0);
  }
  end_test_case();

  sb::print("=== ALL SYSCALL TESTS PASSED ===");
  return 1;
}
