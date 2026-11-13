//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../bits/wasm/memory.hpp"

#if defined(MICRON_BB_PORT_POOL) || defined(MICRON_BB_PORT_GROW) || defined(MICRON_BB_STATIC_POOL) || defined(MICRON_BB_LINKER_POOL)
#error "micron: wasmalloc owns its linear-memory regions; do not select another bbmalloc pool"
#endif
#if defined(MICRON_BB_FAIL_RESULT) && MICRON_BB_FAIL_RESULT != 0
#error "micron: wasmalloc handles allocation failure after releasing the heap lock"
#endif
#define MICRON_BB_NO_DIAG 1
#ifndef MICRON_BB_MAX_REGIONS
#define MICRON_BB_MAX_REGIONS 8
#endif
#ifndef MICRON_BB_MAX_REGION_BYTES
#define MICRON_BB_MAX_REGION_BYTES (256u << 20)
#endif
#ifndef MICRON_BB_MAX_ALIGN
#define MICRON_BB_MAX_ALIGN MICRON_BB_MAX_REGION_BYTES
#endif
#ifndef MICRON_WASM_HEAP_REGION_BYTES
#define MICRON_WASM_HEAP_REGION_BYTES (1u << 20)
#endif

#include "upstream/malloc.hpp"

#if defined(MICRON_BB_PORT_POOL) || defined(MICRON_BB_PORT_GROW) || defined(MICRON_BB_STATIC_POOL) || defined(MICRON_BB_LINKER_POOL)
#error "micron: wasmalloc configuration cannot install a bbmalloc page source"
#endif

namespace micron::wasmalloc
{

inline constexpr usize native_block_alignment = bb::native_alignment;
inline constinit wasm::__mutex __heap_mutex{};
inline constinit usize __next_region_bytes = MICRON_WASM_HEAP_REGION_BYTES;
inline constinit u32 __region_count = 0;

static_assert(MICRON_WASM_HEAP_REGION_BYTES >= 4 * bb::__default_tlsf_sheet);
static_assert(MICRON_WASM_HEAP_REGION_BYTES <= bb::__default_max_region_bytes);
static_assert((MICRON_WASM_HEAP_REGION_BYTES & (MICRON_WASM_HEAP_REGION_BYTES - 1)) == 0);
static_assert(bb::__default_fail_result == 0, "wasmalloc: throwing or terminating bbmalloc failure hooks cannot run under the heap lock");

[[nodiscard]] inline bool
__grow(usize bytes, usize alignment) noexcept
{
  if ( __region_count >= bb::__max_regions ) return false;
  if ( alignment > bb::__default_max_alignment || bytes > static_cast<usize>(-1) - bb::__rz ) return false;
  bytes += bb::__rz;
  usize want = bytes > alignment ? bytes : alignment;
  if ( want > bb::__default_max_region_bytes ) return false;
  usize region = __next_region_bytes;
  while ( region < want ) {
    if ( region > bb::__default_max_region_bytes / 2 ) return false;
    region *= 2;
  }
  usize tags = 0;
  byte *base = nullptr;
  for ( ;; ) {
    tags = region / bb::__default_min_block;
    if ( tags > static_cast<usize>(-1) - region ) return false;
    const usize base_alignment = region < bb::__default_max_alignment ? region : bb::__default_max_alignment;
    const usize place = alignment > base_alignment ? alignment : base_alignment;
    base = static_cast<byte *>(wasm::linear_reserve(region + tags, place));
    if ( base != nullptr ) break;
    if ( region / 2 < want || region / 2 < 4 * bb::__default_tlsf_sheet ) return false;
    region /= 2;
  }
  if ( !bb::attach(base, region, reinterpret_cast<u8 *>(base + region), tags) ) __builtin_trap();
  ++__region_count;
  __next_region_bytes = region <= bb::__default_max_region_bytes / 2 ? region * 2 : region;
  return true;
}

[[nodiscard]] inline chunk<byte>
__allocate(usize bytes, usize alignment) noexcept
{
  if ( bytes == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0 ) return { nullptr, 0 };
  auto result = bb::aligned_balloc(alignment, bytes);
  if ( result.ptr == nullptr && __grow(bytes, alignment) ) result = bb::aligned_balloc(alignment, bytes);
  return result;
}

[[nodiscard]] inline chunk<byte>
aligned_balloc(usize alignment, usize bytes) noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return __allocate(bytes, alignment);
}

