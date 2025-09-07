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

#include <unistd.h>

#include "features.h"
#include "macros.h"

#define CACHE_NULL 0x00
// #define iTLB_4KB_PAGE_32e 0x01
#define iTLB_4MB_PAGE_2e 0x02
#define dTLB_4KB_PAGE_64e 0x03
#define dTLB_4MB_PAGE_8e 0x04
#define dTLB_4MB_PAGE_32e 0x05
#define L1_iCACHE_8KB_32b 0x06
#define L1_iCACHE_16KB_32b 0x08
#define L1_iCACHE_32KB_64b 0x09
#define L1_CACHE_8KB_32b 0x0A
#define iTLB_4MB_PAGE_4e 0x0B
#define L1_CACHE_16KB_32b 0x0C
#define L1_CACHE_16KB_64b 0x0D
#define L1_CACHE_24KB_64b 0x0E
#define L2_CACHE_256KB_64b 0x21
#define L3_CACHE_512KB_64b 0x22
#define L3_CACHE_1MB_64b 0x23
#define L3_CACHE_2MB_64b 0x25
#define L3_CACHE_4MB_64b 0x29
#define L1_CACHE_32KB_64b 0x2C
#define L1_iCACHE_64b 0x30
#define NO_CACHE_N 0x40
#define L2_CACHE_128KB_32b 0x41
#define L2_CACHE_256KB_32b 0x42
#define L2_CACHE_512KB_32b 0x43
#define L2_CACHE_1MB_32b 0x44
#define L2_CACHE_2MB_32b 0x45
#define L3_CACHE_4MB_64bf 0x46
#define L3_CACHE_8MB_64bf 0x47
#define L2_CACHE_3MB_64bu 0x48
#define L2_4MB_64b_A_L3_4MB_64b 0x49
#define L3_CACHE_6MB_12w_64b 0x4A
#define L3_CACHE_8MB_16w_64b 0x4B
#define L3_CACHE_12MB_12w_64b 0x4C
#define L3_CACHE_16MB_16w_64b 0x4D
#define L2_CACHE_6MB_24w_64b 0x4E
#define iTLB_4KB_PAGE_32e 0x4F
#define iTLB_4KB_OR_PAGE_64e 0x50
#define iTLB_4KB_OR_PAGE_128e 0x51
#define iTLB_4KB_OR_PAGE_256e 0x52
#define iTLB_2MB_OR_PAGE_7e 0x55
#define L1_TLB_4MB_PAGE_4w_sa_16e 0x56
#define L1_TLB_4KB_PAGE_4w_sa_16e 0x57
#define dTLB0_4KB_PAGE_16e 0x59
#define dTLB0_2MB_OR_PAGE_5w_a_32e 0x5A
#define dTLB_4KB_OR_PAGE_fa_64e 0x5B
#define dTLB_4KB_OR_PAGE_fa_128e 0x5C
#define dTLB_4KB_OR_PAGE_fa_256e 0x5D
#define L1_CACHE_16KB_8w_sc_64b 0x60
#define L1_CACHE_8KB_4w_sc_64b 0x66
#define L1_CACHE_16KB_4w_sc_64b 0x67
#define L1_CACHE_32KB_4w_sc_64b 0x68
#define TRACE_CACHE_12Kops_8w_sa 0x70
#define TRACE_CACHE_16Kops_8w_sa 0x71
#define TRACE_CACHE_32Kops_8w_sa 0x72
#define iTLB_2MB_PAGE_8e 0x76
#define L2_CACHE_1MB_4w_64b 0x78
#define L2_CACHE_128KB_8w_sc_64b 0x79
#define L2_CACHE_256KB_8w_sc_64b 0x7A
#define L2_CACHE_512KB_8w_sc_64b 0x7B
#define L2_CACHE_1MB_8w_sc_64b 0x7C
#define L2_CACHE_2MB_8w_64 0x7D
#define L2_CACHE_512KB_2w_64b 0x7F
#define L2_CACHE_512KB_8w_sa_64b 0x80
#define L2_CACHE_256KB_8w_sa_32b 0x82
#define L2_CACHE_512KB_8w_sa_32b 0x83
#define L2_CACHE_1MB_8w_sa_32b 0x84
#define L2_CACHE_2MB_8w_sa_64b 0x85
#define L2_CACHE_256KB_4w_64b 0x86
#define L2_CACHE_1MB_8w_64b 0x87
#define iTLB_4KB_4w_128e 0xB0
#define iTLB_4KB_2MB_4w4w_8e_OR 0xB1
#define iTLB_4KB_4w_64e 0xB2
#define dTLB_4KB_4w_128e 0xB3
#define dTLB_4KB_4w_sa_256e 0xB4
#define dTLB_4KB_4w_sa_64e 0xBA
#define dTLB_4KB_OR4w_sa_8e 0xC0
#define SHARE_TLB 0xCA
#define L3_CACHE_512KB_4w_sa_64b 0xD0
#define L3_CACHE_1MB_4w_sa_64b 0xD1
#define L3_CACHE_2MB_4w_sa_64b 0xD2
#define L3_CACHE_1MB_8w_sa_64b 0xD6
#define L3_CACHE_2MB_8w_sa_64b 0xD7
#define L3_CACHE_4MB_8w_sa_64b 0xD8
#define L3_CACHE_1_5MB_12w_sa_64b 0xDC
#define L3_CACHE_3MB_12w_sa_64b 0xDD
#define L3_CACHE_6MB_12w_sa_64b 0xDE
#define L3_CACHE_2MB_16w_sa_64b 0xE2
#define L3_CACHE_4MB_16w_sa_64b 0xE3
#define L3_CACHE_8MB_16w_sa_64b 0xE4
#define L3_CACHE_12MB_24w_sa_64b 0xEA
#define L3_CACHE_18MB_24w_sa_64b 0xEB
#define L3_CACHE_24MB_24w_sa_64b 0xEC
#define PREFETCH_64b 0xF0
#define PREFETCH_128b 0xF1
#define CPUID_2_UNSUPPORT 0xFF

