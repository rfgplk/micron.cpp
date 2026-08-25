//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../snowball/snowball_fuzz.hpp"
#include "../support/lifetime.hpp"
#include "../support/mt.hpp"

#include "../../src/queue/conqueue.hpp"
#include "../../src/queue/crossbeam.hpp"
#include "../../src/queue/disruptor.hpp"
#include "../../src/queue/lambda_queue.hpp"
#include "../../src/queue/spsc_queue.hpp"
#include "../../src/queue/static_mpmc.hpp"

namespace sbf = snowball::fuzzing;

namespace
{

template<usize Bytes> struct payload {
  static_assert(Bytes >= sizeof(u64));
  u64 id;
  byte body[Bytes - sizeof(u64)];

  explicit payload(u64 value = 0) noexcept : id(value)
  {
    for ( usize i = 0; i < sizeof(body); ++i ) body[i] = static_cast<byte>((value * 131u + i * 17u) & 0xffu);
  }

  bool
  valid() const noexcept
  {
    for ( usize i = 0; i < sizeof(body); ++i )
      if ( body[i] != static_cast<byte>((id * 131u + i * 17u) & 0xffu) ) return false;
    return true;
  }
};

template<> struct payload<8> {
  u64 id;

  explicit payload(u64 value = 0) noexcept : id(value) { }

  bool
  valid() const noexcept
  {
    return true;
  }
};

template<typename Atomic>
inline void
note_failure(Atomic &failure, int code) noexcept
{
  int expected = 0;
  failure.compare_exchange_strong(expected, code, micron::memory_order_acq_rel, micron::memory_order_relaxed);
}

template<typename Queue, typename Push, typename Pop>
void
run_spsc(u64 seed, Queue &queue, Push push, Pop pop)
{
  constexpr usize total_base = 24000;
  const usize total = total_base + static_cast<usize>(seed & 2047u);
  micron::atomic_token<int> failure{ 0 };
  micron::atomic_token<bool> consumed_all{ false };
  ltest::barrier_t gate;
  gate.n = 3;

  mtest::parallel(3, [&](int role) {
    u32 sense = 0;
    ltest::barrier_wait(gate, sense);
    if ( role == 0 ) {
      for ( usize i = 0; i < total; ++i ) {
        typename Queue::value_type value(static_cast<u64>(i) ^ seed);
        while ( !push(queue, value) ) ::__cpu_pause();
      }
    } else if ( role == 1 ) {
      typename Queue::value_type value;
      for ( usize i = 0; i < total; ) {
        if ( !pop(queue, value) ) {
          ::__cpu_pause();
          continue;
        }
        if ( value.id != (static_cast<u64>(i) ^ seed) || !value.valid() ) note_failure(failure, 1);
        ++i;
      }
      consumed_all.store(true, micron::memory_order_release);
    } else {
      while ( !consumed_all.get(micron::memory_order_acquire) ) {
        if ( queue.size() > queue.capacity() ) note_failure(failure, 2);
        ::__cpu_pause();
      }
    }
  });

  if ( failure.get(micron::memory_order_acquire) != 0 ) throw "SPSC/disruptor concurrent FIFO, payload, or size sampling";
  if ( !queue.empty() || queue.size() != 0 ) throw "SPSC/disruptor concurrent final state";
}

template<typename Queue, typename Value, typename Make, typename Read>
void
run_mpmc(u64 seed, Queue &queue, Make make, Read read)
{
  constexpr usize producers = 2;
  constexpr usize consumers = 2;
  constexpr usize per = 6000;
  constexpr usize total = producers * per;
  micron::atomic_token<u8> seen[total];
  for ( usize i = 0; i < total; ++i ) seen[i].store(0, micron::memory_order_relaxed);

  micron::atomic_token<usize> produced_done{ 0 };
  micron::atomic_token<usize> consumed{ 0 };
  micron::atomic_token<int> failure{ 0 };
  ltest::barrier_t gate;
  gate.n = producers + consumers + 1;

  mtest::parallel(static_cast<int>(gate.n), [&](int role) {
    u32 sense = 0;
    ltest::barrier_wait(gate, sense);
    if ( role < static_cast<int>(producers) ) {
      const usize base = static_cast<usize>(role) * per;
      for ( usize i = 0; i < per; ++i ) {
        Value value = make(base + i, seed);
        while ( !queue.push(micron::move(value)) ) ::__cpu_pause();
      }
      produced_done.fetch_add(1, micron::memory_order_release);
      return;
    }

    if ( role < static_cast<int>(producers + consumers) ) {
      Value value = make(0, seed);
      usize idle = 0;
      while ( consumed.get(micron::memory_order_acquire) < total ) {
        if ( queue.pop(value) ) {
          idle = 0;
          const usize id = read(value, seed);
          if ( id >= total ) {
            note_failure(failure, 3);
          } else {
            u8 expected = 0;
            if ( !seen[id].compare_exchange_strong(expected, 1, micron::memory_order_acq_rel, micron::memory_order_relaxed) )
              note_failure(failure, 4);
          }
          consumed.fetch_add(1, micron::memory_order_release);
        } else {
          if ( produced_done.get(micron::memory_order_acquire) == producers && ++idle > 1000000 ) {
            note_failure(failure, 5);
            return;
          }
          ::__cpu_pause();
        }
      }
      return;
    }

    while ( consumed.get(micron::memory_order_acquire) < total ) {
      if ( queue.size() > queue.capacity() ) note_failure(failure, 6);
      ::__cpu_pause();
    }
  });

  if ( failure.get(micron::memory_order_acquire) != 0 || consumed.get(micron::memory_order_acquire) != total )
    throw "MPMC concurrent progress/uniqueness/size sampling";
  for ( usize i = 0; i < total; ++i )
    if ( seen[i].get(micron::memory_order_acquire) != 1 ) throw "MPMC concurrent missing value";
  if ( !queue.empty() ) throw "MPMC concurrent final state";
}

void
run_lambda(u64 seed)
{
  constexpr usize producers = 2;
  constexpr usize consumers = 2;
  constexpr usize per = 4096;
  constexpr usize total = producers * per;
  micron::lambda_queue<8> queue;
  micron::atomic_token<u8> seen[total];
  for ( usize i = 0; i < total; ++i ) seen[i].store(0, micron::memory_order_relaxed);
  micron::atomic_token<usize> produced_done{ 0 };
  micron::atomic_token<usize> executed{ 0 };
  micron::atomic_token<int> failure{ 0 };
  ltest::barrier_t gate;
  gate.n = producers + consumers;

  mtest::parallel(static_cast<int>(gate.n), [&](int role) {
    u32 sense = 0;
    ltest::barrier_wait(gate, sense);
    if ( role < static_cast<int>(producers) ) {
      const usize base = static_cast<usize>(role) * per;
      for ( usize i = 0; i < per; ++i ) {
        const usize id = base + i;
        queue.push([&, id, salt = seed] {
          if ( (id ^ salt) != (id ^ seed) ) note_failure(failure, 7);
          u8 expected = 0;
          if ( !seen[id].compare_exchange_strong(expected, 1, micron::memory_order_acq_rel, micron::memory_order_relaxed) )
            note_failure(failure, 8);
          executed.fetch_add(1, micron::memory_order_release);
        });
      }
      produced_done.fetch_add(1, micron::memory_order_release);
      return;
    }

    while ( executed.get(micron::memory_order_acquire) < total ) {
      if ( auto *task = queue.pop() ) {
        task->call();
        task->~node_base_t();
      } else {
        ::__cpu_pause();
      }
      if ( queue.size() > queue.max_size() ) note_failure(failure, 9);
    }
  });

  if ( produced_done.get(micron::memory_order_acquire) != producers || executed.get(micron::memory_order_acquire) != total
       || failure.get(micron::memory_order_acquire) != 0 )
    throw "lambda_queue multi-producer/multi-consumer progress";
  for ( usize i = 0; i < total; ++i )
    if ( seen[i].get(micron::memory_order_acquire) != 1 ) throw "lambda_queue concurrent missing callable";
  if ( !queue.empty() ) throw "lambda_queue concurrent final state";
}

void
run_conqueue_copy_swap(u64 seed)
{
  constexpr usize per = 5000;
  constexpr usize total = 2 * per;
  micron::conqueue<usize, 4> a, b;
  micron::atomic_token<int> failure{ 0 };
  ltest::barrier_t gate;
  gate.n = 3;

  mtest::parallel(3, [&](int role) {
    u32 sense = 0;
    ltest::barrier_wait(gate, sense);
    if ( role < 2 ) {
      auto &queue = role == 0 ? a : b;
      const usize base = static_cast<usize>(role) * per;
      for ( usize i = 0; i < per; ++i ) queue.push((base + i) ^ static_cast<usize>(seed));
    } else {
      for ( usize i = 0; i < 1500; ++i ) {
        a.swap(b);
        micron::conqueue<usize, 4> copy(a);
        micron::conqueue<usize, 4> assigned;
        assigned = b;
        if ( copy.size() > total || assigned.size() > total ) note_failure(failure, 10);
      }
    }
  });

  if ( failure.get(micron::memory_order_acquire) != 0 || a.size() + b.size() != total ) throw "conqueue concurrent copy/swap lost values";
  u8 seen[total]{};
  auto inspect = [&](const usize &encoded) {
    const usize id = encoded ^ static_cast<usize>(seed);
    if ( id >= total || seen[id] ) throw "conqueue concurrent copy/swap duplicate";
    seen[id] = 1;
  };
  a.for_each_locked(inspect);
  b.for_each_locked(inspect);
  for ( usize i = 0; i < total; ++i )
    if ( !seen[i] ) throw "conqueue concurrent copy/swap missing value";
}

}      // namespace

