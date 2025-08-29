#pragma once

#include "../linux/kmemory.hpp"

#include "../../except.hpp"
#include "../../type_traits.hpp"

namespace micron
{

// base allocator which all others inherit from
template <typename T, size_t Sz = alloc_auto_sz>
  requires(micron::is_integral_v<T>)
struct std_allocator {
  inline static T *
  alloc(size_t n)     // allocate 'smartly'
  {
    T *ptr = nullptr;
    if constexpr ( Sz == page_size )
      ptr = micron::map_normal(nullptr, n);
    else if constexpr ( Sz == large_page_size )
      ptr = micron::map_large(nullptr, n);
    if ( ptr == (void *)-1 )
      throw except::memory_error("std_allocator::alloc(): mmap() failed");
    return ptr;
  };

  inline static void
  dealloc(T *mem, size_t len)
  {     // deallocate at location N
    if ( mem == nullptr )
      throw except::memory_error("std_allocator::dealloc(): nullptr was provided");
    // micron::munmap(mem, len);
    if ( micron::munmap(mem, len) == -1 )
      throw except::memory_error("std_allocator::dealloc(): munmap() failed");
    mem = nullptr;
  }
};
};
