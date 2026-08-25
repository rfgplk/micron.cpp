//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../snowball/snowball.hpp"
#include "../support/tracked_types.hpp"

#include "../../src/queue/conqueue.hpp"
#include "../../src/queue/crossbeam.hpp"
#include "../../src/queue/disruptor.hpp"
#include "../../src/queue/iqueue.hpp"
#include "../../src/queue/queue.hpp"
#include "../../src/queue/spsc_queue.hpp"
#include "../../src/queue/static_mpmc.hpp"

#if !defined(__cpp_exceptions)
int
main()
{
  snowball::print("queue exception rigor: skipped without exception support");
  return 1;
}
#else

namespace
{

template<typename T>
void
require_balanced(const char *message)
{
  if ( T::ctor != T::dtor ) throw message;
}

template<typename Q>
void
require_values(const Q &queue, const int *values, usize count, const char *message)
{
  if ( queue.size() != count ) throw message;
  usize i = 0;
  for ( auto it = queue.cbegin(); it != queue.cend(); ++it, ++i ) {
    if ( i >= count || it->v != values[i] ) throw message;
  }
  if ( i != count ) throw message;
}

template<typename Queue, typename PushBatch, typename PopBatch>
void
exercise_batch_exceptions(Queue &queue, PushBatch push_batch, PopBatch pop_batch)
{
  using copy_t = mtest::Throwing<mtest::throw_on::copy_ctor, 30>;
  copy_t input[8];
  for ( int i = 0; i < 8; ++i ) input[i].v = 100 + i;

  copy_t::arm(3);
  bool caught = false;
  try {
    (void)push_batch(queue, input, 8);
  } catch ( ... ) {
    caught = true;
  }
  copy_t::disarm();
  if ( !caught || queue.size() != 0 ) throw "batch construction did not roll back unpublished prefix";

  if ( push_batch(queue, input, 8) != 8 ) throw "batch refill after construction exception";
  copy_t output[8];
  if ( pop_batch(queue, output, 8) != 8 ) throw "batch drain after construction exception";
  for ( int i = 0; i < 8; ++i )
    if ( output[i].v != 100 + i ) throw "batch FIFO after construction exception";
}

}      // namespace

