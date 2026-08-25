//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../src/atomic/atomic.hpp"
#include "../src/bits/__pause.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/memory/cache.hpp"
#include "../src/queue/crossbeam.hpp"
#include "../src/queue/disruptor.hpp"
#include "../src/queue/spsc_queue.hpp"
#include "../src/queue/static_mpmc.hpp"
#include "../src/thread/thread.hpp"
#include "../src/types.hpp"

namespace
{

constexpr usize max_threads = 16;
constexpr usize max_samples = 31;

struct config {
  const char *impl = "crossbeam";
  usize capacity = 4096;
  usize payload = 8;
  usize producers = 4;
  usize consumers = 4;
  usize items_per_producer = 100000;
  usize samples = 7;
  u32 cpus[max_threads]{};
  usize cpu_count = 0;
};

bool
eq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

usize
number(const char *s) noexcept
{
  usize value = 0;
  while ( *s >= '0' && *s <= '9' ) value = value * 10u + static_cast<usize>(*s++ - '0');
  return value;
}

bool
parse_topology(config &cfg, const char *s) noexcept
{
  const char *p = s;
  const usize producers = number(p);
  while ( *p >= '0' && *p <= '9' ) ++p;
  if ( *p != 'p' && *p != 'P' ) return false;
  ++p;
  const usize consumers = number(p);
  while ( *p >= '0' && *p <= '9' ) ++p;
  if ( (*p != 'c' && *p != 'C') || p[1] != '\0' || producers == 0 || consumers == 0 || producers + consumers > max_threads ) return false;
  cfg.producers = producers;
  cfg.consumers = consumers;
  return true;
}

bool
parse_cpus(config &cfg, const char *s) noexcept
{
  cfg.cpu_count = 0;
  while ( *s ) {
    if ( cfg.cpu_count == max_threads || *s < '0' || *s > '9' ) return false;
    cfg.cpus[cfg.cpu_count++] = static_cast<u32>(number(s));
    while ( *s >= '0' && *s <= '9' ) ++s;
    if ( *s == '\0' ) break;
    if ( *s++ != ',' ) return false;
  }
  return cfg.cpu_count != 0;
}

bool
parse(config &cfg, int argc, char **argv) noexcept
{
  for ( int i = 1; i < argc; ++i ) {
    if ( i + 1 == argc ) return false;
    const char *key = argv[i++];
    const char *value = argv[i];
    if ( eq(key, "--impl") )
      cfg.impl = value;
    else if ( eq(key, "--capacity") )
      cfg.capacity = number(value);
    else if ( eq(key, "--payload") )
      cfg.payload = number(value);
    else if ( eq(key, "--topology") || eq(key, "--case") ) {
      if ( !parse_topology(cfg, value) ) return false;
    } else if ( eq(key, "--items") )
      cfg.items_per_producer = number(value);
    else if ( eq(key, "--reps") )
      cfg.samples = number(value);
    else if ( eq(key, "--cpus") ) {
      if ( !parse_cpus(cfg, value) ) return false;
    } else
      return false;
  }

  if ( cfg.cpu_count == 0 ) {
    cfg.cpu_count = cfg.producers + cfg.consumers;
    for ( usize i = 0; i < cfg.cpu_count; ++i ) cfg.cpus[i] = static_cast<u32>(i);
  }
  return cfg.items_per_producer != 0 && cfg.samples >= 5 && cfg.samples <= max_samples && cfg.cpu_count >= cfg.producers + cfg.consumers;
}

void
usage() noexcept
{
  micron::io::println("usage: new_queues_bench --impl spsc|disruptor|crossbeam|static_mpmc"
                      " --capacity 64|512|4096|32768 --payload 8|32|64|256"
                      " --topology 1p1c|2p1c|1p2c|2p2c|4p4c|8p8c"
                      " --cpus 0,1,... --items N --reps N");
}

void
pin(u32 cpu) noexcept
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(cpu);
  micron::posix::sched_setaffinity(0, sizeof(set), set);
}

[[gnu::always_inline]] inline u64
tick() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  u32 lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi) : : "memory");
  return (static_cast<u64>(hi) << 32) | lo;
