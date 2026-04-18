//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory_block.hpp"
#include "../string/strings.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "io.hpp"
#include "paths.hpp"
#include "posix/iosys.hpp"
#include "stream.hpp"

namespace micron
{
namespace io
{

enum class pipe_end_t : i32 { reader = 0, writer = 1 };

struct pipe_result_t {
  bool ok;
  usize bytes;
  bool eof;

  explicit
  operator bool() const noexcept
  {
    return ok;
  }
};

class upipe
{

  static constexpr int __r = static_cast<int>(pipe_end_t::reader);
  static constexpr int __w = static_cast<int>(pipe_end_t::writer);

  bool __r_open;
  bool __w_open;

  static max_t
  _write_all(int fd, const byte *src, usize len)
  {
    usize done = 0;
    while ( done < len ) {
      max_t n = posix::write(fd, src + done, len - done);
      if ( n == 0 ) break;
      if ( n < 0 ) return n;
      done += static_cast<usize>(n);
    }
    return static_cast<max_t>(done);
  }

  static max_t
  __read_all(int fd, byte *dst, usize len)
  {
    usize done = 0;
    while ( done < len ) {
      max_t n = posix::read(fd, dst + done, len - done);
      if ( n == 0 ) break;
      if ( n < 0 ) return n;
      done += static_cast<usize>(n);
    }
    return static_cast<max_t>(done);
  }

  static i32
  _set_nonblock(int fd, bool on)
  {
    i32 flags = static_cast<i32>(micron::syscall(SYS_fcntl, fd, posix::f_getfl, 0));
    if ( flags < 0 ) return flags;
    if ( on )
      flags |= posix::o_nonblock;
    else
      flags &= ~posix::o_nonblock;
    return static_cast<i32>(micron::syscall(SYS_fcntl, fd, posix::f_setfl, flags));
  }

  static i32
  _set_cloexec(int fd, bool on)
  {
    i32 flags = static_cast<i32>(micron::syscall(SYS_fcntl, fd, posix::f_getfd, 0));
    if ( flags < 0 ) return flags;
    if ( on )
      flags |= posix::fd_cloexec;
    else
      flags &= ~posix::fd_cloexec;
    return static_cast<i32>(micron::syscall(SYS_fcntl, fd, posix::f_setfd, flags));
  }

public:
  int fd[2];

  enum class utype { upipe_reader = 0, upipe_writer = 1 };

  utype tp;

  ~upipe()
  {
    if ( __r_open ) {
      posix::close(fd[__r]);
      __r_open = false;
    }
    if ( __w_open ) {
      posix::close(fd[__w]);
      __w_open = false;
    }
  }

  upipe() : __r_open(true), __w_open(true), tp(utype::upipe_writer)
  {
    if ( posix::pipe(fd) == -1 ) exc<except::io_error>("micron::upipe failed to open pipe");
  }

  explicit upipe(utype t) : __r_open(true), __w_open(true), tp(t)
  {
    if ( posix::pipe(fd) == -1 ) exc<except::io_error>("micron::upipe failed to open pipe");
  }

  explicit upipe(bool cloexec) : __r_open(true), __w_open(true), tp(utype::upipe_writer)
  {
    i32 flags = cloexec ? posix::o_cloexec : 0;
    if ( static_cast<i32>(micron::syscall(SYS_pipe2, fd, flags)) == -1 ) exc<except::io_error>("micron::upipe(pipe2) failed to open pipe");
  }

  upipe(const upipe &o) : __r_open(o.__r_open), __w_open(o.__w_open), tp(o.tp)
  {
    fd[__r] = o.fd[__r];
    fd[__w] = o.fd[__w];
  }

  upipe(upipe &&o) noexcept : __r_open(o.__r_open), __w_open(o.__w_open), tp(o.tp)
  {
    fd[__r] = o.fd[__r];
    fd[__w] = o.fd[__w];
    o.__r_open = false;
    o.__w_open = false;
    o.fd[__r] = -1;
    o.fd[__w] = -1;
  }

