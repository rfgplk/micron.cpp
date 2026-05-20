//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../vector.hpp"

#include "bits.hpp"
#include "consts.hpp"
#include "hash.hpp"
#include "header.hpp"
#include "loader.hpp"

namespace micron
{
namespace elf
{

class handle_t
{
  module_t __mod;
  micron::vector<symbol_info_t> __syms;
  bool __ilist = false;

  void
  __build_symbols()
  {
    const dyn_info_t &d = __mod.dyn;
    if ( !d.symtab || !d.strtab || d.symcount == 0 ) return;
    __syms.reserve(d.symcount);
    for ( xword i = 0; i < d.symcount; ++i ) {
      const sym_t &s = d.symtab[i];
      symbol_info_t row;
      row.name = d.strtab + s.name;
      row.size = s.size;
      row.type = elf_st_type(s.info);
      row.binding = elf_st_bind(s.info);
      row.defined = (s.shndx != shn_undef);
      row.address = row.defined ? reinterpret_cast<void *>(__mod.load_base + s.value) : nullptr;
      __syms.push_back(row);
    }
  }

  void
  __link_self() noexcept
  {
    __mod.next = __loaded_modules;
    __loaded_modules = &__mod;
    __ilist = true;
  }

  void
  __unlink_self() noexcept
  {
    if ( !__ilist ) return;
    if ( __loaded_modules == &__mod ) {
      __loaded_modules = __mod.next;
    } else {
      for ( module_t *p = __loaded_modules; p && p->next; p = p->next ) {
        if ( p->next == &__mod ) {
          p->next = __mod.next;
          break;
        }
      }
    }
    __mod.next = nullptr;
    __ilist = false;
  }

  static void
  __relink_from(module_t *oldm, module_t *newm) noexcept
  {
    if ( __loaded_modules == oldm ) {
      __loaded_modules = newm;
      return;
    }
    for ( module_t *p = __loaded_modules; p; p = p->next ) {
      if ( p->next == oldm ) {
        p->next = newm;
        return;
      }
    }
  }

public:
  ~handle_t() { __unlink_self(); }

  handle_t() = default;

  handle_t(const handle_t &) = delete;
  handle_t &operator=(const handle_t &) = delete;

  handle_t(handle_t &&o) noexcept
      : __mod(static_cast<module_t &&>(o.__mod)), __syms(static_cast<micron::vector<symbol_info_t> &&>(o.__syms)), __ilist(o.__ilist)
  {
    o.__ilist = false;
    if ( __ilist ) {
      __relink_from(&o.__mod, &__mod);
    }
  }

  handle_t &
  operator=(handle_t &&o) noexcept
  {
    if ( this != &o ) {
      __unlink_self();
      __mod = static_cast<module_t &&>(o.__mod);
      __syms = static_cast<micron::vector<symbol_info_t> &&>(o.__syms);
      __ilist = o.__ilist;
      o.__ilist = false;
      if ( __ilist ) __relink_from(&o.__mod, &__mod);
    }
    return *this;
  }

  static handle_t
  open(const char *soname, const char *runpath = nullptr)
  {
    handle_t h;
    h.__mod = load(soname, runpath);
    h.__link_self();
    h.__build_symbols();
    return h;
  }

  static handle_t
  open_path(const char *path)
  {
    handle_t h;
    h.__mod = __load_module_from_path(path);
    h.__link_self();
    h.__build_symbols();
    return h;
  }

  bool
  valid() const noexcept
  {
    return __mod.load_base != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  void *
  sym(const char *name) const noexcept
  {
    if ( !valid() ) return nullptr;
    const sym_t *s = lookup_sym(__mod.dyn, name);
    if ( !s || s->shndx == shn_undef ) return nullptr;
    return reinterpret_cast<void *>(__mod.load_base + s->value);
  }

  template<class Fn>
  Fn
  sym_as(const char *name) const noexcept
  {
    return reinterpret_cast<Fn>(sym(name));
  }

  const char *
  soname() const noexcept
  {
    return __mod.dyn.soname;
  }

  const char *
  path() const noexcept
  {
    return __mod.path.c_str();
  }

  u8 *
  base() const noexcept
  {
    return __mod.load_base;
  }

  const micron::vector<symbol_info_t> &
  symbols() const noexcept
  {
    return __syms;
  }

  const dyn_info_t &
  dyn() const noexcept
  {
    return __mod.dyn;
  }
};

};      // namespace elf
};      // namespace micron
