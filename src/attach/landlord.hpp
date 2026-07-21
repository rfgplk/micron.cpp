//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../bits/__pause.hpp"
#include "../types.hpp"

#include "../atomic/atomic.hpp"
#include "../atomic/flag.hpp"

#include "surplus.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// TLS landlord (HOST side)
//
//
//
// the host reserves a fixed surplus in every TLS frame it builds
// guest micron images (canonically .bmg modules) declare thread_locals under -ftls-model=local-exec;
//
// Frame layouts:
//   amd64/x86 (variant II): [ surplus S | host image_block | TCB ]   tp = base + S + block
//   arm       (variant I):  [ TCB | pad | host image_block | pad | surplus S ]   tp = base

namespace micron
{

inline constexpr u32 __micron_attach_max_modules = 32;

struct __attach_slot {
  atomic_token<u32> in_use{ 0 };
  u64 surplus_off = 0;
  i64 tp_bias = 0;
  const byte *tdata = nullptr;
  u64 filesz = 0;
  u64 memsz = 0;
  u64 align = 0;
};

inline __attach_slot __attach_slots[__micron_attach_max_modules];
inline atomic_flag __attach_lock{};

inline __attribute__((always_inline)) void
__attach_lock_acquire() noexcept
{
  while ( __attach_lock.test_and_set(memory_order::acquire) ) __cpu_pause();
}

inline __attribute__((always_inline)) void
__attach_lock_release() noexcept
{
  __attach_lock.clear(memory_order::release);
}

inline bool
__attach_find_hole(u64 memsz, u64 a, u64 *out_off) noexcept
{
  u64 off = 0;
  for ( u32 guard = 0; guard <= __micron_attach_max_modules; ++guard ) {
    off = __attach_round_up(off, a);
    if ( off > __micron_tls_surplus || memsz > __micron_tls_surplus - off ) return false;
    bool clash = false;
    for ( u32 i = 0; i < __micron_attach_max_modules; ++i ) {
      const __attach_slot &s = __attach_slots[i];
      if ( s.in_use.get(memory_order::acquire) == 0 ) continue;
      const u64 lo = s.surplus_off;
      const u64 hi = lo + s.memsz;
      if ( off < hi && lo < off + memsz ) {
        off = hi;      // skip past the occupant and retry from there
        clash = true;
        break;
      }
    }
    if ( !clash ) {
      *out_off = off;
      return true;
    }
  }
  return false;
}

inline int
__attach_tls_assign(const void *tdata, u64 filesz, u64 memsz, u64 align, u64 *out_token, i64 *out_tp_bias, u64 *out_surplus_off) noexcept
{
  if ( out_token ) *out_token = 0;
  const u64 a = align < 16 ? 16 : align;
  if ( a > __micron_tls_surplus_align ) return -1;
  // WARNING: every seeding copy runs over filesz, but only memsz is bounded by the surplus below
  if ( filesz > memsz ) return -1;
  if ( filesz != 0 && tdata == nullptr ) return -1;
  __attach_lock_acquire();
  if ( !__micron_tls_inited || __micron_tls_surplus == 0 ) {
    __attach_lock_release();
    return -1;
  }
  u64 off = 0;
  if ( !__attach_find_hole(memsz, a, &off) ) {
    __attach_lock_release();
    return -1;
  }
  u32 idx = __micron_attach_max_modules;
  for ( u32 i = 0; i < __micron_attach_max_modules; ++i ) {
    if ( __attach_slots[i].in_use.get(memory_order::acquire) == 0 ) {
      idx = i;
      break;
    }
  }
  if ( idx == __micron_attach_max_modules ) {
    __attach_lock_release();
    return -1;
  }
  __attach_slot &s = __attach_slots[idx];
  s.surplus_off = off;
  s.tp_bias = __attach_tp_bias(off);
  s.tdata = static_cast<const byte *>(tdata);
  s.filesz = filesz;
  s.memsz = memsz;
  s.align = a;
  s.in_use.store(1, memory_order::release);
#if defined(MICRON_ENABLE_ATTACH)
  const u64 base_off = __attach_surplus_base_off();
  __attach_frames_lock();
  for ( u32 i = 0; i < __attach_frame_hi; ++i ) {
    byte *base = __attach_frames[i];
    if ( base == nullptr ) continue;
    byte *dst = base + base_off + off;
    for ( u64 k = 0; k < filesz; ++k ) dst[k] = s.tdata[k];
  }
  __attach_frames_unlock();
#endif
  __attach_lock_release();
  if ( out_token ) *out_token = static_cast<u64>(idx) + 1;
  if ( out_tp_bias ) *out_tp_bias = s.tp_bias;
  if ( out_surplus_off ) *out_surplus_off = off;
  return 0;
}

inline void
__attach_tls_release(u64 token) noexcept
{
  if ( token == 0 || token > __micron_attach_max_modules ) return;
  __attach_lock_acquire();
  __attach_slot &s = __attach_slots[token - 1];
  s.tdata = nullptr;
  s.filesz = 0;
  s.memsz = 0;
  s.in_use.store(0, memory_order::release);
  __attach_lock_release();
}

inline void
__attach_copy_registered_tdata(byte *frame_base) noexcept
{
  if ( __micron_tls_surplus == 0 || frame_base == nullptr ) return;
  byte *surplus_base = frame_base + __attach_surplus_base_off();
  __attach_lock_acquire();
  for ( u32 i = 0; i < __micron_attach_max_modules; ++i ) {
    __attach_slot &s = __attach_slots[i];
    if ( s.in_use.get(memory_order::acquire) == 0 ) continue;
    byte *dst = surplus_base + s.surplus_off;
    for ( u64 k = 0; k < s.filesz; ++k ) dst[k] = s.tdata[k];
  }
  __attach_lock_release();
}

inline byte *
__attach_current_surplus_base() noexcept
{
#if defined(__micron_arch_amd64)
  byte *tp = nullptr;
  asm volatile("mov %%fs:0, %0" : "=r"(tp));
  return tp - static_cast<i64>(__micron_tls_surplus + __micron_host_image_block);
#elif defined(__micron_arch_x86)
  byte *tp = nullptr;
  asm volatile("mov %%gs:0, %0" : "=r"(tp));
  return tp - static_cast<i64>(__micron_tls_surplus + __micron_host_image_block);
#elif defined(__micron_arch_arm32)
  byte *tp = nullptr;
  asm volatile("mrc p15, 0, %0, c13, c0, 3" : "=r"(tp));
  return tp + __attach_surplus_base_off();
#elif defined(__micron_arch_arm64)
  byte *tp = nullptr;
  asm volatile("mrs %0, tpidr_el0" : "=r"(tp));
  return tp + __attach_surplus_base_off();
#else
  return nullptr;
#endif
}

inline int
__attach_tls_seed_current(u64 token) noexcept
{
  if ( token == 0 || token > __micron_attach_max_modules || __micron_tls_surplus == 0 ) return -1;
  __attach_slot &s = __attach_slots[token - 1];
  if ( s.in_use.get(memory_order::acquire) == 0 ) return -1;
  byte *surplus_base = __attach_current_surplus_base();
  if ( surplus_base == nullptr ) return -1;
  byte *dst = surplus_base + s.surplus_off;
  for ( u64 k = 0; k < s.filesz; ++k ) dst[k] = s.tdata[k];
  return 0;
}

};      // namespace micron
