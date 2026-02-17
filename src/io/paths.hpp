//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../std.hpp"
#include "../string/format.hpp"
#include "../string/strings.hpp"
#include "../vector/fvector.hpp"
#include "posix/dir.hpp"

namespace micron
{
namespace io
{

typedef micron::sstr<max_name> path_t;

struct pnode_t {
  dir parent;     // DIR*
  dir path;       // DIR*
};

// primary class for managing paths, links with dir
class path
{
  pnode_t nd;     // the node storing the path (note chdir is only explicitly called, all cds are virtual (not changing
                  // main bin path))
public:
  // remove any excess '/', dots etc
  // static since this is useful outside of path and i was too lazy to move it out
  ~path() = default;

  static path_t
  prune(path_t &&str)
  {
    auto itr_slashes = micron::format::find(str, "//");
    auto itr_dots = micron::format::find(str, "...");
    if ( itr_slashes )
      micron::format::replace_all(str, "//", "/");
    if ( itr_dots )
      micron::format::replace_all(str, "...", "..");
    if ( micron::format::ends_with(str, "/") && str.size() > 1 )
      str.erase(str.last());
    return str;
  }

  bool
  directory(const char *name) const
  {
    return is_dir_at(nd.path.fd(), name);
  }

  bool
  file(const char *name) const
  {
    return is_file_at(nd.path.fd(), name);
  }

  bool
  socket(const char *name) const
  {
    return is_socket_at(nd.path.fd(), name);
  }

  bool
  symlink(const char *name) const
  {
    return is_symlink_at(nd.path.fd(), name);
  }

  bool
  block_device(const char *name) const
  {
    return is_block_device_at(nd.path.fd(), name);
  }

  auto
  files(void)
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p ) {
      if ( file(n.a.d_name.c_str()) ) {
        paths.emplace_back(n.a.d_name);
      }
    }
    return paths;
  }

  auto
  dirs(void)
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p ) {
      if ( directory(n.a.d_name.c_str()) ) {
        paths.emplace_back(n.a.d_name);
      }
    }
    return paths;
  }

  auto
  all(void)
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p ) {
      paths.emplace_back(n.a.d_name);
    }
    return paths;
  }

  // get path
  auto
  get(void) const
  {
    return nd.path.name();
  }

  auto
  get_parent(void) const
  {
    return nd.parent.name();
  }

  micron::vector<path_t>
  absolute(const path_t &n)
  {
    micron::vector<path_t> paths;
    paths.emplace_back(path_t());
    ::realpath(n.c_str(), &paths.back()[0]);
    paths.back().adjust_size();
    dir d(n);
    while ( d.name() != "/" ) {
      d.up();
      paths.emplace_back(d.name());
    }
    return paths;
  }

  micron::sstring<posix::path_max>
  absolute(const pnode_t &n)
  {
    micron::vector<path_t> paths;
    paths.emplace_back(n.path.name());
    dir d(n.path.name());
    while ( d.name() != "/" ) {
      d.up();
      paths.emplace_back(d.name());
    }
    micron::sstring<posix::path_max> absolute_path("/");
    auto s = paths.begin();
    for ( auto e = paths.end(); e >= s; --e ) {
      absolute_path += *e;
      absolute_path += "/";
    }
    return absolute_path;
  }

  void
  up(void)
  {
    nd.path = micron::move(nd.parent);
    nd.parent = micron::move(dir(resolve("../" + nd.parent.name())));
  }

  void
  set(const char *str)
  {
    nd.parent = dir(resolve(micron::format::concat<path_t>(str, "/..")));
    nd.path = dir(resolve(str));
  }

  path_t
  resolve(const char *cstr)
  {
    if ( micron::strlen(cstr) >= max_name )
      exc<except::library_error>("io::resolve out of range.");
    micron::sstr<max_name> str(cstr);
    if ( str == "." )
      return absolute(str).at(0);
    else if ( str == ".." or micron::format::ends_with(str, "..") )
      return absolute(str).at(1);
    return str;
  }

  template <is_string T>
  path_t
  resolve(T &&str)
  {
    if ( str == "/" )
      return str;
    if ( str == "." )
      return absolute(str).at(0);
    else if ( str == ".." or micron::format::ends_with(str, "..") )
      return absolute(str).at(1);
    return str;
  }

  // dflt
  path(void) : nd({ dir(resolve("..")), dir(resolve(".")) }) {}

  path(const char *name)
      : nd{ .parent = dir(resolve(prune(micron::format::concat<path_t>(name, "/..")))), .path = dir(resolve(prune(name))) }
  {
  }

  path(const path &o) : nd(o.nd) {}

  path(path &&o) : nd(micron::move(o.nd)) {}

  path &
  operator=(const path &o)
  {
    nd = o.nd;
    return *this;
  }

  path &
  operator=(path &&o)
  {
    nd = micron::move(o.nd);
    return *this;
  }

  bool
  operator==(const path &o)
  {
    return (nd.path.dname == o.nd.path.dname);
  }
};
};
};
