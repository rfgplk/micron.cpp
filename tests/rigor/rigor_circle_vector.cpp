// Exhaustive, adversarial rigor suite for micron::circle_vector<T, N, Sf>.
//
// circle_vector is a fixed-capacity (power-of-two N), stack-allocated ring
// buffer: push/move_back/emplace_back append at the head and OVERWRITE the
// oldest element once full; pop/pop_front drop the oldest; front/back/operator[]
// are logical (index 0 == oldest). This suite drives the API against the
// ref_ring oracle (tests/support/vector_rigor.hpp) with heavy wrap-around, at a
// matrix of N in {2,4,16,256} x element types. Memory correctness for a stack
// ring == no OOB (ASan) + Tracked ctor/dtor balance; nested
// circle_vector<vector<u32>> exercises resource-owning elements. at() throws
// library_error on out-of-range.
//
// Build: `duck build tests/rigor/rigor_circle_vector.cpp`; run bin/rigor_circle_vector.

#include "../../src/io/console.hpp"

#include "../../src/vector/circle_vector.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
#include "../support/oracles.hpp"
#include "../support/tracked_types.hpp"
#include "../support/vector_rigor.hpp"

using namespace snowball;
using mtest::band;
using mtest::prng;

#ifndef RIGOR_ITERS
#define RIGOR_ITERS 10000
#endif
static constexpr usize ITERS = RIGOR_ITERS;

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

template<usize N, typename E>
static void
run_props(void)
{
  using CV = micron::circle_vector<E, N>;

  test_case("cir push/move_back/emplace/pop/pop_front fuzz vs ring oracle");
  {
    prng rng(0x9001u + N);
    CV cv;
    mtest::ref_ring<N> r;
    for ( usize it = 0; it < ITERS; ++it ) {
      u64 op = rng.next() % 6;
      if ( op <= 2 ) {

        u64 raw = mtest::gen_raw(rng, band::full);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        if ( op == 0 )
          cv.push_back(e);
        else if ( op == 1 )
          cv.move_back(micron::move(e));
        else
          mtest::elem<E>::emplace(cv, raw);
        r.push(k);
      } else if ( op == 3 ) {

        if ( !cv.empty() ) {
          u64 of = r.front();
          E got = cv.pop();
          r.pop_front();
          ck(mtest::elem<E>::key(got) == of, "pop-value", it);
        }
      } else if ( op == 4 ) {
        if ( !cv.empty() ) {
          cv.pop_front();
          r.pop_front();
        }
      } else {

        if ( (rng.next() & 7u) == 0u ) {
          cv.clear();
          r.clear();
        }
      }
      ck(mtest::ring_eq<E>(cv, r), "ring", it);
      ck(cv.size() == r.size() && cv.full() == r.full() && cv.empty() == r.empty(), "state", it);
      if ( !cv.empty() ) {
        ck(mtest::elem<E>::key(cv.front()) == r.front(), "front", it);
        ck(mtest::elem<E>::key(cv.back()) == r.back(), "back", it);
        usize idx = static_cast<usize>(rng.next_in(cv.size()));
        ck(mtest::elem<E>::key(cv[idx]) == r[idx], "op[]", it);
        ck(mtest::elem<E>::key(cv.at(idx)) == r[idx], "at", it);
      }
    }
  }
  end_test_case();

  test_case("cir wrap-around holds last N");
  {
    prng rng(0x9100u + N);
    CV cv;
    mtest::ref_ring<N> r;
    for ( usize it = 0; it < ITERS; ++it ) {
      u64 raw = mtest::gen_raw(rng, band::full);
      E e = mtest::elem<E>::make(raw);
      cv.push_back(e);
      r.push(mtest::elem<E>::key(e));
      ck(mtest::ring_eq<E>(cv, r), "wrap", it);
    }
    ck(cv.full(), "ended-full", 0);
    ck(cv.size() == N, "ended-size-N", 0);
  }
  end_test_case();

  test_case("cir iteration logical order");
  {
    prng rng(0x9200u + N);
    for ( usize it = 0; it < 4000; ++it ) {
      CV cv;
      mtest::ref_ring<N> r;
      usize pushes = static_cast<usize>(rng.next_in(3 * N + 1));
      for ( usize i = 0; i < pushes; ++i ) {
        u64 raw = mtest::gen_raw(rng, band::full);
        E e = mtest::elem<E>::make(raw);
        cv.push_back(e);
        r.push(mtest::elem<E>::key(e));
      }
      usize j = 0;
      bool ok = true;
      for ( auto i = cv.begin(); i != cv.end(); ++i, ++j )
        if ( mtest::elem<E>::key(*i) != r[j] ) {
          ok = false;
          break;
        }
      ck(ok && j == cv.size(), "iterate", it);
    }
  }
  end_test_case();
}

