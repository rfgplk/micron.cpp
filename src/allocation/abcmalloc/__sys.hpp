#pragma once

#include "../linux/kmemory.hpp"

#include "../../errno.hpp"
#include "../../except.hpp"
#include "../../type_traits.hpp"

namespace micron
{

inline bool
__validate_mmap_addr(addr_t *ptr)
{
  if ( reinterpret_cast<uintptr_t>(ptr) == -static_cast<uintptr_t>(error::out_of_memory) )
    return false;
  if ( reinterpret_cast<uintptr_t>(ptr) == -static_cast<uintptr_t>(error::overflow) )
    return false;
  if ( reinterpret_cast<uintptr_t>(ptr) == -static_cast<uintptr_t>(error::invalid_arg) )
    return false;
  if ( reinterpret_cast<uintptr_t>(ptr) == -static_cast<uintptr_t>(error::file_exists) )
    return false;
  if ( reinterpret_cast<uintptr_t>(ptr) == -static_cast<uintptr_t>(error::permissions) )
    return false;
  if ( reinterpret_cast<uintptr_t>(ptr) == -static_cast<uintptr_t>(error::try_again) )
    return false;
  return true;
}

// base allocator which all others inherit from
template <typename T, size_t Sz = alloc_auto_sz>
  requires(micron::is_integral_v<T>)
struct sys_allocator {
  static T *
  alloc(size_t n)     // allocate 'smartly'
  {
    // NOTE: from mm/mmap.c - do_mmap
    // mmap will return either, -ENOMEM, -EOVERFLOW, -EINVAL, -EEXIST, -EPERM, -EAGAIN, -EOVERFLOW; depending on
    // fail/error condition. not a raw -1 (this is a libc reconciliation).
    addr_t *ptr = nullptr;
    if constexpr ( Sz == page_size )
      ptr = micron::map_normal(nullptr, n);
    else if constexpr ( Sz == large_page_size )
      ptr = micron::map_large(nullptr, n);
    if ( !__validate_mmap_addr(ptr) )
      throw except::memory_error("sys_allocator::alloc(): mmap() failed");

    if ( micron::madvise(ptr, n, micron::madv_dontneed) != 0 ) {
    }

    return reinterpret_cast<T *>(ptr);
  };

  static void
  dealloc(T *mem, size_t len)
  {     // deallocate at location N
    if ( mem == nullptr )
      throw except::memory_error("sys_allocator::dealloc(): nullptr was provided");
    // micron::munmap(mem, len);
    if ( micron::munmap(mem, len) == -1 )
      throw except::memory_error("sys_allocator::dealloc(): munmap() failed");
    mem = nullptr;
  }
};
};
