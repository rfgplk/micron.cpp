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
#include "posix/utils.hpp"

#include "entry.hpp"
#include "file.hpp"

#include "../mutex/locks.hpp"

namespace micron
{
namespace fsys
{
struct __parallel_wrapper {
  fsys::file<> _fentry;
  micron::mutex mtx;
};

// main class for handling all system functions
// not thread safe
template <io::modes _default_mode = io::modes::read,
          size_t N = 256>     // max size of open handles (entries & dirs), note that the max is usually 1024, but
                              // defaulting to 256
class parallel_system
{
  micron::mutex __mtx;
  micron::weak_pointer<__parallel_wrapper> entries[N];
  inline __attribute__((always_inline)) auto &
  __access(__parallel_wrapper &pw)
  {
    return pw._fentry;
  }
  size_t sz;
  auto
  __find_fd(const io::path_t &p) -> int
  {
    for ( size_t i = 0; i < sz; i++ ) {
      micron::lock_guard(entries[i]->mtx);
      if ( __access(*entries[i]).name() == p ) {
        return __access(*entries[i]).get_fd();
      }
    }
    return -1;
  }
  auto
  __find_id(const io::path_t &p) -> size_t
  {
    for ( size_t i = 0; i < sz; i++ ) {
      micron::lock_guard(entries[i]->mtx);
      if ( __access(*entries[i]).name() == p )
        return i;
    }
    return __max_fs;
  }
  auto &
  __find(const io::path_t &p)
  {
    for ( size_t i = 0; i < sz; i++ ) {
      micron::lock_guard(entries[i]->mtx);
      if ( __access(*entries[i]).name() == p )
        return __access(*entries[i]);
    }
    exc<except::filesystem_error>("micron fsys wasn't able to find file");
  }
  inline __attribute__((always_inline)) size_t
  __locate(const io::path_t &p)
  {
    auto _id = __find_id(p);
    if ( _id == __max_fs ) {
      this->operator[](p);
      _id = sz - 1;
    }
    return _id;
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
  inline __attribute__((always_inline)) void
  __limit()
  {
    if ( sz == N )
      exc<except::filesystem_error>("micron::fsys too many file handles open");
  }
  void
  __lock_all(void)
  {
    for ( size_t i = 0; i < sz; i++ ) {
      micron::lock(entries[i]->mtx);
    }
  }
  void
  __unlock_all(void)
  {
    for ( size_t i = 0; i < sz; i++ ) {
      micron::unlock(entries[i]->mtx);
    }
  }
  void
  __lock(void)
  {
    micron::lock(__mtx);
  }
  void
  __unlock(void)
  {
    micron::unlock(__mtx);
  }

public:
  ~parallel_system()
  {
    for ( size_t i = 0; i < N; i++ ) {
      if ( i < sz )
        (*entries[i]).sync();     // we'll automatically mandate syncing to underlying storage on dest call
      entries[i].clear();
    }
  }
  parallel_system() : entries{ nullptr }, sz(0) {}
  parallel_system(const io::path_t &p, const io::modes c = _default_mode) : entries{ nullptr }, sz(0) { file(p, c); }
  template <typename... T>
    requires((micron::same_as<T, io::path_t> && ...))
  parallel_system(const T &...t)
  {
    (file(t, _default_mode), ...);
  }
  parallel_system(const parallel_system &o) { micron::cmemcpy<sizeof(entries) * 256>(&entries[0], &o.entries[0]); }
  parallel_system(parallel_system &&o) : entries(micron::move(o.entries)) {}
  parallel_system &
  operator=(const parallel_system &o)
  {
    micron::cmemcpy<sizeof(entries) * 256>(&entries[0], &o.entries[0]);
    return *this;
  }
  parallel_system &
  operator=(parallel_system &&o)
  {
    micron::cmemcpy<sizeof(entries) * 256>(&entries[0], &o.entries[0]);
    micron::czero<sizeof(entries) * 256>(&o.entries[0]);
    return *this;
  }
  // search for file
  auto &
  operator[](const io::path_t &p, const io::modes c = _default_mode,
             const io::node_types nd = io::node_types::regular_file)
  {
    __limit();
    for ( size_t i = 0; i < sz; i++ ) {
      micron::lock_guard(entries[i]->mtx);
      if ( __access(*entries[i]).name() == p )
        return __access(*entries[i]);
    }     // as with maps, if file doesn't exist open it
    return append(p, c, nd);
  }
  inline auto &
  append(const io::path_t &p, const io::modes c = _default_mode, const io::node_types nd = io::node_types::regular_file)
  {
    __limit();
    if ( nd == io::node_types::regular_file )
      return file(p, c);
    exc<except::filesystem_error>("micron::fsys[] path wasn't a file");
  }
  inline void
  remove(const io::path_t &p)
  {
    __lock();
    size_t i = 0;
    for ( ; i < sz; i++ ) {
      if ( entries[i]->name() == p ) {
        entries[i]->sync();
        entries[i].clear();
        break;
      }
    }
    for ( ++i; i < sz; i++ )
      entries[i - 1] = micron::move(entries[i]);
    __unlock();
  }
  inline void
  remove(fsys::file<> &fref)
  {
    __lock();
    size_t i = 0;
    for ( ; i < sz; i++ ) {
      if ( *entries[i] == fref ) {
        __access(*entries[i]).sync();
        entries[i].clear();
        break;
      }
    }
    for ( ++i; i < sz; i++ )
      entries[i - 1] = micron::move(entries[i]);
    __unlock();
  }
  void to_persist() = delete;
  auto
  list(void) const
  {
    micron::vector<micron::sstr<io::max_name>> names;
    for ( size_t i = 0; i < sz; i++ ) {
      micron::lock_guard(entries[i]->mtx);
      names.push_back(__access(*entries[i]).name());
    }
    return names;
  }
  auto &
  file(const io::path_t &p, const io::modes mode = _default_mode)
  {
    __lock();
    __limit();
    if ( !entries[sz] ) {
      entries[sz] = new fsys::file(p.c_str(), mode);
      size_t _sz = sz++;
      __unlock();
      return *entries[_sz];
    } else {
      entries[++sz] = new fsys::file(p.c_str(), mode);
      size_t _sz = sz - 1;
      __unlock();
      return *entries[_sz];
    }
  }
  /*
  template <is_string T>
  auto &
  file(const T &name, const io::modes mode = _default_mode)
  {
    if ( !entries[sz] ) {
      entries[sz] = new fsys::file(name, mode);
      return *entries[sz++];
    } else {
      entries[++sz] = new fsys::file(name, mode);
      return *entries[sz - 1];
    }
  }

  auto &
  file(const char *name, const io::modes mode = _default_mode)
  {
    if ( !entries[sz] ) {
      entries[sz] = new fsys::file(name, mode);
      return *entries[sz++];
    } else {
      entries[++sz] = new fsys::file(name, mode);
      return *entries[sz - 1];
    }
  }*/
  void
  rename(const io::path_t &from, const io::path_t &to)
  {
    __lock();
    auto f_id = __find_id(from);
    if ( f_id == __max_fs ) {
      this->operator[](from);
      f_id = sz - 1;
    }

    auto t_id = __find_id(to);
    if ( t_id == __max_fs ) {
      this->operator[](to);
      t_id = sz - 1;
    }

    posix::rename(from.c_str(), to.c_str());
    __unlock();
  }
  // provide both
  void
  move(const io::path_t &from, const io::path_t &to)
  {
    __lock();
    auto f_id = __find_id(from);
    if ( f_id == __max_fs ) {
      this->operator[](from);
      f_id = sz - 1;
    }

    auto t_id = __find_id(to);
    if ( t_id == __max_fs ) {
      this->operator[](to);
      t_id = sz - 1;
    }

    posix::rename(from.c_str(), to.c_str());
    __unlock();
  }
  void
  copy(const io::path_t &from, const io::path_t &to)
  {
    __lock();
    auto f_id = __find_id(from);
    if ( f_id == __max_fs ) {
      this->operator[](from);
      f_id = sz - 1;
    }

    auto t_id = __find_id(to);
    if ( t_id == __max_fs ) {
      this->operator[](to);
      t_id = sz - 1;
    }
    micron::ustr8 buf;
    entries[f_id]->operator>>(buf);
    entries[t_id]->operator=(buf);
    sync();
    __unlock();
  }
  template <typename... Paths>
  void
  copy_list(const io::path_t &from, const Paths &...to)
  {
    (copy(from, to), ...);
  }
  // we don't need this, but for compatibility with the STL we're adding it
  bool
  is_opened(const io::path_t &path) const
  {
    for ( size_t i = 0; i < sz; i++ )
      if ( entries[i]->name() == path )
        return true;
    return false;
  }
  // we don't need this, but for compatibility with the STL we're adding it
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
  permissions(const io::path_t &path) const
  {
    size_t id = __locate(path);
    return entries[id]->permissions();
  }
  auto
  set_permissions(const io::path_t &path, const io::linux_permissions &perms)
  {
    size_t id = __locate(path);
    return entries[id]->set_permissions(perms);
  }
  template <typename... Args>
  auto
  set_permissions(const io::path_t &path, Args... args)
  {
    size_t id = __locate(path);

    struct linux_permissions perms
        = { { false, false, false }, { false, false, false }, { false, false, false } };     // explicitly init to zero
    (__set_perm(perms, args), ...);
    return entries[id]->set_permissions(perms);
  }
  // FILE TYPE FUNCS
  auto
  file_type_at(const io::path_t &p) const
  {
    return io::get_type_at(p.c_str());
  }
  auto
  file_type(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return io::node_types::not_found;
    return io::get_type(f);
  }
  // is_ stl compat
  bool
  is_virtual_file(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_virtual_file(f);
  }
  bool
  is_regular_file(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_file(f);
  }
  bool
  is_block_device(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_block_device(f);
  }
  bool
  is_directory(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_dir(f);
  }
  bool
  is_socket(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_socket(f);
  }
  bool
  is_symlink(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_symlink(f);
  }
  bool
  is_fifo(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 )
      f = this->operator[](p).get_fd();
    if ( f == -1 )
      return false;
    return io::is_fifo(f);
  }

  // is_ stl compat
  bool
  is_regular_file(void) const
  {
    if ( !!entries[sz - 1] ) {
      return io::is_file((*entries[sz - 1]).get_fd());
    }
    return false;
  }
  bool
  is_block_device(void) const
  {
    if ( !!entries[sz - 1] ) {
      return io::is_block_device((*entries[sz - 1]).get_fd());
    }
    return false;
  }
  bool
  is_directory(void) const
  {
    if ( !!entries[sz - 1] ) {
      return io::is_dir((*entries[sz - 1]).get_fd());
    }
    return false;
  }
  bool
  is_socket(void) const
  {
    if ( !!entries[sz - 1] ) {
      return io::is_socket((*entries[sz - 1]).get_fd());
    }
    return false;
  }
  bool
  is_symlink(void) const
  {
    if ( !!entries[sz - 1] ) {
      return io::is_symlink((*entries[sz - 1]).get_fd());
    }
    return false;
  }
  bool
  is_fifo(void) const
  {
    if ( !!entries[sz - 1] ) {
      return io::is_fifo((*entries[sz - 1]).get_fd());
    }
    return false;
  }
};

};
};
