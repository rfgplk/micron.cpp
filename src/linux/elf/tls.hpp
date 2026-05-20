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
inline u64 __tls_next_modid = 1;

struct tls_thread_block_t {
  i32 tid;
  void *blocks[tls_max_modules] = {};
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
  for ( tls_thread_block_t *t = __tls_thread_head; t; t = t->next ) {
    if ( t->tid == me ) return t;
  }
  tls_thread_block_t *t = reinterpret_cast<tls_thread_block_t *>(
      micron::mmap(nullptr, sizeof(tls_thread_block_t), prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(t) ) return nullptr;
  t->tid = me;
  for ( usize i = 0; i < tls_max_modules; ++i ) t->blocks[i] = nullptr;
  t->next = __tls_thread_head;
  __tls_thread_head = t;
  return t;
}

inline u64
tls_register(const u8 *image, usize image_size, usize total_size, usize align) noexcept
{
  if ( __tls_next_modid >= tls_max_modules ) return 0;
  const u64 mid = __tls_next_modid++;
  tls_module_t &m = __tls_modules[mid];
  m.modid = mid;
  m.image = image;
  m.image_size = image_size;
  m.total_size = total_size;
  m.align = align ? align : 8;
  m.active = true;
  return mid;
}

inline void
tls_unregister(u64 modid) noexcept
{
  if ( modid == 0 || modid >= tls_max_modules ) return;
  __tls_modules[modid].active = false;
}

inline void *
tls_alloc_block_for(const tls_module_t &m) noexcept
{
  const usize sz = (m.total_size + m.align - 1) & ~(m.align - 1);
  u8 *p = reinterpret_cast<u8 *>(micron::mmap(nullptr, sz ? sz : 16, prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(p) ) return nullptr;
  if ( m.image_size ) micron::memcpy(p, m.image, m.image_size);
  if ( m.total_size > m.image_size ) {
    micron::memset(p + m.image_size, byte{ 0 }, m.total_size - m.image_size);
  }
  return p;
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
  const tls_module_t &m = __tls_modules[ti->ti_module];
  if ( !m.active ) return nullptr;
  tls_thread_block_t *t = __tls_get_thread();
  if ( !t ) return nullptr;
  if ( !t->blocks[ti->ti_module] ) {
    void *b = tls_alloc_block_for(m);
    if ( !b ) return nullptr;
    t->blocks[ti->ti_module] = b;
  }
  return reinterpret_cast<u8 *>(t->blocks[ti->ti_module]) + ti->ti_offset;
}
