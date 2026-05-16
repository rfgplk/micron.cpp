//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/queue/disruptor.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <atomic>
#include <thread>
#include <vector>

int
main(void)
{
  sb::print("=== DISRUPTOR TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::disruptor<int, 16> q;
    sb::require(q.empty());
    sb::require(q.size() == 0ULL);
    sb::require(q.capacity() == 16ULL);
  }
  sb::end_test_case();

  sb::test_case("capacity rounded to pow2");
  {
    micron::disruptor<int, 10> q;
    sb::require(q.capacity() == 16ULL);
  }
  sb::end_test_case();

  sb::test_case("publish - one value, then consume");
  {
    micron::disruptor<int, 16> q;
    sb::require(q.publish(42));
    sb::require(q.size() == 1ULL);
    int v = 0;
    sb::require(q.consume(v));
    sb::require(v == 42);
    sb::require(q.empty());
  }
  sb::end_test_case();

  sb::test_case("publish full - returns false");
  {
    micron::disruptor<int, 4> q;
    sb::require(q.publish(1));
    sb::require(q.publish(2));
    sb::require(q.publish(3));
    sb::require(q.publish(4));
    sb::require(!q.publish(5));
  }
  sb::end_test_case();

  sb::test_case("FIFO order preserved");
  {
    micron::disruptor<int, 64> q;
    for ( int i = 0; i < 20; ++i ) sb::require(q.publish(i));
    for ( int i = 0; i < 20; ++i ) {
      int v = -1;
      sb::require(q.consume(v));
      sb::require(v == i);
    }
  }
  sb::end_test_case();

  sb::test_case("peek does not consume");
  {
    micron::disruptor<int, 16> q;
    q.publish(10);
    int v = 0;
    sb::require(q.peek(v));
    sb::require(v == 10);
    sb::require(q.size() == 1ULL);
    int v2 = 0;
    sb::require(q.consume(v2));
    sb::require(v2 == 10);
  }
  sb::end_test_case();

  sb::test_case("batched publish then consume");
  {
    micron::disruptor<int, 64> q;
    int data[20];
    for ( int i = 0; i < 20; ++i ) data[i] = i * 3;
    usize n = q.try_publish_batch(data, 20);
    sb::require(n == 20ULL);
    int out[20];
    usize m = q.try_consume_batch(out, 20);
    sb::require(m == 20ULL);
    for ( int i = 0; i < 20; ++i ) sb::require(out[i] == i * 3);
  }
  sb::end_test_case();

  sb::test_case("wrap-around behaves");
  {
    micron::disruptor<int, 4> q;
    for ( int round = 0; round < 100; ++round ) {
      sb::require(q.publish(round));
      int v = 0;
      sb::require(q.consume(v));
      sb::require(v == round);
    }
  }
  sb::end_test_case();

  sb::test_case("producer/consumer threads - 100k items");
  {
    micron::disruptor<int, 1024> q;
    constexpr int N = 100000;
    std::atomic<bool> done{ false };
    std::thread producer([&]() {
      for ( int i = 0; i < N; ) {
        if ( q.publish(i) ) {
          ++i;
        }
      }
      done.store(true);
    });
    std::thread consumer([&]() {
      int expected = 0;
      while ( expected < N ) {
        int v = 0;
        if ( q.consume(v) ) {
          sb::require(v == expected);
          ++expected;
        }
      }
    });
    producer.join();
    consumer.join();
    sb::require(q.empty());
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
