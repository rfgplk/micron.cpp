//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"

#include "../memory/memory.hpp"
#include "../types.hpp"
#include "io.hpp"

#include "../tuple.hpp"

#include "__std.hpp"

#include "../string/conversions/bits.hpp"
#include "../string/conversions/floating_point.hpp"
#include "../string/format.hpp"

#include "../bits/__print.hpp"
#include "../settle_fwd.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io printing engine + echo, the primary universal print facility
// (supersedes console effectively)
//
//   io::echo(a, b, c)            marshal any micron type/container to stdout + '\n'
//   io::echon(...)               same, no trailing newline
//   io::echof("x = {}\n"...)     micron::format {}-string, trailing '\n' appended
//   io::echofn(fmt, ...)         format-string, NO trailing newline
//   io::echo(target, ...)        redirect: target = fd_t (io::stderr, a pipe end, ...),
//                                an os_file-derived handle, or an io::stream<>
//
// the first raw int argument is always a fd_t/file/stream redirect

namespace micron
{
namespace io
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sinks

template<typename S>
concept output_sink = requires(S &s, const char *p, usize n, char c) {
  { s.put(p, n) } -> micron::convertible_to<max_t>;
  { s.put(c) } -> micron::convertible_to<max_t>;
  { s.flush() } -> micron::convertible_to<max_t>;
};

struct stdout_sink {
  static max_t
  put(const char *p, usize n)
  {
    max_t w = io::fwrite(p, n, io::stdout);
    return w < 0 ? w : static_cast<max_t>(n);      // propagate a write error instead of reporting success
  }

  static max_t
  put(char c)
  {
    io::fput(c, io::stdout);
    return 1;
  }

  static max_t
  flush(void)
  {
    io::fflush(io::stdout);
    return 0;
  }

  static fd_t
  target(void)
  {
    return io::stdout;
  }
};

struct stderr_sink {
  static max_t
  put(const char *p, usize n)
  {
    max_t w = io::fwrite(p, n, io::stderr);
    return w < 0 ? w : static_cast<max_t>(n);      // propagate a write error instead of reporting success
  }

  static max_t
  put(char c)
  {
    io::fput(c, io::stderr);
    return 1;
  }

  static max_t
  flush(void)
  {
    io::fflush(io::stderr);
    return 0;
  }

