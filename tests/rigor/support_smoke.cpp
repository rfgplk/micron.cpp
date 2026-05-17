// support_smoke.cpp
// Smoke test for tests/support/* infrastructure. Just exercises the public
// surface to confirm headers compile and the basic behaviors hold.

#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
#include "../support/concept_harness.hpp"
#include "../support/mock_allocators.hpp"
#include "../support/oracles.hpp"
#include "../support/tracked_types.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

int
main()
{
  print("=== SUPPORT INFRA SMOKE ===");

  test_case("Tracked counters work");
  {
    mtest::Tracked<0>::reset();
    {
      mtest::Tracked<0> a;
      mtest::Tracked<0> b(a);                    // copy_ctor
      mtest::Tracked<0> c(micron::move(b));      // move_ctor
      (void)c;
    }
    require(mtest::Tracked<0>::ctor, usize(1));      // default
    require(mtest::Tracked<0>::copy_ctor, usize(1));
    require(mtest::Tracked<0>::move_ctor, usize(1));
    require(mtest::Tracked<0>::dtor, usize(3));
    require_true(mtest::Tracked<0>::live() == 0);
  }
  end_test_case();

  test_case("Throwing default_ctor throws on trip");
  {
    using TH = mtest::Throwing<mtest::throw_on::default_ctor, 1>;
    TH::reset();
    TH::arm(0);      // throw on first call
    sb::expect_throw([]() { TH a; });
    TH::disarm();
  }
  end_test_case();

  test_case("tracking_allocator counts allocations");
  {
    using A = mtest::tracking_allocator<0>;
    A::reset();
    {
      micron::vector<int, A> v;
      v.push_back(1);
      v.push_back(2);
      v.push_back(3);
      require(v.size(), usize(3));
    }
    require(A::outstanding(), i64(0));
  }
  end_test_case();

  test_case("throwing_allocator throws after trip");
  {
    using A = mtest::throwing_allocator<0>;
    A::reset();
    A::arm(0);      // first allocation throws
    bool caught = false;
    try {
      micron::vector<int, A> v;
      v.push_back(99);      // should trigger an allocation -> throw
    } catch ( ... ) {
      caught = true;
    }
    require(caught, true);
    A::disarm();
  }
  end_test_case();

  test_case("reference_vector matches micron::vector");
  {
    mtest::reference_vector<int, 32> ref;
    micron::vector<int> sut;
    for ( int i = 0; i < 10; ++i ) {
      ref.push_back(i);
      sut.push_back(i);
    }
    require_true(ref.matches(sut));
    ref.erase(3);
    sut.erase(usize(3));
    require_true(ref.matches(sut));
  }
  end_test_case();

  test_case("reference_map basic ops");
  {
    mtest::reference_map<int, int, 16> ref;
    require_true(ref.insert(1, 100));      // new
    require_true(ref.insert(2, 200));
    require_true(!ref.insert(1, 101));      // update
    require(ref.size(), usize(2));
    auto *v = ref.find(1);
    require_true(v != nullptr);
    require(*v, 101);
    require_true(ref.erase(2));
    require(ref.size(), usize(1));
  }
  end_test_case();

  test_case("prng is reproducible from seed");
  {
    mtest::prng a(42);
    mtest::prng b(42);
    for ( int i = 0; i < 100; ++i ) {
      require(a.next(), b.next());
    }
  }
  end_test_case();

  test_case("expect_no_throw / expect_throw");
  {
    sb::expect_no_throw([]() {
      int x = 1;
      (void)x;
    });
    sb::expect_throw([]() { micron::exc<micron::except::runtime_error>("test"); });
  }
  end_test_case();

  test_case("expect_leak_free with tracking allocator");
  {
    using A = mtest::tracking_allocator<1>;
    sb::expect_leak_free<A>([]() {
      micron::vector<int, A> v;
      for ( int i = 0; i < 50; ++i ) v.push_back(i);
    });
  }
  end_test_case();

  print("=== ALL SUPPORT SMOKE TESTS PASSED ===");
  return 1;
}
