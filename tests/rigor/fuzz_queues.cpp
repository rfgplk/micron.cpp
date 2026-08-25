//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../snowball/snowball_fuzz.hpp"
#include "../support/queue_fuzz.hpp"

#include "../../src/queue/chase_lev.hpp"
#include "../../src/queue/conqueue.hpp"
#include "../../src/queue/crossbeam.hpp"
#include "../../src/queue/disruptor.hpp"
#include "../../src/queue/queue.hpp"
#include "../../src/queue/spsc_queue.hpp"
#include "../../src/queue/static_mpmc.hpp"

namespace sbf = snowball::fuzzing;
namespace qf = mtest::queue_fuzz;

namespace
{

template<typename Q, usize Max>
void
validate_dynamic(const Q &queue, const qf::fifo_oracle<int, Max> &want)
{
  if ( queue.size() != want.length ) throw "dynamic queue size";
  if ( queue.empty() != (want.length == 0) ) throw "dynamic queue empty";
  if ( want.length ) {
    if ( queue.last() != want.front() ) throw "dynamic queue oldest accessor";
    if ( queue.front() != want.back() ) throw "dynamic queue newest accessor";
  }
  usize i = 0;
  for ( auto it = queue.cbegin(); it != queue.cend(); ++it, ++i )
    if ( i >= want.length || *it != want.at(i) ) throw "dynamic queue iterator order";
  if ( i != want.length ) throw "dynamic queue iterator count";
}

void
fuzz_queue(u64 seed)
{
  qf::prng rng(seed);
  micron::queue<int, 4> queue;
  qf::fifo_oracle<int, 4096> want;
  for ( usize step = 0; step < 6000; ++step ) {
    switch ( rng.below(10) ) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4: {
      if ( want.length == 4096 ) break;
      const int value = static_cast<int>(rng.next());
      queue.push(value);
      want.push(value);
      break;
    }
    case 5:
    case 6: {
      queue.pop();
      int ignored = 0;
      want.pop(ignored);
      break;
    }
    case 7:
      queue.reserve(queue.size() + 1 + rng.below(64));
      break;
    case 8:
      if ( (rng.next() & 31u) == 0 ) {
        queue.clear();
        want.clear();
      }
      break;
    default: {
      micron::queue<int, 4> moved(micron::move(queue));
      queue = micron::move(moved);
      break;
    }
    }
    if ( (step & 15u) == 0 ) validate_dynamic(queue, want);
  }
  validate_dynamic(queue, want);
}

void
fuzz_conqueue(u64 seed)
{
  qf::prng rng(seed);
  micron::conqueue<int, 4> queue;
  qf::fifo_oracle<int, 4096> want;
  int batch[32];
  int out[32];
  for ( usize step = 0; step < 5000; ++step ) {
    const usize op = rng.below(10);
    if ( op < 4 && want.length < 4096 ) {
      const int value = static_cast<int>(rng.next());
      queue.push(value);
      want.push(value);
    } else if ( op < 6 ) {
      int value = 0, expected = 0;
      const bool got = queue.pop(value);
      const bool had = want.pop(expected);
      if ( got != had || (got && value != expected) ) throw "conqueue pop(T&)";
    } else if ( op == 6 ) {
      const usize count = 1 + rng.below(32);
      for ( usize i = 0; i < count; ++i ) batch[i] = static_cast<int>(rng.next());
      if ( want.length + count <= 4096 ) {
        if ( queue.push_batch(batch, count) != count ) throw "conqueue push_batch count";
        for ( usize i = 0; i < count; ++i ) want.push(batch[i]);
      }
    } else if ( op == 7 ) {
      const usize count = rng.below(32);
      const usize expected_count = count < want.length ? count : want.length;
      const usize got = queue.pop_batch(out, count);
      if ( got != expected_count ) throw "conqueue pop_batch count";
      for ( usize i = 0; i < got; ++i ) {
        int expected = 0;
        want.pop(expected);
        if ( out[i] != expected ) throw "conqueue pop_batch order";
      }
    } else if ( op == 8 && (rng.next() & 31u) == 0 ) {
      queue.clear();
      want.clear();
    } else {
      micron::conqueue<int, 4> copy(queue);
      micron::conqueue<int, 4> assigned;
      assigned = copy;
      queue = micron::move(assigned);
    }

    if ( (step & 15u) == 0 ) {
      validate_dynamic(queue, want);
      usize i = 0;
      queue.for_each_locked([&](const int &value) {
        if ( i >= want.length || value != want.at(i) ) throw "conqueue locked traversal";
        ++i;
      });
      if ( i != want.length ) throw "conqueue locked traversal count";
    }
  }
  validate_dynamic(queue, want);
}

