//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory_block.hpp"
#include "../string/strings.hpp"

#include "file.hpp"
#include "stream.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io::cached_file<T>
// resident whole-file editing

namespace micron
{
namespace io
{

template<is_string T = micron::string> class cached_file: public file
{
  T data;
  usize __cursor;

public:
  ~cached_file() = default;

  cached_file() : file(), data(), __cursor(0) { }

  cached_file(const T &name, const io::modes mode)
      : file(name, mode), data(), __cursor(mode == io::modes::append || mode == io::modes::appendread ? size() : 0)
  {
  }

  cached_file(const char *name, const io::modes mode)
      : file(name, mode), data(), __cursor(mode == io::modes::append || mode == io::modes::appendread ? size() : 0)
  {
  }

  template<usize M>
  cached_file(const char (&name)[M], const io::modes mode)
      : file(name, mode), data(), __cursor(mode == io::modes::append || mode == io::modes::appendread ? size() : 0)
  {
  }

  // prior buffered open signature; the internal scratch buffer is gone
  cached_file(const T &name, const io::modes mode, const usize) : cached_file(name, mode) { }

  cached_file(const char *name, const io::modes mode, const usize) : cached_file(name, mode) { }

  template<usize M> cached_file(const char (&name)[M], const io::modes mode, const usize) : cached_file(name, mode) { }

  cached_file(const T &name) : file(name, io::modes::read), data(), __cursor(0) { }

  cached_file(const char *name) : file(name, io::modes::read), data(), __cursor(0) { }

  template<usize M> cached_file(const char (&name)[M]) : file(name, io::modes::read), data(), __cursor(0) { }

  cached_file(const cached_file &o) : file(o), data(o.data), __cursor(o.__cursor) { }

  cached_file(cached_file &&o) : file(micron::move(o)), data(micron::move(o.data)), __cursor(o.__cursor) { o.__cursor = 0; }

  cached_file &
  operator=(const cached_file &o)
  {
    file::operator=(o);
    data = o.data;
    __cursor = o.__cursor;
    return *this;
  }

  cached_file &
  operator=(cached_file &&o)
  {
    file::operator=(micron::move(o));
    data = micron::move(o.data);
    __cursor = o.__cursor;
    o.__cursor = 0;
    return *this;
  }

  bool
  operator==(const cached_file &o)
  {
    return __handle.fd == o.__handle.fd;
  }

  void
  reopen(const T &name, const io::modes mode)
  {
    sync();
    close();
    data.clear();
    file::operator=(micron::move(file(name, mode)));
    __cursor = (mode == io::modes::append || mode == io::modes::appendread ? size() : 0);
  }

  void
  reopen(const char *name, const io::modes mode)
  {
    sync();
    close();
    data.clear();
    file::operator=(micron::move(file(name, mode)));
    __cursor = (mode == io::modes::append || mode == io::modes::appendread ? size() : 0);
  }

  void
  reopen(const T &name)
  {
    reopen(name, io::modes::read);
  }

  void
  reopen(const char *name)
  {
    reopen(name, io::modes::read);
  }

  // prior buffered reopen signature; size accepted and ignored
  void
  reopen(const T &name, const io::modes mode, const usize)
  {
    reopen(name, mode);
  }

  void
  reopen(const char *name, const io::modes mode, const usize)
  {
    reopen(name, mode);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // resident-content verbs
  void
  clear(void)
  {
    data.clear();
    data._buf_set_length(0);
  }

  void
  swap(cached_file &o)
  {
    micron::swap(data, o.data);
    micron::swap(__cursor, o.__cursor);
    micron::swap(fname, o.fname);
    micron::swap(__handle, o.__handle);
    micron::swap(sd, o.sd);
  }

  // load the whole file into the resident buffer
  max_t
  load(void)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    stat<STAT_OVERRIDE>();
    const usize need = static_cast<usize>(sd.st_size);
    if ( need == 0 ) {
      data._buf_set_length(0);
      return 0;
    }
    data.reserve(need + 1);
    byte *p = reinterpret_cast<byte *>(data.data());
    usize got = 0;
    while ( got < need ) {
      max_t r = posix::pread(__handle.fd, p + got, need - got, static_cast<posix::off64_t>(got));
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        data._buf_set_length(got);
        return r;
      }
      if ( r == 0 ) break;
      got += static_cast<usize>(r);
    }
    data._buf_set_length(got);
    return static_cast<max_t>(got);
  }

