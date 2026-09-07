//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// DOES THE DEFAULT HASH STILL PRODUCE THE SAME BYTES?
//
// bits/__arch.hpp clears the vector ISA-presence macros -- __micron_x86_sse*/avx*/fma and
// __micron_arm_neon -- under __micron_simd_generic. It had only ever cleared the WIDTH and the ARM
// TIER, so roughly 150 `#if defined(__micron_x86_avx2)` sites across the tree stayed live under
// MICRON_NO_SIMD, guarding code that needs vector REGISTERS on a question about what -march
// enabled. That is CLAUDE.md hard rule #3, and BAREBONES.md records four of these being repaired by
// hand as "the 9 red cells" without the class ever being repaired.
//
// Clearing them fixes the class in one place. It also touches the one decision in the tree whose
// output is USER-VISIBLE AND PERSISTABLE: which hash `default_hash_64` selects. CLAUDE.md is
// explicit that an SSE2 build and an AVX2 build produce different hash values, and that this is
// "fine in memory, NEVER for persisted or wire data".
//
// hash.hpp:39 already gated __micron_hash_zzz on !defined(__micron_simd_generic) as well as on the
// ISA, so a MICRON_NO_SIMD build was ALREADY taking the ISA-free defaults and clearing the macros
// changes nothing. But "changes nothing" is a claim, and this file is the measurement -- captured
// before the change and asserted after.
//
// THE TWO EXPECTED DIGESTS ARE FIXED HEX-ISH LITERALS ON PURPOSE, the way rigor seeds are. A test
// that recomputes its own expectation cannot fail.
//
//   zzz selected     (AVX2 or NEON, and not the generic tier)  -> 1614250494984754567
//   ISA-free default (murmur128 / rapidhash)                   -> 13468128435704476969
//
// So every MICRON_NO_SIMD cell must land on the second value at EVERY -march tier, and a plain
// --isa base or --isa v2 cell must land on it too. Only an ISA build at v3 or above may differ.
//
// NEGATIVE CONTROL: build any cell with -DMICRON_HASH_STABILITY_NEGATIVE and the expectation is
// inverted, so a green run there means the test cannot fail and the gate is worthless.

#include "../../src/hash/hash.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

namespace
{

// fixed corpus: empty, one byte, a short string, an aligned block, and one long enough to reach the
// bulk path of every implementation
const char *const kCorpus[] = {
  "",
  "a",
  "abc",
  "micron-barebones",
  "0123456789abcdef0123456789abcdef",
  "the quick brown fox jumps over the lazy dog, twice, for a longer input",
};

constexpr u64 kZzz = 1614250494984754567ull;
constexpr u64 kIsaFree = 13468128435704476969ull;

// FNV-1a over the per-input hashes, so one digest covers the whole corpus and a change in any single
// input moves it.
u64
digest(void) noexcept
{
  u64 acc = 0xcbf29ce484222325ull;
  for ( const char *s : kCorpus ) {
    acc ^= micron::hash64(s);
    acc *= 0x100000001b3ull;
  }
  return acc;
}

};      // namespace

int
main(void)
{
  sb::test_case("the default hash digest matches the value pinned for this tier");

  const u64 got = digest();

#if defined(__micron_hash_zzz)
  const u64 want = kZzz;
  const char *which = "zzz (ISA)";
#else
  const u64 want = kIsaFree;
  const char *which = "murmur/rapid (ISA-free)";
#endif

  sb::print("selected: ", which);
  sb::print("digest:   ", got);
  sb::print("expected: ", want);

#if defined(MICRON_HASH_STABILITY_NEGATIVE)
  // the control: this must FAIL on a correct tree, proving the assertion below can fail at all
  sb::require(got != want);
#else
  sb::require(got == want);
#endif

  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // and the property that actually matters for persisted data: the ISA-free digest is the SAME
  // number on every tier, so a kernel or bare-metal build agrees with a hosted one. A cell that
  // selects zzz is exempt and says so.
  sb::test_case("an ISA-free build agrees with every other ISA-free build");
#if defined(__micron_hash_zzz)
  sb::skip("this cell selected zzz; only the ISA-free cells carry the cross-tier claim");
#else
  sb::require(got == kIsaFree);
#endif
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
