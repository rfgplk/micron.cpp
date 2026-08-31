//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../except.hpp"
#include "../types.hpp"
#include "actions.hpp"
#include "allocation/__internal.hpp"
// #define ALLOCATOR_DEBUG 1
#ifdef ALLOCATOR_DEBUG
#define ALLOC_MESSAGE(x, ...)                                                                                                              \
  if constexpr ( __micron_global__alloc_debug == true ) {                                                                                  \
    micron::_micron_log(__FILE__, __LINE__, x, ##__VA_ARGS__);                                                                             \
  }
#else
#define ALLOC_MESSAGE(x, ...)
#endif
#ifdef ALLOCATOR_DEBUG
constexpr static const bool __micron_global__alloc_debug = true;
#else
constexpr static const bool __micron_global__alloc_debug = false;
#endif

// Clang does not inject this compiler-required type until <new> is included. Micron cannot
// include that STL header, so provide the language ABI declaration itself.
#if defined(__clang__) && !defined(_NEW) && !defined(_LIBCPP_NEW) && !defined(_LIBCPP___NEW)
namespace std
{
enum class align_val_t : usize { };
};
#endif
#if !defined(__micron_sanitizer_owns_heap)

// A hosted allocator interposer must see the matching replaceable new/delete pair. When the
// standard backend is libc, inlining only delete turns Valgrind's new allocation into a direct
// free and defeats that pairing. The abc-native path keeps its normal optimizer visibility.
#if !defined(__micron_abcmalloc_std_backend)
#define __MICRON_ALLOC_INTERPOSE __attribute__((noinline))
#else
#define __MICRON_ALLOC_INTERPOSE
#endif

// §17.6.3 — scalar new/delete

[[nodiscard]] __MICRON_ALLOC_INTERPOSE void *
operator new(usize size)
{
  ALLOC_MESSAGE("new(", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  micron::exc<micron::except::memory_error_new_scalar>("micron::operator new(): micron::__alloc failed");
}

[[nodiscard]] __MICRON_ALLOC_INTERPOSE void *
operator new[](usize size)
{
  ALLOC_MESSAGE("new[](", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  micron::exc<micron::except::memory_error_new_array>("micron::operator new[](): micron::__alloc failed");
}

__MICRON_ALLOC_INTERPOSE void
operator delete(void *ptr) noexcept
{
  ALLOC_MESSAGE("delete(", ptr, ")");
  micron::__free(ptr);
}

__MICRON_ALLOC_INTERPOSE void
operator delete[](void *ptr) noexcept
{
  ALLOC_MESSAGE("delete[](", ptr, ")");
  micron::__free(ptr);
}

__MICRON_ALLOC_INTERPOSE void
operator delete(void *ptr, usize size) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete(", ptr, ") size of ", size);
  micron::__free(ptr);
}

__MICRON_ALLOC_INTERPOSE void
operator delete[](void *ptr, usize size) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete[](", ptr, ") size of ", size);
  micron::__free(ptr);
}

// §17.6.3 — aligned scalar/array new and delete
//
// abc::aligned_alloc routes to abc::alloc() when alignment <= 32 (__hdr_offset) and to a shifted aligned-pointer scheme when alignment > 32
//
// NOTE: std::align_val_t is implicitly defined by the compiler under -faligned-new (default for -std=c++17 and later), so we don't need new

namespace micron
{
namespace __aligned_new
{
[[nodiscard, gnu::always_inline]] inline void *
__do_alloc(usize size, usize align)
{
  const usize padded = (size + align - 1) & ~(align - 1);
#if defined(__micron_abcmalloc_std_backend)
  return abc::aligned_alloc(align, padded ? padded : align);
#else
  return ::aligned_alloc(align, padded ? padded : align);
#endif
}

[[gnu::always_inline]] inline void
__do_free(void *ptr, usize align)
{
#if defined(__micron_abcmalloc_std_backend)
  // WARNING: the bound is native_block_alignment, NOT a literal 32 -- under
  // MICRON_ABC_REDZONE it is 16, so a 32-aligned new took the prefix path while
  // this took the raw one and the block start was never released
  if ( align <= abc::native_block_alignment ) {
    micron::__free(ptr);      // alignment fits in the abc header offset, raw alloc was used
  } else {
    abc::aligned_free(ptr);
  }
#else
  (void)align;
  micron::__free(ptr);
#endif
}
};      // namespace __aligned_new
};      // namespace micron

[[nodiscard]] __MICRON_ALLOC_INTERPOSE void *
operator new(usize size, std::align_val_t al)
{
  ALLOC_MESSAGE("new(", size, ", align=", static_cast<usize>(al), ")");
  if ( void *ptr = micron::__aligned_new::__do_alloc(size, static_cast<usize>(al)) ) return ptr;
  micron::exc<micron::except::memory_error_new_scalar_aligned>("micron::operator new(align): aligned_alloc failed");
}

[[nodiscard]] __MICRON_ALLOC_INTERPOSE void *
operator new[](usize size, std::align_val_t al)
{
  ALLOC_MESSAGE("new[](", size, ", align=", static_cast<usize>(al), ")");
  if ( void *ptr = micron::__aligned_new::__do_alloc(size, static_cast<usize>(al)) ) return ptr;
  micron::exc<micron::except::memory_error_new_array_aligned>("micron::operator new[](align): aligned_alloc failed");
}

__MICRON_ALLOC_INTERPOSE void
operator delete(void *ptr, std::align_val_t al) noexcept
{
  ALLOC_MESSAGE("delete(", ptr, ", align=", static_cast<usize>(al), ")");
  micron::__aligned_new::__do_free(ptr, static_cast<usize>(al));
}

__MICRON_ALLOC_INTERPOSE void
operator delete[](void *ptr, std::align_val_t al) noexcept
{
  ALLOC_MESSAGE("delete[](", ptr, ", align=", static_cast<usize>(al), ")");
  micron::__aligned_new::__do_free(ptr, static_cast<usize>(al));
}

__MICRON_ALLOC_INTERPOSE void
operator delete(void *ptr, usize size, std::align_val_t al) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete(", ptr, ") size=", size, ", align=", static_cast<usize>(al));
  micron::__aligned_new::__do_free(ptr, static_cast<usize>(al));
}

__MICRON_ALLOC_INTERPOSE void
operator delete[](void *ptr, usize size, std::align_val_t al) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete[](", ptr, ") size=", size, ", align=", static_cast<usize>(al));
  micron::__aligned_new::__do_free(ptr, static_cast<usize>(al));
}

#undef __MICRON_ALLOC_INTERPOSE
#endif      // !__micron_sanitizer_owns_heap

namespace micron
{
template<typename Type, typename... Args>
inline __attribute__((always_inline)) Type *
__new(Args &&...args)
{
  return new Type(micron::forward<Args &&>(args)...);
}

template<typename Type>
inline __attribute__((always_inline)) auto
__new_arr(usize n)
{
  return new Type[n];
}

template<typename T>
inline __attribute__((always_inline)) void
__delete(T *ptr)
{
  delete ptr;
  ptr = nullptr;
}

template<typename T>
inline __attribute__((always_inline)) void
__const_delete(const T *const ptr)
{
  delete ptr;
}

template<typename T>
inline __attribute__((always_inline)) void
__const_delete_arr(const T *ptr)
{
  delete[] ptr;
  ptr = nullptr;
}

template<typename T>
inline __attribute__((always_inline)) void
__delete_arr(T *ptr)
{
  delete[] ptr;
  ptr = nullptr;
}
};      // namespace micron
