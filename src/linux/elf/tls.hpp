//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/cmemory.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/mmap_bits.hpp"
#include "../../syscall.hpp"
#include "../sys/types.hpp"
#include "bits.hpp"

namespace micron
{
namespace elf
{

constexpr inline usize tls_max_modules = 64;

struct tls_module_t {
  u64 modid;
  const u8 *image;       // pointer to PT_TLS file image inside the module's mapping
  usize image_size;      // p_filesz of PT_TLS
  usize total_size;      // p_memsz of PT_TLS (image_size + bss)
  usize align;           // p_align of PT_TLS
  bool active;
};

inline tls_module_t __tls_modules[tls_max_modules] = {};

inline int __tls_lock_flag = 0;

inline void
__tls_lock() noexcept
{
  while ( __atomic_exchange_n(&__tls_lock_flag, 1, __ATOMIC_ACQUIRE) ) {
  }
}

inline void
__tls_unlock() noexcept
{
  __atomic_store_n(&__tls_lock_flag, 0, __ATOMIC_RELEASE);
}

struct tls_block_t {
  void *ptr = nullptr;      // aligned block start (__tls_get_addr returns ptr + ti_offset)
  void *map = nullptr;      // mmap base (for munmap on unregister)
  usize map_sz = 0;         // mmap length
};

// NOTE: per-thread TLS blocks are keyed on gettid() and NOT reclaimed on thread exit (no thread-exit hook)
// they leak for the process lifetime
struct tls_thread_block_t {
  i32 tid;
  tls_block_t blocks[tls_max_modules] = {};
  tls_thread_block_t *next;
};

inline tls_thread_block_t *__tls_thread_head = nullptr;

inline i32
__current_tid() noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

inline tls_thread_block_t *
__tls_get_thread() noexcept
{
  const i32 me = __current_tid();
  __tls_lock();
  for ( tls_thread_block_t *t = __tls_thread_head; t; t = t->next ) {
    if ( t->tid == me ) {
      __tls_unlock();
      return t;
    }
  }
  __tls_unlock();
  tls_thread_block_t *t = reinterpret_cast<tls_thread_block_t *>(
      micron::mmap(nullptr, sizeof(tls_thread_block_t), prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(t) ) return nullptr;
  t->tid = me;
  for ( usize i = 0; i < tls_max_modules; ++i ) t->blocks[i] = tls_block_t{};
  __tls_lock();
  t->next = __tls_thread_head;
  __tls_thread_head = t;
  __tls_unlock();
  return t;
}

inline u64
tls_register(const u8 *image, usize image_size, usize total_size, usize align) noexcept
{
  __tls_lock();
  u64 mid = 0;
  for ( u64 i = 1; i < tls_max_modules; ++i ) {      // reuse the first inactive slot (modid free-list)
    if ( !__tls_modules[i].active ) {
      mid = i;
      break;
    }
  }
  if ( mid == 0 ) {
    __tls_unlock();
    return 0;
  }
  tls_module_t &m = __tls_modules[mid];
  m.modid = mid;
  m.image = image;
  m.image_size = image_size;
  m.total_size = total_size;
  m.align = align ? align : 8;
  m.active = true;
  __tls_unlock();
  return mid;
}

inline void
tls_unregister(u64 modid) noexcept
{
  if ( modid == 0 || modid >= tls_max_modules ) return;
  __tls_lock();
  // free every thread's block for this module; important if we cycle and not just allow the kernel to reclaim at eop
  for ( tls_thread_block_t *t = __tls_thread_head; t; t = t->next ) {
    if ( t->blocks[modid].map ) {
      micron::munmap(reinterpret_cast<addr_t *>(t->blocks[modid].map), t->blocks[modid].map_sz);
      t->blocks[modid] = tls_block_t{};
    }
  }
  __tls_modules[modid] = tls_module_t{};
  __tls_unlock();
}

inline tls_block_t
tls_alloc_block_for(const tls_module_t &m) noexcept
{
  const usize align = m.align ? m.align : 8;
  usize sz = (m.total_size + align - 1) & ~(align - 1);
  if ( sz == 0 ) sz = 16;
  const usize extra = (align > __micron_page_size_default) ? align : 0;
  const usize map_sz = sz + extra;
  u8 *base = reinterpret_cast<u8 *>(micron::mmap(nullptr, map_sz, prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(base) ) return tls_block_t{};
  u8 *aligned = extra ? reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(base) + align - 1) & ~static_cast<uintptr_t>(align - 1)) : base;
  if ( m.image_size ) micron::memcpy(aligned, m.image, m.image_size);
  if ( m.total_size > m.image_size ) micron::memset(aligned + m.image_size, byte{ 0 }, m.total_size - m.image_size);
  return tls_block_t{ aligned, base, map_sz };
}

struct tls_index_t {
  unsigned long ti_module;
  unsigned long ti_offset;
};

};      // namespace elf
};      // namespace micron

// must be ext C
extern "C" inline void *
__tls_get_addr(micron::elf::tls_index_t *ti) noexcept
{
  using namespace micron::elf;
  if ( !ti || ti->ti_module == 0 || ti->ti_module >= tls_max_modules ) return nullptr;
  tls_thread_block_t *t = __tls_get_thread();
  if ( !t ) return nullptr;
  __tls_lock();
  const tls_module_t &m = __tls_modules[ti->ti_module];
  if ( !m.active ) {
    __tls_unlock();
    return nullptr;
  }
  if ( !t->blocks[ti->ti_module].ptr ) {
    tls_block_t b = tls_alloc_block_for(m);
    if ( !b.ptr ) {
      __tls_unlock();
      return nullptr;
    }
    t->blocks[ti->ti_module] = b;
  }
  void *p = reinterpret_cast<u8 *>(t->blocks[ti->ti_module].ptr) + ti->ti_offset;
  __tls_unlock();
  return p;
}
