// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "malloc.hpp"

#include "../../../types.hpp"

extern "C" [[gnu::used]] __attribute__((malloc, alloc_size(1))) inline void *
malloc(usize size) noexcept
{
  return bb::malloc(size);
}

extern "C" [[gnu::used]] inline void *
calloc(usize num, usize size) noexcept
{
  return bb::calloc(num, size);
}

extern "C" [[gnu::used]] inline void *
realloc(void *ptr, usize size) noexcept
{
  return bb::realloc(ptr, size);
}

extern "C" [[gnu::used]] inline void
free(void *ptr) noexcept
{
  bb::free(ptr);
}

extern "C" [[gnu::used]] inline void *
aligned_alloc(usize alignment, usize size) noexcept
{
  return bb::aligned_alloc(alignment, size);
}

extern "C" [[gnu::used]] inline void *
memalign(usize alignment, usize size) noexcept
{
  return static_cast<void *>(bb::aligned_balloc(alignment, size).ptr);
}

extern "C" [[gnu::used]] inline int
posix_memalign(void **out, usize alignment, usize size) noexcept
{
  if( out == nullptr || alignment < sizeof(void *) || !bb::__bits::__is_pow2(alignment) )
    return 22;
  void *p = static_cast<void *>(bb::aligned_balloc(alignment, size).ptr);
  if( p == nullptr && size != 0 )
    return 12;
  *out = p;
  return 0;
}

extern "C" [[gnu::used]] inline void *
valloc(usize size) noexcept
{
  return static_cast<void *>(bb::aligned_balloc(micron::page_size, size).ptr);
}

extern "C" [[gnu::used]] inline void *
pvalloc(usize size) noexcept
{
  const usize n = bb::__bits::__round_up_sat(size, micron::page_size);
  if( n == bb::__bits::__usize_max )
    return nullptr;
  return static_cast<void *>(bb::aligned_balloc(micron::page_size, n).ptr);
}
