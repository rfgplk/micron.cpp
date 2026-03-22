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

#include "processor.h"
#include "features.h"
#include "sysreg.h"
#include "macros.h"

BEGIN_C_NS

__inline__ char
vendor(const unsigned long midr)
{
  unsigned char impl = midr_implementer(midr);
  switch ( impl ) {
  case IMPL_ARM:       return (char)IMPL_ARM;
  case IMPL_BROADCOM:  return (char)IMPL_BROADCOM;
  case IMPL_CAVIUM:    return (char)IMPL_CAVIUM;
  case IMPL_DEC:       return (char)IMPL_DEC;
  case IMPL_FUJITSU:   return (char)IMPL_FUJITSU;
  case IMPL_HISILICON: return (char)IMPL_HISILICON;
  case IMPL_INFINEON:  return (char)IMPL_INFINEON;
  case IMPL_FREESCALE: return (char)IMPL_FREESCALE;
  case IMPL_NVIDIA:    return (char)IMPL_NVIDIA;
  case IMPL_APM:       return (char)IMPL_APM;
  case IMPL_QUALCOMM:  return (char)IMPL_QUALCOMM;
  case IMPL_SAMSUNG:   return (char)IMPL_SAMSUNG;
  case IMPL_MARVELL:   return (char)IMPL_MARVELL;
  case IMPL_APPLE:     return (char)IMPL_APPLE;
  case IMPL_FARADAY:   return (char)IMPL_FARADAY;
  case IMPL_HXT:       return (char)IMPL_HXT;
  case IMPL_INTEL:     return (char)IMPL_INTEL;
  case IMPL_MICROSOFT: return (char)IMPL_MICROSOFT;
  case IMPL_PHYTIUM:   return (char)IMPL_PHYTIUM;
  case IMPL_AMPERE:    return (char)IMPL_AMPERE;
  default:             return (char)IMPL_UNKNOWN;
  }
}

__inline__ void
spec(processor_t *__restrict__ const p)
{
  frequency_user(&p->core);
  cores_all(&p->core_total, &p->core);

  for ( int j = 0; j < p->core_total && p->core[j].type != CORE_TYPE_NONE; j++ ) {
    topology_set_cpu(j);
    for ( int i = 0; i < 4; i++ )
      cache_from_sysfs(j, i, &p->core[j].caches[i]);
  }
}

#ifdef ARCH_AARCH64

