// bloom_filter_exhaustive.cpp

#include "../../src/heap/bloom.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{
struct pod {
  int a;
  int b;

  bool
  operator==(const pod &o) const
  {
    return a == o.a && b == o.b;
  }
};

static_assert(micron::__bloom_optimal_k(65536, 8192) >= 1, "optimal k must be >= 1");
static_assert(micron::__bloom_optimal_k(8, 8) >= 1, "optimal k must be >= 1 even for tiny m/n");
}      // namespace

int
main()
{
  sb::print("=== BLOOM_FILTER TESTS ===");

  test_case("no false negatives: every inserted key is reported present");
  {
    micron::bloom_filter<int, (1u << 16)> bf;
    for ( int i = 0; i < 1000; i++ ) bf.insert(i);
    for ( int i = 0; i < 1000; i++ ) require_true(bf.contains(i));
  }
  end_test_case();

  test_case("not vacuously true: absent keys are mostly reported absent");
  {
    micron::bloom_filter<int, (1u << 16)> bf;
    for ( int i = 0; i < 1000; i++ ) bf.insert(i);

    usize fp = 0;
    for ( int i = 100000; i < 110000; i++ )
      if ( bf.contains(i) ) ++fp;
    require_true(fp < 500);
  }
  end_test_case();

  test_case("emplace behaves like insert (no false negatives)");
  {
    micron::bloom_filter<int, 4096> bf;
    for ( int i = 0; i < 50; i++ ) {
      int k = i;
      bf.emplace(micron::move(k));
    }
    for ( int i = 0; i < 50; i++ ) require_true(bf.contains(i));
  }
  end_test_case();

  test_case("smallest valid N (64 bits) still has no false negatives");
  {
    micron::bloom_filter<int, 64> bf;
    bf.insert(1);
    bf.insert(2);
    bf.insert(3);
    require_true(bf.contains(1));
    require_true(bf.contains(2));
    require_true(bf.contains(3));
  }
  end_test_case();

  test_case("trivially-copyable struct keys, explicit ExpectedN");
  {
    micron::bloom_filter<pod, 4096, 256> bf;
    pod keys[] = { { 1, 2 }, { 3, 4 }, { -7, 9 }, { 0, 0 }, { 100, 200 } };
    for ( auto &k : keys ) bf.insert(k);
    for ( auto &k : keys ) require_true(bf.contains(k));

    require_false(bf.contains(pod{ 999999, 888888 }));
  }
  end_test_case();

  test_case("explicit L override compiles and gives no false negatives");
  {
    micron::bloom_filter<int, 1024, 128, 3> bf;
    for ( int i = 0; i < 20; i++ ) bf.insert(i * 5);
    for ( int i = 0; i < 20; i++ ) require_true(bf.contains(i * 5));
  }
  end_test_case();

  test_case("default L is well-sized (load-factor driven), not collapsed to ~2");
  {

    constexpr usize k = micron::__bloom_optimal_k((1u << 16), (1u << 16) / 8);
    require_true(k >= 5);
    require_true(k <= 8);
  }
  end_test_case();

  sb::print("=== ALL BLOOM_FILTER TESTS PASSED ===");
  return 1;
}
