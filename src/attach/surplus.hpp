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

// feature probe for hosts staging an upgrade
#define MICRON_ATTACH_HOST_TLS_INIT 1

inline constexpr usize __micron_tls_surplus_align = 64;

#if defined(__micron_attach_capable)
#ifndef MICRON_TLS_SURPLUS_SIZE
#define MICRON_TLS_SURPLUS_SIZE 4096
#endif
inline constexpr usize __micron_tls_surplus = MICRON_TLS_SURPLUS_SIZE;
static_assert(__micron_tls_surplus % __micron_tls_surplus_align == 0,
              "MICRON_TLS_SURPLUS_SIZE must be a multiple of micron::__micron_tls_surplus_align (64)");
#else
inline constexpr usize __micron_tls_surplus = 0;
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SURPLUS (WIP)
//
// declared as one of hosts thread_locals, lives in the PT_TLS image
// guests must not declare one; modules are seated in the hosts surplus
//
// used+retain; prevent the compiler/linker from culling this
#if defined(MICRON_ENABLE_ATTACH)
alignas(__micron_tls_surplus_align) inline thread_local byte __micron_attach_surplus[__micron_tls_surplus] __attribute__((used, retain));
#endif

inline i64 __micron_surplus_tpoff = 0;

inline u64 __micron_host_image_block = 0;
inline bool __micron_tls_inited = false;

inline bool
__attach_host_tls_record(i64 surplus_tpoff, u64 image_block) noexcept
{
  if constexpr ( __micron_tls_surplus == 0 ) {
    return false;
  } else {
    if ( __micron_tls_inited ) return true;
    __micron_surplus_tpoff = surplus_tpoff;
    __micron_host_image_block = image_block;
    __micron_tls_inited = true;
    return true;
  }
}

inline __attribute__((always_inline)) bool
__attach_host_tls_ready() noexcept
{
  // whether the landlord can seat guests yet
  return __micron_tls_inited && __micron_tls_surplus != 0;
}

inline __attribute__((always_inline)) constexpr u64
__attach_round_up(u64 v, u64 a) noexcept
{
  return a <= 1 ? v : ((v + a - 1) / a) * a;
}

inline __attribute__((always_inline)) u64
__attach_surplus_base_off() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  return static_cast<u64>(__micron_surplus_tpoff + static_cast<i64>(__micron_host_image_block));
#else
  return static_cast<u64>(__micron_surplus_tpoff);
#endif
}

inline __attribute__((always_inline)) i64
__attach_tp_bias(u64 off) noexcept
{
  return __micron_surplus_tpoff + static_cast<i64>(off);
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
#else
// no attach registry
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
