
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

namespace abc
{

bool is_present(addr_t *ptr);

bool is_present(byte *ptr);

bool within(const addr_t *ptr);
bool within(addr_t *ptr);
bool within(byte *ptr);
void relinquish(byte *ptr);

byte *mark_at(byte *ptr, usize size);
byte *unmark_at(byte *ptr, usize size);

micron::__chunk<byte> balloc(usize size);

micron::__chunk<byte> fetch(usize size);

template <typename T>
  requires(micron::is_trivial_v<T>)
T *fetch(void);

void retire(byte *ptr);

__attribute__((malloc, alloc_size(1))) auto alloc(usize size) -> byte *;

__attribute__((malloc, alloc_size(1))) byte *salloc(usize size);
void dealloc(byte *ptr);

void dealloc(byte *ptr, usize len);

void freeze(byte *ptr);

void which(void);

void borrow();
__attribute__((malloc, alloc_size(1))) byte *launder(usize size);
template <typename T> usize query_size(T *ptr);

__attribute__((malloc, alloc_size(1))) void *malloc(usize size);
void *calloc(usize num, usize size);
void *realloc(void *ptr, usize size);

void free(void *ptr);
void *aligned_alloc(usize alignment, usize size);

};     // namespace abc
#ifdef ABCMALLOC_DISABLE

extern "C" __attribute__((malloc, alloc_size(1))) void *malloc(usize size);
extern "C" void *calloc(usize num, usize size);
extern "C" void *realloc(void *ptr, usize size);

extern "C" void free(void *ptr);
extern "C" void *aligned_alloc(usize alignment, usize size);
#endif
