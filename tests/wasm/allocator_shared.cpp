//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt

#include "../../src/bits/wasm/threads.hpp"
#include "../../src/cmalloc.hpp"
#include "../../src/memory/allocation/wasmalloc/wasmalloc.hpp"

namespace
{
constexpr unsigned worker_count = 4;
constexpr unsigned kept_count = 24;
unsigned start = 0;

struct block {
  byte *ptr;
  usize size;
  byte value;
};

struct task {
  unsigned id;
  byte *from_main;
  byte *reservation;
  block kept[kept_count + 1]{};
};

task tasks[worker_count]{};

unsigned
random(unsigned &state)
{
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

bool
verify(const block &b)
{
  if ( !b.ptr ) return b.size == 0;
  for ( usize i = 0; i < b.size; ++i )
    if ( b.ptr[i] != b.value ) return false;
  return true;
}

bool
overlap(const void *a, usize an, const void *b, usize bn)
{
  const auto left = reinterpret_cast<uintptr_t>(a), right = reinterpret_cast<uintptr_t>(b);
  return left < right + bn && right < left + an;
}

int
worker(void *arg)
{
  auto &t = *static_cast<task *>(arg);
  while ( __atomic_load_n(&start, __ATOMIC_ACQUIRE) == 0 ) {
  }
  if ( ::calloc(static_cast<usize>(-1), 2) != nullptr || errno != 12 ) return 6;
  errno = 100 + t.id;
  for ( unsigned i = 0; i < 193; ++i )
    if ( t.from_main[i] != t.id + 11 ) return 6;
  auto *moved = static_cast<byte *>(micron::wasmalloc::realloc(t.from_main, 8192));
  if ( !moved ) return 6;
  for ( unsigned i = 0; i < 193; ++i )
    if ( moved[i] != t.id + 11 ) return 6;
  micron::wasmalloc::dealloc(moved);
  t.from_main = nullptr;
  t.reservation = static_cast<byte *>(micron::wasm::linear_reserve(8192, 64));
  if ( !t.reservation ) return 6;
  __builtin_memset(t.reservation, t.id + 31, 8192);

  block live[24]{};
  unsigned rng = 0xbb67ae85u ^ (t.id * 0x9e3779b9u);
  for ( unsigned step = 0; step < 6000; ++step ) {
    auto &b = live[random(rng) % 24];
    if ( !verify(b) ) return 6;
    if ( b.ptr && (random(rng) & 3) == 0 ) {
      micron::wasmalloc::dealloc(b.ptr);
      b = {};
      continue;
    }
    const usize bytes = 1 + random(rng) % 32768;
    auto *next = static_cast<byte *>(micron::wasmalloc::realloc(b.ptr, bytes));
    if ( !next ) return 6;
    for ( usize i = 0; i < (bytes < b.size ? bytes : b.size); ++i )
      if ( next[i] != b.value ) return 6;
    b = { next, bytes, static_cast<byte>((t.id * 17 + step) | 1) };
    __builtin_memset(b.ptr, b.value, b.size);
  }
  for ( auto &b : live ) {
    if ( !verify(b) ) return 6;
    micron::wasmalloc::dealloc(b.ptr);
  }
  for ( unsigned i = 0; i <= kept_count; ++i ) {
    const usize bytes = i == kept_count ? 4u << 20 : 4096 + (t.id * kept_count + i) * 31;
    auto *p = micron::wasmalloc::alloc(bytes);
    if ( !p ) return 6;
    t.kept[i] = { p, bytes, static_cast<byte>(t.id * 37 + i) };
    __builtin_memset(p, t.kept[i].value, bytes);
  }
  if ( errno != 100 + static_cast<int>(t.id) ) return 6;
  return 1;
}
}      // namespace

int
main()
{
#if !defined(__micron_wasm_threads)
  micron::wasm::thread_handle unused;
  return micron::wasm::spawn(worker, nullptr, unused) == -95 ? 1 : 6;
#else
  micron::wasm::thread_handle handles[worker_count]{};
  errno = 77;
  const u32 growth_before = micron::wasm::memory_growth_epoch();
  for ( unsigned i = 0; i < worker_count; ++i ) {
    tasks[i].id = i;
    tasks[i].from_main = micron::wasmalloc::alloc(193);
    if ( !tasks[i].from_main ) return 6;
    __builtin_memset(tasks[i].from_main, i + 11, 193);
    if ( micron::wasm::spawn(worker, &tasks[i], handles[i]) != 0 ) return 6;
  }
  __atomic_store_n(&start, 1u, __ATOMIC_RELEASE);
  for ( auto &handle : handles ) {
    int result = 0;
    if ( micron::wasm::join(handle, &result) != 0 || result != 1 ) return 6;
  }
  if ( errno != 77 ) return 6;
  if ( micron::wasm::memory_growth_epoch() <= growth_before ) return 6;
  for ( unsigned t = 0; t < worker_count; ++t ) {
    for ( unsigned j = 0; j < 8192; ++j )
      if ( tasks[t].reservation[j] != t + 31 ) return 6;
    if ( micron::wasmalloc::within(tasks[t].reservation) ) return 6;
    for ( unsigned i = 0; i <= kept_count; ++i ) {
      const auto &b = tasks[t].kept[i];
      if ( !verify(b) ) return 6;
      for ( unsigned u = 0; u < worker_count; ++u ) {
        if ( overlap(b.ptr, b.size, tasks[u].reservation, 8192) ) return 6;
        for ( unsigned k = 0; k <= kept_count; ++k )
          if ( (t != u || i != k) && overlap(b.ptr, b.size, tasks[u].kept[k].ptr, tasks[u].kept[k].size) ) return 6;
      }
    }
  }
  for ( auto &t : tasks )
    for ( auto &b : t.kept ) micron::wasmalloc::dealloc(b.ptr);
  return 1;
#endif
}