  upipe &
  operator=(const upipe &o)
  {
    fd[__r] = o.fd[__r];
    fd[__w] = o.fd[__w];
    __r_open = o.__r_open;
    __w_open = o.__w_open;
    tp = o.tp;
    return *this;
  }

  upipe &
  operator=(upipe &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( __r_open ) posix::close(fd[__r]);
    if ( __w_open ) posix::close(fd[__w]);
    fd[__r] = o.fd[__r];
    fd[__w] = o.fd[__w];
    __r_open = o.__r_open;
    __w_open = o.__w_open;
    tp = o.tp;
    o.__r_open = false;
    o.__w_open = false;
    o.fd[__r] = -1;
    o.fd[__w] = -1;
    return *this;
  }

  void
  make_reader() noexcept
  {
    tp = utype::upipe_reader;
  }

  void
  make_writer() noexcept
  {
    tp = utype::upipe_writer;
  }

  void
  close_read()
  {
    if ( __r_open ) {
      posix::close(fd[__r]);
      fd[__r] = -1;
      __r_open = false;
    }
  }

  void
  close_write()
  {
    if ( __w_open ) {
      posix::close(fd[__w]);
      fd[__w] = -1;
      __w_open = false;
    }
  }

  bool
  read_open() const noexcept
  {
    return __r_open;
  }

  bool
  write_open() const noexcept
  {
    return __w_open;
  }

  bool
  both_open() const noexcept
  {
    return __r_open && __w_open;
  }

  int
  read_fd() const noexcept
  {
    return fd[__r];
  }

  int
  write_fd() const noexcept
  {
    return fd[__w];
  }

  const int *
  get() const noexcept
  {
    return fd;
  }

  i32
  set_read_nonblocking(bool on = true)
  {
    return __r_open ? _set_nonblock(fd[__r], on) : -1;
  }

  i32
  set_write_nonblocking(bool on = true)
  {
    return __w_open ? _set_nonblock(fd[__w], on) : -1;
  }

  i32
  set_nonblocking(bool on = true)
  {
    i32 r = set_read_nonblocking(on);
    i32 w = set_write_nonblocking(on);
    return (r < 0) ? r : w;
  }

  i32
  set_read_cloexec(bool on = true)
  {
    return __r_open ? _set_cloexec(fd[__r], on) : -1;
  }

  i32
  set_write_cloexec(bool on = true)
  {
    return __w_open ? _set_cloexec(fd[__w], on) : -1;
  }

  i32
  set_cloexec(bool on = true)
  {
    i32 r = set_read_cloexec(on);
    i32 w = set_write_cloexec(on);
    return (r < 0) ? r : w;
  }

  i32
  capacity() const
  {
    int target = __w_open ? fd[__w] : (__r_open ? fd[__r] : -1);
    if ( target < 0 ) return -1;
    return static_cast<i32>(micron::syscall(SYS_fcntl, target, posix::f_getpipe_sz));
  }

  i32
  set_capacity(i32 new_size)
  {
    int target = __w_open ? fd[__w] : (__r_open ? fd[__r] : -1);
    if ( target < 0 ) return -1;
    return static_cast<i32>(micron::syscall(SYS_fcntl, target, posix::f_setpipe_sz, new_size));
  }

  i32
  dup_write_to(int newfd)
  {
    if ( !__w_open ) return -1;
    return static_cast<i32>(micron::syscall(SYS_dup2, fd[__w], newfd));
  }

  i32
  dup_read_to(int newfd)
  {
    if ( !__r_open ) return -1;
    return static_cast<i32>(micron::syscall(SYS_dup2, fd[__r], newfd));
  }

  pipe_result_t
  write_bytes(const byte *src, usize len)
  {
    if ( !__w_open ) return { false, 0, false };
    max_t n = _write_all(fd[__w], src, len);
    if ( n < 0 ) return { false, 0, false };
    return { true, static_cast<usize>(n), false };
  }

  pipe_result_t
  read_bytes(byte *dst, usize len)
  {
    if ( !__r_open ) return { false, 0, false };
    max_t n = __read_all(fd[__r], dst, len);
    if ( n < 0 ) return { false, 0, false };
    return { true, static_cast<usize>(n), n == 0 };
  }

