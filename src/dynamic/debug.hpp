//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "registry.hpp"

namespace micron
{
namespace elf
{
namespace dl
{

struct link_map_t {
  naddr_t l_addr = 0;                // load bias, l_addr + a link-time vaddr == the real address
  const char *l_name = nullptr;      // path it was loaded from
  const ndyn_t *l_ld = nullptr;      // its PT_DYNAMIC
  link_map_t *l_next = nullptr;
  link_map_t *l_prev = nullptr;
};

enum : i32 { rt_consistent = 0, rt_add = 1, rt_delete = 2 };

struct r_debug_t {
  i32 r_version = 1;
  link_map_t *r_map = nullptr;
  naddr_t r_brk = 0;      // address of the breakpoint function below
  i32 r_state = rt_consistent;
  naddr_t r_ldbase = 0;
};

extern "C" [[gnu::noinline, gnu::used]] inline void
_micron_dl_debug_state() noexcept
{
  __asm__ __volatile__("" ::: "memory");
}

inline r_debug_t __micron_r_debug{};
inline link_map_t __maps[max_modules] = {};

#if defined(__micron_freestanding)
extern "C" [[gnu::weak]] r_debug_t *_r_debug_micron = &__micron_r_debug;
#endif

inline void
__debug_init_once() noexcept
{
  if ( __micron_r_debug.r_brk ) return;
  __micron_r_debug.r_version = 1;
  __micron_r_debug.r_brk = reinterpret_cast<naddr_t>(&_micron_dl_debug_state);
  __micron_r_debug.r_state = rt_consistent;
}

inline void
__debug_publish(i32 state) noexcept
{
  __debug_init_once();
  __micron_r_debug.r_state = state;
  _micron_dl_debug_state();
}

inline void
__debug_rebuild_chain() noexcept
{
  __debug_init_once();
  link_map_t *head = nullptr;
  link_map_t *prev = nullptr;
  for ( usize i = 0; i < __slot_high; ++i ) {
    dl_module *m = __slots[i];
    if ( !m || !m->in_use || !m->mod.load_base ) continue;
    link_map_t &lm = __maps[i];
    lm.l_addr = reinterpret_cast<naddr_t>(m->mod.load_base);
    lm.l_name = m->mod.path.c_str();
    lm.l_ld = nullptr;
    lm.l_next = nullptr;
    lm.l_prev = prev;
    if ( prev )
      prev->l_next = &lm;
    else
      head = &lm;
    prev = &lm;
  }
  __micron_r_debug.r_map = head;
}

struct dl_phdr_info_t {
  naddr_t dlpi_addr = 0;
  const char *dlpi_name = nullptr;
  const nphdr_t *dlpi_phdr = nullptr;
  half dlpi_phnum = 0;
  u64 dlpi_adds = 0;
  u64 dlpi_subs = 0;
  usize dlpi_tls_modid = 0;
  void *dlpi_tls_data = nullptr;
};

};      // namespace dl
};      // namespace elf

inline int
dl_iterate_phdr(int (*cb)(elf::dl::dl_phdr_info_t *, usize, void *), void *data)
{
  using namespace micron::elf;
  if ( !cb ) return 0;

  micron::lock_guard __lk(dl::__registry_mtx);
  for ( usize i = 0; i < dl::__slot_high; ++i ) {
    dl::dl_module *m = dl::__slots[i];
    if ( !m || !m->in_use || !m->mod.load_base ) continue;
    dl::dl_phdr_info_t info;
    info.dlpi_addr = reinterpret_cast<naddr_t>(m->mod.load_base);
    info.dlpi_name = m->mod.path.c_str();
    info.dlpi_phdr = m->mod.phdrs;
    info.dlpi_phnum = m->mod.phnum;
    info.dlpi_tls_modid = static_cast<usize>(m->mod.tls_modid);
    if ( int r = cb(&info, sizeof(info), data) ) return r;
  }

  init_host_modules();
  for ( usize k = 0; k < host_count(); ++k ) {
    const host_module_t &h = __host_modules[k];
    if ( !h.valid ) continue;
    dl::dl_phdr_info_t info;
    info.dlpi_addr = static_cast<naddr_t>(h.bias);
    info.dlpi_name = h.path.c_str();
    if ( int r = cb(&info, sizeof(info), data) ) return r;
  }
  return 0;
}

};      // namespace micron
