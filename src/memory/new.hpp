//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../types.hpp"

#include "../allocation/__internal.hpp"

// #define ALLOCATOR_DEBUG 1
#ifdef ALLOCATOR_DEBUG
#include "../io/console.hpp"
#define ALLOC_MESSAGE(x, ...)                                                                                           \
  if constexpr ( __micron_global__alloc_debug == true ) {                                                               \
    micron::_micron_log(__FILE__, __LINE__, x, ##__VA_ARGS__);                                                          \
  }
#else
#define ALLOC_MESSAGE(x, ...)
#endif

#ifdef ALLOCATOR_DEBUG
constexpr static const bool __micron_global__alloc_debug = true;
#else
constexpr static const bool __micron_global__alloc_debug = false;
#endif

// TODO: add C++17 aligning overloads

/*
template <typename... Args>
void *
operator new(size_t size, Args &&...args)
{
  (void)sizeof...(args); // suppress unused warning
  ALLOC_MESSAGE("new args(", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  throw micron::except::memory_error("micron::operator new(): micron::__alloc failed");
}
template <typename... Args>
void *
operator new[](size_t size, Args &&...args)
{
  (void)sizeof...(args); // suppress unused warning
  ALLOC_MESSAGE("new args[](", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  throw micron::except::memory_error("micron::operator new[]: micron::__alloc failed");
}
*/
// leave these as void
void *
operator new(size_t size)
{
  ALLOC_MESSAGE("new(", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  throw micron::except::memory_error("micron::operator new(): micron::__alloc failed");
}

template <typename P>
void *
operator new(size_t size, P *ptr)
{
  (void)size;
  ALLOC_MESSAGE("new(", size, ")");
  ALLOC_MESSAGE("at(", ptr, ")");
  return ptr;
}

void *
operator new[](size_t size)
{
  ALLOC_MESSAGE("new[](", size, ")");
  if ( void *ptr = micron::__alloc(size) ) {
    ALLOC_MESSAGE("returning(", ptr, ")");
    return ptr;
  }
  throw micron::except::memory_error("micron::operator new[]: micron::__alloc failed");
}

void
operator delete(void *ptr) noexcept
{
  ALLOC_MESSAGE("delete(", ptr, ")");
  micron::__free(ptr);
}

void
operator delete(void *ptr, size_t size) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete(", ptr, ") size of ", size);
  micron::__free(ptr);
}

void
operator delete[](void *ptr) noexcept
{
  ALLOC_MESSAGE("delete[](", ptr, ")");
  micron::__free(ptr);
}

void
operator delete[](void *ptr, size_t size) noexcept
{
  (void)size;
  ALLOC_MESSAGE("delete[](", ptr, ") size of ", size);
  micron::__free(ptr);
}

namespace micron
{
template <typename Type, typename... Args>
inline __attribute__((always_inline)) Type *
__new(Args &&...args)
{
  return new Type(micron::forward<Args&&>(args)...);
}
template <typename Type, typename... Args>
inline __attribute__((always_inline)) Type *
__new_arr(Args &&...args)
{
  return new Type[sizeof...(args)](micron::forward<Args>(args)...);
}

template <typename T>
inline __attribute__((always_inline)) void
__delete(T *&ptr)
{
  delete ptr;
  ptr = nullptr;
}

template <typename T>
inline __attribute__((always_inline)) void
__const_delete(const T *const &ptr)
{
  delete ptr;
}
template <typename T>
inline __attribute__((always_inline)) void
__delete_arr(T *&ptr)
{
  delete[] ptr;
  ptr = nullptr;
}
};
