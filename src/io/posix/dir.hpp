//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "iosys.hpp"

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../string/strings.hpp"
#include "../../tuple.hpp"

#include "../../linux/io/inode.hpp"
#include "../../linux/sys/fcntl.hpp"
#include "../../linux/sys/ioctl.hpp"
#include "../bits.hpp"

#include "../../linux/io.hpp"
#include "../../vector/vector.hpp"

#include "file.hpp"

namespace micron
{
namespace io
{
struct pred_name {
  micron::sstr<posix::name_max + 1> _name;

  explicit pred_name(const char *n) : _name(n) {}

  template <is_string T> explicit pred_name(const T &n) : _name(n.c_str()) {}

  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.d_name == _name;
  }
};

struct pred_ext {
  micron::sstr<64> _ext;

  explicit pred_ext(const char *e) : _ext(e[0] == '.' ? e + 1 : e) {}

  template <is_string T> explicit pred_ext(const T &e) : _ext(e.size() && e[0] == '.' ? e.c_str() + 1 : e.c_str()) {}

  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    const char *n = e.d_name.c_str();
    usize nl = e.d_name.size();
    usize el = _ext.size();
    if ( el == 0 || nl <= el ) return false;
    if ( n[nl - el - 1] != '.' ) return false;
    const char *s = n + nl - el;
    const char *ex = _ext.c_str();
    for ( usize i = 0; i < el; ++i )
      if ( s[i] != ex[i] ) return false;
    return true;
  }
};

struct pred_name_contains {
  micron::sstr<posix::name_max + 1> _sub;

  explicit pred_name_contains(const char *s) : _sub(s) {}

  template <is_string T> explicit pred_name_contains(const T &s) : _sub(s.c_str()) {}

  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    const char *hay = e.d_name.c_str();
    const char *ndl = _sub.c_str();
    usize hl = e.d_name.size();
    usize nl = _sub.size();
    if ( nl == 0 ) return true;
    if ( nl > hl ) return false;
    for ( usize i = 0; i <= hl - nl; ++i ) {
      bool ok = true;
      for ( usize j = 0; j < nl; ++j ) {
        if ( hay[i + j] != ndl[j] ) {
          ok = false;
          break;
        }
      }
      if ( ok ) return true;
    }
    return false;
  }
};

struct pred_size_eq {
  posix::off_t _v;

  explicit pred_size_eq(posix::off_t v) : _v(v) {}

  bool
  operator()(posix::off_t s) const noexcept
  {
    return s == _v;
  }
};

struct pred_size_gt {
  posix::off_t _v;

  explicit pred_size_gt(posix::off_t v) : _v(v) {}

  bool
  operator()(posix::off_t s) const noexcept
  {
    return s > _v;
  }
};

struct pred_size_lt {
  posix::off_t _v;

  explicit pred_size_lt(posix::off_t v) : _v(v) {}

  bool
  operator()(posix::off_t s) const noexcept
  {
    return s < _v;
  }
};

struct pred_size_between {
  posix::off_t _lo, _hi;

  pred_size_between(posix::off_t lo, posix::off_t hi) : _lo(lo), _hi(hi) {}

  bool
  operator()(posix::off_t s) const noexcept
  {
    return s >= _lo && s <= _hi;
  }
};

struct pred_type_reg {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_reg();
  }
};

struct pred_type_dir {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_dir();
  }
};

struct pred_type_link {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_lnk();
  }
};

struct pred_type_fifo {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_fifo();
  }
};

struct pred_type_sock {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_sock();
  }
};

struct pred_type_chr {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_chr();
  }
};

struct pred_type_blk {
  bool
  operator()(const posix::__impl_dir &e) const noexcept
  {
    return e.is_blk();
  }
};

inline pred_name
by_name(const char *n)
{
  return pred_name{ n };
}

inline pred_ext
by_ext(const char *e)
{
  return pred_ext{ e };
}

inline pred_name_contains
by_name_contains(const char *s)
{
  return pred_name_contains{ s };
}

