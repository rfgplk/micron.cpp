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

#include "../../../types.hpp"

#include "../kmemory.hpp"

#include "../../../except.hpp"
#include "../../../type_traits.hpp"

#include "malloc.hpp"

namespace abc
{

template <typename T>
  requires(micron::is_integral_v<T>)
struct __abc_allocator {
  static auto
  calloc(size_t n) -> micron::__chunk<byte>     // allocate 'smartly'
  {
    auto mem = abc::fetch(n);
    if ( mem.ptr == (void *)-1 )
      micron::exc<micron::except::memory_error>("abc_allocator::alloc(): mmap() failed");
    return mem;
  };

  static T *
  alloc(size_t n)     // allocate 'smartly'
  {
    T *ptr = abc::alloc(n);
    if ( ptr == (void *)-1 )
      micron::exc<micron::except::memory_error>("abc_allocator::alloc(): mmap() failed");
    return ptr;
  };

  static void
  dealloc(T *mem, size_t len)
  {     // deallocate at location N
    if ( mem == nullptr )
      micron::exc<micron::except::memory_error>("abc_allocator::dealloc(): nullptr was provided");
    abc::dealloc(mem, len);
    mem = nullptr;
  }
};

};     // namespace abc
