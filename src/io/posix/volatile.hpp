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

#include "../../linux/io/inode.hpp"
#include "../../linux/sys/fcntl.hpp"
#include "../../linux/sys/ioctl.hpp"
#include "../../linux/sys/limits.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/mmap_bits.hpp"
#include "../bits.hpp"

#include "../../linux/io.hpp"
#include "file.hpp"

namespace micron
{
namespace io
{

inline constexpr u32 f_seal_seal = 0x0001;
inline constexpr u32 f_seal_shrink = 0x0002;
inline constexpr u32 f_seal_grow = 0x0004;
inline constexpr u32 f_seal_write = 0x0008;
inline constexpr u32 f_add_seals = 1033;
inline constexpr u32 f_get_seals = 1034;

class volatile_t
{
  inline bool
  __alive(void) const
  {
    if ( __handle.fd == posix::invalid_fd ) [[unlikely]] {
      exc<except::io_error>("micron::volatile_t, fd isn't open.");
      return false;
    }
    return true;
  }

  inline void
  __create(const char *name, unsigned int flags)
  {
    __handle.fd = micron::memfd_create(name, flags);
    if ( __handle.has_error() ) exc<except::io_error>("micron::volatile_t, memfd_create failed.");
    micron::zero(&sd);
  }

  inline void
  __clone_fd(i32 src_fd, usize hint_size = 0)
  {
    if ( hint_size == 0 ) {
      stat_t tmp{};
      if ( posix::fstat(fd_t{ src_fd }, tmp) != posix::invalid_fd ) hint_size = static_cast<usize>(tmp.st_size);
    }

    if ( hint_size > 0 ) posix::ftruncate(__handle, static_cast<posix::off_t>(hint_size));

    posix::off_t src_off = 0;
    posix::off_t dst_off = 0;
    usize remaining = hint_size ? hint_size : static_cast<usize>(-1);
    usize chunk = (1u << 20);     // 1mb

    if ( hint_size > 0 ) {
      while ( remaining > 0 ) {
        usize want = remaining < chunk ? remaining : chunk;
        max_t n = micron::copy_file_range(src_fd, &src_off, __handle.fd, &dst_off, want, 0u);
        if ( n <= 0 ) break;
        remaining -= static_cast<usize>(n);
      }
    } else {
      i32 pfd[2];
      posix::pipe(pfd);
      for ( ;; ) {
        max_t spliced = micron::splice(src_fd, nullptr, pfd[1], nullptr, chunk, 0u);
        if ( spliced <= 0 ) {
          posix::close(pfd[0]);
          posix::close(pfd[1]);
          break;
        }
        max_t moved = micron::splice(pfd[0], nullptr, __handle.fd, nullptr, static_cast<usize>(spliced), 0u);
        (void)moved;
      }
    }

    posix::lseek(__handle, 0, posix::seek_set);
    micron::zero(&sd);
  }

public:
  micron::sstr<max_name> fname;
  fd_t __handle;
  mutable stat_t sd;

  ~volatile_t() { close(); }

  volatile_t() : fname(), __handle(-1), sd() { __create("micron_volatile", mfd_cloexec); }

  explicit volatile_t(const char *name, unsigned int flags = mfd_cloexec) : fname(name), __handle(-1), sd() { __create(name, flags); }

  template <is_string T> explicit volatile_t(const T &name, unsigned int flags = mfd_cloexec) : volatile_t(name.c_str(), flags) {}

  explicit volatile_t(const char *name, usize initial_size, unsigned int flags) : fname(name), __handle(-1), sd()
  {
    __create(name, flags);
    posix::ftruncate(__handle, static_cast<posix::off_t>(initial_size));
  }

  volatile_t(const volatile_t &o) : fname(o.fname), __handle(-1), sd()
  {
    __create(fname.c_str(), mfd_cloexec);
    if ( o.__handle.fd != posix::invalid_fd.fd ) __clone_fd(o.__handle.fd);
  }

  volatile_t(volatile_t &&o) noexcept : fname(micron::move(o.fname)), __handle(o.__handle), sd(o.sd)
  {
    o.__handle = posix::invalid_fd;
    micron::zero(&o.sd);
  }

