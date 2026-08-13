//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// integer arithmetic now
template<is_policy P>
inline constexpr usize
__grow_scale(usize len) noexcept
{
  constexpr usize num = static_cast<usize>(P::on_grow * 4.0f + 0.5f);      // 3.0 -> 12, 1.5 -> 6
  constexpr usize den = 4;
  static_assert(num > den, "growth policy must exceed 1.0 or a vector never grows");
  if ( len == 0 ) return 0;
  // saturate rather than wrap
  if ( len > (static_cast<usize>(-1) / num) ) return static_cast<usize>(-1);
  return (len * num) / den;
}

// serial standard allocator, cannot be mempooled, default *tripling* policy
template<is_policy P = serial_allocation_policy> class allocator_serial: private abc_allocator<byte>
{

public:
  ~allocator_serial() = default;
  allocator_serial() = default;
  allocator_serial(const allocator_serial &o) = default;
  allocator_serial(allocator_serial &&o) = default;
  allocator_serial &operator=(const allocator_serial &o) = default;
  allocator_serial &operator=(allocator_serial &&o) = default;

  inline __attribute__((always_inline)) static constexpr usize
  auto_size()
  {
    return P::granularity;
  }

  static chunk<byte>
  create(usize n)
  {
    n = to_granularity<P::granularity>(n);
    return allocate(n);      // create the block, the handler is responsible for calling destroy
  }

  static chunk<byte>
  grow(chunk<byte> memory, usize n)
  {
    // NOTE: in case we somehow provide zerod out mem
    if ( memory.len == 0 and memory.ptr == nullptr ) {
      n = to_page(n);
      n = __grow_scale<P>(n);
      chunk<byte> mem = allocate(n);
      return mem;
    }
    n = to_granularity<P::granularity>(n);
    // integer math
    const usize scaled = __grow_scale<P>(memory.len);
    if ( n < scaled ) n = scaled;
    chunk<byte> mem = allocate(n);
    micron::memcpy(mem.ptr, memory.ptr, memory.len);
    destroy(memory);
    return mem;
  }

  static void
  destroy(const chunk<byte> mem)
  {
    deallocate(mem.ptr, mem.len);
  }

  byte *share(void) = delete;

  static i16
  get_grow()
  {
    return P::on_grow;
  }
};

};      // namespace micron
