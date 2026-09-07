//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../cmalloc.hpp"

#include "../../../except.hpp"
#include "../../../memory/addr.hpp"
#include "../../../port/pages.hpp"
#include "../../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// low-level allocator adapters

namespace micron
{

// default micron allocator, uses abcmalloc directly
//
// The barebones tier takes this SAME arm, not a third one: under __micron_bb_alloc, namespace abc
// is the alias surface over micron::bb (allocation/barebones/abc_shim.hpp), so every name below
// resolves unchanged. That is the payoff of shimming the namespace instead of introducing a second
// allocator adapter -- this file, memory/new.hpp and all six abc:-bypassing containers keep one
// code path across both allocators, so the (K) build exercises the arm the hosted build exercises.
// The #else below is the LIBC arm and is hosted-only; __internal.hpp #errors before it on a
// freestanding build.
#if defined(__micron_abcmalloc_present) || defined(__micron_bb_alloc)
template<typename T> struct abc_allocator {
  // difference between allocate and umanaged_* calls is that allocate pulls memory from the allocator, while unmanaged
  // pulls pages from the kernel directly, for when you need to manage memory yourself
  static auto
  allocate(usize sz) -> __chunk<byte>
  {
    return abc::__abc_allocator<byte>::calloc(sz);
  }

  static auto
  allocate_aligned(usize sz, usize alignment) -> __chunk<byte>
  {
    return abc::__abc_allocator<byte>::allocate_aligned(sz, alignment);
  }

  static void
  deallocate(T *ptr, usize sz)
  {
    if ( ptr == nullptr ) [[unlikely]]
      return;
    return abc::__abc_allocator<byte>::dealloc(ptr, sz);
  }

  static void
  dealloc(T *ptr)
  {
    abc::__abc_allocator<byte>::dealloc(ptr);
  }

  static void
  dealloc_aligned(T *ptr, usize alignment)
  {
    abc::__abc_allocator<byte>::dealloc_aligned(ptr, alignment);
  }

  static T *
  brk_allocate(usize sz)
  {
    return reinterpret_cast<T *>(abc::__abc_allocator<byte>::calloc(sz).ptr);
  }

  static void
  brk_deallocate(T *ptr, usize sz)
  {
    if ( ptr == nullptr ) [[unlikely]]
      return;
    return abc::__abc_allocator<byte>::dealloc(ptr, sz);
  }

  // unmanaged_* means "pages straight from the OS, you manage them" -- deliberately NOT the
  // allocator. micron::sys_allocator is abcmalloc's own raw-mmap helper (abcmalloc/__sys.hpp) and
  // does not exist under the barebones tier, where port::pages IS the page source. Same meaning,
  // same two calls, and it is the same pair the hosted non-abc arm below already uses.
  static T *
  unmanaged_allocate(usize sz)
  {
#if defined(__micron_bb_alloc)
    return reinterpret_cast<T *>(micron::port::page_alloc(sz).ptr);
#else
    return reinterpret_cast<T *>(micron::sys_allocator<byte>::alloc(sz));
#endif
  }

  static void
  unmanaged_deallocate(T *ptr, usize sz)
  {
    if ( ptr == nullptr ) [[unlikely]]
      return;
#if defined(__micron_bb_alloc)
    micron::port::page_free(micron::port::page_span{ reinterpret_cast<addr_t *>(ptr), sz });
#else
    return micron::sys_allocator<byte>::dealloc(ptr, sz);
#endif
  }
};
#else
// abcmalloc is not compiled into this build (MICRON_ABCMALLOC_DISABLE_STD). The same interface,
// backed by the system allocator, so every container above it still resolves. Hosted only -- the
// #error in __internal.hpp catches a freestanding build before it reaches here.
//
// unmanaged_* keeps its meaning: pages straight from the kernel, which is what port::pages is.
template<typename T> struct abc_allocator {
  static auto
  allocate(usize sz) -> __chunk<byte>
  {
    byte *p = micron::ptr_cast<byte *>(micron::__alloc(sz));
    if ( p == nullptr ) exc<except::memory_error>("abc_allocator::allocate(): system allocator failed");
    __builtin_memset(p, 0, sz);      // calloc semantics, matching the abcmalloc arm
    return __chunk<byte>{ p, sz };
  }

  static auto
  allocate_aligned(usize sz, usize alignment) -> __chunk<byte>
  {
    const usize rounded = (sz + alignment - 1) & ~(alignment - 1);
    byte *p = micron::ptr_cast<byte *>(::aligned_alloc(alignment, rounded));
    if ( p == nullptr ) exc<except::memory_error>("abc_allocator::allocate_aligned(): system allocator failed");
    __builtin_memset(p, 0, rounded);
    return __chunk<byte>{ p, rounded };
  }

  static void
  deallocate(T *ptr, usize)
  {
    micron::__free(ptr);
  }

  static void
  dealloc(T *ptr)
  {
    micron::__free(ptr);
  }

  static void
  dealloc_aligned(T *ptr, usize)
  {
    micron::__free(ptr);
  }

  static T *
  brk_allocate(usize sz)
  {
    return reinterpret_cast<T *>(allocate(sz).ptr);
  }

  static void
  brk_deallocate(T *ptr, usize)
  {
    micron::__free(ptr);
  }

  static T *
  unmanaged_allocate(usize sz)
  {
    return reinterpret_cast<T *>(micron::port::page_alloc(sz).ptr);
  }

  static void
  unmanaged_deallocate(T *ptr, usize sz)
  {
    if ( ptr == nullptr ) [[unlikely]]
      return;
    micron::port::page_free(micron::port::page_span{ reinterpret_cast<addr_t *>(ptr), sz });
  }
};
#endif

//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//

#ifdef MICRON_ALLOW_GLIBC_MALLOC
// TODO: legacy delete eventually
// default allocator, use malloc/free
template<typename T> class stl_allocator
{
public:
  constexpr stl_allocator() = default;
  constexpr stl_allocator(const stl_allocator &) = default;
  constexpr stl_allocator(stl_allocator &&) = default;

  static T *
  allocate(usize cnt)
  {
    if ( cnt > static_cast<usize>(-1) / sizeof(T) ) [[unlikely]]
      exc<except::memory_error>("stl_allocator: size overflow");
    const auto ptr = micron::__alloc(sizeof(T) * cnt);
    if ( !ptr ) exc<except::memory_error>("stl_allocator: allocation failed");
    return micron::ptr_cast<T *>(ptr);
  }

  static void
  deallocate(T *ptr, usize)
  {
    micron::__free(ptr);
  }

  friend bool
  operator==(const stl_allocator<T> &, const stl_allocator<T> &) noexcept
  {
    return true;
  }

  friend bool
  operator!=(const stl_allocator<T> &, const stl_allocator<T> &) noexcept
  {
    return false;
  }
};
#endif
};      // namespace micron
