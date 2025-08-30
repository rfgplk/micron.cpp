//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// default micron allocator, uses mmap
template <typename T, size_t Sz> class map_allocator : private std_allocator<byte, Sz>
{
public:
  constexpr map_allocator() = default;
  constexpr map_allocator(const map_allocator &) = default;
  constexpr map_allocator(map_allocator &&) = default;
  T *
  allocate(size_t cnt)
  {
    return std_allocator<byte, Sz>::alloc(cnt);
  }
  void
  deallocate(T *ptr, size_t cnt)
  {
    return std_allocator<byte, Sz>::dealloc(ptr, cnt);
  }
};

};
