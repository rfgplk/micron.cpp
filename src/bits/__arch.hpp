//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// guard cross arch compiles

#ifndef __MICRON_ARCH_H
#define __MICRON_ARCH_H

#include "__compilers.hpp"

// this is here so we don't clutter the root dir
// TODO: change all code macros to use these defs

#if defined(__GNUC__)
#define GCC_VERSION_FULL (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#endif

// NOTE: ARM checks come before the x86/ILP32 fallback because clang on
// armv7 defines `__ILP32__` (pointers are 32-bit) in addition to `__arm__`.
// If x86/ILP32 was checked first, clang arm32 would misroute as 32-bit x86
// and never reach the arm32 branch below.
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#define __micron_arch_amd64 1
#define __micron_arch_width_64 1
#define __wordsize 64
#define __syscall_wordsize 64
inline constexpr unsigned __micron_arch = __micron_arch_amd64;
inline constexpr unsigned __micron_width = __wordsize;
#elif defined(__aarch64__) || defined(__AARCH64EL__) || defined(__AARCH64EB__)
#define __micron_arch_arm64 3
#define __micron_arch_aarch64 3
#define __micron_arch_width_64 1
#define __syscall_wordsize 64
#define __wordsize 64
inline constexpr unsigned __micron_arch = __micron_arch_arm64;
inline constexpr unsigned __micron_width = __wordsize;
#elif defined(__arm__) || defined(__thumb__) || defined(__ARMEL__) || defined(__ARMEB__) || defined(__ARM_ARCH)
#define __micron_arch_arm32 4
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
inline constexpr unsigned __micron_arch = __micron_arch_arm32;
inline constexpr unsigned __micron_width = __wordsize;
#elif defined(__i386__) || defined(_M_IX86) || defined(__ILP32__)
#define __micron_arch_x86 2
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
inline constexpr unsigned __micron_arch = __micron_arch_x86;
inline constexpr unsigned __micron_width = __wordsize;
#elif defined(MICRON_ALLOW_GENERIC_ARCH)
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the generic tier
//
// no ISA backend, no syscall table, no arch-specific asm; everything routes through the scalar
// paths. this is what a new target (riscv64, xtensa, ...) enters on, and it is opt-in so a
// mis-targeted build still fails loudly instead of silently degrading
#define __micron_arch_generic 5
#if !defined(__SIZEOF_POINTER__)
#error "generic arch tier needs __SIZEOF_POINTER__ to size the word"
#endif
#if __SIZEOF_POINTER__ == 8
#define __micron_arch_width_64 1
#define __wordsize 64
#define __syscall_wordsize 64
#elif __SIZEOF_POINTER__ == 4
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
#else
#error "generic arch tier supports 32- and 64-bit pointers only"
#endif
inline constexpr unsigned __micron_arch = __micron_arch_generic;
inline constexpr unsigned __micron_width = __wordsize;
#else
#error                                                                                                                                     \
    "Unsupported architecture. Is your compiler working properly? (define MICRON_ALLOW_GENERIC_ARCH to build on the scalar generic tier)"
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
#define __micron_arch_x86_any 1
#endif

#if defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
#define __micron_arch_arm_any 1
#endif

// arches whose kernel ABI is the asm-generic unified syscall table (arm64; riscv64 in the future)
#if defined(__micron_arch_arm64)
#define __micron_syscall_generic 1
#endif

#if defined(__micron_arch_width_64)
#define __micron_ptr_size 8
#define __micron_ptr_bits 64
#elif defined(__micron_arch_width_32)
#define __micron_ptr_size 4
#define __micron_ptr_bits 32
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// extended scalars
#if defined(__SIZEOF_INT128__)
#define __micron_has_int128 1
#endif

#if defined(__LDBL_MANT_DIG__)
#if __LDBL_MANT_DIG__ == 53
#define __micron_ldbl_binary64 1
#elif __LDBL_MANT_DIG__ == 64
#define __micron_ldbl_x87_80 1
#elif __LDBL_MANT_DIG__ == 113
#define __micron_ldbl_binary128 1
#endif
#endif

#if defined(__SIZEOF_LONG_DOUBLE__)
#define __micron_ldbl_bytes __SIZEOF_LONG_DOUBLE__
#endif

#if defined(__FLT128_MANT_DIG__) && defined(__micron_has_int128) && !defined(__micron_ldbl_binary128)
#define __micron_f128_distinct 1
#endif

