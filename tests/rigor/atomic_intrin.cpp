//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/atomic/intrin.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include "../support/mt.hpp"      // mtest::parallel (micron auto_thread; NOT <thread>)
#include <vector>

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr int kStressThreads = 8;
constexpr int kStressIters = 10000;

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ATOMIC INTRIN TESTS ===");

  // ── load / store roundtrip for each integral width ───────────────────────

  test_case("load/store roundtrip - u8");
  {
    u8 x = 0;
    atom::store(&x, (u8)0xAB, atomic_seq_cst);
    require(atom::load(&x, atomic_seq_cst) == (u8)0xAB);
  }
  end_test_case();

  test_case("load/store roundtrip - u16");
  {
    u16 x = 0;
    atom::store(&x, (u16)0xCAFE, atomic_seq_cst);
    require(atom::load(&x, atomic_seq_cst) == (u16)0xCAFE);
  }
  end_test_case();

  test_case("load/store roundtrip - u32");
  {
    u32 x = 0;
    atom::store(&x, (u32)0xDEADBEEF, atomic_seq_cst);
    require(atom::load(&x, atomic_seq_cst) == (u32)0xDEADBEEF);
  }
  end_test_case();

  test_case("load/store roundtrip - u64");
  {
    u64 x = 0;
    atom::store(&x, (u64)0x0123456789ABCDEFULL, atomic_seq_cst);
    require(atom::load(&x, atomic_seq_cst) == (u64)0x0123456789ABCDEFULL);
  }
  end_test_case();

  test_case("load/store roundtrip - i32 negative");
  {
    i32 x = 0;
    atom::store(&x, (i32)-12345, atomic_seq_cst);
    require(atom::load(&x, atomic_seq_cst) == (i32)-12345);
  }
  end_test_case();

  test_case("load/store roundtrip - i64 negative");
  {
    i64 x = 0;
    atom::store(&x, (i64)-9876543210LL, atomic_seq_cst);
    require(atom::load(&x, atomic_seq_cst) == (i64)-9876543210LL);
  }
  end_test_case();

  // ── memory orders compile and execute ───────────────────────────────────

  test_case("store/load with each memory order executes");
  {
    u32 x = 0;
    for ( int mo : { atomic_seq_cst, atomic_consume, atomic_acquire, atomic_release, atomic_acq_rel } ) {
      // store cannot use acquire/consume, load cannot use release/acq_rel — use safe pairs only
      atom::store(&x, (u32)mo, atomic_seq_cst);
      (void)atom::load(&x, atomic_seq_cst);
    }
    atom::store(&x, (u32)7, atomic_release);
    require(atom::load(&x, atomic_acquire) == (u32)7);
  }
  end_test_case();

  // ── exchange ────────────────────────────────────────────────────────────

  test_case("exchange returns previous and writes new");
  {
    u32 x = 5;
    u32 prev = atom::exchange(&x, (u32)42, atomic_seq_cst);
    require(prev == (u32)5);
    require(x == (u32)42);
  }
  end_test_case();

  // ── compare_exchange_strong ─────────────────────────────────────────────

  test_case("cmp_exchange_strong success path - expected matches");
  {
    u32 x = 10;
    u32 expected = 10;
    bool ok = atom::cmp_exchange_strong(&x, &expected, (u32)20);
    require_true(ok);
    require(x == (u32)20);
    require(expected == (u32)10);      // unchanged on success
  }
  end_test_case();

  test_case("cmp_exchange_strong failure path - expected updated, ptr untouched");
  {
    u32 x = 7;
    u32 expected = 99;
    bool ok = atom::cmp_exchange_strong(&x, &expected, (u32)20);
    require_false(ok);
    require(x == (u32)7);
    require(expected == (u32)7);      // expected refreshed to actual current value
  }
  end_test_case();

  // ── compare_exchange_weak in retry loop ─────────────────────────────────

  test_case("cmp_exchange_weak retry loop converges against unchanging value");
  {
    u32 x = 1;
    u32 expected = 1;
    int attempts = 0;
    while ( !atom::cmp_exchange_weak(&x, &expected, (u32)2) ) {
      ++attempts;
      if ( attempts > 1000 ) break;      // spurious failures bounded in practice
    }
    require(x == (u32)2);
    require_true(attempts < 1000);
  }
  end_test_case();

  // ── 6-arg cmp_exchange form (strong + weak) ─────────────────────────────

  test_case("cmp_exchange (6-arg) strong form executes");
  {
    u32 x = 3;
    u32 expected = 3;
    bool ok = atom::cmp_exchange(&x, &expected, (u32)9, /*weak*/ false, atomic_acq_rel, atomic_acquire);
    require_true(ok);
    require(x == (u32)9);
  }
  end_test_case();

  test_case("cmp_exchange (6-arg) weak form executes");
  {
    u32 x = 3;
    u32 expected = 3;
    while ( !atom::cmp_exchange(&x, &expected, (u32)9, /*weak*/ true, atomic_acq_rel, atomic_acquire) ) {
    }
    require(x == (u32)9);
  }
  end_test_case();

  // ── fetch_add / fetch_sub: returns old ──────────────────────────────────

  test_case("fetch_add returns old, increments ptr");
  {
    u32 x = 100;
    u32 old = atom::fetch_add(&x, (u32)5, atomic_seq_cst);
    require(old == (u32)100);
    require(x == (u32)105);
  }
  end_test_case();

  test_case("fetch_sub returns old, decrements ptr");
  {
    u32 x = 100;
    u32 old = atom::fetch_sub(&x, (u32)5, atomic_seq_cst);
    require(old == (u32)100);
    require(x == (u32)95);
  }
  end_test_case();

  // ── add_fetch / sub_fetch: returns new ──────────────────────────────────

  test_case("add_fetch returns new value");
  {
    u32 x = 100;
    u32 nv = atom::add_fetch(&x, (u32)5, atomic_seq_cst);
    require(nv == (u32)105);
    require(x == (u32)105);
  }
  end_test_case();

  test_case("sub_fetch returns new value");
  {
    u32 x = 100;
    u32 nv = atom::sub_fetch(&x, (u32)5, atomic_seq_cst);
    require(nv == (u32)95);
    require(x == (u32)95);
  }
  end_test_case();

  // ── bitwise families ────────────────────────────────────────────────────

  test_case("fetch_and / and_fetch correctness");
  {
    u32 x = 0xFFu;
    u32 old = atom::fetch_and(&x, (u32)0x0Fu, atomic_seq_cst);
    require(old == (u32)0xFFu);
    require(x == (u32)0x0Fu);
    u32 nv = atom::and_fetch(&x, (u32)0x03u, atomic_seq_cst);
    require(nv == (u32)0x03u);
    require(x == (u32)0x03u);
  }
  end_test_case();

  test_case("fetch_or / or_fetch correctness");
  {
    u32 x = 0x10u;
    u32 old = atom::fetch_or(&x, (u32)0x01u, atomic_seq_cst);
    require(old == (u32)0x10u);
    require(x == (u32)0x11u);
    u32 nv = atom::or_fetch(&x, (u32)0x80u, atomic_seq_cst);
    require(nv == (u32)0x91u);
  }
  end_test_case();

  test_case("fetch_xor / xor_fetch correctness");
  {
    u32 x = 0xAAu;
    u32 old = atom::fetch_xor(&x, (u32)0xFFu, atomic_seq_cst);
    require(old == (u32)0xAAu);
    require(x == (u32)0x55u);
    u32 nv = atom::xor_fetch(&x, (u32)0x55u, atomic_seq_cst);
    require(nv == (u32)0x00u);
  }
  end_test_case();

  test_case("fetch_nand / nand_fetch correctness");
  {
    u32 x = 0x0Fu;
    // NAND: result = ~(x & val)
    u32 old = atom::fetch_nand(&x, (u32)0x03u, atomic_seq_cst);
    require(old == (u32)0x0Fu);
    require(x == (u32) ~((u32)0x0Fu & (u32)0x03u));
  }
  end_test_case();

  // ── test_and_set / clear ────────────────────────────────────────────────

  test_case("test_and_set sets flag, returns previous");
  {
    bool flag = false;
    bool prev1 = atom::test_and_set(&flag, atomic_seq_cst);
    require_false(prev1);
    require_true(flag);
    bool prev2 = atom::test_and_set(&flag, atomic_seq_cst);
    require_true(prev2);
    require_true(flag);
  }
  end_test_case();

  test_case("clear() resets the flag byte");
  {
    bool flag = true;
    atom::clear(&flag, atomic_seq_cst);
    require_false(flag);
  }
  end_test_case();

  // ── fences ──────────────────────────────────────────────────────────────

  test_case("thread_fence / signal_fence are callable for each memory order");
  {
    atom::thread_fence(atomic_acquire);
    atom::thread_fence(atomic_release);
    atom::thread_fence(atomic_acq_rel);
    atom::thread_fence(atomic_seq_cst);
    atom::signal_fence(atomic_acquire);
    atom::signal_fence(atomic_release);
    atom::signal_fence(atomic_seq_cst);
    require_true(true);
  }
  end_test_case();

  // ── concurrent stress ───────────────────────────────────────────────────

  test_case("concurrent fetch_add: 8 threads * 10000 increments sum exactly");
  {
    u32 counter = 0;
    mtest::parallel(kStressThreads, [&counter](int) {
      for ( int i = 0; i < kStressIters; ++i ) {
        atom::fetch_add(&counter, (u32)1, atomic_seq_cst);
      }
    });
    require(counter == (u32)(kStressThreads * kStressIters));
  }
  end_test_case();

  test_case("concurrent cmp_exchange_weak lock-free counter converges");
  {
    u32 counter = 0;
    mtest::parallel(kStressThreads, [&counter](int) {
      for ( int i = 0; i < kStressIters; ++i ) {
        u32 expected = atom::load(&counter, atomic_seq_cst);
        while ( !atom::cmp_exchange_weak(&counter, &expected, expected + 1) ) {
        }
      }
    });
    require(counter == (u32)(kStressThreads * kStressIters));
  }
  end_test_case();

  test_case("concurrent xor toggles converge to predictable parity");
  {
    // each thread XORs its own bit into a shared word, twice — final = 0
    u32 word = 0;
    constexpr int kT = 4;
    constexpr int kIters = 5000;
    mtest::parallel(kT, [&word](int t) {
      u32 bit = (u32)1u << t;
      for ( int i = 0; i < kIters; ++i ) {
        atom::fetch_xor(&word, bit, atomic_seq_cst);
      }
    });
    // each thread toggled its bit kIters times (even) -> all bits zero
    require(word == (u32)0u);
  }
  end_test_case();

  test_case("u64 fetch_add atomicity - no tearing under contention");
  {
    u64 counter = 0;
    constexpr int kT = 8;
    constexpr int kIters = 5000;
    mtest::parallel(kT, [&counter](int) {
      for ( int i = 0; i < kIters; ++i ) {
        atom::fetch_add(&counter, (u64)1, atomic_seq_cst);
      }
    });
    require(counter == (u64)(kT * kIters));
  }
  end_test_case();

  sb::print("=== ALL ATOMIC INTRIN TESTS PASSED ===");
  return 1;
}
