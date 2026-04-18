//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../string/format.hpp"
#include "../string/strings.hpp"
#include "../vector/fvector.hpp"
#include "posix/dir.hpp"

#include "realpath.hpp"

namespace micron
{
namespace io
{

using path_t = micron::sstr<posix::path_max>;

struct pnode_t {
  dir parent;
  dir path;
};

class path
{
  pnode_t nd;

  static path_t
  __parent_of(const path_t &p)
  {
    const char *s = p.c_str();
    usize n = p.size();

    while ( n > 1 && s[n - 1] == '/' ) --n;

    usize sep = n;
    while ( sep > 0 && s[sep - 1] != '/' ) --sep;

    if ( sep == 0 ) return path_t(".");
    if ( sep == 1 ) return path_t("/");

    path_t out;
    for ( usize i = 0; i < sep - 1; ++i ) out += s[i];
    return out;
  }

  static path_t
  __join(const path_t &base, const path_t &rel)
  {
    if ( rel.size() == 0 ) return base;
    if ( rel[0] == '/' ) return rel;
    if ( base.size() == 0 ) return rel;

    path_t out(base);
    if ( out[out.size() - 1] != '/' ) out += '/';
    out += rel;
    return prune(micron::move(out));
  }

public:
  static path_t
  prune(path_t &&str)
  {

    auto itr_slashes = micron::format::find(str, "//");
    if ( itr_slashes ) micron::format::replace_all(str, "//", "/");

    auto itr_dots = micron::format::find(str, "...");
    if ( itr_dots ) micron::format::replace_all(str, "...", "..");

    while ( str.size() > 1 && str[str.size() - 1] == '/' ) str.erase(str.last());

    return str;
  }

  static path_t
  prune(const path_t &str)
  {
    path_t copy(str);
    return prune(micron::move(copy));
  }

  static bool
  is_valid_string(const char *s) noexcept
  {
    return posix::verify(s);
  }

  template <is_string T>
  static bool
  is_valid_string(const T &s) noexcept
  {
    return posix::verify(s);
  }

  bool
  exists() const
  {
    return posix::exists(nd.path.name().c_str());
  }

  bool
  lexists() const
  {
    return posix::lexists(nd.path.name().c_str());
  }

  bool
  is_absolute() const noexcept
  {
    return posix::is_absolute(nd.path.name());
  }

  bool
  is_relative() const noexcept
  {
    return posix::is_relative(nd.path.name());
  }

  bool
  is_mountpoint() const
  {
    return posix::is_mountpoint(nd.path.name().c_str());
  }

  bool
  is_root() const noexcept
  {
    const auto &n = nd.path.name();
    return n.size() == 1 && n[0] == '/';
  }

  bool
  is_file() const
  {
    return posix::is_file(nd.path.name().c_str());
  }

  bool
  is_dir() const
  {
    return posix::is_dir(nd.path.name().c_str());
  }

  bool
  is_socket() const
  {
    return posix::is_socket(nd.path.name().c_str());
  }

  bool
  is_symlink() const
  {
    return posix::is_symlink(nd.path.name().c_str());
  }

  bool
  is_block_device() const
  {
    return posix::is_block_device(nd.path.name().c_str());
  }

  bool
  is_char_device() const
  {
    return posix::is_char_device(nd.path.name().c_str());
  }

  bool
  is_fifo() const
  {
    return posix::is_fifo(nd.path.name().c_str());
  }

  bool
  is_pipe() const
  {
    return is_fifo();
  }

  bool
  is_device() const
  {
    return is_block_device() || is_char_device();
  }

  bool
  directory(const char *name) const
  {
    return nd.path.child_is_dir(name);
  }

  bool
  file(const char *name) const
  {
    return nd.path.child_is_file(name);
  }

  bool
  socket(const char *name) const
  {
    return nd.path.child_is_socket(name);
  }

  bool
  symlink(const char *name) const
  {
    return nd.path.child_is_link(name);
  }

  bool
  block_device(const char *name) const
  {
    return nd.path.child_is_block_device(name);
  }

  bool
  char_device(const char *name) const
  {
    return nd.path.child_is_char_device(name);
  }

  bool
  fifo(const char *name) const
  {
    return nd.path.child_is_fifo(name);
  }

  bool
  pipe(const char *name) const
  {
    return fifo(name);
  }

  bool
  device(const char *name) const
  {
    return block_device(name) || char_device(name);
  }

  bool
  node_exists(const char *name) const
  {
    return nd.path.child_exists(name);
  }

  template <is_string T>
  bool
  directory(const T &s) const
  {
    return directory(s.c_str());
  }