#if defined(__micron_f128_distinct) || defined(__micron_ldbl_x87_80) || defined(__micron_ldbl_binary128)
#define __micron_has_wide_float 1
#endif

// default kernel base page size; arm64 kernels run 4KB, 16KB, or 64KB base pages
//
// (E) HAS NO MMU AND THEREFORE NO PAGE. On bare metal this constant stops meaning "what the
// hardware maps" and starts meaning "the granule port::page_alloc carves the linker pool in".
// MICRON_PORT_PAGE_SIZE names it; the default is 64 and not 4096 because a 256 KiB device cannot
// afford a 4 KiB quantum, and 64 is a power of two >= micron::bb::native_alignment.
//
// IT MUST NEVER BE 0. pages_linux.hpp used to prescribe "default 0, meaning no paging", and that
// does not build: micron::page_size feeds allocation_policy<page_size, page_size, 3, 1>
// (allocation/policies.hpp:30) whose first static_assert is `Granularity != 0`, and bb::carve
// rounds a span by it (barebones/bb_alloc.hpp:177). "Is there paging" is answered by
// port::has_paging, which is a bool, not by overloading a size with a sentinel.
//
// Keyed on MICRON_PORT_METAL rather than __micron_port_metal: port/__backend.hpp includes THIS file
// before deriving the internal spelling, so the internal one does not exist yet at this point.
#if defined(MICRON_PORT_METAL)
#if defined(MICRON_PORT_PAGE_SIZE)
#define __micron_page_size_default MICRON_PORT_PAGE_SIZE
#else
#define __micron_page_size_default 64
#endif
#elif defined(__micron_arch_arm64)
#define __micron_page_size_default 65536
#else
#define __micron_page_size_default 4096
#endif

#if defined(__micron_arch_x86_any)

// sse
#if defined(__SSE__)
#define __micron_x86_sse 1
#endif
#if defined(__SSE2__)
#define __micron_x86_sse2 1
#endif
#if defined(__SSE3__)
#define __micron_x86_sse3 1
#endif
#if defined(__SSSE3__)
#define __micron_x86_ssse3 1
#endif
#if defined(__SSE4_1__)
#define __micron_x86_sse4_1 1
#endif
#if defined(__SSE4_2__)
#define __micron_x86_sse4_2 1
#endif

// avx
#if defined(__AVX__)
#define __micron_x86_avx 1
#endif
#if defined(__AVX2__)
#define __micron_x86_avx2 1
#endif
#if defined(__AVX512F__)
#define __micron_x86_avx512f 1
#endif
#if defined(__AVX512CD__)
#define __micron_x86_avx512cd 1
#endif
#if defined(__AVX512BW__)
#define __micron_x86_avx512bw 1
#endif
#if defined(__AVX512DQ__)
#define __micron_x86_avx512dq 1
#endif
#if defined(__AVX512VL__)
#define __micron_x86_avx512vl 1
#endif
#if defined(__AVX512VNNI__)
#define __micron_x86_avx512vnni 1
#endif
#if defined(__AVX512BF16__)
#define __micron_x86_avx512bf16 1
#endif
#if defined(__AVX512FP16__)
#define __micron_x86_avx512fp16 1
#endif

// crypto
#if defined(__AES__)
#define __micron_x86_aes 1
#endif
#if defined(__PCLMUL__)
#define __micron_x86_pclmul 1
#endif
#if defined(__SHA__)
#define __micron_x86_sha 1
#endif
#if defined(__RDRND__)
#define __micron_x86_rdrnd 1
#endif
#if defined(__RDSEED__)
#define __micron_x86_rdseed 1
#endif

