//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"

#include "../memory_block.hpp"

#include "__serial_core.hpp"
#include "format.hpp"
#include "os/os_file.hpp"
#include "paths.hpp"
#include "stream.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// porcelain file: the primary user-facing file type

namespace micron
{
namespace io
{

class file: public os_file
{
  max_t
  __write_loop(const void *src, usize len)
  {
    const byte *p = static_cast<const byte *>(src);
    usize done = 0;
    while ( done < len ) {
      max_t w = posix::write(__handle.fd, p + done, len - done);
      if ( w < 0 ) [[unlikely]] {
        if ( -w == error::interrupted ) continue;
        if ( -w == error::try_again && !is_nonblock() ) continue;
        return done ? static_cast<max_t>(done) : w;
      }
      if ( w == 0 ) break;
      done += static_cast<usize>(w);
    }
    if ( done ) micron::zero(&sd);      // cached stat (st_size) is stale after a successful write
    return static_cast<max_t>(done);
  }

  max_t
  __read_loop(void *dst, usize len) const
  {
    byte *p = static_cast<byte *>(dst);
    usize got = 0;
    while ( got < len ) {
      max_t r = posix::read(__handle.fd, p + got, len - got);
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        if ( -r == error::try_again && !is_nonblock() ) continue;
        return got ? static_cast<max_t>(got) : r;
      }
      if ( r == 0 ) break;
      got += static_cast<usize>(r);
    }
    return static_cast<max_t>(got);
  }

  template<typename T>
  static void
  __set_length(T &c, usize elems)
  {
    if constexpr ( requires(T t, usize n) { t.set_size(n); } )
      c.set_size(elems);
    else if constexpr ( requires(T t, usize n) { t._buf_set_length(n); } )
      c._buf_set_length(elems);
  }

public:
  using os_file::os_file;
  using os_file::operator=;      

  file() = default;
  ~file() = default;
  file(const file &) = default;
  file(file &&) noexcept = default;
  file &operator=(const file &) = default;
  file &operator=(file &&) noexcept = default;

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // universal write
  // (a) strings -> raw character bytes
  template<is_string T>
  max_t
  write(const T &str)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    return __write_loop(str.c_str(), str.size() * sizeof(typename T::value_type));
  }

