//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/allocator.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mock_allocators.hpp"

namespace
{
alignas(64) byte adapter_storage[64 * 1024]{};
inline micron::arena_resource<micron::allocator_exact<>> adapter_resource{ { adapter_storage, sizeof(adapter_storage) } };
using adapter_allocator = micron::arena_allocator<adapter_resource>;

inline micron::arena_resource<micron::allocator_exact<>, micron::arena_sync::shared> shared_resource{ 32 * 1024 };

struct allocation_record {
  byte *ptr;
  usize len;
};

allocation_record shared_records[512]{};

void
shared_worker(usize worker)
{
  const usize base = worker * 128;
  for ( usize i = 0; i < 128; ++i ) {
    micron::chunk<byte> memory = shared_resource.allocate<32>(17);
    memory.ptr[0] = static_cast<byte>(worker + 1);
    shared_records[base + i] = { memory.ptr, memory.len };
  }
}

bool
non_overlapping(const allocation_record *records, usize count)
{
  for ( usize i = 0; i < count; ++i ) {
    if ( records[i].ptr == nullptr || records[i].len == 0 ) return false;
    const uintptr_t alo = reinterpret_cast<uintptr_t>(records[i].ptr);
    const uintptr_t ahi = alo + records[i].len;
    for ( usize j = i + 1; j < count; ++j ) {
      const uintptr_t blo = reinterpret_cast<uintptr_t>(records[j].ptr);
      const uintptr_t bhi = blo + records[j].len;
      if ( alo < bhi && blo < ahi ) return false;
    }
  }
  return true;
}
}      // namespace

int
main()
{
  sb::print("=== ARENA RESOURCE RIGOR ===");

  sb::test_case("external spans fail strictly without touching caller ownership");
  {
    alignas(64) byte storage[128]{};
    micron::arena_resource<micron::allocator_exact<>> arena{ { storage, sizeof(storage) } };
    micron::chunk<byte> first = arena.allocate<64>(96);
    sb::require(first.ptr, storage);
    bool exhausted = false;
    try {
      (void)arena.allocate<16>(33);
    } catch ( const micron::except::memory_error & ) {
      exhausted = true;
    }
    sb::require_true(exhausted);
    sb::require_true(arena.owns(storage + 127));
    arena.release();
    storage[0] = 0x5a;
    sb::require(storage[0], static_cast<byte>(0x5a));
    sb::require(arena.capacity(), sizeof(storage));
    sb::require(arena.used(), usize{ 0 });
  }
  sb::end_test_case();

  sb::test_case("external overflow opt-in embeds descriptors in owned blocks and releases only upstream storage");
  {
    using upstream = mtest::tracking_allocator<110>;
    upstream::reset();
    alignas(64) byte storage[96]{};
    {
      micron::arena_resource<upstream> arena{ { storage, sizeof(storage) }, micron::arena_overflow::upstream, 128 };
      (void)arena.allocate<32>(80);
      micron::chunk<byte> overflow = arena.allocate<64>(80);
      sb::require_true(overflow.ptr != nullptr && !arena.owns(storage + sizeof(storage)));
      sb::require_true(upstream::outstanding() > 0);
      arena.release();
      sb::require(upstream::outstanding(), i64{ 0 });
      sb::require(arena.block_count(), usize{ 1 });
    }
    storage[95] = 0xa5;
    sb::require(storage[95], static_cast<byte>(0xa5));
  }
  sb::end_test_case();

  sb::test_case("markers rewind exact bump state and reject newer or stale generations");
  {
    micron::arena_resource<micron::allocator_exact<>> arena{ 256 };
    micron::chunk<byte> first = arena.allocate<16>(31);
    auto early = arena.mark();
    micron::chunk<byte> second = arena.allocate<32>(47);
    auto late = arena.mark();
    (void)arena.allocate<16>(23);
    sb::require_true(arena.rewind(early));
    sb::require_false(arena.rewind(late));
    sb::require(arena.allocate<32>(47).ptr, second.ptr);
    sb::require_true(arena.owns(first.ptr));
    arena.reset();
    sb::require_false(arena.rewind(early));
    arena.release();
  }
  sb::end_test_case();

  sb::test_case("latest allocation resizes in place and non-latest relocation preserves the requested prefix");
  {
    micron::arena_resource<micron::allocator_exact<>> arena{ 512 };
    micron::chunk<byte> latest = arena.allocate<64>(32);
    for ( usize i = 0; i < latest.len; ++i ) latest.ptr[i] = static_cast<byte>(i + 1);
    byte *original = latest.ptr;
    latest = arena.resize<64>(latest, 160, 32);
    sb::require(latest.ptr, original);
    for ( usize i = 0; i < 32; ++i ) sb::require(latest.ptr[i], static_cast<byte>(i + 1));

    micron::chunk<byte> old = arena.allocate<16>(24);
    for ( usize i = 0; i < old.len; ++i ) old.ptr[i] = static_cast<byte>(0xc0 + i);
    (void)arena.allocate<16>(24);
    micron::chunk<byte> moved = arena.resize<16>(old, 700, old.len);
    sb::require_true(moved.ptr != old.ptr);
    for ( usize i = 0; i < old.len; ++i ) sb::require(moved.ptr[i], static_cast<byte>(0xc0 + i));
    arena.release();
  }
  sb::end_test_case();

  sb::test_case("owned reset retains blocks, release returns them, and disabled telemetry stays explicit");
  {
    using upstream = mtest::tracking_allocator<111>;
    upstream::reset();
    micron::arena_resource<upstream> arena{ 128 };
    micron::chunk<byte> first = arena.allocate<64>(96);
    (void)arena.allocate<64>(96);
    const i64 retained = upstream::outstanding();
    sb::require_true(retained >= 2);
    arena.reset();
    sb::require(upstream::outstanding(), retained);
    sb::require(arena.allocate<64>(96).ptr, first.ptr);
    const micron::allocator_stats_snapshot stats = arena.stats();
#if defined(MICRON_ALLOCATOR_STATS)
    sb::require_true(stats.enabled);
    sb::require_true(stats.allocations > 0 && stats.resets > 0);
#else
    sb::require_false(stats.enabled);
#endif
    arena.release();
    sb::require(upstream::outstanding(), i64{ 0 });
    sb::require_false(arena.owns(first.ptr));
  }
  sb::end_test_case();

  sb::test_case("static arena adapter drives allocator-aware containers");
  {
    adapter_resource.reset();
    {
      micron::vector<u64, adapter_allocator> values;
      for ( u64 i = 0; i < 512; ++i ) values.push_back(i * 3);
      for ( u64 i = 0; i < 512; ++i ) sb::require(values[i], i * 3);
      sb::require_true(adapter_resource.owns(values.data()));
    }
    adapter_resource.reset();
  }
  sb::end_test_case();

  sb::test_case("shared mode serializes bumps without overlap");
  {
    shared_resource.reset();
    {
      micron::auto_thread<> t0(shared_worker, usize{ 0 });
      micron::auto_thread<> t1(shared_worker, usize{ 1 });
      micron::auto_thread<> t2(shared_worker, usize{ 2 });
      micron::auto_thread<> t3(shared_worker, usize{ 3 });
    }
    sb::require_true(non_overlapping(shared_records, 512));
    shared_resource.release();
  }
  sb::end_test_case();

  sb::print("=== ALL ARENA RESOURCE RIGOR PASSED ===");
  return 1;
}
