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
#include "os/dir.hpp"

#include "realpath.hpp"

namespace micron
{
namespace io
{

using path_t = micron::sstr<posix::path_max>;

// io::path is a lexical, string-backed path handle; holds NO file descriptors and never touches the filesystem on construction
// can represent any path (file, dir, missing) cheaply
class path
{
  path_t __p;      // normalised path string (// collapsed, no trailing slash except root)

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

  // NOTE: filesystem paths are trusted
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

  path_t
  __child(const char *name) const
  {
    return __join(__p, path_t(name));
  }

  bool
  __stat(stat_t &out) const
  {
    return posix::__impl::__stat(__p.c_str(), out);
  }

  bool
  __mode_bit(u32 bit) const
  {
    stat_t st{};
    return __stat(st) && (st.st_mode & bit);
  }

  static linux_permissions
  __perms_of(const char *p)
  {
    stat_t st{};
    if ( !posix::__impl::__stat(p, st) ) return linux_permissions::none();
    linux_permissions perms = linux_permissions::from_mode(st.st_mode);
    perms.setuid = (st.st_mode & posix::s_isuid) != 0;
    perms.setgid = (st.st_mode & posix::s_isgid) != 0;
    perms.sticky = (st.st_mode & posix::s_isvtx) != 0;
    return perms;
  }

