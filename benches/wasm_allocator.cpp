//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt

#include "../src/bits/wasm/host.hpp"
#include "../src/memory/allocation/wasmalloc/wasmalloc.hpp"

namespace
{
constexpr unsigned seed = 0x510e527f;
constexpr unsigned rounds = 100000;
constexpr unsigned samples = 7;
volatile unsigned sink = 0;

struct block {
  byte *ptr;
  usize size;
};

block slots[128]{};

unsigned
random(unsigned &state)
{
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

u64
now()
{
  return static_cast<u64>(__micron_wasm_host_clock(1));
}

void
text(const char *s)
{
  unsigned n = 0;
  while ( s[n] ) ++n;
  (void)micron::wasm::write_diag(s, n, 1);
}

void
number(u64 value)
{
  char buf[24];
  unsigned n = sizeof(buf);
  do {
    buf[--n] = '0' + value % 10;
    value /= 10;
  } while ( value );
  (void)micron::wasm::write_diag(buf + n, sizeof(buf) - n, 1);
}

u64
median(u64 (&values)[samples])
{
  for ( unsigned i = 1; i < samples; ++i ) {
    const auto value = values[i];
    unsigned j = i;
    while ( j && values[j - 1] > value ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
  return values[samples / 2];
}

void
row(const char *trace, u64 operations, u64 duration, u64 growth, u64 peak_requested)
{
  text(trace);
  text(",");
#if defined(__micron_wasm_threads)
  text("shared");
#else
  text("single");
#endif
  text(",");
  number(seed);
  text(",");
  number(operations);
  text(",");
  number(duration);
  text(",");
  number(duration ? operations * 1000000000ull / duration : 0);
  text(",");
  number(growth);
  text(",");
  number(peak_requested);
  text(",");
  number(micron::wasmalloc::musage());
  text(",");
  number(micron::wasmalloc::capacity());
  text(",");
  number(micron::wasm::linear_memory_bytes());
  text("\n");
}

bool
alloc_free(u64 &duration, u64 &peak)
{
  unsigned rng = seed;
  u64 live = 0;
  peak = 0;
  const u64 begin = now();
  for ( unsigned i = 0; i < rounds; ++i ) {
    auto &s = slots[i % 128];
    if ( s.ptr ) {
      sink = sink ^ s.ptr[0];
      micron::wasmalloc::dealloc(s.ptr);
      live -= s.size;
    }
    s.size = 13 + random(rng) % 2048;
    s.ptr = micron::wasmalloc::alloc(s.size);
    if ( !s.ptr ) return false;
    s.ptr[0] = static_cast<byte>(rng);
    live += s.size;
    if ( live > peak ) peak = live;
  }
  for ( auto &s : slots ) {
    sink = sink ^ s.ptr[0];
    micron::wasmalloc::dealloc(s.ptr);
    s = {};
  }
  duration = now() - begin;
  return true;
}

bool
resizing(u64 &duration, u64 &peak)
{
  unsigned rng = seed;
  u64 live = 0;
  peak = 0;
  const u64 begin = now();
  for ( unsigned i = 0; i < rounds; ++i ) {
    auto &s = slots[random(rng) % 128];
    const usize bytes = 32 + random(rng) % 8192;
    auto *p = static_cast<byte *>(micron::wasmalloc::realloc(s.ptr, bytes));
    if ( !p || (s.ptr && p[0] != 0x5a) ) return false;
    live = live - s.size + bytes;
    if ( live > peak ) peak = live;
    s = { p, bytes };
    s.ptr[0] = 0x5a;
  }
  for ( auto &s : slots ) {
    sink = sink ^ s.ptr[0];
    micron::wasmalloc::dealloc(s.ptr);
    s = {};
  }
  duration = now() - begin;
  return true;
}
}      // namespace

int
main()
{
  text("trace,profile,seed,operations,median_ns,operations_per_second,growth_events,peak_requested_bytes,live_granted_bytes,heap_capacity_"
       "bytes,linear_memory_bytes\n");
  for ( unsigned trace = 0; trace < 2; ++trace ) {
    auto run = trace == 0 ? alloc_free : resizing;
    u64 duration = 0, peak = 0;
    for ( unsigned warm = 0; warm < 2; ++warm )
      if ( !run(duration, peak) ) return 6;
    const u32 before = micron::wasm::memory_growth_epoch();
    u64 times[samples]{};
    for ( auto &time : times )
      if ( !run(time, peak) ) return 6;
    row(trace == 0 ? "alloc_free" : "realloc", trace == 0 ? rounds * 2 : rounds + 128, median(times),
        micron::wasm::memory_growth_epoch() - before, peak);
  }
  const u32 before = micron::wasm::memory_growth_epoch();
  const u64 begin = now();
  auto *large = micron::wasmalloc::alloc(64u << 20);
  if ( !large ) return 6;
  const u64 elapsed = now() - begin;
  row("cold_growth_64m", 1, elapsed, micron::wasm::memory_growth_epoch() - before, 64u << 20);
  large[0] = 17;
  large[(64u << 20) - 1] = 29;
  sink = sink ^ large[0] ^ large[(64u << 20) - 1];
  micron::wasmalloc::dealloc(large);
  return 1;
}
