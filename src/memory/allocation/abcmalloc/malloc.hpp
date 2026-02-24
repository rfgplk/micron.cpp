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

#include "../../../type_traits.hpp"
#include "../../../types.hpp"
#include "arena.hpp"
#include "tapi.hpp"

#include "../../../except.hpp"

namespace abc
{

// NOTE: when __guard_abcmalloc leaves scope, it'll automatically release the underlying mutex

// checks if pointer has been alloc.
bool
is_present(addr_t *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->present(ptr);
  }
  __start_abcmalloc_init();
  return __main_arena->present(ptr);
}

bool
is_present(byte *ptr)
{
  return is_present(reinterpret_cast<addr_t *>(ptr));
}

// checks if pointer is addressable at any known page of the allocator, if it is it's valid
bool
within(const addr_t *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->has_provenance(const_cast<addr_t *>(ptr));
  }
  __start_abcmalloc_init();
  return __main_arena->has_provenance(const_cast<addr_t *>(ptr));
}

bool
within(addr_t *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->has_provenance(ptr);
  }
  __start_abcmalloc_init();
  return __main_arena->has_provenance(ptr);
}

bool
within(byte *ptr)
{
  return within(reinterpret_cast<addr_t *>(ptr));
}

void
relinquish(byte *ptr)     // unmaps entire sheet at which ptr lives, resets arena entirely (NOTE: this will
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    __main_arena->reset_page(ptr);
    return;
  }
  __start_abcmalloc_init();
  __main_arena->reset_page(ptr);
}

byte *mark_at(byte *ptr, size_t size);       // hard mark memory at addr. overrides previous entries
byte *unmark_at(byte *ptr, size_t size);     // hard unmark memory at addr. overrides previous entries

micron::__chunk<byte>
balloc(size_t size)     // allocates memory, returns entire memory chunk
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->push(size);
  }
  __start_abcmalloc_init();
  return __main_arena->push(size);
}

micron::__chunk<byte>
fetch(size_t size)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->push(size);
  }
  __start_abcmalloc_init();
  return __main_arena->push(size);
}

template <typename T>
  requires(micron::is_trivial_v<T>)
T *
fetch(void)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    auto mem = __main_arena->push(sizeof(T));
    T *__obj = reinterpret_cast<T *>(mem.ptr);
    return __obj;
  }
  __start_abcmalloc_init();
  auto mem = __main_arena->push(sizeof(T));
  T *__obj = reinterpret_cast<T *>(mem.ptr);
  return __obj;
}

void
retire(byte *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    if ( __main_arena->ts_pop(ptr) ) {
      micron::exc<micron::except::memory_error>("retirr(): wasn't able to free memory");
    }     // wasn't able to throw, add an error
    return;
  }
  __start_abcmalloc_init();
  if ( __main_arena->ts_pop(ptr) ) {
    micron::exc<micron::except::memory_error>("retirr(): wasn't able to free memory");
  }     // wasn't able to throw, add an error
}

__attribute__((malloc, alloc_size(1))) auto
alloc(size_t size) -> byte *     // allocates memory, near iden. func. to malloc
{
  byte *ptr = balloc(size).ptr;
  if ( ptr == (byte *)-1 )
    return nullptr;
  return ptr;
}

__attribute__((malloc, alloc_size(1))) byte *salloc(size_t size);     // applies policies from harden

void
dealloc(byte *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    if ( !__main_arena->pop(ptr) ) {
      // WARNING:: due to poor interop with missing the STL and micron, there are edge cases where global objects constructed by glibc might
      // be invoked through *this* dealloc, instead of the libc free() one. meaning we will already have freed all of our memory. simply
      // fail out silently
      micron::exc<micron::except::memory_error>("dealloc(): wasn't able to free memory");
    }
    return;
  }
  __start_abcmalloc_init();
  if ( !__main_arena->pop(ptr) ) {
    micron::exc<micron::except::memory_error>("dealloc(): wasn't able to free memory");
  }
}

void
dealloc(byte *ptr, size_t len)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    if ( !__main_arena->pop({ ptr, len }) ) {
      micron::exc<micron::except::memory_error>("dealloc(): wasn't able to free memory");

    }     // wasn't able to throw, add an error
    return;
  }
  __start_abcmalloc_init();
  if ( !__main_arena->pop({ ptr, len }) ) {
    micron::exc<micron::except::memory_error>("dealloc(): wasn't able to free memory");
  }     // wasn't able to throw, add an error
}

void
freeze(byte *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    if ( __main_arena->freeze(ptr) ) {

    }     // wasn't able to throw, add an error
    return;
  }
  __start_abcmalloc_init();
  if ( __main_arena->freeze(ptr) ) {
  }
}

// gets all pointers alloc'd by abc
void
which(void)
{
  // TODO: implement
}

void borrow();     //

__attribute__((malloc, alloc_size(1))) byte *
launder(size_t size)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->launder(size).ptr;
  }
  __start_abcmalloc_init();
  return __main_arena->launder(size).ptr;
}

template <typename T>
size_t
query_size(T *ptr)
{
  return __main_arena->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
}

// leave these as void*
// launder()
// query
// inject
// make_at

size_t
musage(void)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->total_usage();
  }
  __start_abcmalloc_init();
  return __main_arena->total_usage();
}

template <u64 Sz>
size_t
musage(void)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), __abcmalloc_locktype> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __start_abcmalloc_init();
    return __main_arena->total_usage_of_class<Sz>();
  }
  __start_abcmalloc_init();
  return __main_arena->total_usage_of_class<Sz>();
}

__attribute__((malloc, alloc_size(1))) void *
malloc(size_t size)     // alloc memory of size 'size', prefer using alloc
{
  return reinterpret_cast<void *>(abc::alloc(size));
}

void *
calloc(size_t num, size_t size)     // alloc's zero'd out memory, prefer using salloc()
{
  if ( size != 0 && (size * num) / size != num )
    return nullptr;

  byte *mem = abc::alloc(size * num);
  if ( !mem )
    return nullptr;
  micron::zero(mem, size * num);
  return mem;
}

void *
realloc(void *ptr, size_t size)     // reallocates memory
{
  // NOTE: this always gets the full size of the allocated memory, not what was requested
  size_t old_size = abc::query_size(reinterpret_cast<addr_t *>(ptr));
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

  size_t copy_size = old_size < size ? old_size : size;
  micron::memcpy(new_block, reinterpret_cast<byte *>(ptr), copy_size);

  abc::dealloc(reinterpret_cast<byte *>(ptr));

  return new_block;
}

void
free(void *ptr)     // frees memory, prefer abc::dealloc always
{
  abc::dealloc(reinterpret_cast<byte *>(ptr));
}

void *aligned_alloc(size_t alignment, size_t size);

};     // namespace abc

#include "malloc-c.hpp"
