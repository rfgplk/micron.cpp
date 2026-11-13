//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../memory/cmemory/memcpy.hpp"
#include "../types.hpp"

// TODO: expand

namespace micron::io::serialize::canonical
{

struct byte_view {
  const byte *data = nullptr;
  usize size = 0;
};

// All scalar encodings are little endian, including on a big-endian host
class writer
{
  byte *__data;
  usize __size;
  usize __position = 0;

public:
  constexpr writer(byte *data, usize size) noexcept : __data(data), __size(data || !size ? size : 0) { }

  constexpr usize
  position() const noexcept
  {
    return __position;
  }

  constexpr usize
  remaining() const noexcept
  {
    return __size - __position;
  }

  constexpr bool
  put_u8(u8 value) noexcept
  {
    if ( !remaining() ) return false;
    __data[__position++] = value;
    return true;
  }

  template<typename T>
    requires(__is_same(T, u16) || __is_same(T, u32) || __is_same(T, u64))
  constexpr bool
  put_unsigned(T value) noexcept
  {
    if ( sizeof(T) > remaining() ) return false;
    for ( usize i = 0; i < sizeof(T); ++i ) __data[__position + i] = static_cast<byte>(value >> (i * 8));
    __position += sizeof(T);
    return true;
  }

  constexpr bool
  put_u16(u16 value) noexcept
  {
    return put_unsigned(value);
  }

  constexpr bool
  put_u32(u32 value) noexcept
  {
    return put_unsigned(value);
  }

  constexpr bool
  put_u64(u64 value) noexcept
  {
    return put_unsigned(value);
  }

  constexpr bool
  put_i64(i64 value) noexcept
  {
    return put_u64(static_cast<u64>(value));
  }

  constexpr bool
  put_f64(f64 value) noexcept
  {
    return put_u64(__builtin_bit_cast(u64, value));
  }

  constexpr bool
  put_bytes(const byte *source, usize count) noexcept
  {
    if ( count > remaining() || (count && !source) ) return false;
    if ( count ) {
      if consteval {
        for ( usize i = 0; i < count; ++i ) __data[__position + i] = source[i];
      } else {
        micron::bytecpy(__data + __position, source, count);
      }
    }
    __position += count;
    return true;
  }
};

class reader
{
  const byte *__data;
  usize __size;
  usize __position = 0;

public:
  constexpr reader(const byte *data, usize size) noexcept : __data(data), __size(data || !size ? size : 0) { }

  constexpr usize
  position() const noexcept
  {
    return __position;
  }

  constexpr usize
  remaining() const noexcept
  {
    return __size - __position;
  }

  constexpr bool
  get_u8(u8 &value) noexcept
  {
    if ( !remaining() ) return false;
    value = __data[__position++];
    return true;
  }

  template<typename T>
    requires(__is_same(T, u16) || __is_same(T, u32) || __is_same(T, u64))
  constexpr bool
  get_unsigned(T &value) noexcept
  {
    if ( sizeof(T) > remaining() ) return false;
    T result = 0;
    for ( usize i = 0; i < sizeof(T); ++i ) result |= static_cast<T>(__data[__position + i]) << (i * 8);
    __position += sizeof(T);
    value = result;
    return true;
  }

  constexpr bool
  get_u16(u16 &value) noexcept
  {
    return get_unsigned(value);
  }

  constexpr bool
  get_u32(u32 &value) noexcept
  {
    return get_unsigned(value);
  }

  constexpr bool
  get_u64(u64 &value) noexcept
  {
    return get_unsigned(value);
  }

  constexpr bool
  get_i64(i64 &value) noexcept
  {
    u64 bits;
    if ( !get_u64(bits) ) return false;
    value = __builtin_bit_cast(i64, bits);
    return true;
  }

  constexpr bool
  get_f64(f64 &value) noexcept
  {
    u64 bits;
    if ( !get_u64(bits) ) return false;
    value = __builtin_bit_cast(f64, bits);
    return true;
  }

  constexpr bool
  get_bytes(usize count, byte_view &value) noexcept
  {
    if ( count > remaining() ) return false;
    value = { __data ? __data + __position : nullptr, count };
    __position += count;
    return true;
  }
};

inline constexpr usize byte_frame_overhead = 16;

constexpr max_t
byte_frame_size(usize count) noexcept
{
  constexpr usize max_count = (sizeof(usize) < sizeof(max_t) ? ~usize{ 0 } : (~usize{ 0 } >> 1)) - byte_frame_overhead;
  if ( count > max_count ) return -error::invalid_arg;
  return static_cast<max_t>(count + byte_frame_overhead);
}

// Canonical MFR1 byte sequence: original 8-byte header, LE u64 count, bytes
constexpr max_t
frame_bytes(byte *destination, usize capacity, const byte *source, usize count) noexcept
{
  const max_t needed = byte_frame_size(count);
  if ( needed < 0 || !destination || static_cast<usize>(needed) > capacity || (count && !source) ) return -error::invalid_arg;
  writer out(destination, capacity);
  const byte magic[8] = { 'M', 'F', 'R', '1', 1, 0, 0, 0 };
  out.put_bytes(magic, sizeof(magic));
  out.put_u64(static_cast<u64>(count));
  out.put_bytes(source, count);
  return needed;
}

constexpr max_t
unframe_bytes(const byte *source, usize count, byte_view &value, u64 maximum_bytes = ~u64{ 0 }) noexcept
{
  if ( !source || count < byte_frame_overhead ) return -error::invalid_arg;
  const byte magic[8] = { 'M', 'F', 'R', '1', 1, 0, 0, 0 };
  for ( usize i = 0; i < sizeof(magic); ++i )
    if ( source[i] != magic[i] ) return -error::invalid_arg;
  reader in(source + 8, count - 8);
  u64 bytes = 0;
  in.get_u64(bytes);
  if ( bytes > maximum_bytes || bytes > static_cast<u64>(count - byte_frame_overhead) ) return -error::invalid_arg;
  const max_t consumed = byte_frame_size(static_cast<usize>(bytes));
  if ( consumed < 0 ) return consumed;
  value = { source + byte_frame_overhead, static_cast<usize>(bytes) };
  return consumed;
}

};      // namespace micron::io::serialize::canonical
