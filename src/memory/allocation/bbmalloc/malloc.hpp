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
#include "harden.hpp"
#include "heap.hpp"
#include "metadata.hpp"
#include "printing.hpp"
#include "stats.hpp"

#include "../../../memory/allocation/kmemory.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"

#if MICRON_BB_FAIL_RESULT == 2
#include "../../../except.hpp"
#endif

namespace bb
{

namespace __bits
{

inline micron::__chunk<byte>
__fail(usize n) noexcept(__default_fail_result != 2)
{
  collect_stats<stat_type::failure>();
  if constexpr( __default_fail_result == 0 ) {
    (void)n;
    return { nullptr, 0 };
  } else if constexpr( __default_fail_result == 1 ) {
    __notice("allocation failed for", n);
    __halt(__critical_exit);
  } else {
#if MICRON_BB_FAIL_RESULT == 2
    micron::exc<micron::except::memory_error>("bbmalloc: allocation failed");
#endif
    (void)n;
    return { nullptr, 0 };
  }
}

inline micron::__chunk<byte>
__finish(micron::__chunk<byte> c, usize requested) noexcept
{
  if( c.ptr == nullptr )
    return c;
  const usize user = c.len - __rz;
  write_redzone(c.ptr, user);
  zero_on_alloc(c.ptr, user);
  collect_stats<stat_type::alloc>();
  collect_stats<stat_type::requested>(requested);
  collect_stats<stat_type::granted>(c.len);
  return { c.ptr, user };
}

};

inline bool
attach(byte *ptr, usize len) noexcept
{
  return __the_heap.attach(ptr, len, nullptr, 0);
}

inline bool
attach(micron::__chunk<byte> region) noexcept
{
  return __the_heap.attach(region.ptr, region.len, nullptr, 0);
}

inline bool
attach(byte *ptr, usize len, u8 *tags, usize tag_cap) noexcept
{
  return __the_heap.attach(ptr, len, tags, tag_cap);
}

inline bool
init() noexcept
{
  return __the_heap.init();
}

inline void
reset() noexcept
{
  __the_heap.reset();
}

__bb_hot micron::__chunk<byte>
balloc(usize size) noexcept(__default_fail_result != 2)
{
  if( size == 0 )
    return { nullptr, 0 };
  usize want = 0;
  if( check_add_overflow(size, __rz, want) )
    return __bits::__fail(size);
  micron::__chunk<byte> c = __the_heap.alloc(want);
  if( c.ptr == nullptr )
    return __bits::__fail(size);
  return __bits::__finish(c, size);
}

inline micron::__chunk<byte>
aligned_balloc(usize alignment, usize size) noexcept(__default_fail_result != 2)
{
  if( size == 0 || !__bits::__is_pow2(alignment) )
    return { nullptr, 0 };
  if( alignment <= native_alignment )
    return balloc(size);
  usize want = 0;
  if( check_add_overflow(size, __rz, want) )
    return __bits::__fail(size);
  micron::__chunk<byte> c = __the_heap.alloc_aligned(want, alignment);
  if( c.ptr == nullptr )
    return __bits::__fail(size);
  return __bits::__finish(c, size);
}

inline micron::__chunk<byte>
zalloc(usize size) noexcept(__default_fail_result != 2)
{
  micron::__chunk<byte> c = balloc(size);
  if( c.ptr != nullptr )
    __builtin_memset(c.ptr, 0, c.len);
  return c;
}

inline micron::__chunk<byte>
fetch(usize size) noexcept(__default_fail_result != 2)
{
  return zalloc(size);
}

template <typename T>
  requires(micron::is_trivially_constructible_v<T>)
inline T *
fetch() noexcept(__default_fail_result != 2)
{
  return reinterpret_cast<T *>(zalloc(sizeof(T)).ptr);
}

__bb_hot __attribute__((malloc, alloc_size(1))) byte *
alloc(usize size) noexcept(__default_fail_result != 2)
{
  return balloc(size).ptr;
}

__attribute__((malloc, alloc_size(1))) inline byte *
salloc(usize size) noexcept(__default_fail_result != 2)
{
  return zalloc(size).ptr;
}

inline usize
query_size(const void *ptr) noexcept
{
  const usize s = __the_heap.size_of(ptr);
  return s == 0 ? 0 : s - __rz;
}

inline bool
is_present(const void *ptr) noexcept
{
  return ptr != nullptr && __the_heap.present(ptr);
}

inline bool
within(const void *ptr) noexcept
{
  return ptr != nullptr && __the_heap.within(ptr);
}

namespace __bits
{

inline bool
__release_located(const __located &l, void *ptr, usize block) noexcept
{
  byte *p = static_cast<byte *>(ptr);
  scrub_on_free(p, block);
  collect_stats<stat_type::dealloc>();
  collect_stats<stat_type::freed>(block);
  return __the_heap.release(l, ptr);
}

};

__bb_hot bool
dealloc(void *ptr) noexcept
{
  if( ptr == nullptr )
    return true;
  const __located l = __the_heap.locate(ptr);
  if( l.len == 0 || l.len < __rz ) [[unlikely]]
    return handle_bad_free(ptr);
  byte *p = static_cast<byte *>(ptr);
  if( !verify_redzone(p, l.len - __rz) ) [[unlikely]]
    (void)handle_redzone_trip(ptr);
  return __bits::__release_located(l, ptr, l.len);
}

inline void
dealloc(byte *ptr) noexcept
{
  (void)dealloc(static_cast<void *>(ptr));
}

inline void
dealloc(byte *ptr, usize len) noexcept
{
  (void)len;
  (void)dealloc(static_cast<void *>(ptr));
}

template <typename T>
  requires(!micron::is_same_v<T, byte> && !micron::is_same_v<T, void>)
inline void
dealloc(T *ptr) noexcept
{
  (void)dealloc(static_cast<void *>(ptr));
}

template <typename T>
  requires(!micron::is_same_v<T, byte> && !micron::is_same_v<T, void>)
inline void
dealloc(T *ptr, usize len) noexcept
{
  (void)len;
  (void)dealloc(static_cast<void *>(ptr));
}

inline void
dealloc(micron::__chunk<byte> memory, usize) noexcept
{
  (void)dealloc(static_cast<void *>(memory.ptr));
}

inline void
aligned_dealloc(byte *ptr, usize) noexcept
{
  (void)dealloc(static_cast<void *>(ptr));
}

inline void
aligned_dealloc(micron::__chunk<byte> memory, usize) noexcept
{
  (void)dealloc(static_cast<void *>(memory.ptr));
}

inline void
aligned_free(void *ptr) noexcept
{
  (void)dealloc(ptr);
}

inline void
aligned_free(void *ptr, usize) noexcept
{
  (void)dealloc(ptr);
}

inline micron::__chunk<byte>
resize(micron::__chunk<byte> old, usize size, usize preserve, usize alignment) noexcept(__default_fail_result != 2)
{
  if( !__bits::__is_pow2(alignment) )
    return { nullptr, 0 };
  if( old.ptr == nullptr )
    return alignment <= native_alignment ? balloc(size) : aligned_balloc(alignment, size);
  if( size == 0 ) {
    (void)dealloc(static_cast<void *>(old.ptr));
    return { nullptr, 0 };
  }
  const __located l = __the_heap.locate(old.ptr);
  const usize block = l.len;
  if( block == 0 || block < __rz ) {
    (void)handle_bad_free(old.ptr);
    return { nullptr, 0 };
  }
  const usize have = block - __rz;
  if( !verify_redzone(old.ptr, have) )
    (void)handle_redzone_trip(old.ptr);
  const bool placed = (reinterpret_cast<uintptr_t>(old.ptr) & (alignment - 1)) == 0;
  usize want = 0;
  const bool sane = !check_add_overflow(size, __rz, want);
  if( size <= have && placed ) {
    const usize nb = sane ? __the_heap.shrink_at(l, want) : 0;
    if( nb != 0 ) {
      const usize now = nb < __rz ? 0 : nb - __rz;
      write_redzone(old.ptr, now);
      if( block > nb )
        collect_stats<stat_type::freed>(block - nb);
      return { old.ptr, now };
    }
    return { old.ptr, have };
  }
  if( placed && sane ) {
    const usize nb = __the_heap.grow_at(l, old.ptr, want);
    if( nb != 0 ) {
      const usize now = nb - __rz;
      write_redzone(old.ptr, now);
      collect_stats<stat_type::granted>(nb - block);
      return { old.ptr, now };
    }
  }
  micron::__chunk<byte> next = alignment <= native_alignment ? balloc(size) : aligned_balloc(alignment, size);
  if( next.ptr == nullptr )
    return { nullptr, 0 };
  usize copy = preserve;
  if( copy > old.len )
    copy = old.len;
  if( copy > have )
    copy = have;
  if( copy > next.len )
    copy = next.len;
  if( copy != 0 )
    __builtin_memcpy(next.ptr, old.ptr, copy);
  (void)__bits::__release_located(l, static_cast<void *>(old.ptr), block);
  return next;
}

[[nodiscard]] inline micron::__chunk<byte>
resize_chunk(micron::__chunk<byte> old, usize size, usize preserve) noexcept(__default_fail_result != 2)
{
  return resize(old, size, preserve, native_alignment);
}

[[nodiscard]] inline micron::__chunk<byte>
aligned_resize(micron::__chunk<byte> old, usize size, usize preserve,
               usize alignment) noexcept(__default_fail_result != 2)
{
  return resize(old, size, preserve, alignment);
}

inline void *
malloc(usize size) noexcept(__default_fail_result != 2)
{
  return static_cast<void *>(alloc(size));
}

inline void *
calloc(usize num, usize size) noexcept(__default_fail_result != 2)
{
  usize total = 0;
  if( check_mul_overflow(num, size, total) )
    return static_cast<void *>(__bits::__fail(size).ptr);
  return static_cast<void *>(salloc(total));
}

inline void *
realloc(void *ptr, usize size) noexcept(__default_fail_result != 2)
{
  if( ptr == nullptr )
    return static_cast<void *>(alloc(size));
  return static_cast<void *>(
      resize({ static_cast<byte *>(ptr), __bits::__usize_max }, size, __bits::__usize_max, native_alignment).ptr);
}

inline void
free(void *ptr) noexcept
{
  (void)dealloc(ptr);
}

inline void *
aligned_alloc(usize alignment, usize size) noexcept(__default_fail_result != 2)
{
  if( !__bits::__is_pow2(alignment) )
    return static_cast<void *>(__bits::__fail(size).ptr);
  if( size == 0 || (size & (alignment - 1)) != 0 )
    return static_cast<void *>(__bits::__fail(size).ptr);
  return static_cast<void *>(aligned_balloc(alignment, size).ptr);
}

inline usize
musage() noexcept
{
  return __the_heap.used();
}

inline usize
available() noexcept
{
  return __the_heap.available();
}

inline usize
largest_free() noexcept
{
  const usize l = __the_heap.largest_free();
  return l <= __rz ? 0 : l - __rz;
}

inline usize
capacity() noexcept
{
  return __the_heap.total();
}

inline u32
fragmentation_permille() noexcept
{
  const u64 avail = __the_heap.available();
  if( avail == 0 )
    return 0;
  const u64 largest = __the_heap.largest_free();
  if( largest >= avail )
    return 0;
  return static_cast<u32>(1000 - (largest * 1000) / avail);
}

inline bb_stats
stats() noexcept
{
  return get_stats();
}

inline void
which() noexcept
{
  __bits::__write("bbmalloc: regions ");
  __bits::__print_unsigned(__the_heap.region_count());
  __bits::__write(" capacity ");
  __bits::__print_unsigned(__the_heap.total());
  __bits::__write(" used ");
  __bits::__print_unsigned(__the_heap.used());
  __bits::__write(" available ");
  __bits::__print_unsigned(__the_heap.available());
  __bits::__write(" largest ");
  __bits::__print_unsigned(__the_heap.largest_free());
  __bits::__write("\n");
  for( u32 i = 0; i < __the_heap.count; ++i ) {
    __bits::__write("  region ");
    __bits::__print_unsigned(i);
    __bits::__write(" sheets ");
    __bits::__print_unsigned(__the_heap.regions[i].tlsf.count);
    __bits::__write(" tlsf used ");
    __bits::__print_unsigned(__the_heap.regions[i].tlsf.allocated_bytes);
    __bits::__write(" buddy used ");
    __bits::__print_unsigned(__the_heap.regions[i].buddy.used());
    __bits::__write("\n");
  }
}

};
