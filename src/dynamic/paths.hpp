//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/elf/auxval.hpp"
#include "../linux/elf/search.hpp"
#include "../linux/process/environ.hpp"
#include "../linux/sys/system.hpp"
#include "../string/format.hpp"

#include "flags.hpp"

namespace micron
{
namespace elf
{
namespace dl
{

inline u32 __secure_exec_state = 0;      // 0 = unprobed, 1 = ordinary, 2 = secure

[[gnu::cold, gnu::noinline]] inline bool
__secure_exec_probe() noexcept
{
  if ( micron::auxval_has(micron::at_secure) ) return micron::getauxval(micron::at_secure) != 0;
  // auxv unreadable (no /proc)
  return posix::getuid() != posix::geteuid() || posix::getgid() != posix::getegid();
}

inline bool
is_secure_exec() noexcept
{
  u32 st = __atomic_load_n(&__secure_exec_state, __ATOMIC_ACQUIRE);
  if ( st == 0 ) [[unlikely]] {
    st = __secure_exec_probe() ? 2u : 1u;
    __atomic_store_n(&__secure_exec_state, st, __ATOMIC_RELEASE);
  }
  return st == 2u;
}

inline bool
__segment_is_trusted(const char *seg, usize len) noexcept
{
  if ( len == 0 || seg[0] != '/' ) return false;
  for ( usize i = 0; i < len; ++i )
    if ( seg[i] == '$' ) return false;
  return true;
}

inline path_str_t
__dirname_of(const char *path) noexcept
{
  path_str_t out;
  if ( !path ) return out;
  const usize n = micron::strlen(path);
  usize cut = 0;
  for ( usize i = n; i-- > 0; ) {
    if ( path[i] == '/' ) {
      cut = i;
      break;
    }
  }
  if ( cut == 0 ) return out;
  for ( usize i = 0; i < cut && out.size() + 1 < out.max_size(); ++i ) out += path[i];
  out.null_term();
  return out;
}

inline path_str_t
__expand_origin(const char *spec, usize len, const char *origin) noexcept
{
  path_str_t out;
  for ( usize i = 0; i < len && out.size() + 1 < out.max_size(); ) {
    if ( spec[i] != '$' ) {
      out += spec[i++];
      continue;
    }
    const bool braced = (i + 1 < len && spec[i + 1] == '{');
    const usize name_at = i + (braced ? 2 : 1);
    auto starts = [&](const char *w) {
      const usize wl = micron::strlen(w);
      if ( name_at + wl > len ) return false;
      for ( usize k = 0; k < wl; ++k )
        if ( spec[name_at + k] != w[k] ) return false;
      return true;
    };
    if ( starts("ORIGIN") ) {
      if ( origin )
        for ( usize k = 0; origin[k] && out.size() + 1 < out.max_size(); ++k ) out += origin[k];
      i = name_at + 6 + (braced ? 1 : 0);
    } else if ( starts("LIB") ) {

      i = name_at + 3 + (braced ? 1 : 0);
    } else if ( starts("PLATFORM") ) {
      i = name_at + 8 + (braced ? 1 : 0);
    } else {
      out += spec[i++];
    }
  }
  out.null_term();
  return out;
}

inline path_str_t
__try_path_list(const char *list, const char *leaf, const char *origin, bool secure = is_secure_exec()) noexcept
{
  path_str_t hit;
  if ( !list || !*list ) return hit;

  const usize n = micron::strlen(list);
  usize start = 0;
  while ( start <= n ) {
    const char *seg_end = micron::format::find(list + start, n - start, ':');
    const usize end = seg_end ? static_cast<usize>(seg_end - list) : n;
    if ( end > start && (!secure || __segment_is_trusted(list + start, end - start)) ) {
      path_str_t dir = __expand_origin(list + start, end - start, origin);
      if ( !dir.empty() ) {
        path_str_t cand = __join_path(dir.c_str(), leaf);
        if ( !cand.empty() && __file_is_native_elf(cand.c_str()) ) return cand;
      }
    }
    if ( !seg_end ) break;
    start = end + 1;
  }
  return hit;
}

inline path_str_t
resolve_dependency(const char *soname, const char *rpath, const char *runpath, const char *origin) noexcept
{
  path_str_t out;
  if ( !soname || !soname[0] ) return out;

  const bool secure = is_secure_exec();

  if ( micron::format::find(soname, micron::strlen(soname), '/') ) {
    if ( secure && soname[0] != '/' ) return out;      // relative path, attacker-chosen cwd
    const usize sn = micron::strlen(soname);
    if ( sn >= out.max_size() ) return path_str_t{};
    for ( usize i = 0; i < sn; ++i ) out += soname[i];
    out.null_term();
    return __file_is_native_elf(out.c_str()) ? out : path_str_t{};
  }

  if ( path_str_t h = __try_path_list(rpath, soname, origin, secure); !h.empty() ) return h;
  if ( !secure )
    if ( path_str_t h = __try_path_list(env_get("LD_LIBRARY_PATH"), soname, origin, secure); !h.empty() ) return h;
  if ( path_str_t h = __try_path_list(runpath, soname, origin, secure); !h.empty() ) return h;

  for ( usize i = 0; i < default_search_path_count; ++i ) {
    path_str_t cand = __join_path(default_search_paths[i], soname);
    if ( !cand.empty() && __file_is_native_elf(cand.c_str()) ) return cand;
  }
  return path_str_t{};
}

};      // namespace dl
};      // namespace elf
};      // namespace micron
