//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_secure
//
// use for secrets that must stay out of swap, core dumps, and forked children

namespace micron
{

inline void
__allocation_secure_zero(byte *memory, usize bytes) noexcept
{
  volatile byte *out = memory;
  for ( usize i = 0; i < bytes; ++i ) out[i] = 0;
  __asm__ __volatile__("" : : "r"(memory) : "memory");
}

template<is_policy P = serial_allocation_policy> class allocator_secure
{
public:
  static constexpr bool allocator_trusted = true;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return page_size;
  }

  [[nodiscard]] static usize
  allocation_extent(usize bytes, usize)
  {
    return allocation_round_up_or_throw(__allocation_policy_capacity<P>(bytes), page_size);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    const usize capacity = allocation_round_up_or_throw(__allocation_policy_capacity<P>(bytes), page_size);
    if ( capacity == 0 ) return { nullptr, 0 };
    chunk<byte> memory = __map_create_aligned(capacity, alignment);
    addr_t *address = reinterpret_cast<addr_t *>(memory.ptr);
    if ( micron::mlock(address, memory.len) != 0 ) {
      micron::munmap(address, memory.len);
      exc<except::memory_error>("allocator_secure: mlock failed");
    }
    if ( micron::madvise(address, memory.len, madv_dontdump) != 0 || micron::madvise(address, memory.len, madv_dontfork) != 0 ) {
      __allocation_secure_zero(memory.ptr, memory.len);
      micron::munlock(address, memory.len);
      micron::munmap(address, memory.len);
      exc<except::memory_error>("allocator_secure: required madvise failed");
    }
    if ( !abc::register_external(memory.ptr, memory.len, abc::external_provenance::secure) ) {
      __allocation_secure_zero(memory.ptr, memory.len);
      micron::munlock(address, memory.len);
      micron::munmap(address, memory.len);
      exc<except::memory_error>("allocator_secure: provenance registration failed");
    }
    return memory;
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create(bytes, 16);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    chunk<byte> next = create(bytes, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied != 0 ) micron::memcpy(next.ptr, old.ptr, copied);
    destroy(old, alignment);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("allocator_secure: growth overflow");
    return resize(old, target, old.len, 16);
  }

  static void
  destroy(chunk<byte> memory, usize) noexcept
  {
    if ( memory.ptr == nullptr ) return;
    (void)abc::unregister_external(memory.ptr, memory.len);
    __allocation_secure_zero(memory.ptr, memory.len);
    micron::munlock(reinterpret_cast<addr_t *>(memory.ptr), memory.len);
    micron::munmap(reinterpret_cast<addr_t *>(memory.ptr), memory.len);
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    destroy(memory, 16);
  }

  [[nodiscard]] static constexpr usize
  recommend(usize current, usize minimum) noexcept
  {
    return __allocation_policy_recommend<P>(current, minimum);
  }

  [[nodiscard]] static constexpr f32
  get_grow() noexcept
  {
    return P::on_grow;
  }

  byte *share(void) = delete;
};

};      // namespace micron
