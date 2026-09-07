//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../cmalloc.hpp"
#include "../../types.hpp"

#if !defined(__micron_abcmalloc_std_backend)
/*permitted*/ #include<cstdlib>
#endif
namespace micron
{
#if defined(__micron_abcmalloc_std_backend)
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

inline constexpr usize __native_alignment = abc::native_block_alignment;

inline __attribute__((always_inline)) void *
__alloc_aligned(usize alignment, usize bytes)
{
  return abc::aligned_alloc(alignment, bytes);
}

inline __attribute__((always_inline)) void
__free_aligned(void *ptr, usize alignment)
{
  abc::aligned_free(ptr, alignment);
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

inline constexpr usize __native_alignment = sizeof(void *) * 2;

inline __attribute__((always_inline)) void *
__alloc_aligned(usize alignment, usize bytes)
{
  return ::aligned_alloc(alignment, bytes);
}

inline __attribute__((always_inline)) void
__free_aligned(void *ptr, usize)
{
  ::free(ptr);
}
#endif
};      // namespace micron
