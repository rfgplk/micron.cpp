//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/addr.hpp"
#include "../../string/sstring.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../sys/fcntl.hpp"
#include "../sys/stat.hpp"
#include "../sys/types.hpp"

#include "inode.hpp"
#include "io_structs.hpp"

#include "sys.hpp"

namespace micron
{
namespace posix
{

struct auto_fd {
  fd_t fd;

  auto_fd() : fd(invalid_fd) {}

  explicit auto_fd(fd_t f) : fd(f) {}

  explicit auto_fd(i32 raw) : fd(raw) {}

  auto_fd(const auto_fd &) = delete;
  auto_fd &operator=(const auto_fd &) = delete;

  auto_fd(auto_fd &&o) noexcept : fd(o.fd) { o.fd.reset(); }

  auto_fd &
  operator=(auto_fd &&o) noexcept
  {
    if ( fd.open() ) posix::close(fd.fd);
    fd = o.fd;
    o.fd.reset();
    return *this;
  }

  ~auto_fd()
  {
    if ( fd.open() ) posix::close(fd.fd);
  }

  bool
  valid() const
  {
    return fd.open();
  }

  explicit
  operator bool() const
  {
    return valid();
  }

  explicit
  operator i32() const
  {
    return fd.fd;
  }

  i32
  raw() const
  {
    return fd.fd;
  }

