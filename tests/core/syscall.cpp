//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/syscall.hpp"
#include "../../src/io/console.hpp"
#include "../../src/memory/mman.hpp"
#include "../../src/std.hpp"
int
main(void)
{
  byte* ptr =  reinterpret_cast<byte *>(mc::syscall(SYS_mmap, nullptr, 4096, mc::PROT_READ | mc::PROT_WRITE, mc::MAP_PRIVATE | mc::MAP_ANONYMOUS, -1, 0));
  ptr[0] = 'H';
  ptr[1] = 'e';
  ptr[2] = 'l';
  ptr[3] = 'l';
  ptr[4] = 'o';
  ptr[5] = 0x0;
  mc::console(reinterpret_cast<const char*>(ptr));
  return 1;
}
