//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Mock allocators for test instrumentation. Each satisfies the contract at
// src/memory/allocation/allocator_types/__scheme.hpp: static auto_size,
// create(usize)->chunk<byte>, grow(chunk<byte>,usize)->chunk<byte>,
// destroy(const chunk<byte>). State is static (counters); use Tag template
// param to get disjoint counters across distinct tests in the same TU.

#include "../../src/except.hpp"
#include "../../src/memory/allocation/allocator_types/bits.hpp"
#include "../../src/memory/allocation/kmemory.hpp"
#include "../../src/memory/cmemory.hpp"

namespace mtest
{

// counts outstanding allocations: increments on create/grow, decrements on
// destroy. Test should assert outstanding() == 0 at end of scope to verify
// leak-freeness.
template<int Tag = 0> class tracking_allocator: private micron::abc_allocator<byte>
{
public:
  static inline usize allocations = 0;
  static inline usize deallocations = 0;
  static inline usize bytes_outstanding = 0;
  static inline usize peak_bytes = 0;

  ~tracking_allocator() = default;
  tracking_allocator() = default;
  tracking_allocator(const tracking_allocator &) = default;
  tracking_allocator(tracking_allocator &&) = default;
  tracking_allocator &operator=(const tracking_allocator &) = default;
  tracking_allocator &operator=(tracking_allocator &&) = default;

  static i64
  outstanding(void) noexcept
  {
    return static_cast<i64>(allocations) - static_cast<i64>(deallocations);
  }

  static void
  reset(void) noexcept
  {
    allocations = 0;
    deallocations = 0;
    bytes_outstanding = 0;
    peak_bytes = 0;
  }

  static constexpr usize
  auto_size(void)
  {
    return 256;
  }

  static micron::chunk<byte>
  create(usize n)
  {
    auto mem = allocate(n);
    if ( mem.ptr != nullptr ) {
      ++allocations;
      bytes_outstanding += mem.len;
      if ( bytes_outstanding > peak_bytes ) peak_bytes = bytes_outstanding;
    }
    return mem;
  }

  static micron::chunk<byte>
  grow(micron::chunk<byte> memory, usize n)
  {
    if ( memory.len == 0 and memory.ptr == nullptr ) {
      auto mem = allocate(n * 2);
      if ( mem.ptr != nullptr ) {
        ++allocations;
        bytes_outstanding += mem.len;
        if ( bytes_outstanding > peak_bytes ) peak_bytes = bytes_outstanding;
      }
      return mem;
    }
    const usize req = (n > memory.len * 2) ? n : memory.len * 2;
    auto mem = allocate(req);
    if ( mem.ptr == nullptr ) return mem;
    micron::memcpy(mem.ptr, memory.ptr, memory.len);
    ++allocations;
    bytes_outstanding += mem.len;
    if ( bytes_outstanding > peak_bytes ) peak_bytes = bytes_outstanding;
    destroy(memory);
    return mem;
  }

  static void
  destroy(const micron::chunk<byte> mem)
  {
    if ( mem.ptr == nullptr ) return;
    ++deallocations;
    bytes_outstanding -= mem.len;
    deallocate(mem.ptr, mem.len);
  }

  byte *share(void) = delete;

  static i16
  get_grow(void)
  {
    return 2;
  }
};

// throws except::memory_error after the (trip+1)-th allocation attempt
// (i.e. trip=0 throws on first allocate). Use arm(n) to (re)configure.
template<int Tag = 0> class throwing_allocator: private micron::abc_allocator<byte>
{
public:
  static inline int trip = -1;      // -1 = never throw
  static inline int call_count = 0;
  static inline usize allocations = 0;
  static inline usize deallocations = 0;

  ~throwing_allocator() = default;
  throwing_allocator() = default;
  throwing_allocator(const throwing_allocator &) = default;
  throwing_allocator(throwing_allocator &&) = default;
  throwing_allocator &operator=(const throwing_allocator &) = default;
  throwing_allocator &operator=(throwing_allocator &&) = default;

  static void
  arm(int trip_count) noexcept
  {
    trip = trip_count;
    call_count = 0;
  }

  static void
  disarm(void) noexcept
  {
    trip = -1;
    call_count = 0;
  }

  static void
  reset(void) noexcept
  {
    trip = -1;
    call_count = 0;
    allocations = 0;
    deallocations = 0;
  }

  static i64
  outstanding(void) noexcept
  {
    return static_cast<i64>(allocations) - static_cast<i64>(deallocations);
  }

  static constexpr usize
  auto_size(void)
  {
    return 256;
  }

  static micron::chunk<byte>
  create(usize n)
  {
    if ( trip >= 0 and call_count++ == trip ) {
      micron::exc<micron::except::memory_error>("throwing_allocator: tripped on create");
    }
    auto mem = allocate(n);
    if ( mem.ptr != nullptr ) ++allocations;
    return mem;
  }

  static micron::chunk<byte>
  grow(micron::chunk<byte> memory, usize n)
  {
    if ( trip >= 0 and call_count++ == trip ) {
      micron::exc<micron::except::memory_error>("throwing_allocator: tripped on grow");
    }
    if ( memory.len == 0 and memory.ptr == nullptr ) {
      auto mem = allocate(n * 2);
      if ( mem.ptr != nullptr ) ++allocations;
      return mem;
    }
    const usize req = (n > memory.len * 2) ? n : memory.len * 2;
    auto mem = allocate(req);
    if ( mem.ptr == nullptr ) return mem;
    micron::memcpy(mem.ptr, memory.ptr, memory.len);
    ++allocations;
    destroy(memory);
    return mem;
  }

  static void
  destroy(const micron::chunk<byte> mem)
  {
    if ( mem.ptr == nullptr ) return;
    ++deallocations;
    deallocate(mem.ptr, mem.len);
  }

  byte *share(void) = delete;

  static i16
  get_grow(void)
  {
    return 2;
  }
};

};      // namespace mtest
