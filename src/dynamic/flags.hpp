//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/cstring.hpp"
#include "../types.hpp"

namespace micron
{

// dlopen style constants
enum class rtld : u32 {
  none = 0x00000,
  lazy = 0x00001,
  now = 0x00002,
  noload = 0x00004,        // "is it already loaded?"
  deepbind = 0x00008,      // search this module before the global scope
  local = 0x00000,         // the default: the module joins no global scope
  global = 0x00100,        // its symbols become available to every later resolution
  nodelete = 0x01000,      // never unmap, even at refcount zero
};

constexpr rtld
operator|(rtld a, rtld b) noexcept
{
  return static_cast<rtld>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr rtld
operator&(rtld a, rtld b) noexcept
{
  return static_cast<rtld>(static_cast<u32>(a) & static_cast<u32>(b));
}

constexpr rtld &
operator|=(rtld &a, rtld b) noexcept
{
  a = a | b;
  return a;
}

constexpr bool
has(rtld set, rtld bit) noexcept
{
  return (static_cast<u32>(set) & static_cast<u32>(bit)) != 0;
}

namespace elf
{
namespace dl
{

inline constexpr usize error_max = 192;

inline thread_local char __err_buf[error_max] = {};
inline thread_local bool __err_set = false;

inline void
__err_clear() noexcept
{
  __err_set = false;
  __err_buf[0] = 0;
}

inline void
__err_set_once(const char *what, const char *detail = nullptr) noexcept
{
  if ( __err_set ) return;
  usize n = 0;
  for ( ; what && what[n] && n + 1 < error_max; ++n ) __err_buf[n] = what[n];
  if ( detail && n + 3 < error_max ) {
    __err_buf[n++] = ':';
    __err_buf[n++] = ' ';
    for ( usize i = 0; detail[i] && n + 1 < error_max; ++i ) __err_buf[n++] = detail[i];
  }
  __err_buf[n] = 0;
  __err_set = true;
}

};      // namespace dl
};      // namespace elf

inline const char *
dynamic_error() noexcept
{
  if ( !elf::dl::__err_set ) return nullptr;
  elf::dl::__err_set = false;
  return elf::dl::__err_buf;
}

};      // namespace micron
