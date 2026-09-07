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

#include "config.hpp"
#include "metadata.hpp"

#include "../../../memory/allocation/kmemory.hpp"
#include "../../../types.hpp"

namespace bb
{

inline bool attach(byte *ptr, usize len) noexcept;
inline bool attach(micron::__chunk<byte> region) noexcept;
inline bool attach(byte *ptr, usize len, u8 *tags, usize tag_cap) noexcept;
inline bool init() noexcept;
inline void reset() noexcept;
inline micron::__chunk<byte> balloc(usize size) noexcept(__default_fail_result != 2);
inline micron::__chunk<byte> zalloc(usize size) noexcept(__default_fail_result != 2);
inline micron::__chunk<byte> aligned_balloc(usize alignment, usize size) noexcept(__default_fail_result != 2);
__attribute__((malloc, alloc_size(1))) inline byte *alloc(usize size) noexcept(__default_fail_result != 2);
__attribute__((malloc, alloc_size(1))) inline byte *salloc(usize size) noexcept(__default_fail_result != 2);
inline bool dealloc(void *ptr) noexcept;
inline void dealloc(byte *ptr) noexcept;
inline void aligned_free(void *ptr) noexcept;
inline void *aligned_alloc(usize alignment, usize size) noexcept(__default_fail_result != 2);
inline usize query_size(const void *ptr) noexcept;
inline bool is_present(const void *ptr) noexcept;
inline bool within(const void *ptr) noexcept;
inline micron::__chunk<byte> resize(micron::__chunk<byte> old, usize size, usize preserve,
                                    usize alignment) noexcept(__default_fail_result != 2);
inline usize musage() noexcept;
inline usize available() noexcept;
inline usize largest_free() noexcept;
inline usize capacity() noexcept;

};