  static fd_t
  target(void)
  {
    return io::stderr;
  }
};

// (b) arbitrary runtime fd
template<usize CAP = 1024> struct fd_sink {
  fd_t __t;
  usize __len = 0;
  i32 __err = 0;
  char __buf[CAP];

  explicit fd_sink(fd_t t) : __t(t) { }

  // looped write mirroring io::file::__write_loop
  max_t
  __write_all(const char *p, usize n)
  {
    usize done = 0;
    while ( done < n ) {
      max_t w = posix::write(__t.fd, p + done, n - done);
      if ( w < 0 ) [[unlikely]] {
        if ( -w == error::interrupted ) continue;
        return w;
      }
      if ( w == 0 ) break;
      done += static_cast<usize>(w);
    }
    return static_cast<max_t>(done);
  }

  max_t
  put(const char *p, usize n)
  {
    if ( __len + n > CAP ) {
      flush();
      if ( n >= CAP ) {
        max_t w = __write_all(p, n);
        if ( w < 0 && !__err ) [[unlikely]]
          __err = static_cast<i32>(w);
        return w;
      }
    }
    micron::memcpy(__buf + __len, p, n);
    __len += n;
    return static_cast<max_t>(n);
  }

  max_t
  put(char c)
  {
    if ( __len == CAP ) flush();
    __buf[__len++] = c;
    return 1;
  }

  max_t
  flush(void)
  {
    if ( !__len ) return 0;
    max_t w = __write_all(__buf, __len);
    __len = 0;
    if ( w < 0 && !__err ) [[unlikely]]
      __err = static_cast<i32>(w);
    return w;
  }

  fd_t
  target(void) const
  {
    return __t;
  }
};

// (d) io::stream<> target
template<int SZ, int CK> struct stream_sink {
  io::stream<SZ, CK> &__s;

  explicit stream_sink(io::stream<SZ, CK> &s) : __s(s) { }

  max_t
  put(const char *p, usize n)
  {
    __s.append(reinterpret_cast<const byte *>(p), n);
    return static_cast<max_t>(n);
  }

  max_t
  put(char c)
  {
    __s.append(reinterpret_cast<const byte *>(&c), 1);
    return 1;
  }

  max_t
  flush(void)
  {
    return 0;
  }
};

// fd-backed sinks expose target() (used for the wide/unicode fput special cases)
template<typename S>
concept fd_backed_sink = requires(const S s) {
  { s.target() } -> micron::convertible_to<fd_t>;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scratch-buffer converters
namespace __impl
{

inline usize
arith_to_buf(char *buf, usize buf_sz, i64 val)
{
  return micron::format::__impl::fmt_int_to_buf(buf, buf_sz, val, 10, false);
}

inline usize
arith_to_buf(char *buf, usize buf_sz, u64 val)
{
  return micron::format::__impl::fmt_uint_to_buf(buf, buf_sz, val, 10, false);
}

inline usize
arith_to_buf(char *buf, [[maybe_unused]] usize buf_sz, f32 val)
{
  return micron::__impl::__ryu::__f32::f2s_buffered(val, buf);
}

inline usize
arith_to_buf(char *buf, [[maybe_unused]] usize buf_sz, f64 val)
{
  return micron::__impl::__ryu::d2s_buffered(val, buf);
}

inline usize
arith_to_buf(char *buf, [[maybe_unused]] usize buf_sz, long double val)
{
  return micron::__impl::__ryu::d2s_buffered(static_cast<f64>(val), buf);
}

inline usize
arith_to_buf(char *buf, usize buf_sz, bool val)
{
  return micron::format::__impl::bool_to_buf(buf, buf_sz, val);
}

inline usize
ptr_to_buf(char *buf, usize buf_sz, const void *ptr)
{
  return micron::format::__impl::ptr_to_buf(buf, buf_sz, ptr);
}

};      // namespace __impl

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk(sink, x)

template<output_sink S, typename T>
  requires micron::is_arithmetic_v<T>
max_t printk(S &s, const T &x);

template<output_sink S, has_cstr T>
  requires micron::is_class_v<T>
max_t printk(S &s, const T &str);

template<output_sink S, typename T>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
max_t printk(S &s, const T &x);

template<output_sink S, typename T>
  requires micron::__print::printable<T>
max_t printk(S &s, const T &x);

template<output_sink S, typename A, typename B> max_t printk(S &s, const pair<A, B> &p);

// char array literal
template<output_sink S, typename T, usize M>
  requires is_char_elem<T>
inline max_t
printk(S &s, T (&c)[M])
{
  const char *p = reinterpret_cast<const char *>(c);
  usize n = 0;
  while ( n < M && p[n] ) ++n;
  return s.put(p, n);
}

// char pointer
template<output_sink S, typename T>
  requires is_char_ptr<T>
inline max_t
printk(S &s, const T &c)
{
  if ( c == nullptr ) return 0;
  return s.put(reinterpret_cast<const char *>(c), micron::strlen(reinterpret_cast<const char *>(c)));
}

// single char
template<output_sink S>
inline max_t
printk(S &s, char c)
{
  return s.put(c);
}

// raw sized print
template<output_sink S>
inline max_t
sprintk(S &s, const char *c, usize len)
{
  return s.put(c, len);
}

// class with .c_str() (all string families; bytes as stored)
template<output_sink S, has_cstr T>
  requires micron::is_class_v<T>
max_t
printk(S &s, const T &str)
{
  return s.put(str.c_str(), micron::strlen(str.c_str()));
}

// non-char pointer (prints the address)
template<output_sink S, typename T>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
max_t
printk(S &s, const T &x)
{
  char buf[24];
  usize n = __impl::ptr_to_buf(buf, 24, static_cast<const void *>(x));
  return s.put(buf, n);
}

// fd_t renders as its integer (an fd_t in NON-leading position is data)
template<output_sink S>
inline max_t
printk(S &s, const fd_t &f)
{
  char buf[24];
  usize n = __impl::arith_to_buf(buf, 24, static_cast<i64>(f.fd));
  return s.put(buf, n);
}

template<output_sink S>
inline max_t
printk(S &s, const micron::settle_note &n)
{
  max_t t = s.put(n.pre, micron::strlen(n.pre));
  if ( n.has_id ) {
    char buf[24];
    usize k = __impl::arith_to_buf(buf, 24, n.id);
    t += s.put(buf, k);
  }
  t += s.put(n.post, micron::strlen(n.post));
  return t;
}

// arithmetic
template<output_sink S, typename T>
  requires micron::is_arithmetic_v<T>
max_t
printk(S &s, const T &x)
{
  char buf[128];
  usize n = 0;

  if constexpr ( micron::is_same_v<T, bool> ) {
    n = __impl::arith_to_buf(buf, 128, x);
  } else if constexpr ( micron::is_same_v<T, f32> || micron::is_same_v<T, float> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<f32>(x));
  } else if constexpr ( micron::is_same_v<T, f64> || micron::is_same_v<T, double> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<f64>(x));
  } else if constexpr ( micron::is_same_v<T, long double> || micron::is_same_v<T, flong> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<long double>(x));
  } else if constexpr ( micron::is_signed_v<T> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<i64>(x));
  } else {
    n = __impl::arith_to_buf(buf, 128, static_cast<u64>(x));
  }

  return s.put(buf, n);
}

