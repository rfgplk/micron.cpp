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
#include "../io/console.hpp"
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

/*
// NOTE:cpp aligned allocs really want align_val_t to be in the std namespace so this is the best workaround i could come up with without
including <new>. may cause compilation options if you include <new> AFTER this file, since theres no reliable way to stop <new> from
including align_val_t #ifndef __cpp_aligned_new namespace std
{
enum class align_val_t : size_t {};
};
#endif
*/
// §17.6.3 — scalar new/delete

[[nodiscard]] void *
operator new(usize size)
{
  ALLOC_MESSAGE("new(", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  micron::exc<micron::except::memory_error>("micron::operator new(): micron::__alloc failed");
}

[[nodiscard]] void *
operator new[](usize size)
{
  ALLOC_MESSAGE("new[](", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  micron::exc<micron::except::memory_error>("micron::operator new[](): micron::__alloc failed");
}

void
operator delete(void *ptr) noexcept
{
  ALLOC_MESSAGE("delete(", ptr, ")");
  micron::__free(ptr);
}

void
operator delete[](void *ptr) noexcept
{
  ALLOC_MESSAGE("delete[](", ptr, ")");
  micron::__free(ptr);
}

void
operator delete(void *ptr, usize size) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete(", ptr, ") size of ", size);
  micron::__free(ptr);
}

void
operator delete[](void *ptr, usize size) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete[](", ptr, ") size of ", size);
  micron::__free(ptr);
}

/*
 * TODO: introduce align_val variants eventually
 *
 */
namespace micron
{
template <typename Type, typename... Args>
inline __attribute__((always_inline)) Type *
__new(Args &&...args)
{
  return new Type(micron::forward<Args &&>(args)...);
}

template <typename Type>
inline __attribute__((always_inline)) auto
__new_arr(usize n)
{
  return new Type[n];
}

template <typename T>
inline __attribute__((always_inline)) void
__delete(T *ptr)
{
  delete ptr;
  ptr = nullptr;
}

template <typename T>
inline __attribute__((always_inline)) void
__const_delete(const T *const ptr)
{
  delete ptr;
}

template <typename T>
inline __attribute__((always_inline)) void
__const_delete_arr(const T *ptr)
{
  delete[] ptr;
  ptr = nullptr;
}

template <typename T>
inline __attribute__((always_inline)) void
__delete_arr(T *ptr)
{
  delete[] ptr;
  ptr = nullptr;
}
};     // namespace micron
