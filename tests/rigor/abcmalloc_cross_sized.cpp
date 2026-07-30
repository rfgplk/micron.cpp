//  Copyright (c) 2025 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cross-thread SIZED deallocation.
//
// the rest of the abcmalloc mt suite frees with the sizeless abc::dealloc(ptr), which swallows a
// routing failure. only the sized overload is fatal (memory_error_abc_dealloc_size), and that is
// the path every micron container destructor takes -- so a misroute here terminates the process.
//
// two cases, both of which used to abort on width-32:
//   1. plain cross-thread sized free of a VA-resident block
//   2. the same after the VA reservation is exhausted, i.e. of a block on a sheet that
//      __va_carve could not place inside the reservation (__owner_of must still find its arena)

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/io/console.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../../src/memory/allocation/abcmalloc/sheet_header.hpp"
#include "../../src/memory/allocation/abcmalloc/va_reserve.hpp"
#include "../../src/std.hpp"

#include "../../src/bits/__pause.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr usize N = 192;
constexpr usize SZ = 96;

struct handoff {
  byte *ptrs[N];
  usize lens[N];
  micron::atomic_token<u32> filled{ 0 };
  micron::atomic_token<u32> freed{ 0 };
};

// allocate on this thread, publish, let the other thread free WITH the size
void
producer(handoff *h)
{
  for ( usize i = 0; i < N; ++i ) {
    auto c = abc::balloc(SZ + (i & 31));
    h->ptrs[i] = c.ptr;
    h->lens[i] = c.len;
  }
  h->filled.store(1, micron::memory_order_release);
  while ( h->freed.get(micron::memory_order_acquire) != 1u ) __cpu_pause();
}

void
consumer(handoff *h)
{
  while ( h->filled.get(micron::memory_order_acquire) != 1u ) __cpu_pause();
  for ( usize i = 0; i < N; ++i ) abc::dealloc(h->ptrs[i], h->lens[i]);
  h->freed.store(1, micron::memory_order_release);
}

constexpr usize BIG_N = 24;
constexpr usize BIG_SZ = 1u << 20;      // big enough that each one needs a fresh sheet

struct big_handoff {
  byte *ptrs[BIG_N];
  usize lens[BIG_N];
  micron::atomic_token<u32> filled{ 0 };
  micron::atomic_token<u32> freed{ 0 };
};

void
big_consumer(big_handoff *h)
{
  while ( h->filled.get(micron::memory_order_acquire) != 1u ) __cpu_pause();
  for ( usize i = 0; i < BIG_N; ++i )
    if ( h->ptrs[i] != nullptr ) abc::dealloc(h->ptrs[i], h->lens[i]);
  h->freed.store(1, micron::memory_order_release);
}

// burn the reservation down so the next sheets land outside it
usize
exhaust_va()
{
  usize spun = 0;
  const u64 cap = static_cast<u64>(abc::__va_reservation_size);
  while ( abc::__va_offset.get(micron::memory_order_acquire) < cap ) {
    if ( abc::__va_carve(abc::__sheet_align * 16) == nullptr ) break;
    if ( ++spun > (cap >> abc::__sheet_align_log2) + 8 ) break;
  }
  return spun;
}

};      // namespace

int
main()
{
  test_case("cross-thread sized free (VA resident)");
  {
    handoff h{};
    micron::auto_thread<> p(producer, &h);
    micron::auto_thread<> c(consumer, &h);
    p.join();
    c.join();
    require_true(h.freed.get(micron::memory_order_acquire) == 1u);
  }
  end_test_case();

  test_case("cross-thread sized free (VA exhausted, unregistered sheets)");
  {
    const usize spun = exhaust_va();
    micron::console("burned ", (u64)spun, " granule runs; va_offset=", abc::__va_offset.get(micron::memory_order_acquire), " of ",
                    (u64)abc::__va_reservation_size, "\n");

    // exhausting the reservation is not enough on its own -- the arena keeps serving small
    // requests from sheets it already owns. force fresh sheets with large blocks until one
    // actually lands outside the reservation (that is the block the old __route_dealloc
    // misfiled as "mine").
    big_handoff bh{};
    usize outside = 0;
    for ( usize i = 0; i < BIG_N; ++i ) {
      auto c = abc::balloc(BIG_SZ);
      bh.ptrs[i] = c.ptr;
      bh.lens[i] = c.len;
      if ( c.ptr != nullptr && !abc::__va_contains(c.ptr) ) ++outside;
    }
    micron::console("blocks outside the reservation: ", (u64)outside, " of ", (u64)BIG_N, "\n");
    // if this ever hits, the test has stopped covering what it was written for
    require_true(outside > 0);

    // every one of them must still name its owning arena
    for ( usize i = 0; i < BIG_N; ++i )
      if ( bh.ptrs[i] != nullptr && !abc::__va_contains(bh.ptrs[i]) ) require_true(abc::__owner_of(bh.ptrs[i]) != nullptr);

    // and the sized free must survive crossing a thread. this is the exact interleaving that
    // raised memory_error_abc_dealloc_size and SIGABRT'd t_parallel_{scan,compact} on armv7.
    bh.filled.store(1, micron::memory_order_release);
    micron::auto_thread<> c(big_consumer, &bh);
    c.join();
    require_true(bh.freed.get(micron::memory_order_acquire) == 1u);
  }
  end_test_case();

  micron::console("=== ALL ABCMALLOC CROSS-THREAD SIZED FREE TESTS PASSED ===\n");
  return 1;
}
