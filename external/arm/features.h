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

#include "macros.h"

BEGIN_C_NS

/* ======================================================================
 * HWCAP bit definitions (Linux ABI-stable, self-contained so we don't
 * depend on a specific kernel header version)
 * ====================================================================== */

/* ---- AArch64 AT_HWCAP (auxv type 16) ---- */
#define _NF_A64_FP              (1UL << 0)
#define _NF_A64_ASIMD           (1UL << 1)
#define _NF_A64_EVTSTRM         (1UL << 2)
#define _NF_A64_AES             (1UL << 3)
#define _NF_A64_PMULL           (1UL << 4)
#define _NF_A64_SHA1            (1UL << 5)
#define _NF_A64_SHA2            (1UL << 6)
#define _NF_A64_CRC32           (1UL << 7)
#define _NF_A64_ATOMICS         (1UL << 8)
#define _NF_A64_FPHP            (1UL << 9)
#define _NF_A64_ASIMDHP         (1UL << 10)
#define _NF_A64_CPUID           (1UL << 11)
#define _NF_A64_ASIMDRDM        (1UL << 12)
#define _NF_A64_JSCVT           (1UL << 13)
#define _NF_A64_FCMA            (1UL << 14)
#define _NF_A64_LRCPC           (1UL << 15)
#define _NF_A64_DCPOP           (1UL << 16)
#define _NF_A64_SHA3            (1UL << 17)
#define _NF_A64_SM3             (1UL << 18)
#define _NF_A64_SM4             (1UL << 19)
#define _NF_A64_ASIMDDP         (1UL << 20)
#define _NF_A64_SHA512          (1UL << 21)
#define _NF_A64_SVE             (1UL << 22)
#define _NF_A64_ASIMDFHM        (1UL << 23)
#define _NF_A64_DIT             (1UL << 24)
#define _NF_A64_USCAT           (1UL << 25)
#define _NF_A64_ILRCPC          (1UL << 26)
#define _NF_A64_FLAGM           (1UL << 27)
#define _NF_A64_SSBS            (1UL << 28)
#define _NF_A64_SB              (1UL << 29)
#define _NF_A64_PACA            (1UL << 30)
#define _NF_A64_PACG            (1UL << 31)

/* ---- AArch64 AT_HWCAP2 (auxv type 26) ---- */
#define _NF_A64_DCPODP          (1ULL << 0)
#define _NF_A64_SVE2            (1ULL << 1)
#define _NF_A64_SVEAES          (1ULL << 2)
#define _NF_A64_SVEPMULL        (1ULL << 3)
#define _NF_A64_SVEBITPERM      (1ULL << 4)
#define _NF_A64_SVESHA3         (1ULL << 5)
#define _NF_A64_SVESM4          (1ULL << 6)
#define _NF_A64_FLAGM2          (1ULL << 7)
#define _NF_A64_FRINT           (1ULL << 8)
#define _NF_A64_SVEI8MM         (1ULL << 9)
#define _NF_A64_SVEF32MM        (1ULL << 10)
#define _NF_A64_SVEF64MM        (1ULL << 11)
#define _NF_A64_SVEBF16         (1ULL << 12)
#define _NF_A64_I8MM            (1ULL << 13)
#define _NF_A64_BF16            (1ULL << 14)
#define _NF_A64_DGH             (1ULL << 15)
#define _NF_A64_RNG             (1ULL << 16)
#define _NF_A64_BTI             (1ULL << 17)
#define _NF_A64_MTE             (1ULL << 18)
#define _NF_A64_ECV             (1ULL << 19)
#define _NF_A64_AFP             (1ULL << 20)
#define _NF_A64_RPRES           (1ULL << 21)
#define _NF_A64_MTE3            (1ULL << 22)
#define _NF_A64_SME             (1ULL << 23)
#define _NF_A64_SME_I16I64      (1ULL << 24)
#define _NF_A64_SME_F64F64      (1ULL << 25)
#define _NF_A64_SME_I8I32       (1ULL << 26)
#define _NF_A64_SME_F16F32      (1ULL << 27)
#define _NF_A64_SME_B16F32      (1ULL << 28)
#define _NF_A64_SME_F32F32      (1ULL << 29)
#define _NF_A64_SME_FA64        (1ULL << 30)
#define _NF_A64_WFXT            (1ULL << 31)
#define _NF_A64_EBF16           (1ULL << 32)
#define _NF_A64_SVE_EBF16       (1ULL << 33)
#define _NF_A64_CSSC            (1ULL << 34)
#define _NF_A64_RPRFM           (1ULL << 35)
#define _NF_A64_SVE2P1          (1ULL << 36)
#define _NF_A64_SME2            (1ULL << 37)
#define _NF_A64_SME2P1          (1ULL << 38)
#define _NF_A64_SME_I16I32      (1ULL << 39)
#define _NF_A64_SME_BI32I32     (1ULL << 40)
#define _NF_A64_SME_B16B16      (1ULL << 41)
#define _NF_A64_SME_F16F16      (1ULL << 42)
#define _NF_A64_MOPS            (1ULL << 43)
#define _NF_A64_HBC             (1ULL << 44)
#define _NF_A64_SVE_B16B16      (1ULL << 45)
#define _NF_A64_LRCPC3          (1ULL << 46)
#define _NF_A64_LSE128          (1ULL << 47)
#define _NF_A64_FPMR            (1ULL << 48)
#define _NF_A64_LUT             (1ULL << 49)
#define _NF_A64_FAMINMAX        (1ULL << 50)
#define _NF_A64_F8CVT           (1ULL << 51)
#define _NF_A64_F8FMA           (1ULL << 52)
#define _NF_A64_F8DP4           (1ULL << 53)
#define _NF_A64_F8DP2           (1ULL << 54)
#define _NF_A64_F8E4M3          (1ULL << 55)
#define _NF_A64_F8E5M2          (1ULL << 56)
#define _NF_A64_SME_LUTV2       (1ULL << 57)
#define _NF_A64_SME_F8F16       (1ULL << 58)
#define _NF_A64_SME_F8F32       (1ULL << 59)
#define _NF_A64_SME_SF8FMA      (1ULL << 60)
#define _NF_A64_SME_SF8DP4      (1ULL << 61)
#define _NF_A64_SME_SF8DP2      (1ULL << 62)
#define _NF_A64_POE             (1ULL << 63)