  // (b) contiguous containers of trivially-copyable elements -> one raw blit of data()
  template<typename T>
    requires(is_contiguous_container<T> && !is_string<T> && micron::is_trivially_copyable_v<typename T::value_type>)
  max_t
  write(const T &c)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    return __write_loop(c.data(), c.size() * sizeof(typename T::value_type));
  }

  // (c) node-based containers (list/doublelist/maps/sets/trees) and iterable containers of
  // non-trivially-copyable elements (vector<string>, ...) -> MFR1 framed stream
  template<typename T>
    requires(is_node_container<T>
             || (is_iterable_container<T> && !is_string<T> && !micron::is_trivially_copyable_v<typename T::value_type>))
  max_t
  write(const T &c)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    max_t need = serialize::framed_size(c);
    if ( need < 0 ) [[unlikely]]
      return need;
    micron::buffer out(static_cast<usize>(need));
    max_t n = serialize::frame_into(out.data(), out.size(), c);
    if ( n < 0 ) [[unlikely]]
      return n;
    return __write_loop(out.data(), static_cast<usize>(n));
  }

  // (d) trivially-copyable plain objects -> sizeof dump; pointers are refused (use (ptr, len))
  template<typename T>
    requires(micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !is_node_container<T>
             && !micron::is_pointer_v<T> && !micron::is_array_v<T>)
  max_t
  write(const T &obj)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    return __write_loop(&obj, sizeof(T));
  }

  // character literals: write("text") excludes the trailing NUL
  template<usize N>
  max_t
  write(const char (&lit)[N])
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    return __write_loop(lit, N ? N - 1 : 0);
  }

  max_t
  write(const char *cstr)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( cstr == nullptr ) [[unlikely]]
      return -error::invalid_arg;
    return __write_loop(cstr, micron::strlen(cstr));
  }

  max_t
  write(const void *p, usize n)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( p == nullptr ) [[unlikely]]
      return -error::invalid_arg;
    return __write_loop(p, n);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // universal read
  template<typename T>
    requires((is_string<T> || is_contiguous_container<T>) && micron::is_trivially_copyable_v<typename T::value_type>)
  T
  read()
  {
    T out{};
    if ( __check() != 0 ) [[unlikely]]
      return out;
    stat<STAT_OVERRIDE>();
    const usize total = static_cast<usize>(sd.st_size);
    const posix::off64_t at = tell();
    const usize remaining = (at >= 0 && total > static_cast<usize>(at)) ? total - static_cast<usize>(at) : 0;
    if ( remaining == 0 ) {
      // virtual files (/proc, /sys) fstat to size 0; grow chunk-by-chunk until EOF
      if constexpr ( sizeof(typename T::value_type) == 1 && requires(T t, typename T::value_type v) { t.push_back(v); } ) {
        byte chunk[4096];
        for ( ;; ) {
          max_t r = posix::read(__handle.fd, chunk, sizeof(chunk));
          if ( r < 0 && -r == error::interrupted ) continue;
          if ( r <= 0 ) break;
          for ( max_t i = 0; i < r; ++i ) out.push_back(static_cast<typename T::value_type>(chunk[i]));
        }
      }
      return out;
    }
    usize elems = remaining / sizeof(typename T::value_type);
    if constexpr ( requires(T t, usize n) { t.reserve(n); } ) out.reserve(elems + 1);
    // never write past the destinations physical capacity
    if constexpr ( requires(const T &t) { t.max_size(); } ) {
      const usize cap = out.max_size();
      if ( elems > cap ) elems = cap;
    }
    max_t got = __read_loop(out.data(), elems * sizeof(typename T::value_type));
    if ( got < 0 ) [[unlikely]]      // EIO/EISDIR/EBADF etc
      exc<except::io_error>("micron::io::file::read: read failed");
    if ( got > 0 ) __set_length(out, static_cast<usize>(got) / sizeof(typename T::value_type));
    return out;
  }

  // in-place: fill the caller-sized container (out.size() elements)
  template<typename T>
    requires((is_string<T> || is_contiguous_container<T>) && micron::is_trivially_copyable_v<typename T::value_type>)
  max_t
  read(T &out)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    return __read_loop(out.data(), out.size() * sizeof(typename T::value_type));
  }

  // (c) node-based containers: reconstruct from the framed remainder of the file
  template<typename T>
    requires(is_node_container<T>
             || (is_iterable_container<T> && !is_string<T> && !micron::is_trivially_copyable_v<typename T::value_type>))
  max_t
  read(T &out)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    const posix::off64_t at = tell();
    if ( at < 0 ) [[unlikely]]
      return at;
    stat<STAT_OVERRIDE>();
    const usize total = static_cast<usize>(sd.st_size);
    if ( total <= static_cast<usize>(at) ) [[unlikely]]
      return -error::invalid_arg;
    const usize len = total - static_cast<usize>(at);
    micron::buffer in(len);
    max_t got = __read_loop(in.data(), len);
    if ( got < 0 ) [[unlikely]]
      return got;
    return serialize::unframe_from(in.data(), static_cast<usize>(got), out);
  }

  template<typename T>
    requires((is_node_container<T>
              || (is_iterable_container<T> && !is_string<T> && !micron::is_trivially_copyable_v<typename T::value_type>))
             && micron::is_default_constructible_v<T>)
  T
  read()
  {
    T out{};
    read(out);
    return out;
  }

  // (d) trivially-copyable plain objects
  template<typename T>
    requires(micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !is_node_container<T>
             && !micron::is_pointer_v<T> && !micron::is_array_v<T>)
  max_t
  read(T &obj)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    return __read_loop(&obj, sizeof(T));
  }

  max_t
  read(void *p, usize n)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( p == nullptr ) [[unlikely]]
      return -error::invalid_arg;
    return __read_loop(p, n);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // streaming sugar
  template<typename T>
    requires((is_string<T> || is_contiguous_container<T>) && micron::is_trivially_copyable_v<typename T::value_type>)
  file &
  operator>>(T &out)
  {
    seek_to(0);
    out = read<T>();
    return *this;
  }

  template<typename T>
  file &
  operator<<(const T &x)
  {
    write(x);
    return *this;
  }

  file &
  operator<<(const char *cstr)
  {
    write(cstr);
    return *this;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // ergonomics
  posix::off64_t
  seek(posix::off64_t offset)
  {
    return seek_to(offset);
  }

  posix::off64_t
  pos(void) const
  {
    return tell();
  }

  // atomically replace the files contents
  // NOTE: filesystem paths are trusted, so this does not defend against an attacker preplanting the temp path
  i32
  atomic_replace(const void *new_data, usize new_len)
  {
    if ( fname.empty() ) [[unlikely]]
      return -error::invalid_arg;
    if ( fname.size() + 16 >= max_name ) [[unlikely]]
      return -error::name_too_long;

    micron::sstr<max_name> tmp_name(fname);
    tmp_name += ".tmp.";
    // u32 on purpose: linux pids fit, and u64 divmod emits libgcc calls on 32-bit targets
    u32 pid = static_cast<u32>(posix::getpid());
    char pid_buf[24];
    usize pi = 0;
    if ( pid == 0 ) {
      pid_buf[pi++] = '0';
    } else {
      u32 p = pid;
      while ( p ) {
        pid_buf[pi++] = '0' + static_cast<char>(p % 10);
        p /= 10;
      }
    }
    for ( usize l = 0, r = pi - 1; l < r; ++l, --r ) {
      char c = pid_buf[l];
      pid_buf[l] = pid_buf[r];
      pid_buf[r] = c;
    }
    pid_buf[pi] = '\0';
    tmp_name += pid_buf;

    // preserve the original files permission bits
    u32 keep_mode = posix::mode_file;
    {
      stat_t orig{};
      if ( posix::exists(fname.c_str(), orig) ) keep_mode = static_cast<u32>(orig.st_mode) & 0777u;
    }

    // create restrictively (0600)
    posix::fd_t tmp{ (posix::openat(posix::at_fdcwd, tmp_name.c_str(),
                                    posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec | posix::o_nofollow, 0600u)) };
    if ( !tmp ) [[unlikely]]
      return tmp.fd < 0 ? tmp.fd : -error::io_error;

    usize written = 0;
    while ( written < new_len ) {
      max_t n = posix::write(tmp.fd, static_cast<const byte *>(new_data) + written, new_len - written);
      if ( n < 0 && -n == error::interrupted ) continue;
      if ( n <= 0 ) {
        posix::close(tmp.fd);
        posix::unlink(tmp_name.c_str());
        return n < 0 ? static_cast<i32>(n) : -error::io_error;
      }
      written += static_cast<usize>(n);
    }

    posix::fchmod(tmp.fd, keep_mode);

    if ( max_t fe = posix::fsync(tmp.fd); fe < 0 ) [[unlikely]] {
      posix::close(tmp.fd);
      posix::unlink(tmp_name.c_str());
      return static_cast<i32>(fe);
    }
    if ( max_t ce = posix::close(tmp.fd); ce < 0 ) [[unlikely]] {
      posix::unlink(tmp_name.c_str());
      return static_cast<i32>(ce);
    }

    if ( i32 r = static_cast<i32>(posix::rename(tmp_name.c_str(), fname.c_str())); r != 0 ) [[unlikely]] {
      posix::unlink(tmp_name.c_str());
      return r;
    }

    // fsync the parent directory
    {
      char dirbuf[max_name];
      const char *s = fname.c_str();
      const usize n = fname.size();
      usize cut = 0;
      bool have = false;
      for ( usize k = 0; k < n; ++k )
        if ( s[k] == '/' ) {
          cut = k;
          have = true;
        }
      usize dl;
      if ( !have ) {
        dirbuf[0] = '.';
        dl = 1;
      } else if ( cut == 0 ) {
        dirbuf[0] = '/';
        dl = 1;
      } else {
        for ( usize k = 0; k < cut; ++k ) dirbuf[k] = s[k];
        dl = cut;
      }
      dirbuf[dl] = '\0';
      posix::fd_t d{ (posix::openat(posix::at_fdcwd, dirbuf, posix::o_rdonly | posix::o_directory | posix::o_cloexec, 0u)) };
      if ( d ) {
        posix::fsync(d.fd);
        posix::close(d.fd);
      }
    }

    micron::zero(&sd);
    return 0;
  }

  template<is_string T>
  i32
  atomic_replace(const T &str)
  {
    return atomic_replace(static_cast<const void *>(str.c_str()), str.size() * sizeof(typename T::value_type));
  }

  template<typename T>
    requires(is_contiguous_container<T> && !is_string<T> && micron::is_trivially_copyable_v<typename T::value_type>)
  i32
  atomic_replace(const T &c)
  {
    return atomic_replace(static_cast<const void *>(c.data()), c.size() * sizeof(typename T::value_type));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // encoded/transformed writes
  template<encode_fn Fn>
  max_t
  write_encoded(Fn &&fn, const byte *src, usize src_len)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    micron::buffer out(src_len * 4 + 64);
    usize out_len = fn(out.begin(), out.size(), src, src_len);
    return __write_loop(out.begin(), out_len);
  }

  template<encode_fn Fn, is_string Tp>
  max_t
  write_encoded(Fn &&fn, const Tp &src)
  {
    return write_encoded(static_cast<Fn &&>(fn), reinterpret_cast<const byte *>(src.c_str()), src.size() * sizeof(typename Tp::value_type));
  }

  template<encode_fn Fn, is_string Tp>
  max_t
  append_encoded(Fn &&fn, const Tp &src)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( posix::off64_t e = seek_end(); e < 0 ) [[unlikely]]
      return e;
    return write_encoded(static_cast<Fn &&>(fn), reinterpret_cast<const byte *>(src.c_str()), src.size() * sizeof(typename Tp::value_type));
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // stream bridging
  template<int SZ, int CK>
  max_t
  to_stream(io::stream<SZ, CK> &s) const
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    s << __handle;
    return static_cast<max_t>(s.size());
  }

  template<int SZ, int CK>
  max_t
  from_stream(io::stream<SZ, CK> &s)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( s.empty() ) return 0;
    max_t n = __write_loop(s.data(), static_cast<usize>(s.size()));
    s.rewind();
    return n;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // lexical path accessors. NOTE: io::path is dir-backed
  io::path_t
  as_path() const
  {
    return io::path_t(fname.c_str());
  }

  io::path_t
  fullpath() const
  {
    return io::canonical(fname.c_str());
  }

  io::path_t
  basename() const
  {
    return io::basename(fname.c_str());
  }

  io::path_t
  extension() const
  {
    return io::extension(fname.c_str());
  }

  io::path_t
  stem() const
  {
    return io::stem(fname.c_str());
  }
};

};      // namespace io
};      // namespace micron