template<typename Queue, typename Push, typename Pop>
void
fuzz_bounded(u64 seed, Queue &queue, Push push, Pop pop)
{
  qf::prng rng(seed);
  qf::fifo_oracle<int, 32> want;
  for ( usize step = 0; step < 10000; ++step ) {
    if ( rng.below(100) < 57 ) {
      const int value = static_cast<int>(rng.next());
      const bool got = push(queue, value);
      const bool expected = want.push(value);
      if ( got != expected ) throw "bounded queue push/full";
    } else {
      int value = 0, expected_value = 0;
      const bool got = pop(queue, value);
      const bool expected = want.pop(expected_value);
      if ( got != expected || (got && value != expected_value) ) throw "bounded queue pop/FIFO";
    }
    if ( queue.size() != want.length || queue.size() > queue.capacity() ) throw "bounded queue sampled size";
    if ( queue.empty() != (want.length == 0) ) throw "bounded queue empty";
  }
}

template<typename Queue, typename PushBatch, typename PopBatch>
void
fuzz_batches(u64 seed, Queue &queue, PushBatch push_batch, PopBatch pop_batch)
{
  qf::prng rng(seed);
  qf::fifo_oracle<int, 32> want;
  int in[17];
  int out[17];
  for ( usize step = 0; step < 5000; ++step ) {
    const usize count = rng.below(18);
    if ( rng.next() & 1u ) {
      for ( usize i = 0; i < count; ++i ) in[i] = static_cast<int>(rng.next());
      const usize room = 32 - want.length;
      const usize expected = count < room ? count : room;
      const usize got = push_batch(queue, in, count);
      if ( got != expected ) throw "bounded batch push count";
      for ( usize i = 0; i < got; ++i ) want.push(in[i]);
    } else {
      const usize expected = count < want.length ? count : want.length;
      const usize got = pop_batch(queue, out, count);
      if ( got != expected ) throw "bounded batch pop count";
      for ( usize i = 0; i < got; ++i ) {
        int value = 0;
        want.pop(value);
        if ( out[i] != value ) throw "bounded batch FIFO";
      }
    }
    if ( queue.size() != want.length ) throw "bounded batch size";
  }
}

void
fuzz_chase_lev(u64 seed)
{
  qf::prng rng(seed);
  micron::chase_lev<u64, 16> fixed;
  micron::chase_lev_grow<u64, 2> grow;
  qf::fifo_oracle<u64, 4096> fixed_want;
  qf::fifo_oracle<u64, 4096> grow_want;

  auto run_one = [&](auto &deque, auto &want, bool bounded) {
    for ( usize step = 0; step < 8000; ++step ) {
      const usize op = rng.below(3);
      if ( op == 0 && (!bounded || want.length < 16) ) {
        const u64 value = rng.next() & 31u;      // includes the sentinel value zero
        const bool got = deque.push_bottom(value);
        if ( got ) want.push(value);
        if ( !got && (!bounded || want.length != 16) ) throw "chase push failure";
      } else if ( op == 1 ) {
        u64 got = 0;
        const bool present = deque.pop_bottom(got);
        if ( present != (want.length != 0) ) throw "chase bool pop state";
        if ( present ) {
          const u64 expected = want.back();
          --want.length;
          if ( got != expected ) throw "chase owner LIFO";
        }
      } else {
        const auto result = deque.try_steal();
        if ( want.length == 0 ) {
          if ( result.__st != micron::steal_status::empty ) throw "chase empty steal status";
        } else {
          if ( result.__st != micron::steal_status::got || result.__v != want.front() ) throw "chase thief FIFO";
          u64 ignored = 0;
          want.pop(ignored);
        }
      }
      if ( deque.size() != want.length ) throw "chase sampled size";
    }
  };

  run_one(fixed, fixed_want, true);
  run_one(grow, grow_want, false);
}

}      // namespace

