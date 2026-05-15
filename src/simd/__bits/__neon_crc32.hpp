//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if !defined(__micron_arch_arm64) && !defined(__micron_arch_arm32)
#error "__neon_crc32.hpp included on a non-ARM build"
#endif

#if !defined(__ARM_FEATURE_CRC32) && !defined(__micron_arm_crc32)
#else

#include "../../types.hpp"

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

#if defined(__micron_compiler_gcc)
#if defined(__micron_arch_arm32)
__inline_g ::u32
__crc32b(::u32 a, ::u8 b) noexcept
{
  ::u32 r;
  __asm__("crc32b %0, %1, %2" : "=r"(r) : "r"(a), "r"((::u32)b));
  return r;
}

__inline_g ::u32
__crc32h(::u32 a, ::u16 b) noexcept
{
  ::u32 r;
  __asm__("crc32h %0, %1, %2" : "=r"(r) : "r"(a), "r"((::u32)b));
  return r;
}

__inline_g ::u32
__crc32w(::u32 a, ::u32 b) noexcept
{
  ::u32 r;
  __asm__("crc32w %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
  return r;
}

__inline_g ::u32
__crc32d(::u32 a, ::u64 b) noexcept
{
  ::u32 r = __crc32w(a, (::u32)b);
  return __crc32w(r, (::u32)(b >> 32));
}

__inline_g ::u32
__crc32cb(::u32 a, ::u8 b) noexcept
{
  ::u32 r;
  __asm__("crc32cb %0, %1, %2" : "=r"(r) : "r"(a), "r"((::u32)b));
  return r;
}

__inline_g ::u32
__crc32ch(::u32 a, ::u16 b) noexcept
{
  ::u32 r;
  __asm__("crc32ch %0, %1, %2" : "=r"(r) : "r"(a), "r"((::u32)b));
  return r;
}

__inline_g ::u32
__crc32cw(::u32 a, ::u32 b) noexcept
{
  ::u32 r;
  __asm__("crc32cw %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
  return r;
}

__inline_g ::u32
__crc32cd(::u32 a, ::u64 b) noexcept
{
  ::u32 r = __crc32cw(a, (::u32)b);
  return __crc32cw(r, (::u32)(b >> 32));
}
#else       // arm64
__inline_g ::u32
__crc32b(::u32 a, ::u8 b) noexcept
{
  return __builtin_aarch64_crc32b(a, b);
}

__inline_g ::u32
__crc32h(::u32 a, ::u16 b) noexcept
{
  return __builtin_aarch64_crc32h(a, b);
}

__inline_g ::u32
__crc32w(::u32 a, ::u32 b) noexcept
{
  return __builtin_aarch64_crc32w(a, b);
}

__inline_g ::u32
__crc32d(::u32 a, ::u64 b) noexcept
{
  return __builtin_aarch64_crc32x(a, b);
}

__inline_g ::u32
__crc32cb(::u32 a, ::u8 b) noexcept
{
  return __builtin_aarch64_crc32cb(a, b);
}

__inline_g ::u32
__crc32ch(::u32 a, ::u16 b) noexcept
{
  return __builtin_aarch64_crc32ch(a, b);
}

__inline_g ::u32
__crc32cw(::u32 a, ::u32 b) noexcept
{
  return __builtin_aarch64_crc32cw(a, b);
}

__inline_g ::u32
__crc32cd(::u32 a, ::u64 b) noexcept
{
  return __builtin_aarch64_crc32cx(a, b);
}
#endif      // arm32 vs arm64
#elif defined(__micron_compiler_clang)
__inline_g ::u32
__crc32b(::u32 a, ::u8 b) noexcept
{
  return __builtin_arm_crc32b(a, b);
}

__inline_g ::u32
__crc32h(::u32 a, ::u16 b) noexcept
{
  return __builtin_arm_crc32h(a, b);
}

__inline_g ::u32
__crc32w(::u32 a, ::u32 b) noexcept
{
  return __builtin_arm_crc32w(a, b);
}

__inline_g ::u32
__crc32d(::u32 a, ::u64 b) noexcept
{
  return __builtin_arm_crc32d(a, b);
}

__inline_g ::u32
__crc32cb(::u32 a, ::u8 b) noexcept
{
  return __builtin_arm_crc32cb(a, b);
}

__inline_g ::u32
__crc32ch(::u32 a, ::u16 b) noexcept
{
  return __builtin_arm_crc32ch(a, b);
}

__inline_g ::u32
__crc32cw(::u32 a, ::u32 b) noexcept
{
  return __builtin_arm_crc32cw(a, b);
}

__inline_g ::u32
__crc32cd(::u32 a, ::u64 b) noexcept
{
  return __builtin_arm_crc32cd(a, b);
}
#endif

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#endif      // __ARM_FEATURE_CRC32
