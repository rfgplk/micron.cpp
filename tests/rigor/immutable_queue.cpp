//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../snowball/snowball_fuzz.hpp"

#include "../../src/queue/iqueue.hpp"

namespace sbf = snowball::fuzzing;

namespace
{

struct oracle {
  static constexpr usize capacity = 512;
  int values[capacity];
  usize length = 0;

  void
  push(int value)
  {
    if ( length == capacity ) throw "immutable oracle overflow";
    values[length++] = value;
  }

  void
  pop()
  {
    if ( length == 0 ) return;
    for ( usize i = 1; i < length; ++i ) values[i - 1] = values[i];
    --length;
  }
};

inline u64
next(u64 &state) noexcept
{
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

void
validate(const micron::immutable_queue<int> &queue, const oracle &want)
{
  if ( queue.size() != want.length ) throw "immutable size disagrees with oracle";
  if ( queue.empty() != (want.length == 0) ) throw "immutable empty disagrees with oracle";
  if ( want.length == 0 ) {
    if ( queue.peek() != nullptr ) throw "immutable empty peek is non-null";
    if ( queue.begin() != queue.end() ) throw "immutable empty iterator range";
    return;
  }

  if ( queue.front() != want.values[0] ) throw "immutable front disagrees with oracle";
  if ( queue.last() != want.values[want.length - 1] ) throw "immutable last disagrees with oracle";
  for ( usize i = 0; i < want.length; ++i )
    if ( queue.at(i) != want.values[i] ) throw "immutable at disagrees with oracle";

  usize seen = 0;
  queue.for_each([&](const int &value) {
    if ( seen >= want.length || value != want.values[seen] ) throw "immutable for_each order";
    ++seen;
  });
  if ( seen != want.length ) throw "immutable for_each count";

  seen = 0;
  for ( auto it = queue.begin(), copy = it; it != queue.end(); ++it, ++copy ) {
    if ( *it != want.values[seen] || *copy != want.values[seen] ) throw "immutable iterator order/copy";
    ++seen;
  }
  if ( seen != want.length ) throw "immutable iterator count";
}

struct alignas(128) over_aligned_value {
  int value;

  explicit over_aligned_value(int v = 0) : value(v) { }

  bool
  operator==(const over_aligned_value &o) const noexcept
  {
    return value == o.value;
  }

  bool
  operator!=(const over_aligned_value &o) const noexcept
  {
    return value != o.value;
  }
};

}      // namespace

int
main()
{
  sbf::check_property(
      "immutable queue persistent state-machine fuzz",
      [](u64 seed) {
        if ( seed == 0 ) seed = 0x9e3779b97f4a7c15ULL;
        micron::immutable_queue<int> versions[64];
        oracle wants[64];
        usize made = 1;

        for ( usize step = 0; step < 1200; ++step ) {
          const usize source = static_cast<usize>(next(seed) % made);
          const usize target = made < 64 ? made++ : static_cast<usize>(next(seed) % 64);
          versions[target] = versions[source];
          wants[target] = wants[source];

          switch ( next(seed) % 7 ) {
          case 0:
          case 1:
          case 2:
            if ( wants[target].length < oracle::capacity ) {
              const int value = static_cast<int>(next(seed));
              versions[target] = versions[target].push(value);
              wants[target].push(value);
            }
            break;
          case 3:
          case 4:
            versions[target] = versions[target].pop();
            wants[target].pop();
            break;
          case 5:
            if ( wants[target].length ) {
              versions[target] = versions[target].update_front([](int value) { return value ^ 0x55aa55aa; });
              wants[target].values[0] ^= 0x55aa55aa;
            }
            break;
          default:
            if ( (next(seed) & 31u) == 0 ) {
              versions[target] = versions[target].clear();
              wants[target].length = 0;
            }
            break;
          }

          validate(versions[target], wants[target]);
          if ( (step & 31u) == 0 ) {
            for ( usize i = 0; i < made; ++i ) validate(versions[i], wants[i]);
          }
        }
      },
      { .seed = 0x1A2B3C4D5E6F7788ULL, .count = 128 }, sbf::spec<u64>{});

  sbf::check_property(
      "immutable queue rotation boundaries and long rear",
      [](int extra) {
        const usize count = 129 + static_cast<usize>(extra & 127);
        micron::immutable_queue<int> q;
        for ( usize i = 0; i < count; ++i ) {
          q = q.push(static_cast<int>(i));
          oracle want;
          for ( usize j = 0; j <= i; ++j ) want.push(static_cast<int>(j));
          validate(q, want);
        }
        for ( usize removed = 0; removed < count; ++removed ) {
          if ( q.front() != static_cast<int>(removed) ) throw "immutable long rotation FIFO";
          q = q.pop();
        }
        if ( !q.empty() ) throw "immutable long rotation drain";
      },
      { .seed = 0xBADC0FFEE0DDF00DULL, .count = 8 }, sbf::range<int>(0, 127));

  snowball::test_case("immutable over-aligned node values");
  {
    micron::immutable_queue<over_aligned_value> q;
    q = q.emplace(1).emplace(2).emplace(3);
    if ( reinterpret_cast<uintptr_t>(q.peek()) % alignof(over_aligned_value) != 0 ) throw "immutable over-aligned address";
    if ( q.front().value != 1 || q.last().value != 3 ) throw "immutable over-aligned FIFO";
  }
  snowball::end_test_case();

  snowball::print("immutable_queue: all fixed-seed fuzz properties held");
  return 1;
}
