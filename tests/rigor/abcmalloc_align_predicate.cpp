//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE ALIGNMENT PREDICATE, AT THE ONE VALUE WHERE GETTING IT WRONG IS OBSERVABLE.
//
// abc::aligned_balloc routes on native_block_alignment (abcmalloc/malloc.hpp:410), which :368
// defines as `__default_redzone ? 16 : __hdr_offset`. With MICRON_ABC_REDZONE that is 16 and
// __hdr_offset is 32, so ANY caller that derives the bound itself -- from __hdr_offset, or from a
// literal 32 -- disagrees with the allocator at exactly alignment 32, and nowhere else. That is why
// this file is only meaningful under -DMICRON_ABC_REDZONE=true and says so at runtime otherwise.
//
// INVERTED POLARITY: each assertion states the property that SHOULD hold, so this file FAILS on a
// tree carrying either defect and the observed failure is the finding.
//
// The two defects are NOT the same failure:
//
//   math/compute.hpp      alloc took the over-aligned path (an INTERIOR pointer with a prefix in
//                         front of it), free took abc::dealloc against the block start it never was.
//   maps/immutable.hpp    both branches agreed with each other and disagreed with the allocator:
//                         alignof(__node) == 32 took the plain branch and came back UNDER-ALIGNED.
//
// MEASURED, reverting each site in turn with MICRON_ABC_REDZONE=true: compute.hpp exits 11
// (abcmalloc's own abort_state), immutable.hpp SIGSEGVs (139). Neither is the quiet leak the
// write-up for this predicted -- do not weaken the assertions below to "it did not crash" on the
// strength of that, because the assertion has to hold in configurations where it IS quiet, and
// because a crash is only what this particular pair of sizes and alignments happens to produce.
//
// A sanitizer can see NEITHER defect: --asan/--tsan switch __internal.hpp to the libc arm, which
// does not have it, and --lsan leaves the abc arm on but cannot see inside abcmalloc's mmap'd
// arenas. That is why the observers here are abcmalloc's own -- is_present() on the block start for
// the free, and the returned address itself for the alignment.
//
// Build: duck test tests/rigor/abcmalloc_align_predicate.cpp --def MICRON_ABC_REDZONE=true -o bin/t

#include "../../src/maps/immutable.hpp"
#include "../../src/math/compute.hpp"
#include "../../src/memory/allocation/__internal.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::test_case;

namespace
{

// exactly the boundary. 32 is __hdr_offset, and it is the only alignment at which the wrong
// predicate and the right one disagree.
struct alignas(32) av32 {
  u64 w[4];
};

// the CONTROL type: above the boundary on every predicate anyone might have written, so it takes the
// aligned path on a correct tree AND a defective one
struct alignas(64) av64 {
  u64 w[8];
};

template<typename V>
bool
nodes_are_aligned(int n) noexcept
{
  micron::immutable_map<int, V> m;
  for ( int k = 0; k < n; ++k ) m = m.insert(k, V{});
  for ( int k = 0; k < n; ++k ) {
    const V *p = m.find(k);
    if ( p == nullptr ) return false;
    if ( (reinterpret_cast<uintptr_t>(p) & (alignof(V) - 1)) != 0 ) return false;
  }
  return true;
}

using host = micron::math::compute::host_domains;

// An over-aligned pointer is INTERIOR: aligned_balloc stashes an __aligned_prefix immediately before
// it, so the block the allocator actually owns starts further back. This is how you get back to it,
// and it is the same helper tests/rigor/abcmalloc_c_aligned.cpp uses.
[[nodiscard]] byte *
block_of(void *p) noexcept
{
  byte *const raw = abc::__aligned_base_of(reinterpret_cast<byte *>(p));
  return raw != nullptr ? raw : reinterpret_cast<byte *>(p);
}

// The observer is is_present() on the BLOCK START, not musage(). musage() sums arena usage in
// sheet-sized granularity, so a few hundred leaked bytes do not move it at all -- the negative
// control in this file exists precisely because that had to be checked rather than assumed, and
// abcmalloc_c_aligned.cpp's own musage case carries a 1 MiB tolerance for the same reason.
bool
round_trips(usize alignment, usize bytes, int iters) noexcept
{
  for ( int i = 0; i < iters; ++i ) {
    void *p = host::allocate(host::host, bytes, alignment);
    if ( p == nullptr ) return false;
    if ( (reinterpret_cast<uintptr_t>(p) & (alignment - 1)) != 0 ) return false;

    byte *const base = block_of(p);
    if ( !abc::is_present(base) ) return false;      // the allocator agrees it owns this
    host::release(host::host, p, bytes, alignment);
    if ( abc::is_present(base) ) return false;      // ...and after release, it must not
  }
  return true;
}

}      // namespace

