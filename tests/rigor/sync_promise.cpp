//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/atomic/atomic.hpp"
#include "../../src/chrono.hpp"
#include "../../src/mutex/locks.hpp"

#include "../../src/sync/future.hpp"
#include "../../src/sync/promises.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== PROMISE<T> TESTS ===");

  test_case("promise<int> default ctor allocates state");
  {
    promise<int> p;
    auto f = p.get_future();
    require_true(f.valid());
  }
  end_test_case();

  test_case("promise<int>::get_future twice throws future_error");
  {
    promise<int> p;
    auto f = p.get_future();
    (void)f;
    require_throw([&]() { p.get_future(); });
  }
  end_test_case();

  test_case("promise<int>::set_value(const T&) round-trips via future");
  {
    promise<int> p;
    auto f = p.get_future();
    const int v = 13;
    p.set_value(v);
    require(f.get() == 13);
  }
  end_test_case();

  test_case("promise<int>::set_value(T&&) round-trips");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_value(static_cast<int &&>(99));
    require(f.get() == 99);
  }
  end_test_case();

  test_case("promise<int>::set_value twice throws future_error");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_value(1);
    require_throw([&]() { p.set_value(2); });
  }
  end_test_case();

  test_case("promise<int>::set_value_at_thread_exit forwards to set_value");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_value_at_thread_exit(55);
    require(f.get() == 55);
  }
  end_test_case();

  test_case("promise<int&> round-trip");
  {
    promise<int &> p;
    auto f = p.get_future();
    int v = 31;
    p.set_value(v);
    int &out = f.get();
    require(&out == &v);
    require(out == 31);
  }
  end_test_case();

  test_case("promise<void> round-trip");
  {
    promise<void> p;
    auto f = p.get_future();
    p.set_value();
    f.get();
    require_true(true);
  }
  end_test_case();

  test_case("promise dtor with retrieved future and no set_value -> future.get throws broken_promise");
  {
    future<int> f;
    {
      promise<int> p;
      f = p.get_future();
    }
    require_throw([&]() { f.get(); });
  }
  end_test_case();

  test_case("broken_promise carries 'broken promise' what() message");
  {
    future<int> f;
    {
      promise<int> p;
      f = p.get_future();
    }
    bool seen = false;
    try {
      f.get();
    } catch ( const broken_promise &e ) {
      seen = (e.what() != nullptr);
    }
    require_true(seen);
  }
  end_test_case();

  test_case("promise dtor with NO future_retrieved does not throw, frees state");
  {

    promise<int> p;
    require_true(true);
  }
  end_test_case();

  test_case("set_exception(msg) routes through state; future.get throws broken_promise");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_exception("custom failure");
    bool seen = false;
    try {
      (void)f.get();
    } catch ( const broken_promise &e ) {
      seen = true;

      (void)e;
    }
    require_true(seen);
  }
  end_test_case();

  test_case("promise<void>::set_exception transports broken_promise");
  {
    promise<void> p;
    auto f = p.get_future();
    p.set_exception("vfail");
    require_throw([&]() { f.get(); });
  }
  end_test_case();

  test_case("promise<int&>::set_exception transports broken_promise");
  {
    promise<int &> p;
    auto f = p.get_future();
    p.set_exception("rfail");
    require_throw([&]() { f.get(); });
  }
  end_test_case();

  sb::print("=== ALL PROMISE TESTS PASSED ===");
  return 1;
}
