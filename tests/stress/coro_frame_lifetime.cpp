//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"
#include "../support/mt.hpp"

using namespace snowball;
namespace coro = micron::coro;

namespace
{

using val = ltest::tracked<0>;
using gen_v = ltest::tracked<1>;
using cap = ltest::tracked<2>;

ltest::live_registry g_reg;

constexpr u64 ROUNDS = 400;
constexpr u64 FANOUT = 64;
constexpr u64 THREADS = 6;

micron::task<val>
make_val(i64 v)
{
  co_return val{ v };
}

micron::task<val>
via_eventual(i64 v)
{
  coro::eventual<val> e;
  co_await coro::call(&e, make_val)(v);
  co_await coro::join;
  co_return micron::move(e).operator*();
}

micron::task<i64>
via_lvalue_slots(i64 v)
{
  val a{}, b{};
  co_await coro::fork[&a, make_val](v);
  co_await coro::call[&b, make_val](v + 1);
  co_await coro::join;
  co_return a.v + b.v;
}

micron::atomic_token<i64> g_discard_seen{ 0 };

micron::task<val>
discarded_child(i64 v)
{
  g_discard_seen.fetch_add(1, micron::memory_order_relaxed);
  co_return val{ v };
}

micron::task<void>
drop_child_result(i64 v)
{
  co_await coro::call(coro::discard, discarded_child)(v);
  co_return;
}

micron::generator<gen_v>
yield_n(i64 n)
{
  for ( i64 i = 0; i < n; ++i ) co_yield gen_v{ i };
}

micron::task<i64>
never_awaited(cap c)
{
  co_return c.v;
}

micron::task<i64>
tree_sum(i64 depth, i64 v)
{
  if ( depth == 0 ) {
    val leaf{ v };
    co_return leaf.v;
  }
  i64 a = 0, b = 0;
  co_await coro::fork[&a, tree_sum](depth - 1, v);
  co_await coro::call[&b, tree_sum](depth - 1, v);
  co_await coro::join;
  co_return a + b;
}

}      // namespace

