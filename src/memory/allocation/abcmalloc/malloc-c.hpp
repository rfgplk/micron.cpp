// Copyright (c) 2025 David Lucius Severus
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

#include "../../../type_traits.hpp"
#include "../../../types.hpp"
#include "tapi.hpp"

#ifndef ABCMALLOC_DISABLE

extern "C" __attribute__((malloc, alloc_size(1))) void *
malloc(usize size)     // alloc memory of size 'size', prefer using alloc
{
  return reinterpret_cast<void *>(abc::alloc(size));
}

extern "C" void *
calloc(usize num, usize size)     // alloc's zero'd out memory, prefer using salloc()
{
  if ( size != 0 && (size * num) / size != num )
    return nullptr;

  byte *mem = abc::alloc(size * num);
  if ( !mem )
    return nullptr;
  micron::zero(mem, size * num);
  return mem;
}

extern "C" void *
realloc(void *ptr, usize size)     // reallocates memory
{
  // NOTE: this always gets the full size of the allocated memory, not what was requested
  usize old_size = abc::query_size(reinterpret_cast<addr_t *>(ptr));
  if ( size == 0 ) {
    abc::dealloc(reinterpret_cast<byte *>(ptr));
    return nullptr;
  }

  if ( !ptr ) {
    return reinterpret_cast<void *>(abc::alloc(size));
  }

  byte *new_block = abc::alloc(size);
  if ( !new_block )
    return nullptr;     // allocation failed

  usize copy_size = old_size < size ? old_size : size;
  micron::memcpy(new_block, reinterpret_cast<byte *>(ptr), copy_size);

  abc::dealloc(reinterpret_cast<byte *>(ptr));

  return new_block;
}

extern "C" void
free(void *ptr)     // frees memory, prefer abc::dealloc always
{
  abc::dealloc(reinterpret_cast<byte *>(ptr));
}

extern "C" void *aligned_alloc(usize alignment, usize size);

#endif
