//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// this is here so we don't clutter the root dir
// TODO: change all code macros to use these defs

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#define __micron_arch_amd64 1
#define __micron_arch_width_64 1
#define __wordsize 64
#define __syscall_wordsize 64
#elif defined(__i386__) || defined(_M_IX86) || defined(__ILP32__)
#define __micron_arch_x86 1
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
#elif defined(__aarch64__) || defined(__AARCH64EL__) || defined(__AARCH64EB__)
#define __micron_arch_arm64 1
#define __micron_arch_width_64 1
#define __syscall_wordsize 64
#define __wordsize 64
#elif defined(__arm__) || defined(__thumb__) || defined(__ARMEL__) || defined(__ARMEB__) || defined(__ARM_ARCH)
#define __micron_arch_arm32 1
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
#else
#error "Unsupported architecture. Is your compiler working properly?"
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
#define __micron_arch_x86_any 1
#endif

#if defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
#define __micron_arch_arm_any 1
#endif

#if defined(__micron_arch_width_64)
#define __micron_ptr_size 8
#define __micron_ptr_bits 64
#elif defined(__micron_arch_width_32)
#define __micron_ptr_size 4
#define __micron_ptr_bits 32
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
#if defined(__ARM_FEATURE_PMULL)
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

// NOTE: this lib is only made for gcc, but it's good to have fallbacks

#if defined(__clang__)
#define __micron_compiler_clang 1
#define __micron_compiler_clang_major __clang_major__
#define __micron_compiler_clang_minor __clang_minor__
#define __micron_compiler_clang_patch __clang_patchlevel__
#elif defined(__GNUC__)
#define __micron_compiler_gcc 1
#define __micron_compiler_gcc_major __GNUC__
#define __micron_compiler_gcc_minor __GNUC_MINOR__
#define __micron_compiler_gcc_patch __GNUC_PATCHLEVEL__
#elif defined(_MSC_VER)
#define __micron_compiler_msvc 1
#define __micron_compiler_msvc_ver _MSC_VER
#elif defined(__INTEL_COMPILER) || defined(__ICC)
#define __micron_compiler_icc 1
#define __micron_compiler_icc_ver __INTEL_COMPILER
#elif defined(__INTEL_LLVM_COMPILER)
#define __micron_compiler_icx 1
#else
#define __micron_compiler_unknown 1
#endif

#if defined(__micron_compiler_gcc) || defined(__micron_compiler_clang)
#define __micron_compiler_gcc_compat 1
#endif

#if defined(__cplusplus)
#define __micron_lang_cpp 1
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

#if !defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0
#define __micron_freestanding 1
#endif