int
main()
{
  snowball::test_case("queue relocation copy failure leaves source intact");
  {
    using value_t = mtest::Throwing<mtest::throw_on::copy_ctor, 1>;
    value_t::reset();
    {
      micron::queue<value_t, 2> queue;
      for ( int i = 0; i < 8; ++i ) queue.push(value_t(i));
      const int want[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
      value_t::arm(3);
      bool caught = false;
      try {
        queue.reserve(queue.max_size() + 8);
      } catch ( ... ) {
        caught = true;
      }
      value_t::disarm();
      if ( !caught ) throw "queue relocation did not inject copy failure";
      require_values(queue, want, 8, "queue changed after failed relocation");
      queue.reserve(queue.max_size() + 8);
      require_values(queue, want, 8, "queue failed relocation recovery");
    }
    require_balanced<value_t>("queue relocation leaked a constructed value");
  }
  snowball::end_test_case();

  snowball::test_case("conqueue relocation and copy assignment unwind");
  {
    using value_t = mtest::Throwing<mtest::throw_on::copy_ctor, 2>;
    value_t::reset();
    {
      micron::conqueue<value_t, 2> source;
      for ( int i = 0; i < 8; ++i ) source.push(value_t(i));
      micron::conqueue<value_t, 2> destination;
      destination.push(value_t(90));
      destination.push(value_t(91));
      const int source_want[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
      const int destination_want[] = { 90, 91 };

      value_t::arm(2);
      bool caught = false;
      try {
        source.reserve(source.max_size() + 8);
      } catch ( ... ) {
        caught = true;
      }
      value_t::disarm();
      if ( !caught ) throw "conqueue reserve did not inject copy failure";
      require_values(source, source_want, 8, "conqueue changed after failed reserve");

      value_t::arm(3);
      caught = false;
      try {
        destination = source;
      } catch ( ... ) {
        caught = true;
      }
      value_t::disarm();
      if ( !caught ) throw "conqueue assignment did not inject copy failure";
      require_values(source, source_want, 8, "conqueue assignment changed source");
      require_values(destination, destination_want, 2, "conqueue assignment changed destination on failure");
      destination = source;
      require_values(destination, source_want, 8, "conqueue assignment recovery");
    }
    require_balanced<value_t>("conqueue relocation/copy leaked a constructed value");
  }
  snowball::end_test_case();

  snowball::test_case("fill constructors unwind constructed prefixes");
  {
    using value_t = mtest::Throwing<mtest::throw_on::default_ctor, 3>;
    value_t::reset();
    value_t::arm(3);
    bool queue_caught = false;
    try {
      micron::queue<value_t, 2> queue(static_cast<umax_t>(8));
      (void)queue;
    } catch ( ... ) {
      queue_caught = true;
    }
    value_t::disarm();
    if ( !queue_caught ) throw "queue fill constructor did not throw";
    require_balanced<value_t>("queue fill constructor leaked prefix");

    value_t::reset();
    value_t::arm(3);
    bool conqueue_caught = false;
    try {
      micron::conqueue<value_t, 2> queue(static_cast<umax_t>(8));
      (void)queue;
    } catch ( ... ) {
      conqueue_caught = true;
    }
    value_t::disarm();
    if ( !conqueue_caught ) throw "conqueue fill constructor did not throw";
    require_balanced<value_t>("conqueue fill constructor leaked prefix");
  }
  snowball::end_test_case();

  snowball::test_case("initializer-list constructors unwind copied prefixes");
  {
    using queue_value_t = mtest::Throwing<mtest::throw_on::copy_ctor, 4>;
    queue_value_t::reset();
    queue_value_t::arm(2);
    bool queue_caught = false;
    try {
      micron::queue<queue_value_t, 2> queue{ queue_value_t(1), queue_value_t(2), queue_value_t(3), queue_value_t(4) };
      (void)queue;
    } catch ( ... ) {
      queue_caught = true;
    }
    queue_value_t::disarm();
    if ( !queue_caught ) throw "queue initializer-list did not throw";
    require_balanced<queue_value_t>("queue initializer-list leaked copied prefix");

    using conqueue_value_t = mtest::Throwing<mtest::throw_on::copy_ctor, 5>;
    conqueue_value_t::reset();
    conqueue_value_t::arm(2);
    bool conqueue_caught = false;
    try {
      micron::conqueue<conqueue_value_t, 2> queue{ conqueue_value_t(1), conqueue_value_t(2), conqueue_value_t(3), conqueue_value_t(4) };
      (void)queue;
    } catch ( ... ) {
      conqueue_caught = true;
    }
    conqueue_value_t::disarm();
    if ( !conqueue_caught ) throw "conqueue initializer-list did not throw";
    require_balanced<conqueue_value_t>("conqueue initializer-list leaked copied prefix");
  }
  snowball::end_test_case();

  snowball::test_case("SPSC and disruptor batch construction rollback");
  {
    using value_t = mtest::Throwing<mtest::throw_on::copy_ctor, 30>;
    value_t::reset();
    {
      micron::spsc_queue<value_t, 8> queue;
      exercise_batch_exceptions(
          queue, [](auto &q, const auto *p, usize n) { return q.push_batch(p, n); },
          [](auto &q, auto *p, usize n) { return q.pop_batch(p, n); });
    }
    require_balanced<value_t>("SPSC batch exception lifetime imbalance");

    value_t::reset();
    {
      micron::disruptor<value_t, 8> queue;
      exercise_batch_exceptions(
          queue, [](auto &q, const auto *p, usize n) { return q.try_publish_batch(p, n); },
          [](auto &q, auto *p, usize n) { return q.try_consume_batch(p, n); });
    }
    require_balanced<value_t>("disruptor batch exception lifetime imbalance");
  }
  snowball::end_test_case();

  snowball::test_case("throwing batch assignment publishes only consumed prefix");
  {
    using value_t = mtest::Throwing<mtest::throw_on::move_assign, 31>;
    value_t::reset();
    {
      value_t input[8];
      value_t output[8];
      for ( int i = 0; i < 8; ++i ) input[i].v = i;
      micron::spsc_queue<value_t, 8> queue;
      queue.push_batch(input, 8);
      value_t::arm(3);
      bool caught = false;
      try {
        (void)queue.pop_batch(output, 8);
      } catch ( ... ) {
        caught = true;
      }
      value_t::disarm();
      if ( !caught || queue.size() != 5 ) throw "SPSC throwing pop_batch cursor";
      if ( queue.pop_batch(output + 3, 5) != 5 ) throw "SPSC pop_batch recovery";
      for ( int i = 0; i < 8; ++i )
        if ( output[i].v != i ) throw "SPSC throwing pop_batch FIFO";
    }
    require_balanced<value_t>("SPSC throwing assignment lifetime imbalance");

    value_t::reset();
    {
      value_t input[8];
      value_t output[8];
      for ( int i = 0; i < 8; ++i ) input[i].v = i;
      micron::disruptor<value_t, 8> queue;
      queue.try_publish_batch(input, 8);
      value_t::arm(3);
      bool caught = false;
      try {
        (void)queue.try_consume_batch(output, 8);
      } catch ( ... ) {
        caught = true;
      }
      value_t::disarm();
      if ( !caught || queue.size() != 5 ) throw "disruptor throwing consume_batch cursor";
      if ( queue.try_consume_batch(output + 3, 5) != 5 ) throw "disruptor consume_batch recovery";
      for ( int i = 0; i < 8; ++i )
        if ( output[i].v != i ) throw "disruptor throwing consume_batch FIFO";
    }
    require_balanced<value_t>("disruptor throwing assignment lifetime imbalance");
  }
  snowball::end_test_case();

  snowball::test_case("crossbeam cancellation and throwing consumption make progress");
  {
    using construct_t = mtest::Throwing<mtest::throw_on::move_ctor, 40>;
    construct_t::reset();
    {
      micron::crossbeam<construct_t, 1> queue;
      construct_t input(7), output(0);
      construct_t::arm(0);
      bool caught = false;
      try {
        queue.push(micron::move(input));
      } catch ( ... ) {
        caught = true;
      }
      construct_t::disarm();
      if ( !caught ) throw "crossbeam construction exception not propagated";
      if ( queue.pop(output) ) throw "crossbeam cancellation became a value";
      if ( !queue.push(construct_t(8)) || !queue.pop(output) || output.v != 8 ) throw "crossbeam wedged after cancellation";
    }
    require_balanced<construct_t>("crossbeam cancellation lifetime imbalance");

    using assign_t = mtest::Throwing<mtest::throw_on::move_assign, 41>;
    assign_t::reset();
    {
      micron::crossbeam<assign_t, 1> queue;
      assign_t output(0);
      queue.push(assign_t(11));
      assign_t::arm(0);
      bool caught = false;
      try {
        queue.pop(output);
      } catch ( ... ) {
        caught = true;
      }
      assign_t::disarm();
      if ( !caught || !queue.empty() ) throw "crossbeam throwing pop did not release claimed slot";
      if ( !queue.push(assign_t(12)) || !queue.pop(output) || output.v != 12 ) throw "crossbeam wedged after throwing pop";
    }
    require_balanced<assign_t>("crossbeam throwing pop lifetime imbalance");
  }
  snowball::end_test_case();

  snowball::test_case("immutable rotation copy failure preserves persistent branch");
  {
    using value_t = mtest::Throwing<mtest::throw_on::copy_ctor, 50>;
    value_t::reset();
    {
      micron::immutable_queue<value_t> base;
      for ( int i = 0; i < 40; ++i ) base = base.push(value_t(i));
      for ( int trip = 0; trip < 12; ++trip ) {
        value_t extra(100 + trip);
        value_t::arm(trip);
        try {
          auto branch = base.push(extra);
          branch = branch.pop();
        } catch ( ... ) {
        }
        value_t::disarm();
        if ( base.size() != 40 ) throw "immutable exception changed persistent size";
        for ( usize i = 0; i < 40; ++i )
          if ( base.at(i).v != static_cast<int>(i) ) throw "immutable exception changed persistent values";
      }
    }
    require_balanced<value_t>("immutable rotation exception leaked a node value");
  }
  snowball::end_test_case();

  snowball::test_case("static_mpmc drain propagates callback exceptions");
  {
    micron::static_mpmc<int, 8> queue;
    for ( int i = 0; i < 4; ++i ) queue.push(i);
    bool caught = false;
    try {
      queue.drain([](int) { throw 77; });
    } catch ( int value ) {
      caught = value == 77;
    }
    if ( !caught || queue.size() != 3 ) throw "static_mpmc drain swallowed callback exception";
  }
  snowball::end_test_case();

  snowball::print("queue exception rigor: all injected failures recovered");
  return 1;
}

#endif