inline pred_size_eq
by_size(posix::off_t sz)
{
  return pred_size_eq{ sz };
}

inline pred_size_gt
by_size_gt(posix::off_t sz)
{
  return pred_size_gt{ sz };
}

inline pred_size_lt
by_size_lt(posix::off_t sz)
{
  return pred_size_lt{ sz };
}

inline pred_size_between
by_size_between(posix::off_t lo, posix::off_t hi)
{
  return pred_size_between{ lo, hi };
}

inline pred_type_reg
by_type_file(void)
{
  return {};
}

inline pred_type_dir
by_type_dir(void)
{
  return {};
}

inline pred_type_link
by_type_link(void)
{
  return {};
}

inline pred_type_fifo
by_type_fifo(void)
{
  return {};
}

inline pred_type_sock
by_type_sock(void)
{
  return {};
}

inline pred_type_chr
by_type_chr(void)
{
  return {};
}

inline pred_type_blk
by_type_blk(void)
{
  return {};
}

template <is_string T>
inline pred_name
by_name(const T &n)
{
  return pred_name{ n };
}

template <is_string T>
inline pred_ext
by_ext(const T &e)
{
  return pred_ext{ e };
}

template <is_string T>
inline pred_name_contains
by_name_contains(const T &s)
{
  return pred_name_contains{ s };
}

template <typename P>
concept entry_predicate = requires(P p, const posix::__impl_dir &e) {
  { p(e) } -> same_as<bool>;
};

template <typename P>
concept size_predicate = requires(P p, posix::off_t sz) {
  { p(sz) } -> same_as<bool>;
};

struct dir : public file {

  micron::vector<posix::__impl_dir> _entries;

  posix::readdir_ctx _ctx{};

  ~dir() = default;

  dir() : file(), _entries() {}

  dir(const char *str) { __open_dir(str); }

  dir(const micron::sstr<max_name + 1> &str) { __open_dir(str.c_str()); }

  dir(const micron::string &str) { __open_dir(str.c_str()); }

  template <is_string T> dir(const T &str) { __open_dir(str.c_str()); }

  dir(const dir &o) : file(o), _entries(o._entries), _ctx{} {}

  dir(dir &&o) noexcept : file(micron::move(o)), _entries(micron::move(o._entries)), _ctx(o._ctx) { micron::zero(&o._ctx); }

  dir &
  operator=(dir &&o) noexcept
  {
    file::operator=(micron::move(o));
    _entries = micron::move(o._entries);
    _ctx = o._ctx;
    micron::zero(&o._ctx);
    return *this;
  }

  dir &
  operator=(const dir &o)
  {
    file::operator=(o);
    _entries = o._entries;
    _ctx = {};
    return *this;
  }

  const posix::__impl_dir &
  operator[](usize i) const noexcept
  {
    return _entries[i];
  }

  posix::__impl_dir &
  operator[](usize i) noexcept
  {
    return _entries[i];
  }

  usize
  count(void) const noexcept
  {
    return _entries.size();
  }

  bool
  empty(void) const noexcept
  {
    return _entries.size() == 0;
  }

  auto
  begin() noexcept
  {
    return _entries.begin();
  }

  auto
  end() noexcept
  {
    return _entries.end();
  }

  auto
  begin(void) const noexcept
  {
    return _entries.begin();
  }

  auto
  end(void) const noexcept
  {
    return _entries.end();
  }

  posix::__impl_dir
  next(void)
  {
    __alive();
    return posix::readdir_r(__handle, _ctx);
  }

  void
  rewind(void)
  {
    if ( __handle.closed() ) return;
    posix::seek_to(__handle, 0);
    micron::zero(&_ctx);
  }

  template <typename Fn>
  void
  for_each(Fn &&fn)
  {
    if ( fname.size() ) posix::for_each_entry(fname.c_str(), fn);
  }