int
main()
{
  sbf::check_property(
      "SPSC concurrent exact FIFO, 256-byte payload, sampled size",
      [](u64 seed) {
        micron::spsc_queue<payload<256>, 64> queue;
        run_spsc(seed, queue, [](auto &q, const auto &v) { return q.push(v); }, [](auto &q, auto &v) { return q.pop(v); });
      },
      { .seed = 0x5150534300000001ULL, .count = 2 }, sbf::spec<u64>{});

  sbf::check_property(
      "disruptor concurrent exact FIFO, 64-byte payload, sampled size",
      [](u64 seed) {
        micron::disruptor<payload<64>, 64> queue;
        run_spsc(seed, queue, [](auto &q, const auto &v) { return q.publish(v); }, [](auto &q, auto &v) { return q.consume(v); });
      },
      { .seed = 0xD157A00000000002ULL, .count = 2 }, sbf::spec<u64>{});

  sbf::check_property(
      "crossbeam 2P2C exact uniqueness, 32-byte payload, sampled size",
      [](u64 seed) {
        micron::crossbeam<payload<32>, 64> queue;
        run_mpmc<decltype(queue), payload<32>>(
            seed, queue, [](usize id, u64 salt) { return payload<32>(static_cast<u64>(id) ^ salt); },
            [](const payload<32> &value, u64 salt) {
              if ( !value.valid() ) return static_cast<usize>(-1);
              return static_cast<usize>(value.id ^ salt);
            });
      },
      { .seed = 0xC2055BEA00000003ULL, .count = 2 }, sbf::spec<u64>{});

  sbf::check_property(
      "static_mpmc 2P2C exact uniqueness and sampled size",
      [](u64 seed) {
        micron::static_mpmc<usize, 64> queue;
        run_mpmc<decltype(queue), usize>(
            seed, queue, [](usize id, u64 salt) { return id ^ static_cast<usize>(salt); },
            [](usize value, u64 salt) { return value ^ static_cast<usize>(salt); });
      },
      { .seed = 0x57A71C0000000004ULL, .count = 2 }, sbf::spec<u64>{});

  snowball::test_case("crossbeam 2P2C non-trivial lifetime and slot reuse");
  {
    using value_t = ltest::tracked<91>;
    value_t::reset();
    {
      micron::crossbeam<value_t, 64> queue;
      run_mpmc<decltype(queue), value_t>(
          0x71F371F371F371F3ULL, queue, [](usize id, u64 salt) { return value_t(static_cast<i64>(id ^ static_cast<usize>(salt))); },
          [](const value_t &value, u64 salt) { return static_cast<usize>(value.v) ^ static_cast<usize>(salt); });
    }
    if ( value_t::faults() != 0 || value_t::live() != 0 ) throw "crossbeam concurrent non-trivial lifetime corruption";
  }
  snowball::end_test_case();

  sbf::check_property("lambda_queue 2P2C exact callable execution", run_lambda, { .seed = 0x1A4BDA0000000005ULL, .count = 2 },
                      sbf::spec<u64>{});
  sbf::check_property("conqueue copy/swap versus concurrent mutation", run_conqueue_copy_swap,
                      { .seed = 0xC04C0E0000000006ULL, .count = 2 }, sbf::spec<u64>{});

  snowball::print("queue concurrent fuzz: all fixed-seed properties held");
  return 1;
}
