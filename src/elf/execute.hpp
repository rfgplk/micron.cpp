//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../linux/elf/elf.hpp"
#include "../type_traits.hpp"

namespace micron
{
namespace elf
{

// execute() — invoke a function symbol resolved from a loaded .so. Multi-
// overload, template form, matching the structure of `execute`/`rexecute` in
// src/linux/process/exec.hpp. Throws library_error if the symbol is missing.

// (1) Primitive form: already-opened handle + cstr name.
template<class R = void, class... Args>
inline R
execute(const handle_t &h, const char *name, Args... args)
{
  using fn_t = R (*)(Args...);
  fn_t fn = h.template sym_as<fn_t>(name);
  if ( !fn ) exc<except::library_error>("micron::elf::execute: symbol not found");
  if constexpr ( micron::is_void_v<R> ) {
    fn(static_cast<Args>(args)...);
    return;
  } else {
    return fn(static_cast<Args>(args)...);
  }
}

// (2) Handle + is_string symbol name.
template<class R = void, is_string S, class... Args>
inline R
execute(const handle_t &h, const S &name, Args... args)
{
  return execute<R>(h, name.c_str(), static_cast<Args>(args)...);
}

// (3) One-shot from absolute path. Opens, invokes, then drops the handle at
//     scope exit (the .so stays mmap'd only for the duration of the call).
template<class R = void, class... Args>
inline R
execute(const char *path, const char *name, Args... args)
{
  handle_t h = handle_t::open_path(path);
  return execute<R>(h, name, static_cast<Args>(args)...);
}

// (4) One-shot with is_string path + is_string symbol name.
template<class R = void, is_string T, is_string S, class... Args>
inline R
execute(const T &path, const S &name, Args... args)
{
  return execute<R>(path.c_str(), name.c_str(), static_cast<Args>(args)...);
}

};      // namespace elf
};      // namespace micron
