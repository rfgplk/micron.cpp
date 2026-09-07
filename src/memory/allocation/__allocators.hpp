//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// The allocator_types/*.hpp fragments are include-less BY DESIGN and must stay that way -- they are
// stitched together below and get everything from this prologue. Six of them name mman symbols, so
// the repair for those is one line here rather than six lines in the fragments.
//
// Under the barebones allocator there is no mmap to reach: (K) has no userspace address space and
// (E) has no MMU. The mman include and the fragments that need it are both gated, together, so the
// two cannot drift apart.
//
// AND THAT REQUIRED THE INCLUDE BELOW, WITHOUT WHICH THIS GATE WAS DEAD TEXT. __micron_bb_alloc is
// defined by defs.hpp:28, and nothing above this line included it -- the file went straight from
// #pragma once into the branch -- so the test was always false and mman.hpp was pulled in on EVERY
// build, kernel and metal included. Its twin forty lines down (the seven excluded allocator_types
// fragments) sits after "__internal.hpp", which reaches defs.hpp transitively, so that one was
// live: the two gates the comment above says "cannot drift apart" already had.
//
// It was invisible because mman.hpp's wrappers are inline and nothing on the (K) path calls them,
// so a kernel object still measured 0 syscalls. It stopped being invisible when
// port/backends/__syscall.hpp gained its backend #error -- which is the whole argument for putting
// that guard at the floor of the ladder rather than at each caller.
//
// This is policies.hpp:45-48's own warning, and BAREBONES.md's "A #if-GATED BRANCH THAT NO CELL
// COMPILES IS DEAD TEXT", landing a third time. defs.hpp is macros only and costs nothing.
#include "../../defs.hpp"

#if !defined(__micron_bb_alloc)
#include "../mman.hpp"
#endif
#include "__internal.hpp"
#include "bits.hpp"
#include "policies.hpp"

namespace micron
{
template<typename P>
concept is_policy = requires {
  P::concurrent;
  P::shareable;
  P::on_grow;
  P::minimum_bytes;
  P::granularity;
  P::growth_numerator;
  P::growth_denominator;
};
};      // namespace micron

// clang-format off: these headers form a dependency chain.
#include "allocator_types/bits.hpp"
#include "allocator_types/__scheme.hpp"
#include "allocator_types/abc_policy_allocator.hpp"

#include "allocator_types/constrained_allocator.hpp"
#include "allocator_types/exact_allocator.hpp"
#include "allocator_types/serial_allocator.hpp"
#include "allocator_types/small_allocator.hpp"
#include "allocator_types/static_allocator.hpp"
#include "allocator_types/monotonic_allocator.hpp"
#include "allocator_types/arena_resource.hpp"

// The seven that do not exist under the barebones allocator. Six of them are mmap machinery --
// guard pages, MAP_FIXED windows, mprotect'd immutability, huge pages -- and the seventh
// (advanced_abc) is built on abcmalloc's own lifetime verbs, aligned_launder/aligned_retire, which
// have no meaning without sheets and arenas. Excluded rather than stubbed: a guarded_allocator that
// cannot place a guard page is not a degraded guarded_allocator, it is a lie.
//
// Nothing outside allocator_types/ names any of them -- verified across src/, and the two test
// files that do (tests/rigor/allocator_fuzz.cpp, tests/compiletests/allocator_types.cpp) are hosted
// and unaffected.
#if !defined(__micron_bb_alloc)
#include "allocator_types/advanced_abc_allocators.hpp"
#include "allocator_types/fixed_map_allocator.hpp"
#include "allocator_types/guarded_allocator.hpp"
#include "allocator_types/huge_allocator.hpp"
#include "allocator_types/map_allocator.hpp"
#include "allocator_types/immutable_allocator.hpp"
#include "allocator_types/secure_allocator.hpp"
#endif
// clang-format on
