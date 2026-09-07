//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE BAREBONES KEEP-SET, INSTANTIATED.
//
// Built, never run. Swept in every mode including the two that have no vector unit and no floating
// point at all: x86 -mno-sse -mno-mmx -mno-80387 (a Linux kernel module) and aarch64
// -mgeneral-regs-only. See tests/compiletests/README or verify_compile_barebones.duck.
//
// WARNING: this file is INTEGER-ONLY on purpose, and that is the whole point of it. Under kernel
// flags there is no FP register class, so a function that merely *returns* a float or double is a
// hard error before its body is ever considered -- which is why tests/compiletests/{math,chrono,
// strings,graph,lazy}.cpp fail in those modes and are expected to. Nothing is wrong with them; they
// exercise floating-point APIs, and a target with no FPU cannot have those. A header-only library
// only pays for what it instantiates, so micron's math headers still *include* cleanly there.
//
// What this file pins is the part that must work with no FPU and no vector unit: containers, the
// allocator seam, strings, hashing, sorting, the generic mem* backend, the lazy views, and regex
// over bytes.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/array.hpp"
#include "../../src/hash/hash.hpp"
#include "../../src/heap/binary_heap.hpp"
#include "../../src/lz.hpp"
#include "../../src/maps.hpp"
#include "../../src/memory/cmemory.hpp"
#include "../../src/memory/cstring.hpp"
#include "../../src/queue.hpp"
#include "../../src/regex.hpp"
#include "../../src/sets/sets.hpp"
#include "../../src/sort/sort.hpp"
#include "../../src/stack.hpp"
#include "../../src/strings.hpp"
#include "../../src/trees.hpp"
#include "../../src/tuple.hpp"
#include "../../src/vector.hpp"

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// containers, over an integer payload and over a non-trivial one

struct payload {
  u64 a;
  u32 b;
  u8 c;
};

[[gnu::used]] u64
containers()
{
  u64 acc = 0;

  micron::vector<u64> v;
  for ( u64 i = 0; i < 64; ++i ) v.push_back(i);
  v.reserve(256);
  for ( auto &x : v ) acc += x;

  micron::vector<payload> vp;
  vp.push_back(payload{ 1, 2, 3 });
  acc += vp[0].a;

  micron::array<u32, 16> arr{};
  arr[3] = 7;
  acc += arr[3];

  const u64 key = 0x1234u;
  const u64 h = micron::hash<micron::hash64_t>(key);

  micron::hopscotch_map<u64, u64> hm;
  hm.insert_asis(h, u64{ 2 });
  acc += hm.size();

  micron::rb_map<u64, u64> rb;
  rb.insert_hash(h, u64{ key }, u64{ 4 });
  acc += rb.size();

  micron::hopscotch_set<u64> st;
  st.insert(u64{ 9 });
  acc += st.size();

  micron::stack<u64> sk;
  sk.push(5);
  acc += sk.size();

  micron::queue<u64> q;
  q.push(6);
  acc += q.size();

  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// strings + the generic mem*/str* backend

[[gnu::used]] u64
strings()
{
  micron::string s = "barebones";
  s += "-core";
  u64 acc = s.size();
  acc += micron::strlen(s.c_str());

  byte a[64]{}, b[64]{};
  micron::memset(a, static_cast<byte>(0x5A), sizeof(a));
  micron::memcpy(b, a, sizeof(a));
  acc += static_cast<u64>(micron::memcmp<byte>(a, b, sizeof(a)) == 0);
  micron::memmove(b + 1, b, sizeof(b) - 1);
  acc += static_cast<u64>(b[1]);
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// hashing and sorting -- integer keys, no zzz (it needs AVX2/NEON and is absent on this tier)

[[gnu::used]] u64
hash_sort()
{
  const byte data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  u64 acc = micron::hash64(data, sizeof(data));
  acc += micron::hash<micron::hash64_t>(u64{ 0xABCDEFu });

  micron::vector<u64> v;
  for ( u64 i = 64; i > 0; --i ) v.push_back(i);
  micron::sort::quick(v);
  acc += v[0];
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the lazy views, integer terminals only

[[gnu::used]] u64
lazy()
{
  micron::vector<u64> v;
  for ( u64 i = 0; i < 32; ++i ) v.push_back(i);
  // filter BEFORE fmap: a pull filter dereferences twice, so fmap first would call f again for
  // every survivor (CLAUDE.md, lz gotchas)
  auto out = v | micron::lz::filter([](u64 x) { return (x & 1u) == 0; }) | micron::lz::fmap([](u64 x) { return x * 3u; })
             | micron::lz::collect<micron::vector<u64>>();
  return out.size() ? out[0] + out[out.size() - 1] : 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// regex over bytes. NOT simd::v8: the class lane API does not exist on the generic tier, which is
// what tests/compiletests/regex.cpp trips over in those modes

[[gnu::used]] bool
regex()
{
  micron::rgx::regex re("^[a-z]+[0-9]*$");
  if ( !re.valid() ) return false;
  return re.has_match_n("barebones42", 11);
}

}      // namespace

int
main()
{
  return 1;
}
