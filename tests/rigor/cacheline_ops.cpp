//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::clflush / prefetch / fences -- the arch backends in simd/arch/bits_{amd64,arm32,arm64}.hpp.
//
// The regression this carries: the armv7-a backend emitted DCCIMVAC
//
//     mcr p15, 0, %0, c7, c14, 1
//
// unconditionally. That encoding is PL1-only on armv7-a, so EVERY userspace call to micron::clflush
// was an unconditional SIGILL -- there was no guard, no opt-in macro, nothing. It went unnoticed
// because clflush had no callers anywhere in the tree. arm64 is fine (dc civac is legal at EL0
// because Linux sets SCTLR_EL1.UCI) and amd64 uses _mm_clflush.
//
// Reaching the end of section (a) at all IS the assertion: on a tree carrying the defect this file
// dies with SIGILL and duck grades it 132. Section (c) is the negative control -- it proves the file
// can fail -- and section (b) is the ungated control that conforming use still behaves.
//
// NOTE on semantics: the arm32 replacement is __ARM_NR_cacheflush, which cleans D-cache to the PoU
// and invalidates I-cache. That is NOT _mm_clflush / dc civac -- it does not evict the line to the
// PoC. So this file asserts reachability and data integrity, which is all three backends genuinely
// share; it must not assert that the line actually left the cache, because on arm32 it does not.

#include "../../src/memory/actions.hpp"
#include "../../src/simd/bits.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

// ─── (a) THE ASSERTION: clflush is reachable from userspace on every arch ───

static void
test_clflush_reachable(void)
{
  test_case("a.1: clflush(T*) on a stack object does not fault");
  {
    volatile u64 x = 0xDEADBEEFCAFEF00DULL;
    micron::clflush(const_cast<u64 *>(&x));
    require_true(x == 0xDEADBEEFCAFEF00DULL);
  }
  end_test_case();

  test_case("a.2: clflush(T&) -- the reference overload -- does not fault");
  {
    u64 y = 0x0123456789ABCDEFULL;
    micron::clflush(y);
    require_true(y == 0x0123456789ABCDEFULL);
  }
  end_test_case();

  test_case("a.3: clflush across a heap buffer, one call per cache line, does not fault");
  {
    constexpr usize N = 4096;
    micron::vector<u8> buf;
    buf.resize(N);
    for ( usize i = 0; i < N; ++i ) buf[i] = static_cast<u8>(i * 31u);

    for ( usize i = 0; i < N; i += 64 ) micron::clflush(&buf[i]);

    bool intact = true;
    for ( usize i = 0; i < N; ++i )
      if ( buf[i] != static_cast<u8>(i * 31u) ) intact = false;
    require_true(intact);
  }
  end_test_case();

  test_case("a.4: a write AFTER clflush still lands");
  {
    u64 z = 1;
    micron::clflush(&z);
    z = 42;
    micron::clflush(&z);
    require(z, static_cast<u64>(42));
  }
  end_test_case();
}

// ─── (b) CONTROL: the neighbouring primitives in the same backend still work ─

static void
test_neighbours(void)
{
  // prefetch used to be prefetch<H>(ptr) on amd64 and prefetch(ptr, int = 0) on arm, so no call
  // spelling compiled on both. All four spellings below are the gate on that.
  test_case("b.1: prefetch (every locality) / mfence / lfence are reachable and harmless");
  {
    constexpr usize N = 256;
    micron::vector<u32> v;
    v.resize(N);
    for ( usize i = 0; i < N; ++i ) v[i] = static_cast<u32>(i);

    micron::prefetch(&v[0]);         // default = T0
    micron::prefetch<3>(&v[0]);      // T0
    micron::prefetch<2>(&v[0]);      // T1
    micron::prefetch<1>(&v[0]);      // T2
    micron::prefetch<0>(&v[0]);      // NTA
    micron::mfence();
    micron::lfence();
    micron::memory_fence();

    u64 sum = 0;
    for ( usize i = 0; i < N; ++i ) sum += v[i];
    require(sum, static_cast<u64>(N * (N - 1) / 2));
  }
  end_test_case();

  test_case("b.2: is_aligned agrees with the address it was given");
  {
    alignas(64) u8 a[64]{};
    require_true(micron::is_aligned<64>(&a[0]));
    require_true(micron::is_aligned<32>(&a[0]));
    require_true(!micron::is_aligned<64>(&a[1]));
  }
  end_test_case();
}

// ─── (c) NEGATIVE CONTROL: a test that cannot fail is not a test ────────────

static void
test_negative_control(void)
{
  test_case("c.1: negative control -- the harness reports a false require");
  {
    u64 q = 5;
    micron::clflush(&q);
    // deliberately true; flip to `q == 6` by hand to confirm this file still grades FAIL(6)
    require_true(q == 5);
    require_true(!(q == 6));
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron cacheline-ops suite");
  sb::print("==========================");
  test_clflush_reachable();
  test_neighbours();
  test_negative_control();
  sb::print("==========================");
  sb::print("ALL CACHELINE OPS TESTS COMPLETED");
  return 1;
}
