//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../string/strings.hpp"
#include "../sum.hpp"

#include "cached_file.hpp"
#include "file.hpp"
#include "ftw.hpp"
#include "paths.hpp"
#include "realpath.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// filesystem free functions

namespace micron
{
// fsys namespace deprecated
namespace io
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// existence / type / permission queries

inline posix::node_types
file_type(const io::path_t &p)
{
  stat_t st{};
  return posix::get_type(p.c_str(), st);
}

inline posix::off64_t
file_size(const io::path_t &p)
{
  stat_t st{};
  if ( posix::stat(p.c_str(), st) != 0 ) return -1;
  return st.st_size;
}

inline time64_t
mtime(const io::path_t &p)
{
  stat_t st{};
  if ( posix::stat(p.c_str(), st) != 0 ) return 0;
  return st.st_mtime;
}

inline time64_t
atime(const io::path_t &p)
{
  stat_t st{};
  if ( posix::stat(p.c_str(), st) != 0 ) return 0;
  return st.st_atime;
}

inline time64_t
ctime(const io::path_t &p)
{
  stat_t st{};
  if ( posix::stat(p.c_str(), st) != 0 ) return 0;
  return st.st_ctime;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// handle openers
inline io::file
open_file(const io::path_t &p, const io::modes m = io::modes::read, const open_opts &opts = {})
{
  i32 fd = static_cast<i32>(posix::open(p.c_str(), compose_open_flags(m, opts), opts.perms));
  const bool __chk = (m != io::modes::append && m != io::modes::create && m != io::modes::readwritecreate);
  if ( fd >= 0 && __chk ) {
    stat_t st{};
    if ( i32 e = static_cast<i32>(posix::fstat(fd_t{ fd }, st)); e != 0 ) [[unlikely]] {
      posix::close(fd);
      return io::file(fd_t{ e }, p.c_str());
    }
    if ( !(posix::__impl::stat_is_reg(st) || posix::__impl::stat_is_chr(st)) ) [[unlikely]] {
      posix::close(fd);
      return io::file(fd_t{ posix::__impl::stat_is_dir(st) ? -error::is_a_dir : -error::invalid_arg }, p.c_str());
    }
  }
  return io::file(fd_t{ fd }, p.c_str());
}

inline io::file
create_file(const io::path_t &p, u32 mode = posix::mode_file)
{
  i32 fd = static_cast<i32>(
      posix::openat(posix::at_fdcwd, p.c_str(), posix::o_wronly | posix::o_create | posix::o_excl | posix::o_cloexec, mode));
  return io::file(fd_t{ fd }, p.c_str());
}

inline max_t
make(const io::path_t &p, u32 mode = posix::mode_file)
{
  io::file f = create_file(p, mode);
  return f.valid() ? 0 : f.raw_fd();
}

inline io::file
tmpfile(const io::path_t &dir)
{
  i32 fd = static_cast<i32>(
      posix::openat(posix::at_fdcwd, dir.c_str(), posix::o_tmpfile | posix::o_rdwr | posix::o_cloexec, posix::mode_file));
  return io::file(fd_t{ fd }, dir.c_str());
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// whole file value io
template<typename T = micron::string>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
micron::option<T, io::error_t>
read_file(const io::path_t &p)
{
  io::file f = open_file(p, io::modes::read);
  if ( !f.valid() ) [[unlikely]]      // open_file rejects directories/unreadable nodes -> -errno lands here (EISDIR, ...)
    return micron::option<T, io::error_t>{ io::error_t(f.raw_fd()) };
  const posix::off64_t sz = f.size();
  if ( sz > 0 ) {
    T out{};
    out.reserve(static_cast<usize>(sz) + 1);
    out.set_size(static_cast<usize>(sz));
    max_t r = f.read(out);
    if ( r < 0 ) [[unlikely]]
      return micron::option<T, io::error_t>{ io::error_t(r) };
    out.set_size(static_cast<usize>(r));      // trim to bytes actually read (file may have shrunk since fstat)
    return micron::option<T, io::error_t>{ micron::move(out) };
  }
  T out{};
  if constexpr ( requires(T t, typename T::value_type v) { t.push_back(v); } ) {
    byte chunk[4096];
    for ( ;; ) {
      max_t r = f.read(static_cast<void *>(chunk), sizeof(chunk));
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        return micron::option<T, io::error_t>{ io::error_t(r) };
      }
      if ( r == 0 ) break;
      for ( max_t i = 0; i < r; ++i ) out.push_back(static_cast<typename T::value_type>(chunk[i]));
    }
  }
  return micron::option<T, io::error_t>{ micron::move(out) };
}

template<typename T>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
max_t
read_file(const io::path_t &p, T &target)
{
  io::file f = open_file(p, io::modes::read);
  if ( !f.valid() ) [[unlikely]]
    return f.raw_fd();
  const posix::off64_t sz = f.size();
  if ( sz > 0 ) {
    if constexpr ( requires(T t, usize n) {
                     t.reserve(n);
                     t.set_size(n);
                   } ) {
      target.reserve(static_cast<usize>(sz) + 1);
      target.set_size(static_cast<usize>(sz));
      max_t r = f.read(target);
      if ( r < 0 ) [[unlikely]] {
        target.set_size(0);
        return r;
      }
      target.set_size(static_cast<usize>(r));
      return r;
    } else {
      if constexpr ( requires(T t) { t.max_size(); } ) {
        if ( static_cast<usize>(sz) > target.max_size() ) [[unlikely]]
          return -error::file_too_big;
      }
      max_t r = f.read(static_cast<void *>(target.data()), static_cast<usize>(sz));
      if ( r < 0 ) [[unlikely]]
        return r;
      if constexpr ( requires(T t, usize n) { t.set_size(n); } ) target.set_size(static_cast<usize>(r));
      return r;
    }
  }
  if constexpr ( requires(T t, typename T::value_type v) { t.push_back(v); } ) {
    if constexpr ( requires(T t) { t.fast_clear(); } )
      target.fast_clear();
    else if constexpr ( requires(T t) { t.clear(); } )
      target.clear();
    else if constexpr ( requires(T t, usize n) { t.set_size(n); } )
      target.set_size(0);
    byte chunk[4096];
    max_t total = 0;
    for ( ;; ) {
      max_t r = f.read(static_cast<void *>(chunk), sizeof(chunk));
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        return r;
      }
      if ( r == 0 ) break;
      for ( max_t i = 0; i < r; ++i ) target.push_back(static_cast<typename T::value_type>(chunk[i]));
      total += r;
    }
    return total;
  } else {
    return f.read(target);
  }
}

// create/truncate p and write c through the universal marshalling tiers
template<typename C>
max_t
write_file(const io::path_t &p, const C &c)
{
  i32 fd = static_cast<i32>(
      posix::openat(posix::at_fdcwd, p.c_str(), posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec, posix::mode_file));
  if ( fd < 0 ) [[unlikely]]
    return fd;
  io::file f(fd_t{ fd }, p.c_str());
  max_t n = f.write(c);
  if ( n >= 0 ) f.flush();
  return n;
}

template<typename C>
max_t
append_file(const io::path_t &p, const C &c)
{
  i32 fd = static_cast<i32>(
      posix::openat(posix::at_fdcwd, p.c_str(), posix::o_wronly | posix::o_create | posix::o_append | posix::o_cloexec, posix::mode_file));
  if ( fd < 0 ) [[unlikely]]
    return fd;
  io::file f(fd_t{ fd }, p.c_str());
  max_t n = f.write(c);
  if ( n >= 0 ) f.flush();
  return n;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mutations
inline max_t
rename(const io::path_t &from, const io::path_t &to)
{
  return posix::rename(from.c_str(), to.c_str());
}

inline max_t
unlink(const io::path_t &p)
{
  return posix::unlink(p.c_str());
}

inline max_t
remove(const io::path_t &p)
{
  return posix::unlink(p.c_str());
}

inline max_t
mkdir(const io::path_t &p, u32 mode = posix::mode_dir)
{
  return posix::mkdir(p.c_str(), mode);
}

inline max_t
mkdir_p(const io::path_t &p, u32 mode = posix::mode_dir)
{
  return posix::mkdir_p(p.c_str(), mode);
}

inline max_t
rmdir(const io::path_t &p)
{
  return posix::rmdir(p.c_str());
}

inline max_t
symlink(const io::path_t &target, const io::path_t &linkpath)
{
  return posix::symlink(target.c_str(), linkpath.c_str());
}

inline max_t
hardlink(const io::path_t &oldpath, const io::path_t &newpath)
{
  return posix::link(oldpath.c_str(), newpath.c_str());
}

inline io::path_t
readlink(const io::path_t &p)
{
  char buf[posix::path_max];
  max_t n = posix::readlink(p.c_str(), buf, sizeof(buf) - 1);
  if ( n < 0 ) return io::path_t();
  buf[n] = '\0';
  return io::path_t(buf);
}

inline max_t
chmod(const io::path_t &p, const io::linux_permissions &perms)
{
  return posix::chmod(p.c_str(), perms.full_mode());
}

inline max_t
chown(const io::path_t &p, posix::uid_t uid, posix::gid_t gid)
{
  return posix::chown(p.c_str(), uid, gid);
}

inline max_t
lchown(const io::path_t &p, posix::uid_t uid, posix::gid_t gid)
{
  return posix::lchown(p.c_str(), uid, gid);
}

inline max_t
touch(const io::path_t &p)
{
  if ( !posix::exists(p.c_str()) ) {
    i32 fd = static_cast<i32>(
        posix::openat(posix::at_fdcwd, p.c_str(), posix::o_wronly | posix::o_create | posix::o_cloexec, posix::mode_file));
    if ( fd < 0 ) return fd;
    posix::close(fd);
    return 0;
  }
  return posix::utimensat(posix::at_fdcwd, p.c_str(), nullptr, 0);
}

// in-kernel copy_file_range -> sendfile -> read/write
inline max_t
copy(const io::path_t &from, const io::path_t &to)
{
  posix::fd_t src{ static_cast<i32>(posix::open(from.c_str(), posix::o_rdonly | posix::o_cloexec)) };
  if ( !src ) return src.fd;
  stat_t st{};
  if ( posix::fstat(src, st) < 0 ) {
    posix::close(src.fd);
    return -error::io_error;
  }
  // refuse a same-file copy
  {
    stat_t dst_st{};
    if ( posix::exists(to.c_str(), dst_st) && dst_st.st_dev == st.st_dev && dst_st.st_ino == st.st_ino ) {
      posix::close(src.fd);
      return -error::invalid_arg;
    }
  }
  posix::fd_t dst{ static_cast<i32>(
      posix::open(to.c_str(), posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec, st.st_mode & 0777u)) };
  if ( !dst ) {
    posix::close(src.fd);
    return dst.fd;
  }

  max_t total = 0;
  bool cfr_ok = true, sf_ok = true;
  for ( ;; ) {
    max_t n = -1;
    if ( cfr_ok ) {
      n = posix::copy_file_range(src.fd, nullptr, dst.fd, nullptr, 1u << 20, 0u);
      if ( n < 0 && (-n == error::cross_device_link || -n == error::invalid_arg || -n == error::bad_syscall) ) {
        cfr_ok = false;
        continue;
      }
    } else if ( sf_ok ) {
      n = posix::sendfile(dst, src, nullptr, 1u << 20);
      if ( n < 0 && (-n == error::invalid_arg || -n == error::bad_syscall) ) {
        sf_ok = false;
        continue;
      }
    } else {
      byte buf[65536];
      n = posix::read(src.fd, buf, sizeof(buf));
      if ( n > 0 ) n = posix::write_all(dst, buf, static_cast<usize>(n));
    }
    if ( n < 0 && -n == error::interrupted ) continue;
    if ( n < 0 ) {
      posix::close(src.fd);
      posix::close(dst.fd);
      return n;
    }
    if ( n == 0 ) break;
    total += n;
  }
  if ( max_t fe = posix::fsync(dst.fd); fe < 0 ) {
    posix::close(src.fd);
    posix::close(dst.fd);
    return fe;
  }
  posix::close(src.fd);
  posix::close(dst.fd);
  return total;
}

// on EXDEV falls back to copy + unlink
inline max_t
move(const io::path_t &from, const io::path_t &to)
{
  max_t r = rename(from, to);
  if ( r == 0 || -r != error::cross_device_link ) return r;
  if ( max_t c = copy(from, to); c < 0 ) return c;
  return unlink(from);
}

template<typename... P>
max_t
copy_list(const io::path_t &from, P &&...to)
{
  max_t total = 0;
  max_t rs[] = { copy(from, static_cast<P &&>(to))... };
  for ( max_t r : rs ) {
    if ( r < 0 ) return r;
    total += r;
  }
  return total;
}

inline micron::option<io::path_t, io::error_t>
resolve(const io::path_t &p)
{
  io::path_t out;
  if ( micron::realpath(p.c_str(), &out[0]) == nullptr ) [[unlikely]]
    return micron::option<io::path_t, io::error_t>{ io::error_t(error::no_entry) };
  out.adjust_size();
  return micron::option<io::path_t, io::error_t>{ micron::move(out) };
}

};      // namespace io

};      // namespace micron