#ifdef x86FEATURES_CPUSET

#include <sched.h>
#include <sys/types.h>
#include <unistd.h>

int
topology_set_cpu(const unsigned long n)
{
  int pid = getpid();
  cpu_set_t cpu;
  if ( -1 == sched_getaffinity(pid, sizeof(cpu), &cpu) )
    return 0;
  CPU_ZERO(&cpu);
  CPU_SET(n, &cpu);
  if ( -1 == sched_setaffinity(pid, sizeof(cpu), &cpu) )
    return 0;
  return 1;
}
#else
__inline__ int
topology_set_cpu(const unsigned long n)
{
  (void)n;
  return 0;
}
#endif

inline cstruct_t
cache_sets(void)
{
  cstruct_t cs = { .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };
  cpuid(0x02, &cs);
  return cs;
}

inline void
cache(const char level, cache_t *const __restrict__ r)
{
  cstruct_t cs = { .eax = 0, .ebx = 0, .ecx = (unsigned char)level, .edx = 0 };
  cpuid_c(0x04, level, &cs);

  r->associative_cache = bit(cs.eax, 9);
  r->self_init = bit(cs.eax, 8);
  r->level = rbit(cs.eax, 7, 5);
  r->type = rbit(cs.eax, 4, 0);
  r->associativity = rbit(cs.ebx, 31, 22) + 1;
  r->phys_line = rbit(cs.ebx, 21, 12) + 1;
  r->coherency = rbit(cs.ebx, 11, 0) + 1;
  r->sets = rbit(cs.ecx, 31, 0) + 1;

  r->size = r->associativity * r->phys_line * r->coherency * r->sets;
}

void frequency_kernel(void);

__inline__ void
frequency_user(core_t (*ptr)[256])
{
  unsigned long long start, end, rax, rdx, aux;
  __rdtscp(rax, rdx, aux);
  start = (rdx << 32) + rax;
  sleep(1);
  __rdtscp(rax, rdx, aux);
  end = (rdx << 32) + rax;
  unsigned long long freq = (end - start);
  for ( int i = 0; i < 256; i++ )
    (*ptr)[i].freq = freq;
}

__inline__ void
cores_all(char *__restrict__ total, core_t (*ptr)[256])
{
  cstruct_t cs = { .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };
  unsigned int counter = 0;
  unsigned int tmp_c = 0;

  cpuid_c(0x04, 0x00, &cs);
  *total = rbit(cs.eax, 31, 26) + 1;
  while ( 1 ) {
    unsigned int tmp = *total;
    cpuid_c(0x0B, tmp_c++, &cs);
    char bits = rbit(cs.eax, 4, 0);
    char apic = rbit(cs.edx, 31, 0);

    if ( rbit(cs.eax, 4, 0) == 0 && rbit(cs.ebx, 15, 0) == 0 )
      break;
    while ( tmp-- ) {
      apic >>= bits;
      (*ptr)[counter].apic = apic;
      (*ptr)[counter++].type = tmp_c;
    }
  };
  for ( ; counter < 256; counter++ )
    (*ptr)[counter].type = 0x00;
}
