//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "paths.hpp"
#include "registry.hpp"
#include "resolve.hpp"

namespace micron
{
namespace elf
{
namespace dl
{

inline constexpr usize max_depth = 32;

struct load_state {
  u32 created[max_modules] = {};
  usize created_n = 0;
  bool failed = false;
};

inline void
__note_created(load_state &st, u32 index) noexcept
{
  if ( st.created_n < max_modules ) st.created[st.created_n++] = index;
}

inline dl_module *__map_recursive(const char *path, rtld flags, load_state &st, usize depth) noexcept;

inline dl_module *
__map_dependency(dl_module *parent, const char *soname, rtld flags, load_state &st, usize depth) noexcept
{

  if ( dl_module *hit = __find_by_soname(soname) ) {
    ++hit->refcount;
    if ( has(flags, rtld::global) ) hit->global = true;
    if ( has(flags, rtld::nodelete) ) hit->nodelete = true;
    return hit;
  }

  if ( const host_module_t *h = host_find(soname) ) {
    if ( __find_by_address(h->base) == nullptr ) return nullptr;
  }

  const path_str_t origin = __dirname_of(parent->mod.path.c_str());
  const path_str_t p = resolve_dependency(soname, parent->mod.dyn.rpath, parent->mod.dyn.runpath, origin.c_str());
  if ( p.empty() ) {
    __err_set_once("dynamic_open: dependency not found", soname);
    st.failed = true;
    return nullptr;
  }

  return __map_recursive(p.c_str(), flags & (rtld::global | rtld::nodelete | rtld::deepbind), st, depth + 1);
}

inline dl_module *
__map_recursive(const char *path, rtld flags, load_state &st, usize depth) noexcept
{
  if ( depth >= max_depth ) {
    __err_set_once("dynamic_open: dependency chain too deep", path);
    st.failed = true;
    return nullptr;
  }

  if ( dl_module *hit = __find_by_path(path) ) {
    ++hit->refcount;
    if ( has(flags, rtld::global) ) hit->global = true;
    if ( has(flags, rtld::nodelete) ) hit->nodelete = true;
    return hit;
  }

  u64 dev = 0, ino = 0;
  if ( !__file_ident(path, dev, ino) ) {
    __err_set_once("dynamic_open: cannot stat", path);
    st.failed = true;
    return nullptr;
  }

  if ( dl_module *hit = __find_by_ident(dev, ino) ) {
    ++hit->refcount;
    if ( has(flags, rtld::global) ) hit->global = true;
    if ( has(flags, rtld::nodelete) ) hit->nodelete = true;
    return hit;
  }

  dl_module *m = __slot_alloc();
  if ( !m ) {
    __err_set_once("dynamic_open: module table full", path);
    st.failed = true;
    return nullptr;
  }
  m->dev = dev;
  m->ino = ino;
  m->refcount = 1;
  m->load_order = __load_seq++;
  m->global = has(flags, rtld::global);
  m->nodelete = has(flags, rtld::nodelete);
  m->deepbind = has(flags, rtld::deepbind);
  __note_created(st, m->index);

  load_opts_t opts{};
  opts.reloc = reloc_mode_t::defer;
  opts.run_init = false;
  opts.apply_relro = false;
  m->mod = __load_module_from_path(path, opts);

  if ( !m->mod.load_base ) {
    __err_set_once("dynamic_open: map failed", path);
    st.failed = true;
    return nullptr;
  }

  const dyn_info_t &d = m->mod.dyn;
  // a module with more DT_NEEDED entries than the table holds still loads
  if ( d.needed_truncated ) __err_set_once("dynamic_open: DT_NEEDED list truncated", path);
  for ( usize i = 0; i < d.needed_count && !st.failed; ++i ) {
    if ( !d.strtab ) break;
    const char *dep_name = d.strtab + d.needed[i];
    dl_module *dep = __map_dependency(m, dep_name, flags, st, depth);
    if ( st.failed ) return nullptr;
    if ( dep && m->dep_count < max_deps ) m->deps[m->dep_count++] = dep->index;
  }
  return st.failed ? nullptr : m;
}

inline bool
__relocate_closure(dl_module *m) noexcept
{
  if ( !m || m->relocated ) return true;
  m->relocated = true;

  for ( u32 k = 0; k < m->dep_count; ++k ) {
    if ( !__relocate_closure(__slots[m->deps[k]]) ) return false;
  }

  if ( !__apply_all_relocs(m->mod, reloc_mode_t::strict, &__resolve_for_module, m) ) {
    __err_set_once("dynamic_open: relocation failed (undefined symbol, or static TLS / TLSDESC / COPY)", m->mod.path.c_str());
    return false;
  }
  __apply_relr(m->mod);
  __apply_relro_now(m->mod);
  return true;
}

inline void
__init_closure(dl_module *m) noexcept
{
  if ( !m || m->initialized ) return;
  m->initialized = true;
  for ( u32 k = 0; k < m->dep_count; ++k ) __init_closure(__slots[m->deps[k]]);
  __run_initializers(m->mod);
}

inline void
__unwind(load_state &st) noexcept
{

  while ( st.created_n-- ) {
    dl_module *m = __slots[st.created[st.created_n]];
    if ( m && m->in_use ) __slot_free(m);
  }
  st.created_n = 0;
}

inline void
__decref_collect(dl_module *m, u32 *doomed, usize &n) noexcept
{
  if ( !m || !m->in_use || m->refcount == 0 ) return;
  if ( --m->refcount > 0 ) return;
  if ( m->nodelete ) return;
  if ( n >= max_modules ) return;
  doomed[n++] = m->index;
  for ( u32 k = 0; k < m->dep_count; ++k ) __decref_collect(__slots[m->deps[k]], doomed, n);
}

inline bool
__close_module(dl_module *m) noexcept
{
  if ( !m || !m->in_use ) return false;

  u32 doomed[max_modules];
  usize n = 0;
  __decref_collect(m, doomed, n);

  for ( usize i = 0; i < n; ++i ) {
    dl_module *d = __slots[doomed[i]];
    if ( d && d->in_use ) __run_finalizers(d->mod);
  }
  for ( usize i = 0; i < n; ++i ) {
    dl_module *d = __slots[doomed[i]];
    if ( d && d->in_use ) __slot_free(d);
  }
  return true;
}

};      // namespace dl
};      // namespace elf
};      // namespace micron
