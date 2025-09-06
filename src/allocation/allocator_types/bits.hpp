//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// default micron allocator, uses abcmalloc directly
template <typename T> struct abc_allocator
{
  // difference between allocate and umanaged_* calls is that allocate pulls memory from the allocator, while unmanaged
  // pulls pages from the kernel directly, for when you need to manage memory yourself
  T *
  allocate(size_t sz)
  {
    return reinterpret_cast<T *>(abc::__abc_allocator<byte>::alloc(sz));
  }
  void
  deallocate(T *ptr, size_t sz)
  {
    return abc::__abc_allocator<byte>::dealloc(ptr, sz);
  }
  T *
  unmanaged_allocate(size_t sz)
  {
    return reinterpret_cast<T *>(micron::sys_allocator<byte>::alloc(sz));
  }
  void
  unmanaged_deallocate(T *ptr, size_t sz)
  {
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
template <typename T> class stl_allocator
{
  constexpr stl_allocator() = default;
  constexpr stl_allocator(const stl_allocator &) = default;
  constexpr stl_allocator(stl_allocator &&) = default;
  T *
  allocate(size_t cnt)
  {
    const auto ptr = micron::__alloc(sizeof(T) * cnt);     // std::malloc(sizeof(T) * cnt);
    if ( !ptr )
      throw except::memory_error();
    return static_cast<T *>(ptr);
  }
  void
  deallocate(T *ptr, size_t cnt)
  {
    micron::__free(ptr);
  }
  friend bool
  operator==(const stl_allocator<T> &a, const stl_allocator<T> &b)
  {
    return true;
  }
  friend bool
  operator!=(const stl_allocator<T> &a, const stl_allocator<T> &b)
  {
    return false;
  }
};
#endif
};
