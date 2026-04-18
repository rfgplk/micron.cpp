//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../attributes.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../simd/intrin.hpp"     // arch-dispatched
#include "../simd/memory.hpp"     // arch-dispatched

// TO ALL READERS
// this set of functions is ever so slightly different from the standard string.h
// set of memory manip. functions
// generally if you'd like to set memory you should use
// bset     = set bytes, in bytes
// memset   = set memory, in bytes or type provided (ie size of the type you provided)
// cmemset  = set memory with known length at compile time
// wordset  = set word, in words
// typeset  = set memory, as if it were provided type
// functions suffixed with a number represent the width or alignment of the function
// functions prefixed with a 'c' all take in certain parameters at compile time

#include "cmemory/memchr.hpp"
#include "cmemory/memcmp.hpp"
#include "cmemory/memcpy.hpp"
#include "cmemory/memmove.hpp"
#include "cmemory/memset.hpp"
#include "stack.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 128-bit SIMD memory primitives

#if defined(__micron_arch_x86_any) || (defined(__micron_arch_arm_any) && defined(__micron_arm_neon))

// memcpy
using simd::amemcpy128;
using simd::memcpy128;
using simd::ntmemcpy128;
using simd::rmemcpy128;

// memmove
using simd::amemmove128;
using simd::memmove128;

// memset
using simd::amemset128;
using simd::memset128;
using simd::ntmemset128;

// memcmp
using simd::amemcmp128;
using simd::memcmp128;

#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 256-bit + 512-bit SIMD memory primitives x86 only

#if defined(__micron_arch_x86_any)

// memcpy
using simd::amemcpy256;
using simd::amemcpy512;
using simd::memcpy256;
using simd::memcpy512;
using simd::ntmemcpy256;
using simd::ntmemcpy512;
using simd::rmemcpy256;
using simd::rmemcpy512;

// memmove
using simd::amemmove256;
using simd::amemmove512;
using simd::memmove256;
using simd::memmove512;

// memset
using simd::amemset256;
using simd::amemset512;
using simd::memset256;
using simd::memset512;
using simd::ntmemset256;
using simd::ntmemset512;

// memcmp
using simd::amemcmp256;
using simd::amemcmp512;
using simd::memcmp256;
using simd::memcmp512;

#endif

};     // namespace micron
