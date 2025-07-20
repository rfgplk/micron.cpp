//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

//#include <sys/mman.h>

#include "../memory/mman.h"

#include "../except.hpp"
#include "../types.hpp"

namespace micron
{

constexpr size_t page_size = 4096;                      // generally is this on linux
constexpr size_t large_page_size = 2 * 1024 * 1024;     // likewise
constexpr size_t alloc_auto_sz = page_size;
constexpr size_t alloc_auto_large_sz = large_page_size;

template <typename T = byte> struct chunk {     // total memory allocated
  T *ptr;
  size_t len;
};

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

template <typename B = byte>
inline B *
map_normal(void *ptr, const size_t n)
{
  return reinterpret_cast<B *>(micron::mmap(ptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
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
  return reinterpret_cast<B *>(micron::mmap(ptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0));
};
template <typename T = byte> class deleter
{
  // using allocator_type = typename std::remove_reference<Alloc>::type;
public:
  deleter() = default;
  deleter(const deleter &) = default;
  deleter(deleter &&) = default;
  deleter &operator=(const deleter &) = default;
  deleter &operator=(deleter &&) = default;

  void
  dealloc(T *mem, size_t len)
  {     // deallocate at location N
    if ( mem == nullptr )
      return;
    micron::munmap(mem, len);
    if ( micron::munmap(mem, len) == -1 )
      throw except::memory_error("Unmapping failed");
    mem = nullptr;
  }
};

// base allocator which all others inherit from
template <typename T, size_t PS = alloc_auto_sz> class allocator : public deleter<byte>
{
public:
  allocator() = default;
  allocator(const allocator &) = default;
  allocator(allocator &&) = default;
  allocator &operator=(const allocator &) = default;
  allocator &operator=(allocator &&) = default;

  T *
  alloc(size_t n)     // allocate 'smartly'
  {
    T *ptr = nullptr;
    if constexpr ( PS == page_size )
      ptr = map_normal(nullptr, n);
    else if constexpr ( PS == large_page_size )
      ptr = map_large(nullptr, n);
    if ( ptr == MAP_FAILED )
      throw except::memory_error("Memory allocation failed.");
    return ptr;
  };
};

};     // namespace micron