// bmis
#if defined(__BMI__)
#define __micron_x86_bmi1 1
#endif
#if defined(__BMI2__)
#define __micron_x86_bmi2 1
#endif
#if defined(__LZCNT__)
#define __micron_x86_lzcnt 1
#endif
#if defined(__POPCNT__)
#define __micron_x86_popcnt 1
#endif
#if defined(__TBM__)
#define __micron_x86_tbm 1
#endif
#if defined(__FMA__)
#define __micron_x86_fma 1
#endif
#if defined(__FMA4__)
#define __micron_x86_fma4 1
#endif
#if defined(__F16C__)
#define __micron_x86_f16c 1
#endif
#if defined(__ADX__)
#define __micron_x86_adx 1
#endif
#if defined(__MOVBE__)
#define __micron_x86_movbe 1
#endif
#if defined(__XSAVE__)
#define __micron_x86_xsave 1
#endif
#if defined(__FSGSBASE__)
#define __micron_x86_fsgsbase 1
#endif
#if defined(__CRC32__)
#define __micron_x86_crc32 1
#endif
#if defined(__RTM__)
#define __micron_x86_rtm 1
#endif
#if defined(__HLE__)
#define __micron_x86_hle 1
#endif
#if defined(__MPX__)
#define __micron_x86_mpx 1
#endif
#if defined(__VAES__)
#define __micron_x86_vaes 1
#endif
#if defined(__VPCLMULQDQ__)
#define __micron_x86_vpclmulqdq 1
#endif
#if defined(__GFNI__)
#define __micron_x86_gfni 1
#endif
#if defined(__CLFLUSHOPT__)
#define __micron_x86_clflushopt 1
#endif
#if defined(__CLWB__)
#define __micron_x86_clwb 1
#endif
#if defined(__WAITPKG__)
#define __micron_x86_waitpkg 1
#endif
#if defined(__ENQCMD__)
#define __micron_x86_enqcmd 1
#endif
#if defined(__SERIALIZE__)
#define __micron_x86_serialize 1
#endif
#if defined(__UINTR__)
#define __micron_x86_uintr 1
#endif
#if defined(__AMX_BF16__) || defined(__AMX_INT8__) || defined(__AMX_TILE__)
#define __micron_x86_amx 1
#endif

#if defined(__micron_x86_avx512f)
#define __micron_x86_simd_width 512
#elif defined(__micron_x86_avx2) || defined(__micron_x86_avx)
#define __micron_x86_simd_width 256
#elif defined(__micron_x86_sse2)
#define __micron_x86_simd_width 128
#else
#define __micron_x86_simd_width 0
#endif

#endif

// the width ladders in algorithm/memory.hpp and array/carray.hpp read this unconditionally, so a
// non-x86 target needs the name to exist rather than be taken as an undefined 0
#if !defined(__micron_x86_simd_width)
#define __micron_x86_simd_width 0
#endif

#if defined(__micron_arch_arm_any)

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define __micron_arm_neon 1
#endif

// sve
#if defined(__ARM_FEATURE_SVE)
#define __micron_arm_sve 1
#endif
#if defined(__ARM_FEATURE_SVE2)
#define __micron_arm_sve2 1
#endif
#if defined(__ARM_FEATURE_SVE_BF16)
#define __micron_arm_sve_bf16 1
#endif
#if defined(__ARM_FEATURE_SVE2_AES)
#define __micron_arm_sve2_aes 1
#endif
#if defined(__ARM_FEATURE_SVE2_SHA3)
#define __micron_arm_sve2_sha3 1
#endif
#if defined(__ARM_FEATURE_SVE2_SM4)
#define __micron_arm_sve2_sm4 1
#endif
#if defined(__ARM_FEATURE_SVE2_BITPERM)
#define __micron_arm_sve2_bitperm 1
#endif

// sme
#if defined(__ARM_FEATURE_SME)
#define __micron_arm_sme 1
#endif
#if defined(__ARM_FEATURE_SME2)
#define __micron_arm_sme2 1
#endif

// crypto
#if defined(__ARM_FEATURE_AES)
#define __micron_arm_aes 1
#endif
#if defined(__ARM_FEATURE_SHA2)
#define __micron_arm_sha2 1
#endif
#if defined(__ARM_FEATURE_SHA3)
#define __micron_arm_sha3 1
#endif
#if defined(__ARM_FEATURE_SHA512)
#define __micron_arm_sha512 1
#endif
#if defined(__ARM_FEATURE_SM3)
#define __micron_arm_sm3 1
#endif
#if defined(__ARM_FEATURE_SM4)
#define __micron_arm_sm4 1
#endif
#if defined(__ARM_FEATURE_CRYPTO)
#define __micron_arm_crypto 1
#endif
// WARNING: (a wtf warning)__ARM_FEATURE_PMULL alone isn't defined by GCC (apparently?!?!) AArch64 at all
// per ACLE, on AArch64 it is __ARM_FEATURE_AES that covers AESE/AESD/AESMC/AESIMC *and* PMULL/PMULL2
// __ARM_FEATURE_PMULL is in practice an AArch32 spelling
#if defined(__ARM_FEATURE_PMULL) || defined(__ARM_FEATURE_CRYPTO) || (defined(__micron_arch_arm64) && defined(__ARM_FEATURE_AES))
#define __micron_arm_pmull 1
#endif
#if defined(__ARM_FEATURE_RNG)
#define __micron_arm_rng 1
#endif