  volatile_t &
  operator=(volatile_t &&o) noexcept
  {
    close();
    fname = micron::move(o.fname);
    __handle = o.__handle;
    sd = o.sd;
    o.__handle = posix::invalid_fd;
    micron::zero(&o.sd);
    return *this;
  }

  volatile_t &
  operator=(const volatile_t &o)
  {
    if ( this == &o ) return *this;
    close();
    fname = o.fname;
    __create(fname.c_str(), mfd_cloexec);
    if ( o.__handle.fd != posix::invalid_fd.fd ) __clone_fd(o.__handle.fd);
    return *this;
  }

  volatile_t &
  operator=(fd_t src)
  {
    __alive();

    posix::ftruncate(__handle, 0);
    posix::lseek(__handle, 0, posix::seek_set);
    micron::zero(&sd);
    __clone_fd(src.fd);
    return *this;
  }

  volatile_t &
  operator=(const file &src)
  {
    return (*this = src.fd());
  }

  void
  close(void)
  {
    if ( __handle.closed() ) return;
    posix::close(__handle.fd);
    __handle.fd = posix::invalid_fd.fd;
  }

  bool
  valid(void) const noexcept
  {
    return !__handle.closed();
  }

  explicit
  operator bool(void) const noexcept
  {
    return valid();
  }

  fd_t
  fd(void) const noexcept
  {
    return __handle;
  }

  i32
  raw_fd(void) const noexcept
  {
    return __handle.fd;
  }

  const auto &
  name(void) const noexcept
  {
    return fname;
  }

  template <bool B = STAT_EXISTING>
  inline void
  stat(void) const
  {
    if constexpr ( B == STAT_EXISTING ) {
      if ( !micron::is_zero(&sd) ) return;
      __alive();
      if ( posix::fstat(__handle, sd) == posix::invalid_fd ) exc<except::io_error>("micron::volatile_t, fstat failed.");
    } else {
      __alive();
      if ( posix::fstat(__handle, sd) == posix::invalid_fd ) exc<except::io_error>("micron::volatile_t, fstat failed.");
    }
  }

  const stat_t &
  refresh_stat(void)
  {
    stat<STAT_OVERRIDE>();
    return sd;
  }

  inline auto
  size(void) const
  {
    __alive();
    stat();
    return sd.st_size;
  }

  posix::ino_t
  inode(void)
  {
    __alive();
    stat();
    return sd.st_ino;
  }

  posix::dev_t
  device(void)
  {
    __alive();
    stat();
    return sd.st_dev;
  }

  posix::nlink_t
  link_count(void)
  {
    __alive();
    stat();
    return sd.st_nlink;
  }

  posix::uid_t
  uid(void)
  {
    __alive();
    stat();
    return sd.st_uid;
  }

  posix::gid_t
  gid(void)
  {
    __alive();
    stat();
    return sd.st_gid;
  }

  posix::time_t
  mtime(void)
  {
    __alive();
    stat();
    return sd.st_mtime;
  }

  posix::time_t
  ctime(void)
  {
    __alive();
    stat();
    return sd.st_ctime;
  }

  posix::blksize_t
  blksize(void)
  {
    __alive();
    stat();
    return sd.st_blksize;
  }

  posix::blkcnt_t
  blocks(void)
  {
    __alive();
    stat();
    return sd.st_blocks;
  }

  posix::mode_t
  mode(void)
  {
    __alive();
    stat();
    return sd.st_mode;
  }

  inline linux_permissions
  permissions(void)
  {
    __alive();
    stat();
    linux_permissions perms = linux_permissions::from_mode(sd.st_mode);
    perms.setuid = (sd.st_mode & posix::s_isuid) != 0;
    perms.setgid = (sd.st_mode & posix::s_isgid) != 0;
    perms.sticky = (sd.st_mode & posix::s_isvtx) != 0;
    return perms;
  }

  inline void
  set_permissions(const linux_permissions &perms)
  {
    __alive();
    posix::fchmod(__handle, perms.full_mode());
    micron::zero(&sd);
  }

  bool
  is_empty(void)
  {
    return size() == 0;
  }

  template <typename T>
  max_t
  read(T *ptr, usize len) const
  {
    if ( __handle.invalid() || ptr == nullptr ) return -1;
    return posix::read(__handle.fd, static_cast<void *>(ptr), len);
  }

