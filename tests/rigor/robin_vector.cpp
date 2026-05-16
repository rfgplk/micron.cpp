//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// robin_vector.cpp — nested-container tests exercising every interesting
// combination of micron::robin_map and micron::vector:
//   * robin_map<K, vector<V>>        — "rmv" (map-of-vectors)
//   * vector<robin_map<K, V>>        — "vrm" (vector-of-maps)
//   * vector<robin_map<K, vector<V>>> — deep nest
//
// Each section drives construction, mutation, erase, lifetime, growth-driven
// moves, and stress vs std::unordered_map<int, std::vector<int>> ground truth.

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/maps/robin.hpp"
#include "../../src/std.hpp"
#include "../../src/string/string.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

namespace
{

struct Tracked {
  static inline int ctor = 0;
  static inline int dtor = 0;
  int v = 0;

  Tracked() : v(0) { ++ctor; }

  Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++ctor;
  }

  ~Tracked() { ++dtor; }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    return *this;
  }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }
};

inline void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

};      // namespace

static void
__diag_terminate()
{
  micron::io::print("DIAG: terminate during test_case = [", sb::__global_test_case, "]\n\r");
  __builtin_trap();
}

static micron::vector<int>
mkvec_int(std::initializer_list<int> init)
{
  micron::vector<int> v;
  for ( int x : init ) v.push_back(x);
  return v;
}

