//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// #include <sys/mman.h>

#include "../../memory/mman.h"

#include "../../except.hpp"
#include "../../types.hpp"
#include "bits.hpp"

namespace micron
{

constexpr size_t page_size = 4096;                      // generally is this on linux
constexpr size_t large_page_size = 2 * 1024 * 1024;     // likewise
constexpr size_t alloc_auto_sz = page_size;
constexpr size_t alloc_auto_large_sz = large_page_size;

template <typename T> using chunk = micron::__chunk<T>;

template <typename T>
chunk<byte>
to_chunk(T *ptr, size_t len)
{
  return chunk<byte>(reinterpret_cast<byte *>(ptr), len);
}

template <typename T = byte> struct mem_block : public chunk<T> {
  mem_block(const chunk<T> &o) : used(1), chunk<T>(o.ptr, o.len) {};
  mem_block(chunk<T> &&o) : used(1), chunk<T>(o.ptr, o.len) {};
  bool used;
};

inline void *
map_normal(void *ptr, const size_t n)
{
  return (micron::mmap(ptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
};

template <typename B = byte>
inline B *
map_frozen(void *ptr, const size_t n)
{
  return reinterpret_cast<B *>(micron::mmap(ptr, n, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
};

template <typename B = byte>
inline B *
map_large(void *ptr, const size_t n)
{
  return reinterpret_cast<B *>(
      micron::mmap(ptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0));
};

// base allocator which all others inherit from
template <typename T, size_t Sz = alloc_auto_sz>
  requires(micron::is_integral_v<T>)
struct std_allocator {
  inline static T *
  alloc(size_t n)     // allocate 'smartly'
  {
    void *ptr = nullptr;
    if constexpr ( Sz == page_size )
      ptr = micron::map_normal(nullptr, n);
    else if constexpr ( Sz == large_page_size )
      ptr = micron::map_large(nullptr, n);
    if ( ptr == (void *)-1 )
      throw except::memory_error("std_allocator::alloc(): mmap() failed");
    return reinterpret_cast<T*>(ptr);
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

// template <typename T = byte, size_t Sz = alloc_auto_sz>
// using std_allocator = __allocator<T, micron::map_normal, micron::map_large, micron::munmap, Sz>;

};     // namespace micron
