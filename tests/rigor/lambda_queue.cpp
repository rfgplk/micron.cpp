//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../snowball/snowball_fuzz.hpp"

#include "../../src/queue/lambda_queue.hpp"
#include "../../src/thread/thread.hpp"

namespace sbf = snowball::fuzzing;

namespace
{

inline u64
next(u64 &state) noexcept
{
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

struct output_log {
  int values[4096];
  usize length = 0;

  void
  add(int value)
  {
    if ( length == 4096 ) throw "lambda output overflow";
    values[length++] = value;
  }
};

struct capture16 {
  output_log *out;
  int value;
  byte pad[16 - sizeof(output_log *) - sizeof(int)];

  void
  operator()()
  {
    out->add(value);
  }
};

struct capture64 {
  output_log *out;
  int value;
  byte pad[64 - sizeof(output_log *) - sizeof(int)];

  void
  operator()()
  {
    out->add(value);
  }
};

static_assert(sizeof(capture16) == 16);
static_assert(sizeof(capture64) == 64);

struct tracked_callable {
  static inline int live = 0;

  tracked_callable() { ++live; }

  tracked_callable(const tracked_callable &) { ++live; }

  tracked_callable(tracked_callable &&) noexcept { ++live; }

  ~tracked_callable() { --live; }

  void
  operator()()
  {
  }
};

#if defined(__cpp_exceptions)
struct throwing_callable {
  static inline bool throw_move = false;

  throwing_callable() = default;
  throwing_callable(const throwing_callable &) = delete;

  throwing_callable(throwing_callable &&)
  {
    if ( throw_move ) throw 7;
  }

  void
  operator()()
  {
    throw 9;
  }
};
#endif

}      // namespace

int
main()
{
  sbf::check_property(
      "lambda queue heterogeneous FIFO fuzz",
      [](u64 seed) {
        if ( seed == 0 ) seed = 0xD1B54A32D192ED03ULL;
        micron::lambda_queue<16> queue;
        output_log got;
        int pending[16];
        usize pending_count = 0;
        usize expected_at = 0;

        for ( usize step = 0; step < 3000; ++step ) {
          const u64 op = next(seed) % 8;
          if ( pending_count < 16 && op < 5 ) {
            const int value = static_cast<int>(next(seed));
            pending[pending_count++] = value;
            switch ( next(seed) % 3 ) {
            case 0:
              queue.push([&got, value] { got.add(value); });
              break;
            case 1:
              queue.push(capture16{ &got, value, {} });
              break;
            default:
              queue.push(capture64{ &got, value, {} });
              break;
            }
          } else if ( pending_count && op < 7 ) {
            if ( op & 1u ) {
              queue.execute();
            } else {
              auto *task = queue.pop();
              if ( !task ) throw "lambda pop missed ready task";
              task->call();
              task->~node_base_t();
            }
            if ( got.values[expected_at++] != pending[0] ) throw "lambda execution order";
            for ( usize i = 1; i < pending_count; ++i ) pending[i - 1] = pending[i];
            --pending_count;
          } else if ( op == 7 ) {
            queue.clear();
            pending_count = 0;
          }

          if ( queue.size() != pending_count ) throw "lambda sampled size";
          if ( queue.empty() != (pending_count == 0) ) throw "lambda empty state";
        }

        while ( pending_count ) {
          queue.execute();
          if ( got.values[expected_at++] != pending[0] ) throw "lambda final drain order";
          for ( usize i = 1; i < pending_count; ++i ) pending[i - 1] = pending[i];
          --pending_count;
        }
      },
      { .seed = 0xF00DFACE12345678ULL, .count = 128 }, sbf::spec<u64>{});

  snowball::test_case("lambda capacity-one slot remains claimed through destruction");
  {
    micron::lambda_queue<1> queue;
    micron::atomic_token<bool> producer_started{ false };
    micron::atomic_token<bool> producer_done{ false };
    int calls = 0;
    queue.push([&] { ++calls; });
    auto *first = queue.pop();
    if ( !first ) throw "lambda capacity-one initial pop";
    {
      micron::auto_thread<> producer([&] {
        producer_started.store(true, micron::memory_order_release);
        queue.push([&] { ++calls; });
        producer_done.store(true, micron::memory_order_release);
      });
      while ( !producer_started.get(micron::memory_order_acquire) ) ::__cpu_pause();
      for ( usize i = 0; i < 10000; ++i ) ::__cpu_pause();
      if ( producer_done.get(micron::memory_order_acquire) ) throw "lambda slot reused while task was running";
      first->call();
      first->~node_base_t();
    }
    if ( !producer_done.get(micron::memory_order_acquire) ) throw "lambda producer did not resume after destruction";
    queue.execute();
    if ( calls != 2 ) throw "lambda capacity-one execution count";
  }
  snowball::end_test_case();

  snowball::test_case("lambda clear and destructor destroy pending captures");
  {
    tracked_callable::live = 0;
    {
      micron::lambda_queue<8> queue;
      for ( int i = 0; i < 8; ++i ) queue.push(tracked_callable{});
      queue.clear();
      if ( tracked_callable::live != 0 ) throw "lambda clear leaked capture";
      for ( int i = 0; i < 4; ++i ) queue.push(tracked_callable{});
    }
    if ( tracked_callable::live != 0 ) throw "lambda destructor leaked capture";
  }
  snowball::end_test_case();

#if defined(__cpp_exceptions)
  snowball::test_case("lambda throwing construction and invocation preserve progress");
  {
    micron::lambda_queue<1> queue;
    throwing_callable callable;
    throwing_callable::throw_move = true;
    bool caught = false;
    try {
      queue.push(micron::move(callable));
    } catch ( int value ) {
      caught = value == 7;
    }
    if ( !caught ) throw "lambda throwing capture was not propagated";

    int calls = 0;
    queue.push([&] {
      ++calls;
      throw 11;
    });
    caught = false;
    try {
      queue.execute();
    } catch ( int value ) {
      caught = value == 11;
    }
    if ( !caught || calls != 1 ) throw "lambda throwing invocation was not propagated";
    queue.push([&] { ++calls; });
    queue.execute();
    if ( calls != 2 ) throw "lambda queue wedged after exception";
  }
  snowball::end_test_case();
#endif

  snowball::print("lambda_queue: all fixed-seed fuzz properties held");
  return 1;
}
