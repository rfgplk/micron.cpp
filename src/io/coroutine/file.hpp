//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../os/os_file.hpp"
#include "../paths.hpp"
#include "__acore.hpp"
#include "__alines.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io::coro::file

namespace micron
{
namespace io
{
namespace coro
{

class file
{
  fd_t __handle{ posix::invalid_fd };
  u64 __cursor = 0;
  micron::sstr<128> fname;
  i32 __nb = __impl::__nb_unknown;      // O_NONBLOCK probe
  bool __append = false;

  [[gnu::always_inline]] i32
  __check() const noexcept
  {
    return __impl::__check(__handle.fd);
  }

  [[gnu::always_inline]] u64
  __woff() const noexcept
  {
    return __append ? static_cast<u64>(-1) : __cursor;
  }

public:
  file() = default;

  file(fd_t adopt, const char *recorded_name, bool append = false) noexcept : __handle(adopt), __append(append)
  {
    if ( recorded_name != nullptr ) fname = recorded_name;
  }

  file(const io::path_t &p, modes m = modes::read, const open_opts &opts = {}) noexcept
      : __handle{ static_cast<i32>(posix::open(p.c_str(), compose_open_flags(m, opts), opts.perms)) },
        __append(m == modes::append || m == modes::appendread)
  {
    fname = p.c_str();
  }

  file(const file &) = delete;
  file &operator=(const file &) = delete;

  file(file &&o) noexcept : __handle(o.__handle), __cursor(o.__cursor), fname(micron::move(o.fname)), __append(o.__append)
  {
    o.__handle = fd_t{ posix::invalid_fd };
    o.__cursor = 0;
  }

  file &
  operator=(file &&o) noexcept
  {
    if ( this != &o ) {
      close();
      __handle = o.__handle;
      __cursor = o.__cursor;
      fname = micron::move(o.fname);
      __append = o.__append;
      o.__handle = fd_t{ posix::invalid_fd };
      o.__cursor = 0;
    }
    return *this;
  }

  ~file() { close(); }

  [[nodiscard]] bool
  valid() const noexcept
  {
    return __handle.fd >= 0;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  [[nodiscard]] fd_t
  fd() const noexcept
  {
    return __handle;
  }

  [[nodiscard]] i32
  raw_fd() const noexcept
  {
    return __handle.fd;
  }

  [[nodiscard]] const micron::sstr<128> &
  name() const noexcept
  {
    return fname;
  }

  [[nodiscard]] io::path_t
  as_path() const
  {
    return io::path_t(fname.c_str());
  }

  [[nodiscard]] u64
  tell() const noexcept
  {
    return __cursor;
  }

  u64
  seek(u64 off) noexcept
  {
    __cursor = off;
    return __cursor;
  }

  void
  rewind() noexcept
  {
    __cursor = 0;
  }

  i32
  stat(stat_t &out) const noexcept
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return static_cast<i32>(posix::fstat(__handle, out));
  }

  [[nodiscard]] max_t
  size() const noexcept
  {
    stat_t st{};
    if ( i32 e = stat(st); e != 0 ) [[unlikely]]
      return e;
    return static_cast<max_t>(st.st_size);
  }

  void
  close() noexcept
  {
    if ( __handle.fd >= 0 ) posix::close(__handle.fd);
    __handle = fd_t{ posix::invalid_fd };
    __cursor = 0;
    __nb = __impl::__nb_unknown;
  }

  [[nodiscard]] fd_t
  release() noexcept
  {
    fd_t out = __handle;
    __handle = fd_t{ posix::invalid_fd };
    __cursor = 0;
    __nb = __impl::__nb_unknown;
    return out;
  }

  // %%%%%%%%%%%%%%%%%
  // raw byte io

  [[nodiscard]] micron::task<max_t>
  read(void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t r = co_await __impl::__read_full(__handle.fd, p, n, __cursor);
    if ( r > 0 ) __cursor += static_cast<u64>(r);
    co_return r;
  }