[[nodiscard]] inline chunk<byte>
balloc(usize bytes) noexcept
{
  return aligned_balloc(native_block_alignment, bytes);
}

[[nodiscard]] inline byte *
alloc(usize bytes) noexcept
{
  return balloc(bytes).ptr;
}

[[nodiscard]] inline void *
try_alloc(usize bytes) noexcept
{
  return alloc(bytes);
}

[[nodiscard]] inline chunk<byte>
zalloc(usize bytes) noexcept
{
  chunk<byte> result = balloc(bytes);
  if ( result.ptr ) __builtin_memset(result.ptr, 0, result.len);
  return result;
}

[[nodiscard]] inline chunk<byte>
fetch(usize bytes) noexcept
{
  return zalloc(bytes);
}

[[nodiscard]] inline byte *
salloc(usize bytes) noexcept
{
  return zalloc(bytes).ptr;
}

inline void
dealloc(void *ptr) noexcept
{
  if ( ptr == nullptr ) return;
  wasm::__lock_guard lock{ __heap_mutex };
  (void)bb::dealloc(ptr);
}

inline void
dealloc(byte *ptr, usize) noexcept
{
  dealloc(static_cast<void *>(ptr));
}

inline void
dealloc(chunk<byte> memory) noexcept
{
  dealloc(memory.ptr);
}

inline void
dealloc(chunk<byte> memory, usize) noexcept
{
  dealloc(memory.ptr);
}

inline void
aligned_dealloc(byte *ptr, usize) noexcept
{
  dealloc(ptr);
}

inline void
aligned_dealloc(chunk<byte> memory, usize) noexcept
{
  dealloc(memory.ptr);
}

inline void
aligned_free(void *ptr, usize = native_block_alignment) noexcept
{
  dealloc(ptr);
}

inline void
free(void *ptr) noexcept
{
  dealloc(ptr);
}

[[nodiscard]] inline usize
query_size(const void *ptr) noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return bb::query_size(ptr);
}

[[nodiscard]] inline bool
within(const void *ptr) noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return bb::within(ptr);
}

[[nodiscard]] inline bool
is_present(const void *ptr) noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return bb::is_present(ptr);
}

[[nodiscard]] inline usize
musage() noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return bb::musage();
}

[[nodiscard]] inline usize
capacity() noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return bb::capacity();
}

[[nodiscard]] inline usize
available() noexcept
{
  wasm::__lock_guard lock{ __heap_mutex };
  return bb::available();
}

[[nodiscard]] inline chunk<byte>
resize(chunk<byte> old, usize bytes, usize preserve, usize alignment) noexcept
{
  if ( alignment == 0 || (alignment & (alignment - 1)) != 0 ) return { nullptr, 0 };
  wasm::__lock_guard lock{ __heap_mutex };
  auto result = bb::resize(old, bytes, preserve, alignment);
  if ( bytes != 0 && result.ptr == nullptr && __grow(bytes, alignment) ) result = bb::resize(old, bytes, preserve, alignment);
  return result;
}

[[nodiscard]] inline chunk<byte>
resize_chunk(chunk<byte> old, usize bytes, usize preserve) noexcept
{
  return resize(old, bytes, preserve, native_block_alignment);
}

[[nodiscard]] inline chunk<byte>
aligned_resize(chunk<byte> old, usize bytes, usize preserve, usize alignment) noexcept
{
  return resize(old, bytes, preserve, alignment);
}

[[nodiscard]] inline void *
realloc(void *ptr, usize bytes) noexcept
{
  return resize({ static_cast<byte *>(ptr), static_cast<usize>(-1) }, bytes, static_cast<usize>(-1), native_block_alignment).ptr;
}

[[nodiscard]] inline void *
malloc(usize bytes) noexcept
{
  return try_alloc(bytes);
}

[[nodiscard]] inline void *
calloc(usize count, usize bytes) noexcept
{
  if ( count != 0 && bytes > static_cast<usize>(-1) / count ) return nullptr;
  return zalloc(count * bytes).ptr;
}

[[nodiscard]] inline void *
aligned_alloc(usize alignment, usize bytes) noexcept
{
  if ( alignment == 0 || (alignment & (alignment - 1)) != 0 || bytes % alignment != 0 ) return nullptr;
  return aligned_balloc(alignment, bytes).ptr;
}

};      // namespace micron::wasmalloc

namespace abc = micron::wasmalloc;
