
#include "../../src/parallel/algo.hpp"
#include "../snowball/snowball.hpp"
#include <algorithm>

namespace coro = micron::coro;
namespace par = micron::parallel;
static int FAILS = 0;

struct KV {
  int key;
  int payload;
};

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("radix int matches std::sort across sizes");
  for ( int N : { 0, 1, 2, 1024, 1025, 100000, 250003 } ) {
    int *a = new int[N ? N : 1];
    int *b = new int[N ? N : 1];
    for ( int i = 0; i < N; ++i ) {
      unsigned v = (unsigned)i * 2654435761u + 7u;
      a[i] = (int)(v % 400001u) - 200000;
      b[i] = a[i];
    }
    coro::sync_wait(par::sort::radix(a, a + N));
    std::sort(b, b + N);
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != b[i] ) ok = false;
    if ( !ok ) sb::print("radix mismatch at N=", N);
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  sb::test_case("radix stable by key (matches std::stable_sort)");
  {
    const int N = 120000;
    KV *a = new KV[N];
    KV *b = new KV[N];
    for ( int i = 0; i < N; ++i ) {
      a[i] = KV{ (i * 7) % 137, i };
      b[i] = a[i];
    }
    coro::sync_wait(par::sort::radix(a, a + N, [](const KV &e) { return e.key; }));
    std::stable_sort(b, b + N, [](const KV &x, const KV &y) { return x.key < y.key; });
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i].key != b[i].key || a[i].payload != b[i].payload ) ok = false;
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL RADIX SORT TESTS PASSED ===");
  return 1;
}
