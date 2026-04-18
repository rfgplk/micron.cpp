// Copyright (c) 2025 David Lucius Severus
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

#include "../../../type_traits.hpp"
#include "../../../types.hpp"
#include "arena.hpp"
#include "tapi.hpp"

#include "../../../except.hpp"

namespace abc
{

// NOTE: when __guard_abcmalloc leaves scope, it'll automatically release the underlying mutex

#define __ABC_LOCK_AND_INIT()                                                                                                              \
  [[maybe_unused]] auto __abc_scoped_lock = [&]() {                                                                                        \
    if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> )                                                \
      return micron::move(__guard_abcmalloc());                                                                                            \
    else                                                                                                                                   \
      return 0;                                                                                                                            \
  }();                                                                                                                                     \
  __start_abcmalloc_init()

static inline bool
__is_sentinel(const byte *ptr) noexcept
{
  return ptr == (const byte *)-1;
}

// checks if pointer has been alloc.
bool
is_present(addr_t *ptr)
{
  __ABC_LOCK_AND_INIT();
  return __main_arena->present(ptr);
}

bool
is_present(byte *ptr)
{
  return is_present(reinterpret_cast<addr_t *>(ptr));
}

template <typename T>
  requires(!micron::same_as<T, byte>)
bool
is_present(T *ptr)
{
  return is_present(reinterpret_cast<addr_t *>(ptr));
}

// checks if pointer is addressable at any known page of the allocator, if it is it's valid
bool
within(const addr_t *ptr)
{
  __ABC_LOCK_AND_INIT();
  return __main_arena->has_provenance(const_cast<addr_t *>(ptr));
}

bool
within(addr_t *ptr)
{
  __ABC_LOCK_AND_INIT();
  return __main_arena->has_provenance(ptr);
}

bool
within(byte *ptr)
{
  return within(reinterpret_cast<addr_t *>(ptr));
}

void
relinquish(byte *ptr)     // unmaps entire sheet at which ptr lives, resets arena entirely
{
  __ABC_LOCK_AND_INIT();
  if ( !ptr ) [[unlikely]]
    return;
  __main_arena->reset_page(ptr);
}

template <typename T>
  requires(!micron::same_as<T, byte>)
void
relinquish(T *__ptr)     // unmaps entire sheet at which ptr lives, resets arena entirely
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  relinquish(ptr);
}

// these bypass the normal allocation path and directly push/pop the given {ptr, size} chunk used for externally-mapped memory that the
// allocator needs to track for provenance or freeze operations

byte *
mark_at(byte *ptr, usize size)
{
  if ( !ptr or size == 0 ) [[unlikely]]
    return nullptr;

  __ABC_LOCK_AND_INIT();

  // verify the region isn't already tracked
  if ( __main_arena->has_provenance(reinterpret_cast<addr_t *>(ptr)) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("mark_at(): region already tracked by allocator");
    return nullptr;
  }

  // NOTE: the allocator does not mmap on behalf of mark_at
  (void)size;
  return ptr;
}

byte *
unmark_at(byte *ptr, usize size)
{
  if ( !ptr or size == 0 ) [[unlikely]]
    return nullptr;

  __ABC_LOCK_AND_INIT();

  if ( !__main_arena->has_provenance(reinterpret_cast<addr_t *>(ptr)) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("unmark_at(): region not tracked by allocator, cannot unmark");
    return nullptr;
  }

  if ( !__main_arena->pop(ptr, size) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("unmark_at(): failed to unmark region");
    return nullptr;
  }

  return ptr;
}

micron::__chunk<byte>
balloc(usize size)     // allocates memory, returns entire memory chunk
{
  if ( size == 0 ) [[unlikely]]
    return { nullptr, 0 };

  __ABC_LOCK_AND_INIT();
  micron::__chunk<byte> mem = __main_arena->push(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]]
    return { nullptr, 0 };
  return mem;
}

micron::__chunk<byte>
fetch(usize size)
{
  if ( size == 0 ) [[unlikely]]
    return { nullptr, 0 };

  __ABC_LOCK_AND_INIT();
  micron::__chunk<byte> mem = __main_arena->push(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]]
    return { nullptr, 0 };
  return mem;
}

template <typename T>
  requires(micron::is_trivial_v<T>)
