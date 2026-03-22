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
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "cpuid.h"
#include "features.h"
#include "macros.h"
#include "processor.h"

BEGIN_C_NS

__inline__ char vendor(const cstruct_t *__restrict__ const ptr) {
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

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return GenuineIntel;

  ebx = *(const unsigned int *)(amd);
  edx = *(const unsigned int *)(amd + 4);
  ecx = *(const unsigned int *)(amd + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return AuthenticAMD;

  ebx = *(const unsigned int *)(centaur);
  edx = *(const unsigned int *)(centaur + 4);
  ecx = *(const unsigned int *)(centaur + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return CentaurHauls;

  ebx = *(const unsigned int *)(cyrix);
  edx = *(const unsigned int *)(cyrix + 4);
  ecx = *(const unsigned int *)(cyrix + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return CyrixInstead;

  ebx = *(const unsigned int *)(transmeta_1);
  edx = *(const unsigned int *)(transmeta_1 + 4);
  ecx = *(const unsigned int *)(transmeta_1 + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return TransmetaCPU;

  ebx = *(const unsigned int *)(transmeta_2);
  edx = *(const unsigned int *)(transmeta_2 + 4);
  ecx = *(const unsigned int *)(transmeta_2 + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return GenuineTMx86;

  ebx = *(const unsigned int *)(natsemi);
  edx = *(const unsigned int *)(natsemi + 4);
  ecx = *(const unsigned int *)(natsemi + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return Geode_by_NSC;

  ebx = *(const unsigned int *)(nexgen);
  edx = *(const unsigned int *)(nexgen + 4);
  ecx = *(const unsigned int *)(nexgen + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return NexGenDriven;

  ebx = *(const unsigned int *)(rise);
  edx = *(const unsigned int *)(rise + 4);
  ecx = *(const unsigned int *)(rise + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return RiseRiseRise;

  ebx = *(const unsigned int *)(SiS);
  edx = *(const unsigned int *)(SiS + 4);
  ecx = *(const unsigned int *)(SiS + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return SiS_SiS_SiS_;

  ebx = *(const unsigned int *)(UMC);
  edx = *(const unsigned int *)(UMC + 4);
  ecx = *(const unsigned int *)(UMC + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return UMC_UMC_UMC_;

  ebx = *(const unsigned int *)(VIA);
  edx = *(const unsigned int *)(VIA + 4);
  ecx = *(const unsigned int *)(VIA + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return VIA_VIA_VIA_;

  ebx = *(const unsigned int *)(Vortex);
  edx = *(const unsigned int *)(Vortex + 4);
  ecx = *(const unsigned int *)(Vortex + 8);

  if (ebx == ptr->ebx && edx == ptr->edx && ecx == ptr->ecx)
    return Vortex86_SoC;

  return 0x00;
}

__inline__ void spec(processor_t *__restrict__ const p) {
  frequency_user(&p->core);
  cores_all(&p->core_total, &p->core);

  for (int j = 0; p->core[j].type != CORE_TYPE_NONE; j++) {
    topology_set_cpu(j);
    char flag = 1;
    for (int i = 0; flag; i++) {
      cache((char)i, &p->core[j].caches[i]);
      flag = (i < 5) && (p->core[j].caches[i].type != NO_CACHE);
    }
  }
}

__inline__ void info(processor_t *__restrict__ const p) {
  cstruct_t c_0;
  cstruct_t c_1;
  cstruct_t c_7_0;
  cstruct_t c_7_1;
  cstruct_t c_24;
  cstruct_t c_8000h;
  cstruct_t c_8008h;

  cpuid(0, &c_0);
  cpuid(1, &c_1);
  cpuid_c(7, 0, &c_7_0);
  cpuid_c(7, 1, &c_7_1);
  cpuid(0x80000001, &c_8000h);
  cpuid(0x80000008, &c_8008h);

  p->family = (char)rbit(c_1.eax, 11, 8);
  p->model = (char)rbit(c_1.eax, 7, 4);
  p->stepping = (char)rbit(c_1.eax, 3, 0);
  p->type = (char)rbit(c_1.eax, 13, 12);
  p->emodel = (char)rbit(c_1.eax, 19, 16);
  p->efamily = (char)rbit(c_1.eax, 27, 20);
  p->vendor = vendor(&c_0);

  /* ---- leaf 1 ECX ---- */
  p->features.sse3 = bit(c_1.ecx, 0);
  p->features.pclmulqdq = bit(c_1.ecx, 1);
  p->features.monitor = bit(c_1.ecx, 3);
  p->features.ssse3 = bit(c_1.ecx, 9);
  p->features.fma = bit(c_1.ecx, 12);
  p->features.fma3 = p->features.fma;
  p->features.smx = bit(c_1.ecx, 6);
  p->features.sse4_1 = bit(c_1.ecx, 19);
  p->features.sse4_2 = bit(c_1.ecx, 20);
  p->features.movbe = bit(c_1.ecx, 22);
  p->features.popcnt = bit(c_1.ecx, 23);
  p->features.aes = bit(c_1.ecx, 25);
  p->features.xsave = bit(c_1.ecx, 26);
  p->features.osxsave = bit(c_1.ecx, 27);
  p->features.avx = bit(c_1.ecx, 28);
  p->features.f16c = bit(c_1.ecx, 29);
  p->features.rdrand = bit(c_1.ecx, 30);
  p->features.hypervisor = bit(c_1.ecx, 31);

  /* ---- leaf 1 EDX ---- */
  p->features.fpu = bit(c_1.edx, 0);
  p->features.tsc = bit(c_1.edx, 4);
  p->features.cmov = bit(c_1.edx, 15);
  p->features.mmx = bit(c_1.edx, 23);
  p->features.sse = bit(c_1.edx, 25);
  p->features.sse2 = bit(c_1.edx, 26);
  p->features.htt = bit(c_1.edx, 28);

  /* ---- leaf 7 sub 0 EBX ---- */
  p->features.fsgsbase = bit(c_7_0.ebx, 0);
  p->features.tsc_adjust = bit(c_7_0.ebx, 1);
  p->features.sgx = bit(c_7_0.ebx, 2);
  p->features.bmi1 = bit(c_7_0.ebx, 3);
  p->features.hle = bit(c_7_0.ebx, 4);
  p->features.avx2 = bit(c_7_0.ebx, 5);
  p->features.smep = bit(c_7_0.ebx, 7);
  p->features.bmi2 = bit(c_7_0.ebx, 8);
  p->features.erms = bit(c_7_0.ebx, 9);
  p->features.invpcid = bit(c_7_0.ebx, 10);
  p->features.rtm = bit(c_7_0.ebx, 11);
  p->features.mpx = bit(c_7_0.ebx, 14);
  p->features.avx512f = bit(c_7_0.ebx, 16);
  p->features.avx512dq = bit(c_7_0.ebx, 17);
  p->features.rdseed = bit(c_7_0.ebx, 18);
  p->features.adx = bit(c_7_0.ebx, 19);
  p->features.smap = bit(c_7_0.ebx, 20);
  p->features.avx512ifma = bit(c_7_0.ebx, 21);
  p->features.clflushopt = bit(c_7_0.ebx, 23);
  p->features.clwb = bit(c_7_0.ebx, 24);
  p->features.avx512pf = bit(c_7_0.ebx, 26);
  p->features.avx512er = bit(c_7_0.ebx, 27);
  p->features.avx512cd = bit(c_7_0.ebx, 28);
  p->features.sha = bit(c_7_0.ebx, 29);
  p->features.avx512bw = bit(c_7_0.ebx, 30);
  p->features.avx512vl = bit(c_7_0.ebx, 31);

  /* ---- leaf 7 sub 0 ECX ---- */
  p->features.prefetchwt1 = bit(c_7_0.ecx, 0);
  p->features.avx512vbmi = bit(c_7_0.ecx, 1);
  p->features.umip = bit(c_7_0.ecx, 2);
  p->features.pku = bit(c_7_0.ecx, 3);
  p->features.waitpkg = bit(c_7_0.ecx, 5);
  p->features.avx512vbmi2 = bit(c_7_0.ecx, 6);
  p->features.gfni = bit(c_7_0.ecx, 8);
  p->features.vaes = bit(c_7_0.ecx, 9);
  p->features.vpclmulqdq = bit(c_7_0.ecx, 10);
  p->features.avx512vnni = bit(c_7_0.ecx, 11);
  p->features.avx512bitalg = bit(c_7_0.ecx, 12);
  p->features.avx512vpopcntdq = bit(c_7_0.ecx, 14);
  p->features.rdpid = bit(c_7_0.ecx, 22);
  p->features.cldemote = bit(c_7_0.ecx, 25);
  p->features.movdiri = bit(c_7_0.ecx, 27);
  p->features.movdir64b = bit(c_7_0.ecx, 28);

  /* ---- leaf 7 sub 0 EDX ---- */
  p->features.avx512_4vnniw = bit(c_7_0.edx, 2);
  p->features.avx512_4fmaps = bit(c_7_0.edx, 3);
  p->features.fsrm = bit(c_7_0.edx, 4);
  p->features.avx512_vp2intersect = bit(c_7_0.edx, 8);
  p->features.serialize = bit(c_7_0.edx, 14);
  p->features.tsxldtrk = bit(c_7_0.edx, 16);
  p->features.pconfig = bit(c_7_0.edx, 18);
  p->features.ibt = bit(c_7_0.edx, 20);
  p->features.amx_bf16 = bit(c_7_0.edx, 22);
  p->features.avx512_fp16 = bit(c_7_0.edx, 23);
  p->features.amx_tile = bit(c_7_0.edx, 24);
  p->features.amx_int8 = bit(c_7_0.edx, 25);
  p->features.spec_ctrl = bit(c_7_0.edx, 26);

  /* ---- leaf 7 sub 1 EAX ---- */
  p->features.sha512 = bit(c_7_1.eax, 0);
  p->features.sm3 = bit(c_7_1.eax, 1);
  p->features.sm4 = bit(c_7_1.eax, 2);
  p->features.rao_int = bit(c_7_1.eax, 3);
  p->features.avx_vnni = bit(c_7_1.eax, 4);
  p->features.avx512_bf16 = bit(c_7_1.eax, 5);
  p->features.cmpccxadd = bit(c_7_1.eax, 7);
  p->features.amx_fp16 = bit(c_7_1.eax, 21);
  p->features.avx_ifma = bit(c_7_1.eax, 23);

  /* ---- leaf 7 sub 1 EDX ---- */
  p->features.avx_vnni_int8 = bit(c_7_1.edx, 4);
  p->features.avx_ne_convert = bit(c_7_1.edx, 5);
  p->features.amx_complex = bit(c_7_1.edx, 8);
  p->features.avx_vnni_int16 = bit(c_7_1.edx, 10);
  p->features.prefetchiti = bit(c_7_1.edx, 14);
  p->features.avx10 = bit(c_7_1.edx, 19);
  p->features.apx_f = bit(c_7_1.edx, 21);

  /* ---- leaf 0x24 sub 0 (AVX10 enumeration) ---- */
  p->features.avx10_version = 0;
  p->features.avx10_128 = 0;
  p->features.avx10_256 = 0;
  p->features.avx10_512 = 0;
  if (p->features.avx10) {
    cpuid_c(0x24, 0, &c_24);
    p->features.avx10_version = (char)rbit(c_24.ebx, 7, 0);
    p->features.avx10_128 = bit(c_24.ebx, 16);
    p->features.avx10_256 = bit(c_24.ebx, 17);
    p->features.avx10_512 = bit(c_24.ebx, 18);
  }

  /* ---- extended leaf 0x80000001 ECX ---- */
  p->features.lahf_lm = bit(c_8000h.ecx, 0);
  p->features.abm = bit(c_8000h.ecx, 5);
  p->features.sse4a = bit(c_8000h.ecx, 6);
  p->features.misalignsse = bit(c_8000h.ecx, 7);
  p->features.prefetchw = bit(c_8000h.ecx, 8);
  p->features.xop = bit(c_8000h.ecx, 11);
  p->features.fma4 = bit(c_8000h.ecx, 16);
  p->features.tbm = bit(c_8000h.ecx, 21);

  /* ---- extended leaf 0x80000001 EDX ---- */
  p->features.syscall = bit(c_8000h.edx, 11);
  p->features.nx = bit(c_8000h.edx, 20);
  p->features.amd_3dnowext = bit(c_8000h.edx, 30);
  p->features.amd_3dnow = bit(c_8000h.edx, 31);

  /* ---- extended leaf 0x80000008 EBX ---- */
  p->features.wbnoinvd = bit(c_8008h.ebx, 9);
}

END_C_NS
