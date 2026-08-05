//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../errno.hpp"
#include "../../types.hpp"
#include "../io/inode.hpp"
#include "../sys/limits.hpp"
#include "environ.hpp"

// execv style path resolver

// TODO: stubby for now, expand later + add fp porcelain overloads

namespace micron
{

inline constexpr const char *default_path = "/usr/local/bin:/usr/bin:/bin";

namespace __impl
{

// join dir[0..dlen) + '/' + cmd into out, nul terminated. false when it will not fit
inline bool
__which_join(char *out, usize cap, const char *dir, usize dlen, const char *cmd, usize clen, usize &joined) noexcept
{
  // dir + '/' + cmd + '\0'
  if ( dlen + 1 + clen + 1 > cap ) return false;
  for ( usize i = 0; i < dlen; ++i ) out[i] = dir[i];
  usize n = dlen;
  if ( n == 0 || out[n - 1] != '/' ) out[n++] = '/';
  for ( usize i = 0; i < clen; ++i ) out[n + i] = cmd[i];
  out[n + clen] = 0;
  joined = n + clen;
  return true;
}

};      // namespace __impl

[[nodiscard]] inline max_t
which_into(const char *cmd, char *out, usize cap) noexcept
{
  if ( cmd == nullptr || cmd[0] == 0 || out == nullptr || cap == 0 ) return -error::invalid_arg;

  usize clen = 0;
  bool slash = false;
  for ( ; cmd[clen] != 0; ++clen )
    if ( cmd[clen] == '/' ) slash = true;

  if ( slash ) {
    if ( clen + 1 > cap ) return -error::name_too_long;
    if ( !micron::posix::is_executable(cmd) ) return -error::no_entry;
    for ( usize i = 0; i <= clen; ++i ) out[i] = cmd[i];
    return static_cast<max_t>(clen);
  }

  const char *path = micron::env_get("PATH");
  if ( path == nullptr || path[0] == 0 ) path = default_path;

  bool too_long = false;
  for ( const char *p = path;; ) {
    const char *e = p;
    for ( ; *e != 0 && *e != ':'; ++e );
    const usize dlen = static_cast<usize>(e - p);

    if ( dlen != 0 && p[0] == '/' ) {
      usize joined = 0;
      if ( __impl::__which_join(out, cap, p, dlen, cmd, clen, joined) ) {
        if ( micron::posix::is_executable(out) ) return static_cast<max_t>(joined);
      } else {
        too_long = true;
      }
    }

    if ( *e == 0 ) break;
    p = e + 1;
  }

  out[0] = 0;
  return too_long ? -error::name_too_long : -error::no_entry;
}

[[nodiscard]] inline max_t
which_into(const char *cmd, char (&out)[posix::path_max]) noexcept
{
  return which_into(cmd, out, posix::path_max);
}

};      // namespace micron
