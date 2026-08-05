//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#define MICRON_CORO_URING

#include "../../src/io/coroutine/coro_io.hpp"

#include "../../src/attributes.hpp"
#include "../../src/exit.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace mio = micron::io;

static constexpr const char *DIR = "/var/tmp/micron_ring_teardown";

static micron::io::path_t
mkpath(const char *name)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += name;
  return p;
}

static micron::task<int>
churn(micron::io::path_t p, int n)
{
  micron::string payload;
  for ( int i = 0; i < 512; ++i ) payload += "micron ring teardown payload\n";

  for ( int i = 0; i < n; ++i ) {
    const max_t w = co_await cio::write_file(p, payload);
    if ( w < 0 ) co_return -1;
    auto r = co_await cio::read_file(p);
    if ( !r.is_first() ) co_return -2;
  }
  co_return 0;
}

gdestructor_ void
__check_runtime_reaped(void)
{
  if ( coro::__engine_state.get(micron::memory_order_acquire) == 0u && coro::__global_engine == nullptr ) return;
  sb::print("FAIL: coroutine runtime was still up when the ring dtors were about to run");
  micron::sys_group_exit(6);
}

int
main(int, char **)
{
  sb::print("=== CORO RING TEARDOWN ===");

  micron::atexit(&__check_runtime_reaped);

  (void)mio::mkdir(mio::path_t(DIR));

  coro::start_coroutine_runtime();

  sb::test_case("io in flight across several workers");
  {
    micron::task<int> ts[4] = { churn(mkpath("a"), 6), churn(mkpath("b"), 6), churn(mkpath("c"), 6), churn(mkpath("d"), 6) };
    for ( int i = 0; i < 4; ++i ) sb::require(coro::sync_wait(micron::move(ts[i])), 0);
  }
  sb::end_test_case();

  sb::require(coro::io_pending(), static_cast<u64>(0));

  sb::print("=== CORO RING TEARDOWN PASSED (exit code is the real gate) ===");
  return 1;
}
