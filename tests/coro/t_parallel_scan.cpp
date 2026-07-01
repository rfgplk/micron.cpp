
#include "../../src/parallel/algo.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace par = micron::parallel;

static int FAILS = 0;

static auto plus = [](long long a, long long b) { return a + b; };
static auto imax = [](int a, int b) { return a > b ? a : b; };

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("scan / scanl across sizes");
  for ( int N : { 0, 1, 2, 1023, 1024, 1025, 100000, 200003 } ) {
    long long *in = new long long[N ? N : 1];
    long long *out = new long long[(N ? N : 1) + 1];
    for ( int i = 0; i < N; ++i ) in[i] = (i % 97);

    coro::sync_wait(par::scan(in, in + N, out, plus));
    {
      bool ok = true;
      long long acc = 0;
      for ( int i = 0; i < N; ++i ) {
        acc += in[i];
        if ( out[i] != acc ) ok = false;
      }
      if ( !ok ) sb::print("scan inclusive mismatch at N=", N);
      sb::check(ok);
    }

    {
      long long *buf = new long long[N ? N : 1];
      for ( int i = 0; i < N; ++i ) buf[i] = in[i];
      coro::sync_wait(par::scan(buf, buf + N, buf, plus));
      bool ok = true;
      long long acc = 0;
      for ( int i = 0; i < N; ++i ) {
        acc += in[i];
        if ( buf[i] != acc ) ok = false;
      }
      if ( !ok ) sb::print("scan in-place mismatch at N=", N);
      sb::check(ok);
      delete[] buf;
    }

    {
      long long *o2 = new long long[N + 1];
      const long long init = 1000;
      coro::sync_wait(par::scanl(in, in + N, o2, init, plus));
      bool ok = (o2[0] == init);
      long long acc = init;
      for ( int i = 0; i < N; ++i ) {
        acc = acc + in[i];
        if ( o2[i + 1] != acc ) ok = false;
      }
      if ( !ok ) sb::print("scanl mismatch at N=", N);
      sb::check(ok);
      delete[] o2;
    }

    delete[] in;
    delete[] out;
  }
  sb::end_test_case();

  sb::test_case("scan prefix-max");
  {
    const int N = 50000;
    int *in = new int[N];
    for ( int i = 0; i < N; ++i ) in[i] = ((i * 31 + 7) % 1000) - 500;
    int *out = new int[N];
    coro::sync_wait(par::scan(in, in + N, out, imax));
    bool ok = true;
    int acc = in[0];
    for ( int i = 0; i < N; ++i ) {
      if ( in[i] > acc ) acc = in[i];
      if ( out[i] != acc ) ok = false;
    }
    sb::check(ok);
    delete[] in;
    delete[] out;
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL PARALLEL SCAN TESTS PASSED ===");
  return 1;
}
