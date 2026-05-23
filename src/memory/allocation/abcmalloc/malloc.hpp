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

static inline bool
__is_sentinel(const byte *ptr) noexcept
{
  return ptr == (const byte *)-1;
}

// checks if pointer has been alloc.
bool
is_present(addr_t *ptr)
{
  // routes via __query_arena so queries across thread arenas resolve correctly
  return __query_arena(ptr)->present(ptr);
}

bool
is_present(byte *ptr)
{
  return is_present(reinterpret_cast<addr_t *>(ptr));
}

template<typename T>
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
  return __query_arena(ptr)->has_provenance(const_cast<addr_t *>(ptr));
}

bool
within(addr_t *ptr)
{
  return __query_arena(ptr)->has_provenance(ptr);
}

bool
within(byte *ptr)
{
  return within(reinterpret_cast<addr_t *>(ptr));
}

void
relinquish(byte *ptr)      // unmaps entire sheet at which ptr lives, resets arena entirely
{
  if ( !ptr ) [[unlikely]]
    return;
  __query_arena(ptr)->reset_page(ptr);
}

template<typename T>
  requires(!micron::same_as<T, byte>)
void
relinquish(T *__ptr)      // unmaps entire sheet at which ptr lives, resets arena entirely
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

  // verify the region isn't already tracked
  if ( __current_arena()->has_provenance(reinterpret_cast<addr_t *>(ptr)) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_mark>("mark_at(): region already tracked by allocator");
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

  if ( !__current_arena()->has_provenance(reinterpret_cast<addr_t *>(ptr)) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_unmark_untracked>("unmark_at(): region not tracked by allocator, cannot unmark");
    return nullptr;
  }

  if ( !__current_arena()->pop(ptr, size) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_unmark_failed>("unmark_at(): failed to unmark region");
    return nullptr;
  }

  return ptr;
}

micron::__chunk<byte>
balloc(usize size)      // allocates memory, returns entire memory chunk
{
  if ( size == 0 ) [[unlikely]]
    return { nullptr, 0 };

  micron::__chunk<byte> mem = __current_arena()->push(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]]
    return { nullptr, 0 };
  return mem;
}

micron::__chunk<byte>
fetch(usize size)
{
  if ( size == 0 ) [[unlikely]]
    return { nullptr, 0 };

  micron::__chunk<byte> mem = __current_arena()->push(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]]
    return { nullptr, 0 };
  return mem;
}

template<typename T>
  requires(micron::is_trivial_v<T>)
