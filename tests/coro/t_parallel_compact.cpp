
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

  sb::test_case("where evens");
  for ( int N : { 0, 1, 1024, 1025, 100000, 200003 } ) {
    int *in = new int[N ? N : 1];
    for ( int i = 0; i < N; ++i ) in[i] = i % 10;

    {
      int *out = new int[N ? N : 1];
      usize cnt = coro::sync_wait(par::where(in, in + N, out, [](int x) { return (x & 1) == 0; }));

      int *exp = new int[N ? N : 1];
      usize ec = 0;
      for ( int i = 0; i < N; ++i )
        if ( (in[i] & 1) == 0 ) exp[ec++] = in[i];
      bool ok = (cnt == ec);
      for ( usize i = 0; ok && i < ec; ++i )
        if ( out[i] != exp[i] ) ok = false;
      if ( !ok ) sb::print("where evens mismatch at N=", N);
      sb::check(ok);
      delete[] out;
      delete[] exp;
    }

    delete[] in;
  }
  sb::end_test_case();

  sb::test_case("unique runs");
  {
    const int N = 100000;
    int *in = new int[N];
    for ( int i = 0; i < N; ++i ) in[i] = i / 50;
    int *out = new int[N];
    usize cnt = coro::sync_wait(par::unique(in, in + N, out));
    int *exp = new int[N];
    usize ec = 0;
    for ( int i = 0; i < N; ++i )
      if ( i == 0 || in[i] != in[i - 1] ) exp[ec++] = in[i];
    bool ok = (cnt == ec);
    for ( usize i = 0; ok && i < ec; ++i )
      if ( out[i] != exp[i] ) ok = false;
    sb::check(ok);
    delete[] in;
    delete[] out;
    delete[] exp;
  }
  sb::end_test_case();

  sb::test_case("unique all-distinct");
  {
    const int N = 50000;
    int *in = new int[N];
    for ( int i = 0; i < N; ++i ) in[i] = i;
    int *out = new int[N];
    usize cnt = coro::sync_wait(par::unique(in, in + N, out));
    bool ok = (cnt == (usize)N);
    for ( int i = 0; ok && i < N; ++i )
      if ( out[i] != i ) ok = false;
    sb::check(ok);
    delete[] in;
    delete[] out;
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL COMPACTION TESTS PASSED ===");
  return 1;
}
