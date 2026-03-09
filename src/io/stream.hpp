//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../types.hpp"

#include "posix/iosys.hpp"

namespace micron
{
namespace io
{

constexpr int __buffer_size = 32768;
constexpr int __buffer_packet = 4096;

template <typename Fn>
concept intercept_fn = requires(Fn fn, byte *data, usize sz) { fn(data, sz); };

template <typename Fn>
concept encode_fn = requires(Fn fn, byte *dst, const byte *src, usize len) {
  { fn(dst, src, len) } -> same_as<usize>;
};

template <typename C>
concept byte_container = requires(C c) {
  { c.begin() } -> convertible_to<const byte *>;
  { c.size() } -> convertible_to<usize>;
  typename C::value_type;
};

struct stream_result_t {
  bool ok;
  usize written;

  explicit
  operator bool() const noexcept
  {
    return ok;
  }
};

struct __intercept_holder {
  void (*fn)(void *ctx, byte *data, usize sz) = nullptr;
  void (*destroy)(void *ctx) = nullptr;
  alignas(16) byte ctx[64] = {};     // up to 64 bytes of closure

  bool
  valid() const noexcept
  {
    return fn != nullptr;
  }

  void
  call(byte *data, usize sz) const
  {
    if ( fn )
      fn(const_cast<void *>(static_cast<const void *>(ctx)), data, sz);
  }