__inline__ void
info(processor_t *__restrict__ const p)
{
  hwcaps_t hw;
  arm_id_regs_t id;

  read_hwcaps(&hw);
  read_id_regs(&id);

  // MIDR
  p->implementer  = midr_implementer(id.midr);
  p->part         = midr_part(id.midr);
  p->variant      = midr_variant(id.midr);
  p->revision     = midr_revision(id.midr);
  p->architecture = midr_architecture(id.midr);
  p->vendor       = vendor(id.midr);

  const unsigned long h = hw.hwcap;
  const unsigned long long h2 = (unsigned long long)hw.hwcap2;

  // core ISA
  p->features.fp       = !!(h & _NF_A64_FP);
  p->features.asimd    = !!(h & _NF_A64_ASIMD);
  p->features.evtstrm  = !!(h & _NF_A64_EVTSTRM);

  // crypto
  p->features.aes      = !!(h & _NF_A64_AES);
  p->features.pmull    = !!(h & _NF_A64_PMULL);
  p->features.sha1     = !!(h & _NF_A64_SHA1);
  p->features.sha2     = !!(h & _NF_A64_SHA2);
  p->features.sha3     = !!(h & _NF_A64_SHA3);
  p->features.sha512   = !!(h & _NF_A64_SHA512);
  p->features.sm3      = !!(h & _NF_A64_SM3);
  p->features.sm4      = !!(h & _NF_A64_SM4);

  // simd / ext
  p->features.crc32    = !!(h & _NF_A64_CRC32);
  p->features.atomics  = !!(h & _NF_A64_ATOMICS);
  p->features.fphp     = !!(h & _NF_A64_FPHP);
  p->features.asimdhp  = !!(h & _NF_A64_ASIMDHP);
  p->features.asimdrdm = !!(h & _NF_A64_ASIMDRDM);
  p->features.asimddp  = !!(h & _NF_A64_ASIMDDP);
  p->features.asimdfhm = !!(h & _NF_A64_ASIMDFHM);
  p->features.bf16     = !!(h2 & _NF_A64_BF16);
  p->features.ebf16    = !!(h2 & _NF_A64_EBF16);
  p->features.i8mm     = !!(h2 & _NF_A64_I8MM);
  p->features.lse128   = !!(h2 & _NF_A64_LSE128);

  // branching
  p->features.jscvt    = !!(h & _NF_A64_JSCVT);
  p->features.fcma     = !!(h & _NF_A64_FCMA);
  p->features.flagm    = !!(h & _NF_A64_FLAGM);
  p->features.flagm2   = !!(h2 & _NF_A64_FLAGM2);

  // ordering
  p->features.lrcpc    = !!(h & _NF_A64_LRCPC);
  p->features.ilrcpc   = !!(h & _NF_A64_ILRCPC);
  p->features.lrcpc3   = !!(h2 & _NF_A64_LRCPC3);
  p->features.uscat    = !!(h & _NF_A64_USCAT);
  p->features.dcpop    = !!(h & _NF_A64_DCPOP);
  p->features.dcpodp   = !!(h2 & _NF_A64_DCPODP);

  // misc
  p->features.frint    = !!(h2 & _NF_A64_FRINT);
  p->features.dit      = !!(h & _NF_A64_DIT);
  p->features.rng      = !!(h2 & _NF_A64_RNG);
  p->features.dgh      = !!(h2 & _NF_A64_DGH);
  p->features.cssc     = !!(h2 & _NF_A64_CSSC);
  p->features.mops     = !!(h2 & _NF_A64_MOPS);
  p->features.hbc      = !!(h2 & _NF_A64_HBC);
  p->features.wfxt     = !!(h2 & _NF_A64_WFXT);
  p->features.rpres    = !!(h2 & _NF_A64_RPRES);
  p->features.ecv      = !!(h2 & _NF_A64_ECV);
  p->features.afp      = !!(h2 & _NF_A64_AFP);
  p->features.rprfm    = !!(h2 & _NF_A64_RPRFM);
  p->features.fpmr     = !!(h2 & _NF_A64_FPMR);
  p->features.poe      = !!(h2 & _NF_A64_POE);

  // pointer auth
  p->features.paca      = !!(h & _NF_A64_PACA);
  p->features.pacg      = !!(h & _NF_A64_PACG);
  p->features.bti       = !!(h2 & _NF_A64_BTI);
  p->features.ssbs      = !!(h & _NF_A64_SSBS);
  p->features.sb        = !!(h & _NF_A64_SB);
  p->features.spec_ctrl = !!(h & _NF_A64_CPUID);

  // mte
  p->features.mte      = !!(h2 & _NF_A64_MTE);
  p->features.mte3     = !!(h2 & _NF_A64_MTE3);

  // sve
  p->features.sve        = !!(h & _NF_A64_SVE);
  p->features.sve2       = !!(h2 & _NF_A64_SVE2);
  p->features.sve2p1     = !!(h2 & _NF_A64_SVE2P1);
  p->features.sve_aes    = !!(h2 & _NF_A64_SVEAES);
  p->features.sve_pmull  = !!(h2 & _NF_A64_SVEPMULL);
  p->features.sve_bitperm = !!(h2 & _NF_A64_SVEBITPERM);
  p->features.sve_sha3   = !!(h2 & _NF_A64_SVESHA3);
  p->features.sve_sm4    = !!(h2 & _NF_A64_SVESM4);
  p->features.sve_i8mm   = !!(h2 & _NF_A64_SVEI8MM);
  p->features.sve_f32mm  = !!(h2 & _NF_A64_SVEF32MM);
  p->features.sve_f64mm  = !!(h2 & _NF_A64_SVEF64MM);
  p->features.sve_bf16   = !!(h2 & _NF_A64_SVEBF16);
  p->features.sve_ebf16  = !!(h2 & _NF_A64_SVE_EBF16);
  p->features.sve_b16b16 = !!(h2 & _NF_A64_SVE_B16B16);

  // sme matrix
  p->features.sme         = !!(h2 & _NF_A64_SME);
  p->features.sme2        = !!(h2 & _NF_A64_SME2);
  p->features.sme2p1      = !!(h2 & _NF_A64_SME2P1);
  p->features.sme_i16i64  = !!(h2 & _NF_A64_SME_I16I64);
  p->features.sme_f64f64  = !!(h2 & _NF_A64_SME_F64F64);
  p->features.sme_i8i32   = !!(h2 & _NF_A64_SME_I8I32);
  p->features.sme_f16f32  = !!(h2 & _NF_A64_SME_F16F32);
  p->features.sme_b16f32  = !!(h2 & _NF_A64_SME_B16F32);
  p->features.sme_f32f32  = !!(h2 & _NF_A64_SME_F32F32);
  p->features.sme_fa64    = !!(h2 & _NF_A64_SME_FA64);
  p->features.sme_i16i32  = !!(h2 & _NF_A64_SME_I16I32);
  p->features.sme_bi32i32 = !!(h2 & _NF_A64_SME_BI32I32);
  p->features.sme_b16b16  = !!(h2 & _NF_A64_SME_B16B16);
  p->features.sme_f16f16  = !!(h2 & _NF_A64_SME_F16F16);
  p->features.sme_lutv2   = !!(h2 & _NF_A64_SME_LUTV2);
  p->features.sme_f8f16   = !!(h2 & _NF_A64_SME_F8F16);
  p->features.sme_f8f32   = !!(h2 & _NF_A64_SME_F8F32);
  p->features.sme_sf8fma  = !!(h2 & _NF_A64_SME_SF8FMA);
  p->features.sme_sf8dp4  = !!(h2 & _NF_A64_SME_SF8DP4);
  p->features.sme_sf8dp2  = !!(h2 & _NF_A64_SME_SF8DP2);

  // fp8
  p->features.lut      = !!(h2 & _NF_A64_LUT);
  p->features.faminmax = !!(h2 & _NF_A64_FAMINMAX);
  p->features.f8cvt    = !!(h2 & _NF_A64_F8CVT);
  p->features.f8fma    = !!(h2 & _NF_A64_F8FMA);
  p->features.f8dp4    = !!(h2 & _NF_A64_F8DP4);
  p->features.f8dp2    = !!(h2 & _NF_A64_F8DP2);
  p->features.f8e4m3   = !!(h2 & _NF_A64_F8E4M3);
  p->features.f8e5m2   = !!(h2 & _NF_A64_F8E5M2);
  p->features.f16c = p->features.fphp && p->features.asimdhp;

  // aarch32 only
  p->features.a32_swp = 0;
  p->features.a32_half = 0;
  p->features.a32_thumb = 0;
  p->features.a32_26bit = 0;
  p->features.a32_fast_mult = 0;
  p->features.a32_fpa = 0;
  p->features.a32_vfp = 0;
  p->features.a32_edsp = 0;
  p->features.a32_java = 0;
  p->features.a32_iwmmxt = 0;
  p->features.a32_crunch = 0;
  p->features.a32_thumbee = 0;
  p->features.a32_neon = 0;
  p->features.a32_vfpv3 = 0;
  p->features.a32_vfpv3d16 = 0;
  p->features.a32_tls = 0;
  p->features.a32_vfpv4 = 0;
  p->features.a32_idiva = 0;
  p->features.a32_idivt = 0;
  p->features.a32_vfpd32 = 0;
  p->features.a32_lpae = 0;
  p->features.a32_evtstrm = 0;
  p->features.a32_aes = 0;
  p->features.a32_pmull = 0;
  p->features.a32_sha1 = 0;
  p->features.a32_sha2 = 0;
  p->features.a32_crc32 = 0;
}