  template <typename T>
  max_t
  write(const T *ptr, usize len)
  {
    if ( __handle.invalid() || ptr == nullptr ) return -1;
    return posix::write(__handle.fd, static_cast<const void *>(ptr), len);
  }

  template <typename T>
  max_t
  read_all(T *ptr, usize len) const
  {
    if ( __handle.invalid() || ptr == nullptr ) return -1;
    return posix::read_all(__handle, static_cast<void *>(ptr), len);
  }

  template <typename T>
  max_t
  write_all(const T *ptr, usize len)
  {
    if ( __handle.invalid() || ptr == nullptr ) return -1;
    return posix::write_all(__handle, static_cast<const void *>(ptr), len);
  }

  template <is_iterable_container T>
  max_t
  read(T &buf) const
  {
    if ( __handle.invalid() ) return -1;
    return posix::read(__handle.fd, buf.data(), buf.max_size());
  }

  template <is_iterable_container T>
  max_t
  write(const T &buf)
  {
    if ( __handle.invalid() ) return -1;
    return posix::write(__handle.fd, buf.data(), buf.size());
  }

  template <is_iterable_container T>
  max_t
  read_all(T &buf) const
  {
    if ( __handle.invalid() ) return -1;
    return posix::read_all(__handle, buf.data(), buf.max_size());
  }

  template <is_iterable_container T>
  max_t
  write_all(const T &buf)
  {
    if ( __handle.invalid() ) return -1;
    return posix::write_all(__handle, buf.data(), buf.size());
  }

  template <is_string T>
  max_t
  write_all(const T &str)
  {
    if ( __handle.invalid() ) return -1;
    return posix::write_all(__handle, str.data(), str.size());
  }

  template <is_iterable_container T>
  volatile_t &
  operator>>(T &buf)
  {
    rewind();
    read_all(buf);
    return *this;
  }

  template <is_string T>
  volatile_t &
  operator>>(T &str)
  {
    rewind();
    read_all(str);
    return *this;
  }

  template <is_iterable_container T>
  volatile_t &
  operator<<(const T &buf)
  {
    write(buf);
    return *this;
  }

  template <is_string T>
  volatile_t &
  operator<<(const T &str)
  {
    write(str);
    return *this;
  }

  volatile_t &
  operator<<(const char *str)
  {
    if ( __handle.invalid() || str == nullptr ) return *this;
    posix::write(__handle.fd, str, micron::strlen(str));
    return *this;
  }

  template <is_iterable_container T>
  max_t
  pread(T &buf, usize len, posix::off_t offset) const
  {
    if ( __handle.invalid() ) return -1;
    return micron::pread(__handle.fd, buf.data(), len, offset);
  }

  template <is_iterable_container T>
  max_t
  pwrite(const T &buf, usize len, posix::off_t offset)
  {
    if ( __handle.invalid() ) return -1;
    return micron::pwrite(__handle.fd, buf.data(), len, offset);
  }

  template <typename T>
  max_t
  pread(T &buf, usize len, posix::off_t offset) const
  {
    return micron::pread(__handle.fd, micron::voidify(buf), len, offset);
  }

  template <typename T>
  max_t
  pwrite(const T &buf, usize len, posix::off_t offset)
  {
    return micron::pwrite(__handle.fd, micron::voidify(buf), len, offset);
  }

  posix::off_t
  seek_to(posix::off_t offset)
  {
    return posix::seek_to(__handle, offset);
  }

  posix::off_t
  seek_by(posix::off_t delta)
  {
    return posix::seek_by(__handle, delta);
  }

  posix::off_t
  seek_end(posix::off_t off = 0)
  {
    return posix::lseek(__handle, posix::seek_end - off, posix::seek_end);
  }

  posix::off_t
  tell(void) const
  {
    return posix::tell(__handle);
  }

  void
  rewind(void)
  {
    seek_to(0);
  }

  i32
  truncate(posix::off_t length)
  {
    i32 r = posix::ftruncate(__handle, length);
    if ( r == 0 ) micron::zero(&sd);
    return r;
  }

