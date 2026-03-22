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

#ifdef __cplusplus
#define BEGIN_C_NS extern "C" {
#define END_C_NS }
#else
#define BEGIN_C_NS
#define END_C_NS
#endif

#ifndef __GNUC__
#define __asm__ asm
#define __inline__ inline
#define __restrict__ restrict
#define __typeof__ typeof
#endif

#ifdef __clang__
#define COMPILER_CLANG
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define COMPILER_GCC
#endif

#ifdef _MSC_VER
#define COMPILER_MICROSOFT
#endif

#if defined(__unix__) || defined(__unix)
#define OS_UNIX
#endif

#if defined(__linux__) || defined(__linux)
#define OS_LINUX
#endif

#ifdef __APPLE__
#define OS_APPLE
#endif

#ifdef __ANDROID__
#define OS_ANDROID
#endif

#ifdef __aarch64__
#define ARCH_AARCH64
#endif

#ifdef __arm__
#define ARCH_AARCH32
#endif

#if !defined(__aarch64__) && !defined(__arm__)
#error "nanoflagsarm requires an ARM target (__aarch64__ or __arm__)"
#endif

#define OPENMO defined(_OPENMP)
#define OPTIMIZING defined(__OPTIMIZE__)
#define ALIGNMENT defined(__BIGGEST_ALIGNMENT__)
#define LITTLE_ENDIAN defined(__ORDER_LITTLE_ENDIAN__)
#define BIG_ENDIAN defined(__ORDER_BIG_ENDIAN__)

#define NEON defined(__ARM_NEON)
#define FP defined(__ARM_FP)
#define SIMD defined(__ARM_NEON)
#define CRC32 defined(__ARM_FEATURE_CRC32)
#define CRYPTO defined(__ARM_FEATURE_CRYPTO)
#define AES_CT defined(__ARM_FEATURE_AES)
#define SHA2_CT defined(__ARM_FEATURE_SHA2)
#define SHA3_CT defined(__ARM_FEATURE_SHA3)
#define SHA512_CT defined(__ARM_FEATURE_SHA512)
#define SM3_CT defined(__ARM_FEATURE_SM3)
#define SM4_CT defined(__ARM_FEATURE_SM4)
#define FP16_CT defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
#define FP16_VEC_CT defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
#define DOTPROD_CT defined(__ARM_FEATURE_DOTPROD)
#define COMPLEX_CT defined(__ARM_FEATURE_COMPLEX)
#define JSCVT_CT defined(__ARM_FEATURE_JCVT)
#define ATOMICS_CT defined(__ARM_FEATURE_ATOMICS)
#define FMA_CT defined(__ARM_FEATURE_FMA)
#define SVE_CT defined(__ARM_FEATURE_SVE)
#define SVE2_CT defined(__ARM_FEATURE_SVE2)
#define SVE2_AES_CT defined(__ARM_FEATURE_SVE2_AES)
#define SVE2_BITPERM_CT defined(__ARM_FEATURE_SVE2_BITPERM)
#define SVE2_SHA3_CT defined(__ARM_FEATURE_SVE2_SHA3)
#define SVE2_SM4_CT defined(__ARM_FEATURE_SVE2_SM4)
#define SME_CT defined(__ARM_FEATURE_SME)
#define SME2_CT defined(__ARM_FEATURE_SME2)
#define BF16_CT defined(__ARM_FEATURE_BF16)
#define I8MM_CT defined(__ARM_FEATURE_MATMUL_INT8)
#define MEMTAG_CT defined(__ARM_FEATURE_MEMORY_TAGGING)
#define BTI_CT defined(__ARM_FEATURE_BTI_DEFAULT)
#define PAC_CT defined(__ARM_FEATURE_PAC_DEFAULT)
#define RNG_CT defined(__ARM_FEATURE_RNG)
#define LS64_CT defined(__ARM_FEATURE_LS64)
#define MOPS_CT defined(__ARM_FEATURE_MOPS)
#define RCPC_CT defined(__ARM_FEATURE_RCPC)
#define FRINT_CT defined(__ARM_FEATURE_FRINT)
#define FLAGM_CT defined(__ARM_FEATURE_FLAGM)