  [[nodiscard]] micron::task<max_t>
  write(const void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(__handle.fd, p, n, __woff());
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  [[nodiscard]] micron::task<max_t>
  read_at(u64 off, void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t r = co_await __impl::__read_full(__handle.fd, p, n, off);
    co_return r;
  }

  [[nodiscard]] micron::task<max_t>
  write_at(u64 off, const void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(__handle.fd, p, n, off);
    co_return w;
  }

  [[nodiscard]] micron::task<max_t>
  read_some(void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    if ( n > __impl::__chunk_cap ) n = __impl::__chunk_cap;
    max_t r = co_await __impl::__read_once(__handle.fd, p, n, __cursor, __nb);
    if ( r > 0 ) __cursor += static_cast<u64>(r);
    co_return r;
  }

  [[nodiscard]] micron::task<max_t>
  write_some(const void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    if ( n > __impl::__chunk_cap ) n = __impl::__chunk_cap;
    max_t w = co_await __impl::__write_once(__handle.fd, p, n, __woff(), __nb);
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  // %%%%%%%%%%%%%%%%%%%%%
  // universal write

  template<is_string T>
  [[nodiscard]] micron::task<max_t>
  write(const T &str)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(__handle.fd, str.c_str(), str.size() * sizeof(typename T::value_type), __woff());
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  template<typename T>
    requires(is_contiguous_container<T> && !is_string<T> && micron::is_trivially_copyable_v<typename T::value_type>)
  [[nodiscard]] micron::task<max_t>
  write(const T &c)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(__handle.fd, c.data(), c.size() * sizeof(typename T::value_type), __woff());
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  template<typename T>
    requires(is_node_container<T>
             || (is_iterable_container<T> && !is_string<T> && !micron::is_trivially_copyable_v<typename T::value_type>))
  [[nodiscard]] micron::task<max_t>
  write(const T &c)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_value(__handle.fd, __woff(), c);
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  template<typename T>
    requires(micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !is_node_container<T>
             && !micron::is_pointer_v<T> && !micron::is_array_v<T> && !fn_like<T>)
  [[nodiscard]] micron::task<max_t>
  write(const T &obj)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(__handle.fd, micron::addressof(obj), sizeof(T), __woff());
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  template<usize N>
  [[nodiscard]] micron::task<max_t>
  write(const char (&lit)[N])
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(__handle.fd, lit, N ? N - 1 : 0, __woff());
    if ( w > 0 && !__append ) __cursor += static_cast<u64>(w);
    co_return w;
  }

  // %%%%%%%%%%%%%%%%%%%
  // universal read

  template<typename T>
    requires(__readable_value<T> && micron::is_default_constructible_v<T>)
  [[nodiscard]] micron::task<micron::option<T, io::error_t>>
  read()
  {
    using Ret = micron::option<T, io::error_t>;
    if ( i32 e = __check() ) [[unlikely]]
      co_return Ret{ io::error_t(e) };
    T out{};
    max_t r = co_await __impl::__read_value_into(__handle.fd, __cursor, out);
    if ( r < 0 ) [[unlikely]]
      co_return Ret{ io::error_t(static_cast<i32>(r)) };
    __cursor += static_cast<u64>(r);
    co_return Ret{ micron::move(out) };
  }

  template<typename T>
    requires __readable_value<T>
  [[nodiscard]] micron::task<max_t>
  read(T &out)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    max_t r = co_await __impl::__read_value_into(__handle.fd, __cursor, out);
    if ( r > 0 ) __cursor += static_cast<u64>(r);
    co_return r;
  }

  // %%%%%%%%%%%%%%%%%
  // functional io

  template<typename Fn>
    requires(fn_like<Fn> && micron::is_invocable_v<Fn> && !micron::is_void_v<micron::invoke_result_t<Fn>>)
  [[nodiscard]] micron::task<max_t>
  write(Fn fn)
  {
    auto val = fn();
    max_t w = co_await write(val);
    co_return w;
  }

