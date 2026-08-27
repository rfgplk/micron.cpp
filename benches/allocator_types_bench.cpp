//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Build:
//   duck build benches/allocator_types_bench.cpp -i . --perf --fp --no-ssp --no-lto -o bin/bench -f
// Run:
//   taskset -c 6 ./bin/bench/allocator_types_bench

#include "../external/bbench/bench.hpp"

#include "../src/allocator.hpp"
#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/thread/thread.hpp"
#include "../src/thread/thread_types/auto_thread.hpp"
#include "../src/vector/vector.hpp"

namespace
{
using events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 samples = 5;
constexpr u32 warmups = 2;
constexpr u64 allocation_ops = 4096;
constexpr u64 vector_size = 1024;
constexpr u64 vector_reps = 32;
constexpr u64 shared_ops = 32768;

struct static_tag;
struct monotonic_tag;

using static_alloc = micron::allocator_static<static_tag, 4u * 1024u * 1024u, 64>;
using monotonic_alloc = micron::allocator_monotonic<monotonic_tag, 4096, micron::allocator_exact<>>;

inline micron::arena_resource<micron::allocator_exact<>> bump_arena{ 512u * 1024u };
inline micron::arena_resource<micron::allocator_exact<>, micron::arena_sync::shared> shared_arena{ 16u * 1024u * 1024u };

volatile uintptr_t pointer_sink = 0;
volatile u64 value_sink = 0;

struct result {
  f64 cycles;
  f64 ipc;
  f64 branch_miss;
};

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t time{};
  micron::clock_gettime(micron::clock_monotonic, time);
  return static_cast<u64>(time.tv_sec) * 1000000000ull + static_cast<u64>(time.tv_nsec);
}