  i32
  resize(posix::off_t new_size)
  {
    return truncate(new_size);
  }

  i32
  clear(void)
  {
    i32 r = truncate(0);
    if ( r == 0 ) rewind();
    return r;
  }

  i32
  reserve(posix::off_t size)
  {
    if ( this->size() < size ) return truncate(size);
    return 0;
  }

  i32
  flush(void)
  {
    return posix::flush(__handle);
  }

  i32
  flush_data(void)
  {
    return posix::flush_data(__handle);
  }

  i32
  sync(void)
  {
    return flush();
  }

  void *
  map_ro(void) const
  {
    __alive();
    usize sz = static_cast<usize>(size());
    if ( sz == 0 ) return nullptr;
    void *p = micron::mmap(nullptr, sz, prot_read, map_shared, __handle.fd, 0);
    return micron::mmap_failed(p) ? nullptr : p;
  }

  void *
  map_rw(void)
  {
    __alive();
    usize sz = static_cast<usize>(size());
    if ( sz == 0 ) return nullptr;
    void *p = micron::mmap(nullptr, sz, prot_read | prot_write, map_shared, __handle.fd, 0);
    return micron::mmap_failed(p) ? nullptr : p;
  }

  void
  unmap(void *addr, usize len)
  {
    micron::munmap(reinterpret_cast<addr_t *>(addr), len);
  }

  max_t
  sendfile_to(i32 dst_fd, usize count, posix::off_t *offset = nullptr) const
  {
    __alive();
    return posix::sendfile(dst_fd, __handle.fd, offset, count);
  }

  max_t
  sendfile_to(fd_t dst, usize count, posix::off_t *offset = nullptr) const
  {
    __alive();
    return posix::sendfile(dst.fd, __handle.fd, offset, count);
  }

  max_t
  sendfile_to(const file &dst, usize count, posix::off_t *offset = nullptr) const
  {
    return sendfile_to(dst.raw_fd(), count, offset);
  }

  max_t
  sendfile_all_to(i32 dst_fd) const
  {
    __alive();
    posix::off_t off = 0;
    return posix::sendfile(dst_fd, __handle.fd, &off, static_cast<usize>(size()));
  }

  max_t
  sendfile_all_to(fd_t dst) const
  {
    return sendfile_all_to(dst.fd);
  }

  max_t
  sendfile_all_to(const file &dst) const
  {
    return sendfile_all_to(dst.raw_fd());
  }

  max_t
  sendfile_from(i32 src_fd, usize count, posix::off_t *offset = nullptr)
  {
    __alive();
    return posix::sendfile(__handle.fd, src_fd, offset, count);
  }

  max_t
  sendfile_from(fd_t src, usize count, posix::off_t *offset = nullptr)
  {
    return sendfile_from(src.fd, count, offset);
  }

  max_t
  sendfile_from(const file &src, usize count, posix::off_t *offset = nullptr)
  {
    return sendfile_from(src.raw_fd(), count, offset);
  }

  max_t
  splice_to(i32 dst_fd, usize count, posix::off_t *src_off = nullptr, posix::off_t *dst_off = nullptr, u32 flags = 0) const
  {
    __alive();
    return micron::splice(__handle.fd, src_off, dst_fd, dst_off, count, flags);
  }

  max_t
  splice_to(fd_t dst, usize count, posix::off_t *src_off = nullptr, posix::off_t *dst_off = nullptr, u32 flags = 0) const
  {
    return splice_to(dst.fd, count, src_off, dst_off, flags);
  }

  max_t
  splice_to(const file &dst, usize count, posix::off_t *src_off = nullptr, posix::off_t *dst_off = nullptr, u32 flags = 0) const
  {
    return splice_to(dst.raw_fd(), count, src_off, dst_off, flags);
  }

  max_t
  splice_from(i32 src_fd, usize count, posix::off_t *src_off = nullptr, posix::off_t *dst_off = nullptr, u32 flags = 0)
  {
    __alive();
    return micron::splice(src_fd, src_off, __handle.fd, dst_off, count, flags);
  }

  max_t
  splice_from(fd_t src, usize count, posix::off_t *src_off = nullptr, posix::off_t *dst_off = nullptr, u32 flags = 0)
  {
    return splice_from(src.fd, count, src_off, dst_off, flags);
  }

