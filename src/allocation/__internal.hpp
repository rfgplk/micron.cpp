//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "abcmalloc/malloc.hpp"

#ifndef MICRON_USE_ABCMALLOC
#include <cstdlib>
#endif
namespace micron
{
#ifdef MICRON_USE_ABCMALLOC
inline __attribute__((always_inline)) byte *
__alloc(size_t sz)
{
  return abc::alloc(sz);
}
template <typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  abc::dealloc(reinterpret_cast<byte *>(ptr));
}
#else
inline __attribute__((always_inline)) void *
__alloc(size_t sz)
{
  return ::malloc(sz);
}
template <typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  ::free(ptr);
}
#endif
};