  // growth loop for virtual (/proc, /sys) files that fstat to size 0
  max_t
  load_kernel(void)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    data.clear();
    byte chunk[4096];
    usize total = 0;
    posix::off64_t off = 0;
    for ( ;; ) {
      max_t r = posix::pread(__handle.fd, chunk, sizeof(chunk), off);
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        return r;
      }
      if ( r == 0 ) break;
      for ( max_t i = 0; i < r; ++i ) data.push_back(static_cast<typename T::value_type>(chunk[i]));
      total += static_cast<usize>(r);
      off += r;
    }
    return static_cast<max_t>(total);
  }

  max_t
  read_bytes(usize sz)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    data.reserve(data.size() + sz + 1);
    byte chunk[4096];
    usize total = 0;
    while ( total < sz ) {
      const usize want = (sz - total) < sizeof(chunk) ? (sz - total) : sizeof(chunk);
      max_t r = posix::pread(__handle.fd, chunk, want, static_cast<posix::off64_t>(__cursor));
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        return total ? static_cast<max_t>(total) : r;
      }
      if ( r == 0 ) break;
      for ( max_t i = 0; i < r; ++i ) data.push_back(static_cast<typename T::value_type>(chunk[i]));
      __cursor += static_cast<usize>(r);
      total += static_cast<usize>(r);
    }
    return static_cast<max_t>(total);
  }

  // persist the resident buffer at the logical cursor; content stays resident
  max_t
  write(void)
  {
    if ( !data ) return 0;
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    const byte *p = reinterpret_cast<const byte *>(data.data());
    const usize len = data.size() * sizeof(typename T::value_type);
    usize done = 0;
    while ( done < len ) {
      max_t w = posix::pwrite(__handle.fd, p + done, len - done, static_cast<posix::off64_t>(__cursor + done));
      if ( w < 0 ) [[unlikely]] {
        if ( -w == error::interrupted ) continue;
        __cursor += done;
        micron::zero(&sd);
        return w;
      }
      if ( w == 0 ) break;
      done += static_cast<usize>(w);
    }
    __cursor += done;
    micron::zero(&sd);
    return static_cast<max_t>(done);
  }

  // expose the porcelain universal marshalling alongside the resident verbs
  using file::read;
  using file::write;

  // persist + release the resident buffer
  max_t
  flush(void)
  {
    max_t n = write();
    if ( n >= 0 ) clear();
    return n;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // resident accessors
  usize
  count(void) const
  {
    return data.size();
  }

  void
  push_copy(const T &str)
  {
    data = str;
  }

  void
  push(T &&str)
  {
    data = micron::move(str);
  }

  auto
  pull(void)
  {
    auto t = micron::move(data);
    data = T();
    return t;
  }

  const auto &
  get(void) const
  {
    return data;
  }

  auto
  get_fd(void) const
  {
    return __handle.fd;
  }

  auto
  load_and_pull(void)
  {
    if ( is_system_virtual() )
      load_kernel();
    else
      load();
    auto tmp = micron::move(data);
    data = T();
    return tmp;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // logical cursor
  cached_file &
  set(const usize s)
  {
    __cursor = s;
    return *this;
  }

  cached_file &
  set_start(void)
  {
    __cursor = 0;
    return *this;
  }

  cached_file &
  set_end(void)
  {
    __cursor = size();
    return *this;
  }

  usize
  seek_pos(void) const
  {
    return __cursor;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // stream-ish sugar over the resident buffer
  cached_file &
  operator>>(T &str)
  {
    str = load_and_pull();
    return *this;
  }

  cached_file &
  operator<<(T &&str)
  {
    push(micron::move(str));
    write();
    return *this;
  }

  cached_file &
  operator<<(const T &str)
  {
    push_copy(str);
    write();
    return *this;
  }

  cached_file &
  operator=(T &&str)
  {
    push(micron::move(str));
    write();
    return *this;
  }

  cached_file &
  operator=(const T &str)
  {
    push_copy(str);
    write();
    return *this;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // appenders
  max_t
  append_raw(const byte *ptr, usize len)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    stat<STAT_OVERRIDE>();
    posix::off64_t dst_off = sd.st_size;
    usize done = 0;
    while ( done < len ) {
      max_t w = posix::pwrite(__handle.fd, ptr + done, len - done, dst_off + static_cast<posix::off64_t>(done));
      if ( w < 0 ) [[unlikely]] {
        if ( -w == error::interrupted ) continue;
        micron::zero(&sd);
        return w;
      }
      if ( w == 0 ) break;
      done += static_cast<usize>(w);
    }
    micron::zero(&sd);
    return static_cast<max_t>(done);
  }

  template<is_string Tp>
  max_t
  append_raw(const Tp &str)
  {
    return append_raw(reinterpret_cast<const byte *>(str.c_str()), str.size() * sizeof(typename Tp::value_type));
  }

  template<is_string Tp>
  max_t
  append(const Tp &str)
  {
    return append_raw(str);
  }

  // append the whole contents of another cached_file
  template<is_string U>
  max_t
  append_file(const cached_file<U> &other, usize chunk_sz = 65536u)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( other.get_fd() < 0 ) [[unlikely]]
      return -error::bad_fd;

    posix::off64_t src_off = 0;
    usize src_sz = static_cast<usize>(const_cast<cached_file<U> &>(other).size());
    stat<STAT_OVERRIDE>();
    posix::off64_t dst_off = sd.st_size;

    micron::buffer win(chunk_sz);
    max_t total = 0;
    while ( static_cast<usize>(src_off) < src_sz ) {
      max_t n = posix::pread(other.get_fd(), win.begin(), chunk_sz, src_off);
      if ( n < 0 ) [[unlikely]] {
        if ( -n == error::interrupted ) continue;
        micron::zero(&sd);
        return n;      // propagate source read error
      }
      if ( n == 0 ) break;
      // write the n read bytes in full at the advancing destination offset
      usize wdone = 0;
      while ( wdone < static_cast<usize>(n) ) {
        max_t w
            = posix::pwrite(__handle.fd, win.begin() + wdone, static_cast<usize>(n) - wdone, dst_off + static_cast<posix::off64_t>(wdone));
        if ( w < 0 ) [[unlikely]] {
          if ( -w == error::interrupted ) continue;
          micron::zero(&sd);
          return w;      // propagate -errno
        }
        if ( w == 0 ) break;
        wdone += static_cast<usize>(w);
      }
      src_off += static_cast<posix::off64_t>(wdone);
      dst_off += static_cast<posix::off64_t>(wdone);
      total += static_cast<max_t>(wdone);
      if ( wdone < static_cast<usize>(n) ) break;      // short write (pwrite == 0): stop
    }
    micron::zero(&sd);
    return total;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // whole-file helpers
  max_t
  copy_to(const char *dest_path, usize chunk_sz = 65536u) const
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    posix::fd_t out{ static_cast<i32>(posix::openat(
        posix::at_fdcwd, dest_path, posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec, posix::mode_file)) };
    if ( !out ) [[unlikely]]
      return out.fd;

    usize src_sz = static_cast<usize>(const_cast<cached_file *>(this)->size());
    posix::off64_t src_off = 0;
    micron::buffer win(chunk_sz);
    max_t total = 0;

    while ( static_cast<usize>(src_off) < src_sz ) {
      max_t n = posix::pread(__handle.fd, win.begin(), chunk_sz, src_off);
      if ( n < 0 ) {
        if ( -n == error::interrupted ) continue;
        posix::close(out.fd);
        return n;      // propagate a source read error; never report a truncated copy as success
      }
      if ( n == 0 ) break;      // EOF
      // write all n read bytes to the destination
      usize wdone = 0;
      while ( wdone < static_cast<usize>(n) ) {
        max_t w = posix::write(out.fd, win.begin() + wdone, static_cast<usize>(n) - wdone);
        if ( w < 0 ) [[unlikely]] {
          if ( -w == error::interrupted ) continue;
          posix::close(out.fd);
          return w;      // never report source size as success
        }
        if ( w == 0 ) break;
        wdone += static_cast<usize>(w);
      }
      src_off += static_cast<posix::off64_t>(wdone);
      total += static_cast<max_t>(wdone);
      if ( wdone < static_cast<usize>(n) ) break;      // short write (write == 0): stop
    }
    posix::close(out.fd);
    return total;
  }

  template<is_string Tp>
  max_t
  copy_to(const Tp &dest_path, usize chunk_sz = 65536u) const
  {
    return copy_to(dest_path.c_str(), chunk_sz);
  }

  // three-way byte comparison: -1 / 0 / 1, or negative errno < -1 on failure
  template<is_string U>
  i32
  compare_to(const cached_file<U> &other, usize chunk_sz = 65536u) const
  {
    if ( __check() != 0 || other.get_fd() < 0 ) [[unlikely]]
      return -error::bad_fd;

    usize a_sz = static_cast<usize>(const_cast<cached_file *>(this)->size());
    usize b_sz = static_cast<usize>(const_cast<cached_file<U> &>(other).size());
    usize scan = 0;
    micron::buffer wa(chunk_sz), wb(chunk_sz);

    while ( scan < a_sz && scan < b_sz ) {
      usize take = chunk_sz < (a_sz - scan) ? chunk_sz : (a_sz - scan);
      take = take < (b_sz - scan) ? take : (b_sz - scan);

      max_t na = posix::pread(__handle.fd, wa.begin(), take, static_cast<posix::off64_t>(scan));
      if ( na < 0 ) {
        if ( -na == error::interrupted ) continue;
        i32 e = static_cast<i32>(na);
        return e < -1 ? e : -error::io_error;      // real error -> errno (<-1), never a bogus verdict
      }
      max_t nb = posix::pread(other.get_fd(), wb.begin(), take, static_cast<posix::off64_t>(scan));
      if ( nb < 0 ) {
        if ( -nb == error::interrupted ) continue;
        i32 e = static_cast<i32>(nb);
        return e < -1 ? e : -error::io_error;
      }
      if ( na == 0 || nb == 0 ) break;      // EOF on one side (not an error)

      usize cmp_n = static_cast<usize>(na) < static_cast<usize>(nb) ? static_cast<usize>(na) : static_cast<usize>(nb);

      for ( usize i = 0; i < cmp_n; ++i ) {
        if ( wa[i] < wb[i] ) return -1;
        if ( wa[i] > wb[i] ) return 1;
      }
      scan += cmp_n;
    }

    if ( a_sz < b_sz ) return -1;
    if ( a_sz > b_sz ) return 1;
    return 0;
  }

  // load with a tap: fn observes every chunk as it streams into the resident buffer
  template<io::intercept_fn Fn>
  max_t
  load_intercepted(Fn &&fn, usize chunk_sz = 65536u)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    stat<STAT_OVERRIDE>();
    usize file_sz = static_cast<usize>(sd.st_size);
    data.reserve(file_sz + 1);

    micron::buffer win(chunk_sz);
    posix::off64_t off = 0;
    max_t total = 0;

    while ( static_cast<usize>(off) < file_sz ) {
      max_t n = posix::pread(__handle.fd, win.begin(), chunk_sz, off);
      if ( n < 0 && -n == error::interrupted ) continue;
      if ( n <= 0 ) break;
      fn(win.begin(), static_cast<usize>(n));
      data.append(reinterpret_cast<const typename T::value_type *>(win.begin()), static_cast<usize>(n));      // raw bytes, not a T object
      off += n;
      total += n;
    }
    return total;
  }

  template<int SZ, int CK>
  max_t
  flush_to_stream(io::stream<SZ, CK> &s) const
  {
    if ( !data ) return 0;
    s << data;
    return static_cast<max_t>(data.size());
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // truncation with cursor clamping
  max_t
  truncate(usize new_size)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    i32 r = static_cast<i32>(posix::ftruncate(__handle.fd, static_cast<posix::off64_t>(new_size)));
    if ( r != 0 ) [[unlikely]]
      return r;
    micron::zero(&sd);
    if ( __cursor > new_size ) __cursor = new_size;
    return 0;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // metadata passthrough renames
  bool
  empty(void)
  {
    return size() == 0;
  }

  posix::mode_t
  permissions_mode(void)
  {
    return mode();
  }

  io::linux_permissions
  perms(void)
  {
    return permissions();
  }

  posix::uid_t
  owner(void)
  {
    return uid();
  }

  posix::gid_t
  group(void)
  {
    return gid();
  }

  posix::ino64_t
  inode_number(void)
  {
    return inode();
  }

  posix::nlink_t
  hard_links(void)
  {
    return link_count();
  }

  time64_t
  modified_time(void)
  {
    return mtime();
  }

  time64_t
  accessed_time(void)
  {
    return atime();
  }

  time64_t
  changed_time(void)
  {
    return ctime();
  }

  bool
  owned_by_me(void) const
  {
    return posix::is_owned_by(__handle, posix::getuid());
  }

  max_t
  sync(void) const
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    posix::fsync(__handle.fd);
    return posix::syncfs(__handle.fd);
  }
};

};      // namespace io

};      // namespace micron
