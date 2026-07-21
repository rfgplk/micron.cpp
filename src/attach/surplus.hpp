//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../bits/__pause.hpp"
#include "../types.hpp"

namespace micron
{

inline constexpr usize __micron_tls_surplus_align = 64;

#if defined(__micron_attach_capable)
#ifndef MICRON_TLS_SURPLUS_SIZE
#define MICRON_TLS_SURPLUS_SIZE 4096
#endif
inline constexpr usize __micron_tls_surplus = MICRON_TLS_SURPLUS_SIZE;
// WARNING: on variant II the host PT_TLS image is placed at base + surplus, so the surplus size is
// part of the host's own thread_local alignment; values that aren't a multiple of the surplus align
// silently misaligns every host thread_local
static_assert(__micron_tls_surplus % __micron_tls_surplus_align == 0,
              "MICRON_TLS_SURPLUS_SIZE must be a multiple of micron::__micron_tls_surplus_align (64)");
#else
inline constexpr usize __micron_tls_surplus = 0;
#endif

inline u64 __micron_host_image_block = 0;
inline u64 __micron_host_head_aligned = 0;
inline bool __micron_tls_inited = false;

inline __attribute__((always_inline)) bool
__attach_surplus_fits_align([[maybe_unused]] u64 p_align) noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  if constexpr ( __micron_tls_surplus == 0 ) {
    return true;
  } else {
    return p_align == 0 || (__micron_tls_surplus % p_align) == 0;
  }
#else
  return true;
#endif
}

inline __attribute__((always_inline)) constexpr u64
__attach_round_up(u64 v, u64 a) noexcept
{
  return a <= 1 ? v : ((v + a - 1) / a) * a;
}

// byte offset from a frame's mmap base to the start of its surplus region
inline __attribute__((always_inline)) u64
__attach_surplus_base_off() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  return 0;
#else
  return __attach_round_up(__micron_host_head_aligned + __micron_host_image_block, __micron_tls_surplus_align);
#endif
}

inline __attribute__((always_inline)) i64
__attach_tp_bias(u64 off) noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  return static_cast<i64>(off) - static_cast<i64>(__micron_tls_surplus + __micron_host_image_block);
#else
  return static_cast<i64>(__attach_surplus_base_off() + off);
#endif
}

#if defined(MICRON_ENABLE_ATTACH)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// live TLS frame registry
inline constexpr u32 __micron_attach_max_frames = 4096;
inline byte *__attach_frames[__micron_attach_max_frames] = {};
inline u32 __attach_frame_hi = 0;
inline unsigned char __attach_frame_lock = 0;

inline __attribute__((always_inline)) void
__attach_frames_lock() noexcept
{
  while ( __atomic_exchange_n(&__attach_frame_lock, static_cast<unsigned char>(1), __ATOMIC_ACQUIRE) ) __cpu_pause();
}

inline __attribute__((always_inline)) void
__attach_frames_unlock() noexcept
{
  __atomic_store_n(&__attach_frame_lock, static_cast<unsigned char>(0), __ATOMIC_RELEASE);
}

// NOTE: registration must happen before the frame is seeded, never after
inline void
__attach_frame_register(byte *base) noexcept
{
  if ( base == nullptr ) return;
  __attach_frames_lock();
  for ( u32 i = 0; i < __micron_attach_max_frames; ++i ) {
    if ( __attach_frames[i] == nullptr ) {
      __attach_frames[i] = base;
      if ( i >= __attach_frame_hi ) __attach_frame_hi = i + 1;
      break;
    }
  }
  // NOTE: a full registry is not fatal
  __attach_frames_unlock();
}

inline void
__attach_frame_unregister(byte *base) noexcept
{
  if ( base == nullptr ) return;
  __attach_frames_lock();
  for ( u32 i = 0; i < __attach_frame_hi; ++i ) {
    if ( __attach_frames[i] == base ) {
      __attach_frames[i] = nullptr;
      break;
    }
  }
  __attach_frames_unlock();
}
#elif defined(__micron_attach_capable)
inline __attribute__((always_inline)) void
__attach_frame_register(byte *) noexcept
{
}

inline __attribute__((always_inline)) void
__attach_frame_unregister(byte *) noexcept
{
}
#endif

};      // namespace micron
