// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "config.hpp"
#include "printing.hpp"

#include "../../../types.hpp"

namespace bb
{

constexpr static const int __critical_exit = 11;

[[gnu::always_inline]] inline bool
check_mul_overflow(usize a, usize b, usize &out) noexcept
{
  return __builtin_mul_overflow(a, b, &out);
}

[[gnu::always_inline]] inline bool
check_add_overflow(usize a, usize b, usize &out) noexcept
{
  return __builtin_add_overflow(a, b, &out);
}

[[gnu::always_inline]] inline bool
check_ptr_valid(const void *p) noexcept
{
  return p != nullptr && p != reinterpret_cast<const void *>(static_cast<uintptr_t>(-1));
}

[[gnu::always_inline]] inline void
zero_on_alloc(byte *p, usize n) noexcept
{
  if constexpr( __default_zero_on_alloc )
    __builtin_memset(p, 0, n);
  else {
    (void)p;
    (void)n;
  }
}

[[gnu::always_inline]] inline void
scrub_on_free(byte *p, usize n) noexcept
{
  if constexpr( __default_zero_on_free )
    __builtin_memset(p, 0, n);
  else if constexpr( __default_poison_on_free )
    __builtin_memset(p, __default_poison_byte, n);
  else {
    (void)p;
    (void)n;
  }
}

constexpr static const usize __rz = __default_redzone ? __default_redzone_size : 0;

[[gnu::always_inline]] inline void
write_redzone(byte *p, usize user) noexcept
{
  if constexpr( __default_redzone )
    __builtin_memset(p + user, __default_redzone_byte, __rz);
  else {
    (void)p;
    (void)user;
  }
}

[[gnu::always_inline]] inline bool
verify_redzone(const byte *p, usize user) noexcept
{
  if constexpr( __default_redzone ) {
    for( usize i = 0; i < __rz; ++i )
      if( p[user + i] != __default_redzone_byte )
        return false;
    return true;
  } else {
    (void)p;
    (void)user;
    return true;
  }
}

__bb_cold bool
handle_bad_free(const void *p) noexcept
{
  if constexpr( __default_double_free_action == 0 ) {
    (void)p;
    return false;
  } else if constexpr( __default_double_free_action == 1 ) {
    __bits::__report("bad or double free", p);
    return false;
  } else {
    __bits::__report("bad or double free", p);
    __bits::__halt(__critical_exit);
  }
}

__bb_cold bool
handle_redzone_trip(const void *p) noexcept
{
  if constexpr( __default_redzone_action == 0 ) {
    (void)p;
    return false;
  } else if constexpr( __default_redzone_action == 1 ) {
    __bits::__report("redzone overrun", p);
    return false;
  } else {
    __bits::__report("redzone overrun", p);
    __bits::__halt(__critical_exit);
  }
}

};