  bool
  peek(byte &out)
  {
    if ( !__r_open ) return false;
    max_t n = posix::read(fd[__r], &out, 1);
    return n == 1;
  }

  template <is_string T>
  pipe_result_t
  write(const T &t)
  {
    return write_bytes(reinterpret_cast<const byte *>(t.c_str()), t.size() * sizeof(typename T::value_type));
  }

  template <is_string T>
  pipe_result_t
  read(T &t)
  {
    if ( t.size() == 0 ) return { true, 0, false };
    return read_bytes(reinterpret_cast<byte *>(&t[0]), t.size() * sizeof(typename T::value_type));
  }

  template <is_string T>
  usize
  read_all(T &t, usize chunk_sz = 4096u)
  {
    usize total = 0;
    micron::buffer win(chunk_sz);
    for ( ;; ) {
      max_t n = posix::read(fd[__r], win.begin(), chunk_sz);
      if ( n <= 0 ) break;
      t.append(reinterpret_cast<const typename T::value_type *>(win.begin()), static_cast<usize>(n));
      total += static_cast<usize>(n);
    }
    return total;
  }

  template <typename T>
    requires(micron::is_object_v<T> && micron::is_convertible_v<T, typename T::value_type>)
  void
  operator()(T &t)
  {
    if ( tp == utype::upipe_reader ) {
      while ( posix::read(fd[__r], &t[0], 1024 > t.size() ? t.size() : 1024) > 0 ) {
      }
    } else if ( tp == utype::upipe_writer ) {
      while ( posix::write(fd[__w], &t[0], t.size()) > 0 ) {
      }
    }
  }

  void
  operator()(byte *t, usize sz)
  {
    if ( tp == utype::upipe_reader ) {
      while ( posix::read(fd[__r], t, sz) > 0 ) {
      }
    } else if ( tp == utype::upipe_writer ) {
      while ( posix::write(fd[__w], t, sz) > 0 ) {
      }
    }
  }

  template <is_string T>
  void
  operator()(T &t)
  {
    if ( tp == utype::upipe_reader ) {
      while ( posix::read(fd[__r], &t[0], 1024 > t.size() ? t.size() : 1024) > 0 ) {
      }
    } else if ( tp == utype::upipe_writer ) {
      while ( posix::write(fd[__w], &t[0], t.size()) > 0 ) {
      }
    }
  }

  template <int SZ, int CK>
  upipe &
  operator<<(io::stream<SZ, CK> &s)
  {
    if ( !__w_open || s.empty() ) return *this;
    _write_all(fd[__w], s.data(), static_cast<usize>(s.size()));
    s.rewind();
    return *this;
  }

  template <int SZ, int CK>
  upipe &
  operator>>(io::stream<SZ, CK> &s)
  {
    if ( !__r_open ) return *this;
    usize cap = s.max_size() - static_cast<usize>(s.size());
    if ( cap == 0 ) return *this;
    micron::buffer win(cap);
    max_t n = posix::read(fd[__r], win.begin(), cap);
    if ( n > 0 ) s.append(win.begin(), static_cast<usize>(n));
    return *this;
  }

  template <int SZ, int CK>
  usize
  drain_to_stream(io::stream<SZ, CK> &s)
  {
    if ( !__r_open ) return 0;
    usize total = 0;
    micron::buffer win(static_cast<usize>(SZ));
    for ( ;; ) {
      usize avail = s.max_size() - static_cast<usize>(s.size());
      if ( avail == 0 ) break;
      usize chunk = static_cast<usize>(SZ) < avail ? static_cast<usize>(SZ) : avail;
      max_t n = posix::read(fd[__r], win.begin(), chunk);
      if ( n <= 0 ) break;
      s.append(win.begin(), static_cast<usize>(n));
      total += static_cast<usize>(n);
    }
    return total;
  }

  max_t
  splice_to(int out_fd, usize len, bool more = false)
  {
    if ( !__r_open ) return -1;
    u32 flags = more ? posix::splice_f_more | posix::splice_f_move : posix::splice_f_move;
    return micron::syscall(SYS_splice, fd[__r], nullptr, out_fd, nullptr, len, flags);
  }

