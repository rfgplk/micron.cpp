// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "../../../syscall.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"

namespace abc
{

// so we don't rely on io
inline void
__write_n(const char *str, usize len)
{
  micron::syscall(SYS_write, 2, micron::voidify(str), len);
  micron::syscall(SYS_write, 2, micron::voidify("\n"), 1);
}

inline void
__write(const char *str, usize len)
{
  micron::syscall(SYS_write, 2, micron::voidify(str), len);
}

template <typename T>
inline void
__print_unsigned(T n)
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

template <typename T>
inline void
__print_signed(T n)
{
  if constexpr ( micron::is_integral_v<T> ) {
    using uT = micron::make_unsigned_t<T>;
    if ( n < 0 ) {
      __write("-", 1);
      __print_unsigned(static_cast<uT>(-n));
    } else {
      __print_unsigned(static_cast<uT>(n));
    }
  }
}

inline void
__print_ptr(const void *p)
{
  uintptr_t addr = reinterpret_cast<uintptr_t>(p);
  char buf[2 + sizeof(uintptr_t) * 2];
  buf[0] = '0';
  buf[1] = 'x';
  for ( int i = 0; i < static_cast<int>(sizeof(uintptr_t) * 2); ++i ) {
    char val = (addr >> ((sizeof(uintptr_t) * 2 - i - 1) * 4)) & 0xF;
    buf[2 + i] = (val < 10) ? ('0' + val) : ('a' + val - 10);
  }
  __write(buf, 2 + sizeof(uintptr_t) * 2);
}

template <typename T>
inline __attribute__((always_inline)) void
__debug_print(const char *str [[maybe_unused]], const T n [[maybe_unused]])
{
  if constexpr ( __default_debug_notices ) {
    __write("\033[34mabcmalloc[] debug: \033[0m ", 28);
    __write(str, micron::strlen(str));
    __print_signed(n);
    __write("\n", 1);
  }
}

template <typename T>
inline __attribute__((always_inline)) void
__debug_print_addr(const char *str [[maybe_unused]], T &t [[maybe_unused]])
{
  if constexpr ( __default_debug_notices ) {
    __write("\033[34mabcmalloc[] debug: \033[0m ", 28);
    __write(str, micron::strlen(str));
    __write("at addr: ", 10);
    __print_ptr(reinterpret_cast<void *>(&t));
    __write("\n", 1);
  }
}

template <typename T>
inline __attribute__((always_inline)) void
__debug_print_addr(const char *str [[maybe_unused]], T *t [[maybe_unused]])
{
  if constexpr ( __default_debug_notices ) {
    __write("\033[34mabcmalloc[] debug: \033[0m ", 28);
    __write(str, micron::strlen(str));
    __write("at addr: ", 10);
    __print_ptr(reinterpret_cast<void *>(t));
    __write("\n", 1);
  }
}

};     // namespace abc