// fp
#if defined(__ARM_FP)
#define __micron_arm_fp 1
#endif
#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__ARM_FP16_FORMAT_ALTERNATIVE)
#define __micron_arm_fp16 1
#endif
#if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
#define __micron_arm_fp16_vec 1
#endif
#if defined(__ARM_FEATURE_BF16)
#define __micron_arm_bf16 1
#endif
#if defined(__ARM_FEATURE_FMA)
#define __micron_arm_fma 1
#endif
#if defined(__ARM_FEATURE_DIRECTED_ROUNDING)
#define __micron_arm_directed_rounding 1
#endif

#if defined(__ARM_FEATURE_CLZ)
#define __micron_arm_clz 1
#endif
#if defined(__ARM_FEATURE_CRC32)
#define __micron_arm_crc32 1
#endif
#if defined(__ARM_FEATURE_DOTPROD)
#define __micron_arm_dotprod 1
#endif
#if defined(__ARM_FEATURE_MATMUL_INT8)
#define __micron_arm_i8mm 1
#endif
#if defined(__ARM_FEATURE_COMPLEX)
#define __micron_arm_fcma 1
#endif
#if defined(__ARM_FEATURE_JCVT)
#define __micron_arm_jcvt 1
#endif
#if defined(__ARM_FEATURE_QRDMX)
#define __micron_arm_rdma 1
#endif

#if defined(__ARM_FEATURE_ATOMICS)
#define __micron_arm_lse 1
#endif
#if defined(__ARM_FEATURE_TME)
#define __micron_arm_tme 1
#endif
#if defined(__ARM_FEATURE_MEMORY_TAGGING)
#define __micron_arm_mte 1
#endif
#if defined(__ARM_FEATURE_BTI)
#define __micron_arm_bti 1
#endif
#if defined(__ARM_FEATURE_PAC_DEFAULT)
#define __micron_arm_pac 1
#endif

#if defined(__micron_arch_arm32)
#if defined(__ARM_ARCH)
#define __micron_arm_arch_version __ARM_ARCH
#endif
#if defined(__thumb2__) || defined(__THUMB_INTERWORK__)
#define __micron_arm_thumb2 1
#endif
#if defined(__thumb__)
#define __micron_arm_thumb 1
#endif
#endif

#if defined(__micron_arm_sve2)
#define __micron_arm_simd_tier 3
#elif defined(__micron_arm_sve)
#define __micron_arm_simd_tier 2
#elif defined(__micron_arm_neon)
#define __micron_arm_simd_tier 1
#else
#define __micron_arm_simd_tier 0
#endif

