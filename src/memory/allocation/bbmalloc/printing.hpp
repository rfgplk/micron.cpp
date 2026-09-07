// Copyright (c) 2026 David Lucius Severus
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

#include "config.hpp"

#include "../../../types.hpp"

#if defined(MICRON_BB_NO_DIAG)
#else
#include "../../../port/panic.hpp"
#endif

namespace bb
{
namespace __bits
{

inline void
__write(const char *s, usize n) noexcept
{
#if defined(MICRON_BB_NO_DIAG)
  (void)s;
  (void)n;
#else
  micron::port::write_diag(s, n);
#endif
}

inline usize
__strlen(const char *s) noexcept
{
  usize n = 0;
  while( s[n] != '\0' )
    ++n;
  return n;
}

inline void
__write(const char *s) noexcept
{
  __write(s, __strlen(s));
}

inline void
__print_unsigned(u64 v) noexcept
{
  char buf[24];
  usize i = sizeof(buf);
  do {
    buf[--i] = static_cast<char>('0' + (v % 10));
    v /= 10;
  } while( v != 0 );
  __write(buf + i, sizeof(buf) - i);
}

inline void
__print_ptr(const void *p) noexcept
{
  char buf[2 + sizeof(void *) * 2];
  buf[0] = '0';
  buf[1] = 'x';
  uintptr_t v = reinterpret_cast<uintptr_t>(p);
  for( usize i = sizeof(buf); i > 2; --i ) {
    const unsigned d = static_cast<unsigned>(v & 0xF);
    buf[i - 1] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
    v >>= 4;
  }
  __write(buf, sizeof(buf));
}

__bb_cold void
__report(const char *msg, const void *p) noexcept
{
  __write("bbmalloc: ");
  __write(msg);
  __write(" at ");
  __print_ptr(p);
  __write("\n");
}

inline void
__notice(const char *msg, u64 v) noexcept
{
  if constexpr( __default_debug_notices ) {
    __write("bbmalloc: ");
    __write(msg);
    __write(" ");
    __print_unsigned(v);
    __write("\n");
  } else {
    (void)msg;
    (void)v;
  }
}

[[noreturn]] inline void
__halt(int code) noexcept
{
#if defined(MICRON_BB_NO_DIAG)
  (void)code;
  for( ;; )
    __builtin_trap();
#else
  micron::port::halt(code);
#endif
}

};
};
