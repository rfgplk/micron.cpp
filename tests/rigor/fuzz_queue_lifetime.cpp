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
#include "../../src/queue/lambda_queue.hpp"
#include "../../src/queue/queue.hpp"
#include "../../src/queue/spsc_queue.hpp"
#include "../../src/queue/static_mpmc.hpp"

namespace sbf = snowball::fuzzing;
namespace qf = mtest::queue_fuzz;

namespace
{

template<usize N>
concept valid_spsc_capacity = requires { sizeof(micron::spsc_queue<int, N>); };
template<usize N>
concept valid_disruptor_capacity = requires { sizeof(micron::disruptor<int, N>); };
template<usize N>
concept valid_crossbeam_capacity = requires { sizeof(micron::crossbeam<int, N>); };
template<usize N>
concept valid_static_mpmc_capacity = requires { sizeof(micron::static_mpmc<int, N>); };
template<usize N>
concept valid_chase_capacity = requires { sizeof(micron::chase_lev<int, N>); };
template<usize N>
concept valid_lambda_capacity = requires { sizeof(micron::lambda_queue<N>); };

static_assert(!valid_spsc_capacity<0>);
static_assert(!valid_disruptor_capacity<0>);
static_assert(!valid_crossbeam_capacity<0>);
static_assert(!valid_static_mpmc_capacity<0>);
static_assert(!valid_static_mpmc_capacity<3>);
static_assert(!valid_chase_capacity<0>);
static_assert(!valid_lambda_capacity<0>);

template<typename Q, usize Max>
void
validate_owning(const Q &queue, const qf::fifo_oracle<int, Max> &want)
{
  if ( queue.size() != want.length ) throw "owning queue size";
  usize i = 0;
  for ( auto it = queue.cbegin(); it != queue.cend(); ++it, ++i ) {
    if ( i >= want.length || it->value() != want.at(i) ) throw "owning queue relocation/FIFO";
  }
  if ( i != want.length ) throw "owning queue iterator count";
}

void
fuzz_dynamic_owning(u64 seed)
{
  qf::owning_value::live = 0;
  {
    qf::prng rng(seed);
    micron::queue<qf::owning_value, 2> queue;
    qf::fifo_oracle<int, 1024> want;
    for ( usize step = 0; step < 5000; ++step ) {
      const usize op = rng.below(9);
      if ( op < 5 && want.length < 1024 ) {
        const int value = static_cast<int>(rng.next());
        queue.push(qf::owning_value(value));
        want.push(value);
      } else if ( op < 7 ) {
        queue.pop();
        int ignored = 0;
        want.pop(ignored);
      } else if ( op == 7 ) {
        queue.reserve(queue.size() + 1 + rng.below(31));
      } else {
        micron::queue<qf::owning_value, 2> moved(micron::move(queue));
        queue = micron::move(moved);
      }
      if ( (step & 7u) == 0 ) validate_owning(queue, want);
    }
    validate_owning(queue, want);
  }
  if ( qf::owning_value::live != 0 ) throw "queue owning lifetime imbalance";
}

void
fuzz_conqueue_owning(u64 seed)
{
  qf::owning_value::live = 0;
  {
    qf::prng rng(seed);
    micron::conqueue<qf::owning_value, 2> queue;
    qf::fifo_oracle<int, 1024> want;
    for ( usize step = 0; step < 4000; ++step ) {
      const usize op = rng.below(10);
      if ( op < 5 && want.length < 1024 ) {
        const int value = static_cast<int>(rng.next());
        queue.push(qf::owning_value(value));
        want.push(value);
      } else if ( op < 7 ) {
        qf::owning_value out(-1);
        int expected = 0;
        const bool got = queue.pop(out);
        const bool had = want.pop(expected);
        if ( got != had || (got && out.value() != expected) ) throw "conqueue owning pop";
      } else if ( op == 7 ) {
        queue.reserve(rng.below(8));      // includes reserve(smaller) after head advances
      } else if ( op == 8 ) {
        micron::conqueue<qf::owning_value, 2> copy(queue);
        queue = copy;
      } else {
        micron::conqueue<qf::owning_value, 2> moved(micron::move(queue));
        queue = micron::move(moved);
      }
      if ( (step & 7u) == 0 ) validate_owning(queue, want);
    }
    validate_owning(queue, want);
  }
  if ( qf::owning_value::live != 0 ) throw "conqueue owning lifetime imbalance";
}

template<typename Queue, typename Push, typename Pop>
void
exercise_deleted_move_assignment(Queue &queue, Push push, Pop pop)
{
  for ( int i = 0; i < 64; ++i ) {
    qf::no_move_assign value(i);
    while ( !push(queue, value) ) {
      qf::no_move_assign out(-1);
      if ( !pop(queue, out) || !out.valid() ) throw "deleted-move fallback pop";
    }
  }
  qf::no_move_assign out(-1);
  while ( pop(queue, out) ) {
    if ( !out.valid() ) throw "deleted-move assignment corrupted self pointer";
  }
}

template<typename Queue, typename Push, typename Pop>
void
exercise_capacity_one(Queue &queue, Push push, Pop pop)
{
  for ( int i = 0; i < 20000; ++i ) {
    if ( !push(queue, i) ) throw "capacity-one first push";
    if ( push(queue, i + 1) ) throw "capacity-one accepted second item";
    int out = -1;
    if ( !pop(queue, out) || out != i ) throw "capacity-one pop/reuse";
    if ( pop(queue, out) ) throw "capacity-one phantom item";
  }
}

}      // namespace