#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __micron_no_fp -- no hardware floating point is usable
//
// Distinct from __micron_simd_generic: that one is about vector registers, this one is about
// floating point at all. A Linux kernel module has neither, and most MCUs have no FPU. Code that
// only uses FP for a heuristic (abcmalloc's sheet-sizing curves) must take an integer path here.
//
// __GCC_IEC_559 == 0 means the target cannot do conforming binary FP -- true for x86 -mno-80387
// -mno-sse, for aarch64 -mgeneral-regs-only, and for arm32 -mfloat-abi=soft. It is ALSO zero under
// -ffast-math, hence the __FAST_MATH__ exclusion; and -Ofast together with kernel flags would
// therefore look like a normal FP target, which is why duck --kernel/--metal pass MICRON_NO_FP
// explicitly rather than relying on the derivation.
//
// AND __GCC_IEC_559 IS NOT ENOUGH ON ITS OWN. Measured on this box, the derivation as it stood
// answered WRONG in three of five configurations that matter:
//
//   g++ -mno-sse -mno-80387                    IEC_559=0                -> fires   correct
//   g++ -Ofast                                 IEC_559=0 FAST_MATH=1    -> no      correct
//   g++ -Ofast -mno-sse -mno-80387             IEC_559=0 FAST_MATH=1    -> NO      WRONG
//   clang++ -mno-sse -mno-80387                IEC_559 NOT DEFINED      -> NO      WRONG
//   clang++ --target=aarch64 -mgeneral-regs-only   IEC_559 NOT DEFINED  -> NO      WRONG
//
// clang does not define __GCC_IEC_559 at all, so on clang the third disjunct is unconditionally
// false and no-FP was only ever reachable by an explicit macro. duck --kernel covers itself by
// passing MICRON_NO_FP, but nothing covers a hand-rolled build or the clang no-FP arm.
//
// So ask the target directly as well. These are capability questions, in the spirit of hard rule
// #3 -- "gate on capability, never on arch" -- and each is the compiler's own statement that the
// register file is gone: no __ARM_FP on aarch64 means -mgeneral-regs-only, __SOFTFP__ is arm32
// -mfloat-abi=soft, and on amd64 the SysV ABI passes float and double in xmm, so no __SSE2__ there
// genuinely means no FP. All three are set by both compilers and survive -Ofast, which is exactly
// what __GCC_IEC_559 does not.
//
// THE SSE DISJUNCT IS amd64, NOT x86_any, AND THAT IS THE TRAP. i386 has x87 whether or not it has
// SSE: `g++ -m32 -march=i686` defines neither __SSE__ nor __SSE2__ and has a perfectly good FPU.
// Writing __micron_arch_x86_any here made every ordinary 32-bit hosted build report no-FP --
// measured, and caught only because the probe covered the negative case. On i386 the question is
// answered by __GCC_IEC_559 (which -mno-80387 does set to 0) or by the explicit macro.
//
// And key on the USER macros for the two ports, not the internal spellings: port/__backend.hpp
// includes THIS file, so __micron_port_kernel does not exist yet here. That is the same reason
// __micron_page_size_default at :150 reads MICRON_PORT_METAL rather than __micron_port_metal.
#if defined(MICRON_NO_FP) || defined(__micron_arch_generic) || defined(MICRON_PORT_KERNEL) || defined(MICRON_PORT_METAL)                     \
    || (defined(__GCC_IEC_559) && __GCC_IEC_559 == 0 && !defined(__FAST_MATH__))                                                            \
    || (defined(__micron_arch_amd64) && !defined(__SSE2__))                                                                                 \
    || (defined(__micron_arch_arm64) && !defined(__ARM_FP)) || defined(__SOFTFP__)
#define __micron_no_fp 1
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __micron_simd_generic -- the scalar SIMD tier selector
//
// one macro drives every simd/*.hpp ladder. it fires in three cases:
//   1. MICRON_NO_SIMD, set explicitly. this is what a kernel module on amd64 uses: the arch has
//      AVX-512, but touching an xmm register outside kernel_fpu_begin() corrupts user FP state
//   2. the generic arch tier, which has no ISA backend at all
//   3. an x86 build below SSE2 or an ARM build without NEON -- previously a hard #error
//
// the generic backends are plain C++ over compiler generic vectors, so they emit no instruction
// the build flags did not authorize (hard rule: no function emits an unauthorized instruction)
#if defined(MICRON_NO_SIMD) || defined(__micron_arch_generic)
#define __micron_simd_generic 1
#elif defined(__micron_arch_x86_any) && !defined(__micron_x86_sse2)
#define __micron_simd_generic 1
#elif defined(__micron_arch_arm_any) && !defined(__micron_arm_neon)
#define __micron_simd_generic 1
#endif

// a generic-SIMD build authorizes no vector width and no ARM SIMD tier, whatever -march said
#if defined(__micron_simd_generic)
#undef __micron_x86_simd_width
#define __micron_x86_simd_width 0
#undef __micron_arm_simd_tier
#define __micron_arm_simd_tier 0