  template<typename Pred>
  micron::fvector<path_t>
  __list(Pred pred) const
  {
    micron::fvector<path_t> out;
    if ( !posix::is_dir(__p.c_str()) ) return out;      // never construct dir on a non-dir (no throw)
    dir d(__p);                                         // fd opens here, closes when d leaves scope
    for ( const auto &n : d.get_children() )
      if ( pred(n) ) out.emplace_back(n.d_name);
    return out;
  }

public:
  static path_t
  prune(path_t &&str)
  {
    if ( micron::format::find(str, "//") ) micron::format::replace_all(str, "//", "/");
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

  template<is_string T>
  static bool
  is_valid_string(const T &s) noexcept
  {
    return posix::verify(s);
  }

  path()
  {
    char buf[posix::path_max];
    if ( micron::realpath(".", buf) != nullptr )
      __p = buf;      // absolute cwd, matching the historical default
    else
      __p = path_t(".");
  }

  path(const char *name) : __p(prune(path_t(name ? name : "."))) { }

  template<is_string T> explicit path(const T &name) : __p(prune(path_t(name.c_str()))) { }

  path(const path &) = default;
  path(path &&) noexcept = default;
  path &operator=(const path &) = default;
  path &operator=(path &&) noexcept = default;

  const path_t &
  get() const noexcept
  {
    return __p;
  }

  path_t
  get_parent() const
  {
    return __parent_of(__p);
  }

  path_t
  basename() const
  {
    const char *s = __p.c_str();
    usize l = __p.size();
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
    return __parent_of(__p);
  }

  micron::vector<micron::sstr<posix::name_max + 1>>
  components() const
  {
    micron::vector<micron::sstr<posix::name_max + 1>> out;
    if ( __p.size() == 0 ) return out;
    const char *p = __p.c_str();
    usize n = __p.size();
    usize start = 0;
    for ( usize i = 0; i <= n; ++i ) {
      if ( p[i] == '/' || p[i] == '\0' ) {
        if ( i > start ) {
          micron::sstr<posix::name_max + 1> comp;
          for ( usize j = start; j < i; ++j ) comp += p[j];
          out.push_back(comp);
        }
        start = i + 1;
      }
    }
    return out;
  }

  usize
  depth() const noexcept
  {
    if ( __p.size() == 0 ) return 0;
    const char *p = __p.c_str();
    usize n = __p.size();
    usize d = 0;
    bool in_comp = false;
    for ( usize i = 0; i < n; ++i ) {
      if ( p[i] == '/' ) {
        in_comp = false;
      } else if ( !in_comp ) {
        in_comp = true;
        ++d;
      }
    }
    return d;
  }

  path_t
  join(const char *rel) const
  {
    return __join(__p, path_t(rel));
  }

  template<is_string T>
  path_t
  join(const T &rel) const
  {
    return join(rel.c_str());
  }

  path_t
  canonical() const
  {
    path_t out;
    if ( micron::realpath(__p.c_str(), &out[0]) == nullptr ) return path_t();
    out.adjust_size();
    return out;
  }

  path_t
  relative_to(const char *base) const
  {
    const char *p = __p.c_str();
    usize pl = __p.size();
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

  template<is_string T>
  path_t
  relative_to(const T &base) const
  {
    return relative_to(base.c_str());
  }

  path_t
  resolve(const char *cstr) const
  {
    return prune(path_t(cstr));
  }

  bool
  exists() const
  {
    return posix::exists(__p.c_str());
  }

  bool
  lexists() const
  {
    return posix::lexists(__p.c_str());
  }

  bool
  is_absolute() const noexcept
  {
    return __p.size() > 0 && __p[0] == '/';
  }

  bool
  is_relative() const noexcept
  {
    return !is_absolute();
  }

  bool
  is_mountpoint() const
  {
    return posix::is_mountpoint(__p.c_str());
  }

  bool
  is_root() const noexcept
  {
    return __p.size() == 1 && __p[0] == '/';
  }

  bool
  is_file() const
  {
    return posix::is_file(__p.c_str());
  }

  bool
  is_dir() const
  {
    return posix::is_dir(__p.c_str());
  }

  bool
  is_socket() const
  {
    return posix::is_socket(__p.c_str());
  }

  bool
  is_symlink() const
  {
    return posix::is_symlink(__p.c_str());
  }

  bool
  is_block_device() const
  {
    return posix::is_block_device(__p.c_str());
  }

  bool
  is_char_device() const
  {
    return posix::is_char_device(__p.c_str());
  }

  bool
  is_fifo() const
  {
    return posix::is_fifo(__p.c_str());
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
    return posix::is_dir(__child(name).c_str());
  }

  bool
  file(const char *name) const
  {
    return posix::is_file(__child(name).c_str());
  }

  bool
  socket(const char *name) const
  {
    return posix::is_socket(__child(name).c_str());
  }

  bool
  symlink(const char *name) const
  {
    return posix::is_symlink(__child(name).c_str());
  }

  bool
  block_device(const char *name) const
  {
    return posix::is_block_device(__child(name).c_str());
  }

  bool
  char_device(const char *name) const
  {
    return posix::is_char_device(__child(name).c_str());
  }

  bool
  fifo(const char *name) const
  {
    return posix::is_fifo(__child(name).c_str());
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
    return posix::exists(__child(name).c_str());
  }

  template<is_string T>
  bool
  directory(const T &s) const
  {
    return directory(s.c_str());
  }

  template<is_string T>
  bool
  file(const T &s) const
  {
    return file(s.c_str());
  }

  template<is_string T>
  bool
  socket(const T &s) const
  {
    return socket(s.c_str());
  }

  template<is_string T>
  bool
  symlink(const T &s) const
  {
    return symlink(s.c_str());
  }

  template<is_string T>
  bool
  block_device(const T &s) const
  {
    return block_device(s.c_str());
  }

  template<is_string T>
  bool
  char_device(const T &s) const
  {
    return char_device(s.c_str());
  }

  template<is_string T>
  bool
  fifo(const T &s) const
  {
    return fifo(s.c_str());
  }

  template<is_string T>
  bool
  pipe(const T &s) const
  {
    return pipe(s.c_str());
  }

  template<is_string T>
  bool
  device(const T &s) const
  {
    return device(s.c_str());
  }

  template<is_string T>
  bool
  node_exists(const T &s) const
  {
    return node_exists(s.c_str());
  }

  bool
  readable() const
  {
    return posix::is_readable(__p.c_str());
  }

  bool
  writable() const
  {
    return posix::is_writable(__p.c_str());
  }

  bool
  executable() const
  {
    return posix::is_executable(__p.c_str());
  }

  bool
  readable(const char *name) const
  {
    return posix::is_readable(__child(name).c_str());
  }

  bool
  writable(const char *name) const
  {
    return posix::is_writable(__child(name).c_str());
  }

  bool
  executable(const char *name) const
  {
    return posix::is_executable(__child(name).c_str());
  }

  template<is_string T>
  bool
  readable(const T &s) const
  {
    return readable(s.c_str());
  }

  template<is_string T>
  bool
  writable(const T &s) const
  {
    return writable(s.c_str());
  }

  template<is_string T>
  bool
  executable(const T &s) const
  {
    return executable(s.c_str());
  }

  linux_permissions
  permissions() const
  {
    return __perms_of(__p.c_str());
  }

  linux_permissions
  permissions(const char *name) const
  {
    return __perms_of(__child(name).c_str());
  }

  template<is_string T>
  linux_permissions
  permissions(const T &name) const
  {
    return permissions(name.c_str());
  }

  i32
  set_permissions(const linux_permissions &p)
  {
    return posix::chmod(__p.c_str(), p.full_mode());
  }

  i32
  set_permissions(const char *name, const linux_permissions &p)
  {
    return posix::chmod(__child(name).c_str(), p.full_mode());
  }

  template<is_string T>
  i32
  set_permissions(const T &name, const linux_permissions &p)
  {
    return set_permissions(name.c_str(), p);
  }

  bool
  owner_read() const
  {
    return __mode_bit(posix::s_irusr);
  }

  bool
  owner_write() const
  {
    return __mode_bit(posix::s_iwusr);
  }

  bool
  owner_exec() const
  {
    return __mode_bit(posix::s_ixusr);
  }

  bool
  group_read() const
  {
    return __mode_bit(posix::s_irgrp);
  }

  bool
  group_write() const
  {
    return __mode_bit(posix::s_iwgrp);
  }

  bool
  group_exec() const
  {
    return __mode_bit(posix::s_ixgrp);
  }

  bool
  other_read() const
  {
    return __mode_bit(posix::s_iroth);
  }

  bool
  other_write() const
  {
    return __mode_bit(posix::s_iwoth);
  }

  bool
  other_exec() const
  {
    return __mode_bit(posix::s_ixoth);
  }

  bool
  has_setuid() const
  {
    return __mode_bit(posix::s_isuid);
  }

  bool
  has_setgid() const
  {
    return __mode_bit(posix::s_isgid);
  }

  bool
  has_sticky() const
  {
    return __mode_bit(posix::s_isvtx);
  }

  posix::uid_t
  uid() const
  {
    return posix::get_uid(__p.c_str());
  }

  posix::gid_t
  gid() const
  {
    return posix::get_gid(__p.c_str());
  }

  bool
  owned_by(posix::uid_t u) const
  {
    return posix::is_owned_by(__p.c_str(), u);
  }

  bool
  in_group(posix::gid_t g) const
  {
    return posix::is_in_group(__p.c_str(), g);
  }

  i32
  chown(posix::uid_t u, posix::gid_t g)
  {
    return micron::fchownat(posix::at_fdcwd, __p.c_str(), u, g, 0);
  }

  i32
  chown(const char *name, posix::uid_t u, posix::gid_t g)
  {
    return micron::fchownat(posix::at_fdcwd, __child(name).c_str(), u, g, 0);
  }

  template<is_string T>
  i32
  chown(const T &name, posix::uid_t u, posix::gid_t g)
  {
    return chown(name.c_str(), u, g);
  }

  posix::off_t
  size() const
  {
    stat_t st{};
    return __stat(st) ? st.st_size : -1;
  }

  posix::ino_t
  inode() const
  {
    stat_t st{};
    return __stat(st) ? st.st_ino : 0;
  }

  posix::dev_t
  device_id() const
  {
    stat_t st{};
    return __stat(st) ? st.st_dev : 0;
  }

  posix::nlink_t
  link_count() const
  {
    stat_t st{};
    return __stat(st) ? st.st_nlink : 0;
  }

  time64_t
  atime() const
  {
    stat_t st{};
    return __stat(st) ? st.st_atime : 0;
  }

  time64_t
  mtime() const
  {
    stat_t st{};
    return __stat(st) ? st.st_mtime : 0;
  }

  time64_t
  ctime() const
  {
    stat_t st{};
    return __stat(st) ? st.st_ctime : 0;
  }

  posix::mode_t
  mode() const
  {
    stat_t st{};
    return __stat(st) ? st.st_mode : 0;
  }

  posix::off_t
  child_size(const char *name) const
  {
    stat_t st{};
    return posix::__impl::__stat(__child(name).c_str(), st) ? st.st_size : -1;
  }

  template<is_string T>
  posix::off_t
  child_size(const T &name) const
  {
    return child_size(name.c_str());
  }

  auto
  files() const
  {
    return __list([this](const posix::__impl_dir &e) { return file(e.d_name.c_str()); });
  }

  auto
  dirs() const
  {
    return __list([this](const posix::__impl_dir &e) { return directory(e.d_name.c_str()); });
  }

  auto
  all() const
  {
    return __list([](const posix::__impl_dir &) { return true; });
  }

  auto
  sockets() const
  {
    return __list([this](const posix::__impl_dir &e) { return socket(e.d_name.c_str()); });
  }

  auto
  symlinks() const
  {
    return __list([this](const posix::__impl_dir &e) { return symlink(e.d_name.c_str()); });
  }

  auto
  block_devices() const
  {
    return __list([this](const posix::__impl_dir &e) { return block_device(e.d_name.c_str()); });
  }

  auto
  char_devices() const
  {
    return __list([this](const posix::__impl_dir &e) { return char_device(e.d_name.c_str()); });
  }

  auto
  fifos() const
  {
    return __list([this](const posix::__impl_dir &e) { return fifo(e.d_name.c_str()); });
  }

  void
  up()
  {
    __p = __parent_of(__p);
  }

  void
  down(const char *name)
  {
    path_t child = __child(name);
    if ( !posix::is_dir(child.c_str()) ) exc<except::io_error>("io::path::down(), child is not a directory.");
    __p = micron::move(child);
  }

  template<is_string T>
  void
  down(const T &name)
  {
    down(name.c_str());
  }

  void
  to_root()
  {
    __p = path_t("/");
  }

  void
  set(const char *str)
  {
    __p = prune(path_t(str ? str : "."));
  }

  template<is_string T>
  void
  set(const T &str)
  {
    set(str.c_str());
  }

  i32
  chdir() const
  {
    if ( !posix::is_dir(__p.c_str()) ) return -1;
    dir d(__p);      // scoped fd; fchdir then close
    return d.chdir();
  }

  bool
  is_same_as(const path &other) const
  {
    return posix::is_same_file(__p.c_str(), other.__p.c_str());
  }

  bool
  is_same_as(const char *other) const
  {
    return posix::is_same_file(__p.c_str(), other);
  }

  template<is_string T>
  bool
  is_same_as(const T &other) const
  {
    return is_same_as(other.c_str());
  }

  bool
  operator==(const path &o) const noexcept
  {
    return __p == o.__p;
  }

  bool
  operator!=(const path &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const path &o) const noexcept
  {
    const char *a = __p.c_str();
    const char *b = o.__p.c_str();
    usize al = __p.size();
    usize bl = o.__p.size();
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
    return __p == s;
  }

  bool
  operator!=(const char *s) const noexcept
  {
    return __p != s;
  }

  template<is_string T>
  bool
  operator==(const T &s) const noexcept
  {
    return __p == s;
  }

  template<is_string T>
  bool
  operator!=(const T &s) const noexcept
  {
    return __p != s;
  }

  path_t
  operator/(const char *rel) const
  {
    return join(rel);
  }

  template<is_string T>
  path_t
  operator/(const T &rel) const
  {
    return join(rel.c_str());
  }
};

};      // namespace io
};      // namespace micron