#elif defined(__micron_arch_arm64)
  u64 value;
  asm volatile("isb; mrs %0, cntvct_el0" : "=r"(value) : : "memory");
  return value;
#else
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
#endif
}

template<usize Bytes> struct payload {
  static_assert(Bytes >= sizeof(u64));
  u64 value;
  byte body[Bytes - sizeof(u64)];

  explicit payload(u64 v = 0) noexcept : value(v)
  {
    for ( usize i = 0; i < sizeof(body); ++i ) body[i] = static_cast<byte>((v + i * 29u) & 0xffu);
  }

  bool
  valid() const noexcept
  {
    for ( usize i = 0; i < sizeof(body); ++i )
      if ( body[i] != static_cast<byte>((value + i * 29u) & 0xffu) ) return false;
    return true;
  }
};

template<> struct payload<8> {
  u64 value;

  explicit payload(u64 v = 0) noexcept : value(v) { }

  bool
  valid() const noexcept
  {
    return true;
  }
};

template<typename T>
[[gnu::always_inline]] inline u64
payload_value(const T &value) noexcept
{
  if constexpr ( micron::is_same_v<T, u64> )
    return value;
  else
    return value.value;
}

template<typename T>
[[gnu::always_inline]] inline bool
payload_valid(const T &value) noexcept
{
  if constexpr ( micron::is_same_v<T, u64> )
    return true;
  else
    return value.valid();
}

template<typename Q> struct queue_ops;

template<typename T, usize N, typename A> struct queue_ops<micron::spsc_queue<T, N, A>> {
  static bool
  push(micron::spsc_queue<T, N, A> &q, const T &v)
  {
    return q.push(v);
  }

  static bool
  pop(micron::spsc_queue<T, N, A> &q, T &v)
  {
    return q.pop(v);
  }

  static constexpr usize footprint = sizeof(micron::spsc_queue<T, N, A>) + N * sizeof(T);
};

template<typename T, usize N, typename A> struct queue_ops<micron::disruptor<T, N, A>> {
  static bool
  push(micron::disruptor<T, N, A> &q, const T &v)
  {
    return q.publish(v);
  }

  static bool
  pop(micron::disruptor<T, N, A> &q, T &v)
  {
    return q.consume(v);
  }

  static constexpr usize footprint = sizeof(micron::disruptor<T, N, A>) + N * sizeof(T);
};

template<typename T, usize N, typename A> struct queue_ops<micron::crossbeam<T, N, A>> {
  static bool
  push(micron::crossbeam<T, N, A> &q, const T &v)
  {
    return q.push(v);
  }

  static bool
  pop(micron::crossbeam<T, N, A> &q, T &v)
  {
    return q.pop(v);
  }

  static constexpr usize cell_align = alignof(T) > micron::cache_line_size() ? alignof(T) : micron::cache_line_size();
  static constexpr usize storage_offset
      = (sizeof(micron::atomic_token<usize>) + sizeof(micron::atomic_token<u8>) + alignof(T) - 1) & ~(alignof(T) - 1);
  static constexpr usize cell_size = (storage_offset + sizeof(T) + cell_align - 1) & ~(cell_align - 1);
  static constexpr usize footprint = sizeof(micron::crossbeam<T, N, A>) + N * cell_size + cell_align - 1;
};

template<typename T, usize N> struct queue_ops<micron::static_mpmc<T, N>> {
  static bool
  push(micron::static_mpmc<T, N> &q, T v)
  {
    return q.push(v);
  }

  static bool
  pop(micron::static_mpmc<T, N> &q, T &v)
  {
    return q.pop(v);
  }

  static constexpr usize footprint = sizeof(micron::static_mpmc<T, N>);
};

struct alignas(64) thread_result {
  u64 count = 0;
  u64 sum = 0;
  u64 xors = 0;
  u64 begin = 0;
  u64 end = 0;
  u64 bad = 0;
  u64 pad[2]{};
};

static_assert(sizeof(thread_result) == 64);