int
main()
{
  sb::print("MICRON_ABC_REDZONE is ", abc::__default_redzone ? "ON" : "OFF",
            " -- native_block_alignment = ", static_cast<u64>(micron::__native_alignment),
            ", __hdr_offset = ", static_cast<u64>(abc::__hdr_offset));
  if constexpr ( !abc::__default_redzone )
    sb::print("NOTE: without -DMICRON_ABC_REDZONE=true the two constants coincide and neither defect "
              "is reachable; the cases below are then regression coverage, not the finding.");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // the seam constant must BE the allocator's, not a copy of one of its inputs
  test_case("micron::__native_alignment is the allocator's routing constant");
  {
    require(micron::__native_alignment == abc::native_block_alignment);
    if constexpr ( abc::__default_redzone ) require(micron::__native_alignment != abc::__hdr_offset);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SITE A -- maps/immutable.hpp. alignof(__node) == 32 because alignof(V) == 32.
  // On the defective tree `alignof(__node) <= 32` sent this down abc::alloc, and under the redzone
  // the user pointer sits a fixed 16 bytes off the block start, so EVERY node is 16 mod 32.
  // Deterministic, not flaky.
  test_case("immutable_map: a 32-aligned value type gets 32-aligned nodes");
  {
    require(nodes_are_aligned<av32>(256));
  }
  end_test_case();

  // CONTROL: above the boundary, so both trees take the aligned path. If this ever fails the aligned
  // branch itself is broken and the case above is not the finding.
  test_case("CONTROL immutable_map: a 64-aligned value type gets 64-aligned nodes");
  {
    require(nodes_are_aligned<av64>(128));
  }
  end_test_case();

  // CONTROL: the ordinary path nothing was supposed to disturb
  test_case("CONTROL immutable_map: <int,int> still round-trips");
  {
    micron::immutable_map<int, int> m;
    for ( int k = 0; k < 512; ++k ) m = m.insert(k, k * 10);
    bool ok = true;
    for ( int k = 0; k < 512; ++k ) {
      const int *p = m.find(k);
      if ( p == nullptr || *p != k * 10 ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  // NEGATIVE CONTROL: the predicate must be able to answer false, or the three cases above prove
  // nothing. An address 16 past a 32-aligned one is not 32-aligned.
  test_case("NEGATIVE CONTROL: the alignment predicate discriminates");
  {
    micron::immutable_map<int, av32> m;
    m = m.insert(1, av32{});
    const auto a = reinterpret_cast<uintptr_t>(m.find(1));
    require((a & 31u) == 0u);
    require(((a + 16u) & 31u) != 0u);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SITE B -- math/compute.hpp. The leak. musage() is the only observer: dealloc's refusal of a
  // foreign pointer is silent unless ABCMALLOC_DOCTOR_HELP is on.
  test_case("host_domains: an alignment-32 allocate/release pair releases the block it took");
  {
    require(round_trips(32, 256, 64));
  }
  end_test_case();

  // CONTROLS: below and above the boundary. Both must hold on a defective tree too -- 16 is under
  // every candidate bound, 64 is over even the wrong one -- which is what pins the failure to 32.
  test_case("CONTROL host_domains: alignment 16 releases the block it took");
  {
    require(round_trips(16, 256, 64));
  }
  end_test_case();

  test_case("CONTROL host_domains: alignment 64 releases the block it took");
  {
    require(round_trips(64, 256, 64));
  }
  end_test_case();

  // NEGATIVE CONTROL: prove is_present() can answer BOTH ways for the same address, or "it is gone
  // after the free" is worthless. The first draft of this file used musage() as the observer and
  // this control is what caught that musage() does not move for a 4 KiB block at all -- it sums
  // arena usage at sheet granularity, so it would have passed every case above vacuously.
  test_case("NEGATIVE CONTROL: is_present() discriminates live from freed");
  {
    byte *b = abc::alloc(4096);
    require(b != nullptr);
    require(abc::is_present(b));
    abc::dealloc(b);
    require(!abc::is_present(b));
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
