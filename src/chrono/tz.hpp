//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../syscall.hpp"
#include "../types.hpp"

#include "civil.hpp"

namespace micron
{
namespace chrono
{
namespace tz
{

inline constexpr usize scratch_recommended = 128u << 10;

template<usize N = 512> struct tz_storage {
  i64 trans[N]{};
  i32 off[N]{};
  usize n = 0;
  i32 first = 0;
  bool loaded = false;

  static constexpr usize capacity = N;

  constexpr tz_table
  view() const noexcept
  {
    if ( !loaded ) return tz_table{};
    tz_table t{ trans, off, n, first };
    return t;
  }
};

namespace __impl
{

constexpr u32
be32(const u8 *p) noexcept
{
  return (static_cast<u32>(p[0]) << 24) | (static_cast<u32>(p[1]) << 16) | (static_cast<u32>(p[2]) << 8) | static_cast<u32>(p[3]);
}

constexpr i64
be64(const u8 *p) noexcept
{
  u64 v = 0;
  for ( usize i = 0; i < 8; ++i ) v = (v << 8) | static_cast<u64>(p[i]);
  return static_cast<i64>(v);
}

struct header {
  u8 version;
  u32 isutcnt;
  u32 isstdcnt;
  u32 leapcnt;
  u32 timecnt;
  u32 typecnt;
  u32 charcnt;
};

constexpr bool
read_header(const u8 *p, usize avail, header &h) noexcept
{
  if ( avail < 44 ) return false;
  if ( p[0] != 'T' || p[1] != 'Z' || p[2] != 'i' || p[3] != 'f' ) return false;
  h.version = p[4];
  h.isutcnt = be32(p + 20);
  h.isstdcnt = be32(p + 24);
  h.leapcnt = be32(p + 28);
  h.timecnt = be32(p + 32);
  h.typecnt = be32(p + 36);
  h.charcnt = be32(p + 40);
  return true;
}

constexpr u64
block_size(const header &h, u64 ts) noexcept
{
  return static_cast<u64>(h.timecnt) * ts + static_cast<u64>(h.timecnt) + static_cast<u64>(h.typecnt) * 6u + static_cast<u64>(h.charcnt)
         + static_cast<u64>(h.leapcnt) * (ts + 4u) + static_cast<u64>(h.isstdcnt) + static_cast<u64>(h.isutcnt);
}

inline bool
parse_v2(const u8 *p, usize avail, i64 *trans, i32 *off, usize cap, usize &n_out, i32 &first_out) noexcept
{
  header h{};
  if ( !read_header(p, avail, h) ) return false;
  if ( h.typecnt == 0 ) return false;
  const u64 need = 44u + block_size(h, 8u);
  if ( need > avail ) return false;

  const u8 *times = p + 44;
  const u8 *idx = times + static_cast<u64>(h.timecnt) * 8u;
  const u8 *ttinfo = idx + h.timecnt;

  u32 firstt = 0;
  for ( u32 i = 0; i < h.typecnt; ++i ) {
    if ( ttinfo[i * 6u + 4u] == 0 ) {
      firstt = i;
      break;
    }
  }
  first_out = static_cast<i32>(be32(ttinfo + firstt * 6u));

  usize n = 0;
  for ( u32 i = 0; i < h.timecnt && n < cap; ++i ) {
    const u32 ti = idx[i];
    if ( ti >= h.typecnt ) continue;
    trans[n] = be64(times + static_cast<u64>(i) * 8u);
    off[n] = static_cast<i32>(be32(ttinfo + static_cast<u64>(ti) * 6u));
    ++n;
  }
  n_out = n;
  return true;
}

};      // namespace __impl

template<usize N>
inline bool
parse(const u8 *data, usize len, tz_storage<N> &out) noexcept
{
  out.loaded = false;
  out.n = 0;
  out.first = 0;
  if ( !data || len < 44 ) return false;

  __impl::header h{};
  if ( !__impl::read_header(data, len, h) ) return false;

  if ( h.version == '2' || h.version == '3' || h.version == '4' ) {
    const u64 skip = 44u + __impl::block_size(h, 4u);
    if ( skip >= len ) return false;
    if ( !__impl::parse_v2(data + skip, len - static_cast<usize>(skip), out.trans, out.off, N, out.n, out.first) ) return false;
    out.loaded = true;
    return true;
  }
  return false;
}

namespace __impl
{

inline max_t
slurp(const char *path, u8 *buf, usize cap) noexcept
{
  const long fd = micron::syscall(SYS_openat, micron::posix::at_fdcwd, path, 0 /* O_RDONLY */, 0);
  if ( micron::syscall_failed(fd) ) return static_cast<max_t>(fd);
  usize got = 0;
  for ( ;; ) {
    if ( got >= cap ) break;
    const long r = micron::syscall(SYS_read, fd, buf + got, cap - got);
    if ( r < 0 ) {
      if ( -r == static_cast<long>(error::interrupted) ) continue;
      micron::syscall(SYS_close, fd);
      return static_cast<max_t>(r);
    }
    if ( r == 0 ) break;
    got += static_cast<usize>(r);
  }
  micron::syscall(SYS_close, fd);
  return static_cast<max_t>(got);
}

constexpr bool
safe_zone(const char *z) noexcept
{
  if ( !z || z[0] == '\0' ) return false;
  if ( z[0] == '/' || z[0] == ':' ) return false;
  usize n = 0;
  for ( ; z[n]; ++n ) {
    const char c = z[n];
    if ( c == '.' ) return false;
    const bool ok
        = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '/' || c == '_' || c == '-' || c == '+';
    if ( !ok ) return false;
    if ( n > 200 ) return false;
  }
  return n != 0;
}

};      // namespace __impl

template<usize N>
inline bool
load_named(const char *zone, u8 *scratch, usize scratch_len, tz_storage<N> &out) noexcept
{
  out.loaded = false;
  if ( !__impl::safe_zone(zone) ) return false;

  char path[256];
  const char pre[] = "/usr/share/zoneinfo/";
  usize k = 0;
  for ( ; pre[k]; ++k ) path[k] = pre[k];
  usize j = 0;
  for ( ; zone[j] && k + 1 < sizeof(path); ++j, ++k ) path[k] = zone[j];
  if ( zone[j] != '\0' ) return false;
  path[k] = '\0';

  const max_t n = __impl::slurp(path, scratch, scratch_len);
  if ( n <= 0 ) return false;
  return parse(scratch, static_cast<usize>(n), out);
}

template<usize N>
inline bool
load_localtime(u8 *scratch, usize scratch_len, tz_storage<N> &out) noexcept
{
  const max_t n = __impl::slurp("/etc/localtime", scratch, scratch_len);
  if ( n <= 0 ) {
    out.loaded = false;
    return false;
  }
  return parse(scratch, static_cast<usize>(n), out);
}

template<usize N>
inline bool
load_local(const char *tz_env, u8 *scratch, usize scratch_len, tz_storage<N> &out) noexcept
{
  if ( tz_env && __impl::safe_zone(tz_env) ) {
    if ( load_named(tz_env, scratch, scratch_len, out) ) return true;
  }
  return load_localtime(scratch, scratch_len, out);
}

};      // namespace tz
};      // namespace chrono
};      // namespace micron
