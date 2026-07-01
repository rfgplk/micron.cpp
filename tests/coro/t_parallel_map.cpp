// correctness check for parallel elementwise + memory maps (Task #2)
#include "../../src/parallel/algo.hpp"
#include "../snowball/snowball.hpp"
#include <cmath>      // std::sqrt / std::fabs oracle for par::sqrt (hosted test)

namespace coro = micron::coro;
namespace par = micron::parallel;

static int FAILS = 0;

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();
  const int N = 100000;

  // fill
  sb::test_case("fill");
  {
    int *a = new int[N];
    coro::sync_wait(par::fill(a, a + N, 7));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != 7 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // generate
  sb::test_case("generate");
  {
    int *a = new int[N];
    int seed = 0;
    (void)seed;
    coro::sync_wait(par::generate(a, a + N, []() { return 3; }));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != 3 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // reverse
  sb::test_case("reverse");
  {
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i;
    coro::sync_wait(par::reverse(a, a + N));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != N - 1 - i ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // reverse_copy
  sb::test_case("reverse_copy");
  {
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i;
    int *b = new int[N];
    coro::sync_wait(par::reverse_copy(a, a + N, b));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( b[i] != N - 1 - i ) ok = false;
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  // sqrt elementwise (oracle: std::sqrt from <cmath>)
  sb::test_case("sqrt elementwise vs std::sqrt");
  {
    double *a = new double[N];
    for ( int i = 0; i < N; ++i ) a[i] = (double)i;
    coro::sync_wait(par::sqrt(a, a + N));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( std::fabs(a[i] - std::sqrt((double)i)) > 1e-6 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // arith add (broadcast)
  sb::test_case("add broadcast");
  {
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i;
    coro::sync_wait(par::add(a, a + N, 100));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != i + 100 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // arith multiply (broadcast)
  sb::test_case("multiply broadcast");
  {
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i;
    coro::sync_wait(par::multiply(a, a + N, 2));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != i * 2 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // copy_n
  sb::test_case("copy_n");
  {
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i * 3;
    int *b = new int[N];
    coro::sync_wait(par::copy_n(a, b, (usize)N));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( b[i] != a[i] ) ok = false;
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  // zero_n (byte count, matching serial contract)
  sb::test_case("zero_n");
  {
    int *a = new int[N];
    for ( int i = 0; i < N; ++i ) a[i] = i + 1;
    coro::sync_wait(par::zero_n(a, (usize)N * sizeof(int)));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != 0 ) ok = false;
    sb::check(ok);
    delete[] a;
  }
  sb::end_test_case();

  // swap_n (byte count, matching serial contract)
  sb::test_case("swap_n");
  {
    int *a = new int[N];
    int *b = new int[N];
    for ( int i = 0; i < N; ++i ) {
      a[i] = i;
      b[i] = -i;
    }
    coro::sync_wait(par::swap_n(a, b, (usize)N * sizeof(int)));
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( a[i] != -i || b[i] != i ) ok = false;
    sb::check(ok);
    delete[] a;
    delete[] b;
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL MAP/ELEMENTWISE TESTS PASSED ===");
  return 1;
}
