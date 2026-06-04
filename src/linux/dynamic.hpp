//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "elf/elf.hpp"
#include "elf/host_modules.hpp"

namespace micron
{

class host_dso
{
  const elf::host_module_t *__mod = nullptr;

public:
  host_dso() = default;

  explicit host_dso(const char *soname) : __mod(elf::host_find(soname))
  {
    if ( !__mod ) throw except::library_error("host_dso: soname not present in host");
  }

  bool
  valid() const noexcept
  {
    return __mod != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  // WARNING: the returned pointer (and any sym_as<Fn>) is valid only while THIS module is alive
  // do __NOT__ retain symbol pointers past the dso
  void *
  sym(const char *name) const noexcept
  {
    if ( !__mod ) return nullptr;
    const elf::sym_t *s = elf::lookup_sym(__mod->dyn, name);
    if ( !s || s->shndx == elf::shn_undef ) return nullptr;
    return reinterpret_cast<void *>(__mod->base + s->value);
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
    return __mod ? __mod->soname : nullptr;
  }

  const char *
  path() const noexcept
  {
    return __mod ? __mod->path.c_str() : nullptr;
  }
};

// NOTE: each dso is an independent owning mapping; there is NO dlopen-style refcount/dedup
class dso
{
  elf::handle_t __handle;

public:
  dso() = default;

  explicit dso(const char *soname) : __handle(elf::handle_t::open(soname)) { }

  dso(const char *soname, const char *runpath) : __handle(elf::handle_t::open(soname, runpath)) { }

  static dso
  from_path(const char *path)
  {
    dso d;
    d.__handle = elf::handle_t::open_path(path);
    return d;
  }

  dso(const dso &) = delete;
  dso &operator=(const dso &) = delete;
  dso(dso &&) noexcept = default;
  dso &operator=(dso &&) noexcept = default;
  ~dso() = default;

  bool
  valid() const noexcept
  {
    return __handle.valid();
  }

  explicit
  operator bool() const noexcept
  {
    return __handle.valid();
  }

  void *
  sym(const char *name) const noexcept
  {
    return __handle.sym(name);
  }

  template<class Fn>
  Fn
  sym_as(const char *name) const noexcept
  {
    return __handle.sym_as<Fn>(name);
  }

  const char *
  soname() const noexcept
  {
    return __handle.soname();
  }

  const char *
  path() const noexcept
  {
    return __handle.path();
  }
};

};      // namespace micron
