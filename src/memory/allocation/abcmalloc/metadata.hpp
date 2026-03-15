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
#include "../../../memory/pointers/sentinel.hpp"
#include "../../../types.hpp"

namespace abc
{
// shared spec for both alloc systems
using ret_flag = micron::sentinel_pointer;
constexpr static const uintptr_t __flag_invalid = 0;
constexpr static const uintptr_t __flag_failure = -1;
constexpr static const uintptr_t __flag_out_of_space = -2;
constexpr static const uintptr_t __flag_tombstoned = -3;
constexpr static const uintptr_t __flag_ok = 1;

struct block_header {
  i32 order;
  i32 flags;
};

enum block_flags : i32 {
  __block_free = 0,
  __block_alloc = 1,
  __block_tombstone = 2,
  __block_temporal = 4,
};

constexpr static usize __hdr_offset = sizeof(micron::simd::i256);

inline block_header *
get_block_header(byte *user_ptr)
{
  return reinterpret_cast<block_header *>(user_ptr - __hdr_offset);
}

inline addr_t *
get_metadata_addr(addr_t *ptr)
{
  return reinterpret_cast<addr_t *>(reinterpret_cast<byte *>(ptr) - __hdr_offset);
}

};     // namespace abc
