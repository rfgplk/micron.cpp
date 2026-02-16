//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../exit.hpp"
#include "../syscall.hpp"

#include "../memory/addr.hpp"

#include "../memory/actions.hpp"
#include "../memory/cstring.hpp"

namespace micron
{
namespace except
{

constexpr static const bool __use_exceptions = true;
// so we don't rely on io
void
__write_n(const char *str_err)
{
  micron::syscall(SYS_write, 2, micron::voidify(str_err), strlen(str_err));
  micron::syscall(SYS_write, 2, micron::voidify("\n"), 1);
}
void
__write(const char *str_err)
{
  micron::syscall(SYS_write, 2, micron::voidify(str_err), strlen(str_err));
}
template <typename T, typename... Args>
__attribute__((always_inline, noreturn)) inline void
raise(Args &&...args)
{
  T ex(args...);
  __write("exception raised: ");
  __write_n(ex.what());
  abort(ex.which());
}

};

template <typename T, typename... Args>
__attribute__((always_inline, noreturn)) inline void
exc(Args &&...args)
{
  if constexpr ( micron::except::__use_exceptions )
    throw T(micron::forward<Args>(args)...);
  else {
    except::raise<T>(micron::forward<Args>(args)...);
  }
}
};
