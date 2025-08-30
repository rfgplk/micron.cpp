//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

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

// default micron allocator, uses abcmalloc directly
template <typename T> class abc_allocator : private abc::__abc_allocator<byte>
{
public:
  constexpr abc_allocator() = default;
  constexpr abc_allocator(const abc_allocator &) = default;
  constexpr abc_allocator(abc_allocator &&) = default;
  T *
  allocate(size_t sz)
  { 
    return abc::__abc_allocator<byte>::alloc(sz);
  }
  void
  deallocate(T *ptr, size_t sz)
  {
    return abc::__abc_allocator<byte>::dealloc(ptr, sz);
  }
};

};
