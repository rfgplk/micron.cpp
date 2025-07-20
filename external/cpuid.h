//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// nanoflagsx86 (for C99 and onwards)
// https://github.com/rfgplk/nanoflagsx86
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#pragma once

#include "macros.h"

BEGIN_C_NS

#define __cpuid(level, a, b, c, d) __asm__("cpuid\n\t" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(level))

#define __cpuid_count(level, count, a, b, c, d)                                                                         \
  __asm__("cpuid\n\t" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(level), "2"(count))

#define __rdtscp(rax, rdx, aux) __asm__ volatile("rdtscp\n" : "=a"(rax), "=d"(rdx), "=c"(aux) : :);

typedef struct cstruct {
  unsigned long eax;
  unsigned long ebx;
  unsigned long ecx;
  unsigned long edx;
} cstruct_t;

__inline__ void
cpuid(const unsigned long num, cstruct_t *const ptr)
{
  __cpuid(num, ptr->eax, ptr->ebx, ptr->ecx, ptr->edx);
}

__inline__ void
cpuid_c(const unsigned long num, const unsigned long c, cstruct_t *const ptr)
{
  __cpuid_count(num, c, ptr->eax, ptr->ebx, ptr->ecx, ptr->edx);
}

__inline__ unsigned int
rbit(const unsigned long r, const unsigned long from, const unsigned long to)
{
  const unsigned long long bits = from - to + 1;
  const unsigned long long mask = (1ULL << bits) - 1ULL;
  return static_cast<unsigned int>((r >> to) & mask);
  // return (r >> to) & ((1ULL << ((from - to + 1) - 1ULL)));
}
__inline__ char
bit(const unsigned long r, const unsigned long b)
{
  return 0x01 & (r >> b);
}

char
have_cpuid(void)
{
  unsigned int eax = 0, ebx = 0;
#ifndef __x86_64__
  /* See if we can use cpuid.  On AMD64 we always can.  */
  __asm__("pushf{l|d}\n\t"
          "pushf{l|d}\n\t"
          "pop{l}\t%0\n\t"
          "mov{l}\t{%0, %1|%1, %0}\n\t"
          "xor{l}\t{%2, %0|%0, %2}\n\t"
          "push{l}\t%0\n\t"
          "popf{l|d}\n\t"
          "pushf{l|d}\n\t"
          "pop{l}\t%0\n\t"
          "popf{l|d}\n\t"
          : "=&r"(eax), "=&r"(ebx)
          : "i"(0x00200000));
#endif

  if ( !((eax ^ ebx) & 0x00200000) )
    return 0;
  return 1;
}

long unsigned int
maximum_leaf(void)
{
  cstruct_t cs = { .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };
  cpuid(0x00, &cs);
  return cs.eax;
}

END_C_NS