  template <typename Fn>
  void
  for_each_entry(Fn &&fn)
  {
    get_children();
    for ( auto &e : _entries ) fn(e);
  }

  template <typename Fn>
  void
  for_each_while(Fn &&fn)
  {
    get_children();
    for ( auto &e : _entries ) {
      if ( !fn(e) ) break;
    }
  }

  const micron::vector<posix::__impl_dir> &
  list(void)
  {
    __alive();
    _entries.clear();
    micron::zero(&_ctx);
    _read_entries();
    return _entries;
  }

  const micron::vector<posix::__impl_dir> &
  get_children(void)
  {
    __alive();
    if ( _entries.empty() ) _read_entries();
    return _entries;
  }

  void
  refresh(void)
  {
    __alive();
    _entries.clear();
    micron::zero(&_ctx);
    _read_entries();
  }

  template <entry_predicate Pred>
  micron::vector<posix::__impl_dir>
  match(Pred &&pred)
  {
    get_children();
    micron::vector<posix::__impl_dir> out;
    for ( const auto &e : _entries )
      if ( pred(e) ) out.push_back(e);
    return out;
  }

  template <size_predicate Pred>
  micron::vector<posix::__impl_dir>
  match_stat(Pred &&pred)
  {
    __alive();
    get_children();
    micron::vector<posix::__impl_dir> out;
    for ( const auto &e : _entries ) {
      if ( !e.is_reg() ) continue;
      stat_t b{};
      if ( !posix::__impl::__fstatat(__handle, e.d_name.c_str(), b) ) continue;
      if ( pred(b.st_size) ) out.push_back(e);
    }
    return out;
  }

  micron::vector<posix::__impl_dir>
  match_name(const char *name)
  {
    return match(by_name(name));
  }

  micron::vector<posix::__impl_dir>
  match_ext(const char *ext)
  {
    return match(by_ext(ext));
  }

  micron::vector<posix::__impl_dir>
  match_size(posix::off_t sz)
  {
    return match_stat(by_size(sz));
  }

  template <is_string T>
  micron::vector<posix::__impl_dir>
  match_name(const T &s)
  {
    return match_name(s.c_str());
  }

  template <is_string T>
  micron::vector<posix::__impl_dir>
  match_ext(const T &s)
  {
    return match_ext(s.c_str());
  }

  bool
  is_empty_dir(void)
  {
    __alive();
    return posix::is_empty_dir(__handle);
  }

  bool
  contains(const char *name)
  {
    get_children();
    for ( const auto &e : _entries )
      if ( e.d_name == name ) return true;
    return false;
  }

  template <is_string T>
  bool
  contains(const T &name)
  {
    return contains(name.c_str());
  }

  dir
  up(void) const
  {
    __alive_dir();
    if ( fname.size() == 0 ) exc<except::io_error>("micron::dir::up(), no path stored.");

    const char *p = fname.c_str();
    usize n = fname.size();

    while ( n > 1 && p[n - 1] == '/' ) --n;

    usize sep = n;
    while ( sep > 0 && p[sep - 1] != '/' ) --sep;

    char parent[posix::path_max];

    if ( sep == 0 ) {
      parent[0] = '.';
      parent[1] = '\0';
    } else if ( sep == 1 ) {
      parent[0] = '/';
      parent[1] = '\0';
    } else {
      usize pl = sep - 1;
      for ( usize i = 0; i < pl; ++i ) parent[i] = p[i];
      parent[pl] = '\0';
    }

    return dir{ parent };
  }

  dir
  to_root(void) const
  {
    return dir{ "/" };
  }

