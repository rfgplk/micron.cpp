
#include "../../src/parallel/algo.hpp"
#include "../../src/vector.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace par = micron::parallel;
static int FAILS = 0;

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();
  micron::vector<int> v(20000);
  for ( int i = 0; i < 20000; ++i ) v[i] = i;

  sb::test_case("sum_range");
  sb::check(coro::sync_wait(par::sum_range(v)) == (long long)19999 * 20000 / 2);
  sb::end_test_case();

  sb::test_case("max_range");
  sb::check(coro::sync_wait(par::max_range(v)) == 19999);
  sb::end_test_case();

  sb::test_case("count_if_range");
  sb::check(coro::sync_wait(par::count_if_range(v, [](int x) { return (x & 1) == 0; })) == 10000);
  sb::end_test_case();

  sb::test_case("is_sorted_range");
  sb::check(coro::sync_wait(par::is_sorted_range(v)));
  sb::end_test_case();

  sb::test_case("reverse_range");
  coro::sync_wait(par::reverse_range(v));
  sb::check(v[0] == 19999 && v[19999] == 0);
  sb::end_test_case();

  sb::test_case("quick_range then is_sorted");
  coro::sync_wait(par::sort::quick_range(v));
  sb::check(coro::sync_wait(par::is_sorted_range(v)));
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO RANGES TESTS PASSED ===");
  return 1;
}
