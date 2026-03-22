//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../memory/cmemory.hpp"
#include "../../../simd/types.hpp"
#include "../../../types.hpp"
#include "config.hpp"
#include "printing.hpp"

namespace abc
{

__attribute__((noreturn)) void
abort_state(void)
{
  __debug_print("abort_state(): fatal allocator error, aborting", 0);
  micron::abort();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"

bool
fail_state(void)
{
  if constexpr ( __default_fail_result == 0 ) {
    // hard abort, no diagnostics
    abort_state();
  } else if constexpr ( __default_fail_result == 1 ) {
    // diagnostic print then abort
    __debug_print("fail_state(): allocator integrity violation (mode=1), aborting", 0);
    abort_state();
  } else if constexpr ( __default_fail_result == 2 ) {
    // silent failure, return false
    __debug_print("fail_state(): allocator integrity violation (mode=2), returning false", 0);
    return false;
  }
  return true;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"

inline __attribute__((always_inline)) bool
handle_double_free([[maybe_unused]] byte *addr)
{
  if constexpr ( __default_double_free_action == 0 ) {
    // ignore silently
    return false;
  } else if constexpr ( __default_double_free_action == 1 ) {
    __debug_print_addr("handle_double_free(): double free / unrecognised block at: ", addr);
    return false;
  } else if constexpr ( __default_double_free_action == 2 ) {
    __debug_print_addr("handle_double_free(): double free / unrecognised block, aborting at: ", addr);
    abort_state();
  }
  return false;
}

inline __attribute__((always_inline)) bool
handle_double_free(const byte *addr)
{
  return handle_double_free(const_cast<byte *>(addr));
}

#pragma GCC diagnostic pop

inline __attribute__((always_inline)) auto
check_constraint(const usize sz) -> bool
{
  if constexpr ( __is_constrained ) {
    if ( sz > __alloc_limit and __alloc_limit != 0 ) {
      __debug_print("check_constraint(): allocation exceeds limit, requested: ", sz);
      __debug_print("check_constraint(): configured limit: ", __alloc_limit);
      return true;
    }
  }
  // catch obvious corruption
  if ( sz > (micron::numeric_limits<usize>::max() >> 1) ) [[unlikely]] {
    __debug_print("check_constraint(): allocation size exceeds half address space, likely overflow: ", sz);
    return true;
  }
  return false;
}

inline __attribute__((always_inline)) bool
check_mul_overflow(usize a, usize b, usize &result)
{
  if ( __builtin_mul_overflow(a, b, &result) ) [[unlikely]] {
    __debug_print("check_mul_overflow(): overflow detected, a: ", a);
    __debug_print("check_mul_overflow(): overflow detected, b: ", b);
    return true;
  }
  return false;
}

inline __attribute__((always_inline)) bool
check_add_overflow(usize a, usize b, usize &result)
{
  if ( __builtin_add_overflow(a, b, &result) ) [[unlikely]] {
    __debug_print("check_add_overflow(): overflow detected, a: ", a);
    __debug_print("check_add_overflow(): overflow detected, b: ", b);
    return true;
  }
  return false;
}

inline __attribute__((always_inline)) bool
check_alignment(const byte *addr)
{
  if constexpr ( __default_check_alignment ) {
    constexpr usize align = alignof(void *);
    if ( reinterpret_cast<uintptr_t>(addr) & (align - 1) ) [[unlikely]] {
      __debug_print_addr("check_alignment(): misaligned pointer: ", addr);
      return false;
    }
  }
  return true;
}

inline __attribute__((always_inline)) bool
check_alignment(const addr_t *addr)
{
  return check_alignment(reinterpret_cast<const byte *>(addr));
}

// TLSF layout: [tlsf_hdr | user_data]
// ...user_ptr = block + __hdr_offset
// ...bsize = *(u32*)(user_ptr - __hdr_offset)

// buddy layout: [user_data ... | block_header]
// ...user_ptr = block_start
// ...header is at block_start + order_size - __hdr_offset (at TAIL)

inline __attribute__((always_inline)) usize
__recover_size_from_hdr(byte *addr)
{
  if constexpr ( __default_unsafe_size_recovery ) {
    u32 bsize = *reinterpret_cast<u32 *>(addr - __hdr_offset);
    if ( bsize < __hdr_offset or bsize > (1ULL << 31) ) [[unlikely]] {
      __debug_print("__recover_size_from_hdr(): suspicious bsize recovered: ", (usize)bsize);
      return 0;
    }
    return static_cast<usize>(bsize) - __hdr_offset;
  }
  (void)addr;
  return 0;
}

inline __attribute__((always_inline)) void
sanitize_on_alloc([[maybe_unused]] byte *addr, [[maybe_unused]] usize sz = 0)
{
  if constexpr ( __default_sanitize ) {
    if ( sz == 0 ) {
      sz = __recover_size_from_hdr(addr);
      if ( sz == 0 )
        return;
    }
    micron::memset(addr, __default_sanitize_with_on_alloc, sz);
  } else {
    // nothing
  }
}

inline __attribute__((always_inline)) void
zero_on_alloc([[maybe_unused]] byte *addr, [[maybe_unused]] usize sz = 0)
{
  if constexpr ( __default_zero_on_alloc ) {
    if ( sz == 0 ) {
      sz = __recover_size_from_hdr(addr);
      if ( sz == 0 )
        return;
    }
    micron::bzero(addr, sz);
  } else {
    // nothing
  }
}

inline __attribute__((always_inline)) void
poison_on_free([[maybe_unused]] byte *addr, [[maybe_unused]] usize sz = 0)
{
  if constexpr ( __default_poison_on_free ) {
    if ( sz == 0 ) {
      sz = __recover_size_from_hdr(addr);
      if ( sz == 0 )
        return;
    }
    micron::memset(addr, __default_poison_byte, sz);
  } else {
    // nothing
  }
}

inline __attribute__((always_inline)) void
zero_on_free([[maybe_unused]] byte *addr, [[maybe_unused]] usize sz = 0)
{
  if constexpr ( __default_zero_on_free ) {
    if ( sz == 0 ) {
      sz = __recover_size_from_hdr(addr);
      if ( sz == 0 )
        return;
    }
    micron::bzero(addr, sz);
  } else {
    // nothibng
  }
}

inline __attribute__((always_inline)) void
full_on_free([[maybe_unused]] byte *addr, [[maybe_unused]] usize sz = 0)
{
  if constexpr ( __default_full_on_free ) {
    if ( sz == 0 ) {
      sz = __recover_size_from_hdr(addr);
      if ( sz == 0 )
        return;
    }
    micron::memset(addr, 0xFF, sz);
  } else {
    // nothing
  }
}

// start canary: __default_redzone_size bytes starting at user_ptr - __default_redzone_size
// end canary: __default_redzone_size bytes starting at user_ptr + user_sz
inline __attribute__((always_inline)) void
write_redzone([[maybe_unused]] byte *user_ptr, [[maybe_unused]] usize user_sz)
{
  if constexpr ( __default_redzone ) {
    byte *lo = user_ptr - __default_redzone_size;
    micron::memset(lo, __default_redzone_byte, __default_redzone_size);
    byte *hi = user_ptr + user_sz;
    micron::memset(hi, __default_redzone_byte, __default_redzone_size);
  } else {
    // nothing
  }
}

inline __attribute__((always_inline)) bool
verify_redzone_leading([[maybe_unused]] const byte *user_ptr)
{
  if constexpr ( __default_redzone ) {
    const byte *lo = user_ptr - __default_redzone_size;
    for ( usize i = 0; i < __default_redzone_size; ++i ) {
      if ( lo[i] != __default_redzone_byte ) {
        __debug_print("verify_redzone_leading(): leading canary corrupted at byte: ", i);
        return false;
      }
    }
    return true;
  }
  // nothing
  return true;
}

inline __attribute__((always_inline)) bool
verify_redzone([[maybe_unused]] const byte *user_ptr, [[maybe_unused]] usize user_sz)
{
  if constexpr ( __default_redzone ) {
    if ( !verify_redzone_leading(user_ptr) )
      return false;
    const byte *hi = user_ptr + user_sz;
    for ( usize i = 0; i < __default_redzone_size; ++i ) {
      if ( hi[i] != __default_redzone_byte ) {
        __debug_print("verify_redzone(): trailing canary corrupted at byte: ", i);
        return false;
      }
    }
    return true;
  }
  return true;
}

inline __attribute__((always_inline)) bool
check_ptr_valid(const byte *ptr)
{
  if ( !ptr ) [[unlikely]]
    return false;
  if ( ptr == (const byte *)-1 ) [[unlikely]] {
    __debug_print("check_ptr_valid(): sentinel pointer (-1) passed to free path", 0);
    return false;
  }
  return true;
}

inline __attribute__((always_inline)) bool
check_ptr_valid(const addr_t *ptr)
{
  return check_ptr_valid(reinterpret_cast<const byte *>(ptr));
}

inline __attribute__((always_inline)) bool
check_chunk_valid(const byte *ptr, usize len)
{
  if ( !check_ptr_valid(ptr) )
    return false;
  if ( len == 0 ) [[unlikely]] {
    __debug_print("check_chunk_valid(): zero-length chunk", 0);
    return false;
  }
  if ( reinterpret_cast<uintptr_t>(ptr) + len < reinterpret_cast<uintptr_t>(ptr) ) [[unlikely]] {
    __debug_print("check_chunk_valid(): ptr + len wraps address space", 0);
    return false;
  }
  return true;
}

static_assert(!(__default_zero_on_alloc and __default_sanitize),
              "abcmalloc: __default_zero_on_alloc and __default_sanitize are mutually exclusive; "
              "zero fills with 0x00, sanitize fills with __default_sanitize_with_on_alloc; pick one");

};     // namespace abc