  dir
  down(const char *name) const
  {
    __alive_dir();
    if ( fname.size() == 0 ) exc<except::io_error>("micron::dir::down(), no path stored.");
    if ( !child_is_dir(name) ) exc<except::io_error>("micron::dir::down(), child is not a directory.");

    const char *base = fname.c_str();
    usize bl = fname.size();
    usize nl = 0;
    while ( name[nl] ) ++nl;

    if ( bl + 1 + nl >= posix::path_max ) exc<except::io_error>("micron::dir::down(), path too long.");

    char child[posix::path_max];
    for ( usize i = 0; i < bl; ++i ) child[i] = base[i];

    if ( bl == 1 && base[0] == '/' ) {

      for ( usize i = 0; i < nl; ++i ) child[1 + i] = name[i];
      child[1 + nl] = '\0';
    } else {
      child[bl] = '/';
      for ( usize i = 0; i < nl; ++i ) child[bl + 1 + i] = name[i];
      child[bl + 1 + nl] = '\0';
    }

    return dir{ child };
  }

  template <is_string T>
  dir
  down(const T &name) const
  {
    return down(name.c_str());
  }

  dir
  down_first(void)
  {
    get_children();
    for ( const auto &e : _entries )
      if ( e.is_dir() ) return down(e.d_name.c_str());
    exc<except::io_error>("micron::dir::down_first(), no subdirectory children.");
    __builtin_unreachable();
  }

