//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "arena.hpp"
#include "tapi.hpp"

namespace abc
{

// NOTE: when __guard_abcmalloc leaves scope, it'll automatically release the underlying mutex

// checks if pointer has been alloc.
bool
is_present(addr_t *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    return __main_arena->present(ptr);
  }
  __init_abcmalloc();
  return __main_arena->present(ptr);
}

bool
is_present(byte *ptr)
{
  return is_present(reinterpret_cast<addr_t *>(ptr));
}

// checks if pointer is addressable at any known page of the allocator, if it is it's valid
bool
within(addr_t *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    return __main_arena->has_provenance(ptr);
  }
  __init_abcmalloc();
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
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    __main_arena->reset_page(ptr);
    return;
  }
  __init_abcmalloc();
  __main_arena->reset_page(ptr);
}

byte *mark_at(byte *ptr, size_t size);       // hard mark memory at addr. overrides previous entries
byte *unmark_at(byte *ptr, size_t size);     // hard unmark memory at addr. overrides previous entries

micron::__chunk<byte>
balloc(size_t size)     // allocates memory, returns entire memory chunk
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    return __main_arena->push(size);
  }
  __init_abcmalloc();
  return __main_arena->push(size);
}

micron::__chunk<byte>
fetch(size_t size)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    return __main_arena->push(size);
  }
  __init_abcmalloc();
  return __main_arena->push(size);
}

template <typename T>
  requires(micron::is_trivial_v<T>)
T *
fetch(void)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    auto mem = __main_arena->push(sizeof(T));
    T *__obj = reinterpret_cast<T *>(mem.ptr);
    return __obj;
  }
  __init_abcmalloc();
  auto mem = __main_arena->push(sizeof(T));
  T *__obj = reinterpret_cast<T *>(mem.ptr);
  return __obj;
}

void
retire(byte *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    if ( __main_arena->ts_pop(ptr) ) {

    }     // wasn't able to throw, add an error
    return;
  }
  __init_abcmalloc();
  if ( __main_arena->ts_pop(ptr) ) {
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
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    if ( __main_arena->pop(ptr) ) {

    }     // wasn't able to throw, add an error
    return;
  }
  __init_abcmalloc();
  if ( !__main_arena->pop(ptr) ) {
  }     // wasn't able to throw, add an error
}

void
dealloc(byte *ptr, size_t len)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    if ( !__main_arena->pop({ ptr, len }) ) {

    }     // wasn't able to throw, add an error
    return;
  }
  __init_abcmalloc();
  if ( !__main_arena->pop({ ptr, len }) ) {
  }     // wasn't able to throw, add an error
}

void
freeze(byte *ptr)
{
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    if ( __main_arena->freeze(ptr) ) {

    }     // wasn't able to throw, add an error
    return;
  }
  __init_abcmalloc();
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
  if constexpr ( micron::is_same_v<decltype(__guard_abcmalloc()), micron::unique_lock<micron::lock_starts::locked>> ) {
    [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
    __init_abcmalloc();
    return __main_arena->launder(size).ptr;
  }
  __init_abcmalloc();
  return __main_arena->launder(size).ptr;
}
template <typename T>
size_t
query_size(T *ptr)
{
  return __main_arena->__size_of_alloc(reinterpret_cast<addr_t*>(ptr));
}

//
// LEGACY STDLIB FUNCTIONS START HERE
//
//
//
// main functions retained for backwards compatibility with the c stdlib
// even those these are fully functional, they aren't intended to be called as the de facto standard allocating fn's

// leave these as void*
__attribute__((malloc, alloc_size(1))) void *
malloc(size_t size)     // alloc memory of size 'size', prefer using alloc
{
  return reinterpret_cast<void *>(alloc(size));
}
void *
calloc(size_t num, size_t size)     // alloc's zero'd out memory, prefer using salloc()
{
  if ( size != 0 && (size * num) / size != num )
    return nullptr;

  byte *mem = alloc(size * num);
  if ( !mem )
    return nullptr;
  micron::zero(mem, size * num);
  return mem;
}
void *
realloc(void *ptr, size_t size)     // reallocates memory
{
  size_t old_size = query_size(reinterpret_cast<addr_t*>(ptr));
  if ( size == 0 ) {
    dealloc(reinterpret_cast<byte *>(ptr));
    return nullptr;
  }

  if ( !ptr ) {
    return reinterpret_cast<void *>(alloc(size));
  }

  byte *new_block = alloc(size);
  if ( !new_block )
    return nullptr;     // allocation failed

  size_t copy_size = old_size < size ? old_size : size;
  micron::memcpy(new_block, reinterpret_cast<byte*>(ptr), copy_size);

  dealloc(reinterpret_cast<byte *>(ptr));

  return new_block;
}

void
free(void *ptr)     // frees memory, prefer dealloc always
{
  dealloc(reinterpret_cast<byte *>(ptr));
}
void *aligned_alloc(size_t alignment, size_t size);
// launder()
// query
// inject
// make_at
};