int
main(void)
{
  sb::print("=== CORO FRAME / VALUE LIFETIME ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  fanout: ", static_cast<usize>(FANOUT),
            "  scale: ", static_cast<usize>(ltest::stress_scale));

  val::reg = &g_reg;

  const i32 wm0 = ltest::fd_watermark();
  coro::start_coroutine_runtime();

  test_case("task<T> co_return: every value destroyed exactly once");
  {
    val::reset();
    g_reg.reset();
    const u64 n = ltest::scaled(ROUNDS);
    i64 sum = 0;
    for ( u64 i = 0; i < n; ++i ) {
      val r = coro::sync_wait(make_val(static_cast<i64>(i)));
      sum += r.v;
    }
    const i64 want = static_cast<i64>(n * (n - 1) / 2);
    require(sum, want);
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
    require(g_reg.collisions.get(micron::memory_order_acquire), static_cast<u64>(0));
    sb::print("     born=", static_cast<usize>(val::born()), " dtor=", static_cast<usize>(val::dtor.get(micron::memory_order_acquire)),
              " foreign=", static_cast<usize>(val::foreign_dtor.get(micron::memory_order_acquire)));
  }
  end_test_case();

  test_case("eventual<T>: fork result moved out, slot destroyed once");
  {
    val::reset();
    const u64 n = ltest::scaled(ROUNDS);
    for ( u64 i = 0; i < n; ++i ) {
      val r = coro::sync_wait(via_eventual(static_cast<i64>(i)));
      require(r.v, static_cast<i64>(i));
    }
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("fork[&lv, fn] / call[&lv, fn]: slot overwritten, not leaked");
  {
    val::reset();
    const u64 n = ltest::scaled(ROUNDS);
    for ( u64 i = 0; i < n; ++i ) {
      const i64 got = coro::sync_wait(via_lvalue_slots(static_cast<i64>(i)));
      require(got, static_cast<i64>(2 * i + 1));
    }
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("call(discard, fn): child result built AND destroyed");
  {
    val::reset();
    g_discard_seen.store(0, micron::memory_order_relaxed);
    const u64 n = ltest::scaled(ROUNDS);
    for ( u64 i = 0; i < n; ++i ) coro::sync_wait(drop_child_result(static_cast<i64>(i)));

    require(static_cast<u64>(g_discard_seen.get(micron::memory_order_acquire)), n);
    require(val::born() >= n, true);
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("spawn_many: vector<T> results destroyed with the vector");
  {
    val::reset();
    const u64 rounds = ltest::scaled(ROUNDS / 8);
    for ( u64 r = 0; r < rounds; ++r ) {
      auto out = coro::sync_wait(coro::spawn_many(FANOUT, [](usize i) -> micron::task<val> { co_return val{ static_cast<i64>(i) }; }));
      require(out.size(), static_cast<usize>(FANOUT));
      i64 sum = 0;
      for ( usize i = 0; i < out.size(); ++i ) sum += out[i].v;
      require(sum, static_cast<i64>(FANOUT * (FANOUT - 1) / 2));
    }
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("when_all: heterogeneous tuple results destroyed once");
  {
    val::reset();
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 i = 0; i < n; ++i ) {
      auto t = coro::sync_wait(coro::when_all(make_val(1), make_val(2), make_val(3)));
      require(micron::get<0>(t).v + micron::get<1>(t).v + micron::get<2>(t).v, static_cast<i64>(6));
    }
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("task<T> destroyed while valid: argument freed, body never ran");
  {
    cap::reset();
    const u64 n = ltest::scaled(ROUNDS);
    for ( u64 i = 0; i < n; ++i ) {
      auto t = never_awaited(cap{ static_cast<i64>(i) });
      require_true(t.valid());
      (void)t;
    }
    require(cap::live(), static_cast<i64>(0));
    require(cap::faults(), static_cast<u64>(0));
    sb::print("     dropped-unstarted frames=", static_cast<usize>(n), " cap born=", static_cast<usize>(cap::born()));
  }
  end_test_case();

  test_case("generator<T> dropped mid-iteration: in-flight yield destroyed");
  {
    gen_v::reset();
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      auto g = yield_n(100);
      i64 taken = 0;
      for ( auto it = g.begin(); it != g.end(); ++it ) {
        (void)(*it).v;
        if ( ++taken == 3 ) break;
      }
      require(taken, static_cast<i64>(3));
    }
    require(gen_v::live(), static_cast<i64>(0));
    require(gen_v::faults(), static_cast<u64>(0));
    sb::print("     generator yields born=", static_cast<usize>(gen_v::born()));
  }
  end_test_case();

  test_case("deep fork tree: leaf values balanced across workers");
  {
    val::reset();
    const u64 rounds = ltest::scaled(4);
    for ( u64 r = 0; r < rounds; ++r ) {
      const i64 got = coro::sync_wait(tree_sum(10, 1));
      require(got, static_cast<i64>(1024));
    }
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
    sb::print("     leaves born=", static_cast<usize>(val::born()),
              " foreign dtors=", static_cast<usize>(val::foreign_dtor.get(micron::memory_order_acquire)));
  }
  end_test_case();

  test_case("concurrent submission from OS threads: totals still exact");
  {
    val::reset();
    g_reg.reset();
    const u64 per = ltest::scaled(ROUNDS / 4);
    mtest::parallel(static_cast<int>(THREADS), [per](int id) {
      for ( u64 i = 0; i < per; ++i ) {
        val r = coro::sync_wait(make_val(static_cast<i64>(id) * 1000 + static_cast<i64>(i)));
        (void)r.v;
      }
    });

    require(val::ctor.get(micron::memory_order_acquire), THREADS * per);
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
    require(g_reg.collisions.get(micron::memory_order_acquire), static_cast<u64>(0));
    sb::print("     threads=", static_cast<usize>(THREADS), " per=", static_cast<usize>(per),
              " foreign dtors=", static_cast<usize>(val::foreign_dtor.get(micron::memory_order_acquire)));
  }
  end_test_case();

  coro::stop_coroutine_runtime();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);
  require(g_reg.tracked_n.get(micron::memory_order_acquire), static_cast<u64>(0));

  sb::print("=== ALL CORO FRAME LIFETIME TESTS PASSED ===");
  return 1;
}