T *
fetch(void)
{
  __ABC_LOCK_AND_INIT();
  auto mem = __main_arena->push(sizeof(T));
  if ( __is_sentinel(mem.ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("fetch<T>(): allocation failed, out of memory");
    return nullptr;
  }
  return reinterpret_cast<T *>(mem.ptr);
}

// tombstone
void
retire(byte *ptr)
{
  if ( !ptr ) [[unlikely]]
    return;

  __ABC_LOCK_AND_INIT();
  if ( !__main_arena->ts_pop(ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("retire(): tombstone free failed, pointer may not have been allocated by this arena");
  }
}

template <typename T>
  requires(!micron::same_as<T, byte>)
void
retire(T *__ptr)
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  retire(ptr);
}

__attribute__((malloc, alloc_size(1))) auto
alloc(usize size) -> byte *     // allocates memory, near iden. func. to malloc
{
  if ( size == 0 ) [[unlikely]]
    return nullptr;

  byte *ptr = balloc(size).ptr;
  return ptr;     // balloc already converts sentinel to nullptr
}

__attribute__((malloc, alloc_size(1))) byte *
salloc(usize size)
{
  if ( size == 0 ) [[unlikely]]
    return nullptr;

  __ABC_LOCK_AND_INIT();
  micron::__chunk<byte> mem = __main_arena->push(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("salloc(): hardened allocation failed, out of memory");
    return nullptr;
  }

  micron::bzero(mem.ptr, mem.len);

  return mem.ptr;
}

void
dealloc(byte *ptr)
{
  if ( !ptr ) [[unlikely]]
    return;

  __ABC_LOCK_AND_INIT();
  // WARNING:: due to poor interop with missing the STL and micron, there are edge cases where global objects constructed by glibc might
  // be invoked through *this* dealloc, instead of the libc free() one
  if ( !__main_arena->pop(ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("dealloc(): free failed, pointer not recognised by allocator or double-free detected");
  }
}

void
dealloc(byte *ptr, usize len)
{
  if ( !ptr ) [[unlikely]]
    return;
  if ( len == 0 ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("dealloc(): zero-length free is invalid");
    return;
  }

  __ABC_LOCK_AND_INIT();
  if ( !__main_arena->pop({ ptr, len }) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("dealloc(): free failed with explicit size, pointer not recognised or size mismatch");
  }
}

template <typename T>
  requires(!micron::same_as<T, byte>)
void
dealloc(T *__ptr)
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  dealloc(ptr);
}

template <typename T>
  requires(!micron::same_as<T, byte>)
void
dealloc(T *__ptr, usize len)
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  dealloc(ptr, len);
}

void
freeze(byte *ptr)
{
  if ( !ptr ) [[unlikely]]
    return;

  __ABC_LOCK_AND_INIT();
  if ( !__main_arena->freeze(ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("freeze(): failed to make region read-only, pointer not recognised by allocator");
  }
}

template <typename T>
  requires(!micron::same_as<T, byte>)
void
freeze(T *__ptr)
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  freeze(ptr);
}

void
which(void)
{
  __ABC_LOCK_AND_INIT();
  // walk every tier's index and report live (non-tombstoned) sheets
  // NOTE: this only reports sheet-level information (base addr + allocated bytes).
  // per-block enumeration would require walking each sheet's internal book for now, report total usage per tier class

  __debug_print("which(): precise tier allocated: ", __main_arena->total_usage_of_class<__class_precise>());
  __debug_print("which(): small tier allocated: ", __main_arena->total_usage_of_class<__class_small>());
  __debug_print("which(): medium tier allocated: ", __main_arena->total_usage_of_class<__class_medium>());
  __debug_print("which(): large tier allocated: ", __main_arena->total_usage_of_class<__class_large>());
  __debug_print("which(): huge tier allocated: ", __main_arena->total_usage_of_class<__class_huge>());
  __debug_print("which(): total: ", __main_arena->total_usage());
  __debug_print("which(): arena metadata remaining: ", __main_arena->__available_buffer());
}

__attribute__((malloc, alloc_size(1))) byte *
launder(usize size)
{
  if ( size == 0 ) [[unlikely]]
    return nullptr;

  __ABC_LOCK_AND_INIT();
  micron::__chunk<byte> mem = __main_arena->launder(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("launder(): temporal allocation failed, out of memory");
    return nullptr;
  }
  return mem.ptr;
}

template <typename T>
usize
query_size(T *ptr)
{
  if ( !ptr ) [[unlikely]]
    return 0;

  __ABC_LOCK_AND_INIT();
  return __main_arena->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
}

// leave these as void*
// launder()
// query
// inject
// make_at

usize
musage(void)
{
  __ABC_LOCK_AND_INIT();
  return __main_arena->total_usage();
}

template <u64 Sz>
usize
musage(void)
{
  __ABC_LOCK_AND_INIT();
  return __main_arena->total_usage_of_class<Sz>();
}

__attribute__((malloc, alloc_size(1))) void *
malloc(usize size)     // alloc memory of size 'size', prefer using alloc
{
  return reinterpret_cast<void *>(abc::alloc(size));
}

