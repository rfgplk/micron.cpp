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

enum vendors_t {
  GenuineIntel = 0xA,
  AuthenticAMD,
  AMDisbetter,
  CentaurHauls,
  CyrixInstead,
  TransmetaCPU,
  GenuineTMx86,
  Geode_by_NSC,
  NexGenDriven,
  RiseRiseRise,
  SiS_SiS_SiS_,
  UMC_UMC_UMC_,
  VIA_VIA_VIA_,
  Vortex86_SoC
};
enum architectures_t {
  i386,
  i486,
  i586,
  pentium,
  lakemont,
  pentium_mmx,
  pentiumpro,
  i686,
  pentium2,
  pentium3,
  pentium3m,
  pentium_m,
  pentium4,
  pentium4m,
  prescott,
  nocona,
  core2,
  nehalem,
  westmere,
  sandybridge,
  ivybridge,
  haswell,
  broadwell,
  skylake,
  bonnell,
  silvermont,
  knl,
  skylake_avx512,
  k6,
  k6_2,
  k6_3,
  athlon,
  athlon_tbird,
  athlon_4,
  athlon_xp,
  athlon_mp,
  k8,
  opteron,
  athlon64,
  athlon_fx,
  k8_sse3,
  opteron_sse3,
  athlon64_sse3,
  amdfam10,
  barcelona,
  bdver1,
  bdbver2,
  bdver3,
  bdver4,
  znver1,
  btver1,
  btver2
};

typedef struct features {
  char adx;
  char aes;
  char erms;
  char f16c;
  char fma;
  char fma3;
  char fma4;
  char vpclmulqdq;
  char bmi1;
  char bmi2;
  char smx;
  char sha;
  char sse;
  char sse2;
  char sse3;
  char ssse3;
  char sse4_1;
  char sse4_2;
  char avx;
  char avx2;
  char avx512f;
  char avx512cd;
  char avx512er;
  char avx512pf;
  char avx512bw;
  char avx512dq;
  char avx512vl;
  char avx512ifma;
  char avx512vbmi;
  char avx512vbmi2;
  char avx512vnni;
  char avx512bitalg;
  char avx512vpopcntdq;
  char avx512_4vnniw;
  char avx512_4vbmi2;
  char avx512_4fmaps;

  char popcnt;
  char htt;
  char mpx;
  char sgx;
  char amd_3dnow;
  char amd_3dnowext;
  char rdrand;
  char hypervisor;
  char mmx;
  char cmov;
  char spec_ctrl;
  char tsc;
  char syscall;
  char fpu;
  char nx;
} features_t;

#define NO_CACHE 0
#define DATA_CACHE 1
#define INSTRUCTION_CACHE 2
#define UNIFIED_CACHE 3

typedef struct cache {
  unsigned short associative_cache;
  unsigned short self_init;
  unsigned short level;
  unsigned short type;
  unsigned short associativity;
  unsigned short phys_line;
  unsigned short coherency;
  unsigned short sets;
  unsigned long size;
} cache_t;

#define CORE_TYPE_NONE 0x00
#define CORE_TYPE_THREAD 0x01
#define CORE_TYPE_CORE 0x02

typedef struct core {
  char type;
  char apic;
  unsigned long long freq;
  cache_t caches[5];
} core_t;

typedef struct processor {
  features_t features;
  char core_total;
  core_t core[256];
  char family;
  char model;
  char stepping;
  char type;
  char emodel;
  char efamily;
  char vendor;
} processor_t;

END_C_NS
