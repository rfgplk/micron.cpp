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

#ifdef __ABC_AMD64
#include "config_amd64.hpp"
#elif defined(__ABC_EMBED)
#include "config_embed.hpp"
#else
// default to this
#include "config_amd64.hpp"
#endif

// doctor is platform agnostic

#if defined(ABCMALLOC_DOCTOR_HELP)
#define ABC_DOCTOR(...) __VA_ARGS__
#define ABC_EFF_REDZONE (::abc::__default_redzone)
#define ABC_EFF_GUARD (::abc::__default_insert_guard_pages)
#define ABC_EFF_PROVENANCE (::abc::__default_enforce_provenance)
#define ABC_EFF_POISON_ON_FREE (::abc::__default_poison_on_free)
#else
#define ABC_DOCTOR(...)
#define ABC_EFF_REDZONE __default_redzone
#define ABC_EFF_GUARD __default_insert_guard_pages
#define ABC_EFF_PROVENANCE __default_enforce_provenance
#define ABC_EFF_POISON_ON_FREE __default_poison_on_free
#endif

#if defined(ABCMALLOC_DOCTOR_HELP)
#include "doctor_fwd.hpp"
#endif
