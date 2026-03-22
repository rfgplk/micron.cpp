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
  /* leaf 1 ecx */
  char sse3;
  char pclmulqdq;
  char monitor;
  char ssse3;
  char fma;
  char fma3;
  char smx;
  char sse4_1;
  char sse4_2;
  char movbe;
  char popcnt;
  char aes;
  char xsave;
  char osxsave;
  char avx;
  char f16c;
  char rdrand;
  char hypervisor;

  /* leaf 1 edx */
  char fpu;
  char tsc;
  char cmov;
  char mmx;
  char sse;
  char sse2;
  char htt;

  /* leaf 7 sub 0 ebx */
  char fsgsbase;
  char tsc_adjust;
  char sgx;
  char bmi1;
  char hle;
  char avx2;
  char smep;
  char bmi2;
  char erms;
  char invpcid;
  char rtm;
  char mpx;
  char avx512f;
  char avx512dq;
  char rdseed;
  char adx;
  char smap;
  char avx512ifma;
  char clflushopt;
  char clwb;
  char avx512pf;
  char avx512er;
  char avx512cd;
  char sha;
  char avx512bw;
  char avx512vl;

  /* leaf 7 sub 0 ecx */
  char prefetchwt1;
  char avx512vbmi;
  char umip;
  char pku;
  char waitpkg;
  char avx512vbmi2;
  char gfni;
  char vaes;
  char vpclmulqdq;
  char avx512vnni;
  char avx512bitalg;
  char avx512vpopcntdq;
  char rdpid;
  char cldemote;
  char movdiri;
  char movdir64b;

  /* leaf 7 sub 0 edx */
  char avx512_4vnniw;
  char avx512_4fmaps;
  char fsrm;
  char avx512_vp2intersect;
  char serialize;
  char tsxldtrk;
  char pconfig;
  char ibt;
  char amx_bf16;
  char avx512_fp16;
  char amx_tile;
  char amx_int8;
  char spec_ctrl;

  /* leaf 7 sub 1 eax */
  char sha512;
  char sm3;
  char sm4;
  char rao_int;
  char avx_vnni;
  char avx512_bf16;
  char cmpccxadd;
  char amx_fp16;
  char avx_ifma;

  /* leaf 7 sub 1 edx */
  char avx_vnni_int8;
  char avx_ne_convert;
  char amx_complex;
  char avx_vnni_int16;
  char prefetchiti;
  char avx10;
  char apx_f;

  /* leaf 0x24 sub 0 (AVX10) */
  char avx10_version;
  char avx10_128;
  char avx10_256;
  char avx10_512;

  /* extended leaf 0x80000001 ecx */
  char lahf_lm;
  char abm;
  char sse4a;
  char misalignsse;
  char prefetchw;
  char xop;
  char fma4;
  char tbm;

  /* extended leaf 0x80000001 edx */
  char syscall;
  char nx;
  char amd_3dnowext;
  char amd_3dnow;

  /* extended leaf 0x80000008 ebx */
  char wbnoinvd;
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