  max_t
  splice_from(int in_fd, usize len, bool more = false)
  {
    if ( !__w_open ) return -1;
    u32 flags = more ? posix::splice_f_more | posix::splice_f_move : posix::splice_f_move;
    return micron::syscall(SYS_splice, in_fd, nullptr, fd[__w], nullptr, len, flags);
  }

  max_t
  splice_to_pipe(upipe &dst, usize len)
  {
    if ( !__r_open || !dst.__w_open ) return -1;
    return splice_to(dst.fd[__w], len);
  }

  max_t
  tee_to(upipe &dst, usize len, bool nonblock = false)
  {
    if ( !__r_open || !dst.__w_open ) return -1;
    u32 flags = nonblock ? posix::splice_f_nonblock : 0u;
    return micron::syscall(SYS_tee, fd[__r], dst.fd[__w], len, flags);
  }

  max_t
  sendfile_from(int in_fd, usize count, posix::off_t *offset = nullptr)
  {
    if ( !__w_open ) return -1;
    return micron::syscall(SYS_sendfile, fd[__w], in_fd, offset, count);
  }

  micron::buffer
  read_buffer(usize hint = 65536u)
  {
    micron::buffer out(hint);
    max_t n = posix::read(fd[__r], out.begin(), hint);
    if ( n < 0 ) n = 0;
    return out;
  }

  pipe_result_t
  write_buffer(const micron::buffer &buf)
  {
    return write_bytes(buf.begin(), buf.size());
  }
};

class npipe
{
  micron::string pipe_name;
  int _fd;
  bool _open;
  bool _owns_file;

  static constexpr usize _chunk = 4096u;

  static max_t
  _write_all(int fd, const byte *src, usize len)
  {
    usize done = 0;
    while ( done < len ) {
      max_t n = posix::write(fd, src + done, len - done);
      if ( n == 0 ) break;
      if ( n < 0 ) return n;
      done += static_cast<usize>(n);
    }
    return static_cast<max_t>(done);
  }

  static max_t
  __read_all_exact(int fd, byte *dst, usize len)
  {
    usize done = 0;
    while ( done < len ) {
      max_t n = posix::read(fd, dst + done, len - done);
      if ( n == 0 ) break;
      if ( n < 0 ) return n;
      done += static_cast<usize>(n);
    }
    return static_cast<max_t>(done);
  }

public:
  ~npipe()
  {
    if ( _open ) {
      posix::close(_fd);
      _open = false;
    }
    if ( _owns_file ) micron::syscall(SYS_unlink, pipe_name.c_str());
  }

  npipe(const micron::string &str, int perms = 0666) : pipe_name(str), _fd(-1), _open(false), _owns_file(true)
  {
    if ( micron::mkfifo(pipe_name.c_str(), static_cast<posix::mode_t>(perms)) == -1 )
      exc<except::io_error>("micron::npipe(mkfifo) failed to create pipe");
    _fd = static_cast<int>(posix::open(pipe_name.c_str(), posix::o_rdwr));
    if ( _fd == -1 ) exc<except::io_error>("micron::npipe(open) failed to open pipe file");
    _open = true;
  }

  npipe(const micron::string &str, bool /*open_existing*/, int flags = posix::o_rdwr)
      : pipe_name(str), _fd(-1), _open(false), _owns_file(false)
  {
    _fd = static_cast<int>(posix::open(pipe_name.c_str(), flags));
    if ( _fd == -1 ) exc<except::io_error>("micron::npipe(open existing) failed to open pipe file");
    _open = true;
  }

  npipe(const npipe &o) = default;

  npipe(npipe &&o) noexcept : pipe_name(micron::move(o.pipe_name)), _fd(o._fd), _open(o._open), _owns_file(o._owns_file)
  {
    o._fd = -1;
    o._open = false;
    o._owns_file = false;
  }

  npipe &operator=(const npipe &) = default;

  npipe &
  operator=(npipe &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( _open ) posix::close(_fd);
    if ( _owns_file ) micron::syscall(SYS_unlink, pipe_name.c_str());
    pipe_name = micron::move(o.pipe_name);
    _fd = o._fd;
    _open = o._open;
    _owns_file = o._owns_file;
    o._fd = -1;
    o._open = false;
    o._owns_file = false;
    return *this;
  }

