//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../except.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"
#include "../kmemory.hpp"
#include "bb_alloc.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// namespace abc, backed by micron::bb

namespace abc
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// layout constants

constexpr static usize __hdr_offset = micron::bb::header_size;
inline constexpr usize native_block_alignment = micron::bb::native_alignment;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocation

inline byte *
alloc(usize __size) noexcept
{
  return micron::bb::alloc(__size);
}

inline byte *
salloc(usize __size) noexcept
{
  return micron::bb::alloc(__size);
}

inline micron::__chunk<byte>
balloc(usize __size) noexcept
{
  return micron::bb::balloc(__size);
}

inline micron::__chunk<byte>
fetch(usize __size) noexcept
{
  return micron::bb::zalloc(__size);
}

[[nodiscard]] inline micron::__chunk<byte>
aligned_balloc(usize __alignment, usize __size) noexcept
{
  return micron::bb::aligned_balloc(__alignment, __size);
}

inline void *
aligned_alloc(usize __alignment, usize __size) noexcept
{
  if ( __alignment == 0 || (__alignment & (__alignment - 1)) != 0 ) [[unlikely]]
    return nullptr;
  if ( __size == 0 || __size % __alignment != 0 ) [[unlikely]]
    return nullptr;
  return reinterpret_cast<void *>(micron::bb::aligned_balloc(__alignment, __size).ptr);
}

inline void *
malloc(usize __size) noexcept
{
  return reinterpret_cast<void *>(micron::bb::alloc(__size));
}

inline void *
calloc(usize __num, usize __size) noexcept
{
  const usize __total = __num * __size;
  if ( __num != 0 && __total / __num != __size ) return nullptr;      // overflow
  return reinterpret_cast<void *>(micron::bb::zalloc(__total).ptr);
}

// %%%%%%%%%%%%%%%%
// deallocation

inline void
dealloc(byte *__ptr) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
dealloc(byte *__ptr, usize) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
aligned_dealloc(byte *__ptr, usize) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
aligned_dealloc(micron::__chunk<byte> __memory, usize) noexcept
{
  micron::bb::dealloc(__memory.ptr);
}

inline void
aligned_free(void *__ptr) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
aligned_free(void *__ptr, usize) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
free(void *__ptr) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
retire(byte *__ptr) noexcept
{
  micron::bb::dealloc(__ptr);
}

inline void
relinquish(byte *__ptr) noexcept
{
  micron::bb::dealloc(__ptr);
}

// %%%%%%%%%%%%
// resize

[[nodiscard]] inline micron::__chunk<byte>
resize_chunk(micron::__chunk<byte> __old, usize __size, usize __preserve) noexcept
{
  return micron::bb::resize(__old, __size, __preserve, native_block_alignment);
}

[[nodiscard]] inline micron::__chunk<byte>
aligned_resize_chunk(micron::__chunk<byte> __old, usize __size, usize __preserve, usize __alignment) noexcept
{
  return micron::bb::resize(__old, __size, __preserve, __alignment);
}

inline void *
realloc(void *__ptr, usize __size) noexcept
{
  const usize __have = micron::bb::query_size(__ptr);
  auto __next = micron::bb::resize({ reinterpret_cast<byte *>(__ptr), __have }, __size, __have, native_block_alignment);
  return reinterpret_cast<void *>(__next.ptr);
}

// %%%%%%%%%%%%%%%%
// queries

inline bool
is_present(addr_t *__ptr) noexcept
{
  return micron::bb::is_present(__ptr);
}

inline bool
is_present(byte *__ptr) noexcept
{
  return micron::bb::is_present(__ptr);
}

inline bool
within(const addr_t *__ptr) noexcept
{
  return micron::bb::within(__ptr);
}

inline bool
within(addr_t *__ptr) noexcept
{
  return micron::bb::within(__ptr);
}

inline bool
within(byte *__ptr) noexcept
{
  return micron::bb::within(__ptr);
}

template<typename T>
inline usize
query_size(T *__ptr) noexcept
{
  return micron::bb::query_size(__ptr);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NEUTRAL STUBS

// NOOPS ON BAREBONES

inline void
freeze(byte *) noexcept
{
}

inline void
freeze_sheet(void *) noexcept
{
}

inline byte *
mark_at(byte *__ptr, usize) noexcept
{
  return __ptr;
}

inline byte *
unmark_at(byte *__ptr, usize) noexcept
{
  return __ptr;
}

inline byte *
launder(usize __size) noexcept
{
  return micron::bb::alloc(__size);
}

inline void
which(void) noexcept
{
}

inline void
borrow(void) noexcept
{
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// __abc_allocator

template<typename T>
  requires(micron::is_integral_v<T>)
struct __abc_allocator {
  static auto
  calloc(usize __n) -> micron::__chunk<byte>
  {
    auto __mem = abc::fetch(__n);
    if ( __mem.ptr == nullptr ) micron::exc<micron::except::critical_error>("bb __abc_allocator::calloc(): pool failed to satisfy request");
    return __mem;
  }

  static T *
  alloc(usize __n)
  {
    T *__ptr = reinterpret_cast<T *>(abc::alloc(__n));
    if ( __ptr == nullptr ) micron::exc<micron::except::critical_error>("bb __abc_allocator::alloc(): pool failed to satisfy request");
    return __ptr;
  }

  static auto
  allocate_aligned(usize __n, usize __alignment) -> micron::__chunk<byte>
  {
    auto __mem = abc::aligned_balloc(__alignment, __n);
    if ( __mem.ptr == nullptr )
      micron::exc<micron::except::critical_error>("bb __abc_allocator::allocate_aligned(): pool failed to satisfy request");
    return __mem;
  }

  static void
  dealloc(T *__mem, usize)
  {
    if ( __mem == nullptr ) micron::exc<micron::except::critical_error>("bb __abc_allocator::dealloc(): nullptr was provided");
    abc::dealloc(reinterpret_cast<byte *>(__mem));
  }

  static void
  dealloc(T *__mem)
  {
    if ( __mem == nullptr ) return;
    abc::dealloc(reinterpret_cast<byte *>(__mem));
  }

  static void
  dealloc_aligned(T *__mem, usize __alignment)
  {
    if ( __mem == nullptr ) return;
    abc::aligned_dealloc(reinterpret_cast<byte *>(__mem), __alignment);
  }
};

};      // namespace abc
