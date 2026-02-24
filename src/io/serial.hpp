//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace io
{
// serializer
// might release as a standalone lib, due to ATROCIOUS performance of existing serializers
// NOTE: currently only works with contiguous memory
namespace serialize
{
// serialize a byte array, of size n
template <size_t C = 65536, is_string T = micron::ustr8>
void
serialize_bytes(fsys::file<T> &f, const byte *b, size_t n)
{
  size_t pb = n < C ? n : C;
  size_t p = 0;
  if ( f.buffer_size() < C )
    pb = f.buffer_size();
  do {
    if ( pb > n )     // more than leftover
      pb = n;
    f.load_buffer(b + p, pb);
    f.write_bytes(pb);
    n -= pb;
    p += pb;
  } while ( n );
}

template <size_t C = 65536, is_string T = micron::ustr8, typename R>
void
serialize_bytes(fsys::file<T> &f, const R *_b, size_t n)
{
  const byte *b = reinterpret_cast<const byte *>(_b);     // overloading like this so we don't have to check for return type of operator&
  size_t pb = n < C ? n : C;
  size_t p = 0;
  if ( f.buffer_size() < C )
    pb = f.buffer_size();
  do {
    if ( pb > n )     // more than leftover
      pb = n;
    f.load_buffer(b + p, pb);
    f.write_bytes(pb);
    n -= pb;
    p += pb;
  } while ( n );
}

template <typename T, is_string R>
  requires micron::is_class_v<T>
void
serialize(fsys::file<R> &f, const T &obj)
{
  serialize_bytes(f, &obj, sizeof(obj));
}

template <typename T, is_string R>
  requires(!micron::is_null_pointer_v<T>)
void
serialize_page(fsys::file<R> &f, const T &obj)
{
  byte *b = reinterpret_cast<byte *>(reinterpret_cast<uintptr_t>(&obj) & ~(4096 - 1));
  serialize_bytes(f, b, 4096);
}

template <typename T>
  requires micron::is_object_v<T>
void
deserialize(const T &strct)
{
}

void write();
void read();
};     // namespace serialize

};     // namespace io
};     // namespace micron
