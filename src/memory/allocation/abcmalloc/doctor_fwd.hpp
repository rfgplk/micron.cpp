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

#if defined(ABCMALLOC_DOCTOR_HELP)

#include "../../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// doctor mode forward declarations

namespace abc
{
namespace doctor
{

void record_alloc(byte *ptr, usize req_size) noexcept;            // arena::push / launder success
void record_free(byte *ptr, usize len) noexcept;                  // arena::pop entry (len==0 when size unknown)
void record_tombstone(byte *ptr, usize len) noexcept;             // arena::ts_pop entry
void record_remote_free(byte *ptr, usize len) noexcept;           // __route_dealloc cross-thread branch
void record_realloc(byte *ptr, usize new_req_size) noexcept;      // arena::resize in-place reuse (refresh req_size + slack canary)

bool on_double_free(byte *ptr, const char *file, int line) noexcept;                       // handle_double_free
void on_free_result(byte *ptr, bool ok, const char *file, int line) noexcept;              // pop/ts_pop unknown-ptr result (non-fatal)
bool on_corruption(byte *ptr, const char *kind, const char *file, int line) noexcept;      // redzone/header/link corruption
bool on_bad_free(byte *ptr, usize len, const char *what, const char *file, int line) noexcept;      // malloc.hpp exc sites
usize check_free_size(byte *ptr, usize claimed, const char *file, int line) noexcept;               // dealloc(ptr,len) size validation

};      // namespace doctor
};      // namespace abc

#endif      // ABCMALLOC_DOCTOR_HELP
