//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../cmalloc.hpp"
#include "../../types.hpp"

#if !defined(MICRON_ABCMALLOC_STD) || defined(__micron_sanitizer_owns_heap)
/*permitted*/ #include<cstdlib>
#endif
namespace micron
{
#if defined(MICRON_ABCMALLOC_STD) && !defined(__micron_sanitizer_owns_heap)
inline __attribute__((always_inline)) byte *
__alloc(usize sz)
{
  return abc::alloc(sz);
}

template<typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  abc::dealloc(reinterpret_cast<byte *>(ptr));
}
#else
inline __attribute__((always_inline)) void *
__alloc(usize sz)
{
  return ::malloc(sz);
}

template<typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  ::free(ptr);
}
#endif
};      // namespace micron
