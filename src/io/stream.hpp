//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"

#include "posix/iosys.hpp"

namespace micron
{

namespace io
{

constexpr int __buffer_size = 32768;
constexpr int __buffer_packet = 4096;

// this object simply reads an existing opened fd_t, it doesn't handle it directly
template <int __sz = __buffer_size, int __chnk = __buffer_packet> class stream
{
  ssize_t __size;
  micron::pointer<micron::buffer> __buffer;

public:
  ~stream() = default;
  stream(void) : __size(0), __buffer(__sz) {}
  stream(const stream &o) = delete;
  stream(stream &&o) : __size(o.__size), __buffer(micron::move(__buffer)) { o.__size = 0; }
  stream &operator=(const stream &o) = delete;
  stream &
  operator=(stream &&o)
  {
    __size = o.__size;
    o.__size = 0;
    __buffer = micron::move(o.__buffer);
    return *this;
  }
  stream &
  operator>>(const fd_t &out)
  {
    if ( out.has_error() or out.closed() )
      exc<except::io_error>("io::stream() operator>>: fd_t is closed or has an error");
    if ( __size ) {
      size_t __buf_i = 0;
      do {
        ssize_t sz = posix::write(out.fd, __buffer->at_pointer(__buf_i), __chnk > __size ? __size : __chnk);
        if ( sz == -1 )
          exc<except::io_error>("posix::write() operator>>: writing failed.");
        if ( sz == 0 )
          break;
        __buf_i += __chnk > __size ? __size : __chnk;
        __size -= sz;
      } while ( __size > 0 );
    }
    return *this;
  }
  template <typename T>
  stream &
  append(T* ptr, size_t count)
  {
    ssize_t c = 0;
    if ( __sz <= (count * sizeof(T)) )
      c = __sz;
    else
      c = count * sizeof(T);
    micron::memcpy(&(*__buffer)[__size], reinterpret_cast<const byte*>(ptr), c);
    __size += c;
    return *this;
  }
  template <is_container_or_string T>
  stream &
  operator<<(const T &in)
  {
    if ( in.empty() )
      exc<except::io_error>("io::stream() operator>>: input container is empty.");
    ssize_t c = 0;
    if ( __sz <= (in.size() * sizeof(typename T::value_type)) )
      c = __sz + __size;
    else
      c = in.size() * sizeof(typename T::value_type) + __size;
    ssize_t __o = __size;
    for ( ssize_t i = __size; i < c; ++i, ++__size )
      *(__buffer->at_pointer(i)) = *(reinterpret_cast<const byte *>(in.begin()) + (i - __o));
    return *this;
  }
  stream &
  operator<<(const fd_t &in)
  {
    if ( in.has_error() or in.closed() )
      exc<except::io_error>("io::stream() operator>>: fd_t is closed or has an error");
    size_t seek = posix::lseek(in.fd, 0, seek_cur);
    do {
      ssize_t bytes_read
          = posix::read(in.fd, __buffer->at_pointer(__size), (__chnk > (__sz - __size)) ? (__sz - __size) : __chnk);
      if ( bytes_read == 0 )
        break;
      seek += static_cast<size_t>(bytes_read);
      __size += bytes_read;
      posix::lseek(in.fd, seek, seek_set);
    } while ( __size < __sz );
    return *this;
  }
  bool
  full(const ssize_t n = 0) const
  {
    return ((size() + n) >= max_size());
  }
  bool
  full(const size_t n = 0) const
  {
    // NOTE: fails if above size of size_t / 2 
    return ((size() + static_cast<ssize_t>(n)) >= max_size());
  }
  byte *
  data()
  {
    return (*__buffer).begin();
  }
  const byte *
  data() const
  {
    return (*__buffer).cbegin();
  }
  auto
  size() const
  {
    return __size;
  }
  auto
  max_size() const
  {
    return __sz;
  }
  auto
  chunk_size() const
  {
    return __chnk;
  }
  void
  clear()
  {
    __size = 0;
    micron::czero<__sz>(__buffer->begin());
  }
  micron::buffer *
  pass()
  {
    return __buffer.release();
  }
  void
  receive(micron::buffer *ptr)
  {
    if ( ptr != nullptr ) {
      __buffer = ptr;
    }
  }
};

template <int __sz = __buffer_size, int __chnk = __buffer_packet> class stream_view
{
  ssize_t __size;
  micron::wptr<micron::buffer> __buffer;

public:
  ~stream_view() {}
  stream_view(void) = delete;
  template <typename T> stream_view(T &p) : __buffer(p){};
  stream_view(const stream_view &o) = delete;
  stream_view(stream_view &&o) = delete;
  stream_view &operator=(const stream_view &o) = delete;
  stream_view &operator=(stream_view &&) = delete;
  stream_view &
  operator>>(const fd_t &out)
  {
    if ( out.has_error() or out.closed() )
      exc<except::io_error>("io::stream_view() operator>>: fd_t is closed or has an error");
    if ( __size ) {
      size_t __buf_i = 0;
      do {
        ssize_t sz = posix::write(out.fd, __buffer->at_pointer(__buf_i), __chnk > __size ? __size : __chnk);
        if ( sz == -1 )
          exc<except::io_error>("posix::write() operator>>: writing failed.");
        if ( sz == 0 )
          break;
        __buf_i += __chnk > __size ? __size : __chnk;
        __size -= sz;
      } while ( __size > 0 );
    }
    return *this;
  }

  template <is_container_or_string T>
  stream_view &
  operator<<(const T &in)
  {
    if ( in.empty() )
      exc<except::io_error>("io::stream_view() operator>>: input container is empty.");
    ssize_t c = 0;
    if ( __sz <= (in.size() * sizeof(typename T::value_type)) )
      c = __sz + __size;
    else
      c = in.size() * sizeof(typename T::value_type) + __size;
    ssize_t __o = __size;
    for ( ssize_t i = __size; i < c; ++i, ++__size )
      *(__buffer->at_pointer(i)) = *(reinterpret_cast<const byte *>(in.begin()) + (i - __o));
    return *this;
  }
  stream_view &
  operator<<(const fd_t &in)
  {
    if ( in.has_error() or in.closed() )
      exc<except::io_error>("io::stream_view() operator>>: fd_t is closed or has an error");
    size_t seek = posix::lseek(in.fd, 0, seek_cur);
    do {
      ssize_t bytes_read
          = posix::read(in.fd, __buffer->at_pointer(__size), (__chnk > (__sz - __size)) ? (__sz - __size) : __chnk);
      if ( bytes_read == 0 )
        break;
      seek += static_cast<size_t>(bytes_read);
      __size += bytes_read;
      posix::lseek(in.fd, seek, seek_set);
    } while ( __size < __sz );
    return *this;
  }
  byte *
  data()
  {
    return (*__buffer).begin();
  }
  const byte *
  data() const
  {
    return (*__buffer).cbegin();
  }
  auto
  size() const
  {
    return __size;
  }
  auto
  max_size() const
  {
    return __sz;
  }
  auto
  chunk_size() const
  {
    return __chnk;
  }
  void
  clear()
  {
    __size = 0;
    micron::czero<__sz>(__buffer->begin());
  }
};

};

};
