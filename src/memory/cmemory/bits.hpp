#pragma once

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

#include "../stack_constants.hpp"

#include "../../allocation/abcmalloc/malloc_forward.hpp"

namespace micron
{
auto get_stack(void) -> stack_t;
auto get_stack_start(void) -> addr_t *;
auto get_stack_size(void) -> size_t;

inline void
__mem_barrier() noexcept
{
  __asm__ __volatile__("" : : : "memory");
}

constexpr umax_t
broadcast_byte(byte b) noexcept
{
  if constexpr ( sizeof(umax_t) == 8 ) {
    return static_cast<umax_t>(b) * 0x0101010101010101ULL;
  } else if constexpr ( sizeof(umax_t) == 4 ) {
    return static_cast<umax_t>(b) * 0x01010101UL;
  } else {
    return static_cast<umax_t>(b);
  }
}
#define stackalloc(x, T) reinterpret_cast<T *>(__builtin_alloca(x));
template <typename F>
constexpr bool
__is_aligned_to(const F *ptr, const u64 alignment) noexcept
{
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
};

template <typename F>
constexpr bool
__is_aligned_to_r(const F &ref, const u64 alignment) noexcept
{
  return (reinterpret_cast<uintptr_t>(&ref) % alignment) == 0;
};

template <typename F>
constexpr bool
__is_aligned(const F *ptr) noexcept
{
  return (reinterpret_cast<uintptr_t>(ptr) % alignof(F)) == 0;
};

template <typename F>
constexpr bool
__is_aligned(const F &ref) noexcept
{
  return (reinterpret_cast<uintptr_t>(&ref) % alignof(F)) == 0;
};

template <typename F>
constexpr bool
__is_region_aligned_to(const F *ptr, const u64 size, const u64 alignment) noexcept
{
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0 && (size % alignment) == 0;
};

template <typename F>
constexpr bool
__is_aligned_to_s(const F *ptr, const u64 alignment) noexcept
{
  if ( ptr == nullptr )
    return false;
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
};

template <typename F>
constexpr bool
__is_aligned_s(const F *ptr) noexcept
{
  if ( ptr == nullptr )
    return false;
  return (reinterpret_cast<uintptr_t>(ptr) % alignof(F)) == 0;
};

template <typename F>
constexpr bool
__is_region_aligned_to_s(const F *ptr, const u64 size, const u64 alignment) noexcept
{
  if ( ptr == nullptr )
    return false;
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0 && (size % alignment) == 0;
};

// all of these functions slow you down a lot - be warned!

template <typename F>
bool
__is_at_stack(const F *ptr, const u64 size) noexcept
{
  const addr_t *addr = reinterpret_cast<const addr_t *>(ptr);
  stack_t st = get_stack();
  if ( (addr < st.start) or (addr + size >= st.start + st.size) )
    return false;
  return true;
}
template <typename F>
bool
__is_at_stack(F &ref, const u64 size) noexcept
{
  addr_t *addr = reinterpret_cast<addr_t *>(ref);
  stack_t st = get_stack();
  if ( (addr < st.start) or ((addr + size) >= (st.start + st.size)) )
    return false;
  return true;
}
template <typename F>
bool
__is_at_stack(const F &ref, const u64 size) noexcept
{
  addr_t *const addr = reinterpret_cast<addr_t *const>(ref);
  stack_t st = get_stack();
  if ( (addr < st.start) or (addr + size >= st.start + st.size) )
    return false;
  return true;
}

// rely on within for now
// TODO: implement bounds checking for non abcmalloc allocators

template <typename F>
bool
__is_at_heap(const F *ptr) noexcept
{
  return abc::within(reinterpret_cast<const addr_t *>(ptr));
}
template <typename F>
bool
__is_at_heap(F &ref) noexcept
{
  return abc::within(reinterpret_cast<addr_t *>(ref));
}

template <typename F>
bool
__is_at_heap(const F &ref) noexcept
{
  return abc::within(reinterpret_cast<addr_t *const>(ref));
}

template <typename F>
bool
__is_valid_address(const F *ptr, const u64 size) noexcept
{
  return (__is_at_stack(ptr, size) or __is_at_heap(ptr));
}

template <typename F>
bool
__is_valid_address(F &ref, const u64 size) noexcept
{
  return (__is_at_stack(ref, size) or __is_at_heap(ref));
}
template <typename F>
bool
__is_valid_address(const F &ref, const u64 size) noexcept
{
  return (__is_at_stack(ref, size) or __is_at_heap(ref));
}
};
