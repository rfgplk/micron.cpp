//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/atomic/atomic.hpp"
#include "../../src/attributes.hpp"
#include "../../src/exit.hpp"
#include "../../src/sync/yield.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{

constexpr int __rounds = 24;
constexpr int __workers = 6;

micron::atomic_token<u32> g_ctor[__rounds]{};
micron::atomic_token<u32> g_dtor[__rounds]{};
micron::atomic_token<u32> g_gate{ 0 };
micron::atomic_token<u32> g_arrived{ 0 };

inline void
barrier_wait(int n) noexcept
{
  const u32 prev = g_arrived.fetch_add(1, micron::memory_order_acq_rel);
  if ( prev + 1u == static_cast<u32>(n) ) {
    g_gate.store(1, micron::memory_order_release);
    return;
  }
  while ( g_gate.get(micron::memory_order_acquire) == 0 ) micron::yield();
}

template<int N> struct counted {
  u64 stamp;

  counted() noexcept : stamp(0)
  {
    g_ctor[N].fetch_add(1, micron::memory_order_acq_rel);

    for ( volatile int i = 0; i < 2000; ++i ) stamp += static_cast<u64>(i);
  }

  ~counted() noexcept { g_dtor[N].fetch_add(1, micron::memory_order_acq_rel); }
};

template<int N>
[[gnu::noinline]] counted<N> &
guarded(void) noexcept
{
  static counted<N> __c;
  return __c;
}

template<int N>
inline void
hit_round(int r) noexcept
{
  if ( r == N )
    (void)guarded<N>();
  else if constexpr ( N > 0 )
    hit_round<N - 1>(r);
}

gdestructor_ void
__check_single_destruction(void)
{
  for ( int i = 0; i < __rounds; ++i ) {
    if ( g_dtor[i].get(micron::memory_order_acquire) > 1 ) {
      sb::print("FAIL: function-local static destroyed more than once");
      micron::sys_group_exit(6);
    }
  }
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== FUNCTION-LOCAL STATIC GUARD RIGOR ===");

  micron::atexit(&__check_single_destruction);

  test_case("N threads racing one function-local static construct it exactly once");
  {
    for ( int r = 0; r < __rounds; ++r ) {
      g_gate.store(0, micron::memory_order_release);
      g_arrived.store(0, micron::memory_order_release);

      mtest::parallel(__workers, [r](int) {
        barrier_wait(__workers);
        hit_round<__rounds - 1>(r);
      });
    }

    for ( int r = 0; r < __rounds; ++r ) require(g_ctor[r].get(micron::memory_order_acquire), 1u);
  }
  end_test_case();

  sb::print("=== FUNCTION-LOCAL STATIC GUARD PASSED ===");
  return 1;
}
