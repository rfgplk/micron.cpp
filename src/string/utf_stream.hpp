//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron::unicode
{

enum class encoding : u8 { automatic, utf8, utf16_le, utf16_be };
enum class decode_status : u8 { scalar, need_input, done, invalid, truncated, unsupported_encoding };

struct decode_result {
  decode_status status = decode_status::need_input;
  usize consumed = 0;
  u32 scalar = 0;
  u64 byte_offset = 0;
};

constexpr bool
is_scalar(u32 value) noexcept
{
  return value <= 0x10ffff && !(value >= 0xd800 && value <= 0xdfff);
}

constexpr u32
encode_utf8(u32 value, char *output) noexcept
{
  if ( !is_scalar(value) ) return 0;
  if ( value < 0x80 ) {
    output[0] = static_cast<char>(value);
    return 1;
  }
  if ( value < 0x800 ) {
    output[0] = static_cast<char>(0xc0 | (value >> 6));
    output[1] = static_cast<char>(0x80 | (value & 63));
    return 2;
  }
  if ( value < 0x10000 ) {
    output[0] = static_cast<char>(0xe0 | (value >> 12));
    output[1] = static_cast<char>(0x80 | ((value >> 6) & 63));
    output[2] = static_cast<char>(0x80 | (value & 63));
    return 3;
  }
  output[0] = static_cast<char>(0xf0 | (value >> 18));
  output[1] = static_cast<char>(0x80 | ((value >> 12) & 63));
  output[2] = static_cast<char>(0x80 | ((value >> 6) & 63));
  output[3] = static_cast<char>(0x80 | (value & 63));
  return 4;
}

template<typename Character>
constexpr bool
decode_utf8(const Character *input, usize size, usize &position, u32 &scalar) noexcept
{
  static_assert(sizeof(Character) == 1);
  if ( position >= size ) return false;
  const auto head = static_cast<u8>(input[position++]);
  if ( head < 0x80 ) {
    scalar = head;
    return true;
  }
  u32 count = 0, minimum = 0;
  if ( head >= 0xc2 && head <= 0xdf ) {
    scalar = head & 31;
    count = 1;
    minimum = 0x80;
  } else if ( head >= 0xe0 && head <= 0xef ) {
    scalar = head & 15;
    count = 2;
    minimum = 0x800;
  } else if ( head >= 0xf0 && head <= 0xf4 ) {
    scalar = head & 7;
    count = 3;
    minimum = 0x10000;
  } else
    return false;
  if ( count > size - position ) return false;
  while ( count-- ) {
    const auto byte = static_cast<u8>(input[position++]);
    if ( (byte & 0xc0) != 0x80 ) return false;
    scalar = (scalar << 6) | (byte & 63);
  }
  return scalar >= minimum && is_scalar(scalar);
}

template<typename Character>
constexpr bool
validate_utf8(const Character *input, usize size) noexcept
{
  usize position = 0;
  while ( position < size ) {
    u32 scalar = 0;
    if ( !decode_utf8(input, size, position, scalar) ) return false;
  }
  return true;
}

// Caller retains only unconsumed bytes. The decoder owns at most four probe bytes
// and one partial scalar; scalar results can consume zero new bytes.
class stream_decoder
{
  encoding __selected = encoding::automatic;
  encoding __requested = encoding::automatic;
  u8 __probe[4]{};
  u8 __probe_size = 0;
  u8 __probe_read = 0;
  bool __detected = false;
  bool __bom = false;
  bool __done = false;
  decode_status __failure = decode_status::need_input;
  u64 __offset = 0;
  u64 __scalar_offset = 0;
  u32 __value = 0;
  u32 __minimum = 0;
  u16 __high = 0;
  u8 __remaining = 0;
  u8 __unit_byte = 0;
  bool __half_unit = false;

  constexpr void
  detect() noexcept
  {
    __detected = true;
    __selected = __requested == encoding::automatic ? encoding::utf8 : __requested;
    if ( __requested == encoding::automatic && __probe_size >= 4
         && ((__probe[0] == 0 && __probe[1] == 0 && __probe[2] == 0xfe && __probe[3] == 0xff)
             || (__probe[0] == 0xff && __probe[1] == 0xfe && __probe[2] == 0 && __probe[3] == 0)
             || (__probe[0] == 0 && __probe[1] == 0 && __probe[2] == 0 && __probe[3] != 0)
             || (__probe[0] != 0 && __probe[1] == 0 && __probe[2] == 0 && __probe[3] == 0)) ) {
      __failure = decode_status::unsupported_encoding;
      return;
    }
    if ( __probe_size >= 3 && __probe[0] == 0xef && __probe[1] == 0xbb && __probe[2] == 0xbf ) {
      if ( __requested != encoding::automatic && __requested != encoding::utf8 ) {
        __failure = decode_status::invalid;
        return;
      }
      __selected = encoding::utf8;
      __bom = true;
      __probe_read = 3;
    } else if ( __probe_size >= 2 && ((__probe[0] == 0xff && __probe[1] == 0xfe) || (__probe[0] == 0xfe && __probe[1] == 0xff)) ) {
      const auto bom = __probe[0] == 0xff ? encoding::utf16_le : encoding::utf16_be;
      if ( __requested != encoding::automatic && __requested != bom ) {
        __failure = decode_status::invalid;
        return;
      }
      __selected = bom;
      __bom = true;
      __probe_read = 2;
    } else if ( __requested == encoding::automatic && __probe_size >= 4 ) {
      // XML's signature also permits an encoding declaration before a BOM-less UTF-16 document.
      if ( __probe[0] == 0 && __probe[1] == '<' && __probe[2] == 0 && __probe[3] == '?' )
        __selected = encoding::utf16_be;
      else if ( __probe[0] == '<' && __probe[1] == 0 && __probe[2] == '?' && __probe[3] == 0 )
        __selected = encoding::utf16_le;
    }
  }

public:
  constexpr explicit stream_decoder(encoding source = encoding::automatic) noexcept : __requested(source) { }

  constexpr bool
  had_bom() const noexcept
  {
    return __bom;
  }

  constexpr encoding
  source_encoding() const noexcept
  {
    return __selected;
  }

  constexpr u64
  byte_offset() const noexcept
  {
    return __offset;
  }

  constexpr bool
  at_scalar_boundary() const noexcept
  {
    return __detected && __probe_read == __probe_size && __remaining == 0 && !__half_unit && __high == 0 && !__done;
  }

  // Advance a separately validated ASCII run (UTF-8 only), for bounded SIMD readers.
  constexpr bool
  advance_ascii(usize bytes) noexcept
  {
    if ( __selected != encoding::utf8 || !at_scalar_boundary() || static_cast<u64>(bytes) > ~u64(0) - __offset ) return false;
    __offset += bytes;
    return true;
  }

  template<typename Character>
    requires(sizeof(Character) == 1)
  constexpr decode_result
  next(const Character *input, usize size, bool final = false) noexcept
  {
    usize consumed = 0;
    if ( __failure != decode_status::need_input ) return { __failure, 0, 0, __scalar_offset };
    if ( __done ) {
      if ( size == 0 ) return { decode_status::done, 0, 0, __offset };
      __failure = decode_status::invalid;
      __scalar_offset = __offset;
      return { __failure, 0, 0, __offset };
    }
    if ( !__detected ) {
      while ( __probe_size < 4 && consumed < size ) {
        __probe[__probe_size++] = static_cast<u8>(input[consumed++]);
        ++__offset;
      }
      if ( __probe_size < 4 && !final ) return { decode_status::need_input, consumed, 0, __offset };
      detect();
      if ( __failure != decode_status::need_input ) return { __failure, consumed, 0, 0 };
    }
    for ( ;; ) {
      u8 byte = 0;
      u64 position = 0;
      if ( __probe_read < __probe_size ) {
        position = __probe_read;
        byte = __probe[__probe_read++];
      } else if ( consumed < size ) {
        position = __offset++;
        byte = static_cast<u8>(input[consumed++]);
      } else {
        if ( !final ) return { decode_status::need_input, consumed, 0, __offset };
        if ( __remaining || __half_unit || __high ) {
          __failure = decode_status::truncated;
          return { __failure, consumed, 0, __scalar_offset };
        }
        __done = true;
        return { decode_status::done, consumed, 0, __offset };
      }
      if ( __selected == encoding::utf8 ) {
        if ( __remaining == 0 ) {
          __scalar_offset = position;
          if ( byte < 0x80 ) return { decode_status::scalar, consumed, byte, __scalar_offset };
          if ( byte >= 0xc2 && byte <= 0xdf ) {
            __remaining = 1;
            __value = byte & 31;
            __minimum = 0x80;
          } else if ( byte >= 0xe0 && byte <= 0xef ) {
            __remaining = 2;
            __value = byte & 15;
            __minimum = 0x800;
          } else if ( byte >= 0xf0 && byte <= 0xf4 ) {
            __remaining = 3;
            __value = byte & 7;
            __minimum = 0x10000;
          } else {
            __failure = decode_status::invalid;
            return { __failure, consumed, 0, __scalar_offset };
          }
        } else {
          if ( (byte & 0xc0) != 0x80 ) {
            __failure = decode_status::invalid;
            return { __failure, consumed, 0, __scalar_offset };
          }
          __value = (__value << 6) | (byte & 63);
          if ( --__remaining == 0 ) {
            if ( __value < __minimum || !is_scalar(__value) ) {
              __failure = decode_status::invalid;
              return { __failure, consumed, 0, __scalar_offset };
            }
            return { decode_status::scalar, consumed, __value, __scalar_offset };
          }
        }
      } else {
        if ( !__half_unit ) {
          if ( !__high ) __scalar_offset = position;
          __unit_byte = byte;
          __half_unit = true;
          continue;
        }
        __half_unit = false;
        const u16 unit = __selected == encoding::utf16_le ? static_cast<u16>(__unit_byte | (u16(byte) << 8))
                                                          : static_cast<u16>((u16(__unit_byte) << 8) | byte);
        if ( __high ) {
          if ( unit < 0xdc00 || unit > 0xdfff ) {
            __failure = decode_status::invalid;
            return { __failure, consumed, 0, __scalar_offset };
          }
          const u32 value = 0x10000 + ((u32(__high) - 0xd800) << 10) + (u32(unit) - 0xdc00);
          __high = 0;
          return { decode_status::scalar, consumed, value, __scalar_offset };
        }
        if ( unit >= 0xd800 && unit <= 0xdbff )
          __high = unit;
        else if ( unit >= 0xdc00 && unit <= 0xdfff ) {
          __failure = decode_status::invalid;
          return { __failure, consumed, 0, __scalar_offset };
        } else
          return { decode_status::scalar, consumed, unit, __scalar_offset };
      }
    }
  }
};

}      // namespace micron::unicode
