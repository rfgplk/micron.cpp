//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/cstring.hpp"
#include "../../string/format.hpp"
#include "../../string/sstring.hpp"
#include "../io/sys.hpp"
#include "../sys/fcntl.hpp"
#include "bits.hpp"

namespace micron
{
namespace elf
{

// NOTE: these are hard coded search paths; we don't read ld.so.cache nor LD_LIBRARY_PATH; for full proper support that's needed and i'll
// get around to adding it eventually
inline constexpr const char *default_search_paths[] = {
  "/lib64",
  "/usr/lib64",
  "/lib",
  "/usr/lib",
  "/lib/x86_64-linux-gnu",
  "/usr/lib/x86_64-linux-gnu",
  "/lib/aarch64-linux-gnu",
  "/usr/lib/aarch64-linux-gnu",
};

inline constexpr usize default_search_path_count = sizeof(default_search_paths) / sizeof(default_search_paths[0]);

using path_str_t = micron::sstring<512>;

// c arr style, we want to avoid io/ since it pulls in the allocator

inline bool
__file_exists(const char *path) noexcept
{
  i32 fd = posix::open(path, posix::o_rdonly);
  if ( fd < 0 ) return false;
  posix::close(fd);
  return true;
}

inline path_str_t
__join_path(const char *dir, const char *leaf) noexcept
{
  path_str_t out;
  const usize dn = micron::strlen(dir);
  const usize ln = micron::strlen(leaf);
  if ( dn == 0 || dn + 1 + ln + 1 >= out.max_size() ) return out;
  for ( usize i = 0; i < dn; ++i ) out += dir[i];
  if ( out[out.size() - 1] != '/' ) out += '/';
  for ( usize i = 0; i < ln; ++i ) out += leaf[i];
  out.null_term();
  return out;
}

inline path_str_t
__try_runpath(const char *runpath, const char *soname) noexcept
{
  path_str_t hit;
  if ( !runpath ) return hit;

  const usize rp_len = micron::strlen(runpath);
  usize start = 0;
  while ( start < rp_len ) {
    const char *seg_end = micron::format::find(runpath + start, rp_len - start, ':');
    const usize end = seg_end ? static_cast<usize>(seg_end - runpath) : rp_len;
    if ( end > start ) {
      if ( end - start >= 512u ) {
        start = end + 1;
        continue;
      }
      micron::sstring<512> dir;
      for ( usize i = start; i < end; ++i ) dir += runpath[i];
      dir.null_term();
      path_str_t cand = __join_path(dir.c_str(), soname);
      if ( !cand.empty() && __file_exists(cand.c_str()) ) return cand;
    }
    start = end + 1;
  }
  return hit;
}

inline path_str_t
resolve_soname(const char *soname, const char *runpath = nullptr) noexcept
{
  path_str_t out;
  if ( !soname || !soname[0] ) return out;

  const usize sn_len = micron::strlen(soname);

  if ( micron::format::find(soname, sn_len, '/') ) {
    if ( sn_len >= out.max_size() ) return path_str_t{};
    for ( usize i = 0; i < sn_len; ++i ) out += soname[i];
    out.null_term();
    return __file_exists(out.c_str()) ? out : path_str_t{};
  }

  if ( path_str_t hit = __try_runpath(runpath, soname); !hit.empty() ) return hit;

  for ( usize i = 0; i < default_search_path_count; ++i ) {
    path_str_t cand = __join_path(default_search_paths[i], soname);
    if ( !cand.empty() && __file_exists(cand.c_str()) ) return cand;
  }
  return path_str_t{};
}

};      // namespace elf
};      // namespace micron