  void
  reset()
  {
    if ( destroy )
      destroy(ctx);
    fn = nullptr;
    destroy = nullptr;
    micron::zero(ctx);
  }
};

template <int __sz = __buffer_size, int __chnk = __buffer_packet> class stream
{
  max_t __size;
  micron::pointer<micron::buffer> __buffer;
  __intercept_holder __hook;

  void
  __check_fd(const fd_t &fd, const char *where) const
  {
    if ( fd.has_error() || fd.closed() )
      exc<except::io_error>(where);
  }

  void
  __intercept()
  {
    if ( __hook.valid() )
      __hook.call(__buffer->begin(), static_cast<usize>(__size));
  }

  usize
  __raw_append(const byte *src, usize len)
  {
    usize avail = static_cast<usize>(__sz) - static_cast<usize>(__size);
    usize take = len < avail ? len : avail;
    micron::memcpy(__buffer->begin() + __size, src, take);
    __size += static_cast<max_t>(take);
    return take;
  }

public:
  ~stream() { __hook.reset(); }

  stream() : __size(0), __buffer(__sz), __hook{} {}

  stream(const stream &) = delete;
  stream &operator=(const stream &) = delete;

  stream(stream &&o) noexcept : __size(o.__size), __buffer(micron::move(o.__buffer)), __hook(o.__hook)
  {
    o.__size = 0;
    o.__hook.fn = nullptr;
    o.__hook.destroy = nullptr;
  }

  stream &
  operator=(stream &&o) noexcept
  {
    if ( this == &o )
      return *this;
    __hook.reset();
    __size = o.__size;
    __buffer = micron::move(o.__buffer);
    __hook = o.__hook;
    o.__size = 0;
    o.__hook.fn = nullptr;
    o.__hook.destroy = nullptr;
    return *this;
  }

  template <intercept_fn Fn>
  void
  set_intercept(Fn &&fn)
  {
    static_assert(sizeof(Fn) <= sizeof(__hook.ctx), "micron::stream intercept closure exceeds 64 bytes; use a pointer.");
    __hook.reset();
    new (static_cast<void *>(__hook.ctx)) Fn(micron::move(fn));
    __hook.fn = [](void *ctx, byte *data, usize sz) { (*reinterpret_cast<Fn *>(ctx))(data, sz); };
    __hook.destroy = [](void *ctx) { reinterpret_cast<Fn *>(ctx)->~Fn(); };
  }

  void
  clear_intercept() noexcept
  {
    __hook.reset();
  }

  bool
  has_intercept() const noexcept
  {
    return __hook.valid();
  }

  void
  run_intercept()
  {
    if ( __size )
      __intercept();
  }

  template <encode_fn Fn>
  usize
  encode(Fn &&fn, const byte *src, usize src_len)
  {

    micron::buffer tmp(__sz);
    usize out_len = fn(tmp.begin(), src, src_len);
    usize appended = __raw_append(tmp.begin(), out_len);
    if ( appended )
      __intercept();
    return appended;
  }

  template <encode_fn Fn>
  usize
  encode(Fn &&fn, const byte *src, usize src_len, micron::buffer &scratch)
  {

    if ( scratch.size() < src_len )
      exc<except::io_error>("micron::stream::encode() scratch buffer too small.");
    usize out_len = fn(scratch.begin(), src, src_len);
    usize appended = __raw_append(scratch.begin(), out_len);
    if ( appended )
      __intercept();
    return appended;
  }

  template <encode_fn Fn, is_string T>
  usize
  encode(Fn &&fn, const T &in)
  {
    return encode(micron::move(fn), reinterpret_cast<const byte *>(in.c_str()), in.size());
  }

  template <encode_fn Fn, is_container_or_string T>
  usize
  encode(Fn &&fn, const T &in)
  {
    return encode(micron::move(fn), reinterpret_cast<const byte *>(in.begin()), in.size() * sizeof(typename T::value_type));
  }

  template <encode_fn Fn>
  usize
  encode_into(Fn &&fn, byte *dst, usize dst_cap) const
  {
    if ( __size == 0 )
      return 0;
    return fn(dst, __buffer->begin(), static_cast<usize>(__size));
  }

  template <encode_fn Fn>
  usize
  encode_into(Fn &&fn, micron::buffer &dst) const
  {
    return encode_into(micron::move(fn), dst.begin(), dst.size());
  }

  template <encode_fn Fn>
  void
  encode_inplace(Fn &&fn)
  {
    if ( __size == 0 )
      return;
    micron::buffer tmp(static_cast<usize>(__size));
    usize out_len = fn(tmp.begin(), __buffer->begin(), static_cast<usize>(__size));
    usize copy = out_len < static_cast<usize>(__sz) ? out_len : static_cast<usize>(__sz);
    micron::memcpy(__buffer->begin(), tmp.begin(), copy);
    __size = static_cast<max_t>(copy);
    __intercept();
  }

  template <is_string T>
  stream &
  operator<<(const T &in)
  {
    if ( in.empty() )
      exc<except::io_error>("io::stream operator<<: input string is empty.");
    __raw_append(reinterpret_cast<const byte *>(in.c_str()), in.size() * sizeof(typename T::value_type));
    __intercept();
    return *this;
  }

  template <is_container_or_string T>
    requires(!is_string<T>)
  stream &
  operator<<(const T &in)
  {
    if ( in.empty() )
      exc<except::io_error>("io::stream operator<<: input container is empty.");
    __raw_append(reinterpret_cast<const byte *>(in.begin()), in.size() * sizeof(typename T::value_type));
    __intercept();
    return *this;
  }

  stream &
  operator<<(const fd_t &in)
  {
    __check_fd(in, "io::stream operator<<: fd_t is closed or has an error");
    usize seek = static_cast<usize>(posix::lseek(in.fd, 0, posix::seek_cur));
    do {
      usize avail = static_cast<usize>(__sz) - static_cast<usize>(__size);
      usize chunk = static_cast<usize>(__chnk) < avail ? static_cast<usize>(__chnk) : avail;
      max_t bytes_read = posix::read(in.fd, __buffer->at_pointer(__size), chunk);
      if ( bytes_read <= 0 )
        break;
      seek += static_cast<usize>(bytes_read);
      __size += bytes_read;
      posix::lseek(in.fd, static_cast<posix::off_t>(seek), posix::seek_set);
    } while ( static_cast<usize>(__size) < static_cast<usize>(__sz) );
    __intercept();
    return *this;
  }

  stream &
  append(const byte *src, usize len)
  {
    __raw_append(src, len);
    __intercept();
    return *this;
  }

  template <typename T>
  stream &
  append(const T *ptr, usize count)
  {
    __raw_append(reinterpret_cast<const byte *>(ptr), count * sizeof(T));
    __intercept();
    return *this;
  }

  template <typename T>
  stream &
  append(const T &obj, usize count)
  {
    __raw_append(micron::real_addr_as<const byte *>(obj), count * sizeof(T));
    __intercept();
    return *this;
  }

  stream &
  append_byte(byte b)
  {
    if ( static_cast<usize>(__size) >= static_cast<usize>(__sz) )
      exc<except::io_error>("io::stream::append_byte: buffer full.");
    (*__buffer)[static_cast<usize>(__size)] = b;
    ++__size;
    __intercept();
    return *this;
  }

  stream &
  operator>>(const fd_t &out)
  {
    __check_fd(out, "io::stream operator>>: fd_t is closed or has an error");
    if ( __size == 0 ) [[unlikely]]
      return *this;
    usize buf_i = 0;
    do {
      usize chunk = static_cast<usize>(__chnk) < static_cast<usize>(__size) ? static_cast<usize>(__chnk) : static_cast<usize>(__size);
      max_t sz = posix::write(out.fd, __buffer->at_pointer(buf_i), chunk);
      if ( sz == -1 ) [[unlikely]]
        exc<except::io_error>("io::stream operator>>: write failed.");
      if ( sz == 0 )
        break;
      buf_i += chunk;
      __size -= sz;
    } while ( __size > 0 );
    return *this;
  }

  template <is_string T>
  stream &
  operator>>(T &out)
  {
    if ( __size == 0 )
      return *this;
    out.append(reinterpret_cast<const typename T::value_type *>(__buffer->begin()), static_cast<usize>(__size));
    clear();
    return *this;
  }

  template <is_container_or_string T>
    requires(!is_string<T>)
  stream &
  operator>>(T &out)
  {
    if ( __size == 0 )
      return *this;
    const byte *p = __buffer->begin();
    for ( max_t i = 0; i < __size; ++i )
      out.push_back(static_cast<typename T::value_type>(p[i]));
    clear();
    return *this;
  }

  template <is_string T>
  void
  dump(T &out) const
  {
    if ( __size == 0 )
      return;
    out.append(reinterpret_cast<const typename T::value_type *>(__buffer->begin()), static_cast<usize>(__size));
  }

  void
  dump(micron::buffer &out) const
  {
    if ( __size == 0 )
      return;
    if ( out.size() < static_cast<usize>(__size) )
      out.resize(static_cast<usize>(__size));
    micron::memcpy(out.begin(), __buffer->begin(), static_cast<usize>(__size));
  }

  usize
  dump(byte *dst, usize dst_cap) const
  {
    usize copy = static_cast<usize>(__size) < dst_cap ? static_cast<usize>(__size) : dst_cap;
    micron::memcpy(dst, __buffer->begin(), copy);
    return copy;
  }

  template <is_string T>
  void
  dump_and_clear(T &out)
  {
    dump(out);
    clear();
  }

  void
  dump_and_clear(micron::buffer &out)
  {
    dump(out);
    clear();
  }

  stream_result_t
  flush_to(const fd_t &out)
  {
    if ( out.has_error() || out.closed() )
      return { false, 0 };
    if ( __size == 0 )
      return { true, 0 };
    usize buf_i = 0;
    max_t remaining = __size;
    do {
      usize chunk = static_cast<usize>(__chnk) < static_cast<usize>(remaining) ? static_cast<usize>(__chnk) : static_cast<usize>(remaining);
      max_t sz = posix::write(out.fd, __buffer->at_pointer(buf_i), chunk);
      if ( sz == -1 )
        return { false, buf_i };
      if ( sz == 0 )
        break;
      buf_i += chunk;
      remaining -= sz;
    } while ( remaining > 0 );
    return { true, buf_i };
  }

  const byte *
  data() const noexcept
  {
    return __buffer->begin();
  }

  byte *
  data() noexcept
  {
    return __buffer->begin();
  }

  max_t
  size() const noexcept
  {
    return __size;
  }

  max_t
  remaining() const noexcept
  {
    return __sz - __size;
  }

  usize
  max_size() const noexcept
  {
    return static_cast<usize>(__sz);
  }

  usize
  chunk_size() const noexcept
  {
    return static_cast<usize>(__chnk);
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  bool
  full(usize extra = 0) const noexcept
  {
    return (static_cast<usize>(__size) + extra) >= static_cast<usize>(__sz);
  }

  byte
  operator[](usize i) const
  {
    if ( i >= static_cast<usize>(__size) )
      exc<except::runtime_error>("io::stream[]: index out of bounds.");
    return (*__buffer)[i];
  }

  byte &
  operator[](usize i)
  {
    if ( i >= static_cast<usize>(__size) )
      exc<except::runtime_error>("io::stream[]: index out of bounds.");
    return (*__buffer)[i];
  }

  byte *
  begin() noexcept
  {
    return __buffer->begin();
  }

  byte *
  end() noexcept
  {
    return __buffer->begin() + __size;
  }

  const byte *
  begin() const noexcept
  {
    return __buffer->begin();
  }

  const byte *
  end() const noexcept
  {
    return __buffer->begin() + __size;
  }

  bool
  operator==(const stream &o) const noexcept
  {
    if ( __size != o.__size )
      return false;
    const byte *a = __buffer->begin();
    const byte *b = o.__buffer->begin();
    for ( max_t i = 0; i < __size; ++i )
      if ( a[i] != b[i] )
        return false;
    return true;
  }

  bool
  operator!=(const stream &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const stream &o) const noexcept
  {
    const byte *a = __buffer->begin();
    const byte *b = o.__buffer->begin();
    max_t n = __size < o.__size ? __size : o.__size;
    for ( max_t i = 0; i < n; ++i ) {
      if ( a[i] < b[i] )
        return true;
      if ( a[i] > b[i] )
        return false;
    }
    return __size < o.__size;
  }

  bool
  operator>(const stream &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const stream &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const stream &o) const noexcept
  {
    return !(*this < o);
  }

  bool
  operator==(const byte *buf) const noexcept
  {
    if ( !buf )
      return false;
    const byte *p = __buffer->begin();
    for ( max_t i = 0; i < __size; ++i )
      if ( p[i] != buf[i] )
        return false;
    return true;
  }

  bool
  operator!=(const byte *buf) const noexcept
  {
    return !(*this == buf);
  }

  template <is_string T>
  bool
  operator==(const T &str) const noexcept
  {
    if ( static_cast<usize>(__size) != str.size() * sizeof(typename T::value_type) )
      return false;
    return operator==(reinterpret_cast<const byte *>(str.c_str()));
  }

  template <is_string T>
  bool
  operator!=(const T &str) const noexcept
  {
    return !(*this == str);
  }

  void
  clear()
  {
    __size = 0;
    micron::czero<__sz>(__buffer->begin());
  }

  void
  clear_valid()
  {
    if ( __size )
      micron::memset(__buffer->begin(), 0, static_cast<usize>(__size));
    __size = 0;
  }

  void
  rewind() noexcept
  {
    __size = 0;
  }

  micron::buffer *
  pass()
  {
    __size = 0;
    return __buffer.release();
  }

  void
  receive(micron::buffer *ptr)
  {
    if ( ptr )
      __buffer = ptr;
  }
};

template <int __sz = __buffer_size, int __chnk = __buffer_packet> class stream_view
{
  max_t __size;
  micron::wptr<micron::buffer> __buffer;
  __intercept_holder __hook;

  void
  __check_fd(const fd_t &fd, const char *where) const
  {
    if ( fd.has_error() || fd.closed() )
      exc<except::io_error>(where);
  }

  void
  __intercept()
  {
    if ( __hook.valid() )
      __hook.call(__buffer->begin(), static_cast<usize>(__size));
  }

  usize
  __raw_append(const byte *src, usize len)
  {
    usize avail = static_cast<usize>(__sz) - static_cast<usize>(__size);
    usize take = len < avail ? len : avail;
    micron::memcpy(__buffer->begin() + __size, src, take);
    __size += static_cast<max_t>(take);
    return take;
  }

public:
  ~stream_view() { __hook.reset(); }

  stream_view() = delete;

  template <typename T> explicit stream_view(T &p) : __size(0), __buffer(p), __hook{} {}

  stream_view(const stream_view &) = delete;
  stream_view(stream_view &&) = delete;
  stream_view &operator=(const stream_view &) = delete;
  stream_view &operator=(stream_view &&) = delete;

  template <intercept_fn Fn>
  void
  set_intercept(Fn &&fn)
  {
    static_assert(sizeof(Fn) <= sizeof(__hook.ctx), "micron::stream_view intercept closure exceeds 64 bytes.");
    __hook.reset();
    new (static_cast<void *>(__hook.ctx)) Fn(micron::move(fn));
    __hook.fn = [](void *ctx, byte *data, usize sz) { (*reinterpret_cast<Fn *>(ctx))(data, sz); };
    __hook.destroy = [](void *ctx) { reinterpret_cast<Fn *>(ctx)->~Fn(); };
  }

  void
  clear_intercept() noexcept
  {
    __hook.reset();
  }

  bool
  has_intercept() const noexcept
  {
    return __hook.valid();
  }

  void
  run_intercept()
  {
    if ( __size )
      __intercept();
  }

  template <encode_fn Fn>
  usize
  encode_into(Fn &&fn, byte *dst, usize dst_cap) const
  {
    if ( __size == 0 )
      return 0;
    return fn(dst, __buffer->begin(), static_cast<usize>(__size));
  }

  template <encode_fn Fn>
  usize
  encode_into(Fn &&fn, micron::buffer &dst) const
  {
    return encode_into(micron::move(fn), dst.begin(), dst.size());
  }

  template <is_string T>
  stream_view &
  operator<<(const T &in)
  {
    if ( in.empty() )
      exc<except::io_error>("io::stream_view operator<<: input is empty.");
    __raw_append(reinterpret_cast<const byte *>(in.c_str()), in.size() * sizeof(typename T::value_type));
    __intercept();
    return *this;
  }

  template <is_container_or_string T>
    requires(!is_string<T>)
  stream_view &
  operator<<(const T &in)
  {
    if ( in.empty() )
      exc<except::io_error>("io::stream_view operator<<: input is empty.");
    __raw_append(reinterpret_cast<const byte *>(in.begin()), in.size() * sizeof(typename T::value_type));
    __intercept();
    return *this;
  }

  stream_view &
  operator<<(const fd_t &in)
  {
    __check_fd(in, "io::stream_view operator<<: fd_t is closed or has an error");
    usize seek = static_cast<usize>(posix::lseek(in.fd, 0, posix::seek_cur));
    do {
      usize avail = static_cast<usize>(__sz) - static_cast<usize>(__size);
      usize chunk = static_cast<usize>(__chnk) < avail ? static_cast<usize>(__chnk) : avail;
      max_t bytes_read = posix::read(in.fd, __buffer->at_pointer(__size), chunk);
      if ( bytes_read <= 0 )
        break;
      seek += static_cast<usize>(bytes_read);
      __size += bytes_read;
      posix::lseek(in.fd, static_cast<posix::off_t>(seek), posix::seek_set);
    } while ( static_cast<usize>(__size) < static_cast<usize>(__sz) );
    __intercept();
    return *this;
  }

  stream_view &
  operator>>(const fd_t &out)
  {
    __check_fd(out, "io::stream_view operator>>: fd_t is closed or has an error");
    if ( __size == 0 )
      return *this;
    usize buf_i = 0;
    max_t remaining = __size;
    do {
      usize chunk = static_cast<usize>(__chnk) < static_cast<usize>(remaining) ? static_cast<usize>(__chnk) : static_cast<usize>(remaining);
      max_t sz = posix::write(out.fd, __buffer->at_pointer(buf_i), chunk);
      if ( sz == -1 )
        exc<except::io_error>("io::stream_view operator>>: write failed.");
      if ( sz == 0 )
        break;
      buf_i += chunk;
      remaining -= sz;
    } while ( remaining > 0 );
    return *this;
  }

  template <is_string T>
  stream_view &
  operator>>(T &out)
  {
    if ( __size == 0 )
      return *this;
    out.append(reinterpret_cast<const typename T::value_type *>(__buffer->begin()), static_cast<usize>(__size));
    clear();
    return *this;
  }

  template <is_string T>
  void
  dump(T &out) const
  {
    if ( __size == 0 )
      return;
    out.append(reinterpret_cast<const typename T::value_type *>(__buffer->begin()), static_cast<usize>(__size));
  }

  void
  dump(micron::buffer &out) const
  {
    if ( __size == 0 )
      return;
    if ( out.size() < static_cast<usize>(__size) )
      out.resize(static_cast<usize>(__size));
    micron::memcpy(out.begin(), __buffer->begin(), static_cast<usize>(__size));
  }

  usize
  dump(byte *dst, usize dst_cap) const
  {
    usize copy = static_cast<usize>(__size) < dst_cap ? static_cast<usize>(__size) : dst_cap;
    micron::memcpy(dst, __buffer->begin(), copy);
    return copy;
  }

  const byte *
  data() const noexcept
  {
    return __buffer->begin();
  }

  byte *
  data() noexcept
  {
    return __buffer->begin();
  }

  max_t
  size() const noexcept
  {
    return __size;
  }

  usize
  max_size() const noexcept
  {
    return static_cast<usize>(__sz);
  }

  usize
  chunk_size() const noexcept
  {
    return static_cast<usize>(__chnk);
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  bool
  full(usize extra = 0) const noexcept
  {
    return (static_cast<usize>(__size) + extra) >= static_cast<usize>(__sz);
  }

  byte *
  begin() noexcept
  {
    return __buffer->begin();
  }

  byte *
  end() noexcept
  {
    return __buffer->begin() + __size;
  }

  const byte *
  begin() const noexcept
  {
    return __buffer->begin();
  }

  const byte *
  end() const noexcept
  {
    return __buffer->begin() + __size;
  }

  bool
  operator==(const stream_view &o) const noexcept
  {
    if ( __size != o.__size )
      return false;
    const byte *a = __buffer->begin();
    const byte *b = o.__buffer->begin();
    for ( max_t i = 0; i < __size; ++i )
      if ( a[i] != b[i] )
        return false;
    return true;
  }

  bool
  operator!=(const stream_view &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const stream_view &o) const noexcept
  {
    const byte *a = __buffer->begin();
    const byte *b = o.__buffer->begin();
    max_t n = __size < o.__size ? __size : o.__size;
    for ( max_t i = 0; i < n; ++i ) {
      if ( a[i] < b[i] )
        return true;
      if ( a[i] > b[i] )
        return false;
    }
    return __size < o.__size;
  }

  bool
  operator>(const stream_view &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const stream_view &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const stream_view &o) const noexcept
  {
    return !(*this < o);
  }

  template <int S, int C>
  bool
  operator==(const stream<S, C> &o) const noexcept
  {
    if ( __size != o.size() )
      return false;
    const byte *a = __buffer->begin();
    const byte *b = o.data();
    for ( max_t i = 0; i < __size; ++i )
      if ( a[i] != b[i] )
        return false;
    return true;
  }

  template <is_string T>
  bool
  operator==(const T &str) const noexcept
  {
    if ( static_cast<usize>(__size) != str.size() * sizeof(typename T::value_type) )
      return false;
    const byte *p = __buffer->begin();
    const byte *s = reinterpret_cast<const byte *>(str.c_str());
    for ( max_t i = 0; i < __size; ++i )
      if ( p[i] != s[i] )
        return false;
    return true;
  }

  template <is_string T>
  bool
  operator!=(const T &str) const noexcept
  {
    return !(*this == str);
  }

  void
  clear()
  {
    __size = 0;
    micron::czero<__sz>(__buffer->begin());
  }

  void
  clear_valid()
  {
    if ( __size )
      micron::memset(__buffer->begin(), 0, static_cast<usize>(__size));
    __size = 0;
  }

  void
  rewind() noexcept
  {
    __size = 0;
  }
};

template <int SZ_A, int CK_A, int SZ_B, int CK_B>
inline void
pipe(stream<SZ_A, CK_A> &src, stream<SZ_B, CK_B> &dst)
{
  if ( src.empty() )
    return;
  dst.append(src.data(), static_cast<usize>(src.size()));
  src.rewind();
}

template <int SZ_A, int CK_A, int SZ_B, int CK_B, encode_fn Fn>
inline void
pipe_encode(stream<SZ_A, CK_A> &src, stream<SZ_B, CK_B> &dst, Fn &&fn)
{
  if ( src.empty() )
    return;
  dst.encode(micron::move(fn), src.data(), static_cast<usize>(src.size()));
  src.rewind();
}

template <int SZ, int CK>
inline bool
operator==(const byte *buf, const stream<SZ, CK> &s) noexcept
{
  return s == buf;
}

template <int SZ, int CK>
inline bool
operator!=(const byte *buf, const stream<SZ, CK> &s) noexcept
{
  return !(s == buf);
}

};     // namespace io
};     // namespace micron