f64
median(f64 *values) noexcept
{
  for ( u32 i = 1; i < samples; ++i ) {
    const f64 value = values[i];
    u32 j = i;
    while ( j && values[j - 1] > value ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
  return values[samples / 2];
}

template<class Alloc>
void
prepare() noexcept
{
  if constexpr ( requires { Alloc::reset(); } ) Alloc::reset();
}

template<class Alloc>
void
release() noexcept
{
  if constexpr ( requires { Alloc::release(); } ) Alloc::release();
}

template<typename Prepare, typename Kernel>
result
measure(u64 operations, Prepare prepare, Kernel kernel)
{
  for ( u32 i = 0; i < warmups; ++i ) {
    prepare();
    kernel();
  }

  f64 cycles[samples];
  f64 ipc[samples];
  f64 misses[samples];
  for ( u32 i = 0; i < samples; ++i ) {
    prepare();
    events counters{ bbench::quiet{} };
    counters.open();
    counters.begin();
    kernel();
    counters.end();

    const f64 cyc = static_cast<f64>(static_cast<u64>(counters.get<bbench::hardware_cycles>().retrieve()));
    const f64 ins = static_cast<f64>(static_cast<u64>(counters.get<bbench::hardware_instructions>().retrieve()));
    const f64 branches = static_cast<f64>(static_cast<u64>(counters.get<bbench::branches>().retrieve()));
    const f64 miss = static_cast<f64>(static_cast<u64>(counters.get<bbench::branch_misses>().retrieve()));
    cycles[i] = cyc / static_cast<f64>(operations);
    ipc[i] = cyc ? ins / cyc : 0.0;
    misses[i] = branches ? miss * 100.0 / branches : 0.0;
  }
  return { median(cycles), median(ipc), median(misses) };
}

struct decimal {
  u64 whole;
  u32 fraction;
};

decimal
format(f64 value) noexcept
{
  const u64 scaled = static_cast<u64>(value * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
}

void
print_decimal(f64 value)
{
  if ( value < 0 ) {
    micron::io::print("-");
    value = -value;
  }
  const decimal number = format(value);
  micron::io::print(number.whole, ".", number.fraction < 10 ? "0" : "", number.fraction);
}

void
row(const char *group, const char *name, const result &value)
{
  micron::io::print(group, "  ", name, "  cycles/op=");
  print_decimal(value.cycles);
  micron::io::print("  IPC=");
  print_decimal(value.ipc);
  micron::io::print("  branch-miss%=");
  print_decimal(value.branch_miss);
  micron::io::print("\n");
}

void
scalar_row(const char *group, const char *name, const char *unit, f64 value)
{
  micron::io::print(group, "  ", name, "  ", unit, "=");
  print_decimal(value);
  micron::io::print("\n");
}

[[gnu::noinline]] void
direct_abc_kernel()
{
  using allocate_fn = micron::chunk<byte> (*)(usize);
  allocate_fn allocate = static_cast<allocate_fn>(&abc::balloc);
  // Keep the direct case at the same call boundary as the adapter; otherwise GCC specializes only this kernel.
  asm volatile("" : "+r"(allocate));
  uintptr_t sink = 0;
  for ( u64 i = 0; i < allocation_ops; ++i ) {
    micron::chunk<byte> memory = allocate(64);
    if ( memory.ptr == nullptr ) micron::exc<micron::except::memory_error>("direct abcmalloc benchmark allocation failed");
    sink ^= reinterpret_cast<uintptr_t>(memory.ptr);
    abc::dealloc(memory.ptr, memory.len);
  }
  pointer_sink ^= sink;
}

[[gnu::noinline]] void
middle_native_kernel()
{
  using traits = micron::allocator_traits<micron::allocator_exact<>>;
  uintptr_t sink = 0;
  for ( u64 i = 0; i < allocation_ops; ++i ) {
    micron::chunk<byte> memory = traits::allocate<16>(64);
    sink ^= reinterpret_cast<uintptr_t>(memory.ptr);
    traits::deallocate<16>(memory);
  }
  pointer_sink ^= sink;
}

[[gnu::noinline]] void
arena_bump_kernel()
{
  uintptr_t sink = 0;
  for ( u64 i = 0; i < allocation_ops; ++i ) sink ^= reinterpret_cast<uintptr_t>(bump_arena.allocate<16>(64).ptr);
  pointer_sink ^= sink;
}

[[gnu::noinline]] void
arena_rewind_kernel()
{
  uintptr_t sink = 0;
  for ( u64 i = 0; i < allocation_ops; ++i ) {
    const auto marker = bump_arena.mark();
    sink ^= reinterpret_cast<uintptr_t>(bump_arena.allocate<16>(64).ptr);
    (void)bump_arena.rewind(marker);
  }
  pointer_sink ^= sink;
}

[[gnu::noinline]] void
temporal_kernel()
{
  uintptr_t sink = 0;
  micron::chunk<byte> memory = micron::allocator_temporal::launder<16>(64);
  for ( u64 i = 0; i < allocation_ops; ++i ) {
    memory = micron::allocator_temporal::launder<16>(64);
    sink ^= reinterpret_cast<uintptr_t>(memory.ptr);
  }
  micron::allocator_temporal::retire<16>(memory);
  pointer_sink ^= sink;
}

result
direct_abc_case()
{
  return measure(allocation_ops, [] { }, direct_abc_kernel);
}

result
middle_native_case()
{
  return measure(allocation_ops, [] { }, middle_native_kernel);
}

result
arena_bump_case()
{
  return measure(allocation_ops, [] { bump_arena.reset(); }, arena_bump_kernel);
}

result
arena_rewind_case()
{
  return measure(allocation_ops, [] { bump_arena.reset(); }, arena_rewind_kernel);
}

result
temporal_case()
{
  return measure(allocation_ops, [] { }, temporal_kernel);
}

template<class Alloc>
result
allocation_case()
{
  auto kernel = [] {
    for ( u64 i = 0; i < allocation_ops; ++i ) {
      micron::chunk<byte> memory = Alloc::create(64);
      memory.ptr[0] = static_cast<byte>(i);
      pointer_sink ^= reinterpret_cast<uintptr_t>(memory.ptr);
      Alloc::destroy(memory);
    }
  };
  result value = measure(allocation_ops, [] { prepare<Alloc>(); }, kernel);
  release<Alloc>();
  return value;
}

template<class Alloc>
result
vector_case()
{
  auto kernel = [] {
    for ( u64 repetition = 0; repetition < vector_reps; ++repetition ) {
      micron::vector<u64, Alloc> values;
      for ( u64 i = 0; i < vector_size; ++i ) values.push_back(i + repetition);
      value_sink += values[vector_size - 1];
    }
  };
  result value = measure(vector_reps * vector_size, [] { prepare<Alloc>(); }, kernel);
  release<Alloc>();
  return value;
}

template<class Alloc>
result
strict_case()
{
  constexpr u64 operations = 128;
  auto kernel = [] {
    for ( u64 i = 0; i < operations; ++i ) {
      micron::chunk<byte> memory = Alloc::create(64);
      memory.ptr[0] = static_cast<byte>(i);
      pointer_sink ^= reinterpret_cast<uintptr_t>(memory.ptr);
      Alloc::destroy(memory);
    }
  };
  return measure(operations, [] { }, kernel);
}

result
fixed_case()
{
  addr_t *candidate = micron::mmap(nullptr, micron::page_size, micron::prot_read | micron::prot_write,
                                   micron::map_private | micron::map_anonymous, -1, 0);
  if ( micron::mmap_failed(candidate) ) micron::exc<micron::except::memory_error>("fixed benchmark reservation failed");
  micron::munmap(candidate, micron::page_size);
  constexpr u64 operations = 128;
  auto kernel = [candidate] {
    for ( u64 i = 0; i < operations; ++i ) {
      micron::chunk<byte> memory = micron::fixed_map_allocator::create_at(candidate, 64);
      memory.ptr[0] = static_cast<byte>(i);
      pointer_sink ^= reinterpret_cast<uintptr_t>(memory.ptr);
      micron::fixed_map_allocator::destroy(memory);
    }
  };
  return measure(operations, [] { }, kernel);
}

f64
fragmentation_case()
{
  constexpr usize count = 1024;
  micron::chunk<byte> memory[count]{};
  usize requested = 0;
  usize granted = 0;
  u32 state = 0xa110'ca7eu;
  for ( usize i = 0; i < count; ++i ) {
    state = state * 1664525u + 1013904223u;
    const usize bytes = 1 + (state & 2047u);
    memory[i] = abc::balloc(bytes);
    requested += bytes;
    granted += memory[i].len;
  }
  for ( usize i = 0; i < count; i += 2 ) abc::dealloc(memory[i].ptr);
  for ( usize i = 1; i < count; i += 2 ) abc::dealloc(memory[i].ptr);
  return requested ? (static_cast<f64>(granted - requested) * 100.0 / static_cast<f64>(requested)) : 0.0;
}

f64
shared_arena_case(usize threads)
{
  f64 values[samples]{};
  for ( u32 sample = 0; sample < samples; ++sample ) {
    shared_arena.reset();
    u64 results[8]{};
    micron::__thread_pointer<micron::auto_thread<>> workers[8];
    const u64 begin = now_ns();
    for ( usize thread = 0; thread < threads; ++thread ) {
      workers[thread] = micron::solo::spawn<micron::auto_thread<>>([&, thread] {
        uintptr_t local = 0;
        for ( u64 i = 0; i < shared_ops; ++i ) {
          micron::chunk<byte> memory = shared_arena.allocate<16>(32);
          memory.ptr[0] = static_cast<byte>(i);
          local ^= reinterpret_cast<uintptr_t>(memory.ptr);
        }
        results[thread] = local;
      });
    }
    for ( usize thread = 0; thread < threads; ++thread ) micron::solo::join(workers[thread]);
    const u64 elapsed = now_ns() - begin;
    for ( usize thread = 0; thread < threads; ++thread ) pointer_sink ^= results[thread];
    values[sample] = static_cast<f64>(elapsed) / static_cast<f64>(threads * shared_ops);
  }
  return median(values);
}
}      // namespace

int
main()
{
  micron::io::println("allocator types: median of 5, 2 warmups");
  micron::io::println("allocation round trips / arena bumps, 64 bytes");
  const result direct = direct_abc_case();
  const result middle = middle_native_case();
  row("fast", "direct-abcmalloc-al16", direct);
  row("fast", "allocator-traits-al16", middle);
  scalar_row("fast", "middle-delta", "percent", direct.cycles ? (middle.cycles - direct.cycles) * 100.0 / direct.cycles : 0.0);
  row("fast", "arena-bump-al16", arena_bump_case());
  row("fast", "arena-mark-rewind", arena_rewind_case());
  row("fast", "temporal-alias", temporal_case());
  row("alloc", "default", allocation_case<micron::allocator_serial<>>());
  row("alloc", "exact", allocation_case<micron::allocator_exact<>>());
  row("alloc", "mmap", allocation_case<micron::map_allocator<>>());
  row("alloc", "static", allocation_case<static_alloc>());
  row("alloc", "monotonic", allocation_case<monotonic_alloc>());

  micron::io::println("vector growth, 1024 u64 elements");
  row("vector", "default", vector_case<micron::allocator_serial<>>());
  row("vector", "exact", vector_case<micron::allocator_exact<>>());
  row("vector", "mmap", vector_case<micron::map_allocator<>>());
  row("vector", "static", vector_case<static_alloc>());
  row("vector", "monotonic", vector_case<monotonic_alloc>());

  micron::io::println("strict security/debug mappings, 64 bytes");
  try {
    row("strict", "secure", strict_case<micron::allocator_secure<>>());
  } catch ( const micron::except::memory_error & ) {
    micron::io::println("strict  secure  unavailable (mlock/madvise)");
  }
  row("strict", "guarded", strict_case<micron::allocator_guarded<>>());
  row("strict", "immutable", strict_case<micron::allocator_immutable>());
  row("strict", "fixed", fixed_case());

  micron::io::println("fragmentation and shared-arena latency");
  scalar_row("fragmentation", "abcmalloc-mixed-1-2048", "overhead-percent", fragmentation_case());
  scalar_row("shared", "1-thread", "ns/op", shared_arena_case(1));
  scalar_row("shared", "2-thread", "ns/op", shared_arena_case(2));
  scalar_row("shared", "4-thread", "ns/op", shared_arena_case(4));
  scalar_row("shared", "8-thread", "ns/op", shared_arena_case(8));
  bump_arena.release();
  shared_arena.release();
  return pointer_sink == static_cast<uintptr_t>(-1) && value_sink == static_cast<u64>(-1) ? 2 : 0;
}
