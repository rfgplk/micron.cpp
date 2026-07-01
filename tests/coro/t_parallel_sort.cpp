
#include "../../src/parallel/algo.hpp"
#include "../snowball/snowball.hpp"
#include <algorithm>

namespace coro = micron::coro;
namespace par = micron::parallel;
static int FAILS = 0;

struct KV {
  int key;
  int idx;
};

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("merge sort matches std::sort across sizes");
  for ( int N : { 0, 1, 2, 1024, 1025, 4096, 100000, 250003 } ) {
    int *a = new int[N ? N : 1];
    int *b = new int[N ? N : 1];
    for ( int i = 0; i < N; ++i ) {
      unsigned v = (unsigned)i * 2654435761u;
      a[i] = (int)(v % 100000u);
      b[i] = a[i];
    }

    coro::sync_wait(par::sort::merge(a, a + N));
    std::sort(b, b + N);
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != b[i] ) ok = false;
    if ( !ok ) sb::print("merge sort mismatch at N=", N);
    sb::check(ok);

    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("merge sort descending");
  {
    const int N = 50000;
    int *a = new int[N];
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) {
      a[i] = (i * 31 + 7) % 9973;
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

  sb::test_case("merge sort stable (key+idx match std::stable_sort)");
  {
    const int N = 120000;
    KV *a = new KV[N];
    KV *b = new KV[N];
    for ( int i = 0; i < N; ++i ) {
      a[i] = KV{ (i * 7) % 137, i };
      b[i] = a[i];
    }
    auto bykey = [](const KV &x, const KV &y) { return x.key < y.key; };
    coro::sync_wait(par::sort::stable(a, a + N, bykey));
    std::stable_sort(b, b + N, bykey);
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i].key != b[i].key || a[i].idx != b[i].idx ) ok = false;
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL MERGE SORT TESTS PASSED ===");
  return 1;
}