int
main()
{
  sbf::check_property("queue dynamic FIFO state fuzz", fuzz_queue, { .seed = 0xA11CE001ULL, .count = 64 }, sbf::spec<u64>{});
  sbf::check_property("conqueue dynamic/bulk/copy state fuzz", fuzz_conqueue, { .seed = 0xA11CE002ULL, .count = 64 }, sbf::spec<u64>{});

  snowball::test_case("conqueue batch compacts into reclaimed head space");
  {
    micron::conqueue<int, 8> queue;
    int first[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    int second[4] = { 8, 9, 10, 11 };
    int out[12]{};
    queue.push_batch(first, 8);
    if ( queue.pop_batch(out, 4) != 4 ) throw "conqueue batch compaction setup";
    const usize capacity = queue.max_size();
    if ( queue.push_batch(second, 4) != 4 || queue.max_size() != capacity ) throw "conqueue batch grew despite reclaimed space";
    if ( queue.pop_batch(out, 8) != 8 ) throw "conqueue batch compaction drain";
    for ( int i = 0; i < 8; ++i )
      if ( out[i] != i + 4 ) throw "conqueue batch compaction FIFO";
  }
  snowball::end_test_case();

  sbf::check_property(
      "spsc bounded FIFO state and wrap fuzz",
      [](u64 seed) {
        micron::spsc_queue<int, 32> queue;
        fuzz_bounded(seed, queue, [](auto &q, int v) { return q.push(v); }, [](auto &q, int &v) { return q.pop(v); });
        queue.clear();
        fuzz_batches(
            seed ^ 0x1111u, queue, [](auto &q, const int *p, usize n) { return q.push_batch(p, n); },
            [](auto &q, int *p, usize n) { return q.pop_batch(p, n); });
      },
      { .seed = 0xA11CE003ULL, .count = 64 }, sbf::spec<u64>{});

  sbf::check_property(
      "disruptor bounded FIFO state and wrap fuzz",
      [](u64 seed) {
        micron::disruptor<int, 32> queue;
        fuzz_bounded(seed, queue, [](auto &q, int v) { return q.publish(v); }, [](auto &q, int &v) { return q.consume(v); });
        queue.clear();
        fuzz_batches(
            seed ^ 0x2222u, queue, [](auto &q, const int *p, usize n) { return q.try_publish_batch(p, n); },
            [](auto &q, int *p, usize n) { return q.try_consume_batch(p, n); });
      },
      { .seed = 0xA11CE004ULL, .count = 64 }, sbf::spec<u64>{});

  sbf::check_property(
      "crossbeam bounded FIFO state fuzz",
      [](u64 seed) {
        micron::crossbeam<int, 32> queue;
        fuzz_bounded(seed, queue, [](auto &q, int v) { return q.push(v); }, [](auto &q, int &v) { return q.pop(v); });
      },
      { .seed = 0xA11CE005ULL, .count = 64 }, sbf::spec<u64>{});

  sbf::check_property(
      "static_mpmc bounded FIFO state fuzz",
      [](u64 seed) {
        micron::static_mpmc<int, 32> queue;
        fuzz_bounded(seed, queue, [](auto &q, int v) { return q.push(v); }, [](auto &q, int &v) { return q.pop(v); });
      },
      { .seed = 0xA11CE006ULL, .count = 64 }, sbf::spec<u64>{});

  sbf::check_property("both Chase-Lev variants owner/thief fuzz", fuzz_chase_lev, { .seed = 0xA11CE007ULL, .count = 64 }, sbf::spec<u64>{});

  snowball::print("queue fuzz: all fixed-seed state properties held");
  return 1;
}
