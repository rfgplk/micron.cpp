//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// mmap allocator, bypasses malloc
template <is_policy P = serial_allocation_policy> class map_allocator : private sys_allocator<byte>
{
public:
  ~map_allocator() = default;
  map_allocator() = default;
  map_allocator(const map_allocator &o) = default;
  map_allocator(map_allocator &&o) = default;
  map_allocator &operator=(const map_allocator &o) = default;
  map_allocator &operator=(map_allocator &&o) = default;

  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return page_size;
  }

  static chunk<byte>
  create(size_t n)
  {
    n = to_page(n);
    return { sys_allocator<byte>::alloc(n), n };
  }

  static chunk<byte>
  grow(chunk<byte> memory, size_t n)
  {
    // NOTE: in case we somehow provide zerod out mem
    if ( memory.len == 0 and memory.ptr == nullptr ) {
      n = to_page(n);
      n = static_cast<size_t>(static_cast<f32>(n) * static_cast<f32>(P::on_grow));
      chunk<byte> mem{ sys_allocator<byte>::alloc(n), n };
      return mem;
    }
    n = to_page(n);

    f32 n_f = static_cast<f32>(n);
    f32 len_f = static_cast<f32>(memory.len);
    f32 result_f = (n_f / len_f < P::on_grow) ? P::on_grow * len_f : (n_f / len_f) * len_f;
    n = static_cast<size_t>(result_f);
    chunk<byte> mem{ sys_allocator<byte>::alloc(n), n };
    micron::memcpy(mem.ptr, memory.ptr, memory.len);
    destroy(memory);
    return mem;
  }

  static void
  destroy(const chunk<byte> mem)
  {
    sys_allocator<byte>::dealloc(mem.ptr, mem.len);
  }

  byte *share(void) = delete;

  static i16
  get_grow()
  {
    return P::on_grow;
  }
};

};     // namespace micron