template<usize N, typename E>
static void
run_copy_caps(void)
{
  using CV = micron::circle_vector<E, N>;

  test_case("cir copy/move independence + flags vs oracle");
  {
    prng rng(0x9300u + N);
    for ( usize it = 0; it < 4000; ++it ) {
      CV cv;
      mtest::ref_ring<N> r;
      usize pushes = static_cast<usize>(rng.next_in(2 * N + 1));
      for ( usize i = 0; i < pushes; ++i ) {
        u64 raw = mtest::gen_raw(rng, band::full);
        E e = mtest::elem<E>::make(raw);
        cv.push_back(e);
        r.push(mtest::elem<E>::key(e));
      }

      CV cp(cv);
      ck(mtest::ring_eq<E>(cp, r), "copy", it);
      if ( !cv.empty() ) {
        cp[0] = mtest::elem<E>::make(~r[0]);
        ck(mtest::elem<E>::key(cv[0]) == r[0], "copy-indep", it);
      }

      CV fresh(cv);
      CV mv(micron::move(fresh));
      ck(mtest::ring_eq<E>(mv, r), "move", it);

      CV ca;
      ca = cv;
      ck(mtest::ring_eq<E>(ca, r), "copy-assign", it);
    }
  }
  end_test_case();

  test_case("cir capacity edges + at() OOB throws");
  {
    CV cv;
    require(cv.empty(), true);
    require(cv.capacity(), N);
    require(cv.max_size(), N);
    for ( usize i = 0; i < N; ++i ) cv.push_back(mtest::elem<E>::make(static_cast<u64>(i)));
    require(cv.full(), true);
    require(cv.size(), N);
    expect_throw_type<micron::except::library_error>([&] { (void)cv.at(N); });
    cv.clear();
    require(cv.empty(), true);
    require(cv.size(), 0u);
  }
  end_test_case();
}

static void
run_memory(void)
{
  using Tr = mtest::Tracked<15>;
  using CV = micron::circle_vector<Tr, 16>;

  test_case("cir Tracked lifetime balanced after churn");
  {
    Tr::reset();
    {
      prng rng(0xC1C1u);
      CV cv;
      for ( usize it = 0; it < 40000; ++it ) {
        u64 op = rng.next() % 4;
        if ( op <= 1 )
          cv.push_back(Tr(static_cast<int>(rng.next() & 0xffff)));
        else if ( op == 2 ) {
          if ( !cv.empty() ) (void)cv.pop();
        } else {
          if ( !cv.empty() ) cv.pop_front();
        }
      }
    }
    ck(Tr::live() == 0u, "tracked-balance", 0);
  }
  end_test_case();
}

static u64
rowkey(const micron::vector<u32> &r)
{
  u64 h = 1469598103934665603ull;
  h = (h ^ static_cast<u64>(r.size())) * 1099511628211ull;
  for ( usize i = 0; i < r.size(); ++i ) h = (h ^ static_cast<u64>(r[i])) * 1099511628211ull;
  return h;
}

static micron::vector<u32>
mkrow(prng &rng)
{
  micron::vector<u32> r;
  usize len = static_cast<usize>(rng.next_in(8));
  u32 base = static_cast<u32>(rng.next());
  for ( usize i = 0; i < len; ++i ) r.push_back(base + static_cast<u32>(i) * 2654435761u);
  return r;
}

static void
run_nested(void)
{
  using V = micron::vector<u32>;
  using CVV = micron::circle_vector<V, 16>;

  test_case("circle_vector<vector<u32>,16> nested ring vs oracle");
  {
    prng rng(0xD00Du);
    CVV cv;
    mtest::ref_ring<16> r;
    for ( usize it = 0; it < ITERS; ++it ) {
      u64 op = rng.next() % 4;
      if ( op <= 1 ) {
        V row = mkrow(rng);
        u64 k = rowkey(row);
        cv.move_back(micron::move(row));
        r.push(k);
      } else if ( op == 2 ) {
        if ( !cv.empty() ) {
          cv.pop_front();
          r.pop_front();
        }
      } else {
        if ( (rng.next() & 3u) == 0u ) {
          cv.clear();
          r.clear();
        }
      }
      bool ok = static_cast<usize>(cv.size()) == r.size();
      for ( usize i = 0; ok && i < r.size(); ++i )
        if ( rowkey(cv[i]) != r[i] ) ok = false;
      ck(ok, "nested-ring", it);
    }

    CVV cp(cv);
    if ( !cv.empty() ) {
      u64 before = rowkey(cv[0]);
      cp[0].push_back(0xFFu);
      ck(rowkey(cv[0]) == before, "nested-copy-indep", 0);
    }
  }
  end_test_case();
}

template<usize N>
static void
run_caps(const char *nm)
{
  sb::print("--- circle_vector<*, ", nm, "> ---");
  run_props<N, u8>();
  run_props<N, u16>();
  run_props<N, u32>();
  run_props<N, u64>();
  run_props<N, mtest::big>();
  run_props<N, mtest::Tracked<0>>();
  run_copy_caps<N, u32>();
  run_copy_caps<N, mtest::big>();
  run_copy_caps<N, mtest::Tracked<1>>();
}

int
main(int, char **)
{
  sb::print("=== CIRCLE_VECTOR RIGOR ===");

  run_caps<2>("N=2");
  run_caps<4>("N=4");
  run_caps<16>("N=16");
  run_caps<256>("N=256");

  sb::print("--- memory / lifetime (Tracked) ---");
  run_memory();

  sb::print("--- nested containers (circle_vector<vector<...>>) ---");
  run_nested();

  sb::print("=== CIRCLE_VECTOR RIGOR DONE ===");
  return 1;
}
