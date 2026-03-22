// nanoflagsarm (for C99 and onwards)
// https://github.com/rfgplk/nanoflagsarm
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

#include "features.h"
#include "macros.h"

#include <sys/auxv.h>

BEGIN_C_NS

__inline__ unsigned int
rfield(const unsigned long r, const unsigned long hi, const unsigned long lo)
{
  const unsigned long bits = hi - lo + 1;
  const unsigned long mask = (1UL << bits) - 1UL;
  return (unsigned int)((r >> lo) & mask);
}

__inline__ char
bfield(const unsigned long r, const unsigned long b)
{
  return (char)(0x01 & (r >> b));
}

typedef struct hwcaps {
  unsigned long hwcap;
  unsigned long hwcap2;
} hwcaps_t;

__inline__ void
read_hwcaps(hwcaps_t *__restrict__ const h)
{
  h->hwcap = getauxval(_NF_AT_HWCAP);
  h->hwcap2 = getauxval(_NF_AT_HWCAP2);
}

// ring0 only, kernel traps us

#ifdef ARCH_AARCH64

typedef struct arm_id_regs {
  unsigned long midr;
  unsigned long revidr;
  unsigned long isar0;
  unsigned long isar1;
  unsigned long isar2;
  unsigned long pfr0;
  unsigned long pfr1;
  unsigned long mmfr0;
  unsigned long mmfr1;
  unsigned long mmfr2;
  unsigned long zfr0;
  unsigned long smfr0;
  unsigned long ctr;
} arm_id_regs_t;

#define __mrs_read(reg, val) __asm__ volatile("mrs %0, " #reg : "=r"(val))

__inline__ void
read_midr(unsigned long *__restrict__ val)
{
  __mrs_read(midr_el1, *val);
}

__inline__ void
read_id_regs(arm_id_regs_t *__restrict__ const r)
{
  __mrs_read(midr_el1, r->midr);
  __mrs_read(revidr_el1, r->revidr);
  __mrs_read(id_aa64isar0_el1, r->isar0);
  __mrs_read(id_aa64isar1_el1, r->isar1);
  __mrs_read(id_aa64isar2_el1, r->isar2);
  __mrs_read(id_aa64pfr0_el1, r->pfr0);
  __mrs_read(id_aa64pfr1_el1, r->pfr1);
  __mrs_read(id_aa64mmfr0_el1, r->mmfr0);
  __mrs_read(id_aa64mmfr1_el1, r->mmfr1);
  __mrs_read(id_aa64mmfr2_el1, r->mmfr2);
  __mrs_read(id_aa64zfr0_el1, r->zfr0);
  // smfr0 may be undef
  r->smfr0 = 0;
}

__inline__ void
read_id_regs_sme(arm_id_regs_t *__restrict__ const r)
{
  __mrs_read(id_aa64smfr0_el1, r->smfr0);
}

__inline__ void
read_ctr(unsigned long *__restrict__ val)
{
  __mrs_read(ctr_el0, *val);
}

__inline__ unsigned long long
read_cntvct(void)
{
  unsigned long long val;
  __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
  return val;
}

__inline__ unsigned long long
read_cntfrq(void)
{
  unsigned long long val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(val));
  return val;
}

// have_idreg(): equivalent of x86 have_cpuid()
// Returns 1 if the kernel exposes ID registers to EL0 via MRS trapping
// (>= 4.11) bit 11 of AT_HWCAP

char
have_idreg(void)
{
  unsigned long h = getauxval(_NF_AT_HWCAP);
  if ( h & _NF_A64_CPUID )
    return 1;
  return 0;
}

