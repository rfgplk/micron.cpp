//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../linux/io/fd.hpp"
#include "../../types.hpp"
#include "host.hpp"

namespace micron
{
using posix::fd_t;
}

namespace micron::posix
{
inline max_t
write(int fd, const void *p, usize n) noexcept
{
  if ( fd != 1 && fd != 2 ) return -9;
  return __micron_wasm_host_write(p, n, static_cast<unsigned>(fd));
}
}      // namespace micron::posix

namespace micron::io
{
inline constexpr fd_t stdout{ 1 }, stderr{ 2 };
template<int, int, bool> class stream;

inline max_t
fwrite(const char *p, usize n, fd_t fd)
{
  return posix::write(fd.fd, p, n);
}

inline max_t
fput(char c, fd_t fd)
{
  return fwrite(&c, 1, fd);
}

inline max_t
fput(const char *p, fd_t fd)
{
  usize n = 0;
  while ( p[n] ) ++n;
  return fwrite(p, n, fd);
}

inline max_t
fflush(fd_t) noexcept
{
  return 0;
}

inline max_t
unifput(const char32_t *p, fd_t fd)
{
  max_t result = 0;
  while ( *p ) {
    unsigned c = static_cast<unsigned>(*p++);
    char buf[4];
    unsigned n;
    if ( c <= 0x7f ) {
      buf[0] = c;
      n = 1;
    } else if ( c <= 0x7ff ) {
      buf[0] = 0xc0 | (c >> 6);
      buf[1] = 0x80 | (c & 63);
      n = 2;
    } else if ( c <= 0xffff && !(c >= 0xd800 && c <= 0xdfff) ) {
      buf[0] = 0xe0 | (c >> 12);
      buf[1] = 0x80 | ((c >> 6) & 63);
      buf[2] = 0x80 | (c & 63);
      n = 3;
    } else if ( c >= 0x10000 && c <= 0x10ffff ) {
      buf[0] = 0xf0 | (c >> 18);
      buf[1] = 0x80 | ((c >> 12) & 63);
      buf[2] = 0x80 | ((c >> 6) & 63);
      buf[3] = 0x80 | (c & 63);
      n = 4;
    } else {
      buf[0] = '\xef';
      buf[1] = '\xbf';
      buf[2] = '\xbd';
      n = 3;
    }
    const auto r = fwrite(buf, n, fd);
    if ( r < 0 ) return r;
    result += r;
  }
  return result;
}

inline max_t
wfput(const wchar_t *p, fd_t fd)
{
  return unifput(reinterpret_cast<const char32_t *>(p), fd);
}
}      // namespace micron::io