T *
fetch(void)
{
  auto mem = __current_arena()->push(sizeof(T));
  if ( __is_sentinel(mem.ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_fetch_oom>("fetch<T>(): allocation failed, out of memory");
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

  if ( !__query_arena(ptr)->ts_pop(ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_retire>(
        "retire(): tombstone free failed, pointer may not have been allocated by this arena");
  }
}

template<typename T>
  requires(!micron::same_as<T, byte>)
void
retire(T *__ptr)
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  retire(ptr);
}

__attribute__((malloc, alloc_size(1))) auto
alloc(usize size) -> byte *      // allocates memory, near iden. func. to malloc
{
  if ( size == 0 ) [[unlikely]]
    return nullptr;

  byte *ptr = balloc(size).ptr;
  return ptr;      // balloc already converts sentinel to nullptr
}

__attribute__((malloc, alloc_size(1))) byte *
salloc(usize size)
{
  if ( size == 0 ) [[unlikely]]
    return nullptr;

  micron::__chunk<byte> mem = __current_arena()->push(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_salloc_oom>("salloc(): hardened allocation failed, out of memory");
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

  // WARNING:: due to poor interop with missing the STL and micron, there are edge cases where global objects constructed by glibc might
  // be invoked through *this* dealloc, instead of the libc free() one
  if ( !__route_dealloc(ptr, 0) ) [[unlikely]] {
    // REGARDING FOREIGN POINTERS
    // there are certain cases (aka bad code) where we genuinely get hit by a rouge pointer not owned by us;
    // there are two possible ways to handle it, a) pass it off to the real allocator which owns it (impossible) b) throw an error
    // since in testing this only occured during atexit destructor calls, we will simple fall out silently and ignore dead pointers
    (void)ptr;
    // this effectively means that dealloc() NO LONGER has free() like semantics, since you can pass any pointer you want and it won't
    // crash/alert you, be careful!
  }
}

void
dealloc(byte *ptr, usize len)
{
  if ( !ptr ) [[unlikely]]
    return;
  if ( len == 0 ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_dealloc_zero>("dealloc(): zero-length free is invalid");
    return;
  }

  // dealloc(ptr len) is always explicit us, no fall throughs; treat it as a hard error, we went wrong somewhere
  if ( !__route_dealloc(ptr, len) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_dealloc_size>(
        "dealloc(): free failed with explicit size, pointer not recognised or size mismatch");
  }
}

template<typename T>
  requires(!micron::same_as<T, byte>)
void
dealloc(T *__ptr)
{
  byte *ptr = reinterpret_cast<byte *>(__ptr);
  dealloc(ptr);
}

template<typename T>
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

  if ( !__query_arena(ptr)->freeze(ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_freeze>("freeze(): failed to make region read-only, pointer not recognised by allocator");
  }
}

template<typename T>
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
  // walk every tier's index and report live (non-tombstoned) sheets
  // NOTE: this only reports sheet-level information (base addr + allocated bytes).
  // per-block enumeration would require walking each sheet's internal book for now, report total usage per tier class
  usize tot_precise = 0, tot_small = 0, tot_medium = 0, tot_large = 0, tot_huge = 0;
  usize tot_all = 0, tot_meta_avail = 0;
  __for_each_live_arena([&](__arena &a) {
    tot_precise += a.total_usage_of_class<__class_precise>();
    tot_small += a.total_usage_of_class<__class_small>();
    tot_medium += a.total_usage_of_class<__class_medium>();
    tot_large += a.total_usage_of_class<__class_large>();
    tot_huge += a.total_usage_of_class<__class_huge>();
    tot_all += a.total_usage();
    tot_meta_avail += a.__available_buffer();
  });
  __debug_print("which(): precise tier allocated: ", tot_precise);
  __debug_print("which(): small tier allocated: ", tot_small);
  __debug_print("which(): medium tier allocated: ", tot_medium);
  __debug_print("which(): large tier allocated: ", tot_large);
  __debug_print("which(): huge tier allocated: ", tot_huge);
  __debug_print("which(): total: ", tot_all);
  __debug_print("which(): arena metadata remaining: ", tot_meta_avail);
}

__attribute__((malloc, alloc_size(1))) byte *
launder(usize size)
{
  if ( size == 0 ) [[unlikely]]
    return nullptr;

  micron::__chunk<byte> mem = __current_arena()->launder(size);
  if ( __is_sentinel(mem.ptr) ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_launder>("launder(): temporal allocation failed, out of memory");
    return nullptr;
  }
  return mem.ptr;
}

template<typename T>
usize
query_size(T *ptr)
{
  if ( !ptr ) [[unlikely]]
    return 0;

  return __query_arena(ptr)->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
}

// leave these as void*
// launder()
// query
// inject
// make_at

usize
musage(void)
{
  usize total = 0;
  __for_each_live_arena([&](__arena &a) { total += a.total_usage(); });
  return total;
}

template<u64 Sz>
usize
musage(void)
{
  usize total = 0;
  __for_each_live_arena([&](__arena &a) { total += a.template total_usage_of_class<Sz>(); });
  return total;
}

__attribute__((malloc, alloc_size(1))) void *
malloc(usize size)      // alloc memory of size 'size', prefer using alloc
{
  return reinterpret_cast<void *>(abc::alloc(size));
}

void *
calloc(usize num, usize size)      // alloc's zero'd out memory, prefer using salloc()
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
realloc(void *ptr, usize size)      // reallocates memory
{
  if ( !ptr ) return reinterpret_cast<void *>(abc::alloc(size));

  if ( size == 0 ) {
    abc::dealloc(reinterpret_cast<byte *>(ptr));
    return nullptr;
  }

  byte *result = __current_arena()->resize(reinterpret_cast<byte *>(ptr), size);
  if ( !result ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_realloc_unknown>("realloc(): resize failed (pointer not recognised or allocation OOM)");
    return nullptr;
  }
  return reinterpret_cast<void *>(result);
}

void
free(void *ptr)      // frees memory, prefer abc::dealloc always
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

  uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw);
  uintptr_t aligned_addr = reinterpret_cast<uintptr_t>(ptr);
  if ( raw_addr >= aligned_addr ) [[unlikely]] {
    micron::exc<micron::except::memory_error_abc_aligned_free_bad>(
        "aligned_free(): stashed raw pointer is invalid, this pointer was not allocated by aligned_alloc");
    return;
  }

  abc::dealloc(raw);
}

};      // namespace abc

#include "malloc-c.hpp"