  fd_t
  release()
  {
    fd_t tmp = fd;
    fd.reset();
    return tmp;
  }
};

inline fd_t
open_read(const char *path)
{
  return fd_t{ (posix::openat(at_fdcwd, path, o_rdonly | o_cloexec, 0)) };
}

inline fd_t
open_write(const char *path, u32 mode = mode_file)
{
  return fd_t{ (posix::openat(at_fdcwd, path, o_wronly | o_create | o_trunc | o_cloexec, mode)) };
}

inline fd_t
open_append(const char *path, u32 mode = mode_file)
{
  return fd_t{ (posix::openat(at_fdcwd, path, o_wronly | o_create | o_append | o_cloexec, mode)) };
}

inline fd_t
open_rdwr(const char *path, u32 mode = mode_file)
{
  return fd_t{ (posix::openat(at_fdcwd, path, o_rdwr | o_create | o_cloexec, mode)) };
}

inline fd_t
open_excl(const char *path, u32 mode = mode_file)
{
  return fd_t{ (posix::openat(at_fdcwd, path, o_rdwr | o_create | o_excl | o_cloexec, mode)) };
}

inline fd_t
open_flags(const char *path, i32 flags, u32 mode = 0)
{
  return fd_t{ (posix::openat(at_fdcwd, path, flags, mode)) };
}

inline fd_t
open_at(fd_t dirfd, const char *path, i32 flags, u32 mode = 0)
{
  return fd_t{ (posix::openat(dirfd.fd, path, flags, mode)) };
}

inline fd_t
create_file(const char *path, u32 mode = mode_file)
{
  return fd_t{ (posix::creat(path, mode)) };
}

inline auto_fd
open_read_s(const char *path)
{
  return auto_fd{ open_read(path) };
}

inline auto_fd
open_write_s(const char *path, u32 mode = mode_file)
{
  return auto_fd{ open_write(path, mode) };
}

inline auto_fd
open_append_s(const char *path, u32 mode = mode_file)
{
  return auto_fd{ open_append(path, mode) };
}

inline auto_fd
open_rdwr_s(const char *path, u32 mode = mode_file)
{
  return auto_fd{ open_rdwr(path, mode) };
}

inline i32
close_fd(fd_t &fd)
{
  i32 r = -1;
  if ( fd.open() ) {
    r = (posix::close(fd.fd));
    fd.reset();
  }
  return r;
}

inline dir_t
opendir(const char *path)
{
  return dir_t{ (posix::openat(at_fdcwd, path, o_rdonly | o_directory | o_cloexec, 0)) };
}

inline dir_t
opendir_at(fd_t parent, const char *name)
{
  return dir_t{ (posix::openat(parent.fd, name, o_rdonly | o_directory | o_cloexec, 0)) };
}

inline i32
closedir(fd_t &ds)
{
  return close_fd(ds);
}

inline __impl_dir
readdir(const fd_t &_f)
{
  static char __readdir_buf[8192];
  static usize __readdir_bufpos = 0;
  static usize __readdir_nread = 0;
  static i32 __readdir_last_fd = -1;

  if ( _f.fd != __readdir_last_fd ) {
    __readdir_bufpos = 0;
    __readdir_nread = 0;
    __readdir_last_fd = _f.fd;
  }

  if ( __readdir_bufpos >= __readdir_nread ) {
    __readdir_nread = static_cast<usize>(posix::getdents64(_f.fd, &__readdir_buf, sizeof(__readdir_buf)));
    __readdir_bufpos = 0;
    if ( __readdir_nread <= 0 ) return __impl_dir{ "", dt_end, 0 };
  }

  __linux_kernel_dirent64 *p = reinterpret_cast<__linux_kernel_dirent64 *>(__readdir_buf + __readdir_bufpos);
  __readdir_bufpos += p->d_reclen;
  return { p->d_name, p->d_type, static_cast<ino_t>(p->d_ino) };
}

struct readdir_ctx {
  char buf[8192];
  usize bufpos = 0;
  usize nread = 0;
};

inline __impl_dir
readdir_r(const fd_t &_f, readdir_ctx &ctx)
{
  if ( ctx.bufpos >= ctx.nread ) {
    ctx.nread = static_cast<usize>(posix::getdents64(_f.fd, ctx.buf, sizeof(ctx.buf)));
    ctx.bufpos = 0;
    if ( ctx.nread <= 0 ) return { "", dt_end, 0 };
  }

  __linux_kernel_dirent64 *p = reinterpret_cast<__linux_kernel_dirent64 *>(ctx.buf + ctx.bufpos);
  ctx.bufpos += p->d_reclen;
  return { p->d_name, p->d_type, static_cast<ino_t>(p->d_ino) };
}

inline i32
mkdir_d(const char *path)
{
  return (posix::mkdir(path, mode_dir));
}

inline i32
mkdir_at(fd_t dirfd, const char *name, u32 mode = mode_dir)
{
  return (posix::mkdirat(dirfd.fd, name, mode));
}

// mkdir -p equiv, make all parents
inline i32
mkdir_p(const char *path, u32 mode = mode_dir)
{
  char buf[path_max];
  usize i = 0;
  while ( path[i] && i < path_max - 1 ) {
    buf[i] = path[i];
    ++i;
  }
  buf[i] = '\0';

  for ( usize j = 1; j <= i; ++j ) {
    if ( buf[j] == '/' || buf[j] == '\0' ) {
      char saved = buf[j];
      buf[j] = '\0';
      i32 r = posix::mkdir(buf, mode);
      // eexist, not loading in errno
      if ( r < 0 && static_cast<u32>(-r) != 17u ) return r;
      buf[j] = saved;
    }
  }
  return 0;
}

inline i32
remove(const char *path)
{
  i32 r = posix::unlink(path);
  if ( r < 0 ) r = (posix::rmdir(path));
  return r;
}

inline bool
path_exists(const char *path)
{
  return posix::access(path, f_ok) == 0;
}

template <typename T>
inline max_t
write_all(fd_t fd, const T &buf, usize len)
{
  usize written = 0;
  const byte *p = real_addr_as<const byte>(buf);
  while ( written < len ) {
    max_t r = posix::write(fd.fd, p + written, len - written);
    if ( r < 0 ) return r;
    if ( r == 0 ) break;
    written += static_cast<usize>(r);
  }
  return static_cast<max_t>(written);
}

template <typename T>
inline max_t
write_all(fd_t fd, const T *buf, usize len)
{
  usize written = 0;
  const byte *p = static_cast<const byte *>(buf);
  while ( written < len ) {
    max_t r = posix::write(fd.fd, p + written, len - written);
    if ( r < 0 ) return r;
    if ( r == 0 ) break;
    written += static_cast<usize>(r);
  }
  return static_cast<max_t>(written);
}

template <typename T>
inline max_t
read_all(fd_t fd, T *buf, usize len)
{
  usize got = 0;
  byte *p = static_cast<byte *>(buf);
  while ( got < len ) {
    max_t r = posix::read(fd.fd, p + got, len - got);
    if ( r < 0 ) return r;
    if ( r == 0 ) break;     // EOF
    got += static_cast<usize>(r);
  }
  return static_cast<max_t>(got);
}

inline posix::off_t
seek_to(fd_t fd, posix::off_t offset)
{
  return (posix::lseek(fd.fd, offset, seek_set));
}

inline posix::off_t
seek_by(fd_t fd, posix::off_t delta)
{
  return (posix::lseek(fd.fd, delta, seek_cur));
}

inline posix::off_t
tell(fd_t fd)
{
  return (posix::lseek(fd.fd, 0, seek_cur));
}

inline i32
truncate(fd_t fd, posix::off_t length)
{
  return (posix::ftruncate(fd.fd, length));
}

inline i32
clear_file(fd_t fd)
{
  return truncate(fd, 0);
}

inline i32
flush(fd_t fd)
{
  return (posix::fsync(fd.fd));
}

inline i32
flush_data(fd_t fd)
{
  return (posix::fdatasync(fd.fd));
}

inline max_t
copy_fd(fd_t out_fd, fd_t in_fd, usize count)
{
  return posix::sendfile(out_fd.fd, in_fd.fd, nullptr, count);
}

inline i32
copy_file(const char *src, const char *dst, u32 mode = mode_file)
{
  fd_t in = open_read(src);
  if ( !in ) return in.fd;

  fd_t out = open_write(dst, mode);
  if ( !out ) {
    close_fd(in);
    return out.fd;
  }

  posix::off_t sz = file_size(in);
  i32 r = 0;
  if ( sz > 0 ) {
    max_t sent = copy_fd(out, in, static_cast<usize>(sz));
    r = (sent < 0) ? static_cast<i32>(sent) : 0;
  }
  close_fd(in);
  close_fd(out);
  return r;
}

struct dir_handle {
  dir_t fd;

