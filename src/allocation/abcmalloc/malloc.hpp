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

#define MICRON_USE_ABCMALLOC 1

namespace abc
{
void
relinquish(byte *ptr)     // unmaps entire sheet at which ptr lives, resets arena entirely (NOTE: this will
{
  __init_abcmalloc();
  __main_arena->reset_page(ptr);
}

byte *mark_at(byte *ptr, size_t size);       // hard mark memory at addr. overrides previous entries
byte *unmark_at(byte *ptr, size_t size);     // hard unmark memory at addr. overrides previous entries

micron::__chunk<byte>
balloc(size_t size)     // allocates memory, returns entire memory chunk
{
  __init_abcmalloc();
  return __main_arena->push(size);
}

micron::__chunk<byte>
fetch(size_t size)
{
  __init_abcmalloc();
  return __main_arena->push(size);
}

template <typename T>
  requires(micron::is_trivial_v<T>)
T *
fetch(void)
{
  __init_abcmalloc();
  auto mem = __main_arena->push(sizeof(T));
  T *__obj = reinterpret_cast<T *>(mem.ptr);
  return __obj;
}

void
retire(byte *ptr)
{
  __init_abcmalloc();
  if ( __main_arena->ts_pop(ptr) ) {
  }     // wasn't able to throw, add an error
}

__attribute__((malloc, alloc_size(1))) auto
alloc(size_t size) -> byte *     // allocates memory, near iden. func. to malloc
{
  return balloc(size).ptr;
}

template <typename __S_ptr>
__attribute__((malloc, alloc_size(1))) __S_ptr
alloc_as_ptr(size_t size);     // allocates memory, returns packaged as a smart pointer

__attribute__((malloc, alloc_size(1))) byte *salloc(size_t size);     // applies policies from harden
void
dealloc(byte *ptr)
{
  __init_abcmalloc();
  if ( __main_arena->pop(ptr) ) {
  }     // wasn't able to throw, add an error
}

void
dealloc(byte *ptr, size_t len)
{
  __init_abcmalloc();
  if ( __main_arena->pop({ ptr, len }) ) {
  }     // wasn't able to throw, add an error
}

void
freeze(byte *ptr)
{
  __init_abcmalloc();
  if ( __main_arena->freeze(ptr) ) {
  }
}

// gets all pointers alloc'd by abc
void which(void);

void borrow();      //
void launder();     // realloc's memory at address
void query();       // queries the allocator for info
void make_at();     // creates memory at a set memory address

void inject();     // injects a function

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
void *calloc(size_t num, size_t size);     // alloc's zero'd out memory, prefer using salloc()
void *realloc(void *ptr, size_t size);     // reallocates memory
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
