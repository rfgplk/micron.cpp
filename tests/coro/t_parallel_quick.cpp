
#include "../../src/parallel/algo.hpp"
#include "../snowball/snowball.hpp"
#include <algorithm>

namespace coro = micron::coro;
namespace par = micron::parallel;
static int FAILS = 0;

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("quick sort matches std::sort across sizes");
  for ( int N : { 0, 1, 2, 1024, 1025, 4096, 100000, 250003 } ) {
    int *a = new int[N ? N : 1];
    int *b = new int[N ? N : 1];
    for ( int i = 0; i < N; ++i ) {
      unsigned v = (unsigned)i * 0x9E3779B1u + 1234567u;
      a[i] = (int)(v % 200000u);
      b[i] = a[i];
    }

    coro::sync_wait(par::sort::quick(a, a + N));
    std::sort(b, b + N);
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != b[i] ) ok = false;
    if ( !ok ) sb::print("quick mismatch at N=", N);
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("sort descending (=quick) vs std::sort");
  {
    const int N = 80000;
    int *a = new int[N];
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) {
      a[i] = (i * 48271) % 99991;
      b[i] = a[i];
    }
    coro::sync_wait(par::sort::sort(a, a + N, [](int x, int y) { return x > y; }));
    std::sort(b, b + N, [](int x, int y) { return x > y; });
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != b[i] ) ok = false;
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("quick already-sorted + reverse-sorted");
  {
    const int N = 60000;
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i;
    coro::sync_wait(par::sort::quick(a, a + N));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != i ) ok = false;
    sb::check(ok);
    for ( int i = 0; i < N; ++i ) a[i] = N - i;
    coro::sync_wait(par::sort::quick(a, a + N));
    ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != i + 1 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL QUICKSORT TESTS PASSED ===");
  return 1;
}
