
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
  for ( int i = 0; i < N; ++i ) a[i] = i;

  sb::test_case("for_each increment");
  coro::sync_wait(par::for_each(a, a + N, [](int &x) { x += 1; }));
  bool ok = true;
  for ( int i = 0; i < N; ++i )
    if ( a[i] != i + 1 ) ok = false;
  sb::check(ok);
  sb::end_test_case();

  sb::test_case("reduce sum");
  long long sum_par = coro::sync_wait(par::reduce(a, a + N, 0LL, [](long long x, long long y) { return x + y; }));
  long long sum_ser = 0;
  for ( int i = 0; i < N; ++i ) sum_ser += a[i];
  sb::check(sum_par == sum_ser);
  sb::end_test_case();

  sb::test_case("transform x2");
  int *b = new int[N];
  coro::sync_wait(par::transform(a, a + N, b, [](int x) { return x * 2; }));
  ok = true;
  for ( int i = 0; i < N; ++i )
    if ( b[i] != a[i] * 2 ) ok = false;
  sb::check(ok);
  sb::end_test_case();

  sb::test_case("binary transform x+y");
  int *c = new int[N];
  coro::sync_wait(par::transform(a, a + N, b, c, [](int x, int y) { return x + y; }));
  ok = true;
  for ( int i = 0; i < N; ++i )
    if ( c[i] != a[i] + b[i] ) ok = false;
  sb::check(ok);
  sb::end_test_case();

  sb::test_case("all_of / any_of / none_of");
  {
    bool all_pos = coro::sync_wait(par::all_of(a, a + N, [](int x) { return x > 0; }));
    sb::check(all_pos);
    bool any_big = coro::sync_wait(par::any_of(a, a + N, [](int x) { return x == 150000; }));
    sb::check(any_big);
    bool none_neg = coro::sync_wait(par::none_of(a, a + N, [](int x) { return x < 0; }));
    sb::check(none_neg);
    bool any_huge = coro::sync_wait(par::any_of(a, a + N, [](int x) { return x > 10000000; }));
    sb::check(!any_huge);
  }
  sb::end_test_case();

  sb::test_case("count_if even");
  usize cnt_par = coro::sync_wait(par::count_if(a, a + N, [](int x) { return (x & 1) == 0; }));
  usize cnt_ser = 0;
  for ( int i = 0; i < N; ++i )
    if ( (a[i] & 1) == 0 ) ++cnt_ser;
  sb::check(cnt_par == cnt_ser);
  sb::end_test_case();

  sb::test_case("empty-range edge cases");
  {
    bool e_all = coro::sync_wait(par::all_of(a, a, [](int) { return false; }));
    sb::check(e_all);
    bool e_any = coro::sync_wait(par::any_of(a, a, [](int) { return true; }));
    sb::check(!e_any);
    long long e_red = coro::sync_wait(par::reduce(a, a, 42LL, [](long long x, long long y) { return x + y; }));
    sb::check(e_red == 42);
  }
  sb::end_test_case();

  delete[] a;
  delete[] b;
  delete[] c;
  coro::stop_coroutine_runtime();

  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL BASIC TESTS PASSED ===");
  return 1;
}
