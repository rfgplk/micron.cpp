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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wignored-attributes"

typedef u16 __u16u __attribute__((aligned(1), may_alias));
typedef u32 __u32u __attribute__((aligned(1), may_alias));
typedef u64 __u64u __attribute__((aligned(1), may_alias));
typedef u64 __v16 __attribute__((vector_size(16), may_alias));
typedef __v16 __v16_u __attribute__((aligned(1)));
#if defined(__micron_x86_avx2)
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
  return *reinterpret_cast<const __v16_u *>(p);
}

[[gnu::always_inline]] static inline void
__st16(byte *p, __v16 v) noexcept
{
  *reinterpret_cast<__v16_u *>(p) = v;
}

#if defined(__micron_x86_avx2)
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
#if defined(__micron_arch_x86_any)
  __asm__("" : "+x"(a), "+x"(b), "+x"(c), "+x"(e));
#else
  __asm__("" : "+w"(a), "+w"(b), "+w"(c), "+w"(e));
#endif
}

#if defined(__micron_x86_avx2)
[[gnu::always_inline]] static inline void
__pin4(__v32 &a, __v32 &b, __v32 &c, __v32 &e) noexcept
{
  __asm__("" : "+x"(a), "+x"(b), "+x"(c), "+x"(e));
}
#endif

#pragma GCC diagnostic pop

};      // namespace __ml
};      // namespace micron
