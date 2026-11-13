//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0x6a09e667u
#endif
#ifndef MICRON_WASM_FUZZ_STEPS
#define MICRON_WASM_FUZZ_STEPS 20000
#endif

#include "../../src/bits/wasm/threads.hpp"
#include "../../src/memory/allocation/wasmalloc/wasmalloc.hpp"
#include "fuzz_support.hpp"

namespace
{
namespace wa = micron::wasmalloc;

struct record {
  byte *ptr{};
  usize bytes{}, capacity{}, alignment{};
  u32 salt{};
};

byte
pattern(u32 salt, usize offset)
{
  const u32 mixed = salt ^ (u32(offset) * 0x9e3779b9u);
  return byte((mixed ^ (mixed >> 11) ^ (mixed >> 23)) & 255u);
}

void
fill(const record &item)
{
  for ( usize i = 0; i < item.bytes; ++i ) item.ptr[i] = pattern(item.salt, i);
}

bool
matches(const record &item, usize count)
{
  for ( usize i = 0; i < count; ++i )
    if ( item.ptr[i] != pattern(item.salt, i) ) return false;
  return true;
}

bool
check_all(const record *items, usize count)
{
  for ( usize i = 0; i < count; ++i ) {
    const auto &a = items[i];
    if ( !WASM_FUZZ_CHECK(matches(a, a.bytes)) ) return false;
    if ( !a.ptr ) continue;
    const u64 begin = reinterpret_cast<uintptr_t>(a.ptr), end = begin + a.capacity;
    for ( usize j = i + 1; j < count; ++j ) {
      if ( !items[j].ptr ) continue;
      const u64 other = reinterpret_cast<uintptr_t>(items[j].ptr);
      if ( !WASM_FUZZ_CHECK(end <= other || other + items[j].capacity <= begin) ) return false;
    }
  }
  return true;
}

bool
trace(usize slots, u32 iterations, u32 seed)
{
  record items[64]{};
  constexpr usize edges[] = { 1,    15,   16,   17,   31,   32,   33,   63,   64,    65,    255,   256,   257,  1007,
                              1008, 1009, 1023, 1024, 1025, 4095, 4096, 4097, 16383, 16384, 16385, 32767, 65535 };
  for ( u32 step = 0; step < iterations; ++step ) {
    wasm_fuzz::step = step;
    auto &item = items[wasm_fuzz::next(seed) % slots];
    if ( !WASM_FUZZ_CHECK(matches(item, item.bytes)) ) return false;
    const unsigned op = wasm_fuzz::next(seed) & 7u;
    const usize bytes = (wasm_fuzz::next(seed) & 1) ? edges[wasm_fuzz::next(seed) % (sizeof(edges) / sizeof(edges[0]))]
                                                    : 1 + wasm_fuzz::next(seed) % 65536;
    const usize alignment = usize(1) << (wasm_fuzz::next(seed) % 17);
    if ( op == 0 ) {
      wa::dealloc(item.ptr);
      item = {};
    } else if ( op == 1 ) {
      const auto result = wa::aligned_resize({ item.ptr, item.bytes }, 0, item.bytes, alignment);
      if ( !WASM_FUZZ_CHECK(result.ptr == nullptr && result.len == 0) ) return false;
      item = {};
    } else if ( op == 2 ) {
      const auto failed = wa::aligned_resize({ item.ptr, item.bytes }, static_cast<usize>(-1), item.bytes, alignment);
      if ( !WASM_FUZZ_CHECK(failed.ptr == nullptr && failed.len == 0) || !WASM_FUZZ_CHECK(matches(item, item.bytes)) ) return false;
      const auto misaligned = wa::aligned_resize({ item.ptr, item.bytes }, bytes, item.bytes, 3);
      if ( !WASM_FUZZ_CHECK(misaligned.ptr == nullptr) || !WASM_FUZZ_CHECK(matches(item, item.bytes)) ) return false;
    } else {
      micron::chunk<byte> result;
      usize keep = 0;
      if ( op == 3 ) {
        wa::dealloc(item.ptr);
        item = {};
        result = wa::zalloc(bytes);
        if ( !WASM_FUZZ_CHECK(result.ptr != nullptr && result.len >= bytes) ) return false;
        bool zero = true;
        for ( usize i = 0; i < result.len; ++i ) zero &= result.ptr[i] == 0;
        if ( !WASM_FUZZ_CHECK(zero) ) return false;
      } else if ( op == 4 ) {
        wa::dealloc(item.ptr);
        item = {};
        result = wa::aligned_balloc(alignment, bytes);
      } else {
        const usize preserve = item.bytes ? wasm_fuzz::next(seed) % (item.bytes + 17) : 0;
        keep = preserve < item.bytes ? preserve : item.bytes;
        if ( keep > bytes ) keep = bytes;
        result = wa::aligned_resize({ item.ptr, item.bytes }, bytes, preserve, alignment);
      }
      if ( !WASM_FUZZ_CHECK(result.ptr != nullptr && result.len >= bytes) ) return false;
      const usize placed = op == 3 ? wa::native_block_alignment : alignment;
      if ( !WASM_FUZZ_CHECK((reinterpret_cast<uintptr_t>(result.ptr) & (placed - 1)) == 0)
           || !WASM_FUZZ_CHECK(wa::is_present(result.ptr) && wa::within(result.ptr))
           || !WASM_FUZZ_CHECK(wa::query_size(result.ptr) == result.len)
           || !WASM_FUZZ_CHECK(matches({ result.ptr, keep, result.len, placed, item.salt }, keep)) )
        return false;
      item = { result.ptr, bytes, result.len, placed, wasm_fuzz::next(seed) };
      fill(item);
    }
    if ( step % 64 == 0 && !check_all(items, slots) ) return false;
  }
  const bool okay = check_all(items, slots);
  for ( auto &item : items ) wa::dealloc(item.ptr);
  return okay;
}

bool
boundaries()
{
  if ( !WASM_FUZZ_CHECK(wa::try_alloc(0) == nullptr) || !WASM_FUZZ_CHECK(wa::calloc(static_cast<usize>(-1), 2) == nullptr)
       || !WASM_FUZZ_CHECK(wa::aligned_balloc(0, 8).ptr == nullptr) || !WASM_FUZZ_CHECK(wa::aligned_balloc(3, 8).ptr == nullptr)
       || !WASM_FUZZ_CHECK(wa::aligned_balloc(1u << 31, 8).ptr == nullptr) )
    return false;
  wa::dealloc(nullptr);
  for ( usize alignment = 1; alignment <= 1u << 20; alignment *= 2 ) {
    const auto item = wa::aligned_balloc(alignment, 73);
    if ( !WASM_FUZZ_CHECK(item.ptr != nullptr && item.len >= 73)
         || !WASM_FUZZ_CHECK((reinterpret_cast<uintptr_t>(item.ptr) & (alignment - 1)) == 0) )
      return false;
    wa::dealloc(item.ptr);
  }
  record control{ wa::alloc(17), 17, 0, 16, 0x510e527fu };
  if ( !WASM_FUZZ_CHECK(control.ptr != nullptr) ) return false;
  fill(control);
  control.ptr[8] ^= 1;
  if ( !WASM_FUZZ_CHECK(!matches(control, control.bytes)) ) return false;
  control.ptr[8] ^= 1;
  if ( !WASM_FUZZ_CHECK(matches(control, control.bytes)) ) return false;
  wa::dealloc(control.ptr);
  return true;
}

bool
exhaustion()
{
  record retained{ wa::alloc(1023), 1023, 0, 16, 0x1f83d9abu };
  if ( !WASM_FUZZ_CHECK(retained.ptr != nullptr) ) return false;
  fill(retained);
  constexpr usize pressure_bytes = 2u << 20;
  void *held[256]{};
  usize count = 0;
  while ( count < sizeof(held) / sizeof(held[0]) ) {
    void *next = wa::try_alloc(pressure_bytes);
    if ( !next ) break;
    held[count++] = next;
  }
  if ( !WASM_FUZZ_CHECK(count > 0 && count < sizeof(held) / sizeof(held[0])) ) return false;
  const u64 cursor = micron::wasm::__linear_cursor;
  if ( !WASM_FUZZ_CHECK(wa::realloc(retained.ptr, 32u << 20) == nullptr) || !WASM_FUZZ_CHECK(micron::wasm::__linear_cursor == cursor)
       || !WASM_FUZZ_CHECK(matches(retained, retained.bytes)) )
    return false;
  for ( usize i = 0; i < count; ++i ) wa::dealloc(held[i]);
  void *reused = wa::try_alloc(pressure_bytes);
  if ( !WASM_FUZZ_CHECK(reused != nullptr && micron::wasm::__linear_cursor == cursor) ) return false;
  wa::dealloc(reused);
  wa::dealloc(retained.ptr);
  if ( !WASM_FUZZ_CHECK(micron::wasm::linear_reserve(static_cast<usize>(-1), 16) == nullptr)
       || !WASM_FUZZ_CHECK(micron::wasm::__linear_cursor == cursor) )
    return false;
  return true;
}

#if defined(__micron_wasm_threads)
struct job {
  u32 seed;
  record transferred;
};

job jobs[4]{};
unsigned gate = 0;

int
worker(void *arg)
{
  auto &task = *static_cast<job *>(arg);
  while ( !__atomic_load_n(&gate, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&gate, 0);
  if ( !WASM_FUZZ_CHECK(matches(task.transferred, task.transferred.bytes)) || !trace(16, (wasm_fuzz::steps + 3) / 4, task.seed) ) return 6;
  const auto next = wa::aligned_resize({ task.transferred.ptr, task.transferred.bytes }, 8193, task.transferred.bytes, 4096);
  if ( !WASM_FUZZ_CHECK(next.ptr != nullptr) || !WASM_FUZZ_CHECK(matches({ next.ptr, 193, next.len, 4096, task.transferred.salt }, 193)) )
    return 6;
  task.transferred = { next.ptr, 8193, next.len, 4096, task.seed };
  fill(task.transferred);
  return 1;
}

bool
shared_trace()
{
  micron::wasm::thread_handle handles[4]{};
  for ( unsigned i = 0; i < 4; ++i ) {
    jobs[i].seed = wasm_fuzz::seed ^ (0x9e3779b9u * (i + 1));
    if ( !jobs[i].seed ) jobs[i].seed = 0xbb67ae85u;
    jobs[i].transferred = { wa::alloc(193), 193, 0, 16, i + 1 };
    if ( !WASM_FUZZ_CHECK(jobs[i].transferred.ptr != nullptr) ) return false;
    fill(jobs[i].transferred);
    if ( !WASM_FUZZ_CHECK(micron::wasm::spawn(worker, &jobs[i], handles[i]) == 0) ) return false;
  }
  __atomic_store_n(&gate, 1u, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(&gate);
  for ( auto &handle : handles ) {
    int result = 0;
    if ( !WASM_FUZZ_CHECK(micron::wasm::join(handle, &result) == 0 && result == 1) ) return false;
  }
  record transferred[4];
  for ( unsigned i = 0; i < 4; ++i ) transferred[i] = jobs[i].transferred;
  if ( !check_all(transferred, 4) ) return false;
  for ( auto &task : jobs ) wa::dealloc(task.transferred.ptr);
  return true;
}
#endif
}      // namespace

int
main()
{
  auto *reservation = static_cast<byte *>(micron::wasm::linear_reserve(4096, 4096));
  if ( !WASM_FUZZ_CHECK(reservation != nullptr && !wa::within(reservation)) ) return 6;
  for ( unsigned i = 0; i < 4096; ++i ) reservation[i] = pattern(0x5be0cd19u, i);
  if ( !boundaries() || !trace(64, wasm_fuzz::steps, wasm_fuzz::seed) ) return 6;
#if defined(__micron_wasm_threads)
  if ( !shared_trace() ) return 6;
#endif
  if ( !exhaustion() ) return 6;
  bool intact = true;
  for ( unsigned i = 0; i < 4096; ++i ) intact &= reservation[i] == pattern(0x5be0cd19u, i);
  if ( !WASM_FUZZ_CHECK(intact && !wa::within(reservation)) ) return 6;
  return wasm_fuzz::finish("memory");
}