  template <is_string T>
  bool
  file(const T &s) const
  {
    return file(s.c_str());
  }

  template <is_string T>
  bool
  socket(const T &s) const
  {
    return socket(s.c_str());
  }

  template <is_string T>
  bool
  symlink(const T &s) const
  {
    return symlink(s.c_str());
  }

  template <is_string T>
  bool
  block_device(const T &s) const
  {
    return block_device(s.c_str());
  }

  template <is_string T>
  bool
  char_device(const T &s) const
  {
    return char_device(s.c_str());
  }

  template <is_string T>
  bool
  fifo(const T &s) const
  {
    return fifo(s.c_str());
  }

  template <is_string T>
  bool
  pipe(const T &s) const
  {
    return pipe(s.c_str());
  }

  template <is_string T>
  bool
  device(const T &s) const
  {
    return device(s.c_str());
  }

  template <is_string T>
  bool
  node_exists(const T &s) const
  {
    return node_exists(s.c_str());
  }

  bool
  readable() const
  {
    return posix::is_readable(nd.path.name().c_str());
  }

  bool
  writable() const
  {
    return posix::is_writable(nd.path.name().c_str());
  }

  bool
  executable() const
  {
    return posix::is_executable(nd.path.name().c_str());
  }

  bool
  readable(const char *name) const
  {
    return nd.path.child_is_readable(name);
  }

  bool
  writable(const char *name) const
  {
    return nd.path.child_is_writable(name);
  }

  bool
  executable(const char *name) const
  {
    return nd.path.child_is_executable(name);
  }

  template <is_string T>
  bool
  readable(const T &s) const
  {
    return readable(s.c_str());
  }

  template <is_string T>
  bool
  writable(const T &s) const
  {
    return writable(s.c_str());
  }

  template <is_string T>
  bool
  executable(const T &s) const
  {
    return executable(s.c_str());
  }

  linux_permissions
  permissions()
  {
    return nd.path.permissions();
  }

  linux_permissions
  permissions(const char *name) const
  {
    return nd.path.child_permissions(name);
  }

  template <is_string T>
  linux_permissions
  permissions(const T &name) const
  {
    return permissions(name.c_str());
  }

  void
  set_permissions(const linux_permissions &p)
  {
    nd.path.set_permissions(p);
  }

  i32
  set_permissions(const char *name, const linux_permissions &p)
  {
    return nd.path.chmod_child(name, p);
  }

  template <is_string T>
  i32
  set_permissions(const T &name, const linux_permissions &p)
  {
    return set_permissions(name.c_str(), p);
  }

  bool
  owner_read()
  {
    return nd.path.mode_user_read();
  }

  bool
  owner_write()
  {
    return nd.path.mode_user_write();
  }

  bool
  owner_exec()
  {
    return nd.path.mode_user_exec();
  }

  bool
  group_read()
  {
    return nd.path.mode_group_read();
  }

  bool
  group_write()
  {
    return nd.path.mode_group_write();
  }

  bool
  group_exec()
  {
    return nd.path.mode_group_exec();
  }

  bool
  other_read()
  {
    return nd.path.mode_other_read();
  }

  bool
  other_write()
  {
    return nd.path.mode_other_write();
  }

  bool
  other_exec()
  {
    return nd.path.mode_other_exec();
  }

  bool
  has_setuid()
  {
    return nd.path.has_setuid();
  }

  bool
  has_setgid()
  {
    return nd.path.has_setgid();
  }

  bool
  has_sticky()
  {
    return nd.path.has_sticky();
  }

  posix::uid_t
  uid()
  {
    return nd.path.uid();
  }

  posix::gid_t
  gid()
  {
    return nd.path.gid();
  }

  bool
  owned_by(posix::uid_t uid)
  {
    return nd.path.is_owned_by(uid);
  }

  bool
  in_group(posix::gid_t gid)
  {
    return nd.path.is_in_group(gid);
  }

  i32
  chown(posix::uid_t uid, posix::gid_t gid)
  {
    return nd.path.set_owner(uid, gid);
  }

  i32
  chown(const char *name, posix::uid_t uid, posix::gid_t gid)
  {
    return nd.path.chown_child(name, uid, gid);
  }

  template <is_string T>
  i32
  chown(const T &name, posix::uid_t uid, posix::gid_t gid)
  {
    return chown(name.c_str(), uid, gid);
  }

  posix::off_t
  size()
  {
    return nd.path.size();
  }

  posix::ino_t
  inode()
  {
    return nd.path.inode();
  }

  posix::dev_t
  device_id()
  {
    return nd.path.device();
  }

