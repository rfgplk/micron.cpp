//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"

// WARNING: (!!!!!!) CPP UNALIGNED MAY_ALIAS LOADS/STORES
// these ____MUST____ stay ordinary loads/stores, NEVER asm
// non-volatile asm whose only effect is a "=m" output is DCE eligible and
// CULLED UNDER -Ofast -flto in some always_inline folding contexts

namespace micron
{
namespace __ml
{

__micron_diagnostic_push
__micron_diagnostic_ignored("-Wpsabi")
__micron_diagnostic_ignored("-Wignored-attributes")
typedef u16 __u16u __attribute__((aligned(1), may_alias));
typedef u32 __u32u __attribute__((aligned(1), may_alias));
typedef u64 __u64u __attribute__((aligned(1), may_alias));
#if defined(__micron_simd_generic)
// WARNING: __ld16 RETURNS this type by value, and a compiler generic vector cannot be returned by
// value under -mno-sse ("SSE register return with SSE disabled") or -mgeneral-regs-only -- the two
// flag sets the generic tier exists for. always_inline does not help: GCC rejects the declared ABI
// before it ever considers inlining, and -fsyntax-only does not catch it because the check happens
// at codegen. A two-word POD has the same size and the same load-then-store shape and passes in
// GPRs. See simd/arch/memory_generic.hpp.
struct __v16 {
  u64 w[2];
};
// WARNING: attributes on a typedef of an already-defined struct are IGNORED ("ignoring attributes
// applied to __v16 after definition"), so __v16_u cannot weaken alignment the way the vector
// typedef does. __ld16/__st16 below go through __builtin_memcpy on this tier instead, which is
// alignment-agnostic and aliasing-safe by construction and lowers to the same pair of loads/stores.
typedef __v16 __v16_u;
#else
typedef u64 __v16 __attribute__((vector_size(16), may_alias));
typedef __v16 __v16_u __attribute__((aligned(1)));
#endif
#if defined(__micron_x86_avx2) && !defined(__micron_simd_generic)
typedef u64 __v32 __attribute__((vector_size(32), may_alias));
typedef __v32 __v32_u __attribute__((aligned(1)));
#endif

[[gnu::always_inline]] static inline u16
__ldu16(const byte *p) noexcept
{
  return *reinterpret_cast<const __u16u *>(p);
}

[[gnu::always_inline]] static inline void
__stu16(byte *p, u16 v) noexcept
{
  *reinterpret_cast<__u16u *>(p) = v;
}

[[gnu::always_inline]] static inline u32
__ldu32(const byte *p) noexcept
{
  return *reinterpret_cast<const __u32u *>(p);
}

[[gnu::always_inline]] static inline void
__stu32(byte *p, u32 v) noexcept
{
  *reinterpret_cast<__u32u *>(p) = v;
}

[[gnu::always_inline]] static inline u64
__ldu64(const byte *p) noexcept
{
  return *reinterpret_cast<const __u64u *>(p);
}

[[gnu::always_inline]] static inline void
__stu64(byte *p, u64 v) noexcept
{
  *reinterpret_cast<__u64u *>(p) = v;
}

[[gnu::always_inline]] static inline __v16
__ld16(const byte *p) noexcept
{
#if defined(__micron_simd_generic)
  __v16 v;
  __builtin_memcpy(&v, p, 16);
  return v;
#else
  return *reinterpret_cast<const __v16_u *>(p);
#endif
}

[[gnu::always_inline]] static inline void
__st16(byte *p, __v16 v) noexcept
{
#if defined(__micron_simd_generic)
  __builtin_memcpy(p, &v, 16);
#else
  *reinterpret_cast<__v16_u *>(p) = v;
#endif
}

#if defined(__micron_x86_avx2) && !defined(__micron_simd_generic)
[[gnu::always_inline]] static inline __v32
__ld32(const byte *p) noexcept
{
  return *reinterpret_cast<const __v32_u *>(p);
}

[[gnu::always_inline]] static inline void
__st32(byte *p, __v32 v) noexcept
{
  *reinterpret_cast<__v32_u *>(p) = v;
}
#endif

// scheduling for batched bulk loops
// makes every subsequent store dependent on all four loaded values
[[gnu::always_inline]] static inline void
__pin4(__v16 &a, __v16 &b, __v16 &c, __v16 &e) noexcept
{
#if defined(__micron_simd_generic)
  // no vector register class to pin to; pin the eight machine words instead, which is the same
  // "every store depends on all four loads" dependency the vector form builds
  __asm__("" : "+r"(a.w[0]), "+r"(a.w[1]), "+r"(b.w[0]), "+r"(b.w[1]), "+r"(c.w[0]), "+r"(c.w[1]), "+r"(e.w[0]), "+r"(e.w[1]));
#elif defined(__micron_arch_x86_any)
  __asm__("" : "+x"(a), "+x"(b), "+x"(c), "+x"(e));
#else
  __asm__("" : "+w"(a), "+w"(b), "+w"(c), "+w"(e));
#endif
}

#if defined(__micron_x86_avx2) && !defined(__micron_simd_generic)
[[gnu::always_inline]] static inline void
__pin4(__v32 &a, __v32 &b, __v32 &c, __v32 &e) noexcept
{
  __asm__("" : "+x"(a), "+x"(b), "+x"(c), "+x"(e));
}
#endif

__micron_diagnostic_pop
};      // namespace __ml
};      // namespace micron
