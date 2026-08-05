//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1
#define MICRON_CORO_URING

#include "../../src/stdcoro.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"

using namespace snowball;
namespace coro = micron::coro;

namespace
{

constexpr u64 ROUNDS = 64;

micron::vector<i64>
want_vec(usize k)
{
  micron::vector<i64> v(k);
  for ( usize i = 0; i < k; ++i ) v[i] = static_cast<i64>(i * 7u + 3u);
  return v;
}

bool
vec_ok(const micron::vector<i64> &v, usize k)
{
  if ( v.size() != k ) return false;
  for ( usize i = 0; i < k; ++i )
    if ( v[i] != static_cast<i64>(i * 7u + 3u) ) return false;
  return true;
}

bool
str_ok(const micron::string &s, usize k)
{
  if ( s.size() != k ) return false;
  for ( usize i = 0; i < k; ++i )
    if ( s[i] != static_cast<char>('a' + (i % 26u)) ) return false;
  return true;
}

micron::task<micron::vector<i64>>
make_vec(usize k)
{
  co_return want_vec(k);
}

micron::task<micron::string>
make_str(usize k)
{
  micron::string s;
  s.reserve(k + 1);
  for ( usize i = 0; i < k; ++i ) s += static_cast<char>('a' + (i % 26u));
  co_return s;
}

micron::task<i32>
via_eventual_fork(usize k)
{
  coro::eventual<micron::vector<i64>> e;
  co_await coro::fork(&e, make_vec)(k);
  co_await coro::join;
  auto v = micron::move(e).operator*();
  co_return vec_ok(v, k) ? 0 : -6000;
}

micron::task<i32>
via_eventual_call(usize k)
{
  coro::eventual<micron::string> e;
  co_await coro::call(&e, make_str)(k);
  co_await coro::join;
  auto s = micron::move(e).operator*();
  co_return str_ok(s, k) ? 0 : -6001;
}

micron::task<i32>
via_lvalue_slot(usize k)
{
  micron::vector<i64> slot;
  co_await coro::fork(micron::addressof(slot), make_vec)(k);
  co_await coro::join;
  co_return vec_ok(slot, k) ? 0 : -6002;
}

micron::task<i32>
nested(usize outer)
{
  auto rows = co_await coro::spawn_many(outer, [](usize i) -> micron::task<micron::vector<i64>> { co_return co_await make_vec(i + 1u); });
  if ( rows.size() != outer ) co_return -6003;
  for ( usize i = 0; i < rows.size(); ++i )
    if ( !vec_ok(rows[i], i + 1u) ) co_return -6004;
  co_return 0;
}

}      // namespace

int
main(void)
{
  sb::print("=== CORO CONTAINER PAYLOAD (operator& regression) ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  scale: ", static_cast<usize>(ltest::stress_scale));

  const i32 wm0 = ltest::fd_watermark();
  coro::start_coroutine_runtime();

  const u64 n = ltest::scaled(ROUNDS);

  test_case("eventual<vector> and eventual<string>: fork and call both deliver intact contents");
  {
    for ( u64 r = 0; r < n; ++r ) {
      const usize k = 1u + static_cast<usize>(r % 97u);
      require(static_cast<i32>(coro::sync_wait(via_eventual_fork(k))), 0);
      require(static_cast<i32>(coro::sync_wait(via_eventual_call(k))), 0);
    }
  }
  end_test_case();

  test_case("fork lvalue slot of container type receives the whole container");
  {
    for ( u64 r = 0; r < n; ++r ) {
      const usize k = 1u + static_cast<usize>(r % 61u);
      require(static_cast<i32>(coro::sync_wait(via_lvalue_slot(k))), 0);
    }
  }
  end_test_case();

  test_case("futex_future<container> via coro::schedule delivers intact contents");
  {
    for ( u64 r = 0; r < n / 2u; ++r ) {
      const usize k = 1u + static_cast<usize>(r % 53u);
      {
        auto f = coro::schedule(make_vec(k));
        auto v = f.get();
        require_true(vec_ok(v, k));
      }
      {
        auto f = coro::schedule(make_str(k));
        auto s = f.get();
        require_true(str_ok(s, k));
      }
    }
  }
  end_test_case();

  test_case("spawn_many<container> fills every slot with its own container");
  {
    for ( u64 r = 0; r < n / 8u; ++r ) require(static_cast<i32>(coro::sync_wait(nested(24))), 0);
  }
  end_test_case();

  test_case("when_all over heterogeneous container results");
  {
    for ( u64 r = 0; r < n / 4u; ++r ) {
      const usize a = 1u + static_cast<usize>(r % 31u);
      const usize b = 1u + static_cast<usize>(r % 17u);
      auto t = coro::sync_wait(coro::when_all(make_vec(a), make_str(b)));
      require_true(vec_ok(micron::get<0>(t), a));
      require_true(str_ok(micron::get<1>(t), b));
    }
  }
  end_test_case();

  test_case("all paths concurrently across workers");
  {
    micron::vector<micron::futex_future<i32>> fs;
    for ( u64 r = 0; r < n / 2u; ++r ) {
      const usize k = 1u + static_cast<usize>(r % 43u);
      fs.push_back(coro::schedule(via_eventual_fork(k)));
      fs.push_back(coro::schedule(via_eventual_call(k)));
      fs.push_back(coro::schedule(via_lvalue_slot(k)));
    }
    for ( usize i = 0; i < fs.size(); ++i ) require(fs[i].get(), 0);
    sb::print("     concurrent deliveries=", static_cast<usize>(fs.size()));
  }
  end_test_case();

  require(coro::io_pending(), static_cast<u64>(0));
  coro::stop_coroutine_runtime();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  sb::print("=== ALL CORO CONTAINER PAYLOAD TESTS PASSED ===");
  return 1;
}