int
main()
{
  sbf::check_property("queue owning/self-referential relocation fuzz", fuzz_dynamic_owning, { .seed = 0xC001D00D00000001ULL, .count = 96 },
                      sbf::spec<u64>{});
  sbf::check_property("conqueue owning relocation/copy/reserve fuzz", fuzz_conqueue_owning, { .seed = 0xC001D00D00000002ULL, .count = 96 },
                      sbf::spec<u64>{});

  snowball::test_case("moved-from growable queues remain empty and reusable");
  {
    micron::queue<int, 2> source;
    source.push(1).push(2);
    micron::queue<int, 2> destination(micron::move(source));
    if ( !source.empty() || source.begin() != source.end() ) throw "queue moved-from traversal";
    source.reserve(8);
    source.push(3);
    if ( source.size() != 1 || source.last() != 3 ) throw "queue moved-from reuse";

    micron::conqueue<int, 2> locked_source;
    locked_source.push(4).push(5);
    micron::conqueue<int, 2> locked_destination(micron::move(locked_source));
    if ( !locked_source.empty() || locked_source.begin() != locked_source.end() ) throw "conqueue moved-from traversal";
    locked_source.reserve(8);
    locked_source.push(6);
    int out = -1;
    if ( !locked_source.pop(out) || out != 6 ) throw "conqueue moved-from reuse";

    if ( destination.size() != 2 || locked_destination.size() != 2 ) throw "move lost destination values";
  }
  snowball::end_test_case();

  snowball::test_case("deleted move assignment falls back to copy assignment");
  {
    qf::no_move_assign::live = 0;
    {
      micron::queue<qf::no_move_assign, 2> dynamic;
      for ( int i = 0; i < 100; ++i ) dynamic.push(qf::no_move_assign(i));
      for ( int i = 0; i < 60; ++i ) dynamic.pop();
      dynamic.reserve(256);
      for ( auto &v : dynamic )
        if ( !v.valid() ) throw "queue deleted-move relocation";

      micron::conqueue<qf::no_move_assign, 2> locked;
      for ( int i = 0; i < 100; ++i ) locked.push(qf::no_move_assign(i));
      qf::no_move_assign out(-1);
      for ( int i = 0; i < 60; ++i )
        if ( !locked.pop(out) || !out.valid() ) throw "conqueue deleted-move pop";
      locked.reserve(256);

      micron::spsc_queue<qf::no_move_assign, 8> spsc;
      exercise_deleted_move_assignment(spsc, [](auto &q, const auto &v) { return q.push(v); }, [](auto &q, auto &v) { return q.pop(v); });
      micron::disruptor<qf::no_move_assign, 8> disruptor;
      exercise_deleted_move_assignment(
          disruptor, [](auto &q, const auto &v) { return q.publish(v); }, [](auto &q, auto &v) { return q.consume(v); });
      micron::crossbeam<qf::no_move_assign, 8> crossbeam;
      exercise_deleted_move_assignment(
          crossbeam, [](auto &q, const auto &v) { return q.push(v); }, [](auto &q, auto &v) { return q.pop(v); });
    }
    if ( qf::no_move_assign::live != 0 ) throw "deleted-move lifetime imbalance";
  }
  snowball::end_test_case();

  snowball::test_case("bounded capacity-one wrap and slot reuse");
  {
    micron::spsc_queue<int, 1> spsc;
    exercise_capacity_one(spsc, [](auto &q, int v) { return q.push(v); }, [](auto &q, int &v) { return q.pop(v); });
    micron::disruptor<int, 1> disruptor;
    exercise_capacity_one(disruptor, [](auto &q, int v) { return q.publish(v); }, [](auto &q, int &v) { return q.consume(v); });
    micron::crossbeam<int, 1> crossbeam;
    exercise_capacity_one(crossbeam, [](auto &q, int v) { return q.push(v); }, [](auto &q, int &v) { return q.pop(v); });
    micron::static_mpmc<int, 1> mpmc;
    exercise_capacity_one(mpmc, [](auto &q, int v) { return q.push(v); }, [](auto &q, int &v) { return q.pop(v); });
    micron::chase_lev<int, 1> chase;
    exercise_capacity_one(chase, [](auto &q, int v) { return q.push_bottom(v); }, [](auto &q, int &v) { return q.pop_bottom(v); });
  }
  snowball::end_test_case();

  snowball::test_case("non-default and over-aligned destruction paths");
  {
    qf::non_default_value::live = 0;
    {
      micron::crossbeam<qf::non_default_value, 4> queue;
      queue.push(qf::non_default_value(1));
      queue.push(qf::non_default_value(2));
      queue.clear();
    }
    if ( qf::non_default_value::live != 0 ) throw "crossbeam clear requires/default-constructs or leaks T";

    micron::crossbeam<qf::over_aligned_value, 4> aligned;
    aligned.push(qf::over_aligned_value(7));
    qf::over_aligned_value out;
    if ( !aligned.pop(out) || out.value != 7 ) throw "crossbeam over-aligned value";
  }
  snowball::end_test_case();

  snowball::test_case("scoped enum payloads preserve all bits");
  {
    constexpr qf::scoped_value value = qf::scoped_value::high;
    qf::scoped_value out = qf::scoped_value::zero;
    micron::spsc_queue<qf::scoped_value, 2> spsc;
    if ( !spsc.push(value) || !spsc.pop(out) || out != value ) throw "spsc scoped enum";
    micron::disruptor<qf::scoped_value, 2> disruptor;
    if ( !disruptor.publish(value) || !disruptor.consume(out) || out != value ) throw "disruptor scoped enum";
    micron::crossbeam<qf::scoped_value, 2> crossbeam;
    if ( !crossbeam.push(value) || !crossbeam.pop(out) || out != value ) throw "crossbeam scoped enum";
    micron::static_mpmc<qf::scoped_value, 2> mpmc;
    if ( !mpmc.push(value) || !mpmc.pop(out) || out != value ) throw "static_mpmc scoped enum";
    micron::chase_lev<qf::scoped_value, 2> chase;
    if ( !chase.push_bottom(value) || !chase.pop_bottom(out) || out != value ) throw "chase scoped enum";
  }
  snowball::end_test_case();

  snowball::print("queue lifetime fuzz: all fixed-seed properties held");
  return 1;
}