  template<typename Fn>
    requires(fn_deducible<Fn> && fn_arity_v<Fn> == 1 && micron::is_default_constructible_v<fn_arg0_t<Fn>> && __readable_value<fn_arg0_t<Fn>>
             && micron::is_invocable_v<Fn, fn_arg0_exact_t<Fn>>
             && micron::distinct<__unit_if_void_t<micron::invoke_result_t<Fn, fn_arg0_exact_t<Fn>>>, io::error_t>)
  [[nodiscard]] auto
  read(Fn fn) -> micron::task<micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, fn_arg0_exact_t<Fn>>>, io::error_t>>
  {
    using A0 = fn_arg0_exact_t<Fn>;
    using Ret = micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, A0>>, io::error_t>;
    if ( i32 e = __check() ) [[unlikely]]
      co_return Ret{ io::error_t(e) };
    fn_arg0_t<Fn> val{};
    max_t r = co_await __impl::__read_value_into(__handle.fd, __cursor, val);
    if ( r < 0 ) [[unlikely]]
      co_return Ret{ io::error_t(static_cast<i32>(r)) };
    __cursor += static_cast<u64>(r);
    if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, A0>> ) {
      fn(micron::forward<A0>(val));
      co_return Ret{ unit_t{} };
    } else {
      co_return Ret{ fn(micron::forward<A0>(val)) };
    }
  }

  template<typename Fn>
    requires(fn_deducible<Fn> && fn_arity_v<Fn> == 1 && micron::is_default_constructible_v<fn_arg0_t<Fn>> && __readable_value<fn_arg0_t<Fn>>
             && ((micron::is_void_v<fn_ret_t<Fn>> && micron::same_as<fn_arg0_exact_t<Fn>, fn_arg0_t<Fn> &>)
                 || micron::same_as<micron::remove_cvref_t<fn_ret_t<Fn>>, fn_arg0_t<Fn>>))
  [[nodiscard]] micron::task<max_t>
  modify(Fn fn)
  {
    using T = fn_arg0_t<Fn>;
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    if ( __append ) [[unlikely]]
      co_return -error::invalid_arg;
    T val{};
    const max_t old_bytes = co_await __impl::__read_value_into(__handle.fd, 0, val);
    if ( old_bytes < 0 ) [[unlikely]]
      co_return old_bytes;
    if constexpr ( micron::is_void_v<fn_ret_t<Fn>> )
      fn(val);
    else
      val = fn(micron::move(val));
    const max_t need = __impl::__marshal_size(val);
    if ( need < 0 ) [[unlikely]]
      co_return need;
    max_t n = co_await __impl::__write_value(__handle.fd, 0, val);
    if ( n < 0 ) [[unlikely]]
      co_return n;
    if ( n != need ) [[unlikely]]
      co_return -error::io_error;
    if ( n < old_bytes ) {
      i32 t = co_await micron::coro::io::ftruncate(__handle.fd, static_cast<u64>(n));
      if ( t != 0 ) [[unlikely]]
        co_return t;
    }
    __cursor = static_cast<u64>(n);
    co_return n;
  }

  template<chunk_fn Fn>
  [[nodiscard]] micron::task<max_t>
  read_with(Fn fn)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    micron::buffer win(fp_window);
    max_t total = 0;
    for ( ;; ) {
      max_t r = co_await __impl::__read_full(__handle.fd, win.data(), fp_window, __cursor);
      if ( r < 0 ) [[unlikely]]
        co_return r;
      if ( r == 0 ) break;
      fn(reinterpret_cast<const byte *>(win.data()), static_cast<usize>(r));
      __cursor += static_cast<u64>(r);
      total += r;
    }
    co_return total;
  }