  max_t
  splice_from(const file &src, usize count, posix::off_t *src_off = nullptr, posix::off_t *dst_off = nullptr, u32 flags = 0)
  {
    return splice_from(src.raw_fd(), count, src_off, dst_off, flags);
  }

  max_t
  copy_to(file &dst, usize count, posix::off_t src_off = -1, posix::off_t dst_off = -1) const
  {
    posix::off_t *sp = (src_off < 0) ? nullptr : &src_off;
    posix::off_t *dp = (dst_off < 0) ? nullptr : &dst_off;
    return micron::copy_file_range(__handle.fd, sp, dst.raw_fd(), dp, count, 0u);
  }

  max_t
  copy_to(volatile_t &dst, usize count, posix::off_t src_off = -1, posix::off_t dst_off = -1) const
  {
    posix::off_t *sp = (src_off < 0) ? nullptr : &src_off;
    posix::off_t *dp = (dst_off < 0) ? nullptr : &dst_off;
    return micron::copy_file_range(__handle.fd, sp, dst.__handle.fd, dp, count, 0u);
  }

  max_t
  copy_from(const file &src, usize count, posix::off_t src_off = -1, posix::off_t dst_off = -1)
  {
    posix::off_t *sp = (src_off < 0) ? nullptr : &src_off;
    posix::off_t *dp = (dst_off < 0) ? nullptr : &dst_off;
    max_t r = micron::copy_file_range(src.raw_fd(), sp, __handle.fd, dp, count, 0u);
    if ( r > 0 ) micron::zero(&sd);
    return r;
  }

  max_t
  copy_from(const volatile_t &src, usize count, posix::off_t src_off = -1, posix::off_t dst_off = -1)
  {
    posix::off_t *sp = (src_off < 0) ? nullptr : &src_off;
    posix::off_t *dp = (dst_off < 0) ? nullptr : &dst_off;
    max_t r = micron::copy_file_range(src.__handle.fd, sp, __handle.fd, dp, count, 0u);
    if ( r > 0 ) micron::zero(&sd);
    return r;
  }

  fd_t
  dup(void) const
  {
    return fd_t{ micron::dup(__handle.fd) };
  }

  fd_t
  dup2(i32 target) const
  {
    return fd_t{ micron::dup2(__handle.fd, target) };
  }

  volatile_t
  clone(void) const
  {
    volatile_t v(fname.c_str(), mfd_cloexec);
    if ( __handle.fd != posix::invalid_fd.fd ) v.__clone_fd(__handle.fd, static_cast<usize>(size()));
    return v;
  }

  i32
  get_status_flags(void) const
  {
    return posix::fcntl(__handle.fd, posix::f_getfl);
  }

  i32
  set_status_flags(i32 flags)
  {
    return posix::fcntl(__handle.fd, posix::f_setfl, flags);
  }

  i32
  get_fd_flags(void) const
  {
    return posix::fcntl(__handle.fd, posix::f_getfd);
  }

  i32
  set_fd_flags(i32 flags)
  {
    return posix::fcntl(__handle.fd, posix::f_setfd, flags);
  }

  bool
  is_cloexec(void) const
  {
    i32 f = get_fd_flags();
    return f >= 0 && (f & 1);
  }

  i32
  set_cloexec(bool on = true)
  {
    i32 cur = get_fd_flags();
    if ( cur < 0 ) return cur;
    return set_fd_flags(on ? (cur | 1) : (cur & ~1));
  }

  bool
  is_nonblock(void) const
  {
    i32 f = get_status_flags();
    return f >= 0 && (f & posix::o_nonblock);
  }

  i32
  set_nonblock(bool on = true)
  {
    i32 cur = get_status_flags();
    if ( cur < 0 ) return cur;
    return set_status_flags(on ? (cur | posix::o_nonblock) : (cur & ~posix::o_nonblock));
  }

  template <typename... Args>
  auto
  fcntl(i32 cmd, Args &&...args)
  {
    return posix::fcntl(__handle.fd, cmd, args...);
  }

  i32
  add_seals(i32 seal_flags)
  {
    __alive();
    return posix::fcntl(__handle.fd, f_add_seals, seal_flags);
  }