  bool
  valid() const noexcept
  {
    return _open && _fd >= 0;
  }

  int
  get() const noexcept
  {
    return _fd;
  }

  const micron::string &
  path() const noexcept
  {
    return pipe_name;
  }

  void
  close()
  {
    if ( _open ) {
      posix::close(_fd);
      _fd = -1;
      _open = false;
    }
  }

  void
  unlink()
  {
    if ( !pipe_name.empty() ) {
      micron::syscall(SYS_unlink, pipe_name.c_str());
      _owns_file = false;
    }
  }

  void
  reopen(int flags = posix::o_rdwr)
  {
    if ( _open ) close();
    _fd = static_cast<int>(posix::open(pipe_name.c_str(), flags));
    if ( _fd == -1 ) exc<except::io_error>("micron::npipe::reopen failed");
    _open = true;
  }

  i32
  set_nonblocking(bool on = true)
  {
    if ( !valid() ) return -1;
    i32 flags = static_cast<i32>(micron::syscall(SYS_fcntl, _fd, posix::f_getfl, 0));
    if ( flags < 0 ) return flags;
    if ( on )
      flags |= posix::o_nonblock;
    else
      flags &= ~posix::o_nonblock;
    return static_cast<i32>(micron::syscall(SYS_fcntl, _fd, posix::f_setfl, flags));
  }

  i32
  set_cloexec(bool on = true)
  {
    if ( !valid() ) return -1;
    i32 flags = static_cast<i32>(micron::syscall(SYS_fcntl, _fd, posix::f_getfd, 0));
    if ( flags < 0 ) return flags;
    if ( on )
      flags |= posix::fd_cloexec;
    else
      flags &= ~posix::fd_cloexec;
    return static_cast<i32>(micron::syscall(SYS_fcntl, _fd, posix::f_setfd, flags));
  }

  bool
  is_fifo() const
  {
    return valid() && posix::is_fifo(_fd);
  }

  posix::off_t
  size() const
  {
    if ( !valid() ) return -1;
    return posix::get_size(_fd);
  }

  posix::mode_t
  permissions() const
  {
    if ( !valid() ) return 0;
    return posix::get_permissions(_fd);
  }

  i32
  chmod(posix::mode_t mode)
  {
    if ( pipe_name.empty() ) return -1;
    return static_cast<i32>(micron::syscall(SYS_chmod, pipe_name.c_str(), mode));
  }

  bool
  readable() const
  {
    return valid() && posix::is_readable(pipe_name.c_str());
  }

  bool
  writable() const
  {
    return valid() && posix::is_writable(pipe_name.c_str());
  }

  bool
  accessible() const
  {
    return valid() && posix::exists(pipe_name.c_str());
  }

  pipe_result_t
  write(const byte *src, usize len)
  {
    if ( !valid() ) return { false, 0, false };
    max_t n = _write_all(_fd, src, len);
    if ( n < 0 ) return { false, 0, false };
    return { true, static_cast<usize>(n), false };
  }

  pipe_result_t
  read(byte *dst, usize len)
  {
    if ( !valid() ) return { false, 0, false };
    max_t n = __read_all_exact(_fd, dst, len);
    if ( n < 0 ) return { false, 0, false };
    return { true, static_cast<usize>(n), n == 0 };
  }

  template <typename T>
    requires(micron::is_object_v<T> && micron::is_convertible_v<T, typename T::value_type>)
  void
  write(T &t)
  {
    if ( !valid() ) return;
    _write_all(_fd, reinterpret_cast<const byte *>(&t[0]), t.size() * sizeof(typename T::value_type));
  }

  void
  write(const byte *t, usize sz)
  {
    if ( !valid() ) return;
    _write_all(_fd, t, sz);
  }

  template <is_string T>
  void
  write(const T &t)
  {
    if ( !valid() ) return;
    _write_all(_fd, reinterpret_cast<const byte *>(t.c_str()), t.size() * sizeof(typename T::value_type));
  }