  template<producer_fn Fn>
  [[nodiscard]] micron::task<max_t>
  write_with(Fn fn)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    micron::buffer win(fp_window);
    max_t total = 0;
    for ( ;; ) {
      usize n = fn(reinterpret_cast<byte *>(win.data()), fp_window);
      if ( n == 0 ) break;
      if ( n > fp_window ) [[unlikely]]
        co_return -error::invalid_arg;
      max_t w = co_await __impl::__write_full(__handle.fd, win.data(), n, __woff());
      if ( w < 0 ) [[unlikely]]
        co_return w;
      if ( static_cast<usize>(w) != n ) [[unlikely]]
        co_return -error::io_error;
      if ( !__append ) __cursor += static_cast<u64>(w);
      total += w;
    }
    co_return total;
  }

  template<typename Fn>
    requires(line_fn<Fn> || micron::is_invocable_v<Fn, const micron::string &>)
  [[nodiscard]] micron::task<max_t>
  each_line(Fn fn)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    __aline_cursor cur(__handle.fd, __cursor);
    micron::string ln;
    max_t count = 0;
    for ( ;; ) {
      bool more = co_await cur.next(ln);
      if ( !more ) break;
      if constexpr ( line_fn<Fn> )
        fn(ln.c_str(), ln.size());
      else
        fn(ln);
      ++count;
    }
    if ( i32 e = cur.error() ) [[unlikely]]
      co_return e;
    __cursor = cur.offset();
    co_return count;
  }

  template<typename R, typename Fn>
    requires(micron::distinct<R, io::error_t> && (line_fold_raw<Fn, R> || line_fold_str<Fn, R>))
  [[nodiscard]] micron::task<micron::option<R, io::error_t>>
  fold_lines(R init, Fn fn)
  {
    using Ret = micron::option<R, io::error_t>;
    if ( i32 e = __check() ) [[unlikely]]
      co_return Ret{ io::error_t(e) };
    __aline_cursor cur(__handle.fd, __cursor);
    micron::string ln;
    for ( ;; ) {
      bool more = co_await cur.next(ln);
      if ( !more ) break;
      if constexpr ( line_fold_raw<Fn, R> )
        init = fn(micron::move(init), ln.c_str(), ln.size());
      else
        init = fn(micron::move(init), ln);
    }
    if ( i32 e = cur.error() ) [[unlikely]]
      co_return Ret{ io::error_t(e) };
    __cursor = cur.offset();
    co_return Ret{ micron::move(init) };
  }

  [[nodiscard]] __aline_cursor
  lines(usize chunk_sz = __lines::chunk)
  {
    return __aline_cursor(__handle.fd, __cursor, chunk_sz);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // durability / space (all blocking)

  [[nodiscard]] micron::task<i32>
  sync()
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    i32 r = co_await micron::coro::io::fsync(__handle.fd);
    co_return r;
  }

  [[nodiscard]] micron::task<i32>
  datasync()
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    i32 r = co_await micron::coro::io::fsync(__handle.fd, micron::uring::fsync_datasync);
    co_return r;
  }

  [[nodiscard]] micron::task<i32>
  truncate(u64 len)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    i32 r = co_await micron::coro::io::ftruncate(__handle.fd, len);
    co_return r;
  }

  [[nodiscard]] micron::task<i32>
  allocate(u64 off, u64 len)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;
    i32 r = co_await micron::coro::io::fallocate(__handle.fd, 0, off, len);
    co_return r;
  }

  [[nodiscard]] micron::task<i32>
  advise(u64 off, u64 len, i32 advice)
  {
    if ( i32 e = __check() ) [[unlikely]]
      co_return e;

    u64 done = 0;
    do {
      u64 want = len - done;
      if ( want > __impl::__chunk_cap ) want = __impl::__chunk_cap;
      i32 r = co_await micron::coro::io::fadvise(__handle.fd, off + done, static_cast<u32>(want), advice);
      if ( r != 0 ) [[unlikely]]
        co_return r;
      done += want;
    } while ( done < len );
    co_return 0;
  }
};

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
