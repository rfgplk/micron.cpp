//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../string/strings.hpp"
#include "io.hpp"

#include "paths.hpp"
#include "posix/file.hpp"

#include "file.hpp"
#include "filesystem.hpp"      // shares __max_fs sentinel

#include "../mutex/locks.hpp"

namespace micron
{
namespace fsys
{
template<io::modes _default_mode = io::modes::read, usize N = 256> class parallel_system
{
  mutable micron::mutex __mtx;
  micron::unique_pointer<fsys::file<>> entries[N];
  usize sz;

  inline __attribute__((always_inline)) void
  __limit()
  {
    if ( sz == N ) exc<except::filesystem_error>("micron::fsys too many file handles open");
  }

  int
  __find_fd(const io::path_t &p)
  {
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == p ) return entries[i]->get_fd();
    return -1;
  }

  usize
  __find_id(const io::path_t &p)
  {
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == p ) return i;
    return __max_fs;
  }

  fsys::file<> &
  __append(const io::path_t &p, const io::modes mode)
  {
    __limit();
    entries[sz] = new fsys::file<>(p.c_str(), mode);
    return *entries[sz++];
  }

  fsys::file<> &
  __get_or_open(const io::path_t &p, const io::modes mode)
  {
    usize id = __find_id(p);
    if ( id != __max_fs ) return *entries[id];
    return __append(p, mode);
  }

  int
  __fd_or_open(const io::path_t &p)
  {
    int f = __find_fd(p);
    if ( f == -1 ) f = __get_or_open(p, _default_mode).get_fd();
    return f;
  }

