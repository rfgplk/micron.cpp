//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE BAREBONES ALLOCATOR, EXERCISED -- and exercised HOSTED, on purpose.
//
// micron::bb is selected automatically by MICRON_PORT_KERNEL and MICRON_PORT_METAL, but it is also
// selectable on its own with MICRON_BAREBONES_ALLOC, and that separation exists precisely so this
// file can exist. An allocator whose only test environment is a kernel module is an allocator
// nobody tests: you cannot run a rigor suite under insmod, a failure there is a dmesg oops rather
// than a require() line, and a bug costs a reboot. Here the page source is the linux backend and
// everything above it is the same code the module runs.
//
// What is actually being asserted, in order of how expensively it fails:
//
//  (a) allocation_extent is PURE, MONOTONIC and IDEMPOTENT. This is the one that fails silently and
//      immediately: __owned_memory_resource stores element capacity and NOTHING ELSE, then
//      reconstructs the byte length by re-running the rule (mutable_resource.hpp:21-37). A rule
//      that is not a fixed point throws on the FIRST mc::vector<T> ever constructed
//      (mutable_resource.hpp:45), so "it compiled" tells you nothing at all.
//  (b) the three allocator_traits postconditions (__scheme.hpp:374-401): non-null for a non-zero
//      request, ptr % alignment == 0, len >= bytes.
//  (c) the round trip a container actually performs -- construct, grow across several element
//      sizes, destroy -- because (a) is checked at every one of those points, not just the first.
//  (d) bb's own block bookkeeping: query_size, is_present, the double-free guard, and that
//      over-aligned blocks free through the SAME path as plain ones (the property that lets the
//      shim ignore the alignment argument on every dealloc form).
//  (e) that within() is honest about being approximate -- it must say no to an address that was
//      never mapped, which is the only direction its caller relies on.
//
// Seeds are fixed hex literals. Never time-based: a fuzz test that cannot be replayed is a bug
// report you cannot act on.

#include "../../src/maps.hpp"
#include "../../src/memory/allocation/barebones/bb_alloc.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