  posix::nlink_t
  link_count()
  {
    return nd.path.link_count();
  }

  posix::time_t
  atime()
  {
    return nd.path.atime();
  }

  posix::time_t
  mtime()
  {
    return nd.path.mtime();
  }

  posix::time_t
  ctime()
  {
    return nd.path.ctime();
  }

  posix::mode_t
  mode()
  {
    return nd.path.mode();
  }

  posix::off_t
  child_size(const char *name) const
  {
    return nd.path.child_size(name);
  }

  template <is_string T>
  posix::off_t
  child_size(const T &name) const
  {
    return child_size(name.c_str());
  }

  const auto &
  get() const
  {
    return nd.path.name();
  }

  const auto &
  get_parent() const
  {
    return nd.parent.name();
  }

  path_t
  basename() const
  {
    const auto &n = nd.path.name();
    const char *s = n.c_str();
    usize l = n.size();

    while ( l > 1 && s[l - 1] == '/' ) --l;

    usize sep = l;
    while ( sep > 0 && s[sep - 1] != '/' ) --sep;

    path_t out;
    for ( usize i = sep; i < l; ++i ) out += s[i];
    return out;
  }

  path_t
  extension() const
  {
    path_t base = basename();
    const char *s = base.c_str();
    usize l = base.size();
    if ( l == 0 || s[0] == '.' ) return path_t();

    usize dot = l;
    while ( dot > 0 && s[dot - 1] != '.' ) --dot;
    if ( dot == 0 ) return path_t();

    path_t out;
    for ( usize i = dot - 1; i < l; ++i ) out += s[i];
    return out;
  }

  path_t
  stem() const
  {
    path_t base = basename();
    path_t ext = extension();
    if ( ext.size() == 0 ) return base;

    path_t out;
    usize keep = base.size() - ext.size();
    for ( usize i = 0; i < keep; ++i ) out += base[i];
    return out;
  }

  path_t
  dirname() const
  {
    return __parent_of(nd.path.name());
  }

  auto
  components() const
  {
    return nd.path.path_components();
  }

  usize
  depth() const noexcept
  {
    return nd.path.depth();
  }

  path_t
  join(const char *rel) const
  {
    return __join(nd.path.name(), path_t(rel));
  }

  template <is_string T>
  path_t
  join(const T &rel) const
  {
    return join(rel.c_str());
  }

  path_t
  canonical() const
  {
    path_t out;
    if ( micron::realpath(nd.path.name().c_str(), &out[0]) == nullptr ) return path_t();
    out.adjust_size();
    return out;
  }

  path_t
  relative_to(const char *base) const
  {
    const char *p = nd.path.name().c_str();
    usize pl = nd.path.name().size();
    usize bl = 0;
    while ( base[bl] ) ++bl;

    usize common = 0;
    usize i = 0;
    while ( i < pl && i < bl && p[i] == base[i] ) {
      if ( p[i] == '/' ) common = i + 1;
      ++i;
    }
    if ( i == bl && (p[i] == '/' || p[i] == '\0') ) common = i + (p[i] == '/');

    usize up = 0;
    for ( usize j = common; j < bl; ++j )
      if ( base[j] == '/' ) ++up;
    if ( common < bl ) ++up;

    path_t out;
    for ( usize j = 0; j < up; ++j ) {
      if ( out.size() ) out += '/';
      out += "..";
    }
    if ( common < pl ) {
      if ( out.size() ) out += '/';
      for ( usize j = common; j < pl; ++j ) out += p[j];
    }
    if ( out.size() == 0 ) out += '.';
    return out;
  }

  template <is_string T>
  path_t
  relative_to(const T &base) const
  {
    return relative_to(base.c_str());
  }