// AND IT AUTHORIZES NO VECTOR ISA EITHER. The width and tier were cleared here and the
// ISA-PRESENCE macros were not, so roughly 150 `#if defined(__micron_x86_avx2)` / `#if
// defined(__micron_arm_neon)` sites across the tree stayed live under MICRON_NO_SIMD -- guarding
// code that needs vector REGISTERS on a question about what -march enabled. That is hard rule #3
// ("gate on capability, never on arch") and BAREBONES.md records four of these being repaired by
// hand as "the 9 red cells"; the class was never repaired. Clearing them here fixes all of it in
// one place and, better, converts any future one into a compile error rather than a wrong branch.
//
// THE SCALAR ISA MACROS ARE DELIBERATELY KEPT. popcnt, bmi1 and bmi2 are general-purpose-register
// instructions -- popcnt, tzcnt, andn, bzhi -- with no vector register anywhere near them, and they
// are perfectly legal in ring 0. MICRON_NO_SIMD means "no vector unit", not "no instructions newer
// than i386", and clearing these would cost a kernel build its bit-manipulation fast paths for
// nothing. fma goes, because it is xmm/ymm.
//
// Hash values do not move: hash.hpp:39 already gates __micron_hash_zzz on !__micron_simd_generic
// as well as on the ISA, so a MICRON_NO_SIMD build was already taking the ISA-free defaults.
// Verified against a fixed corpus across {plain, MICRON_NO_SIMD} x {base, v2, v3} before and after.
#undef __micron_x86_sse
#undef __micron_x86_sse2
#undef __micron_x86_sse3
#undef __micron_x86_ssse3
#undef __micron_x86_sse4_1
#undef __micron_x86_sse4_2
#undef __micron_x86_avx
#undef __micron_x86_avx2
#undef __micron_x86_fma
#undef __micron_x86_avx512f
#undef __micron_x86_avx512bw
#undef __micron_x86_avx512cd
#undef __micron_x86_avx512dq
#undef __micron_x86_avx512vl
#undef __micron_x86_avx512vnni
#undef __micron_x86_avx512bf16
#undef __micron_x86_avx512fp16
#undef __micron_arm_neon
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __micron_no_tls -- no ELF thread-local storage
//
// a kernel module and a bare-metal image have no TLS runtime, so a namespace-scope thread_local is
// not merely slow there, it does not link. code that keeps per-thread state for convenience must
// degrade to a single shared object under this macro.
//
// opt-in only, and deliberately NOT derived from __micron_arch_generic: a new arch says nothing
// about whether the target has TLS, and guessing wrong turns working state into shared state
// silently. duck --kernel/--metal pass it explicitly, the way they pass MICRON_NO_FP.
//
// It IS derived from the two port selectors, though, and that is a different question from the
// arch one: "is this a kernel module" and "is this a bare-metal image" both answer "there is no TLS
// runtime here" definitionally, which is what the paragraph above already says. Leaving it opt-in
// meant a hand-rolled -DMICRON_PORT_KERNEL build without -DMICRON_NO_TLS compiled thread_local
// state into a module that has no TLS -- and on x86-64 in ring 0 %fs is the PER-CPU base, so a
// @tpoff access is not a link error, it is a write into arbitrary per-CPU kernel memory.
//
// The USER macros, for the include-order reason given at :150 and in the no-FP block above:
// port/__backend.hpp includes this file, so the __micron_port_* spellings do not exist yet.
#if defined(MICRON_NO_TLS) || defined(MICRON_PORT_KERNEL) || defined(MICRON_PORT_METAL)
#define __micron_no_tls 1
#endif

// NOTE: this lib is only made for gcc, but it's good to have fallbacks

#if defined(__cplusplus)
#define __micron_lang_cpp 1
// NOTE: gcc still reports the c++26 placeholder 202400L, so this cannot test for a 2026xx value
#if __cplusplus > 202302L
#define __micron_lang_cpp26 1
#endif
#if __cplusplus >= 202302L
#define __micron_lang_cpp23 1
#endif
#if __cplusplus >= 202002L
#define __micron_lang_cpp20 1
#endif
#if __cplusplus >= 201703L
#define __micron_lang_cpp17 1
#endif
#if __cplusplus >= 201402L
#define __micron_lang_cpp14 1
#endif
#if __cplusplus >= 201103L
#define __micron_lang_cpp11 1
#endif
#endif

// reflection (p2996)
#if defined(__micron_lang_cpp26) && defined(__cpp_impl_reflection)
#define __micron_reflection 1
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// optimize flags

#if defined(__OPTIMIZE__)
#define __micron_optimizing 1
#else
#define __micron_debug_build 1
#endif

#if defined(__OPTIMIZE_SIZE__)
#define __micron_optimize_size 1
#endif

#if defined(NDEBUG)
#define __micron_ndebug 1
#endif

#if defined(__LTO__) || defined(__FLTO__)
#define __micron_lto 1
#endif

