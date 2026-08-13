//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// fuzz_algo_core — differential fuzzing of algorithm.hpp, accumulate.hpp, fold.hpp,
// filter.hpp and arith.hpp.
//
// Beyond diffing against naive oracles, this leans on metamorphic laws, which catch classes of
// bug an oracle diff can miss because they hold regardless of the input:
//
//     reverse . reverse           == identity
//     rotate_left k . rotate_right k == identity
//     rotate_left k                == rotate_left (k mod n)
//     reverse                      is a permutation (multiset-preserving)
//     count_if p + count_if !p     == n
//     filter p ++ filter !p        is a permutation of the input

#include "../snowball/snowball_fuzz.hpp"
#include "../support/algo_fuzz.hpp"

#include "../../src/algorithm/accumulate.hpp"
#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/arith.hpp"
#include "../../src/algorithm/filter.hpp"
#include "../../src/algorithm/fold.hpp"

namespace sb = snowball;
namespace sbf = snowball::fuzzing;

using mtest::prng;
using mtest::fuzz::sweep;
namespace ref = mtest::rigor::ref;

namespace
{

constexpr usize MAXN = 1024;

constexpr auto v_pos = []<typename U>(U x)
  requires(!micron::is_pointer_v<U>)
{ return x > U(0); };
constexpr auto p_pos = []<typename U>(const U *x) { return *x > U(0); };

template<typename T>
micron::vector<T>
as_vec(const T *p, usize n)
{
  micron::vector<T> v;
  v.resize(n);
  for ( usize i = 0; i < n; ++i ) v[i] = p[i];
  return v;
}

template<typename T>
void
rearrange_suite(const char *tag, u64 seed)
{
  static T work[MAXN];
  static T want[MAXN];
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &rng) {
        const usize k = n ? static_cast<usize>(rng.next() % (2 * n + 1)) : 0;

        for ( usize i = 0; i < n; ++i ) work[i] = buf[i];
        for ( usize i = 0; i < n; ++i ) want[i] = buf[i];
        ref::naive_reverse(want, n);
        if ( n ) micron::reverse(work, work + n - 1);
        FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !ref::naive_equal(work, want, n), "reverse disagrees with oracle");
        if ( n ) micron::reverse(work, work + n - 1);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(work, buf, n), "reverse is not its own inverse");

        {
          micron::vector<T> v = as_vec(buf, n);
          micron::reverse(v);
          FUZZ_FAIL_IF(v.size() != n, "reverse[container] changed the size");
          for ( usize i = 0; i < n; ++i )
            FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&v[i], &want[i], 1), "reverse[container] disagrees with oracle");
        }

        for ( usize i = 0; i < n; ++i ) work[i] = buf[i];
        for ( usize i = 0; i < n; ++i ) want[i] = buf[i];
        micron::rotate_left(work, work + n, k);
        ref::naive_rotate_left(want, n, n ? k % n : 0);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(work, want, n), "rotate_left disagrees with oracle");

        micron::rotate_right(work, work + n, k);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(work, buf, n), "rotate_right does not invert rotate_left");

        FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !mtest::fuzz::fref::naive_is_permutation(want, n, buf, n),
                     "rotate_left is not a permutation");

        for ( usize i = 0; i < n; ++i ) work[i] = buf[i];
        for ( usize i = 0; i < n; ++i ) want[i] = buf[i];
        micron::shift_left(work, work + n, k);
        ref::naive_shift_left(want, n, k);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(work, want, n), "shift_left disagrees with oracle");

        for ( usize i = 0; i < n; ++i ) work[i] = buf[i];
        for ( usize i = 0; i < n; ++i ) want[i] = buf[i];
        micron::shift_right(work, work + n, k);
        ref::naive_shift_right(want, n, k);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(work, want, n), "shift_right disagrees with oracle");

        for ( usize i = 0; i < n; ++i ) want[i] = buf[i];
        ref::naive_reverse(want, n);
        micron::reverse_copy(static_cast<const T *>(buf), buf + n, work);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(work, want, n), "reverse_copy disagrees with oracle");
      },
      seed);
}

