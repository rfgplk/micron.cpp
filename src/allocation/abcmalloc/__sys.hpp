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
      exc<except::memory_error>("sys_allocator::alloc(): mmap() failed");

    if ( micron::madvise(ptr, n, micron::madv_dontneed) != 0 ) {
    }

    return reinterpret_cast<T *>(ptr);
  };

  static void
  dealloc(T *mem, size_t len)
  {     // deallocate at location N
    if ( mem == nullptr )
      exc<except::memory_error>("sys_allocator::dealloc(): nullptr was provided");
    // micron::munmap(mem, len);
    if ( micron::munmap(mem, len) == -1 )
      exc<except::memory_error>("sys_allocator::dealloc(): munmap() failed");
    mem = nullptr;
  }
};
};
