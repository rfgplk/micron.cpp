//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory_block.hpp"
#include "../sum.hpp"
#include "__serial_core.hpp"
#include "file.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io-facing serialize surface

namespace micron
{
namespace io
{
namespace serialize
{

// frame c and write it at the file's current offset; one allocation, one write
template<typename C>
max_t
frame(io::file &f, const C &c)
{
  max_t need = framed_size(c);
  if ( need < 0 ) [[unlikely]]
    return need;
  micron::buffer out(static_cast<usize>(need));
  max_t n = frame_into(out.data(), out.size(), c);
  if ( n < 0 ) [[unlikely]]
    return n;
  return f.write(static_cast<const void *>(out.data()), static_cast<usize>(n));
}

// read the remainder of the file from the current offset and reconstruct out
template<typename C>
max_t
unframe(io::file &f, C &out)
{
  const posix::off64_t at = f.tell();
  if ( at < 0 ) [[unlikely]]
    return at;
  f.refresh_stat();
  const usize total = static_cast<usize>(f.size());
  if ( total <= static_cast<usize>(at) ) [[unlikely]]
    return -error::invalid_arg;
  const usize len = total - static_cast<usize>(at);
  micron::buffer in(len);
  max_t got = f.read(static_cast<void *>(in.data()), len);
  if ( got < 0 ) [[unlikely]]
    return got;
  return unframe_from(in.data(), static_cast<usize>(got), out);
}

template<typename C>
micron::option<C, io::error_t>
unframe(io::file &f)
{
  C out{};
  max_t r = unframe(f, out);
  if ( r < 0 ) [[unlikely]]
    return micron::option<C, io::error_t>{ io::error_t(static_cast<i32>(r)) };
  return micron::option<C, io::error_t>{ micron::move(out) };
}

// raw dumps
template<usize Chunk = 65536>
max_t
serialize_bytes(io::file &f, const byte *b, usize n)
{
  return f.write(static_cast<const void *>(b), n);
}

template<usize Chunk = 65536, typename R>
  requires(!micron::is_same_v<R, byte>)
max_t
serialize_bytes(io::file &f, const R *b, usize n)
{
  return f.write(static_cast<const void *>(b), n);
}

// raw sizeof dump
template<typename T>
  requires micron::is_trivially_copyable_v<T>
max_t
serialize(io::file &f, const T &obj)
{
  return f.write(static_cast<const void *>(&obj), sizeof(T));
}

template<typename T>
  requires micron::is_trivially_copyable_v<T>
max_t
deserialize(io::file &f, T &out)
{
  static_assert(__impl::mfr_pod_safe<T>,
                "micron::io::serialize::deserialize: refusing to reconstruct this type from possibly-untrusted bytes "
                "(a pointer/bool/enum/opaque-POD field can form an invalid value). Add `using mfr_pod_safe_tag = void;` "
                "to the type if EVERY bit pattern is a valid value.");
  return f.read(static_cast<void *>(&out), sizeof(T));
}

template<typename T>
  requires(!micron::is_null_pointer_v<T>)
max_t
serialize_page(io::file &f, const T &obj)
{
  const byte *b
      = reinterpret_cast<const byte *>(reinterpret_cast<uintptr_t>(&obj) & ~(static_cast<uintptr_t>(__micron_page_size_default) - 1));
  return f.write(static_cast<const void *>(b), __micron_page_size_default);
}

};      // namespace serialize
};      // namespace io
};      // namespace micron
