//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"

#include "../memory_block.hpp"

#include "__lines.hpp"
#include "__serial_core.hpp"
#include "fn.hpp"
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

template<typename T>
concept __bulk_value = (is_string<T> || is_contiguous_container<T>) && micron::is_trivially_copyable_v<typename T::value_type>;

template<typename T>
concept __framed_value
    = is_node_container<T> || (is_iterable_container<T> && !is_string<T> && !micron::is_trivially_copyable_v<typename T::value_type>);

template<typename T>
concept __object_value = micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !is_node_container<T>
                         && !micron::is_pointer_v<T> && !micron::is_array_v<T> && !fn_like<T>;

template<typename T>
concept __readable_value = __bulk_value<T> || __framed_value<T> || __object_value<T>;

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

  template<typename T>
    requires __readable_value<T>
  max_t
  __read_value(T &out)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if constexpr ( __bulk_value<T> ) {
      stat<STAT_OVERRIDE>();
      const usize total = static_cast<usize>(sd.st_size);
      const posix::off64_t at = tell();
      const usize remaining = (at >= 0 && total > static_cast<usize>(at)) ? total - static_cast<usize>(at) : 0;
      if ( remaining == 0 ) {
        // virtual files (/proc, /sys) fstat to size 0; grow chunk-by-chunk until EOF
        max_t got = 0;
        if constexpr ( sizeof(typename T::value_type) == 1 && requires(T t, typename T::value_type v) { t.push_back(v); } ) {
          byte chunk[4096];
          for ( ;; ) {
            max_t r = posix::read(__handle.fd, chunk, sizeof(chunk));
            if ( r < 0 ) [[unlikely]] {
              if ( -r == error::interrupted ) continue;
              return r;
            }
            if ( r == 0 ) break;
            for ( max_t i = 0; i < r; ++i ) out.push_back(static_cast<typename T::value_type>(chunk[i]));
            got += r;
          }
        }
        return got;
      }
      const usize elems = remaining / sizeof(typename T::value_type);
      if constexpr ( requires(T t, usize n) { t.reserve(n); } ) out.reserve(elems + 1);
      // a fixed-capacity destination that cannot hold the remainder is an error, not a silent
      // prefix: modify() reads this count back as the file's old size, so reporting a short
      // read as a complete one makes it truncate away the tail it never saw.
      if constexpr ( requires(const T &t) { t.max_size(); } ) {
        if ( elems > out.max_size() ) [[unlikely]]
          return -error::file_too_big;
      }
      max_t got = __read_loop(out.data(), elems * sizeof(typename T::value_type));
      if ( got < 0 ) [[unlikely]]
        return got;
      if ( got > 0 ) __set_length(out, static_cast<usize>(got) / sizeof(typename T::value_type));
      return got;
    } else if constexpr ( __framed_value<T> ) {
      const posix::off64_t at = tell();
      if ( at < 0 ) [[unlikely]]
        return at;
      stat<STAT_OVERRIDE>();
      if ( static_cast<usize>(sd.st_size) <= static_cast<usize>(at) ) return 0;
      return read(out);      // non-throwing framed read
    } else {
      // trivially-copyable object: a short read is corruption
      max_t got = __read_loop(static_cast<void *>(micron::addressof(out)), sizeof(T));
      if ( got < 0 ) [[unlikely]]
        return got;
      if ( static_cast<usize>(got) != sizeof(T) ) [[unlikely]]
        return -error::io_error;
      return got;
    }
  }

  // rebind __handle onto fname after a rename swapped a new inode in behind it. F_GETFL
  // reports the access mode and status flags but never O_CREAT/O_TRUNC/O_EXCL (those are
  // consumed by open and not retained), so reusing them cannot re-truncate the new file.
  i32
  __rebind(void)
  {
    const i32 fl = get_status_flags();
    if ( fl < 0 ) [[unlikely]]
      return fl;
    const bool keep_cloexec = is_cloexec();      // an fd flag, not reported by F_GETFL
    const i32 nfd = static_cast<i32>(posix::openat(posix::at_fdcwd, fname.c_str(), fl | (keep_cloexec ? posix::o_cloexec : 0), 0u));
    if ( nfd < 0 ) [[unlikely]]
      return nfd;
    posix::close(__handle.fd);
    __handle.fd = nfd;
    micron::zero(&sd);      // new inode: every cached stat field is stale
    return 0;
  }

  // byte length the write tiers will emit for v (same constexpr dispatch as write())
  template<typename T>
  static max_t
  __marshal_size(const T &v)
  {
    if constexpr ( __bulk_value<T> )
      return static_cast<max_t>(v.size() * sizeof(typename T::value_type));
    else if constexpr ( __framed_value<T> )
      return serialize::framed_size(v);
    else
      return static_cast<max_t>(sizeof(T));
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

  // (d) trivially-copyable plain objects -> sizeof dump; pointers are refused
  template<typename T>
    requires(micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !is_node_container<T>
             && !micron::is_pointer_v<T> && !micron::is_array_v<T> && !fn_like<T>)
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
    const usize elems = remaining / sizeof(typename T::value_type);
    if constexpr ( requires(T t, usize n) { t.reserve(n); } ) out.reserve(elems + 1);
    if constexpr ( requires(const T &t) { t.max_size(); } ) {
      if ( elems > out.max_size() ) [[unlikely]] {
        exc<except::io_error>("micron::io::file::read: destination cannot hold the file");
        return out;      // exc<> may return; never read past capacity
      }
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

  // (d) trivially-copyable plain objects; callables are refused
  template<typename T>
    requires(micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !is_node_container<T>
             && !micron::is_pointer_v<T> && !micron::is_array_v<T> && !fn_like<T>)
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

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // functional io

  // producer write
  //   f.write([]{ return build_report(); });
  template<typename Fn>
    requires(fn_like<Fn> && micron::is_invocable_v<Fn> && !micron::is_void_v<micron::invoke_result_t<Fn>>)
  max_t
  write(Fn &&fn)
  {
    return write(micron::forward<Fn>(fn)());
  }

  // consumer read
  //   f.read([](micron::vector<int> v){ ... });
  template<typename Fn>
    requires(fn_deducible<Fn> && fn_arity_v<Fn> == 1 && micron::is_default_constructible_v<fn_arg0_t<Fn>> && __readable_value<fn_arg0_t<Fn>>
             && micron::is_invocable_v<Fn, fn_arg0_exact_t<Fn>>
             && micron::distinct<__unit_if_void_t<micron::invoke_result_t<Fn, fn_arg0_exact_t<Fn>>>, io::error_t>)
  auto
  read(Fn &&fn) -> micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, fn_arg0_exact_t<Fn>>>, io::error_t>
  {
    using A0 = fn_arg0_exact_t<Fn>;
    using Ret = micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, A0>>, io::error_t>;
    fn_arg0_t<Fn> val{};
    if ( max_t r = __read_value(val); r < 0 ) [[unlikely]]
      return Ret{ io::error_t(static_cast<i32>(r)) };
    if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, A0>> ) {
      micron::forward<Fn>(fn)(micron::forward<A0>(val));
      return Ret{ unit_t{} };
    } else {
      return Ret{ micron::forward<Fn>(fn)(micron::forward<A0>(val)) };
    }
  }

  // read-modify-write over the whole file
  //   pure:     T -> T        f.modify([](micron::string s){ return transform(s); });
  //   in-place: void(T&)      f.modify([](micron::string &s){ s += "\n"; });
  // in-place rewrite, not crash-atomic
  template<typename Fn>
    requires(fn_deducible<Fn> && fn_arity_v<Fn> == 1 && micron::is_default_constructible_v<fn_arg0_t<Fn>> && __readable_value<fn_arg0_t<Fn>>
             && ((micron::is_void_v<fn_ret_t<Fn>> && micron::same_as<fn_arg0_exact_t<Fn>, fn_arg0_t<Fn> &>)
                 || micron::same_as<micron::remove_cvref_t<fn_ret_t<Fn>>, fn_arg0_t<Fn>>))
  max_t
  modify(Fn &&fn)
  {
    using T = fn_arg0_t<Fn>;
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    // O_APPEND forces every write to EOF, so the rewrite below would land past the old content
    // and the trailing truncate would then cut the file down to the new length -- destroying
    // both versions. There is no way to rewrite in place through an append-only handle.
    if ( i32 fl = get_status_flags(); fl < 0 ) [[unlikely]]
      return fl;
    else if ( fl & posix::o_append ) [[unlikely]]
      return -error::invalid_arg;
    if ( posix::off64_t s = seek_to(0); s < 0 ) [[unlikely]]
      return s;
    T val{};
    const max_t old_bytes = __read_value(val);      // empty file: default T, fn still applied
    if ( old_bytes < 0 ) [[unlikely]]
      return old_bytes;
    if constexpr ( micron::is_void_v<fn_ret_t<Fn>> )
      micron::forward<Fn>(fn)(val);
    else
      val = micron::forward<Fn>(fn)(micron::move(val));
    const max_t need = __marshal_size(val);
    if ( need < 0 ) [[unlikely]]
      return need;
    if ( posix::off64_t s = seek_to(0); s < 0 ) [[unlikely]]
      return s;
    max_t n = write(val);
    if ( n < 0 ) [[unlikely]]
      return n;
    if ( n != need ) [[unlikely]]      // short write: old tail may survive past new content
      return -error::io_error;
    if ( n < old_bytes )
      if ( i32 t = truncate(static_cast<posix::off64_t>(n)); t != 0 ) [[unlikely]]
        return t;
    return n;
  }

  // crash atomic modify via atomic_replace (tmp + fsync + rename + dir-fsync)
  template<typename Fn>
    requires(fn_deducible<Fn> && fn_arity_v<Fn> == 1 && micron::is_default_constructible_v<fn_arg0_t<Fn>> && __readable_value<fn_arg0_t<Fn>>
             && ((micron::is_void_v<fn_ret_t<Fn>> && micron::same_as<fn_arg0_exact_t<Fn>, fn_arg0_t<Fn> &>)
                 || micron::same_as<micron::remove_cvref_t<fn_ret_t<Fn>>, fn_arg0_t<Fn>>))
  max_t
  modify_atomic(Fn &&fn)
  {
    using T = fn_arg0_t<Fn>;
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    if ( posix::off64_t s = seek_to(0); s < 0 ) [[unlikely]]
      return s;
    T val{};
    if ( max_t r = __read_value(val); r < 0 ) [[unlikely]]
      return r;
    if constexpr ( micron::is_void_v<fn_ret_t<Fn>> )
      micron::forward<Fn>(fn)(val);
    else
      val = micron::forward<Fn>(fn)(micron::move(val));
    if constexpr ( __bulk_value<T> ) {
      if ( i32 r = atomic_replace(val); r != 0 ) [[unlikely]]
        return r;
      return static_cast<max_t>(val.size() * sizeof(typename T::value_type));
    } else if constexpr ( __framed_value<T> ) {
      max_t need = serialize::framed_size(val);
      if ( need < 0 ) [[unlikely]]
        return need;
      micron::buffer out(static_cast<usize>(need));
      max_t n = serialize::frame_into(out.data(), out.size(), val);
      if ( n < 0 ) [[unlikely]]
        return n;
      if ( i32 r = atomic_replace(static_cast<const void *>(out.data()), static_cast<usize>(n)); r != 0 ) [[unlikely]]
        return r;
      return n;
    } else {
      if ( i32 r = atomic_replace(static_cast<const void *>(micron::addressof(val)), sizeof(T)); r != 0 ) [[unlikely]]
        return r;
      return static_cast<max_t>(sizeof(T));
    }
  }

  // stream the file's remainder through fn in fixed windows
  template<chunk_fn Fn>
  max_t
  read_with(Fn &&fn)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    micron::buffer win(fp_window);
    max_t total = 0;
    for ( ;; ) {
      max_t r = __read_loop(win.data(), fp_window);
      if ( r < 0 ) [[unlikely]]
        return r;      // a mid-stream failure is a failure: `total` here would be a plausible
                       // short count the caller cannot tell apart from a clean smaller file
      if ( r == 0 ) break;
      fn(static_cast<const byte *>(win.data()), static_cast<usize>(r));
      total += r;
    }
    return total;
  }

  // producer streaming
  template<producer_fn Fn>
  max_t
  write_with(Fn &&fn)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    micron::buffer win(fp_window);
    max_t total = 0;
    for ( ;; ) {
      usize n = fn(win.data(), fp_window);
      if ( n == 0 ) break;
      if ( n > fp_window ) [[unlikely]]
        return -error::invalid_arg;
      max_t w = __write_loop(win.data(), n);
      if ( w < 0 ) [[unlikely]]
        return w;      // as in read_with: a byte count cannot express "and then it failed"
      if ( static_cast<usize>(w) != n ) [[unlikely]]
        return -error::io_error;
      total += w;
    }
    return total;
  }

  // apply fn to every line of the file's remainder
  template<typename Fn>
    requires(line_fn<Fn> || micron::is_invocable_v<Fn, const micron::string &>)
  max_t
  each_line(Fn &&fn)
  {
    if ( i32 __e = __check() ) [[unlikely]]
      return __e;
    __line_cursor cur(__handle.fd);
    max_t count = 0;
    while ( cur.next() ) {
      if constexpr ( line_fn<Fn> )
        fn(cur.line().c_str(), cur.line().size());
      else
        fn(cur.line());
      ++count;
    }
    if ( i32 e = cur.error() ) [[unlikely]]
      return e;
    return count;
  }

  // left fold over lines: fn(R, line) -> R with either line shape
  template<typename R, typename Fn>
    requires(micron::distinct<R, io::error_t> && (line_fold_raw<Fn, R> || line_fold_str<Fn, R>))
  auto
  fold_lines(R init, Fn &&fn) -> micron::option<R, io::error_t>
  {
    using Ret = micron::option<R, io::error_t>;
    if ( i32 __e = __check() ) [[unlikely]]
      return Ret{ io::error_t(__e) };
    __line_cursor cur(__handle.fd);
    while ( cur.next() ) {
      if constexpr ( line_fold_raw<Fn, R> )
        init = fn(micron::move(init), cur.line().c_str(), cur.line().size());
      else
        init = fn(micron::move(init), cur.line());
    }
    if ( i32 e = cur.error() ) [[unlikely]]
      return Ret{ io::error_t(e) };
    return Ret{ micron::move(init) };
  }

  // lazy single-pass line range over the remainder
  lines_range
  lines(usize chunk_sz = __lines::chunk)
  {
    return lines_range(__handle.fd, chunk_sz);
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

    // the contents are on disk, but this handle still refers to the replaced inode
    return __rebind();
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
