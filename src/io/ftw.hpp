//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "paths.hpp"
#include "os/iosys.hpp"

namespace micron
{
namespace io
{
namespace __ftw
{
enum class collect { dirs, all, files };

struct node_id {
  posix::dev_t dev;
  posix::ino64_t ino;
};

constexpr u32 ftw_max_depth = 100;

template<typename Sink>
inline bool
walk(path &&p, Sink &&sink, node_id *chain, u32 depth, collect what, bool is_root = false)
{
  if ( depth >= ftw_max_depth ) return true;

  // entries collected at this level
  // NB: this must stay exception-free. An unreadable sub-directory (EACCES) is skipped, not fatal,
  // and that cannot be expressed with try/catch: below the eh gate exc<> aborts instead of
  // throwing, so a catch-based skip would kill the process mid-walk. try_*() report instead.
  micron::fvector<path_t> coll;
  if ( const i32 r = (what == collect::files) ? p.try_files(coll) : (what == collect::all ? p.try_all(coll) : p.try_dirs(coll));
       r < 0 ) {
    if ( is_root ) exc_e<except::io_error>(r, "micron::io::ftw, failed to open root directory.");
    return true;      // unreadable sub-directory -> skip
  }
  for ( auto &n : coll ) {
    if ( n == "." || n == ".." ) continue;
    if ( !sink(p.join(n.c_str())) ) return false;
  }

  // recurse into sub-directories only
  micron::fvector<path_t> subdirs;
  if ( const i32 r = p.try_dirs(subdirs); r < 0 ) {
    if ( is_root ) exc_e<except::io_error>(r, "micron::io::ftw, failed to open root directory.");
    return true;      // unreadable sub-directory -> skip
  }
  for ( auto &n : subdirs ) {
    if ( n == "." || n == ".." ) continue;
    path_t child = p.join(n.c_str());
    // NOTE:: paths are trusted; not racefree against an attacker swapping child for a symlink between the check and the open
    if ( posix::is_symlink(child.c_str()) ) continue;      // WARNING: do NOT follow symlinks

    posix::stat_t st{};
    if ( !posix::__impl::__stat(child.c_str(), st) ) continue;
    node_id id{ st.st_dev, static_cast<posix::ino64_t>(st.st_ino) };
    bool seen = false;
    for ( u32 i = 0; i < depth; ++i )
      if ( chain[i].dev == id.dev && chain[i].ino == id.ino ) {
        seen = true;
        break;
      }
    if ( seen ) continue;      // directory cycle -> skip

    chain[depth] = id;
    if ( !walk(path(child.c_str()), sink, chain, depth + 1, what) ) return false;
  }
  return true;
}

template<typename Sink>
inline bool
run_sink(path &&p, Sink &&sink, collect what)
{
  node_id chain[ftw_max_depth];
  u32 start = 0;
  posix::stat_t st{};
  if ( posix::__impl::__stat(p.get().c_str(), st) ) {
    chain[0] = node_id{ st.st_dev, static_cast<posix::ino64_t>(st.st_ino) };
    start = 1;
  }
  return walk(micron::move(p), sink, chain, start, what, true);
}

inline micron::fvector<path_t>
run(path &&p, collect what)
{
  micron::fvector<path_t> out;
  run_sink(
      micron::move(p),
      [&out](path_t &&x) {
        out.push_back(micron::move(x));
        return true;
      },
      what);
  return out;
}

template<typename Fn>
inline usize
visit(path &&p, Fn &&fn, collect what)
{
  usize n = 0;
  run_sink(
      micron::move(p),
      [&](path_t &&x) -> bool {
        ++n;
        if constexpr ( micron::is_convertible_v<micron::invoke_result_t<Fn, const path_t &>, bool> )
          return static_cast<bool>(fn(x));
        else {
          fn(x);
          return true;
        }
      },
      what);
  return n;
}
}      // namespace __ftw

inline auto
ftw(path &&p)
{
  return __ftw::run(micron::move(p), __ftw::collect::dirs);
}

inline auto
ftw_all(path &&p)
{
  return __ftw::run(micron::move(p), __ftw::collect::all);
}

inline auto
ftw_files(path &&p)
{
  return __ftw::run(micron::move(p), __ftw::collect::files);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// visitor forms
// stops the walk on false
template<typename Fn>
  requires micron::invocable<Fn, const path_t &>
inline usize
ftw(path &&p, Fn &&fn)
{
  return __ftw::visit(micron::move(p), micron::forward<Fn>(fn), __ftw::collect::dirs);
}

template<typename Fn>
  requires micron::invocable<Fn, const path_t &>
inline usize
ftw_all(path &&p, Fn &&fn)
{
  return __ftw::visit(micron::move(p), micron::forward<Fn>(fn), __ftw::collect::all);
}

template<typename Fn>
  requires micron::invocable<Fn, const path_t &>
inline usize
ftw_files(path &&p, Fn &&fn)
{
  return __ftw::visit(micron::move(p), micron::forward<Fn>(fn), __ftw::collect::files);
}

// left fold over every visited path
template<typename R, typename Fn>
  requires fn_fold<Fn, R, const path_t &>
inline R
ftw_fold(path &&p, R init, Fn &&fn)
{
  __ftw::run_sink(
      micron::move(p),
      [&](path_t &&x) -> bool {
        init = fn(micron::move(init), x);
        return true;
      },
      __ftw::collect::all);
  return init;
}
};      // namespace io
};      // namespace micron