  auto
  files()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( file(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  dirs()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( directory(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  all()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  sockets()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( socket(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  symlinks()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( symlink(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  block_devices()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( block_device(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  char_devices()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( char_device(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  auto
  fifos()
  {
    micron::fvector<path_t> paths;
    auto &p = nd.path.get_children();
    for ( auto &n : p )
      if ( fifo(n.d_name.c_str()) ) paths.emplace_back(n.d_name);
    return paths;
  }

  micron::vector<path_t>
  absolute(const path_t &n)
  {
    micron::vector<path_t> paths;

    path_t abs;
    if ( !micron::realpath(n.c_str(), &abs[0]) ) return paths;

    abs.adjust_size();

    paths.emplace_back(abs);

    dir d(abs);
    while ( d.name() != "/" ) {
      dir parent = d.up();
      paths.emplace_back(parent.name());
      d = micron::move(parent);
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
      dir parent = d.up();
      paths.emplace_back(parent.name());
      d = micron::move(parent);
    }

    micron::sstring<posix::path_max> abs("/");
    for ( auto e = paths.end(); e >= paths.begin(); --e ) {
      abs += *e;
      abs += "/";
    }
    return prune(path_t(abs.c_str()));
  }

  path_t
  resolve(const char *cstr)
  {
    if ( micron::strlen(cstr) >= max_name ) exc<except::library_error>("io::path::resolve out of range.");
    path_t str(cstr);
    if ( str == "." ) return absolute(str).at(0);
    if ( str == ".." || micron::format::ends_with(str, "..") ) return absolute(str).at(1);
    return prune(micron::move(str));
  }

  template <is_string T>
  path_t
  resolve(T &&str)
  {
    if ( str == "/" ) return path_t(str.c_str());
    if ( str == "." ) return absolute(path_t(str.c_str())).at(0);
    if ( str == ".." || micron::format::ends_with(str, "..") ) return absolute(path_t(str.c_str())).at(1);
    return prune(path_t(str.c_str()));
  }

  void
  up()
  {
    nd.path = micron::move(nd.parent);
    nd.parent = dir(__parent_of(nd.path.name()));
  }

  void
  down(const char *name)
  {
    if ( !directory(name) ) exc<except::io_error>("io::path::down(), child is not a directory.");
    dir child = nd.path.down(name);
    nd.parent = micron::move(nd.path);
    nd.path = micron::move(child);
  }

  template <is_string T>
  void
  down(const T &name)
  {
    down(name.c_str());
  }

  void
  set(const char *str)
  {
    path_t clean = resolve(str);
    path_t par_str = __parent_of(clean);
    nd.parent = dir(par_str);
    nd.path = dir(clean);
  }

  template <is_string T>
  void
  set(const T &str)
  {
    set(str.c_str());
  }

  void
  to_root()
  {
    nd.parent = dir("/");
    nd.path = dir("/");
  }

  i32
  chdir() const
  {
    return nd.path.chdir();
  }

  bool
  is_same_as(const path &other) const
  {
    return posix::is_same_file(nd.path.name().c_str(), other.nd.path.name().c_str());
  }

  bool
  is_same_as(const char *other) const
  {
    return posix::is_same_file(nd.path.name().c_str(), other);
  }

  template <is_string T>
  bool
  is_same_as(const T &other) const
  {
    return is_same_as(other.c_str());
  }

  bool
  operator==(const path &o) const noexcept
  {
    return nd.path.name() == o.nd.path.name();
  }

  bool
  operator!=(const path &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const path &o) const noexcept
  {
    const char *a = nd.path.name().c_str();
    const char *b = o.nd.path.name().c_str();
    usize al = nd.path.name().size();
    usize bl = o.nd.path.name().size();
    usize n = al < bl ? al : bl;
    for ( usize i = 0; i < n; ++i ) {
      if ( (unsigned char)a[i] < (unsigned char)b[i] ) return true;
      if ( (unsigned char)a[i] > (unsigned char)b[i] ) return false;
    }
    return al < bl;
  }

  bool
  operator>(const path &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const path &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const path &o) const noexcept
  {
    return !(*this < o);
  }

  bool
  operator==(const char *s) const noexcept
  {
    return nd.path.name() == s;
  }

  bool
  operator!=(const char *s) const noexcept
  {
    return nd.path.name() != s;
  }

  template <is_string T>
  bool
  operator==(const T &s) const noexcept
  {
    return nd.path.name() == s;
  }

  template <is_string T>
  bool
  operator!=(const T &s) const noexcept
  {
    return nd.path.name() != s;
  }

  path_t
  operator/(const char *rel) const
  {
    return join(rel);
  }

  template <is_string T>
  path_t
  operator/(const T &rel) const
  {
    return join(rel);
  }

  path() : nd({ dir(resolve("..")), dir(resolve(".")) }) {}

  path(const char *name)
      : nd{ .parent = dir(resolve(prune(micron::format::concat<path_t>(name, "/..")))), .path = dir(resolve(prune(name))) }
  {
  }

  template <is_string T> explicit path(const T &name) : path(name.c_str()) {}

  path(const path &o) : nd(o.nd) {}

  path(path &&o) noexcept : nd(micron::move(o.nd)) {}

  path &
  operator=(const path &o)
  {
    nd = o.nd;
    return *this;
  }

  path &
  operator=(path &&o) noexcept
  {
    nd = micron::move(o.nd);
    return *this;
  }
};

};     // namespace io
};     // namespace micron
