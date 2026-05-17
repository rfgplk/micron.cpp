// vector_try_reserve.cpp
// Regression test for B3: try_reserve no longer throws when the requested
// capacity is already satisfied. Previously the check was inverted
// (`if (n < capacity) throw`), making try_reserve unusable when a vector
// happened to be large enough already.

#include "../../src/std.hpp"

#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_nothrow;
using sb::require_true;
using sb::test_case;

int
main()
{
  print("=== VECTOR TRY_RESERVE TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("try_reserve no-ops when n <= capacity");
  {
    micron::vector<int> v;
    v.reserve(64);
    const usize cap = v.max_size();
    // request a smaller capacity than currently allocated; must not throw
    bool threw = false;
    try {
      v.try_reserve(32);
    } catch ( ... ) {
      threw = true;
    }
    require(threw, false);
    require(v.max_size(), cap);
    // request the exact capacity; must not throw
    threw = false;
    try {
      v.try_reserve(cap);
    } catch ( ... ) {
      threw = true;
    }
    require(threw, false);
    require(v.max_size(), cap);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("try_reserve expands when n > capacity");
  {
    micron::vector<int> v;
    v.reserve(8);
    const usize old_cap = v.max_size();
    v.try_reserve(old_cap + 100);
    require_true(v.max_size() >= old_cap + 100);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("try_reserve on empty vector allocates fresh");
  {
    micron::vector<int> v;
    v.try_reserve(128);
    require_true(v.max_size() >= usize(128));
  }
  end_test_case();

  print("=== ALL VECTOR TRY_RESERVE TESTS PASSED ===");
  return 1;
}
