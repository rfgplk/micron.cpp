//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// demo: micron running in ring 0.

#include "../../src/maps.hpp"
#include "../../src/port/port.hpp"
#include "../../src/print.hpp"
#include "../../src/sort/sort.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector.hpp"

#if !defined(__micron_port_kernel)
#error "mod_demo.cpp must be built with -DMICRON_PORT_KERNEL"
#endif

namespace
{

// deterministic, so the printed numbers are checkable against the source rather than "some value"
constexpr u64 kElems = 4096;

u64
splitmix(u64 &s) noexcept
{
  u64 z = (s += 0x9E37'79B9'7F4A'7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58'476D'1CE4'E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D0'49BB'1331'11EBull;
  return z ^ (z >> 31);
}

};      // namespace

extern "C" int
mc_demo_start(void)
{
  micron::println("hello from ring 0");

  // %%%% vector + sort
  micron::vector<u64> v;
  u64 seed = 0x0BADC0DE'12345678ull;
  for ( u64 i = 0; i < kElems; ++i ) v.push_back(splitmix(seed) % 100000u);
  micron::sort::quick(v);

  bool sorted = true;
  for ( usize i = 1; i < v.size(); ++i )
    if ( v[i - 1] > v[i] ) sorted = false;

  u64 sum = 0;
  for ( auto x : v ) sum += x;
  micron::println("vector n=", v.size(), " sorted=", sorted, " sum=", sum, " min=", v[0], " max=", v[v.size() - 1]);

  // %%%% hopscotch map
  micron::hopscotch_map<u64, u64> m;
  for ( u64 i = 0; i < 2048; ++i ) m.insert(i, i * 3);
  u64 msum = 0;
  for ( u64 i = 0; i < 2048; ++i ) msum += m[i];
  const u64 mwant = (2047ull * 2048ull / 2ull) * 3ull;
  micron::println("map n=", m.size(), " sum=", msum, " expected=", mwant, " ok=", msum == mwant);

  // %%%% string
  micron::string s = "micron";
  for ( int i = 0; i < 64; ++i ) s += "-barebones";
  micron::println("string len=", s.size(), " head=", "micron-barebones");

  // %%%% hashing
  const u64 h = micron::hash64(reinterpret_cast<const byte *>("micron in the kernel"), 20);
  micron::println("hash64 = ", h);

  // %%%% the port surface, reported so dmesg shows the seam is live
  micron::println("cpu=", micron::port::cpu_id(), " pid=", micron::port::exec_id(), " tgid=", micron::port::process_id());
  const auto he = micron::port::heap_extent();
  micron::println("ram total=", he.total, " free=", he.free);
  micron::println("mono_ns=", micron::port::mono_ticks());

  const bool ok = sorted && (v.size() == kElems) && (msum == mwant) && (s.size() == 6 + 64 * 10);
  micron::println(ok ? "ALL CHECKS PASSED" : "CHECKS FAILED");
  return ok ? 0 : -22;      // -EINVAL, so a failure refuses to load rather than reporting success
}

extern "C" void
mc_demo_stop(void)
{
  micron::println("demo teardown");
}