/* ---- AArch32 AT_HWCAP (auxv type 16) ---- */
#define _NF_A32_SWP             (1UL << 0)
#define _NF_A32_HALF            (1UL << 1)
#define _NF_A32_THUMB           (1UL << 2)
#define _NF_A32_26BIT           (1UL << 3)
#define _NF_A32_FAST_MULT       (1UL << 4)
#define _NF_A32_FPA             (1UL << 5)
#define _NF_A32_VFP             (1UL << 6)
#define _NF_A32_EDSP            (1UL << 7)
#define _NF_A32_JAVA            (1UL << 8)
#define _NF_A32_IWMMXT          (1UL << 9)
#define _NF_A32_CRUNCH          (1UL << 10)
#define _NF_A32_THUMBEE         (1UL << 11)
#define _NF_A32_NEON            (1UL << 12)
#define _NF_A32_VFPv3           (1UL << 13)
#define _NF_A32_VFPv3D16        (1UL << 14)
#define _NF_A32_TLS             (1UL << 15)
#define _NF_A32_VFPv4           (1UL << 16)
#define _NF_A32_IDIVA           (1UL << 17)
#define _NF_A32_IDIVT           (1UL << 18)
#define _NF_A32_VFPD32          (1UL << 19)
#define _NF_A32_LPAE            (1UL << 20)
#define _NF_A32_EVTSTRM         (1UL << 21)

/* ---- AArch32 AT_HWCAP2 (auxv type 26) ---- */
#define _NF_A32_AES             (1UL << 0)
#define _NF_A32_PMULL           (1UL << 1)
#define _NF_A32_SHA1            (1UL << 2)
#define _NF_A32_SHA2            (1UL << 3)
#define _NF_A32_CRC32           (1UL << 4)

/* ---- auxv types ---- */
#define _NF_AT_HWCAP  16
#define _NF_AT_HWCAP2 26

/* ======================================================================
 * MIDR_EL1 implementer codes
 * ====================================================================== */
enum implementer_t {
  IMPL_ARM          = 0x41,
  IMPL_BROADCOM     = 0x42,
  IMPL_CAVIUM       = 0x43,
  IMPL_DEC          = 0x44,
  IMPL_FUJITSU      = 0x46,
  IMPL_HISILICON    = 0x48,
  IMPL_INFINEON     = 0x49,
  IMPL_FREESCALE    = 0x4D,
  IMPL_NVIDIA       = 0x4E,
  IMPL_APM          = 0x50,
  IMPL_QUALCOMM     = 0x51,
  IMPL_SAMSUNG      = 0x53,
  IMPL_MARVELL      = 0x56,
  IMPL_APPLE        = 0x61,
  IMPL_FARADAY      = 0x66,
  IMPL_HXT          = 0x68,
  IMPL_INTEL        = 0x69,
  IMPL_MICROSOFT    = 0x6D,
  IMPL_PHYTIUM      = 0x70,
  IMPL_AMPERE       = 0xC0,
  IMPL_UNKNOWN      = 0x00
};

/* ======================================================================
 * well-known ARM part numbers
 * ====================================================================== */
enum arm_part_t {
  PART_A53         = 0xD03,
  PART_A55         = 0xD05,
  PART_A57         = 0xD07,
  PART_A72         = 0xD08,
  PART_A73         = 0xD09,
  PART_A75         = 0xD0A,
  PART_A76         = 0xD0B,
  PART_A77         = 0xD0D,
  PART_A78         = 0xD41,
  PART_A78C        = 0xD4B,
  PART_A710        = 0xD47,
  PART_A715        = 0xD4D,
  PART_A720        = 0xD81,
  PART_X1          = 0xD44,
  PART_X2          = 0xD48,
  PART_X3          = 0xD4E,
  PART_X4          = 0xD82,
  PART_V1          = 0xD40,
  PART_V2          = 0xD4F,
  PART_N1          = 0xD0C,
  PART_N2          = 0xD49,
  PART_N3          = 0xD84,
  PART_APPLE_M1_F  = 0x022,
  PART_APPLE_M1_P  = 0x023,
  PART_APPLE_M2_F  = 0x032,
  PART_APPLE_M2_P  = 0x033,
  PART_NEOVERSE_E1 = 0xD4A,
  PART_UNKNOWN     = 0x000
};

