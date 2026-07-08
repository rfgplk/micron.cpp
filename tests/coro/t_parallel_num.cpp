
#include "../../src/math.hpp"
#include "../../src/parallel/algo.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace par = micron::parallel;

static int FAILS = 0;

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();
  const int N = 200000;
  int *a = new int[N];
  for ( int i = 0; i < N; ++i ) a[i] = (int)(((unsigned)i * 1103515245u + 12345u) & 0x3fffffu);

  sb::test_case("sum");
  {
    auto s = coro::sync_wait(par::sum(a, a + N));
    unsigned long long ser = 0;
    for ( int i = 0; i < N; ++i ) ser += (unsigned)a[i];
    sb::check((unsigned long long)s == ser);
  }
  sb::end_test_case();

  sb::test_case("mean");
  {
    double m = coro::sync_wait(par::mean(a, a + N));
    double ser = 0;
    for ( int i = 0; i < N; ++i ) ser += a[i];
    ser /= N;
    sb::check(micron::math::fabs(m - ser) < 1e-3);
  }
  sb::end_test_case();

  sb::test_case("max / min");
  {
    int mx = coro::sync_wait(par::max(a, a + N));
    int mn = coro::sync_wait(par::min(a, a + N));
    int smx = a[0], smn = a[0];
    for ( int i = 0; i < N; ++i ) {
      if ( a[i] > smx ) smx = a[i];
      if ( a[i] < smn ) smn = a[i];
    }
    sb::check(mx == smx);
    sb::check(mn == smn);
  }
  sb::end_test_case();

  sb::test_case("max_at / min_at leftmost");
  {
    int *mxp = coro::sync_wait(par::max_at(a, a + N));
    int *mnp = coro::sync_wait(par::min_at(a, a + N));
    int smx = a[0], smn = a[0];
    int smxi = 0, smni = 0;
    for ( int i = 0; i < N; ++i ) {
      if ( a[i] > smx ) {
        smx = a[i];
        smxi = i;
      }
      if ( a[i] < smn ) {
        smn = a[i];
        smni = i;
      }
    }
    sb::check(mxp - a == smxi);
    sb::check(mnp - a == smni);
  }
  sb::end_test_case();

  sb::test_case("inner_product");
  {
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) b[i] = i & 7;
    long long ip = coro::sync_wait(par::inner_product(a, a + N, b, 0LL));
    long long ser = 0;
    for ( int i = 0; i < N; ++i ) ser += (long long)a[i] * b[i];
    sb::check(ip == ser);
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("count");
  {
    int target = a[1234];
    usize c = coro::sync_wait(par::count(a, a + N, target));
    usize ser = 0;
    for ( int i = 0; i < N; ++i )
      if ( a[i] == target ) ++ser;
    sb::check(c == ser);
  }
  sb::end_test_case();

  sb::test_case("equal same / differ");
  {
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) b[i] = a[i];
    bool eq = coro::sync_wait(par::equal(a, a + N, b));
    sb::check(eq);
    b[N / 2] += 1;
    bool eq2 = coro::sync_wait(par::equal(a, a + N, b));
    sb::check(!eq2);
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("is_sorted ascending / broken tail");
  {
    bool s1 = coro::sync_wait(par::is_sorted(a, a + N));
    (void)s1;
    int *srt = new int[N];
    for ( int i = 0; i < N; ++i ) srt[i] = i;
    bool s2 = coro::sync_wait(par::is_sorted(srt, srt + N));
    sb::check(s2);
    srt[N - 1] = -1;
    bool s3 = coro::sync_wait(par::is_sorted(srt, srt + N));
    sb::check(!s3);
    delete[] srt;
  }
  sb::end_test_case();

  sb::test_case("contains present / absent");
  {
    bool c1 = coro::sync_wait(par::contains(a, a + N, a[9999]));
    sb::check(c1);
    bool c2 = coro::sync_wait(par::contains(a, a + N, -42));
    sb::check(!c2);
  }
  sb::end_test_case();

  delete[] a;
  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL NUMERIC TESTS PASSED ===");
  return 1;
}