#if defined(__PROFILE_GENERATE__) || defined(__PROFILE_ARCS__)
#define __micron_pgo_instrument 1
#endif
#if defined(__PROFILE_USE__)
#define __micron_pgo_use 1
#endif

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
#define __micron_sanitize_asan 1
#endif
#if defined(__SANITIZE_THREAD__) || (defined(__has_feature) && __has_feature(thread_sanitizer))
#define __micron_sanitize_tsan 1
#endif
#if defined(__SANITIZE_MEMORY__) || (defined(__has_feature) && __has_feature(memory_sanitizer))
#define __micron_sanitize_msan 1
#endif
#if defined(__SANITIZE_UNDEFINED__) || (defined(__has_feature) && __has_feature(undefined_behavior_sanitizer))
#define __micron_sanitize_ubsan 1
#endif
#if defined(__micron_sanitize_asan) || defined(__micron_sanitize_tsan) || defined(__micron_sanitize_msan)                                  \
    || defined(__micron_sanitize_ubsan)
#define __micron_sanitized 1
#endif

#if defined(__micron_sanitize_asan) || defined(__micron_sanitize_tsan) || defined(__micron_sanitize_msan)
#define __micron_sanitizer_owns_heap 1
#endif

// %%%%%%%%%%%%%%%%%%%%%%%
// again, linux only but good to have all just in case

#if defined(__linux__)
#define __micron_os_linux 1
#elif defined(__APPLE__) && defined(__MACH__)
#define __micron_os_macos 1
#elif defined(_WIN32) || defined(_WIN64)
#define __micron_os_windows 1
#elif defined(__FreeBSD__)
#define __micron_os_freebsd 1
#elif defined(__OpenBSD__)
#define __micron_os_openbsd 1
#elif defined(__NetBSD__)
#define __micron_os_netbsd 1
#elif defined(__DragonFly__)
#define __micron_os_dragonfly 1
#elif defined(__ANDROID__)
#define __micron_os_android 1
#else
#define __micron_os_unknown 1
#endif

#if defined(__micron_os_linux) || defined(__micron_os_macos) || defined(__micron_os_freebsd) || defined(__micron_os_openbsd)               \
    || defined(__micron_os_netbsd) || defined(__micron_os_dragonfly) || defined(__micron_os_android)
#define __micron_os_posix 1
#endif

#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __micron_endian_little 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __micron_endian_big 1
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define __micron_endian_pdp 1
#endif
#elif defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN)
#define __micron_endian_little 1
#elif defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
#define __micron_endian_big 1
#endif

#if defined(__FAST_MATH__)
#define __micron_fast_math
#endif

#if !defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0
#define __micron_freestanding 1
#endif

#if defined(MICRON_ENABLE_ATTACH) || defined(MICRON_ATTACH_MODULE)
#define __micron_attach_capable 1
#endif

#if defined(__micron_arch_arm32)
using __micron_guard_t = int;
#else
using __micron_guard_t = long long int;
#endif

#if defined(MICRON_ATTACH_MODULE)
// WARNING: these have to be declared here, before anything else in the include graph
extern "C" {
__attribute__((visibility("hidden"))) int __cxa_guard_acquire(__micron_guard_t *);
__attribute__((visibility("hidden"))) void __cxa_guard_release(__micron_guard_t *);
__attribute__((visibility("hidden"))) void __cxa_guard_abort(__micron_guard_t *);
}
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __micron_no_ssp
//
// suppress the stack-protector prologue on a single function
//
// WARNING: __attribute__((naked)) does NOT imply "no compiler-generated prologue"
// under -fstack-protector-all gcc still prepends the canary spill (?!?!?!?!?):
//
//     ldr r3, [pc, #..]   ; &__stack_chk_guard
//     ldr r3, [r3]
//     str r3, [sp, #4]    ; <-- ABOVE sp: a naked fn reserved no frame
//     <the naked body>
//
// on x86 gcc emits no canary for naked fns and the store would land in the 128b red
// zone anyway, which is why amd64 never noticed
#if defined(__has_attribute)
#if __has_attribute(no_stack_protector)
#define __micron_no_ssp __attribute__((no_stack_protector))
#endif
#endif
#if !defined(__micron_no_ssp)
// gcc < 11 / clang < 12: the historical workaround. costs nothing here because every
// naked fn in the tree is also noinline, so losing the inline-into-caller opts is moot
#if defined(__micron_compiler_gcc)
#define __micron_no_ssp __attribute__((optimize("no-stack-protector")))
#else
#define __micron_no_ssp
#endif
#endif

#endif
