//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../tuple.hpp"
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

inline void
walk(path &&p, micron::fvector<path_t> &out, node_id *chain, u32 depth, collect what)
{
  if ( depth >= ftw_max_depth ) return;

  // entries collected at this level
  micron::fvector<path_t> coll = (what == collect::files) ? p.files() : (what == collect::all ? p.all() : p.dirs());
  for ( auto &n : coll ) {
    if ( n == "." || n == ".." ) continue;
    out.push_back(p.join(n.c_str()));
  }

  // recurse into sub-directories only
  micron::fvector<path_t> subdirs = p.dirs();
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
    try {
      walk(path(child.c_str()), out, chain, depth + 1, what);
    } catch ( except::__base_exception & ) {
    }
  }
}

inline micron::fvector<path_t>
run(path &&p, collect what)
{
  micron::fvector<path_t> out;
  node_id chain[ftw_max_depth];
  u32 start = 0;
  posix::stat_t st{};
  if ( posix::__impl::__stat(p.get().c_str(), st) ) {
    chain[0] = node_id{ st.st_dev, static_cast<posix::ino64_t>(st.st_ino) };
    start = 1;
  }
  walk(micron::move(p), out, chain, start, what);
  return out;
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
};      // namespace io
};      // namespace micron
