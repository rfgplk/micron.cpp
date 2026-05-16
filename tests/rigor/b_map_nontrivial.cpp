//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/b_map.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"
#include "../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

#include <cstdio>

struct heavy_v {
  micron::svector<int> data{};
  int tag;
  heavy_v() = default;

  explicit heavy_v(int t) : data{}, tag(t)
  {
    for ( int i = 0; i < 5; ++i ) data.push_back(t * 10 + i);
  }

  heavy_v(const heavy_v &o) : data(o.data), tag(o.tag) { }

  heavy_v(heavy_v &&o) noexcept : data(micron::move(o.data)), tag(o.tag) { o.tag = -1; }

  heavy_v &
  operator=(const heavy_v &o)
  {
    data = o.data;
    tag = o.tag;
    return *this;
  }

  heavy_v &
  operator=(heavy_v &&o) noexcept
  {
    data = micron::move(o.data);
    tag = o.tag;
    o.tag = -1;
    return *this;
  }

  bool
  is_intact() const noexcept
  {
    if ( tag < 0 ) return false;
    if ( data.size() != 5u ) return false;
    for ( int i = 0; i < 5; ++i )
      if ( data[static_cast<usize>(i)] != tag * 10 + i ) return false;
    return true;
  }
};

int
main(void)
{
  sb::print("=== BTREE MAP NON-TRIVIAL V TESTS ===");

  sb::test_case("non-trivial V: insert/find round-trip, intact across splits");
  {
    micron::btree_map<int, heavy_v> m;
    constexpr int N = 200;
    for ( int i = 0; i < N; ++i ) {
      m.insert(i, heavy_v(i));
    }
    for ( int i = 0; i < N; ++i ) {
      heavy_v *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(p->is_intact());
      sb::require(p->tag == i);
    }
    sb::require(m.size() == static_cast<usize>(N));
  }
  sb::end_test_case();

  sb::test_case("non-trivial V: erase preserves remaining values");
  {
    micron::btree_map<int, heavy_v> m;
    constexpr int N = 150;
    for ( int i = 0; i < N; ++i ) m.insert(i, heavy_v(i));
    for ( int i = 0; i < N; i += 2 ) (void)m.erase(i);
    for ( int i = 0; i < N; ++i ) {
      heavy_v *p = m.find(i);
      if ( i % 2 == 0 ) {
        sb::require(p == nullptr);
      } else {
        sb::require(p != nullptr);
        sb::require(p->is_intact());
        sb::require(p->tag == i);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("non-trivial V: overwrite via re-insert");
  {
    micron::btree_map<int, heavy_v> m;
    for ( int i = 0; i < 40; ++i ) m.insert(i, heavy_v(i));
    for ( int i = 0; i < 40; ++i ) m.insert(i, heavy_v(i + 1000));
    for ( int i = 0; i < 40; ++i ) {
      heavy_v *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(p->tag == i + 1000);
      sb::require(p->is_intact());
    }
  }
  sb::end_test_case();

  sb::test_case("non-trivial V: rehash preserves all values");
  {
    micron::btree_map<int, heavy_v> m(16);
    constexpr int N = 400;
    for ( int i = 0; i < N; ++i ) m.insert(i, heavy_v(i));
    for ( int i = 0; i < N; ++i ) {
      heavy_v *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(p->is_intact());
      sb::require(p->tag == i);
    }
  }
  sb::end_test_case();

  sb::test_case("non-trivial V: erase-then-reinsert through buffered ops");
  {
    micron::btree_map<int, heavy_v> m;
    constexpr int N = 80;
    for ( int i = 0; i < N; ++i ) m.insert(i, heavy_v(i));
    for ( int i = 0; i < N; ++i ) (void)m.erase(i);
    for ( int i = 0; i < N; ++i ) sb::require(m.find(i) == nullptr);
    for ( int i = 0; i < N; ++i ) m.insert(i, heavy_v(i * 2));
    for ( int i = 0; i < N; ++i ) {
      heavy_v *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(p->tag == i * 2);
      sb::require(p->is_intact());
    }
  }
  sb::end_test_case();

  sb::test_case("non-trivial V/K: hstring key + heavy_v value");
  {
    micron::btree_map<micron::hstring<char>, heavy_v> m;
    constexpr int N = 60;
    char buf[32];
    for ( int i = 0; i < N; ++i ) {
      std::snprintf(buf, sizeof(buf), "k_%04d", i);
      m.insert(micron::hstring<char>(buf), heavy_v(i));
    }
    for ( int i = 0; i < N; ++i ) {
      std::snprintf(buf, sizeof(buf), "k_%04d", i);
      heavy_v *p = m.find(micron::hstring<char>(buf));
      sb::require(p != nullptr);
      sb::require(p->tag == i);
      sb::require(p->is_intact());
    }
  }
  sb::end_test_case();

  sb::test_case("non-trivial V: no default constructor required for erase");
  {

    micron::btree_map<int, heavy_v> m;
    for ( int i = 0; i < 20; ++i ) m.insert(i, heavy_v(i));
    sb::require(m.erase(7));
    sb::require(!m.erase(7));
    sb::require(m.find(7) == nullptr);
    sb::require(m.find(8) != nullptr && m.find(8)->is_intact());
  }
  sb::end_test_case();

  sb::print("=== ALL BTREE MAP NON-TRIVIAL V TESTS PASSED ===");
  return 1;
}