template<typename Q>
u64
sample(const config &cfg, u64 expected_sum, u64 expected_xor, bool &valid)
{
  using value_t = typename Q::value_type;
  Q *queue = new Q;
  thread_result result[max_threads]{};
  micron::atomic_token<usize> ready{ 0 };
  micron::atomic_token<bool> go{ false };
  micron::__thread_pointer<micron::auto_thread<>> workers[max_threads];
  const usize threads = cfg.producers + cfg.consumers;
  const usize total = cfg.producers * cfg.items_per_producer;
  value_t *input = new value_t[total];
  for ( usize i = 0; i < total; ++i ) input[i] = value_t(static_cast<u64>(i + 1));

  for ( usize tid = 0; tid < threads; ++tid ) {
    workers[tid] = micron::solo::spawn([&, tid] {
      pin(cfg.cpus[tid]);
      ready.fetch_add(1, micron::memory_order_release);
      while ( !go.get(micron::memory_order_acquire) ) ::__cpu_pause();
      thread_result &mine = result[tid];
      mine.begin = tick();
      if ( tid < cfg.producers ) {
        const usize base = tid * cfg.items_per_producer;
        for ( usize i = 0; i < cfg.items_per_producer; ++i ) {
          const value_t &value = input[base + i];
          while ( !queue_ops<Q>::push(*queue, value) ) ::__cpu_pause();
          ++mine.count;
          mine.sum += payload_value(value);
          mine.xors ^= payload_value(value);
        }
      } else {
        const usize consumer = tid - cfg.producers;
        const usize quota = total / cfg.consumers + (consumer < total % cfg.consumers ? 1 : 0);
        value_t value;
        while ( mine.count < quota ) {
          if ( queue_ops<Q>::pop(*queue, value) ) {
            ++mine.count;
            mine.sum += payload_value(value);
            mine.xors ^= payload_value(value);
            mine.bad += !payload_valid(value);
          } else {
            ::__cpu_pause();
          }
        }
      }
      mine.end = tick();
    });
  }

  while ( ready.get(micron::memory_order_acquire) != threads ) ::__cpu_pause();
  go.store(true, micron::memory_order_release);
  for ( usize i = 0; i < threads; ++i ) micron::solo::join(workers[i]);

  u64 first = static_cast<u64>(-1), last = 0;
  u64 produced_count = 0, produced_sum = 0, produced_xor = 0;
  u64 consumed_count = 0, consumed_sum = 0, consumed_xor = 0, bad = 0;
  for ( usize i = 0; i < threads; ++i ) {
    if ( result[i].begin < first ) first = result[i].begin;
    if ( result[i].end > last ) last = result[i].end;
    if ( i < cfg.producers ) {
      produced_count += result[i].count;
      produced_sum += result[i].sum;
      produced_xor ^= result[i].xors;
    } else {
      consumed_count += result[i].count;
      consumed_sum += result[i].sum;
      consumed_xor ^= result[i].xors;
      bad += result[i].bad;
    }
  }

  valid = produced_count == total && consumed_count == total && produced_sum == expected_sum && consumed_sum == expected_sum
          && produced_xor == expected_xor && consumed_xor == expected_xor && bad == 0 && queue->empty();
  delete[] input;
  delete queue;
  return last - first;
}

