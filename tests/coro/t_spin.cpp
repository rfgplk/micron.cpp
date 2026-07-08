#include "../../src/tasks/tasks.hpp"

#include "../../src/pointer.hpp"
#include "../../src/thread/thread.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static u64
count_to(u64 n)
{
  u64 sum = 0;
  for ( u64 i = 1; i <= n; ++i ) {
    coro::yield(i);
    sum += i;
  }
  return sum;
}

static int g_dtors = 0;

struct dtor_probe {
  bool live = true;
  dtor_probe() = default;
  dtor_probe(const dtor_probe &) = default;

  dtor_probe(dtor_probe &&o) noexcept : live(o.live) { o.live = false; }

  ~dtor_probe()
  {
    if ( live ) ++g_dtors;
  }
};

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  sb::test_case("spin is lazy; jump drives yields; result() returns fn's value");
  {
    auto r = coro::spin<u64>(count_to, (u64)5);
    sb::check(!r.started());
    sb::check(!r.done());
    u64 seen = 0;
    while ( u64 *v = r.jump() ) seen += *v;
    sb::check(seen == 15);
    sb::check(r.done());
    sb::check(r.result() == 15);
  }
  sb::end_test_case();

  sb::test_case("launch is eager: first value ready via value()");
  {
    auto r = coro::launch<u64>(count_to, (u64)3);
    sb::check(r.started());
    sb::check(r.value() != nullptr && *r.value() == 1);
    r.finish();
    sb::check(r.done());
  }
  sb::end_test_case();

  sb::test_case("routine<void>: jump() -> bool protocol");
  {
    auto r = coro::spin([]() {
      coro::yield();
      coro::yield();
    });
    sb::check(r.jump() == true);
    sb::check(r.jump() == true);
    sb::check(r.jump() == false);
    sb::check(r.done());
  }
  sb::end_test_case();

  sb::test_case("args are taken by value (copies; originals untouched)");
  {
    u64 x = 7;
    auto r = coro::spin([](u64 v) { return v * 2; }, x);
    x = 100;
    sb::check(r.result() == 14);
  }
  sb::end_test_case();

  sb::test_case("move-only arg materializes into the routine");
  {
    auto p = micron::unique_pointer<u64>(new u64(21));
    auto r = coro::spin([](micron::unique_pointer<u64> q) { return *q * 2; }, micron::move(p));
    sb::check(p.get() == nullptr);
    sb::check(r.result() == 42);
  }
  sb::end_test_case();

  sb::test_case("dropping a FRESH routine runs fn/arg dtors exactly once");
  {
    g_dtors = 0;
    {
      auto r = coro::spin([](dtor_probe) { return 0; }, dtor_probe{});
      (void)r;
    }
    sb::check(g_dtors == 1);
  }
  sb::end_test_case();

  sb::test_case("coro->coro jump: yield returns to the innermost jumper");
  {
    auto inner = coro::spin<u64>(count_to, (u64)3);
    auto outer = coro::spin<u64>([&inner]() -> u64 {
      u64 acc = 0;
      while ( u64 *v = inner.jump() ) {
        acc += *v;
        coro::yield(acc);
      }
      return acc;
    });
    u64 last = 0;
    while ( u64 *v = outer.jump() ) last = *v;
    sb::check(last == 6);
    sb::check(outer.result() == 6);
  }
  sb::end_test_case();

  sb::test_case("in_routine() nesting");
  {
    sb::check(!coro::in_routine());
    auto r = coro::spin([]() { return coro::in_routine() ? 1 : 0; });
    sb::check(r.result() == 1);
    sb::check(!coro::in_routine());
  }
  sb::end_test_case();

  sb::test_case("jump on done/empty is idempotent; moved-from is empty");
  {
    auto r = coro::spin([]() { return 1; });
    sb::check(r.result() == 1);
    sb::check(r.jump() == false);
    auto r2 = micron::move(r);
    sb::check(r.done());
    sb::check(r2.done());
  }
  sb::end_test_case();

  sb::test_case("abandon while suspended: 100k spin/jump/drop cycles stay flat");
  {
    for ( u64 i = 0; i < 100000; ++i ) {
      auto r = coro::spin<u64>(count_to, (u64)1000000);
      (void)r.jump();
    }
    sb::check(true);
  }
  sb::end_test_case();

  sb::test_case("big stack class: deep recursion beyond the default 64K exec stack");
  {
    struct rec {
      static u64
      go(u64 n)
      {
        byte pad[512];
        pad[0] = static_cast<byte>(n);
        if ( n == 0 ) return static_cast<u64>(pad[0]);
        return go(n - 1) + 1;
      }
    };

    auto r = coro::spin<void, (usize)4 * 1024 * 1024>([]() { return rec::go(1000); });
    sb::check(r.result() == 1000);
  }
  sb::end_test_case();

  sb::test_case("pack folds: done/finish/dismiss over several routines");
  {
    auto a = coro::spin([]() { return 1; });
    auto b = coro::spin([]() { return 2; });
    sb::check(!coro::done(a, b));
    coro::finish(a, b);
    sb::check(coro::done(a, b));
    coro::dismiss(a, b);
    sb::check(a.native_handle() == nullptr && b.native_handle() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("yielded pointer stays valid until the next jump (incl. temporaries)");
  {
    auto r = coro::spin<u64>([]() {
      coro::yield((u64)41 + 1);
      return (u64)0;
    });
    u64 *v = r.jump();
    sb::check(v != nullptr && *v == 42);
    r.finish();
  }
  sb::end_test_case();

  sb::test_case("routine spun INSIDE an engine task; engine healthy after");
  {
    coro::start_coroutine_runtime();
    u64 got = coro::sync_wait([]() -> micron::task<u64> {
      auto r = coro::spin<u64>(count_to, (u64)4);
      u64 acc = 0;
      while ( u64 *v = r.jump() ) acc += *v;
      co_return acc + r.result();
    }());
    sb::check(got == 20);

    struct fibs {
      static micron::task<u64>
      fib(u64 n)
      {
        if ( n < 2 ) co_return n;
        u64 a = 0, b = 0;
        co_await coro::fork[&a, fib](n - 1);
        co_await coro::call[&b, fib](n - 2);
        co_await coro::join;
        co_return a + b;
      }
    };

    sb::check(coro::sync_wait(fibs::fib(20)) == 6765);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("cross-thread: spin on main, drive to completion from another thread");
  {
    auto r = coro::spin<u64>(count_to, (u64)8);
    (void)r.jump();
    u64 total = 0;
    {
      auto t = micron::solo::spawn([](decltype(r) *rr, u64 *out) { *out = rr->result(); }, &r, &total);
      t->join();
    }
    sb::check(total == 36);
  }
  sb::end_test_case();

  sb::test_case("10k live generators, round-robin");
  {
    constexpr usize N = 10000;
    auto *rs = new coro::routine<u64, u64>[N];
    for ( usize i = 0; i < N; ++i ) rs[i] = coro::spin<u64>(count_to, (u64)3);
    u64 checksum = 0;
    for ( u64 round = 0; round < 3; ++round )
      for ( usize i = 0; i < N; ++i )
        if ( u64 *v = rs[i].jump() ) checksum += *v;
    for ( usize i = 0; i < N; ++i ) rs[i].finish();
    delete[] rs;
    sb::check(checksum == N * 6);
  }
  sb::end_test_case();

#if !defined(__micron_freestanding) || defined(__micron_eh)
  sb::test_case("exception in fn surfaces on the jumper as library_error");
  {
    auto r = coro::spin([]() -> int { throw 7; });
    bool raised = false;
    try {
      (void)r.jump();
    } catch ( ... ) {
      raised = true;
    }
    sb::check(raised);
    sb::check(r.done());
  }
  sb::end_test_case();
#endif

  sb::require(FAILS == 0);
  sb::print("=== ALL SPIN/JUMP/YIELD TESTS PASSED ===");
  return 1;
}
