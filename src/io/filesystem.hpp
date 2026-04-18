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

#include "../mutex/locks.hpp"

namespace micron
{
namespace fsys
{

constexpr static const usize __max_fs = 0xFFFFFFFFFFFFFFFF;

template <io::modes __default_mode = io::modes::read, usize N = 256> class system
{
  micron::unique_pointer<fsys::file<>> entries[N];
  usize sz;

  auto
  __find_fd(const io::path_t &p) -> int
  {
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == p ) return (*entries[i]).get_fd();
    return -1;
  }

  auto
  __find_id(const io::path_t &p) -> usize
  {
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == p ) return i;
    return __max_fs;
  }

  auto &
  __find(const io::path_t &p)
  {
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == p ) return *entries[i];
    exc<except::filesystem_error>("micron fsys wasn't able to find file");
  }

  inline __attribute__((always_inline)) usize
  __locate(const io::path_t &p)
  {
    auto _id = __find_id(p);
    if ( _id == __max_fs ) {
      this->operator[](p);
      _id = sz - 1;
    }
    return _id;
  }

  inline __attribute__((always_inline)) usize
  __locate(const io::path_t &p, const io::modes mode)
  {
    auto _id = __find_id(p);
    if ( _id == __max_fs ) {
      __limit();
      entries[sz] = new fsys::file<>(p.c_str(), mode);
      ++sz;
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
    if ( sz == N ) exc<except::filesystem_error>("micron::fsys too many file handles open");
  }

public:
  ~system()
  {
    for ( usize i = 0; i < N; i++ ) {
      if ( i < sz ) (*entries[i]).sync();
      entries[i].clear();
    }
  }

  system() : entries{ nullptr }, sz(0) {}

  system(const io::path_t &p, const io::modes c = __default_mode) : entries{ nullptr }, sz(0) { file(p, c); }

  template <typename... T>
    requires((micron::same_as<T, io::path_t> && ...))
  system(const T &...t)
  {
    (file(t, __default_mode), ...);
  }

  system(const system &o) { micron::cmemcpy<sizeof(entries) * 256>(&entries[0], &o.entries[0]); }

  system(system &&o) : entries(micron::move(o.entries)) {}

  system &
  operator=(const system &o)
  {
    micron::cmemcpy<sizeof(entries) * 256>(&entries[0], &o.entries[0]);
    return *this;
  }

  system &
  operator=(system &&o)
  {
    micron::cmemcpy<sizeof(entries) * 256>(&entries[0], &o.entries[0]);
    micron::czero<sizeof(entries) * 256>(&o.entries[0]);
    return *this;
  }

  auto &
  operator[](const io::path_t &p, const io::modes c = __default_mode, const posix::node_types nd = posix::node_types::regular_file)
  {
    __limit();
    for ( usize i = 0; i < sz; i++ )
      if ( entries[i]->name() == p ) return *entries[i];
    return append(p, c, nd);
  }

  inline auto &
  append(const io::path_t &p, const io::modes c = __default_mode, const posix::node_types nd = posix::node_types::regular_file)
  {
    __limit();
    if ( nd == posix::node_types::regular_file ) return file(p, c);
    exc<except::filesystem_error>("micron::fsys[] path wasn't a file");
  }

  inline void
  remove(const io::path_t &p)
  {
    usize i = 0;
    for ( ; i < sz; i++ ) {
      if ( entries[i]->name() == p ) {
        entries[i]->sync();
        entries[i].clear();
        break;
      }
    }
    for ( ++i; i < sz; i++ ) entries[i - 1] = micron::move(entries[i]);
  }

  inline void
  remove(fsys::file<> &fref)
  {
    usize i = 0;
    for ( ; i < sz; i++ ) {
      if ( *entries[i] == fref ) {
        entries[i]->sync();
        entries[i].clear();
        break;
      }
    }
    for ( ++i; i < sz; i++ ) entries[i - 1] = micron::move(entries[i]);
  }

  void to_persist() = delete;

  auto
  list(void) const
  {
    micron::vector<micron::sstr<posix::name_max>> names;
    for ( usize i = 0; i < sz; i++ ) names.push_back(entries[i]->name());
    return names;
  }

  auto &
  file(const io::path_t &p, const io::modes mode = __default_mode)
  {
    __limit();
    if ( !entries[sz] ) {
      entries[sz] = new fsys::file<>(p.c_str(), mode);
      return *entries[sz++];
    } else {
      entries[++sz] = new fsys::file<>(p.c_str(), mode);
      return *entries[sz - 1];
    }
  }

  void
  rename(const io::path_t &from, const io::path_t &to)
  {
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
  }

  void
  move(const io::path_t &from, const io::path_t &to)
  {
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
  }

  void
  copy(const io::path_t &from, const io::path_t &to)
  {
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
    micron::string buf;
    entries[f_id]->operator>>(buf);
    entries[t_id]->operator=(buf);
    sync();
  }

  template <typename... Paths>
  void
  copy_list(const io::path_t &from, const Paths &...to)
  {
    (copy(from, to), ...);
  }

  bool
  is_opened(const io::path_t &path) const
  {
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
  permissions(const io::path_t &path) const
  {
    usize id = __locate(path);
    return entries[id]->permissions();
  }

  auto
  set_permissions(const io::path_t &path, const io::linux_permissions &perms)
  {
    usize id = __locate(path);
    return entries[id]->set_permissions(perms);
  }

  template <typename... Args>
  auto
  set_permissions(const io::path_t &path, Args... args)
  {
    usize id = __locate(path);
    struct io::linux_permissions perms = { { false, false, false }, { false, false, false }, { false, false, false } };
    (__set_perms(perms, args), ...);
    return entries[id]->set_permissions(perms);
  }

  auto
  file_type_at(const io::path_t &p) const
  {
    return posix::get_type_at(p.c_str());
  }

  auto
  file_type(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return posix::node_types::not_found;
    return posix::get_type(f);
  }

  bool
  is_virtual_file(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_virtual_file(f);
  }

  bool
  is_regular_file(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_file(f);
  }

  bool
  is_block_device(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_block_device(f);
  }

  bool
  is_directory(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_dir(f);
  }

  bool
  is_socket(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_socket(f);
  }

  bool
  is_symlink(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_symlink(f);
  }

  bool
  is_fifo(const io::path_t &p) const
  {
    auto f = __find_fd(p);
    if ( f == -1 ) f = this->operator[](p).get_fd();
    if ( f == -1 ) return false;
    return posix::is_fifo(f);
  }

  bool
  is_regular_file(void) const
  {
    if ( !!entries[sz - 1] ) return posix::is_file((*entries[sz - 1]).get_fd());
    return false;
  }

  bool
  is_block_device(void) const
  {
    if ( !!entries[sz - 1] ) return posix::is_block_device((*entries[sz - 1]).get_fd());
    return false;
  }

  bool
  is_directory(void) const
  {
    if ( !!entries[sz - 1] ) return posix::is_dir((*entries[sz - 1]).get_fd());
    return false;
  }

  bool
  is_socket(void) const
  {
    if ( !!entries[sz - 1] ) return posix::is_socket((*entries[sz - 1]).get_fd());
    return false;
  }

  bool
  is_symlink(void) const
  {
    if ( !!entries[sz - 1] ) return posix::is_symlink((*entries[sz - 1]).get_fd());
    return false;
  }

  bool
  is_fifo(void) const
  {
    if ( !!entries[sz - 1] ) return posix::is_fifo((*entries[sz - 1]).get_fd());
    return false;
  }

  usize
  open_count() const noexcept
  {
    return sz;
  }

  usize
  max_handles() const noexcept
  {
    return N;
  }

  bool
  handles_full() const noexcept
  {
    return sz == N;
  }

  bool
  handles_empty() const noexcept
  {
    return sz == 0;
  }

  void
  sync()
  {
    for ( usize i = 0; i < sz; i++ ) entries[i]->sync();
  }

  void
  flush_all()
  {
    for ( usize i = 0; i < sz; i++ ) entries[i]->flush();
  }

  void
  close(const io::path_t &p)
  {
    remove(p);
  }

  auto &
  set(const io::path_t &p, const usize s)
  {
    return entries[__locate(p)]->set(s);
  }

  auto &
  set_start(const io::path_t &p)
  {
    return entries[__locate(p)]->set_start();
  }

  auto &
  set_end(const io::path_t &p)
  {
    return entries[__locate(p)]->set_end();
  }

  usize
  seek_pos(const io::path_t &p)
  {
    return entries[__locate(p)]->seek_pos();
  }

  void
  load(const io::path_t &p)
  {
    entries[__locate(p)]->load();
  }

  void
  load_kernel(const io::path_t &p)
  {
    entries[__locate(p)]->load_kernel();
  }

  void
  write(const io::path_t &p)
  {
    entries[__locate(p, io::modes::write)]->write();
  }

  void
  flush(const io::path_t &p)
  {
    entries[__locate(p, io::modes::write)]->flush();
  }

  void
  clear(const io::path_t &p)
  {
    entries[__locate(p)]->clear();
  }

  void
  read_bytes(const io::path_t &p, usize sz_)
  {
    entries[__locate(p)]->read_bytes(sz_);
  }

  void
  write_bytes(const io::path_t &p, usize sz_)
  {
    entries[__locate(p, io::modes::write)]->write_bytes(sz_);
  }

  void
  load_buffer(const io::path_t &p, const byte *b, const usize n)
  {
    entries[__locate(p)]->load_buffer(b, n);
  }

  auto
  buffer_size(const io::path_t &p)
  {
    return entries[__locate(p)]->buffer_size();
  }

  void
  push(const io::path_t &p, micron::string &&str)
  {
    entries[__locate(p, io::modes::write)]->push(micron::move(str));
  }

  void
  push_copy(const io::path_t &p, const micron::string &str)
  {
    entries[__locate(p, io::modes::write)]->push_copy(str);
  }

  auto
  pull(const io::path_t &p)
  {
    return entries[__locate(p)]->pull();
  }

  const auto &
  get(const io::path_t &p)
  {
    return entries[__locate(p)]->get();
  }

  auto
  load_and_pull(const io::path_t &p)
  {
    return entries[__locate(p)]->load_and_pull();
  }

  usize
  count(const io::path_t &p)
  {
    return entries[__locate(p)]->count();
  }

  system &
  operator<<(const io::path_t &p)
  {

    __locate(p);
    return *this;
  }

  template <is_string T>
  system &
  write_string(const io::path_t &p, const T &str)
  {
    entries[__locate(p, io::modes::write)]->operator<<(str);
    return *this;
  }

  template <is_string T>
  system &
  write_string(const io::path_t &p, T &&str)
  {
    entries[__locate(p, io::modes::write)]->operator<<(micron::move(str));
    return *this;
  }

  template <is_string T>
  system &
  read_string(const io::path_t &p, T &str)
  {
    entries[__locate(p)]->operator>>(str);
    return *this;
  }

  bool
  empty(const io::path_t &p)
  {
    return entries[__locate(p)]->empty();
  }

  bool
  is_regular(const io::path_t &p)
  {
    return entries[__locate(p)]->is_regular();
  }

  bool
  is_virtual(const io::path_t &p)
  {
    return entries[__locate(p)]->is_virtual();
  }

  auto
  permissions_mode(const io::path_t &p)
  {
    return entries[__locate(p)]->permissions_mode();
  }

  auto
  perms(const io::path_t &p)
  {
    return entries[__locate(p)]->perms();
  }

  auto
  owner(const io::path_t &p)
  {
    return entries[__locate(p)]->owner();
  }

  auto
  group(const io::path_t &p)
  {
    return entries[__locate(p)]->group();
  }

  auto
  inode_number(const io::path_t &p)
  {
    return entries[__locate(p)]->inode_number();
  }

  auto
  hard_links(const io::path_t &p)
  {
    return entries[__locate(p)]->hard_links();
  }

  auto
  size(const io::path_t &p)
  {
    return entries[__locate(p)]->size();
  }

  auto
  modified_time(const io::path_t &p)
  {
    return entries[__locate(p)]->modified_time();
  }

  auto
  accessed_time(const io::path_t &p)
  {
    return entries[__locate(p)]->accessed_time();
  }

  auto
  changed_time(const io::path_t &p)
  {
    return entries[__locate(p)]->changed_time();
  }

  bool
  owned_by_me(const io::path_t &p)
  {
    return entries[__locate(p)]->owned_by_me();
  }

  auto
  as_path(const io::path_t &p)
  {
    return entries[__locate(p)]->as_path();
  }

  auto
  fullpath(const io::path_t &p)
  {
    return entries[__locate(p)]->fullpath();
  }

  auto
  basename(const io::path_t &p)
  {
    return entries[__locate(p)]->basename();
  }

  auto
  extension(const io::path_t &p)
  {
    return entries[__locate(p)]->extension();
  }

  auto
  stem(const io::path_t &p)
  {
    return entries[__locate(p)]->stem();
  }

  byte
  read_u8(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u8(off);
  }

  i8
  read_i8(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i8(off);
  }

  u16
  read_u16le(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u16le(off);
  }

  u16
  read_u16be(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u16be(off);
  }

  i16
  read_i16le(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i16le(off);
  }

  i16
  read_i16be(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i16be(off);
  }

  u32
  read_u32le(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u32le(off);
  }

  u32
  read_u32be(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u32be(off);
  }

  i32
  read_i32le(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i32le(off);
  }

  i32
  read_i32be(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i32be(off);
  }

  u64
  read_u64le(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u64le(off);
  }

  u64
  read_u64be(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_u64be(off);
  }

  i64
  read_i64le(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i64le(off);
  }

  i64
  read_i64be(const io::path_t &p, posix::off_t off)
  {
    return entries[__locate(p)]->read_i64be(off);
  }

  micron::buffer
  slice(const io::path_t &p, posix::off_t off, usize len)
  {
    return entries[__locate(p)]->slice(off, len);
  }

  void
  slice_into(const io::path_t &p, micron::buffer &dst, posix::off_t off, usize len)
  {
    entries[__locate(p)]->slice_into(dst, off, len);
  }

  void
  hex_at(const io::path_t &p, char *dst, posix::off_t off, usize len)
  {
    entries[__locate(p)]->hex_at(dst, off, len);
  }

  io::bin_match_t
  search(const io::path_t &p, const byte *pat, usize plen, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->search(pat, plen, window_sz);
  }

  io::bin_match_t
  search(const io::path_t &p, const char *pat, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->search(pat, window_sz);
  }

  template <is_string Tp>
  io::bin_match_t
  search(const io::path_t &p, const Tp &pat, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->search(pat, window_sz);
  }

  io::bin_match_t
  search_file(const io::path_t &p, const byte *pat, usize plen, usize w = fsys::file<>::default_search_window)
  {
    return search(p, pat, plen, w);
  }

  io::bin_match_t
  search_file(const io::path_t &p, const char *pat, usize w = fsys::file<>::default_search_window)
  {
    return search(p, pat, w);
  }

  template <is_string Tp>
  io::bin_match_t
  search_file(const io::path_t &p, const Tp &pat, usize w = fsys::file<>::default_search_window)
  {
    return search(p, pat, w);
  }

  micron::vector<io::bin_match_t>
  find_all(const io::path_t &p, const byte *pat, usize plen, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->find_all(pat, plen, window_sz);
  }

  micron::vector<io::bin_match_t>
  find_all(const io::path_t &p, const char *pat, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->find_all(pat, window_sz);
  }

  template <is_string Tp>
  micron::vector<io::bin_match_t>
  find_all(const io::path_t &p, const Tp &pat, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->find_all(pat, window_sz);
  }

  io::bin_stats_t
  analyse(const io::path_t &p, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->analyse(window_sz);
  }

  double
  entropy(const io::path_t &p, usize window_sz = fsys::file<>::default_search_window)
  {
    return entries[__locate(p)]->entropy(window_sz);
  }

  template <int SZ, int CK>
  void
  to_stream(const io::path_t &p, io::stream<SZ, CK> &s)
  {
    entries[__locate(p)]->to_stream(s);
  }

  template <int SZ, int CK>
  void
  from_stream(const io::path_t &p, io::stream<SZ, CK> &s)
  {
    entries[__locate(p, io::modes::write)]->from_stream(s);
  }

  template <int SZ, int CK>
  void
  flush_to_stream(const io::path_t &p, io::stream<SZ, CK> &s)
  {
    entries[__locate(p)]->flush_to_stream(s);
  }

  template <io::encode_fn Fn, is_string Tp>
  void
  write_encoded(const io::path_t &p, Fn &&fn, const Tp &src)
  {
    entries[__locate(p, io::modes::write)]->write_encoded(micron::forward<Fn>(fn), src);
  }

  template <io::encode_fn Fn>
  void
  write_encoded(const io::path_t &p, Fn &&fn, const byte *src, usize src_len)
  {
    entries[__locate(p, io::modes::write)]->write_encoded(micron::forward<Fn>(fn), src, src_len);
  }

  template <io::encode_fn Fn, is_string Tp>
  void
  append_encoded(const io::path_t &p, Fn &&fn, const Tp &src)
  {
    entries[__locate(p, io::modes::append)]->append_encoded(micron::forward<Fn>(fn), src);
  }

  void
  truncate(const io::path_t &p, usize new_size)
  {
    entries[__locate(p, io::modes::write)]->truncate(new_size);
  }

  void
  append_file(const io::path_t &dst, const io::path_t &src, usize chunk_sz = 65536u)
  {
    usize d_id = __locate(dst, io::modes::appendread);
    usize s_id = __locate(src);
    entries[d_id]->append_file(*entries[s_id], chunk_sz);
  }

  void
  append_raw(const io::path_t &p, const byte *data_ptr, usize len)
  {
    entries[__locate(p, io::modes::appendread)]->append_raw(data_ptr, len);
  }

  template <is_string Tp>
  void
  append_raw(const io::path_t &p, const Tp &str)
  {
    entries[__locate(p, io::modes::appendread)]->append_raw(str);
  }

  void
  copy_to(const io::path_t &src, const char *dest_path, usize chunk_sz = 65536u)
  {
    entries[__locate(src)]->copy_to(dest_path, chunk_sz);
  }

  template <is_string Tp>
  void
  copy_to(const io::path_t &src, const Tp &dest_path, usize chunk_sz = 65536u)
  {
    entries[__locate(src)]->copy_to(dest_path, chunk_sz);
  }

  i32
  compare_to(const io::path_t &a, const io::path_t &b, usize chunk_sz = 65536u)
  {
    usize a_id = __locate(a);
    usize b_id = __locate(b);
    return entries[a_id]->compare_to(*entries[b_id], chunk_sz);
  }

  bool
  same_file(const io::path_t &a, const io::path_t &b) const
  {
    return posix::is_same_file(a.c_str(), b.c_str());
  }

  template <io::intercept_fn Fn>
  void
  load_intercepted(const io::path_t &p, Fn &&fn, usize chunk_sz = 65536u)
  {
    entries[__locate(p)]->load_intercepted(micron::forward<Fn>(fn), chunk_sz);
  }

  void
  atomic_replace(const io::path_t &p, const byte *new_data, usize new_len)
  {
    entries[__locate(p, io::modes::write)]->atomic_replace(new_data, new_len);
  }

  template <is_string Tp>
  void
  atomic_replace(const io::path_t &p, const Tp &str)
  {
    entries[__locate(p, io::modes::write)]->atomic_replace(str);
  }

  void
  atomic_replace(const io::path_t &p, const micron::buffer &buf)
  {
    entries[__locate(p, io::modes::write)]->atomic_replace(buf);
  }

  void
  reopen(const io::path_t &p, const io::modes mode)
  {
    entries[__locate(p)]->reopen(p, mode);
  }

  void
  reopen(const io::path_t &p, const io::modes mode, const usize buf_sz)
  {
    entries[__locate(p)]->reopen(p, mode, buf_sz);
  }

  void
  swap(const io::path_t &a, const io::path_t &b)
  {
    usize a_id = __locate(a);
    usize b_id = __locate(b);
    entries[a_id]->swap(*entries[b_id]);
  }

  fsys::file<> &
  at(const io::path_t &p)
  {
    return *entries[__locate(p)];
  }

  const fsys::file<> &
  at(const io::path_t &p) const
  {
    usize id = __find_id(p);
    if ( id == __max_fs ) exc<except::filesystem_error>("micron::fsys::system::at — file not open.");
    return *entries[id];
  }

  fsys::file<> &
  at(usize idx)
  {
    if ( idx >= sz ) exc<except::filesystem_error>("micron::fsys::system::at — index out of range.");
    return *entries[idx];
  }

  const fsys::file<> &
  at(usize idx) const
  {
    if ( idx >= sz ) exc<except::filesystem_error>("micron::fsys::system::at — index out of range.");
    return *entries[idx];
  }

  auto &
  create(const io::path_t &p, const io::modes mode = io::modes::write)
  {
    return file(p, mode);
  }

  void
  unlink(const io::path_t &p)
  {
    if ( __find_id(p) != __max_fs ) remove(p);
    posix::unlink(p.c_str());
  }

  template <typename Fn>
  void
  sync_if(Fn &&pred)
  {
    for ( usize i = 0; i < sz; i++ )
      if ( pred(*entries[i]) ) entries[i]->sync();
  }

  template <typename Fn>
  void
  for_each(Fn &&fn)
  {
    for ( usize i = 0; i < sz; i++ ) fn(*entries[i]);
  }

  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    for ( usize i = 0; i < sz; i++ ) fn(static_cast<const fsys::file<> &>(*entries[i]));
  }
};

};     // namespace fsys
};     // namespace micron