  inline __attribute__((always_inline)) void
  __set_perms(io::linux_permissions &perms, io::permission_types x)
  {
    if ( x == io::permission_types::owner_read )
      perms.owner.read = true;
    else if ( x == io::permission_types::owner_write )
      perms.owner.write = true;
    else if ( x == io::permission_types::owner_execute )
      perms.owner.execute = true;
    else if ( x == io::permission_types::group_read )
      perms.group.read = true;
    else if ( x == io::permission_types::group_write )
      perms.group.write = true;
    else if ( x == io::permission_types::group_execute )
      perms.group.execute = true;
    else if ( x == io::permission_types::others_read )
      perms.others.read = true;
    else if ( x == io::permission_types::others_write )
      perms.others.write = true;
    else if ( x == io::permission_types::others_execute )
      perms.others.execute = true;
  }

public:
  ~parallel_system()
  {
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i] ) entries[i]->sync();
  }

  parallel_system() : sz(0) { }

  parallel_system(const io::path_t &p, const io::modes c = _default_mode) : sz(0) { file(p, c); }

  template<typename... T>
    requires((micron::same_as<T, io::path_t> && ...))
  parallel_system(const T &...t) : sz(0)
  {
    (file(t, _default_mode), ...);
  }

  parallel_system(const parallel_system &) = delete;
  parallel_system &operator=(const parallel_system &) = delete;

  parallel_system(parallel_system &&o) noexcept : sz(o.sz)
  {
    for ( usize i = 0; i < N; i++ ) entries[i] = micron::move(o.entries[i]);
    o.sz = 0;
  }

  parallel_system &
  operator=(parallel_system &&o) noexcept
  {
    if ( this == &o ) return *this;
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i] ) entries[i]->sync();
    for ( usize i = 0; i < N; i++ ) entries[i] = micron::move(o.entries[i]);
    sz = o.sz;
    o.sz = 0;
    return *this;
  }

  auto &
  operator[](const io::path_t &p, const io::modes c = _default_mode, const posix::node_types nd = posix::node_types::regular_file)
  {
    micron::lock_guard g(__mtx);
    if ( nd != posix::node_types::regular_file ) exc<except::filesystem_error>("micron::fsys[] path wasn't a file");
    return __get_or_open(p, c);
  }

  inline auto &
  append(const io::path_t &p, const io::modes c = _default_mode, const posix::node_types nd = posix::node_types::regular_file)
  {
    micron::lock_guard g(__mtx);
    if ( nd != posix::node_types::regular_file ) exc<except::filesystem_error>("micron::fsys[] path wasn't a file");
    return __append(p, c);
  }

  inline void
  remove(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    for ( usize i = 0; i < sz; i++ ) {
      if ( entries[i]->name() == p ) {
        entries[i]->sync();
        for ( usize j = i + 1; j < sz; j++ ) entries[j - 1] = micron::move(entries[j]);
        --sz;
        entries[sz].reset();
        return;
      }
    }
  }

  inline void
  remove(fsys::file<> &fref)
  {
    micron::lock_guard g(__mtx);
    for ( usize i = 0; i < sz; i++ ) {
      if ( *entries[i] == fref ) {
        entries[i]->sync();
        for ( usize j = i + 1; j < sz; j++ ) entries[j - 1] = micron::move(entries[j]);
        --sz;
        entries[sz].reset();
        return;
      }
    }
  }

  void to_persist() = delete;

  auto
  list(void) const
  {
    micron::lock_guard g(__mtx);
    micron::vector<micron::sstr<io::max_name>> names;
    for ( usize i = 0; i < sz; i++ ) names.push_back(entries[i]->name());
    return names;
  }

  auto &
  file(const io::path_t &p, const io::modes mode = _default_mode)
  {
    micron::lock_guard g(__mtx);
    return __append(p, mode);
  }

  void
  rename(const io::path_t &from, const io::path_t &to)
  {
    micron::lock_guard g(__mtx);
    __get_or_open(from, _default_mode);
    __get_or_open(to, _default_mode);
    posix::rename(from.c_str(), to.c_str());
  }

  // provide both
  void
  move(const io::path_t &from, const io::path_t &to)
  {
    micron::lock_guard g(__mtx);
    __get_or_open(from, _default_mode);
    __get_or_open(to, _default_mode);
    posix::rename(from.c_str(), to.c_str());
  }

  void
  copy(const io::path_t &from, const io::path_t &to)
  {
    micron::lock_guard g(__mtx);
    fsys::file<> &f = __get_or_open(from, _default_mode);
    fsys::file<> &t = __get_or_open(to, io::modes::readwritecreate);
    micron::string buf;      // fsys::file<> is char-based (T = micron::string)
    f.operator>>(buf);
    t.operator=(buf);
    t.sync();
  }

  template<typename... Paths>
  void
  copy_list(const io::path_t &from, const Paths &...to)
  {
    (copy(from, to), ...);
  }

  // compatibility helpers
  bool
  is_opened(const io::path_t &path) const
  {
    micron::lock_guard g(__mtx);
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == path ) return true;
    return false;
  }

  bool
  equivalent(const io::path_t &path, const io::path_t &cmp) const
  {
    return path == cmp;
  }

  bool
  exists(const io::path_t &path) const
  {
    return (posix::access(path.c_str(), posix::access_ok) == 0);
  }

  bool
  accessible(const io::path_t &path) const
  {
    return (posix::access(path.c_str(), posix::read_ok | posix::execute_ok) == 0);
  }

  auto
  permissions(const io::path_t &path)
  {
    micron::lock_guard g(__mtx);
    return __get_or_open(path, _default_mode).permissions();
  }

  auto
  set_permissions(const io::path_t &path, const io::linux_permissions &perms)
  {
    micron::lock_guard g(__mtx);
    return __get_or_open(path, _default_mode).set_permissions(perms);
  }

  template<typename... Args>
  auto
  set_permissions(const io::path_t &path, Args... args)
  {
    micron::lock_guard g(__mtx);
    io::linux_permissions perms
        = { { false, false, false }, { false, false, false }, { false, false, false } };      // explicitly init to zero
    (__set_perms(perms, args), ...);                                                          // was __set_perm (undeclared)
    return __get_or_open(path, _default_mode).set_permissions(perms);
  }

  auto
  file_type_at(const io::path_t &p) const
  {
    return posix::get_type_at(p.c_str());
  }

  auto
  file_type(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    if ( f == -1 ) return posix::node_types::not_found;
    return posix::get_type(f);
  }

  bool
  is_virtual_file(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_virtual_file(f);
  }

  bool
  is_regular_file(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_file(f);
  }

  bool
  is_block_device(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_block_device(f);
  }

  bool
  is_directory(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_dir(f);
  }

  bool
  is_socket(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_socket(f);
  }

  bool
  is_symlink(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_symlink(f);
  }

  bool
  is_fifo(const io::path_t &p)
  {
    micron::lock_guard g(__mtx);
    int f = __fd_or_open(p);
    return f != -1 && posix::is_fifo(f);
  }

  // queries on the most-recently-opened handle
  bool
  is_regular_file(void) const
  {
    micron::lock_guard g(__mtx);
    return sz != 0 && !!entries[sz - 1] && posix::is_file(entries[sz - 1]->get_fd());
  }

  bool
  is_block_device(void) const
  {
    micron::lock_guard g(__mtx);
    return sz != 0 && !!entries[sz - 1] && posix::is_block_device(entries[sz - 1]->get_fd());
  }

  bool
  is_directory(void) const
  {
    micron::lock_guard g(__mtx);
    return sz != 0 && !!entries[sz - 1] && posix::is_dir(entries[sz - 1]->get_fd());
  }

  bool
  is_socket(void) const
  {
    micron::lock_guard g(__mtx);
    return sz != 0 && !!entries[sz - 1] && posix::is_socket(entries[sz - 1]->get_fd());
  }

  bool
  is_symlink(void) const
  {
    micron::lock_guard g(__mtx);
    return sz != 0 && !!entries[sz - 1] && posix::is_symlink(entries[sz - 1]->get_fd());
  }

  bool
  is_fifo(void) const
  {
    micron::lock_guard g(__mtx);
    return sz != 0 && !!entries[sz - 1] && posix::is_fifo(entries[sz - 1]->get_fd());
  }
};

};      // namespace fsys
};      // namespace micron