#endif

#ifdef ARCH_AARCH32

__inline__ void
info(processor_t *__restrict__ const p)
{
  hwcaps_t hw;
  arm_id_regs_t id;

  read_hwcaps(&hw);
  read_id_regs(&id);

  // MIDR
  p->implementer  = midr_implementer(id.midr);
  p->part         = midr_part(id.midr);
  p->variant      = midr_variant(id.midr);
  p->revision     = midr_revision(id.midr);
  p->architecture = midr_architecture(id.midr);
  p->vendor       = vendor(id.midr);

  const unsigned long h  = hw.hwcap;
  const unsigned long h2 = hw.hwcap2;

  // hwcaps
  p->features.a32_swp       = !!(h & _NF_A32_SWP);
  p->features.a32_half      = !!(h & _NF_A32_HALF);
  p->features.a32_thumb     = !!(h & _NF_A32_THUMB);
  p->features.a32_26bit     = !!(h & _NF_A32_26BIT);
  p->features.a32_fast_mult = !!(h & _NF_A32_FAST_MULT);
  p->features.a32_fpa       = !!(h & _NF_A32_FPA);
  p->features.a32_vfp       = !!(h & _NF_A32_VFP);
  p->features.a32_edsp      = !!(h & _NF_A32_EDSP);
  p->features.a32_java      = !!(h & _NF_A32_JAVA);
  p->features.a32_iwmmxt    = !!(h & _NF_A32_IWMMXT);
  p->features.a32_crunch    = !!(h & _NF_A32_CRUNCH);
  p->features.a32_thumbee   = !!(h & _NF_A32_THUMBEE);
  p->features.a32_neon      = !!(h & _NF_A32_NEON);
  p->features.a32_vfpv3     = !!(h & _NF_A32_VFPv3);
  p->features.a32_vfpv3d16  = !!(h & _NF_A32_VFPv3D16);
  p->features.a32_tls       = !!(h & _NF_A32_TLS);
  p->features.a32_vfpv4     = !!(h & _NF_A32_VFPv4);
  p->features.a32_idiva     = !!(h & _NF_A32_IDIVA);
  p->features.a32_idivt     = !!(h & _NF_A32_IDIVT);
  p->features.a32_vfpd32    = !!(h & _NF_A32_VFPD32);
  p->features.a32_lpae      = !!(h & _NF_A32_LPAE);
  p->features.a32_evtstrm   = !!(h & _NF_A32_EVTSTRM);

  // crypto
  p->features.a32_aes       = !!(h2 & _NF_A32_AES);
  p->features.a32_pmull     = !!(h2 & _NF_A32_PMULL);
  p->features.a32_sha1      = !!(h2 & _NF_A32_SHA1);
  p->features.a32_sha2      = !!(h2 & _NF_A32_SHA2);
  p->features.a32_crc32     = !!(h2 & _NF_A32_CRC32);

  p->features.fp      = p->features.a32_vfp;
  p->features.asimd   = p->features.a32_neon;
  p->features.evtstrm = p->features.a32_evtstrm;
  p->features.aes     = p->features.a32_aes;
  p->features.pmull   = p->features.a32_pmull;
  p->features.sha1    = p->features.a32_sha1;
  p->features.sha2    = p->features.a32_sha2;
  p->features.crc32   = p->features.a32_crc32;

  // aarch64 only
  p->features.sha3 = 0;
  p->features.sha512 = 0;
  p->features.sm3 = 0;
  p->features.sm4 = 0;
  p->features.atomics = 0;
  p->features.lse128 = 0;
  p->features.fphp = 0;
  p->features.asimdhp = 0;
  p->features.asimdrdm = 0;
  p->features.asimddp = 0;
  p->features.asimdfhm = 0;
  p->features.bf16 = 0;
  p->features.ebf16 = 0;
  p->features.i8mm = 0;
  p->features.f16c = 0;
  p->features.jscvt = 0;
  p->features.fcma = 0;
  p->features.flagm = 0;
  p->features.flagm2 = 0;
  p->features.lrcpc = 0;
  p->features.ilrcpc = 0;
  p->features.lrcpc3 = 0;
  p->features.uscat = 0;
  p->features.dcpop = 0;
  p->features.dcpodp = 0;
  p->features.frint = 0;
  p->features.dit = 0;
  p->features.rng = 0;
  p->features.dgh = 0;
  p->features.cssc = 0;
  p->features.mops = 0;
  p->features.hbc = 0;
  p->features.wfxt = 0;
  p->features.rpres = 0;
  p->features.ecv = 0;
  p->features.afp = 0;
  p->features.rprfm = 0;
  p->features.fpmr = 0;
  p->features.poe = 0;
  p->features.paca = 0;
  p->features.pacg = 0;
  p->features.bti = 0;
  p->features.ssbs = 0;
  p->features.sb = 0;
  p->features.spec_ctrl = 0;
  p->features.mte = 0;
  p->features.mte3 = 0;
  p->features.sve = 0;
  p->features.sve2 = 0;
  p->features.sve2p1 = 0;
  p->features.sve_aes = 0;
  p->features.sve_pmull = 0;
  p->features.sve_bitperm = 0;
  p->features.sve_sha3 = 0;
  p->features.sve_sm4 = 0;
  p->features.sve_i8mm = 0;
  p->features.sve_f32mm = 0;
  p->features.sve_f64mm = 0;
  p->features.sve_bf16 = 0;
  p->features.sve_ebf16 = 0;
  p->features.sve_b16b16 = 0;
  p->features.sme = 0;
  p->features.sme2 = 0;
  p->features.sme2p1 = 0;
  p->features.sme_i16i64 = 0;
  p->features.sme_f64f64 = 0;
  p->features.sme_i8i32 = 0;
  p->features.sme_f16f32 = 0;
  p->features.sme_b16f32 = 0;
  p->features.sme_f32f32 = 0;
  p->features.sme_fa64 = 0;
  p->features.sme_i16i32 = 0;
  p->features.sme_bi32i32 = 0;
  p->features.sme_b16b16 = 0;
  p->features.sme_f16f16 = 0;
  p->features.sme_lutv2 = 0;
  p->features.sme_f8f16 = 0;
  p->features.sme_f8f32 = 0;
  p->features.sme_sf8fma = 0;
  p->features.sme_sf8dp4 = 0;
  p->features.sme_sf8dp2 = 0;
  p->features.lut = 0;
  p->features.faminmax = 0;
  p->features.f8cvt = 0;
  p->features.f8fma = 0;
  p->features.f8dp4 = 0;
  p->features.f8dp2 = 0;
  p->features.f8e4m3 = 0;
  p->features.f8e5m2 = 0;
}

#endif

END_C_NS