  micron::vector<micron::sstr<posix::name_max + 1>>
  path_components(void) const
  {
    micron::vector<micron::sstr<posix::name_max + 1>> out;
    if ( fname.size() == 0 ) return out;
    const char *p = fname.c_str();
    usize n = fname.size();
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
  depth(void) const noexcept
  {
    if ( fname.size() == 0 ) return 0;
    const char *p = fname.c_str();
    usize n = fname.size();
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

  bool
  child_is_file(const char *n) const
  {
    __alive_dir();
    return posix::is_file_at(__handle.fd, n);
  }

  bool
  child_is_dir(const char *n) const
  {
    __alive_dir();
    return posix::is_dir_at(__handle.fd, n);
  }

  bool
  child_is_link(const char *n) const
  {
    __alive_dir();
    return posix::is_symlink_at(__handle.fd, n);
  }

  bool
  child_is_fifo(const char *n) const
  {
    __alive_dir();
    return posix::is_fifo_at(__handle.fd, n);
  }

  bool
  child_is_socket(const char *n) const
  {
    __alive_dir();
    return posix::is_socket_at(__handle.fd, n);
  }

  bool
  child_is_block_device(const char *n) const
  {
    __alive_dir();
    return posix::is_block_device_at(__handle.fd, n);
  }

  bool
  child_is_char_device(const char *n) const
  {
    __alive_dir();
    return posix::is_char_device_at(__handle.fd, n);
  }

  bool
  child_exists(const char *n) const
  {
    __alive_dir();
    return posix::exists_at(__handle, n);
  }

  bool
  child_is_readable(const char *n) const
  {
    __alive_dir();
    return posix::is_readable_at(__handle, n);
  }

  bool
  child_is_writable(const char *n) const
  {
    __alive_dir();
    return posix::is_writable_at(__handle, n);
  }

  bool
  child_is_executable(const char *n) const
  {
    __alive_dir();
    return posix::is_executable_at(__handle, n);
  }

  template <is_string T>
  bool
  child_is_file(const T &s) const
  {
    return child_is_file(s.c_str());
  }

  template <is_string T>
  bool
  child_is_dir(const T &s) const
  {
    return child_is_dir(s.c_str());
  }

  template <is_string T>
  bool
  child_is_link(const T &s) const
  {
    return child_is_link(s.c_str());
  }

  template <is_string T>
  bool
  child_is_fifo(const T &s) const
  {
    return child_is_fifo(s.c_str());
  }

  template <is_string T>
  bool
  child_is_socket(const T &s) const
  {
    return child_is_socket(s.c_str());
  }

  template <is_string T>
  bool
  child_is_block_device(const T &s) const
  {
    return child_is_block_device(s.c_str());
  }

  template <is_string T>
  bool
  child_is_char_device(const T &s) const
  {
    return child_is_char_device(s.c_str());
  }

  template <is_string T>
  bool
  child_exists(const T &s) const
  {
    return child_exists(s.c_str());
  }

  template <is_string T>
  bool
  child_is_readable(const T &s) const
  {
    return child_is_readable(s.c_str());
  }

  template <is_string T>
  bool
  child_is_writable(const T &s) const
  {
    return child_is_writable(s.c_str());
  }

  template <is_string T>
  bool
  child_is_executable(const T &s) const
  {
    return child_is_executable(s.c_str());
  }

  posix::off_t
  child_size(const char *name) const
  {
    __alive_dir();
    stat_t b{};
    return posix::__impl::__fstatat(__handle, name, b) ? b.st_size : -1;
  }

  linux_permissions
  child_permissions(const char *name) const
  {
    __alive_dir();
    stat_t b{};
    if ( !posix::__impl::__fstatat(__handle, name, b) ) return linux_permissions::none();
    linux_permissions p = linux_permissions::from_mode(b.st_mode);
    p.setuid = (b.st_mode & posix::s_isuid) != 0;
    p.setgid = (b.st_mode & posix::s_isgid) != 0;
    p.sticky = (b.st_mode & posix::s_isvtx) != 0;
    return p;
  }

  template <is_string T>
  posix::off_t
  child_size(const T &s) const
  {
    return child_size(s.c_str());
  }

  template <is_string T>
  linux_permissions
  child_permissions(const T &s) const
  {
    return child_permissions(s.c_str());
  }

  posix::fd_t
  open_child(const char *name, i32 flags = posix::o_rdonly | posix::o_cloexec, u32 mode = 0)
  {
    __alive();
    return posix::open_at(__handle, name, flags, mode);
  }

  template <is_string T>
  posix::fd_t
  open_child(const T &name, i32 flags = posix::o_rdonly | posix::o_cloexec, u32 mode = 0)
  {
    return open_child(name.c_str(), flags, mode);
  }

  i32
  make_child_dir(const char *name, const linux_permissions &p = perm_dir_default)
  {
    __alive();
    i32 r = posix::mkdir_at(__handle, name, p.to_mode());
    if ( r == 0 ) refresh();
    return r;
  }

  i32
  remove_child(const char *name)
  {
    __alive();
    i32 r = (micron::unlinkat(__handle.fd, name, 0));
    if ( r == 0 ) refresh();
    return r;
  }

  i32
  remove_child_dir(const char *name)
  {
    __alive();
    i32 r = (micron::unlinkat(__handle.fd, name, posix::at_removedir));
    if ( r == 0 ) refresh();
    return r;
  }

  i32
  rename_child(const char *oldname, const char *newname)
  {
    __alive();
    i32 r = (posix::renameat(__handle, oldname, __handle, newname));
    if ( r == 0 ) refresh();
    return r;
  }

  i32
  chmod_child(const char *name, const linux_permissions &p)
  {
    __alive();
    return (posix::fchmodat(__handle, name, p.full_mode(), 0));
  }

  i32
  chmod_child(const char *name, posix::mode_t mode)
  {
    __alive();
    return (posix::fchmodat(__handle, name, mode, 0));
  }

  i32
  chown_child(const char *name, posix::uid_t uid, posix::gid_t gid, bool follow_links = true)
  {
    __alive();
    i32 flags = follow_links ? 0 : posix::at_symlink_nofollow;
    return (posix::fchownat(__handle, name, uid, gid, flags));
  }

  i32
  symlink_child(const char *target, const char *linkname)
  {
    __alive();
    i32 r = (posix::symlinkat(target, __handle, linkname));
    if ( r == 0 ) refresh();
    return r;
  }

  i32
  link_child(const char *oldname, const char *newname)
  {
    __alive();
    i32 r = (posix::linkat(__handle, oldname, __handle, newname, 0));
    if ( r == 0 ) refresh();
    return r;
  }

  template <is_string T>
  i32
  make_child_dir(const T &n, const linux_permissions &p = perm_dir_default)
  {
    return make_child_dir(n.c_str(), p);
  }

  template <is_string T>
  i32
  remove_child(const T &s)
  {
    return remove_child(s.c_str());
  }

  template <is_string T>
  i32
  remove_child_dir(const T &s)
  {
    return remove_child_dir(s.c_str());
  }

  template <is_string T, is_string U>
  i32
  rename_child(const T &o, const U &n)
  {
    return rename_child(o.c_str(), n.c_str());
  }

  template <is_string T>
  i32
  chmod_child(const T &n, const linux_permissions &p)
  {
    return chmod_child(n.c_str(), p);
  }

  template <is_string T>
  i32
  chmod_child(const T &n, posix::mode_t m)
  {
    return chmod_child(n.c_str(), m);
  }

  template <is_string T>
  i32
  chown_child(const T &n, posix::uid_t u, posix::gid_t g, bool f = true)
  {
    return chown_child(n.c_str(), u, g, f);
  }

  template <is_string T, is_string U>
  i32
  symlink_child(const T &t, const U &l)
  {
    return symlink_child(t.c_str(), l.c_str());
  }

  template <is_string T, is_string U>
  i32
  link_child(const T &o, const U &n)
  {
    return link_child(o.c_str(), n.c_str());
  }

  i32
  chdir(void) const
  {
    __alive_dir();
    return (posix::fchdir(__handle));
  }

  static dir
  create(const char *path, const linux_permissions &p = perm_dir_default)
  {
    posix::mkdir(path, p.to_mode());
    return dir{ path };
  }

  template <is_string T>
  static dir
  create(const T &path, const linux_permissions &p = perm_dir_default)
  {
    return create(path.c_str(), p);
  }

  static dir
  create_all(const char *path, const linux_permissions &p = perm_dir_default)
  {
    posix::mkdir_p(path, p.to_mode());
    return dir{ path };
  }

  template <is_string T>
  static dir
  create_all(const T &path, const linux_permissions &p = perm_dir_default)
  {
    return create_all(path.c_str(), p);
  }

private:
  inline void
  __alive_dir(void) const
  {
    if ( __handle.fd == posix::invalid_fd ) exc<except::io_error>("micron::dir, fd isn't open.");
  }

  void
  _read_entries(void)
  {
    posix::readdir_ctx ctx{};
    posix::seek_to(__handle, 0);
    for ( ;; ) {
      posix::__impl_dir e = posix::readdir_r(__handle, ctx);
      if ( e.at_end() ) break;
      if ( posix::is_dot_entry(e.d_name.c_str()) ) continue;
      _entries.push_back(e);
    }
  }

  inline void
  __open_dir(const char *str)
  {
    if ( !posix::verify(str) ) exc<except::io_error>("error in creating micron::dir, malformed string.");
    if ( !posix::exists(str) ) exc<except::io_error>("micron::dir, path doesn't exist.");
    if ( !posix::is_dir(str) ) exc<except::io_error>("micron::dir, path isn't a directory.");

    __handle.fd = (posix::open(str, posix::o_rdonly | posix::o_directory | posix::o_cloexec));
    if ( auto r = __handle.has_error() ) exc_e<except::io_error>(r, "micron::dir, failed to open.");

    fname = str;
    micron::zero(&sd);
  }
};

inline dir
open_dir(const char *path)
{
  return dir{ path };
}

template <is_string T>
inline dir
open_dir(const T &path)
{
  return dir{ path.c_str() };
}

inline dir
create_dir(const char *path, const linux_permissions &p = perm_dir_default)
{
  return dir::create(path, p);
}

template <is_string T>
inline dir
create_dir(const T &path, const linux_permissions &p = perm_dir_default)
{
  return dir::create(path.c_str(), p);
}

inline dir
create_dirs(const char *path, const linux_permissions &p = perm_dir_default)
{
  return dir::create_all(path, p);
}

template <is_string T>
inline dir
create_dirs(const T &path, const linux_permissions &p = perm_dir_default)
{
  return dir::create_all(path.c_str(), p);
}

};     // namespace io
};     // namespace micron