  dir_handle() : fd(invalid_fd) {}

  explicit dir_handle(const char *path) : fd(opendir(path)) {}

  explicit dir_handle(dir_t d) : fd(d) {}

  dir_handle(const dir_handle &) = delete;
  dir_handle &operator=(const dir_handle &) = delete;

  dir_handle(dir_handle &&o) noexcept : fd(o.fd) { o.fd.reset(); }

  ~dir_handle()
  {
    if ( fd.open() ) close_fd(fd);
  }

  bool
  valid() const
  {
    return fd.open();
  }

  explicit
  operator bool() const
  {
    return valid();
  }

  __impl_dir
  next()
  {
    if ( !valid() ) return { "", dt_end, 0 };
    return readdir(fd);
  }

  void
  rewind()
  {
    micron::syscall(SYS_lseek, fd.fd, 0, seek_set);
  }
};

template <typename Fn>
inline void
for_each_entry(const char *path, Fn &&fn)
{
  dir_handle dh(path);
  if ( !dh ) return;

  readdir_ctx ctx{};
  for ( ;; ) {
    __impl_dir e = readdir_r(dh.fd, ctx);
    if ( e.at_end() ) break;
    // Skip self and parent entries
    if ( e.d_name[0] == '.' && (e.d_name[1] == '\0' || (e.d_name[1] == '.' && e.d_name[2] == '\0')) ) continue;
    fn(e);
  }
}

};     // namespace posix

};     // namespace micron