void *
calloc(usize num, usize size)     // alloc's zero'd out memory, prefer using salloc()
{
  if ( num == 0 or size == 0 ) return nullptr;

  usize total;
  if ( check_mul_overflow(num, size, total) ) [[unlikely]]
    return nullptr;

  byte *mem = abc::alloc(total);
  if ( !mem ) return nullptr;
  micron::zero(mem, total);
  return mem;
}

void *
realloc(void *ptr, usize size)     // reallocates memory
{
  if ( !ptr ) return reinterpret_cast<void *>(abc::alloc(size));

  if ( size == 0 ) {
    abc::dealloc(reinterpret_cast<byte *>(ptr));
    return nullptr;
  }

  // NOTE: this always gets the full size of the allocated memory, not what was requested
  usize old_size = abc::query_size(reinterpret_cast<addr_t *>(ptr));
  if ( old_size == 0 ) [[unlikely]] {
    micron::exc<micron::except::memory_error>("realloc(): cannot determine size of existing allocation, pointer not recognised");
    return nullptr;
  }

  // if the existing block is already large enough and not wastefully oversized, reuse it
  if ( old_size >= size and size > (old_size >> 1) ) return ptr;

  byte *new_block = abc::alloc(size);
  if ( !new_block ) [[unlikely]]
    return nullptr;     // allocation failed, old block untouched

  usize copy_size = old_size < size ? old_size : size;
  micron::memcpy(new_block, reinterpret_cast<byte *>(ptr), copy_size);

  abc::dealloc(reinterpret_cast<byte *>(ptr));

  return new_block;
}

void
free(void *ptr)     // frees memory, prefer abc::dealloc always
{
  if ( !ptr ) [[unlikely]]
    return;
  abc::dealloc(reinterpret_cast<byte *>(ptr));
}

// C11 aligned_alloc

// WARNING: when alignment > __hdr_offset (32), the returned pointer differs from the allocator's block start. abc::free / abc::dealloc will
// NOT work: you __MUST__ use abc::aligned_free for these pointers. when alignment <= __hdr_offset, the pointer is identical to a normal
// allocation and abc::free works normally.

void *
aligned_alloc(usize alignment, usize size)
{
  // !alignment must be power of two
  if ( alignment == 0 or (alignment & (alignment - 1)) != 0 ) [[unlikely]]
    return nullptr;

  // !size must be a multiple of alignment
  if ( size % alignment != 0 ) [[unlikely]]
    return nullptr;

  if ( size == 0 ) [[unlikely]]
    return nullptr;

  if ( alignment <= __hdr_offset ) return reinterpret_cast<void *>(abc::alloc(size));

  usize overhead = alignment + sizeof(void *);
  usize total;
  if ( check_add_overflow(size, overhead, total) ) [[unlikely]]
    return nullptr;

  byte *raw = abc::alloc(total);
  if ( !raw ) [[unlikely]]
    return nullptr;

  // find the first aligned address at least sizeof(void*) bytes past raw
  uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw) + sizeof(void *);
  uintptr_t aligned_addr = (raw_addr + alignment - 1) & ~(alignment - 1);
  byte *aligned = reinterpret_cast<byte *>(aligned_addr);

  // stash the original pointer immediately before the aligned return
  *reinterpret_cast<byte **>(aligned - sizeof(void *)) = raw;

  return reinterpret_cast<void *>(aligned);
}

void
aligned_free(void *ptr)
{
  if ( !ptr ) [[unlikely]]
    return;

  // recover the original raw pointer stashed before the aligned address
  byte *raw = *reinterpret_cast<byte **>(reinterpret_cast<byte *>(ptr) - sizeof(void *));

  // sanity: if the stashed pointer equals the aligned pointer, this was abnormal allocation shouldn't happen through the aligned_alloc path
  // reroute to regular dealloc
  if ( raw == reinterpret_cast<byte *>(ptr) ) [[unlikely]] {
    abc::dealloc(reinterpret_cast<byte *>(ptr));
    return;
  }

  // verify the raw pointer is at least vaguely sane: it must be before
  // the aligned pointer and within one page of it (alignment cannot exceed the system page size in any case)
  uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw);
  uintptr_t aligned_addr = reinterpret_cast<uintptr_t>(ptr);
  if ( raw_addr >= aligned_addr or (aligned_addr - raw_addr) > __system_pagesize ) [[unlikely]] {
    micron::exc<micron::except::memory_error>(
        "aligned_free(): stashed raw pointer is invalid, this pointer was not allocated by aligned_alloc");
    return;
  }

  abc::dealloc(raw);
}

};     // namespace abc

#include "malloc-c.hpp"