void
sort(f64 *values, usize n) noexcept
{
  for ( usize i = 1; i < n; ++i ) {
    const f64 value = values[i];
    usize j = i;
    while ( j && values[j - 1] > value ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
}

f64
median(f64 *values, usize n) noexcept
{
  sort(values, n);
  return values[n / 2];
}

f64
absolute(f64 value) noexcept
{
  return value < 0 ? -value : value;
}

void
decimal(const char *label, f64 value, const char *unit) noexcept
{
  const u64 scaled = static_cast<u64>(value * 100.0 + 0.5);
  micron::io::print(label, scaled / 100, ".", scaled % 100 < 10 ? "0" : "", scaled % 100, " ", unit);
}

template<typename Q>
int
run(const config &cfg)
{
  const usize total = cfg.producers * cfg.items_per_producer;
  u64 expected_sum = 0, expected_xor = 0;
  for ( usize i = 1; i <= total; ++i ) {
    expected_sum += i;
    expected_xor ^= i;
  }

  f64 values[max_samples];
  for ( usize pass = 0; pass < cfg.samples + 2; ++pass ) {
    bool valid = false;
    const u64 elapsed = sample<Q>(cfg, expected_sum, expected_xor, valid);
    if ( !valid ) {
      micron::io::println("validation failed: count/sum/xor/payload mismatch");
      return 2;
    }
    if ( pass >= 2 ) values[pass - 2] = static_cast<f64>(elapsed) / static_cast<f64>(total);
  }

  f64 copy[max_samples];
  for ( usize i = 0; i < cfg.samples; ++i ) copy[i] = values[i];
  const f64 med = median(copy, cfg.samples);
  for ( usize i = 0; i < cfg.samples; ++i ) copy[i] = absolute(values[i] - med);
  const f64 mad = median(copy, cfg.samples);
  const f64 relative_mad = med > 0 ? 100.0 * mad / med : 0.0;
  const f64 threshold = 3.0 * relative_mad > 5.0 ? 3.0 * relative_mad : 5.0;

  micron::io::print("impl=", cfg.impl, " topology=", cfg.producers, "P", cfg.consumers, "C capacity=", cfg.capacity,
                    " payload=", cfg.payload, "B items=", total, " object=", sizeof(Q), "B footprint=", queue_ops<Q>::footprint, "B\n");
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm64)
  decimal("median=", med, "cycles/item");
#else
  decimal("median=", med, "ns/item");
#endif
  decimal(" MAD=", mad, "");
  decimal(" relative-MAD=", relative_mad, "%");
  decimal(" acceptance-threshold=", threshold, "%\n");
  micron::io::print("raw=");
  for ( usize i = 0; i < cfg.samples; ++i ) {
    decimal(i ? "," : "", values[i], "");
  }
  micron::io::print("\n");
  return 0;
}

template<usize Cap, usize Bytes>
int
dispatch_impl(const config &cfg)
{
  if ( eq(cfg.impl, "spsc") ) {
    if ( cfg.producers != 1 || cfg.consumers != 1 ) return 3;
    return run<micron::spsc_queue<payload<Bytes>, Cap>>(cfg);
  }
  if ( eq(cfg.impl, "disruptor") ) {
    if ( cfg.producers != 1 || cfg.consumers != 1 ) return 3;
    return run<micron::disruptor<payload<Bytes>, Cap>>(cfg);
  }
  if ( eq(cfg.impl, "crossbeam") ) return run<micron::crossbeam<payload<Bytes>, Cap>>(cfg);
  if ( eq(cfg.impl, "static_mpmc") ) {
    if constexpr ( Bytes == 8 ) return run<micron::static_mpmc<u64, Cap>>(cfg);
    return 4;
  }
  return 5;
}

template<usize Cap>
int
dispatch_payload(const config &cfg)
{
  switch ( cfg.payload ) {
  case 8:
    return dispatch_impl<Cap, 8>(cfg);
  case 32:
    return dispatch_impl<Cap, 32>(cfg);
  case 64:
    return dispatch_impl<Cap, 64>(cfg);
  case 256:
    return dispatch_impl<Cap, 256>(cfg);
  default:
    return 4;
  }
}

}      // namespace

int
main(int argc, char **argv)
{
  config cfg;
  if ( !parse(cfg, argc, argv) ) {
    usage();
    return 1;
  }

  int result = 0;
  switch ( cfg.capacity ) {
  case 64:
    result = dispatch_payload<64>(cfg);
    break;
  case 512:
    result = dispatch_payload<512>(cfg);
    break;
  case 4096:
    result = dispatch_payload<4096>(cfg);
    break;
  case 32768:
    result = dispatch_payload<32768>(cfg);
    break;
  default:
    result = 4;
    break;
  }

  if ( result == 3 ) micron::io::println("spsc/disruptor require --topology 1p1c");
  if ( result == 4 ) micron::io::println("unsupported capacity/payload (static_mpmc supports only 8-byte atomic payloads)");
  if ( result == 5 ) micron::io::println("unknown implementation");
  return result;
}