  i32
  get_seals(void) const
  {
    __alive();
    return posix::fcntl(__handle.fd, f_get_seals);
  }

  i32
  seal_write(void)
  {
    return add_seals(f_seal_write | f_seal_grow | f_seal_shrink);
  }

  i32
  seal_all(void)
  {
    return add_seals(f_seal_write | f_seal_grow | f_seal_shrink | f_seal_seal);
  }

  bool
  is_write_sealed(void) const
  {
    i32 s = get_seals();
    return s >= 0 && (s & f_seal_write);
  }

  i32
  lock_shared(void)
  {
    posix::flock_t fl{ 0, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return posix::fcntl(__handle.fd, posix::f_setlkw, &fl);
  }

  i32
  lock_exclusive(void)
  {
    posix::flock_t fl{ 1, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return posix::fcntl(__handle.fd, posix::f_setlkw, &fl);
  }

  i32
  try_lock_shared(void)
  {
    posix::flock_t fl{ 0, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return posix::fcntl(__handle.fd, posix::f_setlk, &fl);
  }

  i32
  try_lock_exclusive(void)
  {
    posix::flock_t fl{ 1, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return posix::fcntl(__handle.fd, posix::f_setlk, &fl);
  }

  i32
  unlock(void)
  {
    posix::flock_t fl{ 2, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return posix::fcntl(__handle.fd, posix::f_setlk, &fl);
  }

  i32
  advise(posix::off_t offset, posix::off_t len, i32 advice)
  {
    return posix::fadvise(__handle, offset, len, advice);
  }

  i32
  advise_sequential(void)
  {
    return advise(0, 0, posix::fadv_sequential);
  }

  i32
  advise_random(void)
  {
    return advise(0, 0, posix::fadv_random);
  }

  i32
  advise_willneed(posix::off_t offset, posix::off_t len)
  {
    return advise(offset, len, posix::fadv_willneed);
  }

  i32
  advise_dontneed(posix::off_t offset, posix::off_t len)
  {
    return advise(offset, len, posix::fadv_dontneed);
  }

  max_t
  getxattr(const char *attr_name, void *value, usize sz) const
  {
    return posix::fgetxattr(__handle.fd, attr_name, value, sz);
  }

  i32
  setxattr(const char *attr_name, const void *value, usize sz, i32 flags = 0)
  {
    return posix::fsetxattr(__handle.fd, attr_name, value, sz, flags);
  }

  i32
  removexattr(const char *attr_name)
  {
    return posix::fremovexattr(__handle.fd, attr_name);
  }

  max_t
  listxattr(char *list, usize sz) const
  {
    return posix::flistxattr(__handle.fd, list, sz);
  }

  static volatile_t
  sealable(const char *name = "micron_volatile_sealed")
  {
    return volatile_t{ name, mfd_cloexec | mfd_allow_sealing };
  }

  static volatile_t
  with_size(posix::off_t sz, const char *name = "micron_volatile")
  {
    volatile_t v{ name, static_cast<usize>(sz), mfd_cloexec };
    return v;
  }

  static volatile_t
  from_file(const file &src, const char *name = "micron_volatile")
  {
    volatile_t v{ name, mfd_cloexec };
    v.__clone_fd(src.raw_fd(), static_cast<usize>(const_cast<file &>(src).size()));
    return v;
  }

  static volatile_t
  from_fd(fd_t src, const char *name = "micron_volatile")
  {
    volatile_t v{ name, mfd_cloexec };
    v.__clone_fd(src.fd);
    return v;
  }
};

inline volatile_t
make_volatile(const char *name = "micron_volatile")
{
  return volatile_t{ name };
}

inline volatile_t
make_volatile_sized(posix::off_t sz, const char *name = "micron_volatile")
{
  return volatile_t::with_size(sz, name);
}

inline volatile_t
make_volatile_sealable(const char *name = "micron_volatile")
{
  return volatile_t::sealable(name);
}

inline volatile_t
make_volatile_from_file(const file &src)
{
  return volatile_t::from_file(src);
}

inline volatile_t
make_volatile_from_fd(fd_t src)
{
  return volatile_t::from_fd(src);
}

};     // namespace io
};     // namespace micron