  template <typename T>
    requires(micron::is_object_v<T> && micron::is_convertible_v<T, typename T::value_type>)
  void
  read(T &t)
  {
    if ( !valid() || t.size() == 0 ) return;
    __read_all_exact(_fd, reinterpret_cast<byte *>(&t[0]), t.size() * sizeof(typename T::value_type));
  }

  void
  read(byte *t, usize sz)
  {
    if ( !valid() ) return;
    __read_all_exact(_fd, t, sz);
  }

  template <is_string T>
  void
  read(T &t)
  {
    if ( !valid() || t.size() == 0 ) return;
    __read_all_exact(_fd, reinterpret_cast<byte *>(&t[0]), t.size() * sizeof(typename T::value_type));
  }

  template <is_string T>
  usize
  read_all(T &t, usize chunk_sz = _chunk)
  {
    if ( !valid() ) return 0;
    usize total = 0;
    micron::buffer win(chunk_sz);
    for ( ;; ) {
      max_t n = posix::read(_fd, win.begin(), chunk_sz);
      if ( n <= 0 ) break;
      t.append(reinterpret_cast<const typename T::value_type *>(win.begin()), static_cast<usize>(n));
      total += static_cast<usize>(n);
    }
    return total;
  }

  pipe_result_t
  write(const micron::buffer &buf)
  {
    return write(buf.begin(), buf.size());
  }

  micron::buffer
  read_buffer(usize hint = 65536u)
  {
    micron::buffer out(hint);
    max_t n = posix::read(_fd, out.begin(), hint);
    if ( n < 0 ) n = 0;
    return out;
  }

  template <is_string T>
  npipe &
  operator<<(const T &t)
  {
    write(t);
    return *this;
  }

  template <is_string T>
  npipe &
  operator>>(T &t)
  {
    read_all(t);
    return *this;
  }

  template <int SZ, int CK>
  npipe &
  operator<<(io::stream<SZ, CK> &s)
  {
    if ( !valid() || s.empty() ) return *this;
    _write_all(_fd, s.data(), static_cast<usize>(s.size()));
    s.rewind();
    return *this;
  }

  template <int SZ, int CK>
  npipe &
  operator>>(io::stream<SZ, CK> &s)
  {
    if ( !valid() ) return *this;
    usize avail = s.max_size() - static_cast<usize>(s.size());
    if ( avail == 0 ) return *this;
    micron::buffer win(avail);
    max_t n = posix::read(_fd, win.begin(), avail);
    if ( n > 0 ) s.append(win.begin(), static_cast<usize>(n));
    return *this;
  }

  max_t
  splice_to(int out_fd, usize len)
  {
    if ( !valid() ) return -1;
    return micron::syscall(SYS_splice, _fd, nullptr, out_fd, nullptr, len, posix::splice_f_move);
  }

  max_t
  splice_from(int in_fd, usize len)
  {
    if ( !valid() ) return -1;
    return micron::syscall(SYS_splice, in_fd, nullptr, _fd, nullptr, len, posix::splice_f_move);
  }

  max_t
  splice_to_pipe(npipe &dst, usize len)
  {
    return splice_to(dst._fd, len);
  }
};

struct pipe_pair_t {
  upipe p2c;
  upipe c2p;

  pipe_pair_t() : p2c(upipe::utype::upipe_writer), c2p(upipe::utype::upipe_reader) {}

  void
  parent_side()
  {
    p2c.close_read();
    c2p.close_write();
  }

  void
  child_side(bool redirect_stdio = false)
  {
    p2c.close_write();
    c2p.close_read();
    if ( redirect_stdio ) {
      p2c.dup_read_to(STDIN_FILENO);
      c2p.dup_write_to(STDOUT_FILENO);
      p2c.close_read();
      c2p.close_write();
    }
  }
};

inline pipe_pair_t
make_pipe_pair()
{
  return pipe_pair_t{};
}

inline npipe
open_npipe_write(const micron::string &name)
{
  return npipe(name, true, posix::o_wronly | posix::o_nonblock);
}

inline npipe
open_npipe_read(const micron::string &name)
{
  return npipe(name, true, posix::o_rdonly);
}

};     // namespace io
};     // namespace micron