template<typename T>
void
write_suite(const char *tag, u64 seed)
{
  static T work[MAXN];
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &rng) {
        const T wide = static_cast<T>(static_cast<i64>(rng.next() % 100000u) - 50000);

        for ( usize i = 0; i < n; ++i ) work[i] = buf[i];
        micron::fill(work, work + n, wide);
        for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(work[i] == wide), "fill did not write the exact value");

        micron::fill_n(work, n, wide);
        for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(work[i] == wide), "fill_n did not write the exact value");

        {
          micron::vector<T> v = as_vec(buf, n);
          micron::fill(v, wide);
          for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(v[i] == wide), "fill[container] did not write the exact value");
          micron::clear(v, wide);
          for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(v[i] == wide), "clear did not write the exact value");
          micron::clear(v);
          for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(v[i] == T{}), "clear did not zero");
        }

        {
          usize ctr = 0;
          micron::generate(work, work + n, [&ctr]() { return static_cast<T>(ctr++); });
          for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(work[i] == static_cast<T>(i)), "generate did not apply the functor in order");
        }

        {
          for ( usize i = 0; i < n; ++i ) work[i] = buf[i];
          micron::transform(work, work + n, [](T x) { return static_cast<T>(x + T(1)); });
          for ( usize i = 0; i < n; ++i )
            FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !(work[i] == static_cast<T>(buf[i] + T(1))),
                         "transform[ptr] disagrees with the elementwise result");

          micron::vector<T> v = as_vec(buf, n);
          micron::transform(v, [](T x) { return static_cast<T>(x + T(1)); });
          for ( usize i = 0; i < n; ++i )
            FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&v[i], &work[i], 1), "transform[container] disagrees with transform[ptr]");
        }
      },
      seed);
}

template<typename T>
void
reduce_suite(const char *tag, u64 seed)
{
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &) {
        if ( n == 0 ) return;
        const bool chk = mtest::fuzz::cmp_diff_ok(buf, n);
        const T *b = buf;

        if ( chk ) {
          const T gmx = micron::max(b, b + n), wmx = ref::naive_max(b, n);
          const T gmn = micron::min(b, b + n), wmn = ref::naive_min(b, n);
          FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&gmx, &wmx, 1), "max disagrees with oracle");
          FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&gmn, &wmn, 1), "min disagrees with oracle");
        }

        FUZZ_FAIL_IF(chk && micron::max_at(b, b + n) != b + ref::naive_max_idx(b, n), "max_at disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::min_at(b, b + n) != b + ref::naive_min_idx(b, n), "min_at disagrees with oracle");

        if ( chk ) {
          const T mx = micron::max(b, b + n);
          const T mn = micron::min(b, b + n);
          for ( usize i = 0; i < n; ++i ) {
            FUZZ_FAIL_IF(b[i] > mx, "max is not an upper bound");
            FUZZ_FAIL_IF(b[i] < mn, "min is not a lower bound");
          }
        }

        if ( chk ) {
          const T lo = ref::naive_min(b, n);
          const T hi = ref::naive_max(b, n);
          for ( usize i = 0; i < n; ++i ) {

            const T c = micron::clamp(b[i], lo, hi);
            FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&c, &b[i], 1), "clamp altered a value already inside the interval");
          }
        }

        micron::vector<T> v = as_vec(b, n);
        if constexpr ( micron::is_integral_v<T> ) {

          umax_t want = 0;
          for ( usize i = 0; i < n; ++i ) want += static_cast<umax_t>(b[i]);
          FUZZ_FAIL_IF(micron::sum(v) != want, "sum[integral] disagrees with oracle");
        } else {

          bool finite = true;
          for ( usize i = 0; i < n; ++i ) {
            const u64 bits = __builtin_bit_cast(u64, static_cast<f64>(b[i]));
            if ( (bits & 0x7ff0000000000000ull) == 0x7ff0000000000000ull ) finite = false;
          }
          if ( finite ) {
            f128 exact = 0;
            f128 mag = 0;
            for ( usize i = 0; i < n; ++i ) {
              exact += static_cast<f128>(b[i]);
              mag += static_cast<f128>(b[i] < T(0) ? -b[i] : b[i]);
            }
            const f128 got = micron::sum(v);
            const f128 err = (got > exact) ? (got - exact) : (exact - got);
            FUZZ_FAIL_IF(err > mag * static_cast<f128>(1e-15) + static_cast<f128>(1e-300),
                         "sum[float] is outside the error bound for a valid summation");
          }
        }

        if constexpr ( micron::is_integral_v<T> ) {
          const umax_t init = 7;
          umax_t want = init;
          for ( usize i = 0; i < n; ++i ) want += static_cast<umax_t>(b[i]);
          FUZZ_FAIL_IF(micron::accumulate(b, b + n, init) != want, "accumulate disagrees with oracle");

          FUZZ_FAIL_IF(micron::accumulate(b, b + n, init, [](umax_t a, const T &x) { return a + static_cast<umax_t>(x); }) != want,
                       "accumulate[Fn] disagrees with the operator+ form");

          const usize lim = n / 2;
          umax_t wlim = init;
          for ( usize i = 0; i < lim; ++i ) wlim += static_cast<umax_t>(b[i]);
          FUZZ_FAIL_IF(micron::accumulate(b, b + n, init, lim) != wlim, "accumulate[limit] disagrees with oracle");
        }

        if constexpr ( micron::is_integral_v<T> ) {
          auto plus = [](umax_t a, const T *x) { return a + static_cast<umax_t>(*x); };
          umax_t want = 0;
          for ( usize i = 0; i < n; ++i ) want += static_cast<umax_t>(b[i]);
          FUZZ_FAIL_IF(micron::fold_left(b, b + n, umax_t(0), plus) != want, "fold_left disagrees with oracle");
          FUZZ_FAIL_IF(micron::fold(b, b + n, umax_t(0), plus, n) != want, "fold disagrees with fold_left");

          auto rplus = [](const T *x, umax_t a) { return a + static_cast<umax_t>(*x); };
          FUZZ_FAIL_IF(micron::fold_right(b, b + n, rplus, umax_t(0)) != want, "fold_right disagrees with fold_left over a commutative op");
          auto counted = micron::fold_left_counted(b, b + n, umax_t(0), plus);
          FUZZ_FAIL_IF(counted.a != want || counted.b != n, "fold_left_counted disagrees with fold_left");

          FUZZ_FAIL_IF(micron::fold_left_while(b, b + n, umax_t(0), plus, [](umax_t, const T *) { return true; }) != want,
                       "fold_left_while with a true predicate is not fold_left");

          FUZZ_FAIL_IF(micron::fold_left_while(b, b + n, umax_t(0), plus, [](umax_t, const T *) { return false; }) != umax_t(0),
                       "fold_left_while with a false predicate consumed elements");
        }
      },
      seed);
}

