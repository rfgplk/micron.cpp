//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// DWARF LEB128 readers and DW_EH_PE pointer-encoding decoder

namespace micron::eh
{

// DW_EH_PE value formats (low nibble) and applications (high nibble)
enum : u8 {
  DW_EH_PE_absptr = 0x00,
  DW_EH_PE_uleb128 = 0x01,
  DW_EH_PE_udata2 = 0x02,
  DW_EH_PE_udata4 = 0x03,
  DW_EH_PE_udata8 = 0x04,
  DW_EH_PE_sleb128 = 0x09,
  DW_EH_PE_sdata2 = 0x0a,
  DW_EH_PE_sdata4 = 0x0b,
  DW_EH_PE_sdata8 = 0x0c,

  DW_EH_PE_pcrel = 0x10,
  DW_EH_PE_textrel = 0x20,
  DW_EH_PE_datarel = 0x30,
  DW_EH_PE_funcrel = 0x40,
  DW_EH_PE_aligned = 0x50,

  DW_EH_PE_indirect = 0x80,
  DW_EH_PE_omit = 0xff
};

struct decode_ctx {
  usize pcrel_base;      // address of the encoded field (set per-read by reader)
  usize text_base;       // _Unwind_GetTextRelBase
  usize data_base;       // _Unwind_GetDataRelBase (or .eh_frame_hdr base for the search table)
  usize func_base;       // current function start (funcrel)
};

inline u64
read_uleb128(const byte *&p) noexcept
{
  u64 result = 0;
  unsigned shift = 0;
  byte b;
  do {
    b = *p++;
    result |= static_cast<u64>(b & 0x7f) << shift;
    shift += 7;
  } while ( b & 0x80 );
  return result;
}

inline i64
read_sleb128(const byte *&p) noexcept
{
  i64 result = 0;
  unsigned shift = 0;
  byte b;
  do {
    b = *p++;
    result |= static_cast<i64>(b & 0x7f) << shift;
    shift += 7;
  } while ( b & 0x80 );
  if ( shift < 64 && (b & 0x40) ) result |= -(static_cast<i64>(1) << shift);      // sign-extend
  return result;
}

template<typename T>
inline T
read_unaligned(const byte *p) noexcept
{
  T v;
  __builtin_memcpy(&v, p, sizeof(T));
  return v;
}

inline usize
read_encoded(u8 enc, const byte *&p, const decode_ctx &ctx) noexcept
{
  if ( enc == DW_EH_PE_omit ) return 0;

  const byte *const field = p;
  usize result = 0;

  if ( (enc & 0x70) == DW_EH_PE_aligned ) {
    usize a = reinterpret_cast<usize>(p);
    a = (a + sizeof(void *) - 1) & ~(static_cast<usize>(sizeof(void *) - 1));
    p = reinterpret_cast<const byte *>(a);
    result = read_unaligned<usize>(p);
    p += sizeof(void *);
    return result;
  }

  switch ( enc & 0x0f ) {
  case DW_EH_PE_absptr:
    result = read_unaligned<usize>(p);
    p += sizeof(usize);
    break;
  case DW_EH_PE_uleb128:
    result = static_cast<usize>(read_uleb128(p));
    break;
  case DW_EH_PE_udata2:
    result = read_unaligned<u16>(p);
    p += 2;
    break;
  case DW_EH_PE_udata4:
    result = read_unaligned<u32>(p);
    p += 4;
    break;
  case DW_EH_PE_udata8:
    result = static_cast<usize>(read_unaligned<u64>(p));
    p += 8;
    break;
  case DW_EH_PE_sleb128:
    result = static_cast<usize>(read_sleb128(p));
    break;
  case DW_EH_PE_sdata2:
    result = static_cast<usize>(static_cast<intptr_t>(read_unaligned<i16>(p)));
    p += 2;
    break;
  case DW_EH_PE_sdata4:
    result = static_cast<usize>(static_cast<intptr_t>(read_unaligned<i32>(p)));
    p += 4;
    break;
  case DW_EH_PE_sdata8:
    result = static_cast<usize>(read_unaligned<i64>(p));
    p += 8;
    break;
  default:
    return 0;
  }

  if ( result != 0 || (enc & 0x0f) == DW_EH_PE_absptr ) {
    switch ( enc & 0x70 ) {
    case DW_EH_PE_pcrel:
      result += reinterpret_cast<usize>(field);
      break;
    case DW_EH_PE_textrel:
      result += ctx.text_base;
      break;
    case DW_EH_PE_datarel:
      result += ctx.data_base;
      break;
    case DW_EH_PE_funcrel:
      result += ctx.func_base;
      break;
    default:
      break;      // absolute
    }
    if ( enc & DW_EH_PE_indirect ) result = read_unaligned<usize>(reinterpret_cast<const byte *>(result));
  }

  return result;
}

inline usize
encoded_size(u8 enc) noexcept
{
  switch ( enc & 0x0f ) {
  case DW_EH_PE_udata2:
  case DW_EH_PE_sdata2:
    return 2;
  case DW_EH_PE_udata4:
  case DW_EH_PE_sdata4:
    return 4;
  case DW_EH_PE_udata8:
  case DW_EH_PE_sdata8:
    return 8;
  case DW_EH_PE_absptr:
    return sizeof(void *);
  default:
    return 0;
  }
}

};      // namespace micron::eh

#endif      // __micron_eh
