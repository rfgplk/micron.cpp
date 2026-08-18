//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../type_traits.hpp"

#include "debug.hpp"
#include "load.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// dynamic loading call fns

namespace micron
{

struct dynamic_t {
  u32 index = 0;
  u32 generation = 0;

  explicit constexpr
  operator bool() const noexcept
  {
    return generation != 0;
  }

  constexpr bool
  operator==(const dynamic_t &o) const noexcept
  {
    return index == o.index && generation == o.generation;
  }
};

inline constexpr dynamic_t dynamic_default{ 0xffffffffu, 0xffffffffu };
inline constexpr dynamic_t dynamic_next{ 0xfffffffeu, 0xffffffffu };

constexpr bool
dynamic_lazy_supported() noexcept
{
  return false;
}

inline dynamic_t
dynamic_open(const char *soname_or_path, rtld flags = rtld::now | rtld::local)
{
  using namespace micron::elf::dl;
  __err_clear();

  if ( !soname_or_path || !*soname_or_path ) exc<except::library_error>("dynamic_open: empty name");

  micron::lock_guard __lk(__registry_mtx);

  if ( dl_module *hit = __find_by_path(soname_or_path) ) {
    ++hit->refcount;
    if ( has(flags, rtld::global) ) hit->global = true;
    if ( has(flags, rtld::nodelete) ) hit->nodelete = true;
    return dynamic_t{ hit->index, hit->generation };
  }

  const elf::path_str_t p = resolve_dependency(soname_or_path, nullptr, nullptr, nullptr);
  if ( p.empty() ) {
    __err_set_once("dynamic_open: not found", soname_or_path);
    if ( has(flags, rtld::noload) ) return dynamic_t{};
    exc<except::library_error>("dynamic_open: library not found in any search path");
  }

  if ( has(flags, rtld::noload) ) {
    u64 dev = 0, ino = 0;
    if ( !__file_ident(p.c_str(), dev, ino) ) return dynamic_t{};
    dl_module *hit = __find_by_ident(dev, ino);
    if ( !hit ) return dynamic_t{};
    ++hit->refcount;
    return dynamic_t{ hit->index, hit->generation };
  }

  load_state st;
  dl_module *root = __map_recursive(p.c_str(), flags, st, 0);
  if ( !root || st.failed ) {
    __unwind(st);
    exc<except::library_error>("dynamic_open: failed to map the dependency closure");
  }

  if ( !__relocate_closure(root) ) {
    __unwind(st);
    exc<except::library_error>("dynamic_open: relocation failed");
  }

  elf::dl::__debug_publish(elf::dl::rt_add);
  elf::dl::__debug_rebuild_chain();
  elf::dl::__debug_publish(elf::dl::rt_consistent);

  __init_closure(root);

  return dynamic_t{ root->index, root->generation };
}

template<is_string T>
inline dynamic_t
dynamic_open(const T &name, rtld flags = rtld::now | rtld::local)
{
  return dynamic_open(name.c_str(), flags);
}

inline bool
dynamic_close(dynamic_t d) noexcept
{
  using namespace micron::elf::dl;
  __err_clear();
  if ( d.generation == 0xffffffffu ) return false;

  micron::lock_guard __lk(__registry_mtx);
  dl_module *m = __slot_at(d.index, d.generation);
  if ( !m ) {
    __err_set_once("dynamic_close: stale or invalid handle");
    return false;
  }
  const bool ok = __close_module(m);

  __debug_publish(rt_delete);
  __debug_rebuild_chain();
  __debug_publish(rt_consistent);
  return ok;
}

inline void *
dynamic_sym(dynamic_t d, const char *name) noexcept
{
  using namespace micron::elf::dl;
  __err_clear();
  if ( !name || !*name ) return nullptr;

  micron::lock_guard __lk(__registry_mtx);

  if ( d == dynamic_default ) {
    if ( found_t f = __lookup_global(name, nullptr) ) return f.addr;
    return __host_resolve(name);
  }

  if ( d == dynamic_next ) {

    dl_module *caller = __find_by_address(__builtin_return_address(0));
    if ( !caller ) {
      __err_set_once("dynamic_sym: RTLD_NEXT called from outside any loaded module");
      return nullptr;
    }
    if ( found_t f = __lookup_global(name, nullptr, caller->load_order) ) return f.addr;
    __err_set_once("dynamic_sym: no next definition", name);
    return nullptr;
  }

  dl_module *m = __slot_at(d.index, d.generation);
  if ( !m ) {
    __err_set_once("dynamic_sym: stale or invalid handle");
    return nullptr;
  }

  if ( found_t f = __lookup_closure(m, name, nullptr) ) return f.addr;
  __err_set_once("dynamic_sym: symbol not found", name);
  return nullptr;
}

template<is_string S>
inline void *
dynamic_sym(dynamic_t d, const S &name) noexcept
{
  return dynamic_sym(d, name.c_str());
}

template<class Fn>
inline Fn
dynamic_sym_as(dynamic_t d, const char *name) noexcept
{
  return reinterpret_cast<Fn>(dynamic_sym(d, name));
}

inline dynamic_t
dynamic_owner(const void *addr) noexcept
{
  using namespace micron::elf::dl;
  micron::lock_guard __lk(__registry_mtx);
  dl_module *m = __find_by_address(addr);
  return m ? dynamic_t{ m->index, m->generation } : dynamic_t{};
}

inline const char *
dynamic_path(dynamic_t d) noexcept
{
  using namespace micron::elf::dl;
  micron::lock_guard __lk(__registry_mtx);
  dl_module *m = __slot_at(d.index, d.generation);
  return m ? m->mod.path.c_str() : nullptr;
}

inline const char *
dynamic_soname(dynamic_t d) noexcept
{
  using namespace micron::elf::dl;
  micron::lock_guard __lk(__registry_mtx);
  dl_module *m = __slot_at(d.index, d.generation);
  return m ? m->mod.dyn.soname : nullptr;
}

inline u32
dynamic_refcount(dynamic_t d) noexcept
{
  using namespace micron::elf::dl;
  micron::lock_guard __lk(__registry_mtx);
  dl_module *m = __slot_at(d.index, d.generation);
  return m ? m->refcount : 0;
}

template<class R = void, class... Args>
inline R
dynamic_call(dynamic_t d, const char *name, Args... args)
{
  using fn_t = R (*)(Args...);
  fn_t fn = dynamic_sym_as<fn_t>(d, name);
  if ( !fn ) exc<except::library_error>("dynamic_call: symbol not found");
  if constexpr ( micron::is_void_v<R> ) {
    fn(static_cast<Args>(args)...);
    return;
  } else {
    return fn(static_cast<Args>(args)...);
  }
}

template<class R = void, is_string S, class... Args>
inline R
dynamic_call(dynamic_t d, const S &name, Args... args)
{
  return dynamic_call<R>(d, name.c_str(), static_cast<Args>(args)...);
}

template<class R = void, class... Args>
inline R
dynamic_call(const char *soname_or_path, const char *name, Args... args)
{
  dynamic_t d = dynamic_open(soname_or_path);
  return dynamic_call<R>(d, name, static_cast<Args>(args)...);
}

template<class R = void, is_string T, is_string S, class... Args>
inline R
dynamic_call(const T &soname_or_path, const S &name, Args... args)
{
  return dynamic_call<R>(soname_or_path.c_str(), name.c_str(), static_cast<Args>(args)...);
}

};      // namespace micron