template<typename T>
void
select_suite(const char *tag, u64 seed)
{
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &) {
        const bool chk = mtest::fuzz::cmp_diff_ok(buf, n);
        micron::vector<T> v = as_vec(buf, n);

        micron::vector<T> want;
        for ( usize i = 0; i < n; ++i )
          if ( buf[i] > T(0) ) want.push_back(buf[i]);

        auto got = micron::filter(v, p_pos);
        FUZZ_FAIL_IF(chk && got.size() != want.size(), "filter kept the wrong number of elements");
        if ( chk )
          for ( usize i = 0; i < got.size(); ++i ) FUZZ_FAIL_IF(!(got[i] == want[i]), "filter kept the wrong elements or order");

        micron::vector<T> v2 = as_vec(buf, n);
        auto rest = micron::filter(v2, [](const T *x) { return !(*x > T(0)); });
        FUZZ_FAIL_IF(chk && got.size() + rest.size() != n, "filter over a predicate and its negation does not partition");

        micron::vector<T> v3 = as_vec(buf, n);
        micron::filter_inplace(v3, p_pos);
        FUZZ_FAIL_IF(chk && v3.size() != want.size(), "filter_inplace disagrees with filter on size");
        if ( chk )
          for ( usize i = 0; i < v3.size(); ++i ) FUZZ_FAIL_IF(!(v3[i] == want[i]), "filter_inplace disagrees with filter on contents");

        micron::vector<T> v4 = as_vec(buf, n);
        auto w = micron::where(v4, p_pos);
        FUZZ_FAIL_IF(chk && w.size() != want.size(), "where disagrees with filter on size");

        const usize lim = n / 3;
        micron::vector<T> v5 = as_vec(buf, n);
        auto lf = micron::filter(v5, p_pos, lim);
        FUZZ_FAIL_IF(lf.size() > lim, "filter[limit] exceeded the limit");
        FUZZ_FAIL_IF(chk && lf.size() != (want.size() < lim ? want.size() : lim), "filter[limit] kept the wrong number");
      },
      seed);
}

