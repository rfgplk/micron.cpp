// rigor_algo_accumulate_fold.cpp — snowball suite for
//   src/algorithm/accumulate.hpp + src/algorithm/fold.hpp
//
// Coverage:
//   accumulate            (ptr/container × default/Fn × no-limit/limit ×
//                          runtime/compile-time + map overloads)
//   fold_left             (ptr/container; Fn signature is Fn(A, const T*))
//   fold_right            (ptr/container; Fn signature is Fn(const T*, A))
//   fold                  (alias for fold_left)
//   fold_left_counted     (returns pair<A, usize>)
//   fold_left_while       (pred-bounded)
//   fold_build            (container-returning fold)
//   fold_left(map)        (Fn takes (A, K&, V&))
//
// NOTE: accumulate's Fn takes (A, const T&) — REFERENCE — while fold_left's
// Fn takes (A, const T*) — POINTER. Easy to mix up.

#include "../../src/algorithm/accumulate.hpp"
#include "../../src/algorithm/fold.hpp"
#include "../../src/maps/heap_swiss.hpp"
#include "../../src/maps/rb_map.hpp"

#include "../support/algo_rigor.hpp"

using namespace mtest::rigor;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

static int
mul_ref_acc(int acc, const int &x)
{
  return acc + x;
}

int
main()
{
  sb::print("=== ALGO/ACCUMULATE+FOLD RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // accumulate
  // ════════════════════════════════════════════════════════════════════

  test_case("accumulate[ptr,init=0] sums sequence");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    auto s = micron::accumulate(a, a + 5, 0);
    require(s, 15);
  }
  end_test_case();

  test_case("accumulate[ptr,init=100] adds to running");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    auto s = micron::accumulate(a, a + 5, 100);
    require(s, 115);
  }
  end_test_case();

  test_case("accumulate[ptr,Fn=ref] multiplication via fn");
  {
    int a[4] = { 2, 3, 4, 5 };
    auto p = micron::accumulate(a, a + 4, 1, [](int acc, const int &x) { return acc * x; });
    require(p, 120);
  }
  end_test_case();

  test_case("accumulate[ptr,init=0,limit=3] bounded");
  {
    int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto s = micron::accumulate(a, a + 10, 0, usize(3));
    require(s, 6);
  }
  end_test_case();

  test_case("accumulate[ptr,Fn,limit] bounded with fn");
  {
    int a[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    auto s = micron::accumulate(a, a + 10, 0, [](int acc, const int &x) { return acc + x * 2; }, usize(5));
    require(s, 10);      // 2+2+2+2+2 = 10
  }
  end_test_case();

  test_case("accumulate[container] default init");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto s = micron::accumulate(v, 0);      // explicit init avoids overload ambig.
    require(s, 15);
  }
  end_test_case();

  test_case("accumulate[container,Fn]");
  {
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto p = micron::accumulate(v, 1, [](int acc, const int &x) { return acc * x; });
    require(p, 24);      // 1*2*3*4
  }
  end_test_case();

  test_case("accumulate[container,init,limit] bounded");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i + 1;
    auto s = micron::accumulate(v, 0, usize(3));
    require(s, 6);
  }
  end_test_case();

  test_case("accumulate[ptr] empty range returns init");
  {
    int a[1];
    auto s = micron::accumulate(a, a, 42);
    require(s, 42);
  }
  end_test_case();

  test_case("accumulate adversarial sizes against naive");
  {
    for ( usize n : kAdversarialSizes ) {
      if ( n == 0 ) continue;
      int a[1024];
      pat_sorted(a, n);
      umax_t actual = static_cast<umax_t>(micron::accumulate(a, a + n, umax_t(0)));
      umax_t expected = ref::naive_accumulate(a, n, umax_t(0));
      require(actual, expected);
    }
  }
  end_test_case();

  test_case("accumulate<Fn>(ptr) compile-time");
  {
    int a[4] = { 1, 2, 3, 4 };
    auto s = micron::accumulate<mul_ref_acc>(a, a + 4, 0);
    require(s, 10);
  }
  end_test_case();

  test_case("accumulate<Fn>(ptr,limit) compile-time bounded");
  {
    int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto s = micron::accumulate<mul_ref_acc>(a, a + 10, 0, usize(4));
    require(s, 10);      // 1+2+3+4
  }
  end_test_case();

  // accumulate map overloads ─────────────────────────────────────────

  test_case("accumulate[heap_swiss_map] sums values");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i + 1);
    int s = micron::accumulate(m, 0);
    require(s, 55);      // 1+2+...+10
  }
  end_test_case();

  test_case("accumulate[heap_swiss_map,Fn] custom fn over k,v");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 5; ++i ) m.insert(i, 10);
    int s = micron::accumulate(m, 0, [](int acc, const int &k, const int &v) { return acc + k + v; });
    // sum of (k + v) for k in {0..4}, v always 10: (0+1+2+3+4) + (5*10) = 10 + 50 = 60
    require(s, 60);
  }
  end_test_case();

  test_case("accumulate[rb_map] ordered iteration");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 8; ++i ) m.insert(i, i * 2);
    int s = micron::accumulate(m, 0);
    require(s, 56);      // 0+2+4+...+14
  }
  end_test_case();

  property_test(
      "accumulate vs naive (10k random)",
      [](u32 raw_n, u32 raw_init) {
        usize n = (raw_n & 0x3f) + 1;
        int buf[64];
        prng rng(raw_n + 67);
        pat_random_small(buf, n, rng, -1000, 1000);
        int init = static_cast<int>(raw_init & 0xff);
        umax_t actual = static_cast<umax_t>(micron::accumulate(buf, buf + n, umax_t(init)));
        umax_t expected = ref::naive_accumulate(buf, n, umax_t(init));
        require(actual, expected);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // fold_left  (Fn signature: Fn(A, const T*))
  // ════════════════════════════════════════════════════════════════════

  test_case("fold_left[ptr,Fn-T*] sum");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    auto s = micron::fold_left(a, a + 5, 0, [](int acc, const int *x) { return acc + *x; });
    require(s, 15);
  }
  end_test_case();

  test_case("fold_left[ptr,Fn-T*] empty returns init");
  {
    int a[1];
    auto s = micron::fold_left(a, a, 100, [](int acc, const int *) { return acc; });
    require(s, 100);
  }
  end_test_case();

  test_case("fold_left[ptr,Fn-T*,limit] bounded");
  {
    int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto s = micron::fold_left(a, a + 10, 0, [](int acc, const int *x) { return acc + *x; }, usize(4));
    require(s, 10);      // 1+2+3+4
  }
  end_test_case();

  test_case("fold_left[container,Fn-T*]");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto s = micron::fold_left(v, 0, [](int acc, const int *x) { return acc + *x; });
    require(s, 15);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fold_right  (Fn signature: Fn(const T*, A))
  // ════════════════════════════════════════════════════════════════════

  test_case("fold_right[ptr,Fn-T*] reverse-order fold (sum still correct)");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    auto s = micron::fold_right(a, a + 5, [](const int *x, int acc) { return acc + *x; }, 0);
    require(s, 15);
  }
  end_test_case();

  test_case("fold_right[ptr,Fn-T*] string-like concat preserves reverse");
  {
    int a[4] = { 1, 2, 3, 4 };
    // Build a positional sum to detect order: acc * 10 + *x means
    // right-fold of [1,2,3,4] with init 0 → ((((0)*10 + 4)*10 + 3)*10 + 2)*10 + 1 = 4321
    auto s = micron::fold_right(a, a + 4, [](const int *x, int acc) { return acc * 10 + *x; }, 0);
    require(s, 4321);
  }
  end_test_case();

  test_case("fold_right[ptr] empty returns init");
  {
    int a[1];
    auto s = micron::fold_right(a, a, [](const int *, int acc) { return acc + 1; }, 99);
    require(s, 99);
  }
  end_test_case();

  test_case("fold_right[container,Fn-T*]");
  {
    micron::vector<int> v(4, 0);
    v[0] = 1;
    v[1] = 2;
    v[2] = 3;
    v[3] = 4;
    auto s = micron::fold_right(v, [](const int *x, int acc) { return acc * 10 + *x; }, 0);
    require(s, 4321);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fold = alias for fold_left
  // ════════════════════════════════════════════════════════════════════

  test_case("fold[ptr] aliases fold_left");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    auto s1 = micron::fold(a, a + 5, 0, [](int acc, const int *x) { return acc + *x; }, usize(5));
    auto s2 = micron::fold_left(a, a + 5, 0, [](int acc, const int *x) { return acc + *x; });
    require(s1, s2);
  }
  end_test_case();

  test_case("fold[container] aliases fold_left");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto s = micron::fold(v, 0, [](int acc, const int *x) { return acc + *x; });
    require(s, 15);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fold_left_counted (returns pair<A, usize>)
  // ════════════════════════════════════════════════════════════════════

  test_case("fold_left_counted[ptr] returns acc + count");
  {
    int a[7] = { 10, 20, 30, 40, 50, 60, 70 };
    auto pr = micron::fold_left_counted(a, a + 7, 0, [](int acc, const int *x) { return acc + *x; });
    require(pr.a, 280);
    require(pr.b, usize(7));
  }
  end_test_case();

  test_case("fold_left_counted[container]");
  {
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto pr = micron::fold_left_counted(v, 0, [](int acc, const int *x) { return acc + *x; });
    require(pr.a, 10);
    require(pr.b, usize(4));
  }
  end_test_case();

  test_case("fold_left_counted[ptr] empty returns init,0");
  {
    int a[1];
    auto pr = micron::fold_left_counted(a, a, 99, [](int acc, const int *) { return acc + 1; });
    require(pr.a, 99);
    require(pr.b, usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fold_left_while (pred-bounded)
  // ════════════════════════════════════════════════════════════════════

  test_case("fold_left_while stops when predicate fails");
  {
    int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto s = micron::fold_left_while(
        a, a + 10, 0, [](int acc, const int *x) { return acc + *x; }, [](int acc, const int *) { return acc < 10; });
    // Stop as soon as acc >= 10. Accumulating 1+2+3+4=10, pred(10) fails before adding 5.
    require(s, 10);
  }
  end_test_case();

  test_case("fold_left_while[container] same semantics");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i + 1;
    auto s = micron::fold_left_while(v, 0, [](int acc, const int *x) { return acc + *x; }, [](int acc, const int *) { return acc < 20; });
    // pred checked before fn. acc=15 still < 20 so we add 6 → acc=21,
    // then pred(21) false → stop. Final s = 21.
    require(s, 21);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fold_build (container-returning fold)
  // ════════════════════════════════════════════════════════════════════

  test_case("fold_build accumulates into a vector");
  {
    micron::vector<int> in(5, 0);
    for ( int i = 0; i < 5; ++i ) in[i] = i + 1;
    micron::vector<int> out;
    auto r = micron::fold_build(in, out, [](micron::vector<int> acc, const int *x) {
      acc.push_back(*x * 10);
      return acc;
    });
    require(r.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(r[i], (i + 1) * 10);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fold_left on map
  // ════════════════════════════════════════════════════════════════════

  test_case("fold_left[map] visits k/v");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 5; ++i ) m.insert(i, (i + 1) * 100);
    int s = micron::fold_left(m, 0, [](int acc, const int &k, const int &v) { return acc + k + v; });
    // sum(k+v) = (0+100)+(1+200)+(2+300)+(3+400)+(4+500) = 100+201+302+403+504 = 1510
    require(s, 1510);
  }
  end_test_case();

  property_test(
      "fold_left[ptr] sum vs naive (10k random)",
      [](u32 raw_n, u32 raw_init) {
        usize n = (raw_n & 0x3f) + 1;
        int buf[64];
        prng rng(raw_n + 71);
        pat_random_small(buf, n, rng, -100, 100);
        int init = static_cast<int>(raw_init & 0xff);
        auto actual = micron::fold_left(buf, buf + n, init, [](int acc, const int *x) { return acc + *x; });
        int expected = ref::naive_fold_left(buf, n, init, [](int a, int b) { return a + b; });
        require(actual, expected);
      },
      10000);

  sb::print("=== ALGO/ACCUMULATE+FOLD RIGOR SUITE PASSED ===");
  return 1;
}
