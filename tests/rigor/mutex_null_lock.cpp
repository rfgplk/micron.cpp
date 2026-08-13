//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/mutex/locks.hpp"

#include "../../src/concepts.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

template<typename M>
concept has_reset_pmf = requires(M m) {
  { m.retrieve() };
  { m() };
};

}      // namespace

static_assert(micron::is_mutex<micron::null_lock>, "null_lock must satisfy is_mutex");
static_assert(has_reset_pmf<micron::null_lock>, "null_lock must work with the reset-PMF guards");
static_assert(has_reset_pmf<micron::mutex>, "mutex does, which is what lock_guard dispatches through");

int
main(void)
{
  using namespace micron;
  sb::print("=== NULL_LOCK TESTS ===");

  test_case("every operation is a no-op and it never reports itself held");
  {
    null_lock m;
    require_false(m.is_locked());
    m.lock();
    require_false(m.is_locked());
    require_true(m.try_lock());
    require_true(m.try_lock());
    m.unlock();
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("it is genuinely free: an empty class, no state");
  {
    static_assert(is_empty_v<null_lock>, "null_lock must add nothing to a container that holds one");
    static_assert(sizeof(null_lock) == 1, "an empty class is still one byte standalone");
    require_true(true);
  }
  end_test_case();

  test_case("variadic lock / try_lock / unlock accept it");
  {
    null_lock a, b, c;
    micron::lock(a, b, c);
    require_true(micron::try_lock(a, b, c));
    micron::unlock(a, b, c);
    require(try_lock_all(a, b, c) == -1);
    micron::unlock(a, b, c);
    lock_all(a, b, c);
    micron::unlock(a, b, c);
    require_false(a.is_locked());
  }
  end_test_case();

  test_case("lock_set over null_locks compiles and is inert");
  {
    null_lock a, b;
    {
      lock_set<null_lock, null_lock> g(a, b);
      require_true(g.owns_lock());
      require_false(a.is_locked());
    }
    require_false(a.is_locked());
  }
  end_test_case();

  test_case("as a single-threaded policy parameter it costs nothing");
  {

    struct with_null {
      [[no_unique_address]] null_lock lk;
      u64 v;
    };

    struct without {
      u64 v;
    };

    static_assert(sizeof(with_null) == sizeof(without), "null_lock must vanish under [[no_unique_address]]");
    require_true(true);
  }
  end_test_case();

  test_case("the reset-PMF guards accept it now, and are still inert");
  {
    null_lock m;
    {
      lock_guard<null_lock> g(m);
      require_false(m.is_locked());
    }
    {
      auto_guard<null_lock> g(m);
      require_false(m.is_locked());
    }
    {
      unique_lock<lock_starts::locked, null_lock> u(m);
      require_true(u.owns_lock());
      u.unlock();
      require_false(u.owns_lock());
    }
    {
      unique_lock<lock_starts::attempt, null_lock> u(m);
      require_true(u.owns_lock());
    }
    {
      unique_lock<lock_starts::defer, null_lock> u(m);
      require_false(u.owns_lock());
      u.lock();
      require_true(u.owns_lock());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("non-copyable / non-movable, like every other lock");
  {
    static_assert(!is_copy_constructible_v<null_lock>);
    static_assert(!is_move_constructible_v<null_lock>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL NULL_LOCK TESTS PASSED ===");
  return 1;
}
