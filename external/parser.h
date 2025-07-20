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

#include "processor.h"
#include "features.h"
#include "macros.h"
#include "cpuid.h"

BEGIN_C_NS
__inline__ char
vendor(const cstruct_t *__restrict__ const ptr)
{
  const char intel[] = "GenuineIntel";
  const char amd[] = "AuthenticAMD";
  const char centaur[] = "CentaurHauls";
  const char cyrix[] = "CyrixInstead";
  const char transmeta_1[] = "TransmetaCPU";
  const char transmeta_2[] = "GenuineTMx86";
  const char natsemi[] = "Geode by NSC";
  const char nexgen[] = "NexGenDriven";
  const char rise[] = "RiseRiseRise";
  const char SiS[] = "SiS SiS SiS ";
  const char UMC[] = "UMC UMC UMC ";
  const char VIA[] = "VIA VIA VIA ";
  const char Vortex[] = "Vortex86 SoC";

  unsigned int ebx = *(const unsigned int *)(intel);
  unsigned int edx = *(const unsigned int *)(intel + 4);
  unsigned int ecx = *(const unsigned int *)(intel + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return GenuineIntel;

  ebx = *(const unsigned int *)(amd);
  edx = *(const unsigned int *)(amd + 4);
  ecx = *(const unsigned int *)(amd + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return AuthenticAMD;

  ebx = *(const unsigned int *)(centaur);
  edx = *(const unsigned int *)(centaur + 4);
  ecx = *(const unsigned int *)(centaur + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return CentaurHauls;

  ebx = *(const unsigned int *)(cyrix);
  edx = *(const unsigned int *)(cyrix + 4);
  ecx = *(const unsigned int *)(cyrix + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return CyrixInstead;

  ebx = *(const unsigned int *)(transmeta_1);
  edx = *(const unsigned int *)(transmeta_1 + 4);
  ecx = *(const unsigned int *)(transmeta_1 + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return TransmetaCPU;

  ebx = *(const unsigned int *)(transmeta_2);
  edx = *(const unsigned int *)(transmeta_2 + 4);
  ecx = *(const unsigned int *)(transmeta_2 + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return GenuineTMx86;

  ebx = *(const unsigned int *)(natsemi);
  edx = *(const unsigned int *)(natsemi + 4);
  ecx = *(const unsigned int *)(natsemi + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return Geode_by_NSC;

  ebx = *(const unsigned int *)(nexgen);
  edx = *(const unsigned int *)(nexgen + 4);
  ecx = *(const unsigned int *)(nexgen + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return NexGenDriven;

  ebx = *(const unsigned int *)(rise);
  edx = *(const unsigned int *)(rise + 4);
  ecx = *(const unsigned int *)(rise + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return RiseRiseRise;

  ebx = *(const unsigned int *)(SiS);
  edx = *(const unsigned int *)(SiS + 4);
  ecx = *(const unsigned int *)(SiS + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return SiS_SiS_SiS_;

  ebx = *(const unsigned int *)(UMC);
  edx = *(const unsigned int *)(UMC + 4);
  ecx = *(const unsigned int *)(UMC + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return UMC_UMC_UMC_;

  ebx = *(const unsigned int *)(VIA);
  edx = *(const unsigned int *)(VIA + 4);
  ecx = *(const unsigned int *)(VIA + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return VIA_VIA_VIA_;

  ebx = *(const unsigned int *)(Vortex);
  edx = *(const unsigned int *)(Vortex + 4);
  ecx = *(const unsigned int *)(Vortex + 8);

  if ( ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx )
    return Vortex86_SoC;
  return 0x00;
}
__inline__ void
spec(processor_t *__restrict__ const p)
{
  frequency_user(&p->core);
  cores_all(&p->core_total, &p->core);

  for ( int j = 0; p->core[j].type != CORE_TYPE_NONE; j++ ) {
    topology_set_cpu(j);
    char flag = 1;
    for ( int i = 0; flag; i++ ) {
      cache(i, &p->core[j].caches[i]);
      flag = (i < 5) && (p->core[j].caches[i].type != NO_CACHE);
    }
  }
}
__inline__ void
info(processor_t *__restrict__ const p)
{
  cstruct_t c_0;
  cstruct_t c_1;
  cstruct_t c_3;
  cstruct_t c_4;
  cstruct_t c_7;
  cstruct_t c_8000h;
  cpuid(0, &c_0);
  cpuid(1, &c_1);
  cpuid(3, &c_3);
  cpuid(4, &c_4);
  cpuid_c(7, 0, &c_7);
  cpuid(0x80000001, &c_8000h);

  p->family = rbit(c_1.eax, 11, 8);
  p->model = rbit(c_1.eax, 7, 4);
  p->stepping = rbit(c_1.eax, 3, 0);
  p->type = rbit(c_1.eax, 13, 12);
  p->emodel = rbit(c_1.eax, 19, 16);
  p->efamily = rbit(c_1.eax, 27, 20);
  p->vendor = vendor(&c_0);

  p->features.adx = bit(c_7.ebx, 19);
  p->features.aes = bit(c_1.ecx, 25);
  p->features.erms = bit(c_7.ebx, 9);
  p->features.f16c = bit(c_1.ecx, 29);
  p->features.fma = bit(c_1.ecx, 12);
  p->features.fma3 = p->features.fma;
  p->features.fma4 = bit(c_8000h.ecx, 16);
  p->features.vpclmulqdq = bit(c_7.ecx, 10);
  p->features.bmi1 = bit(c_7.ebx, 3);
  p->features.bmi2 = bit(c_7.ebx, 8);
  p->features.smx = bit(c_1.ecx, 6);
  p->features.sha = bit(c_7.ebx, 29);
  p->features.sse = bit(c_1.edx, 25);
  p->features.sse2 = bit(c_1.edx, 26);
  p->features.sse3 = bit(c_1.ecx, 0);
  p->features.ssse3 = bit(c_1.ecx, 9);
  p->features.sse4_1 = bit(c_1.ecx, 19);
  p->features.sse4_2 = bit(c_1.ecx, 20);
  p->features.avx = bit(c_1.ecx, 28);
  p->features.avx2 = bit(c_7.ebx, 5);
  p->features.avx512f = bit(c_7.ebx, 16);
  p->features.avx512cd = bit(c_7.ebx, 28);
  p->features.avx512er = bit(c_7.ebx, 27);
  p->features.avx512pf = bit(c_7.ebx, 26);
  p->features.avx512bw = bit(c_7.ebx, 30);
  p->features.avx512dq = bit(c_7.ebx, 17);
  p->features.avx512vl = bit(c_7.ebx, 31);
  p->features.avx512ifma = bit(c_7.ebx, 21);
  p->features.avx512vbmi = bit(c_7.ecx, 1);
  p->features.avx512vbmi2 = bit(c_7.ecx, 6);
  p->features.avx512vnni = bit(c_7.ecx, 11);
  p->features.avx512bitalg = bit(c_7.ecx, 12);
  p->features.avx512vpopcntdq = bit(c_7.ecx, 14);
  p->features.avx512_4vnniw = bit(c_7.edx, 2);
  p->features.avx512_4fmaps = bit(c_7.edx, 3);

  p->features.popcnt = bit(c_1.ecx, 23);
  p->features.htt = bit(c_1.edx, 28);
  p->features.mpx = bit(c_7.ebx, 14);
  p->features.sgx = bit(c_7.ebx, 2);
  p->features.amd_3dnow = bit(c_8000h.edx, 30);
  p->features.amd_3dnowext = bit(c_8000h.edx, 31);
  p->features.rdrand = bit(c_1.ecx, 30);
  p->features.hypervisor = bit(c_1.ecx, 31);
  p->features.mmx = bit(c_8000h.edx, 23);
  p->features.cmov = bit(c_8000h.edx, 15);
  p->features.spec_ctrl = bit(c_7.edx, 26);
  p->features.tsc = bit(c_8000h.edx, 4);
  p->features.syscall = bit(c_8000h.edx, 11);
  p->features.fpu = bit(c_8000h.edx, 0);
  p->features.nx = bit(c_8000h.edx, 20);
}

END_C_NS