namespace
{

struct rng {
  u64 s;
  constexpr explicit rng(u64 seed) noexcept : s(seed) { }
  u64
  next() noexcept
  {
    u64 z = (s += 0x9E37'79B9'7F4A'7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58'476D'1CE4'E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D0'49BB'1331'11EBull;
    return z ^ (z >> 31);
  }
};

// the allocator every container actually gets
using alloc_t = micron::allocator_serial<>;
using traits_t = micron::allocator_traits<alloc_t>;

// a non-trivial payload, so the resource takes the copy path rather than the memcpy path
struct payload {
  u64 a;
  u32 b;
  char c;
  payload() : a(0), b(0), c('x') { }
  explicit payload(u64 v) : a(v), b(static_cast<u32>(v)), c('y') { }
};

template<typename T>
bool
round_trip()
{
  micron::vector<T> v;
  for ( u64 i = 0; i < 700; ++i ) v.push_back(T{});
  if ( v.size() != 700 ) return false;
  v.clear();
  return true;
}

};      // namespace

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (a) allocation_extent: idempotent, monotonic, pure
  //
  // f(f(x)) == f(x) is exactly what __extent_matches re-derives on every free and every resize.
  sb::test_case("allocation_extent is idempotent: f(f(x)) == f(x)");
  {
    bool ok = true;
    for ( usize a : { usize{ 1 }, usize{ 8 }, usize{ 16 }, usize{ 32 }, usize{ 64 }, usize{ 4096 } } ) {
      for ( usize n = 0; n <= 300000 && ok; n = (n < 64 ? n + 1 : n + 617) ) {
        const usize once = traits_t::allocation_extent(n, a);
        const usize twice = traits_t::allocation_extent(once, a);
        if ( once != twice ) ok = false;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("allocation_extent is monotonic and never under-reports");
  {
    bool ok = true;
    usize prev = 0;
    for ( usize n = 1; n <= 200000 && ok; n += 331 ) {
      const usize e = traits_t::allocation_extent(n, 64);
      if ( e < n ) ok = false;      // must never come out below the request
      if ( e < prev ) ok = false;   // must never move backwards
      prev = e;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("allocation_extent is pure: same answer every time, no hidden state");
  {
    bool ok = true;
    rng r(0x5DEE'CE66'D3A7'11B3ull);
    for ( int i = 0; i < 4000 && ok; ++i ) {
      const usize n = static_cast<usize>(r.next() % 100000u) + 1;
      const usize first = traits_t::allocation_extent(n, 64);
      // churn the allocator hard in between -- a cursor- or freelist-derived extent would drift
      auto scratch = traits_t::allocate<64>(n);
      traits_t::deallocate<64>(scratch);
      if ( traits_t::allocation_extent(n, 64) != first ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (b) the three postconditions allocator_traits checks when it does not trust the allocator
  sb::test_case("allocate() postconditions: non-null, aligned, len >= bytes");
  {
    bool ok = true;
    rng r(0xC0FF'EE12'3456'789Aull);
    for ( int i = 0; i < 2000 && ok; ++i ) {
      const usize n = static_cast<usize>(r.next() % 65536u) + 1;
      for ( usize a : { usize{ 8 }, usize{ 16 }, usize{ 32 }, usize{ 64 }, usize{ 256 } } ) {
        auto c = traits_t::allocate(n, a);
        if ( c.ptr == nullptr ) { ok = false; break; }
        if ( (reinterpret_cast<uintptr_t>(c.ptr) & (a - 1)) != 0 ) { ok = false; break; }
        if ( c.len < n ) { ok = false; break; }
        // writable across the whole grant -- a header that overlapped the payload would show here
        __builtin_memset(c.ptr, 0x5A, c.len);
        traits_t::deallocate(c, a);
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("allocate(0) is {nullptr, 0} and deallocating it is legal");
  {
    auto c = traits_t::allocate(0, 64);
    sb::require(c.ptr == nullptr && c.len == 0);
    traits_t::deallocate(c, 64);
    micron::bb::dealloc(static_cast<void *>(nullptr));
    sb::require(true);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (c) the round trip a container performs. sizeof(T) varies deliberately: the reconstruction is
  //     (len / sizeof(T)) * sizeof(T), so an element size that does not divide the grant is the
  //     case where a non-idempotent rule shows up.
  sb::test_case("mutable_resource round trip across element sizes");
  {
    sb::require(round_trip<u8>());
    sb::require(round_trip<u16>());
    sb::require(round_trip<u32>());
    sb::require(round_trip<u64>());
    sb::require(round_trip<payload>());
  }
  sb::end_test_case();

  sb::test_case("vector grows, holds its values, and survives repeated churn");
  {
    bool ok = true;
    for ( int pass = 0; pass < 12 && ok; ++pass ) {
      micron::vector<u64> v;
      for ( u64 i = 0; i < 20000; ++i ) v.push_back(i ^ 0xA5A5ull);
      if ( v.size() != 20000 ) ok = false;
      for ( u64 i = 0; i < 20000 && ok; ++i )
        if ( v[static_cast<usize>(i)] != (i ^ 0xA5A5ull) ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // WARNING: this case is sensitive to something that is NOT the allocator. micron::hashes::zzz --
  // the default hash above -march=x86-64-v2 -- silently loses inserts in hopscotch_map (3 of 1000,
  // 10 of 4000, 37 of 8000, measured), identically on abcmalloc and on bb. Build this suite with
  // MICRON_NO_ZZZ_HASH, as verify_compile_barebones.duck does, or the failure you see here is that
  // defect wearing the allocator's coat. See ISSUES.md.
  sb::test_case("map and string over the barebones allocator");
  {
    micron::hopscotch_map<u64, u64> m;
    for ( u64 i = 0; i < 4000; ++i ) m.insert(i, i * 3);
    u64 sum = 0;
    for ( u64 i = 0; i < 4000; ++i ) sum += m[i];
    sb::require(m.size() == 4000);
    sb::require(sum == (3999ull * 4000ull / 2ull) * 3ull);

    micron::string s = "bb";
    for ( int i = 0; i < 500; ++i ) s += "0123456789";
    sb::require(s.size() == 2 + 5000);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (d) bb's own bookkeeping
  // NOTE: this used to assert query_size(p) == n exactly, which was only ever true of the
  // page-per-allocation stub -- it stored the requested length in a header. A buddy tier's only
  // metadata is a tag byte holding an ORDER, and a TLSF block records its class size, so neither
  // can answer what the caller originally asked for. The contract is therefore the one bbmalloc's
  // own bbmalloc_traits.cpp makes: the grant covers the request and query_size covers the grant.
  sb::test_case("query_size covers the grant, and the grant covers the request");
  {
    bool ok = true;
    rng r(0x1234'5678'9ABC'DEF0ull);
    for ( int i = 0; i < 500 && ok; ++i ) {
      const usize n = static_cast<usize>(r.next() % 30000u) + 1;
      auto c = micron::bb::balloc(n);
      if ( c.ptr == nullptr || c.len < n ) ok = false;
      else if ( micron::bb::query_size(c.ptr) < c.len ) ok = false;
      else {
        // the whole reported length must be writable -- a header overlapping the payload shows here
        __builtin_memset(c.ptr, 0x5A, c.len);
      }
      micron::bb::dealloc(c.ptr);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // the extent rule is a FIXED POINT and the tier boundary MOVES WITH THE REDZONE, so every bound
  // here is written in terms of bb::__class_small and bb::__rz and never as a literal. A build with
  // MICRON_BB_REDZONE=1 routes on size + __rz <= __class_small, which shifts the last small request
  // down by __rz; a test spelling 1024 would pass at one setting and fail at the other.
  sb::test_case("allocation_extent is a fixed point across the tier boundary");
  {
    using A = micron::bb_allocator;
    const usize S = micron::bb::__class_small;
    const usize R = micron::bb::__rz;
    const usize last_small = S - R;
    bool ok = true;
    if ( A::allocation_extent(0, 16) != 0 ) ok = false;
    if ( A::allocation_extent(last_small - 15, 16) != last_small ) ok = false;
    if ( A::allocation_extent(last_small, 16) != last_small ) ok = false;
    if ( A::allocation_extent(last_small + 1, 16) != 2 * S - R ) ok = false;
    if ( A::allocation_extent(8, 32) != 32 - R ) ok = false;
    for ( usize s : { usize{ 3 }, usize{ 5 }, usize{ 7 }, usize{ 12 }, usize{ 24 }, usize{ 48 }, usize{ 96 } } ) {
      for ( usize k = 1; k <= 3000 && ok; ++k ) {
        const usize e = A::allocation_extent(k * s, 16);
        const usize usable = (e / s) * s;
        if ( A::allocation_extent(usable, 16) != e ) ok = false;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("over-aligned blocks free through the same path as plain ones");
  {
    bool ok = true;
    // 8192 is deliberately absent: attach() aligns a region base to __default_max_alignment (4096)
    // and aligned_balloc refuses A > base_align, so an 8192 cell would pass or fail on where the
    // page source happened to land the region -- a coin flip, not a test.
    for ( usize a : { usize{ 32 }, usize{ 64 }, usize{ 128 }, usize{ 256 }, usize{ 1024 }, usize{ 4096 } } ) {
      for ( usize n : { usize{ 1 }, usize{ 33 }, usize{ 4095 }, usize{ 4096 }, usize{ 100000 } } ) {
        auto c = micron::bb::aligned_balloc(a, n);
        if ( c.ptr == nullptr ) { ok = false; break; }
        if ( (reinterpret_cast<uintptr_t>(c.ptr) & (a - 1)) != 0 ) { ok = false; break; }
        if ( !micron::bb::is_present(c.ptr) ) { ok = false; break; }
        __builtin_memset(c.ptr, 0x3C, n);
        // the plain dealloc, deliberately -- alignment must not be needed to free
        micron::bb::dealloc(c.ptr);
        if ( micron::bb::is_present(c.ptr) ) { ok = false; break; }
      }
      if ( !ok ) break;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // EXPECTED OUTPUT: this case prints one line, "bbmalloc: bad or double free at 0x...". That is
  // MICRON_BB_DOUBLE_FREE_ACTION=1 -- report and refuse -- working, and it is the only evidence a
  // passing run gives that the refusal happened at all. Do not silence it; a clean log here would
  // mean the second free was accepted.
  sb::test_case("is_present goes false after a free, so a double free is a no-op");
  {
    auto c = micron::bb::balloc(777);
    sb::require(c.ptr != nullptr && micron::bb::is_present(c.ptr));
    micron::bb::dealloc(c.ptr);
    sb::require(!micron::bb::is_present(c.ptr));
    micron::bb::dealloc(c.ptr);      // must not double-release
    sb::require(!micron::bb::is_present(c.ptr));
  }
  sb::end_test_case();

  sb::test_case("resize preserves the prefix and clamps the copy");
  {
    bool ok = true;
    for ( usize n : { usize{ 16 }, usize{ 1000 }, usize{ 9000 } } ) {
      auto c = micron::bb::balloc(n);
      for ( usize i = 0; i < n; ++i ) c.ptr[i] = static_cast<byte>(i & 0xFF);
      auto grown = micron::bb::resize(c, n * 4, n, 32);
      if ( grown.ptr == nullptr ) { ok = false; break; }
      for ( usize i = 0; i < n; ++i )
        if ( grown.ptr[i] != static_cast<byte>(i & 0xFF) ) { ok = false; break; }
      // shrink: the copy must clamp at the NEW size, not the old one
      auto shrunk = micron::bb::resize(grown, n / 2 + 1, n * 4, 32);
      if ( shrunk.ptr == nullptr ) { ok = false; break; }
      for ( usize i = 0; i < n / 2 + 1 && i < n; ++i )
        if ( shrunk.ptr[i] != static_cast<byte>(i & 0xFF) ) { ok = false; break; }
      micron::bb::dealloc(shrunk.ptr);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (e) provenance. within() is approximate BY CONSTRUCTION -- it answers from a [low, high]
  //     watermark, so it can say yes inside a hole. The direction it must get right is the other
  //     one: an address nowhere near the pool is not ours.
  sb::test_case("is_present/within reject foreign pointers");
  {
    u64 on_stack = 0xDEAD'BEEFull;
    sb::require(!micron::bb::is_present(&on_stack));
    sb::require(micron::bb::query_size(&on_stack) == 0);
    sb::require(!micron::bb::is_present(nullptr));
    // a low address can never be inside a mapped span
    sb::require(!micron::bb::within(reinterpret_cast<const void *>(usize{ 0x40 })));
  }
  sb::end_test_case();

  sb::test_case("within says yes for a live block");
  {
    auto c = micron::bb::balloc(4096);
    sb::require(c.ptr != nullptr);
    sb::require(micron::bb::within(c.ptr));
    micron::bb::dealloc(c.ptr);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (f) growth -- micron's defaults, not bb's
  //
  // bbmalloc takes memory through attach() from ONE compile-time source and never grows on its own.
  // The page-per-allocation stub this replaced was effectively unbounded, and the cells that run
  // this file pass no pool macro at all, so bb_alloc.hpp defaults MICRON_BB_PORT_POOL +
  // MICRON_BB_PORT_GROW and the heap attaches another port::page_alloc region on exhaustion. Without
  // that, a container growing past the initial pool gets exc<critical_error>, which under (K) is
  // BUG() and on a kernel booted panic_on_oops=1 takes the machine down.
  //
  // NEGATIVE CONTROL: build with -DMICRON_BB_NO_GROW and this case must FAIL (exit 6) -- measured,
  // the same workload then throws critical_error at the pool boundary. A gate that cannot fail is
  // worse than no gate.
  sb::test_case("the heap attaches another region rather than failing at the initial pool");
  {
    const usize cap0 = micron::bb::capacity();
    const u32 regions0 = micron::bb::__the_heap.count;
    sb::require(cap0 != 0 && regions0 != 0);

    // three times the initial pool, held live so nothing can be reused
    const usize per = 1u << 20;
    const usize want = cap0 * 3;
    micron::vector<micron::vector<u64>> held;
    usize live = 0;
    while ( live < want ) {
      micron::vector<u64> v;
      for ( usize i = 0; i < per / sizeof(u64); ++i ) v.push_back(static_cast<u64>(i));
      live += per;
      held.push_back(micron::move(v));
    }
    sb::require(micron::bb::capacity() > cap0);
    sb::require(micron::bb::__the_heap.count > regions0);
    sb::require(micron::bb::__the_heap.count <= micron::bb::__max_regions);

    bool ok = true;
    for ( usize i = 0; i < held.size() && ok; ++i )
      if ( held[i].size() != per / sizeof(u64) || held[i][0] != 0 ) ok = false;
    sb::require(ok);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
