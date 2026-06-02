//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/hash/hash.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/trees/art.hpp"

#include "../snowball/snowball.hpp"

#include <cstdio>

namespace
{

inline const char *
clash_byte() noexcept
{
  static const char b = 'x';
  return &b;
}

struct clash {
  using pointer = const char *;
  using const_iterator = const char *;
  using iterator = const char *;

  u32 id;

  clash() noexcept : id(0) { }

  explicit clash(u32 i) noexcept : id(i) { }

  const char *
  data() const noexcept
  {
    return clash_byte();
  }

  const char *
  cbegin() const noexcept
  {
    return clash_byte();
  }

  const char *
  cend() const noexcept
  {
    return clash_byte() + 1;
  }

  const char *
  begin() const noexcept
  {
    return clash_byte();
  }

  const char *
  end() const noexcept
  {
    return clash_byte() + 1;
  }

  usize
  size() const noexcept
  {
    return 1;
  }

  bool
  operator==(const clash &o) const noexcept
  {
    return id == o.id;
  }
};

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

}      // namespace

int
main(void)
{
  sb::print("=== ART ADVERSARIAL TESTS ===");

  sb::test_case("collision chain - insert + find all");
  {
    micron::art<clash, int> a;
    const u32 N = 500;
    for ( u32 i = 0; i < N; ++i ) sb::require(a.insert(clash(i), static_cast<int>(i) * 7));
    sb::require(a.size() == static_cast<usize>(N));
    for ( u32 i = 0; i < N; ++i ) {
      int *v = a.find(clash(i));
      sb::require(v != nullptr);
      sb::require(*v == static_cast<int>(i) * 7);
    }
    sb::require(a.find(clash(N + 1)) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("collision chain - duplicate update + erase head/middle/tail");
  {
    micron::art<clash, int> a;
    const u32 N = 300;
    for ( u32 i = 0; i < N; ++i ) a.insert(clash(i), static_cast<int>(i));
    a.insert(clash(10), 9999);
    sb::require(a.size() == static_cast<usize>(N));
    sb::require(*a.find(clash(10)) == 9999);

    u32 erased = 0;
    for ( u32 i = 0; i < N; i += 2 ) {
      sb::require(a.erase(clash(i)));
      ++erased;
    }
    sb::require(a.size() == static_cast<usize>(N - erased));
    for ( u32 i = 0; i < N; ++i ) {
      int *v = a.find(clash(i));
      if ( i % 2 == 0 )
        sb::require(v == nullptr);
      else
        sb::require(v != nullptr);
    }
    for ( u32 i = 1; i < N; i += 2 ) sb::require(a.erase(clash(i)));
    sb::require(a.empty());
  }
  sb::end_test_case();

  sb::test_case("collision chain - for_each / at / contains");
  {
    micron::art<clash, int> a;
    for ( u32 i = 0; i < 50; ++i ) a.insert(clash(i), static_cast<int>(i));
    sb::require(a.contains(clash(25)));
    sb::require(!a.contains(clash(100)));
    sb::require(a.at(clash(7)) == 7);

    bool threw = false;
    try {
      a.at(clash(100));
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);

    long sum = 0;
    int cnt = 0;
    a.for_each([&](const clash &, const int &v) {
      sum += v;
      ++cnt;
    });
    sb::require(cnt == 50);
    long expect = 0;
    for ( int i = 0; i < 50; ++i ) expect += i;
    sb::require(sum == expect);
  }
  sb::end_test_case();

  sb::test_case("churn - 20000 insert / erase / reinsert");
  {
    micron::art<u64, u64> a;
    const u64 N = 20000;
    for ( u64 i = 0; i < N; ++i ) a.insert(splitmix64(i), i);
    sb::require(a.size() == static_cast<usize>(N));
    for ( u64 i = 0; i < N; i += 2 ) sb::require(a.erase(splitmix64(i)));
    sb::require(a.size() == static_cast<usize>(N / 2));
    for ( u64 i = 0; i < N; i += 2 ) a.insert(splitmix64(i), i + 1);
    sb::require(a.size() == static_cast<usize>(N));
    for ( u64 i = 0; i < N; ++i ) {
      u64 *v = a.find(splitmix64(i));
      sb::require(v != nullptr);
      sb::require(*v == ((i % 2 == 0) ? (i + 1) : i));
    }
    a.clear();
    sb::require(a.empty());
  }
  sb::end_test_case();

  sb::test_case("shared hash-byte prefix - forces split recursion");
  {

    micron::art<u64, int> a;
    const u8 target = static_cast<u8>(micron::hash<u64>(static_cast<u64>(0)) & 0xFFu);
    int found = 0;
    for ( u64 i = 1; found < 64 && i < 4000000ULL; ++i ) {
      if ( static_cast<u8>(micron::hash<u64>(i) & 0xFFu) == target ) {
        a.insert(i, found);
        ++found;
      }
    }
    sb::require(found == 64);
    sb::require(a.size() == 64ULL);
    int seen = 0;
    a.for_each([&](const u64 &, const int &) { ++seen; });
    sb::require(seen == 64);
  }
  sb::end_test_case();

  sb::test_case("move constructor over collision chains");
  {
    micron::art<clash, int> a;
    for ( u32 i = 0; i < 100; ++i ) a.insert(clash(i), static_cast<int>(i));
    micron::art<clash, int> b(micron::move(a));
    sb::require(b.size() == 100ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find(clash(50)) == 50);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