int
maximum_arch(void)
{
  unsigned long h = getauxval(_NF_AT_HWCAP);
  unsigned long long h2 = (unsigned long long)getauxval(_NF_AT_HWCAP2);

  if ( !(h & _NF_A64_FP) || !(h & _NF_A64_ASIMD) )
    return 0;

  int level = 80;

  if ( h & _NF_A64_ATOMICS )
    level = 81;
  if ( level >= 81 && (h & _NF_A64_DCPOP) )
    level = 82;
  if ( level >= 82 && (h & _NF_A64_JSCVT) && (h & _NF_A64_FCMA) && (h & _NF_A64_LRCPC) && (h & _NF_A64_PACA) )
    level = 83;
  if ( level >= 83 && (h & _NF_A64_DIT) && (h & _NF_A64_FLAGM) && (h & _NF_A64_ILRCPC) && (h & _NF_A64_USCAT) )
    level = 84;
  if ( level >= 84 && (h & _NF_A64_SSBS) && (h & _NF_A64_SB) && (h2 & _NF_A64_BTI) && (h2 & _NF_A64_FRINT) )
    level = 85;
  if ( level >= 85 && (h2 & _NF_A64_BF16) && (h2 & _NF_A64_I8MM) )
    level = 86;
  if ( level >= 86 && (h2 & _NF_A64_WFXT) )
    level = 87;
  if ( level >= 87 && (h2 & _NF_A64_MOPS) && (h2 & _NF_A64_HBC) )
    level = 88;
  if ( level >= 88 && (h2 & _NF_A64_CSSC) && (h2 & _NF_A64_RPRFM) )
    level = 89;
  if ( level >= 85 && (h2 & _NF_A64_SVE2) )
    level = (level < 90) ? 90 : level;
  if ( level >= 90 && (h2 & _NF_A64_SME) )
    level = (level < 92) ? 92 : level;
  if ( level >= 92 && (h2 & _NF_A64_SME2) )
    level = (level < 94) ? 94 : level;

  return level;
}

#endif

#ifdef ARCH_AARCH32

typedef struct arm_id_regs {
  unsigned long midr;
  unsigned long ctr;
  unsigned long id_pfr0;
  unsigned long id_pfr1;
  unsigned long id_mmfr0;
  unsigned long id_isar0;
  unsigned long id_isar1;
  unsigned long id_isar2;
  unsigned long id_isar3;
  unsigned long id_isar4;
  unsigned long id_isar5;
} arm_id_regs_t;

__inline__ void
read_midr(unsigned long *__restrict__ val)
{
  __asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(*val));
}

__inline__ void
read_id_regs(arm_id_regs_t *__restrict__ const r)
{
  __asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(r->midr));
  __asm__ volatile("mrc p15, 0, %0, c0, c0, 1" : "=r"(r->ctr));
  __asm__ volatile("mrc p15, 0, %0, c0, c1, 0" : "=r"(r->id_pfr0));
  __asm__ volatile("mrc p15, 0, %0, c0, c1, 1" : "=r"(r->id_pfr1));
  __asm__ volatile("mrc p15, 0, %0, c0, c1, 4" : "=r"(r->id_mmfr0));
  __asm__ volatile("mrc p15, 0, %0, c0, c2, 0" : "=r"(r->id_isar0));
  __asm__ volatile("mrc p15, 0, %0, c0, c2, 1" : "=r"(r->id_isar1));
  __asm__ volatile("mrc p15, 0, %0, c0, c2, 2" : "=r"(r->id_isar2));
  __asm__ volatile("mrc p15, 0, %0, c0, c2, 3" : "=r"(r->id_isar3));
  __asm__ volatile("mrc p15, 0, %0, c0, c2, 4" : "=r"(r->id_isar4));
  __asm__ volatile("mrc p15, 0, %0, c0, c2, 5" : "=r"(r->id_isar5));
}

__inline__ void
read_ctr(unsigned long *__restrict__ val)
{
  __asm__ volatile("mrc p15, 0, %0, c0, c0, 1" : "=r"(*val));
}

__inline__ unsigned long long
read_cntvct(void)
{
  unsigned long lo, hi;
  __asm__ volatile("mrrc p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
  return ((unsigned long long)hi << 32) | lo;
}

__inline__ unsigned long long
read_cntfrq(void)
{
  unsigned long val;
  __asm__ volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(val));
  return (unsigned long long)val;
}

char
have_idreg(void)
{
  unsigned long midr;
  __asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(midr));
  return (midr != 0) ? 1 : 0;
}

int
maximum_arch(void)
{
  unsigned long midr;
  __asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(midr));
  return (int)rfield(midr, 19, 16);
}

#endif

__inline__ unsigned char
midr_implementer(const unsigned long midr)
{
  return (unsigned char)rfield(midr, 31, 24);
}

__inline__ unsigned char
midr_variant(const unsigned long midr)
{
  return (unsigned char)rfield(midr, 23, 20);
}

__inline__ unsigned char
midr_architecture(const unsigned long midr)
{
  return (unsigned char)rfield(midr, 19, 16);
}

__inline__ unsigned short
midr_part(const unsigned long midr)
{
  return (unsigned short)rfield(midr, 15, 4);
}

__inline__ unsigned char
midr_revision(const unsigned long midr)
{
  return (unsigned char)rfield(midr, 3, 0);
}

END_C_NS
