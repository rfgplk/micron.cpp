//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// no longer uses system immintrin.h and associated headers
// immintin.h pulled in mm_malloc.h/stdlib.h/and a whole host of other system headers which we DON'T need + it interfered with other code in
// our lib
//
// currently all code dispatches to a compiler builtin wherever possible rather than inline assembly
//
// NOTE: MICRON_SIMD_INJECT_INTRIN_TYPES + MICRON_SIMD_INJECT_INTRIN_SYMS alias all declarations back to global scope so you don't need to
// append micron::simd::... for each

#include "../bits/__arch.hpp"

// NOTE: force prevent #include <immintrin.h> from including any code, this isn't portable and may break, if you get namespace resolution
// issues this is the place to look
#ifndef __IMMINTRIN_H
#define __IMMINTRIN_H 1
#endif
#ifndef _IMMINTRIN_H_INCLUDED
#define _IMMINTRIN_H_INCLUDED 1
#endif
#ifndef _XMMINTRIN_H_INCLUDED
#define _XMMINTRIN_H_INCLUDED 1
#endif
#ifndef _EMMINTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED 1
#endif
#ifndef _PMMINTRIN_H_INCLUDED
#define _PMMINTRIN_H_INCLUDED 1
#endif
#ifndef _TMMINTRIN_H_INCLUDED
#define _TMMINTRIN_H_INCLUDED 1
#endif
#ifndef _SMMINTRIN_H_INCLUDED
#define _SMMINTRIN_H_INCLUDED 1
#endif
#ifndef _NMMINTRIN_H_INCLUDED
#define _NMMINTRIN_H_INCLUDED 1
#endif
#ifndef _WMMINTRIN_H_INCLUDED
#define _WMMINTRIN_H_INCLUDED 1
#endif
#ifndef _AVXINTRIN_H_INCLUDED
#define _AVXINTRIN_H_INCLUDED 1
#endif
#ifndef _AVX2INTRIN_H_INCLUDED
#define _AVX2INTRIN_H_INCLUDED 1
#endif
#ifndef _MMINTRIN_H_INCLUDED
#define _MMINTRIN_H_INCLUDED 1
#endif
#ifndef _ARM_NEON_H
#define _ARM_NEON_H 1
#endif
#ifndef __ARM_NEON_H
#define __ARM_NEON_H 1
#endif
#ifndef _AARCH64_NEON_H_
#define _AARCH64_NEON_H_ 1
#endif

// WARNING: these MUST be set before any __bits/* header is included
#ifndef MICRON_SIMD_INJECT_INTRIN_TYPES
#define MICRON_SIMD_INJECT_INTRIN_TYPES 1
#endif
#ifndef MICRON_SIMD_INJECT_INTRIN_SYMS
#define MICRON_SIMD_INJECT_INTRIN_SYMS 1
#endif

// x86 / amd64
#if defined(__micron_arch_x86_any)

#include "__bits/__mask_types.hpp"
#include "__bits/__round_modes.hpp"
#include "__bits/__vector_types_amd64.hpp"

// NOTE: always pull every ISA header on x86; each file has its own gnu::target attribute
// this allows you to compile even when targets are mismatched
#include "__bits/__aes.hpp"
#include "__bits/__avx.hpp"
#include "__bits/__avx2.hpp"
#include "__bits/__avx512bw.hpp"
#include "__bits/__avx512dq.hpp"
#include "__bits/__avx512f.hpp"
#include "__bits/__avx512vl.hpp"
#include "__bits/__bmi.hpp"
#include "__bits/__fma.hpp"
#include "__bits/__popcnt.hpp"
#include "__bits/__prefetch_fence.hpp"
#include "__bits/__sse2.hpp"
#include "__bits/__sse3.hpp"
#include "__bits/__sse4_1.hpp"
#include "__bits/__sse4_2.hpp"
#include "__bits/__ssse3.hpp"

#include "__bits/__imm_shuffles.hpp"

#endif

// AArch64 NEON
#if defined(__micron_arch_arm64) && defined(__micron_arm_neon)

#include "__bits/__neon_arm64.hpp"
#include "__bits/__vector_types_arm64.hpp"

#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
#include "__bits/__neon_fma.hpp"
#endif

#if defined(__micron_arm_aes) || defined(__ARM_FEATURE_AES) || defined(__ARM_FEATURE_CRYPTO)
#include "__bits/__neon_aes.hpp"
#endif

#if defined(__micron_arm_crc32) || defined(__ARM_FEATURE_CRC32)
#include "__bits/__neon_crc32.hpp"
#endif

#endif

// armv7-a NEON
#if defined(__micron_arch_arm32) && defined(__micron_arm_neon)

#include "__bits/__neon_arm32.hpp"
#include "__bits/__vector_types_arm32.hpp"

#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
#include "__bits/__neon_fma.hpp"
#endif

#if defined(__micron_arm_aes) || defined(__ARM_FEATURE_AES) || defined(__ARM_FEATURE_CRYPTO)
#include "__bits/__neon_aes.hpp"
#endif

#if defined(__micron_arm_crc32) || defined(__ARM_FEATURE_CRC32)
#include "__bits/__neon_crc32.hpp"
#endif

#endif

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
using namespace ::micron::simd::__bits;
#endif

#if defined(__micron_freestanding)
#if defined(_STDLIB_H) || defined(_SYS_TYPES_H) || defined(_FEATURES_H) || defined(__MM_MALLOC_H_INCLUDED)
#error "vendor SIMD header leaked libc into a freestanding micron build"
#endif
#endif
