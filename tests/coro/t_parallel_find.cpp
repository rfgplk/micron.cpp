
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
  for ( int i = 0; i < N; ++i ) a[i] = i % 1000;

  sb::test_case("find / find_if leftmost + absent");
  {
    int *r = coro::sync_wait(par::find(a, a + N, 777));
    int exp = -1;
    for ( int i = 0; i < N; ++i )
      if ( a[i] == 777 ) {
        exp = i;
        break;
      }
    sb::check(r - a == exp);
    int *r2 = coro::sync_wait(par::find(a, a + N, 99999));
    sb::check(r2 == a + N);
    int *r3 = coro::sync_wait(par::find_if(a, a + N, [](int x) { return x > 990; }));
    int e3 = -1;
    for ( int i = 0; i < N; ++i )
      if ( a[i] > 990 ) {
        e3 = i;
        break;
      }
    sb::check(r3 - a == e3);
  }
  sb::end_test_case();

  sb::test_case("find_last rightmost");
  {
    int *r = coro::sync_wait(par::find_last(a, a + N, 777));
    int exp = -1;
    for ( int i = 0; i < N; ++i )
      if ( a[i] == 777 ) exp = i;
    sb::check(r - a == exp);
  }
  sb::end_test_case();

  sb::test_case("mismatch");
  {
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) b[i] = a[i];
    b[12345] = -1;
    int *r = coro::sync_wait(par::mismatch(a, a + N, b));
    sb::check(r - a == 12345);
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("adjacent_find + none");
  {
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) b[i] = i;
    b[5000] = b[5001] = 42;
    int *r = coro::sync_wait(par::adjacent_find(b, b + N));
    sb::check(r - b == 5000);
    int *c = new int[N];
    for ( int i = 0; i < N; ++i ) c[i] = i;
    int *r2 = coro::sync_wait(par::adjacent_find(c, c + N));
    sb::check(r2 == c + N);
    delete[] b;
    delete[] c;
  }
  sb::end_test_case();

  sb::test_case("search + absent");
  {
    int pat[4] = { 100, 101, 102, 103 };
    int *r = coro::sync_wait(par::search(a, a + N, pat, pat + 4));
    int exp = -1;
    for ( int i = 0; i + 4 <= N; ++i )
      if ( a[i] == 100 && a[i + 1] == 101 && a[i + 2] == 102 && a[i + 3] == 103 ) {
        exp = i;
        break;
      }
    sb::check(r - a == exp);
    int npat[3] = { 5, 5, 9999 };
    int *r2 = coro::sync_wait(par::search(a, a + N, npat, npat + 3));
    sb::check(r2 == a + N);
  }
  sb::end_test_case();

  sb::test_case("search_n");
  {
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) b[i] = i;
    b[8000] = b[8001] = b[8002] = 7;
    int *r = coro::sync_wait(par::search_n(b, b + N, (usize)3, 7));
    sb::check(r - b == 8000);
    delete[] b;
  }
  sb::end_test_case();

  delete[] a;
  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL FIND TESTS PASSED ===");
  return 1;
}
