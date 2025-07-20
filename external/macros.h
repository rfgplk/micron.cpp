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

#define OPENMO defined(_OPENMP)
#define OPTIMIZING defined(__OPTIMIZE__)
#define ALIGNMENT defined(__BIGGEST_ALIGNMENT__)
//#define LITTLE_ENDIAN defined(__ORDER_LITTLE_ENDIAN__)
//#define BIG_ENDIAN defined(__ORDER_BIG_ENDIAN__)

#define AMD_3DNow defined(__3dNOW__)
#define AMD64 defined(__amd64__)
#define x64 defined(__x86_64__)
#define ADX defined(__ADX__)
#define FMA defined(__FMA__)
#define MMX defined(__MMX__)
#define RANDOM_SEED defined(__RDSEED__)
#define PCLMUL defined(__PCLMUL__)
#define POPCNT defined(__POPCNT__)
#define AES defined(__AES__)
#define F16C defined(__F16C__)
#define BMI defined(__BMI__)
#define BMI2 defined(__BMI2__)
#define SHA defined(__SHA__)
#define SSE defined(__SSE__)
#define SSE_MATH defined(__SSE_MATH__)
#define SSE2 defined(__SSE2__)
#define SSE2_MATH defined(__SSE2_MATH__)
#define SSE3 defined(__SSE3__)
#define SSSE3 defined(__SSSE3__)
#define SSE4_1 defined(__SSE4_1__)
#define SSE4_2 defined(__SSE4_2__)
#define AVX defined(__AVX__)
#define AVX2 defined(__AVX2__)
#define AVX512F defined(__AVX512F__)
#define AVX512BW defined(__AVX512BW__)
#define AVX512CD defined(__AVX512CD__)
#define AVX512VL defined(__AVX512VL__)
