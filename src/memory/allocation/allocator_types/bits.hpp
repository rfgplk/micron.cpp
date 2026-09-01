//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../cmalloc.hpp"

#include "../../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// low-level allocator adapters

namespace micron
{

// default micron allocator, uses abcmalloc directly
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

  static T *
  unmanaged_allocate(usize sz)
  {
    return reinterpret_cast<T *>(micron::sys_allocator<byte>::alloc(sz));
  }

  static void
  unmanaged_deallocate(T *ptr, usize sz)
  {
    if ( ptr == nullptr ) [[unlikely]]
      return;
    return micron::sys_allocator<byte>::dealloc(ptr, sz);
  }
};

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
