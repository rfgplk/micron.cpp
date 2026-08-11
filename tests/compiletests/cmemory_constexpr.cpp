//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: the cmemory primitives the constexpr string tier stands on. Not run --
// every assertion here is a static_assert, so failing to compile IS the failing test.
//
// this lives in compiletests rather than rigor for a reason: tests/rigor/memcmp.cpp cannot link on
// a hosted build at all (micron::get_stack() is declared but only defined under
// __micron_freestanding -- src/memory/stack.hpp:31), which is why `memcmp` sits on
// tests/rigor/FAILING.md. static_asserts need no linker, and they sweep every arch x opt x
// freestanding cell of verify_compile.duck instead of just amd64 hosted.

#include "../../src/memory/cmemory/memcmp.hpp"
#include "../../src/memory/cmemory/memcpy.hpp"
#include "../../src/memory/cmemory/memset.hpp"

namespace mc = micron;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmp -- the SIGN, and the same answer folded as executed
//
// until 2026-08-11 the sizeof(T) != 1 arm and the whole of constexpr_memcmp returned
// `reinterpret_cast<const byte*>(&src[i]) - reinterpret_cast<const byte*>(&dest[i])` -- the
// distance between two unrelated objects. meaningless at run time, illegal in a constant
// expression, and invisible to any test that only asks `!= 0`.

constexpr static const char __lo[] = "abc";
constexpr static const char __hi[] = "abd";

static_assert(mc::memcmp<char, char>(__lo, __hi, 3) < 0);
static_assert(mc::memcmp<char, char>(__hi, __lo, 3) > 0);
static_assert(mc::memcmp<char, char>(__lo, __lo, 3) == 0);

static_assert(mc::constexpr_memcmp(__lo, __hi, 3) < 0);
static_assert(mc::constexpr_memcmp(__hi, __lo, 3) > 0);
static_assert(mc::constexpr_memcmp(__lo, __lo, 3) == 0);

// a difference past the window is not a difference
static_assert(mc::memcmp<char, char>(__lo, __hi, 2) == 0);

// narrow elements compare as UNSIGNED bytes. a signed-char comparison answers the other way, which
// would make the consteval path disagree with __memcmp_bytes on 0x80..0xff
constexpr static const char __high[] = { '\x80', '\0' };
constexpr static const char __low[] = { '\x01', '\0' };
static_assert(mc::memcmp<char, char>(__high, __low, 1) > 0);
static_assert(mc::memcmp<char, char>(__low, __high, 1) < 0);

// wide elements order by element value
constexpr static const char32_t __w1[] = U"abc";
constexpr static const char32_t __w2[] = U"abd";
static_assert(mc::memcmp<char32_t, char32_t>(__w1, __w2, 3) < 0);
static_assert(mc::memcmp<char32_t, char32_t>(__w2, __w1, 3) > 0);
static_assert(mc::memcmp<char32_t, char32_t>(__w1, __w1, 3) == 0);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cmemcmp / rcmemcmp -- the compile-time-count siblings, which this gate did NOT cover
//
// they kept the pointer-distance formula in four unrolled copies each long after memcmp,
// rmemcmp and constexpr_memcmp were repointed at __memcmp_scalar, and they were left
// non-constexpr besides -- so none of the assertions below could even be written. every in-tree
// caller passes T = byte and lands on __memcmp_bytes, which is exactly why the sizeof(T) != 1
// arms went unnoticed: the sign they returned was the distance between two unrelated objects.

constexpr static const u32 __c1[4] = { 1, 2, 3, 4 };
constexpr static const u32 __c2[4] = { 1, 2, 3, 5 };
static_assert(mc::cmemcmp<4, u32>(__c1, __c2) < 0);
static_assert(mc::cmemcmp<4, u32>(__c2, __c1) > 0);
static_assert(mc::cmemcmp<4, u32>(__c1, __c1) == 0);
// the answer is a SIGN, not a scaled distance
static_assert(mc::cmemcmp<4, u32>(__c1, __c2) == -1);
static_assert(mc::cmemcmp<4, u32>(__c2, __c1) == 1);

// the non-multiple-of-4 arm is a different loop
constexpr static const u16 __t1[3] = { 7, 7, 7 };
constexpr static const u16 __t2[3] = { 7, 8, 7 };
static_assert(mc::cmemcmp<3, u16>(__t1, __t2) < 0);
static_assert(mc::cmemcmp<3, u16>(__t2, __t1) > 0);
static_assert(mc::cmemcmp<3, u16>(__t1, __t1) == 0);

// a difference past the window is not a difference
static_assert(mc::cmemcmp<3, u32>(__c1, __c2) == 0);

// the sizeof(T) == 1 arm folds too -- the SIMD path sits behind `if !consteval`
static_assert(mc::cmemcmp<3, char>(__lo, __hi) < 0);
static_assert(mc::cmemcmp<3, char>(__hi, __lo) > 0);
static_assert(mc::cmemcmp<3, char>(__lo, __lo) == 0);
static_assert(mc::cmemcmp<1, char>(__high, __low) > 0);      // unsigned, as memcmp is

// wide elements, same ordering rule as memcmp
static_assert(mc::cmemcmp<3, char32_t>(__w1, __w2) < 0);
static_assert(mc::cmemcmp<3, char32_t>(__w2, __w1) > 0);

// rcmemcmp reaches the same core through a reference
static_assert(mc::rcmemcmp<4, u32>(__c1, __c2) < 0);
static_assert(mc::rcmemcmp<4, u32>(__c2, __c1) > 0);
static_assert(mc::rcmemcmp<4, u32>(__c1, __c1) == 0);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset family -- czero is what constexpr ~sstring() runs

consteval bool
__czero_zeroes(void)
{
  char b[8] = { 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x' };
  mc::czero<8>(b);
  for ( usize i = 0; i < 8; ++i )
    if ( b[i] != '\0' ) return false;
  return true;
}
static_assert(__czero_zeroes());

consteval bool
__cbyteset_narrow(void)
{
  unsigned char b[4] = {};
  mc::cbyteset<4>(b, 0xAB);
  return b[0] == 0xAB && b[1] == 0xAB && b[2] == 0xAB && b[3] == 0xAB;
}
static_assert(__cbyteset_narrow());

// a wide element must carry the fill in EVERY byte, the way __memset_bytes leaves it
consteval bool
__cbyteset_wide(void)
{
  u32 b[2] = {};
  mc::cbyteset<8>(b, 0xAB);
  return b[0] == 0xABABABABu && b[1] == 0xABABABABu;
}
static_assert(__cbyteset_wide());

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy family

consteval bool
__memcpy_copies(void)
{
  char d[4] = {};
  const char s[4] = { 'w', 'x', 'y', 'z' };
  mc::memcpy(d, s, 4);
  return d[0] == 'w' && d[1] == 'x' && d[2] == 'y' && d[3] == 'z';
}
static_assert(__memcpy_copies());

// constexpr_bytecpy could never be constant-evaluated before 2026-08-11 -- it opened with an
// unconditional reinterpret_cast<byte*>, so the one thing its name promised was the one thing it
// could not do
consteval bool
__constexpr_bytecpy_copies(void)
{
  char d[4] = {};
  const char s[4] = { 'w', 'x', 'y', 'z' };
  mc::constexpr_bytecpy(d, s, 4);
  return d[0] == 'w' && d[3] == 'z';
}
static_assert(__constexpr_bytecpy_copies());

int
main()
{
  return 1;
}
