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

#if !defined(ABCMALLOC_DISABLE) && !defined(__micron_sanitizer_owns_heap)

// NOTE: these should be noexcept so we avoid conflicting declarations

extern "C" __attribute__((malloc, alloc_size(1))) void *
malloc(usize size) noexcept      // alloc memory of size 'size', prefer using alloc
{
  return reinterpret_cast<void *>(abc::alloc(size));
}

extern "C" void *
calloc(usize num, usize size) noexcept      // alloc's zero'd out memory, prefer using salloc()
{
  if ( size != 0 && (size * num) / size != num ) return nullptr;

  byte *mem = abc::alloc(size * num);
  if ( !mem ) return nullptr;
  micron::zero(mem, size * num);
  return mem;
}

extern "C" void *
realloc(void *ptr, usize size) noexcept      // reallocates memory
{
  if ( !ptr ) return reinterpret_cast<void *>(abc::alloc(size));
  if ( size == 0 ) {
    abc::free(ptr);
    return nullptr;
  }

  // NOTE: this always gets the full size of the allocated memory, not what was
  // requested;
  // for posix_memalign() the block starts before ptr
  byte *const old = reinterpret_cast<byte *>(ptr);
  byte *const base = abc::__aligned_base_of(old);
  const usize block = abc::query_size(micron::ptr_cast<addr_t *>(base != nullptr ? base : old));
  const usize displacement = base != nullptr ? static_cast<usize>(old - base) : 0u;
  const usize old_size = block > displacement ? block - displacement : 0u;

  byte *new_block = abc::alloc(size);
  if ( !new_block ) return nullptr;      // allocation failed

  const usize copy_size = old_size < size ? old_size : size;
  if ( copy_size != 0 ) micron::memcpy(new_block, old, copy_size);

  if ( base != nullptr )
    abc::dealloc(base);
  else
    abc::dealloc(old);

  return new_block;
}

extern "C" void
free(void *ptr) noexcept      // frees memory, prefer abc::dealloc always
{
  abc::free(ptr);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// POSIX ALIGNED FUNCTIONS
//
// WARNING: ITS ABSOLUTELY CRITICAL that we declare all aligned_* posix compliant fns;
// posix_memalign/memalign/valloc/pvalloc; if any external lib/code calls those fns they
// will INTERPOSE LIBCs malloc and handing back those ptrs to us on a regular free()
//
// NOTE: abc::aligned_balloc is used rather than abc::aligned_alloc

extern "C" void *
aligned_alloc(usize alignment, usize size) noexcept
{
  return reinterpret_cast<void *>(abc::aligned_balloc(alignment, size).ptr);
}

extern "C" int
posix_memalign(void **out, usize alignment, usize size) noexcept
{
  if ( out == nullptr ) return 22;      // EINVAL
  // POSIX: alignment is a power of two AND a multiple of sizeof(void *)
  if ( alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0 ) return 22;
  if ( size == 0 ) {
    *out = nullptr;
    return 0;
  }
  void *mem = reinterpret_cast<void *>(abc::aligned_balloc(alignment, size).ptr);
  if ( mem == nullptr ) return 12;      // ENOMEM
  *out = mem;
  return 0;
}

extern "C" void *
memalign(usize alignment, usize size) noexcept
{
  return reinterpret_cast<void *>(abc::aligned_balloc(alignment, size).ptr);
}

extern "C" void *
valloc(usize size) noexcept
{
  return reinterpret_cast<void *>(abc::aligned_balloc(abc::__system_pagesize, size).ptr);
}

extern "C" void *
pvalloc(usize size) noexcept
{
  const usize rounded = (size + abc::__system_pagesize - 1) & ~(abc::__system_pagesize - 1);
  return reinterpret_cast<void *>(abc::aligned_balloc(abc::__system_pagesize, rounded).ptr);
}

#endif
