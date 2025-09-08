//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// #include <sys/mman.h>

#include "../../memory/mman.hpp"

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

template <typename B = byte>
inline B *
map_normal(addr_t *ptr, const size_t n)
{
  return reinterpret_cast<B *>(micron::mmap(ptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
};

template <typename B = byte>
inline B *
map_frozen(addr_t *ptr, const size_t n)
{
  return reinterpret_cast<B *>(micron::mmap(ptr, n, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
};

template <typename B = byte>
inline B *
map_large(addr_t *ptr, const size_t n)
{
  return reinterpret_cast<B *>(
      micron::mmap(ptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0));
};
};     // namespace micron
