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

#include "../defs.hpp"

#include "../errno.hpp"

namespace micron
{
namespace except
{

// so we don't rely on io
void
__write_n(const char *str_err)
{
  micron::syscall(SYS_write, 2, micron::voidify(str_err), strlen(str_err));
  micron::syscall(SYS_write, 2, micron::voidify("\n"), 1);
}

void
__write(const char *str_err, size_t sz)
{
  micron::syscall(SYS_write, 2, micron::voidify(str_err), sz);
}

void
__write(const char *str_err)
{
  micron::syscall(SYS_write, 2, micron::voidify(str_err), strlen(str_err));
}

template <typename T>
inline void
__write_unsigned(T n)
{
  char buf[32];
  int i = 0;
  if ( n == 0 ) {
    buf[i++] = '0';
  } else {
    while ( n > 0 ) {
      buf[i++] = '0' + (n % 10);
      n /= 10;
    }
    // reverse
    for ( int j = 0; j < i / 2; ++j ) {
      char tmp = buf[j];
      buf[j] = buf[i - j - 1];
      buf[i - j - 1] = tmp;
    }
  }
  __write(buf, i);
}

template <typename T, typename... Args>
__attribute__((always_inline, noreturn)) inline void
raise(Args &&...args)
{
  T ex(args...);
  __write("exception raised: ");
  __write_n(ex.what());
  __write("errno: ");
  __write_unsigned(errno);
  __write("\n");
  abort(ex.which());
}

template <typename T, typename... Args>
__attribute__((always_inline, noreturn)) inline void
raise_silent(Args &&...args)
{
  T ex(args...);
  abort(ex.which());
}

};     // namespace except

template <typename T, typename... Args>
__attribute__((always_inline, noreturn)) inline void
exc_silent(Args &&...args)
{
  if constexpr ( micron::except::__use_exceptions )
    throw T(micron::forward<Args>(args)...);
  else {
    except::raise_silent<T>(micron::forward<Args>(args)...);
  }
}

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
};     // namespace micron
