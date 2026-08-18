//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/elf/host_modules.hpp"

#include "registry.hpp"
#include "versioning.hpp"

namespace micron
{
namespace elf
{
namespace dl
{

struct __seen_set {
  u64 w[(max_modules + 63) / 64] = {};

  bool
  test_and_set(u32 i) noexcept
  {
    if ( i >= max_modules ) return true;
    const u64 bit = u64(1) << (i & 63);
    if ( w[i >> 6] & bit ) return true;
    w[i >> 6] |= bit;
    return false;
  }
};

struct found_t {
  void *addr = nullptr;
  dl_module *owner = nullptr;

  explicit
  operator bool() const noexcept
  {
    return addr != nullptr;
  }
};

inline found_t
__lookup_one(dl_module *m, const char *name, const char *want) noexcept
{
  if ( !m || !m->mod.load_base ) return {};
  const nsym_t *s = lookup_versioned(m->mod.dyn, name, want);
  if ( !s || s->shndx == shn_undef ) return {};
  void *a = reinterpret_cast<void *>(m->mod.load_base + s->value);
  if ( elf_st_type(s->info) == stt_gnu_ifunc ) {
    using ifn = void *(*)();
    a = reinterpret_cast<ifn>(a)();
  }
  return found_t{ a, m };
}

inline found_t
__lookup_closure(dl_module *root, const char *name, const char *want) noexcept
{
  if ( !root ) return {};
  __seen_set seen;
  u32 queue[max_modules];
  usize head = 0, tail = 0;

  queue[tail++] = root->index;
  seen.test_and_set(root->index);

  while ( head < tail ) {
    dl_module *m = __slots[queue[head++]];
    if ( !m || !m->in_use ) continue;
    if ( found_t f = __lookup_one(m, name, want) ) return f;
    for ( u32 k = 0; k < m->dep_count && tail < max_modules; ++k ) {
      const u32 di = m->deps[k];
      if ( !seen.test_and_set(di) ) queue[tail++] = di;
    }
  }
  return {};
}

inline found_t
__lookup_global(const char *name, const char *want, u64 skip_upto = 0) noexcept
{
  found_t best;
  u64 best_order = ~u64(0);
  for ( usize i = 0; i < __slot_high; ++i ) {
    dl_module *m = __slots[i];
    if ( !m || !m->in_use || !m->global ) continue;
    if ( m->load_order <= skip_upto ) continue;
    if ( m->load_order >= best_order ) continue;
    if ( found_t f = __lookup_one(m, name, want) ) {
      best = f;
      best_order = m->load_order;
    }
  }
  return best;
}

inline void *
__host_resolve(const char *name) noexcept
{
  void *p = host_resolve_sym(name);
  if ( !p ) return nullptr;
  dl_module *owner = __find_by_address(p);
  if ( owner && !owner->global ) return nullptr;
  return p;
}

inline void *
__resolve_for_module(void *user, const char *name, u32 sym_index) noexcept
{
  dl_module *self = reinterpret_cast<dl_module *>(user);
  if ( !self ) return __host_resolve(name);

  const dyn_info_t &d = self->mod.dyn;
  const char *want = wanted_version(d, sym_index);

  const bool symbolic = self->deepbind || (d.flags & df_symbolic) != 0;
  if ( symbolic ) {
    if ( found_t f = __lookup_one(self, name, want) ) return f.addr;
  }

  if ( found_t f = __lookup_global(name, want) ) return f.addr;
  if ( found_t f = __lookup_closure(self, name, want) ) return f.addr;

  if ( void *h = __host_resolve(name) ) return h;

  if ( want ) {
    if ( found_t f = __lookup_global(name, nullptr) ) return f.addr;
    if ( found_t f = __lookup_closure(self, name, nullptr) ) return f.addr;
  }
  return nullptr;
}

};      // namespace dl
};      // namespace elf
};      // namespace micron