// volatile arithmetic / pointer: read once, delegate
template<output_sink S, typename T>
  requires micron::is_arithmetic_v<T>
max_t
printk(S &s, const volatile T &x)
{
  T copy = const_cast<const T &>(x);
  return printk(s, copy);
}

template<output_sink S, typename T>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
max_t
printk(S &s, const volatile T &x)
{
  T copy = const_cast<const T &>(x);
  return printk(s, copy);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// classified containers

template<output_sink S> struct __sink_out {
  S &__s;
  max_t __t = 0;

  void
  raw(const char *p, usize n)
  {
    __t += __s.put(p, n);
  }

  void
  num(u64 v)
  {
    char buf[24];
    usize n = __impl::arith_to_buf(buf, 24, v);
    __t += __s.put(buf, n);
  }

  template<typename E>
  void
  elem(const E &e)
  {
    __t += printk(__s, e);
  }
};

template<output_sink S, typename T>
  requires micron::__print::printable<T>
max_t
printk(S &s, const T &x)
{
  __sink_out<S> o{ s };
  micron::__print::render(o, x);
  return o.__t;
}

// pair: [a, b]
template<output_sink S, typename A, typename B>
max_t
printk(S &s, const pair<A, B> &p)
{
  max_t t = s.put("[", 1);
  t += printk(s, p.a);
  t += s.put(", ", 2);
  t += printk(s, p.b);
  t += s.put("]", 1);
  return t;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// printkn(sink, x)

template<output_sink S, typename T>
max_t
printkn(S &s, const T &x)
{
  using U = micron::remove_cvref_t<T>;
  max_t t = 0;
  if constexpr ( has_cstr<U> && micron::is_class_v<U> && requires { typename U::value_type; } ) {
    if constexpr ( micron::is_same_v<typename U::value_type, wchar_t> && fd_backed_sink<S> ) {
      wfput(x.w_str(), s.target());
    } else if constexpr ( micron::is_same_v<typename U::value_type, char32_t> && fd_backed_sink<S> ) {
      io::unifput(x.uni_str(), s.target());
    } else {
      t += printk(s, x);
    }
  } else {
    t += printk(s, x);
  }
  t += s.put('\n');
  return t;
}

namespace __echo_impl
{

template<typename T> struct is_stream_inst: micron::false_type {
};

template<int SZ, int CK> struct is_stream_inst<io::stream<SZ, CK>>: micron::true_type {
};

};      // namespace __echo_impl

// file-like handles are matched structurally (fd() -> fd_t)
template<typename T>
concept __fd_handle = requires(const micron::remove_cvref_t<T> &t) {
  { t.fd() } -> micron::same_as<fd_t>;
} && micron::is_class_v<micron::remove_cvref_t<T>>;

template<typename T>
concept echo_target
    = micron::is_same_v<micron::remove_cvref_t<T>, fd_t> || __echo_impl::is_stream_inst<micron::remove_cvref_t<T>>::value || __fd_handle<T>;

namespace __echo_impl
{

template<output_sink S, typename... Ts>
inline max_t
run(S &s, bool newline, const Ts &...args)
{
  max_t total = 0;
  max_t err = 0;
  (
      [&] {
        max_t r = printk(s, args);
        if ( r < 0 && err == 0 ) [[unlikely]]
          err = r;
        else
          total += r;
      }(),
      ...);
  if ( newline ) total += s.put('\n');
  return err != 0 ? err : total;
}

};      // namespace __echo_impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// echo / echon

template<typename First, typename... Rest>
  requires(!echo_target<First> && !micron::any_settling<First, Rest...>)
inline max_t
echo(const First &first, const Rest &...rest)
{
  stdout_sink s;
  return __echo_impl::run(s, true, first, rest...);
}

// settling form
template<typename First, typename... Rest>
  requires(!echo_target<First> && micron::any_settling<First, Rest...>)
inline max_t
echo(First &&first, Rest &&...rest)
{
  return micron::__settle_impl::__then(
      [](const auto &...v) -> max_t {
        stdout_sink s;
        return __echo_impl::run(s, true, v...);
      },
      micron::forward<First>(first), micron::forward<Rest>(rest)...);
}

inline max_t
echo(void)
{
  stdout_sink s;
  return s.put('\n');
}

// no trailing newline
template<typename First, typename... Rest>
  requires(!echo_target<First> && !micron::any_settling<First, Rest...>)
inline max_t
echon(const First &first, const Rest &...rest)
{
  stdout_sink s;
  max_t r = __echo_impl::run(s, false, first, rest...);
  s.flush();
  return r;
}

template<typename First, typename... Rest>
  requires(!echo_target<First> && micron::any_settling<First, Rest...>)
inline max_t
echon(First &&first, Rest &&...rest)
{
  return micron::__settle_impl::__then(
      [](const auto &...v) -> max_t {
        stdout_sink s;
        max_t r = __echo_impl::run(s, false, v...);
        s.flush();
        return r;
      },
      micron::forward<First>(first), micron::forward<Rest>(rest)...);
}

// redirecting forms
template<typename Target, typename... Ts>
  requires(echo_target<Target> && !micron::any_settling<Ts...>)
inline max_t
echo(Target &&tgt, const Ts &...args)
{
  using U = micron::remove_cvref_t<Target>;
  if constexpr ( micron::is_same_v<U, fd_t> ) {
    if ( tgt.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ tgt };
    max_t r = __echo_impl::run(s, true, args...);
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  } else if constexpr ( __echo_impl::is_stream_inst<U>::value ) {
    stream_sink s{ tgt };
    return __echo_impl::run(s, true, args...);
  } else {      // fd-bearing handle (io::file & friends): write through its fd
    fd_t t = tgt.fd();
    if ( t.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ t };
    max_t r = __echo_impl::run(s, true, args...);
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  }
}

template<typename Target, typename... Ts>
  requires(echo_target<Target> && micron::any_settling<Ts...>)
inline max_t
echo(Target &&tgt, Ts &&...args)
{
  return micron::__settle_impl::__then([&](const auto &...v) -> max_t { return micron::io::echo(micron::forward<Target>(tgt), v...); },
                                       micron::forward<Ts>(args)...);
}

template<typename Target, typename... Ts>
  requires(echo_target<Target> && !micron::any_settling<Ts...>)
inline max_t
echon(Target &&tgt, const Ts &...args)
{
  using U = micron::remove_cvref_t<Target>;
  if constexpr ( micron::is_same_v<U, fd_t> ) {
    if ( tgt.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ tgt };
    max_t r = __echo_impl::run(s, false, args...);
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  } else if constexpr ( __echo_impl::is_stream_inst<U>::value ) {
    stream_sink s{ tgt };
    return __echo_impl::run(s, false, args...);
  } else {
    fd_t t = tgt.fd();
    if ( t.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ t };
    max_t r = __echo_impl::run(s, false, args...);
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  }
}

template<typename Target, typename... Ts>
  requires(echo_target<Target> && micron::any_settling<Ts...>)
inline max_t
echon(Target &&tgt, Ts &&...args)
{
  return micron::__settle_impl::__then([&](const auto &...v) -> max_t { return micron::io::echon(micron::forward<Target>(tgt), v...); },
                                       micron::forward<Ts>(args)...);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// echof / echofn: explicit micron::format {} strings
namespace __echo_impl
{

template<output_sink S>
inline max_t
__put_fill_run(S &s, char fill, usize n)
{
  if ( !n ) return 0;
  char buf[64];
  const usize k0 = n < sizeof(buf) ? n : sizeof(buf);
  for ( usize i = 0; i < k0; ++i ) buf[i] = fill;
  max_t t = 0;
  while ( n ) {
    const usize k = n < sizeof(buf) ? n : sizeof(buf);
    t += s.put(buf, k);
    n -= k;
  }
  return t;
}

template<output_sink S>
inline void
apply_padding_sink(S &s, max_t &total, const char *content, usize content_len, const micron::format::__impl::fmt_spec &spec)
{
  if ( spec.width == 0 || content_len >= spec.width ) {
    total += s.put(content, content_len);
    return;
  }
  usize pad_total = spec.width - content_len;
  char fill = spec.fill ? spec.fill : ' ';
  char align = spec.align;
  if ( align == '\0' ) align = (spec.type == 's' || spec.type == '\0') ? '<' : '>';

  if ( align == '>' ) {
    total += __put_fill_run(s, fill, pad_total);
    total += s.put(content, content_len);
  } else if ( align == '<' ) {
    total += s.put(content, content_len);
    total += __put_fill_run(s, fill, pad_total);
  } else {
    usize left = pad_total / 2;
    usize right = pad_total - left;
    total += __put_fill_run(s, fill, left);
    total += s.put(content, content_len);
    total += __put_fill_run(s, fill, right);
  }
}

template<output_sink S>
inline void
format_one_sink(S &, max_t &, const char *, const char *, usize)
{
  // base case: no args left
}

template<output_sink S, typename T, typename... Rest>
inline void
format_one_sink(S &s, max_t &total, const char *spec_start, const char *spec_end, usize arg_index, const T &val, const Rest &...rest)
{
  if ( arg_index > 0 ) {
    format_one_sink(s, total, spec_start, spec_end, arg_index - 1, rest...);
    return;
  }
  micron::format::__impl::fmt_spec spec = micron::format::__impl::parse_spec(spec_start, spec_end);
  using U = micron::remove_cvref_t<T>;
  if constexpr ( requires(char *b, const U &v) { micron::format::formatter<U>::write(b, usize{}, v, spec); } ) {
    char buf[micron::format::__impl::__fmt_buf_size];
    usize n = micron::format::formatter<U>::write(buf, micron::format::__impl::__fmt_buf_size, val, spec);
    apply_padding_sink(s, total, buf, n, spec);
  } else if constexpr ( requires(hstring<schar> &o, const U &v) { micron::format::formatter<U>::write_str(o, v, spec); } ) {
    hstring<schar> out;
    micron::format::formatter<U>::write_str(out, val, spec);
    apply_padding_sink(s, total, out.c_str(), out.size(), spec);
  } else {
    static_assert(sizeof(U) == 0, "io::echof: no formatter<T> for this argument and it is not an echo-printable container");
  }
}

template<output_sink S, typename... Args>
inline max_t
format_to_sink(S &s, const char *fmt, const Args &...args)
{
  max_t total = 0;
  if ( fmt == nullptr ) return 0;
  usize auto_index = 0;
  const char *p = fmt;
  while ( *p ) {
    if ( *p == '{' ) {
      if ( *(p + 1) == '{' ) {
        total += s.put('{');
        p += 2;
        continue;
      }
      ++p;
      const char *close = p;
      while ( *close && *close != '}' ) ++close;
      if ( *close != '}' ) break;
      const char *colon = p;
      while ( colon < close && *colon != ':' ) ++colon;
      usize index = auto_index;
      bool has_explicit_index = false;
      if ( colon > p ) {
        const char *num_p = p;
        bool all_digits = true;
        while ( num_p < colon ) {
          if ( *num_p < '0' || *num_p > '9' ) {
            all_digits = false;
            break;
          }
          ++num_p;
        }
        if ( all_digits ) {
          index = 0;
          num_p = p;
          while ( num_p < colon ) {
            index = index * 10 + (*num_p - '0');
            ++num_p;
          }
          has_explicit_index = true;
        }
      }
      const char *spec_start = (colon < close) ? colon + 1 : close;
      const char *spec_end = close;
      format_one_sink(s, total, spec_start, spec_end, index, args...);
      if ( !has_explicit_index )
        ++auto_index;
      else
        auto_index = index + 1;
      p = close + 1;
    } else if ( *p == '}' && *(p + 1) == '}' ) {
      total += s.put('}');
      p += 2;
    } else {
      const char *run = p;
      while ( *p && *p != '{' && *p != '}' ) ++p;
      if ( p != run )
        total += s.put(run, static_cast<usize>(p - run));
      else {
        total += s.put(*p);
        ++p;
      }
    }
  }
  return total;
}

};      // namespace __echo_impl

template<typename... Args>
  requires(!micron::any_settling<Args...>)
inline max_t
echof(const char *fmt, const Args &...args)
{
  stdout_sink s;
  max_t r = __echo_impl::format_to_sink(s, fmt, args...);
  r += s.put('\n');
  return r;
}

template<typename... Args>
  requires(micron::any_settling<Args...>)
inline max_t
echof(const char *fmt, Args &&...args)
{
  return micron::__settle_impl::__then([&](const auto &...v) -> max_t { return micron::io::echof(fmt, v...); },
                                       micron::forward<Args>(args)...);
}

template<typename... Args>
  requires(!micron::any_settling<Args...>)
inline max_t
echofn(const char *fmt, const Args &...args)
{
  stdout_sink s;
  max_t r = __echo_impl::format_to_sink(s, fmt, args...);
  s.flush();
  return r;
}

template<typename... Args>
  requires(micron::any_settling<Args...>)
inline max_t
echofn(const char *fmt, Args &&...args)
{
  return micron::__settle_impl::__then([&](const auto &...v) -> max_t { return micron::io::echofn(fmt, v...); },
                                       micron::forward<Args>(args)...);
}

template<typename Target, typename... Args>
  requires(echo_target<Target> && !micron::any_settling<Args...>)
inline max_t
echof(Target &&tgt, const char *fmt, const Args &...args)
{
  using U = micron::remove_cvref_t<Target>;
  if constexpr ( micron::is_same_v<U, fd_t> ) {
    if ( tgt.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ tgt };
    max_t r = __echo_impl::format_to_sink(s, fmt, args...);
    r += s.put('\n');
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  } else if constexpr ( __echo_impl::is_stream_inst<U>::value ) {
    stream_sink s{ tgt };
    max_t r = __echo_impl::format_to_sink(s, fmt, args...);
    r += s.put('\n');
    return r;
  } else {
    fd_t t = tgt.fd();
    if ( t.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ t };
    max_t r = __echo_impl::format_to_sink(s, fmt, args...);
    r += s.put('\n');
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  }
}

template<typename Target, typename... Args>
  requires(echo_target<Target> && micron::any_settling<Args...>)
inline max_t
echof(Target &&tgt, const char *fmt, Args &&...args)
{
  return micron::__settle_impl::__then(
      [&](const auto &...v) -> max_t { return micron::io::echof(micron::forward<Target>(tgt), fmt, v...); },
      micron::forward<Args>(args)...);
}

template<typename Target, typename... Args>
  requires(echo_target<Target> && !micron::any_settling<Args...>)
inline max_t
echofn(Target &&tgt, const char *fmt, const Args &...args)
{
  using U = micron::remove_cvref_t<Target>;
  if constexpr ( micron::is_same_v<U, fd_t> ) {
    if ( tgt.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ tgt };
    max_t r = __echo_impl::format_to_sink(s, fmt, args...);
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  } else if constexpr ( __echo_impl::is_stream_inst<U>::value ) {
    stream_sink s{ tgt };
    return __echo_impl::format_to_sink(s, fmt, args...);
  } else {
    fd_t t = tgt.fd();
    if ( t.fd < 0 ) [[unlikely]]
      return -error::bad_fd;
    fd_sink<> s{ t };
    max_t r = __echo_impl::format_to_sink(s, fmt, args...);
    max_t f = s.flush();
    if ( s.__err ) [[unlikely]]
      return s.__err;
    return f < 0 ? f : r;
  }
}

template<typename Target, typename... Args>
  requires(echo_target<Target> && micron::any_settling<Args...>)
inline max_t
echofn(Target &&tgt, const char *fmt, Args &&...args)
{
  return micron::__settle_impl::__then(
      [&](const auto &...v) -> max_t { return micron::io::echofn(micron::forward<Target>(tgt), fmt, v...); },
      micron::forward<Args>(args)...);
}

inline void
flush(void)
{
  io::fflush(stdout);
}

template<typename... T>
  requires(!micron::any_settling<T...>)
inline void
print(const T &...str)
{
  stdout_sink s;
  (printk(s, str), ...);
  flush();
}

template<typename... T>
void
print(T &&...str)
{
  if constexpr ( micron::any_settling<T...> ) {
    micron::__settle_impl::__then(
        [](const auto &...v) {
          stdout_sink s;
          (printk(s, v), ...);
          flush();
        },
        micron::forward<T>(str)...);
  } else {
    stdout_sink s;
    (printk(s, str), ...);
    flush();
  }
}

template<typename... T>
  requires(!micron::any_settling<T...>)
inline void
printn(const T &...str)
{
  stdout_sink s;
  (printkn(s, str), ...);
}

template<typename... T>
void
printn(T &&...str)
{
  if constexpr ( micron::any_settling<T...> ) {
    micron::__settle_impl::__then(
        [](const auto &...v) {
          stdout_sink s;
          (printkn(s, v), ...);
        },
        micron::forward<T>(str)...);
  } else {
    stdout_sink s;
    (printkn(s, str), ...);
  }
}

template<typename... T>
  requires(!micron::any_settling<T...>)
inline void
println(const T &...str)
{
  stdout_sink s;
  if constexpr ( sizeof...(T) > 1 ) {
    (printk(s, str), ...);
    s.put('\n');
  } else {
    (printkn(s, str), ...);
  }
}

template<typename... T>
  requires(micron::any_settling<T...>)
inline void
println(T &&...str)
{
  micron::__settle_impl::__then(
      [](const auto &...v) {
        stdout_sink s;
        if constexpr ( sizeof...(v) > 1 ) {
          (printk(s, v), ...);
          s.put('\n');
        } else {
          (printkn(s, v), ...);
        }
      },
      micron::forward<T>(str)...);
}

template<typename... T>
inline void
println(const T *...str)
{
  stdout_sink s;
  if constexpr ( sizeof...(T) > 1 ) {
    (printk(s, str), ...);
    s.put('\n');
  } else {
    (printkn(s, str), ...);
  }
}

template<typename... T>
inline void
error(T &&...str)
{
  if constexpr ( micron::any_settling<T...> ) {
    micron::__settle_impl::__then(
        [](const auto &...v) {
          stderr_sink s;
          (printk(s, v), ...);
        },
        micron::forward<T>(str)...);
  } else {
    stderr_sink s;
    (printk(s, str), ...);
  }
}

template<typename... T>
inline void
errorn(T &&...str)
{
  if constexpr ( micron::any_settling<T...> ) {
    micron::__settle_impl::__then(
        [](const auto &...v) {
          stderr_sink s;
          ((printk(s, v), s.put('\n')), ...);
        },
        micron::forward<T>(str)...);
  } else {
    stderr_sink s;
    ((printk(s, str), s.put('\n')), ...);
  }
}

template<typename... T>
inline void
errorln(T &&...str)
{
  if constexpr ( micron::any_settling<T...> ) {
    micron::__settle_impl::__then(
        [](const auto &...v) {
          stderr_sink s;
          (printk(s, v), ...);
          s.put('\n');
        },
        micron::forward<T>(str)...);
  } else {
    stderr_sink s;
    (printk(s, str), ...);
    s.put('\n');
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// binary hex dump

template<is_printable_container T>
void
bin(const T &data)
{
  stdout_sink s;
  char buf[4096];
  usize cnt = 0;
  usize remaining = data.size();
  while ( remaining > 0 ) {
    usize chunk = remaining > 2047 ? 2047 : remaining;      // 2*chunk+1 must stay within buf[4096]
    const auto *src = reinterpret_cast<const u8 *>(&data[cnt]);
    for ( usize i = 0; i < chunk; ++i ) {
      buf[i * 2] = micron::__impl::__hex_lower[src[i] >> 4];
      buf[i * 2 + 1] = micron::__impl::__hex_lower[src[i] & 0xF];
    }
    s.put(buf, chunk * 2);
    cnt += chunk;
    remaining -= chunk;
  }
}

template<has_cstr T>
void
bin(const T &data)
{
  stdout_sink s;
  char buf[4096];
  usize cnt = 0;
  usize remaining = data.size();
  while ( remaining > 0 ) {
    usize chunk = remaining > 2047 ? 2047 : remaining;
    const auto *src = reinterpret_cast<const u8 *>(&data[cnt]);
    for ( usize i = 0; i < chunk; ++i ) {
      buf[i * 2] = micron::__impl::__hex_lower[src[i] >> 4];
      buf[i * 2 + 1] = micron::__impl::__hex_lower[src[i] & 0xF];
    }
    s.put(" 0x", 3);
    s.put(buf, chunk * 2);
    cnt += chunk;
    remaining -= chunk;
  }
}

// stubbed
inline void
print_buffer(char **buf [[maybe_unused]])
{
}

};      // namespace io
};      // namespace micron