int
main(void)
{
  std::set_terminate(__diag_terminate);
  sb::print("=== ROBIN-VECTOR NESTED EXHAUSTIVE TESTS ===");

  sb::test_case("rmv - insert vector + find returns matching content");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.insert(1, mkvec_int({ 10, 20, 30 }));
    auto *v = m.find(1);
    sb::require(v != nullptr);
    sb::require(v->size() == 3u);
    sb::require((*v)[0] == 10);
    sb::require((*v)[1] == 20);
    sb::require((*v)[2] == 30);
  }
  sb::end_test_case();

  sb::test_case("rmv - find() returns mutable ref; mutation persists in map");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.insert(7, mkvec_int({ 1, 2, 3 }));
    auto *v = m.find(7);
    sb::require(v != nullptr);
    v->push_back(99);
    sb::require(m.find(7)->size() == 4u);
    sb::require((*m.find(7))[3] == 99);
  }
  sb::end_test_case();

  sb::test_case("rmv - re-insert with same key replaces vector value");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.insert(5, mkvec_int({ 1, 2, 3, 4 }));
    sb::require(m.find(5)->size() == 4u);
    m.insert(5, mkvec_int({ 100 }));
    sb::require(m.find(5)->size() == 1u);
    sb::require((*m.find(5))[0] == 100);
    sb::require(m.size() == 1u);
  }
  sb::end_test_case();

  sb::test_case("rmv - operator[] default-constructs an empty vector for missing key");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    auto &v = m[42];
    sb::require(v.empty());
    sb::require(m.size() == 1u);
    v.push_back(5);
    v.push_back(6);
    sb::require(m.find(42)->size() == 2u);
  }
  sb::end_test_case();

  sb::test_case("rmv - emplace constructs vector in place from initializer_list");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.emplace(3, std::initializer_list<int>{ 7, 8, 9 });
    auto *v = m.find(3);
    sb::require(v != nullptr);
    sb::require(v->size() == 3u);
    sb::require((*v)[2] == 9);
  }
  sb::end_test_case();

  sb::test_case("rmv - erase removes the vector entirely");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.insert(1, mkvec_int({ 1, 2 }));
    m.insert(2, mkvec_int({ 3, 4 }));
    sb::require(m.size() == 2u);
    sb::require(m.erase(1));
    sb::require(m.size() == 1u);
    sb::require(m.find(1) == nullptr);
    sb::require(m.find(2) != nullptr);
  }
  sb::end_test_case();

  sb::test_case("rmv - clear destroys every contained vector");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    for ( int i = 0; i < 20; ++i ) m.insert(i, mkvec_int({ i, i + 1, i + 2 }));
    sb::require(m.size() == 20u);
    m.clear();
    sb::require(m.empty());
    for ( int i = 0; i < 20; ++i ) sb::require(m.find(i) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("rmv - for_each visits every (key, vector) pair");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, mkvec_int({ i * 10, i * 20 }));
    std::set<int> seen_keys;
    long long total_value = 0;
    m.for_each([&](auto &node) {
      seen_keys.insert(node.key);
      for ( size_t k = 0; k < node.value.size(); ++k ) total_value += node.value[k];
    });
    sb::require(seen_keys.size() == 10u);
    long long expected = 0;
    for ( int i = 0; i < 10; ++i ) expected += i * 10 + i * 20;
    sb::require(total_value == expected);
  }
  sb::end_test_case();

  sb::test_case("rmv - vector at key grows with push_back across calls");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.insert(1, mkvec_int({}));
    auto *v = m.find(1);
    for ( int i = 0; i < 200; ++i ) v->push_back(i);
    sb::require(m.find(1)->size() == 200u);
    for ( int i = 0; i < 200; ++i ) sb::require((*m.find(1))[i] == i);
  }
  sb::end_test_case();

  sb::test_case("rmv - vector at key shrinks with pop_back");
  {
    micron::robin_map<int, micron::vector<int>> m(64);
    m.insert(1, mkvec_int({ 1, 2, 3, 4, 5 }));
    auto *v = m.find(1);
    v->pop_back();
    v->pop_back();
    sb::require(m.find(1)->size() == 3u);
  }
  sb::end_test_case();

  sb::test_case("rmv - move ctor transfers all vectors, source becomes empty");
  {
    micron::robin_map<int, micron::vector<int>> a(64);
    for ( int i = 0; i < 12; ++i ) a.insert(i, mkvec_int({ i, i, i }));
    sb::require(a.size() == 12u);
    micron::robin_map<int, micron::vector<int>> b(micron::move(a));
    sb::require(b.size() == 12u);
    sb::require(a.size() == 0u);
    for ( int i = 0; i < 12; ++i ) {
      auto *v = b.find(i);
      sb::require(v != nullptr);
      sb::require(v->size() == 3u);
      sb::require((*v)[0] == i);
    }
  }
  sb::end_test_case();

  sb::test_case("rmv - lifetime: ctor/dtor balanced when value vector holds Tracked");
  {
    reset_tracked();
    {
      micron::robin_map<int, micron::vector<Tracked>> m(64);
      for ( int i = 0; i < 15; ++i ) {
        micron::vector<Tracked> v;
        for ( int j = 0; j < 4; ++j ) v.emplace_back(i * 10 + j);
        m.insert(i, micron::move(v));
      }
      sb::require(m.size() == 15u);

      m.find(3)->push_back(Tracked(999));
      sb::require(m.find(3)->size() == 5u);
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("rmv - erase mid-chain triggers backward_shift without leaking vectors");
  {
    reset_tracked();
    {
      micron::robin_map<int, micron::vector<Tracked>> m(64);
      for ( int i = 0; i < 30; ++i ) {
        micron::vector<Tracked> v;
        v.emplace_back(i);
        m.insert(i, micron::move(v));
      }
      for ( int i = 0; i < 30; i += 2 ) sb::require(m.erase(i));
      sb::require(m.size() == 15u);
      for ( int i = 1; i < 30; i += 2 ) {
        auto *v = m.find(i);
        sb::require(v != nullptr);
        sb::require(v->size() == 1u);
        sb::require((*v)[0].v == i);
      }
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("rmv - many keys, each holding a large vector");
  {
    const int K = 200;
    const int VLEN = 50;
    micron::robin_map<int, micron::vector<int>> m(1024);
    for ( int i = 0; i < K; ++i ) {
      micron::vector<int> v;
      for ( int j = 0; j < VLEN; ++j ) v.push_back(i * 1000 + j);
      m.insert(i, micron::move(v));
    }
    sb::require(m.size() == (size_t)K);
    for ( int i = 0; i < K; ++i ) {
      auto *v = m.find(i);
      sb::require(v != nullptr);
      sb::require(v->size() == (size_t)VLEN);
      sb::require((*v)[0] == i * 1000);
      sb::require((*v)[VLEN - 1] == i * 1000 + VLEN - 1);
    }
  }
  sb::end_test_case();

  sb::test_case("rmv - operator[] accumulator: index vectors built up over inserts");
  {
    micron::robin_map<int, micron::vector<int>> buckets(64);
    int data[20] = { 1, 2, 1, 3, 2, 1, 4, 3, 2, 1, 5, 4, 3, 2, 1, 6, 5, 4, 3, 2 };
    for ( int i = 0; i < 20; ++i ) buckets[data[i]].push_back(i);
    sb::require(buckets.find(1)->size() == 5u);
    sb::require(buckets.find(2)->size() == 5u);
    sb::require(buckets.find(3)->size() == 4u);
    sb::require(buckets.find(4)->size() == 3u);
    sb::require(buckets.find(5)->size() == 2u);
    sb::require(buckets.find(6)->size() == 1u);
  }
  sb::end_test_case();

  sb::test_case("vrm - emplace_back constructs a map in place");
  {
    micron::vector<micron::robin_map<int, int>> vec;
    vec.emplace_back(64);
    vec.back().insert(1, 100);
    vec.back().insert(2, 200);
    sb::require(vec.size() == 1u);
    sb::require(vec.back().size() == 2u);
    sb::require(*vec.back().find(1) == 100);
  }
  sb::end_test_case();

  sb::test_case("vrm - push_back several maps, each retains contents");
  {
    micron::vector<micron::robin_map<int, int>> vec;
    for ( int i = 0; i < 5; ++i ) {
      micron::robin_map<int, int> m(64);
      for ( int j = 0; j < 10; ++j ) m.insert(j, i * 100 + j);
      vec.emplace_back(micron::move(m));
    }
    sb::require(vec.size() == 5u);
    for ( int i = 0; i < 5; ++i )
      for ( int j = 0; j < 10; ++j ) sb::require(*vec[i].find(j) == i * 100 + j);
  }
  sb::end_test_case();

  sb::test_case("vrm - emplace_back growth path: vector relocates maps without losing keys");
  {
    micron::vector<micron::robin_map<int, int>> vec;
    const int N = 50;
    for ( int i = 0; i < N; ++i ) {
      micron::robin_map<int, int> m(64);
      m.insert(i, i * 7);
      m.insert(i + 1000, (i + 1000) * 7);
      vec.emplace_back(micron::move(m));
    }
    sb::require(vec.size() == (size_t)N);
    for ( int i = 0; i < N; ++i ) {
      auto *v1 = vec[i].find(i);
      auto *v2 = vec[i].find(i + 1000);
      sb::require(v1 != nullptr);
      sb::require(*v1 == i * 7);
      sb::require(v2 != nullptr);
      sb::require(*v2 == (i + 1000) * 7);
    }
  }
  sb::end_test_case();

  sb::test_case("vrm - modify map through vec[i]");
  {
    micron::vector<micron::robin_map<int, int>> vec;
    for ( int i = 0; i < 3; ++i ) {
      micron::robin_map<int, int> m(64);
      vec.emplace_back(micron::move(m));
    }
    vec[1].insert(5, 555);
    sb::require(*vec[1].find(5) == 555);
    sb::require(vec[0].find(5) == nullptr);
    sb::require(vec[2].find(5) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("vrm - pop_back destroys the last map");
  {
    reset_tracked();
    {
      micron::vector<micron::robin_map<int, Tracked>> vec;
      micron::robin_map<int, Tracked> m(64);
      m.insert(1, Tracked(10));
      m.insert(2, Tracked(20));
      vec.emplace_back(micron::move(m));
      sb::require(vec.size() == 1u);
      vec.pop_back();
      sb::require(vec.empty());
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("vrm - clear destroys every map");
  {
    reset_tracked();
    {
      micron::vector<micron::robin_map<int, Tracked>> vec;
      for ( int i = 0; i < 10; ++i ) {
        micron::robin_map<int, Tracked> m(64);
        for ( int j = 0; j < 3; ++j ) m.insert(j, Tracked(i * 100 + j));
        vec.emplace_back(micron::move(m));
      }
      sb::require(vec.size() == 10u);
      vec.clear();
      sb::require(vec.empty());
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("vrm - move ctor of vector preserves map contents in target");
  {
    micron::vector<micron::robin_map<int, int>> a;
    for ( int i = 0; i < 4; ++i ) {
      micron::robin_map<int, int> m(64);
      m.insert(0, i + 1);
      a.emplace_back(micron::move(m));
    }
    micron::vector<micron::robin_map<int, int>> b(micron::move(a));
    sb::require(b.size() == 4u);
    sb::require(a.size() == 0u);
    for ( int i = 0; i < 4; ++i ) sb::require(*b[i].find(0) == i + 1);
  }
  sb::end_test_case();

  sb::test_case("vrm - swap two vectors-of-maps exchanges contents");
  {
    micron::vector<micron::robin_map<int, int>> a, b;
    {
      micron::robin_map<int, int> m(64);
      m.insert(1, 100);
      a.emplace_back(micron::move(m));
    }
    {
      micron::robin_map<int, int> m(64);
      m.insert(2, 200);
      m.insert(3, 300);
      b.emplace_back(micron::move(m));
    }
    a.swap(b);
    sb::require(a.size() == 1u);
    sb::require(b.size() == 1u);
    sb::require(*a[0].find(2) == 200);
    sb::require(*b[0].find(1) == 100);
  }
  sb::end_test_case();

  sb::test_case("vrm - iterate vector, query keys per map");
  {
    micron::vector<micron::robin_map<int, int>> vec;
    for ( int i = 0; i < 5; ++i ) {
      micron::robin_map<int, int> m(64);
      for ( int j = 0; j <= i; ++j ) m.insert(j, j * 11);
      vec.emplace_back(micron::move(m));
    }

    for ( size_t i = 0; i < vec.size(); ++i ) sb::require(vec[i].size() == i + 1);
  }
  sb::end_test_case();

  sb::test_case("vrm - heavy growth: 200 maps, each with 20 keys");
  {
    const int N = 200;
    micron::vector<micron::robin_map<int, int>> vec;
    for ( int i = 0; i < N; ++i ) {
      micron::robin_map<int, int> m(64);
      for ( int j = 0; j < 20; ++j ) m.insert(j, i * 100 + j);
      vec.emplace_back(micron::move(m));
    }
    sb::require(vec.size() == (size_t)N);

    for ( int i = 0; i < N; i += 13 ) {
      for ( int j = 0; j < 20; ++j ) {
        auto *v = vec[i].find(j);
        sb::require(v != nullptr);
        sb::require(*v == i * 100 + j);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("deep - build, mutate, and read three layers");
  {
    micron::vector<micron::robin_map<int, micron::vector<int>>> vec;
    for ( int i = 0; i < 3; ++i ) {
      micron::robin_map<int, micron::vector<int>> m(64);
      for ( int j = 0; j < 4; ++j ) m.insert(j, mkvec_int({ i * 10 + j, i * 10 + j + 1 }));
      vec.emplace_back(micron::move(m));
    }
    sb::require(vec.size() == 3u);
    for ( int i = 0; i < 3; ++i )
      for ( int j = 0; j < 4; ++j ) {
        auto *v = vec[i].find(j);
        sb::require(v != nullptr);
        sb::require(v->size() == 2u);
        sb::require((*v)[0] == i * 10 + j);
      }

    vec[1].find(2)->push_back(7777);
    sb::require(vec[1].find(2)->size() == 3u);
    sb::require((*vec[1].find(2))[2] == 7777);
  }
  sb::end_test_case();

  sb::test_case("deep - clear inner map clears all its vectors");
  {
    reset_tracked();
    {
      micron::vector<micron::robin_map<int, micron::vector<Tracked>>> vec;
      {
        micron::robin_map<int, micron::vector<Tracked>> m(64);
        for ( int j = 0; j < 5; ++j ) {
          micron::vector<Tracked> v;
          v.emplace_back(j);
          v.emplace_back(j + 100);
          m.insert(j, micron::move(v));
        }
        vec.emplace_back(micron::move(m));
      }
      sb::require(vec.size() == 1u);
      sb::require(vec[0].size() == 5u);
      vec[0].clear();
      sb::require(vec[0].empty());
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("deep - dropping outer vector destroys every level");
  {
    reset_tracked();
    {
      micron::vector<micron::robin_map<int, micron::vector<Tracked>>> vec;
      for ( int i = 0; i < 4; ++i ) {
        micron::robin_map<int, micron::vector<Tracked>> m(64);
        for ( int j = 0; j < 3; ++j ) {
          micron::vector<Tracked> v;
          for ( int k = 0; k < 2; ++k ) v.emplace_back(i * 100 + j * 10 + k);
          m.insert(j, micron::move(v));
        }
        vec.emplace_back(micron::move(m));
      }
      sb::require(vec.size() == 4u);
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("heavy - rmv vs unordered_map<int, std::vector<int>>: 10k mixed ops");
  {
    micron::robin_map<int, micron::vector<int>> m(2048);
    std::unordered_map<int, std::vector<int>> truth;
    std::mt19937 rng(0xBABAFEED);
    const int OPS = 10000;
    for ( int op = 0; op < OPS; ++op ) {
      int k = std::uniform_int_distribution<int>(0, 199)(rng);
      int choice = std::uniform_int_distribution<int>(0, 4)(rng);
      if ( choice == 0 ) {

        int n = std::uniform_int_distribution<int>(0, 5)(rng);
        micron::vector<int> v;
        std::vector<int> tv;
        for ( int i = 0; i < n; ++i ) {
          int val = std::uniform_int_distribution<int>(0, 1000)(rng);
          v.push_back(val);
          tv.push_back(val);
        }
        m.insert(k, micron::move(v));
        truth[k] = tv;
      } else if ( choice == 1 ) {

        int val = std::uniform_int_distribution<int>(0, 1000)(rng);
        m[k].push_back(val);
        truth[k].push_back(val);
      } else if ( choice == 2 ) {
        bool em = m.erase(k);
        auto et = truth.erase(k);
        sb::require(em == (et > 0));
      } else if ( choice == 3 ) {
        auto *mv = m.find(k);
        auto tit = truth.find(k);
        if ( tit == truth.end() ) {
          sb::require(mv == nullptr);
        } else {
          sb::require(mv != nullptr);
          sb::require(mv->size() == tit->second.size());
        }
      } else {
        sb::require(m.contains(k) == (truth.count(k) > 0));
      }
      sb::require(m.size() == truth.size());
    }

    for ( auto &kv : truth ) {
      auto *mv = m.find(kv.first);
      sb::require(mv != nullptr);
      sb::require(mv->size() == kv.second.size());
      for ( size_t i = 0; i < kv.second.size(); ++i ) sb::require((*mv)[i] == kv.second[i]);
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - vrm: 500 maps, then random per-map insert/find pattern");
  {
    const int N = 500;
    micron::vector<micron::robin_map<int, int>> vec;
    for ( int i = 0; i < N; ++i ) {
      micron::robin_map<int, int> m(64);
      vec.emplace_back(micron::move(m));
    }
    sb::require(vec.size() == (size_t)N);

    std::mt19937 rng(7);
    std::vector<std::vector<std::pair<int, int>>> truth(N);
    const int OPS = 5000;
    for ( int op = 0; op < OPS; ++op ) {
      int vi = std::uniform_int_distribution<int>(0, N - 1)(rng);
      int k = std::uniform_int_distribution<int>(0, 30)(rng);
      int v = std::uniform_int_distribution<int>(0, 100000)(rng);
      vec[vi].insert(k, v);

      bool found = false;
      for ( auto &kv : truth[vi] ) {
        if ( kv.first == k ) {
          kv.second = v;
          found = true;
          break;
        }
      }
      if ( !found ) truth[vi].emplace_back(k, v);
    }

    for ( int i = 0; i < N; ++i ) {
      sb::require(vec[i].size() == truth[i].size());
      for ( auto &kv : truth[i] ) {
        auto *p = vec[i].find(kv.first);
        sb::require(p != nullptr);
        sb::require(*p == kv.second);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - deep nest: 20 outer slots, 50 inner keys, 10-elem leaf vectors");
  {
    const int O = 20, K = 50, L = 10;
    micron::vector<micron::robin_map<int, micron::vector<int>>> vec;
    for ( int i = 0; i < O; ++i ) {
      micron::robin_map<int, micron::vector<int>> m(128);
      for ( int j = 0; j < K; ++j ) {
        micron::vector<int> leaf;
        for ( int k = 0; k < L; ++k ) leaf.push_back(i * 100'000 + j * 1'000 + k);
        m.insert(j, micron::move(leaf));
      }
      vec.emplace_back(micron::move(m));
    }
    sb::require(vec.size() == (size_t)O);

    for ( int i : { 0, 5, 13, 19 } ) {
      for ( int j : { 0, 25, 49 } ) {
        auto *v = vec[i].find(j);
        sb::require(v != nullptr);
        sb::require(v->size() == (size_t)L);
        sb::require((*v)[0] == i * 100'000 + j * 1'000);
        sb::require((*v)[L - 1] == i * 100'000 + j * 1'000 + L - 1);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - lifetime balance across deep nested stress");
  {
    reset_tracked();
    {
      micron::vector<micron::robin_map<int, micron::vector<Tracked>>> vec;
      const int O = 8, K = 12, L = 3;
      for ( int i = 0; i < O; ++i ) {
        micron::robin_map<int, micron::vector<Tracked>> m(64);
        for ( int j = 0; j < K; ++j ) {
          micron::vector<Tracked> v;
          for ( int k = 0; k < L; ++k ) v.emplace_back(i * 100 + j * 10 + k);
          m.insert(j, micron::move(v));
        }
        vec.emplace_back(micron::move(m));
      }

      for ( int i = 0; i < O; ++i )
        for ( int j = 0; j < K; j += 2 ) vec[i].find(j)->emplace_back(-1);

      for ( int i = 0; i < O; ++i ) vec[i].erase(3);

      for ( int i = 0; i < 2; ++i ) vec.pop_back();
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