template<typename T>
void
arith_suite(const char *tag, u64 seed)
{
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &rng) {
        const T y = static_cast<T>(1 + (rng.next() % 7u));
        micron::vector<T> v = as_vec(buf, n);

        micron::add(v, y);
        for ( usize i = 0; i < n; ++i )
          FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !(v[i] == static_cast<T>(buf[i] + y)), "add disagrees elementwise");

        if constexpr ( micron::is_integral_v<T> ) {
          micron::subtract(v, y);
          for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(v[i] == buf[i]), "subtract does not invert add");
        }

        micron::vector<T> m = as_vec(buf, n);
        micron::multiply(m, y);
        for ( usize i = 0; i < n; ++i )
          FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !(m[i] == static_cast<T>(buf[i] * y)), "multiply disagrees elementwise");

        micron::vector<T> d = as_vec(buf, n);
        micron::divide(d, y);
        for ( usize i = 0; i < n; ++i )
          FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !(d[i] == static_cast<T>(buf[i] / y)), "divide disagrees elementwise");
      },
      seed);
}

}      // namespace

int
main(void)
{
  sb::print("=== ALGO/CORE FUZZ SUITE ===");

  rearrange_suite<u8>("rearrange u8", 0xC0BE1001ULL);
  rearrange_suite<i32>("rearrange i32", 0xC0BE1002ULL);
  rearrange_suite<u64>("rearrange u64", 0xC0BE1003ULL);
  rearrange_suite<f64>("rearrange f64", 0xC0BE1004ULL);

  write_suite<u8>("write u8", 0xD00D2001ULL);
  write_suite<i16>("write i16", 0xD00D2002ULL);
  write_suite<i32>("write i32", 0xD00D2003ULL);
  write_suite<u64>("write u64", 0xD00D2004ULL);
  write_suite<f64>("write f64", 0xD00D2005ULL);

  reduce_suite<u8>("reduce u8", 0xFEED3001ULL);
  reduce_suite<i32>("reduce i32", 0xFEED3002ULL);
  reduce_suite<u64>("reduce u64", 0xFEED3003ULL);
  reduce_suite<f64>("reduce f64", 0xFEED3004ULL);

  select_suite<i32>("select i32", 0x5E1EC7001ULL);
  select_suite<i64>("select i64", 0x5E1EC7002ULL);
  select_suite<f64>("select f64", 0x5E1EC7003ULL);

  arith_suite<i32>("arith i32", 0xA217A001ULL);
  arith_suite<f64>("arith f64", 0xA217A002ULL);

  sbf::check_property(
      "reverse is an involution and a permutation",
      [](micron::vector<i32> v) {
        micron::vector<i32> orig = v;
        micron::reverse(v);
        if ( v.size() != orig.size() ) FUZZ_FAIL("reverse changed the size");
        if ( !mtest::fuzz::fref::naive_is_permutation(v.begin(), v.size(), orig.begin(), orig.size()) )
          FUZZ_FAIL("reverse is not a permutation");
        micron::reverse(v);
        for ( usize i = 0; i < v.size(); ++i )
          if ( v[i] != orig[i] ) FUZZ_FAIL("reverse is not its own inverse");
      },
      { .seed = 0x2E000001, .count = 20000 }, sbf::vector_of(sbf::spec<i32>{}).len(0, 64));

  sbf::check_property(
      "sum over a concatenation is the sum of the parts",
      [](micron::vector<u32> a, micron::vector<u32> b) {
        micron::vector<u32> c = a;
        for ( usize i = 0; i < b.size(); ++i ) c.push_back(b[i]);
        if ( micron::sum(c) != micron::sum(a) + micron::sum(b) ) FUZZ_FAIL("sum is not additive over concatenation");
      },
      { .seed = 0x50ADD001, .count = 20000 }, sbf::vector_of(sbf::spec<u32>{}).len(0, 48), sbf::vector_of(sbf::spec<u32>{}).len(0, 48));

  sbf::check_property(
      "fill writes the exact value for any width",
      [](micron::vector<i64> v, i64 val) {
        micron::fill(v, val);
        for ( usize i = 0; i < v.size(); ++i )
          if ( v[i] != val ) FUZZ_FAIL("fill did not write the exact value");
      },
      { .seed = 0xF111ADD1, .count = 20000 }, sbf::vector_of(sbf::spec<i64>{}).len(0, 64), sbf::special_ints<i64>());

  sb::print("=== ALGO/CORE FUZZ SUITE PASSED ===");
  return 1;
}
