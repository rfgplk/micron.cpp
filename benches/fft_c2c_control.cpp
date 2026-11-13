//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
// Calibration for perf c2c: distinct counters on one line versus separate lines.
#include "../src/atomic/intrin.hpp"
#include "../src/chrono/clock.hpp"
#include "../src/io/console.hpp"
#include "../src/math/compute/thread_pool.hpp"

alignas(64) static u64 counters[16]{};

struct alignas(64) state {
  u32 ready{}, start{}, done{};
  int pin_error{};
  usize iterations;
  u64 *counter;
  unsigned cpu;
};

static void
worker(void *context) noexcept
{
  auto &s = *static_cast<state *>(context);
  micron::posix::cpu_set_t mask(s.cpu);
  s.pin_error = micron::posix::sched_setaffinity(0, sizeof(mask), mask);
  micron::atom::store(&s.ready, 1u, micron::atomic_release);
  while ( !micron::atom::load(&s.start, micron::atomic_acquire) ) micron::posix::sched_yield();
  for ( usize i = 0; i < s.iterations; ++i ) micron::atom::fetch_add(s.counter, u64(1), micron::atomic_relaxed);
  micron::atom::store(&s.done, 1u, micron::atomic_release);
}

static usize
number(const char *p)
{
  usize n = 0;
  for ( ; *p >= '0' && *p <= '9'; ++p ) n = 10 * n + usize(*p - '0');
  return n;
}

int
main(int argc, char **argv)
{
  if ( argc != 5 ) {
    micron::io::println("fft_c2c_control packed|separate ITERATIONS CPU_A CPU_B");
    return 2;
  }
  bool packed = argv[1][0] == 'p';
  usize iterations = number(argv[2]), a = number(argv[3]), b = number(argv[4]);
  if ( !iterations || iterations > 1000000000 || a >= 1024 || b >= 1024 || a == b ) return 2;
  micron::concurrent_arena arena;
  (void)arena.create();
  micron::math::compute::thread_pool_executor executor(arena);
  state s{ 0, 0, 0, 0, iterations, counters + (packed ? 1 : 8), unsigned(b) };
  if ( !executor.submit(worker, &s) ) return 2;
  micron::posix::cpu_set_t mask{ unsigned(a) };
  int pin_error = micron::posix::sched_setaffinity(0, sizeof(mask), mask);
  while ( !micron::atom::load(&s.ready, micron::atomic_acquire) ) micron::posix::sched_yield();
  auto begin = micron::chrono::mono_ns();
  micron::atom::store(&s.start, 1u, micron::atomic_release);
  for ( usize i = 0; i < iterations; ++i ) micron::atom::fetch_add(counters, u64(1), micron::atomic_relaxed);
  while ( !micron::atom::load(&s.done, micron::atomic_acquire) ) micron::posix::sched_yield();
  auto ns = micron::chrono::mono_ns() - begin;
  micron::io::println("layout,iterations,cpu_a,cpu_b,elapsed_ns,counter_a,counter_b,pin_a,pin_b");
  micron::io::println(packed ? "packed" : "separate", ",", iterations, ",", a, ",", b, ",", ns, ",", counters[0], ",", *s.counter, ",",
                      pin_error, ",", s.pin_error);
  return pin_error || s.pin_error || counters[0] != iterations || *s.counter != iterations ? 2 : 0;
}