/* ======================================================================
 * features_t
 * ====================================================================== */
typedef struct features {
  /* ---- core ISA (AArch64) ---- */
  char fp;
  char asimd;
  char evtstrm;

  /* ---- crypto ---- */
  char aes;
  char pmull;
  char sha1;
  char sha2;
  char sha3;
  char sha512;
  char sm3;
  char sm4;

  /* ---- integer / SIMD ---- */
  char crc32;
  char atomics;
  char lse128;
  char fphp;
  char asimdhp;
  char asimdrdm;
  char asimddp;
  char asimdfhm;
  char bf16;
  char ebf16;
  char i8mm;
  char f16c;

  /* ---- branch / control flow ---- */
  char jscvt;
  char fcma;
  char flagm;
  char flagm2;

  /* ---- memory ordering ---- */
  char lrcpc;
  char ilrcpc;
  char lrcpc3;
  char uscat;

  /* ---- cache maintenance ---- */
  char dcpop;
  char dcpodp;

  /* ---- misc scalar ---- */
  char frint;
  char dit;
  char rng;
  char dgh;
  char cssc;
  char mops;
  char hbc;
  char wfxt;
  char rpres;
  char ecv;
  char afp;
  char rprfm;
  char fpmr;
  char poe;

  /* ---- pointer auth / speculation ---- */
  char paca;
  char pacg;
  char bti;
  char ssbs;
  char sb;
  char spec_ctrl;

  /* ---- MTE ---- */
  char mte;
  char mte3;

  /* ---- SVE family ---- */
  char sve;
  char sve2;
  char sve2p1;
  char sve_aes;
  char sve_pmull;
  char sve_bitperm;
  char sve_sha3;
  char sve_sm4;
  char sve_i8mm;
  char sve_f32mm;
  char sve_f64mm;
  char sve_bf16;
  char sve_ebf16;
  char sve_b16b16;

  /* ---- SME family ---- */
  char sme;
  char sme2;
  char sme2p1;
  char sme_i16i64;
  char sme_f64f64;
  char sme_i8i32;
  char sme_f16f32;
  char sme_b16f32;
  char sme_f32f32;
  char sme_fa64;
  char sme_i16i32;
  char sme_bi32i32;
  char sme_b16b16;
  char sme_f16f16;
  char sme_lutv2;
  char sme_f8f16;
  char sme_f8f32;
  char sme_sf8fma;
  char sme_sf8dp4;
  char sme_sf8dp2;

  /* ---- FP8 / newest ---- */
  char lut;
  char faminmax;
  char f8cvt;
  char f8fma;
  char f8dp4;
  char f8dp2;
  char f8e4m3;
  char f8e5m2;

  /* ---- AArch32-only (set only on AArch32 targets) ---- */
  char a32_swp;
  char a32_half;
  char a32_thumb;
  char a32_26bit;
  char a32_fast_mult;
  char a32_fpa;
  char a32_vfp;
  char a32_edsp;
  char a32_java;
  char a32_iwmmxt;
  char a32_crunch;
  char a32_thumbee;
  char a32_neon;
  char a32_vfpv3;
  char a32_vfpv3d16;
  char a32_tls;
  char a32_vfpv4;
  char a32_idiva;
  char a32_idivt;
  char a32_vfpd32;
  char a32_lpae;
  char a32_evtstrm;
  char a32_aes;
  char a32_pmull;
  char a32_sha1;
  char a32_sha2;
  char a32_crc32;
} features_t;

/* ======================================================================
 * cache
 * ====================================================================== */

#define NO_CACHE 0
#define DATA_CACHE 1
#define INSTRUCTION_CACHE 2
#define UNIFIED_CACHE 3

typedef struct cache {
  unsigned short type;
  unsigned short level;
  unsigned int line_size;
  unsigned int associativity;
  unsigned int sets;
  unsigned long size;
} cache_t;

/* ======================================================================
 * core / topology
 * ====================================================================== */

#define CORE_TYPE_NONE 0x00
#define CORE_TYPE_CORE 0x01

typedef struct core {
  char type;
  unsigned char cluster;
  unsigned long long timer_freq;
  cache_t caches[4];
} core_t;

/* ======================================================================
 * processor_t
 * ====================================================================== */

typedef struct processor {
  features_t features;
  char core_total;
  core_t core[256];
  unsigned char implementer;
  unsigned short part;
  unsigned char variant;
  unsigned char revision;
  unsigned char architecture;
  char vendor;
} processor_t;

END_C_NS
