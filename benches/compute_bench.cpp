//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Build: duck build benches/compute_bench.cpp --perf --fp --no-ssp --no-lto -o bin/compute-bench -f
// Run:   taskset -c <quiet-cpu> bin/compute-bench/compute_bench

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/math/compute.hpp"
#include "../src/math/compute/thread_pool.hpp"

namespace mcx = micron::math::compute;

namespace
{

constexpr u32 measurements = 7;
constexpr u32 warmups = 2;
volatile u64 sink{};

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t time{};
  micron::clock_gettime(micron::clock_monotonic, time);
  return static_cast<u64>(time.tv_sec) * 1000000000ull + static_cast<u64>(time.tv_nsec);
}

f64
minimum(const f64 *values, usize count) noexcept
{
  f64 result = values[0];
  for ( usize i = 1; i < count; ++i )
    if ( values[i] < result ) result = values[i];
  return result;
}

void
row(const char *name, usize items, f64 nanoseconds)
{
  micron::io::print("  ", name);
  for ( usize i = micron::strlen(name); i < 31; ++i ) micron::io::print(" ");
  const u64 hundredths = static_cast<u64>(nanoseconds * 100.0 + 0.5);
  micron::io::print("N=", items, "  ", hundredths / 100, ".");
  if ( hundredths % 100 < 10 ) micron::io::print("0");
  micron::io::println(hundredths % 100, " ns/item");
}

void
metric(const char *name, usize value)
{
  micron::io::print("  ", name);
  for ( usize i = micron::strlen(name); i < 31; ++i ) micron::io::print(" ");
  micron::io::println(value);
}

template<typename Fn>
void
measure(const char *name, usize items, usize repetitions, Fn fn)
{
  f64 samples[measurements]{};
  for ( u32 sample = 0; sample < measurements + warmups; ++sample ) {
    const u64 begin = now_ns();
    u64 value = 0;
    for ( usize repetition = 0; repetition < repetitions; ++repetition ) value += static_cast<u64>(fn());
    const u64 elapsed = now_ns() - begin;
    sink = sink + value;
    if ( sample >= warmups ) samples[sample - warmups] = static_cast<f64>(elapsed) / static_cast<f64>(items * repetitions);
  }
  row(name, items, minimum(samples, measurements));
}

enum class dtype : u8 { u64 = 0 };
enum class opcode : u8 { no_op = 0, copy, add, view, increment };

struct operation {
  opcode code{};
  usize offset{};
};

struct context {
  micron::atomic_token<u64> calls{ 0 };
};

struct registry {
  using operation_type = operation;
  using dtype_type = dtype;
  using context_type = context;

  static constexpr usize
  dtype_size(dtype_type) noexcept
  {
    return sizeof(u64);
  }

  static constexpr usize
  output_count(const operation_type &) noexcept
  {
    return 1;
  }

  static mcx::compute_status
  infer(const operation_type &, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>> inputs,
        micron::raw_slice<mcx::tensor_descriptor<dtype_type>> outputs) noexcept
  {
    if ( inputs.is_empty() || outputs.size() != 1 ) return mcx::compute_status::invalid_plan;
    outputs[0] = inputs[0];
    outputs[0].access = mcx::tensor_access::read_write;
    return mcx::compute_status::ok;
  }

  static mcx::compute_status
  validate(const operation_type &operation, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>> inputs,
           micron::raw_slice<mcx::tensor_descriptor<dtype_type>> outputs) noexcept
  {
    const usize wanted = operation.code == opcode::add ? 2 : 1;
    return inputs.size() == wanted && outputs.size() == 1 ? mcx::compute_status::ok : mcx::compute_status::invalid_plan;
  }

  static mcx::compute_status
  aliases(const operation_type &operation, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>>,
          micron::raw_slice<const mcx::tensor_descriptor<dtype_type>>, micron::raw_slice<mcx::alias_declaration> aliases) noexcept
  {
    if ( operation.code == opcode::view ) aliases[0] = { mcx::alias_mode::read_only_view, 0, operation.offset };
    if ( operation.code == opcode::increment ) aliases[0] = { mcx::alias_mode::destructive_in_place, 0, operation.offset };
    return mcx::compute_status::ok;
  }

  static mcx::compute_status
  execute(const operation_type &operation, context_type &context, micron::raw_slice<const mcx::tensor_view<dtype_type>> inputs,
          micron::raw_slice<mcx::tensor_view<dtype_type>> outputs) noexcept
  {
    context.calls.fetch_add(1, micron::memory_order_relaxed);
    if ( operation.code == opcode::no_op || operation.code == opcode::view ) return mcx::compute_status::ok;
    const usize count = outputs[0].descriptor->byte_size / sizeof(u64);
    const u64 *left = inputs[0].template as<u64>();
    u64 *output = outputs[0].template as<u64>();
    if ( operation.code == opcode::increment ) {
      for ( usize i = 0; i < count; ++i ) ++output[i];
    } else if ( operation.code == opcode::copy ) {
      for ( usize i = 0; i < count; ++i ) output[i] = left[i];
    } else {
      const u64 *right = inputs[1].template as<u64>();
      for ( usize i = 0; i < count; ++i ) output[i] = left[i] + right[i];
    }
    return mcx::compute_status::ok;
  }
};

using plan_type = mcx::compute_plan<registry>;

mcx::tensor_descriptor<dtype>
descriptor(usize elements, mcx::tensor_access access = mcx::tensor_access::read_only)
{
  mcx::tensor_descriptor<dtype> result;
  result.dtype = dtype::u64;
  result.dimensions.push_back(elements);
  result.access = access;
  return result;
}

struct plan_bundle {
  plan_type plan;
  mcx::tensor_id input;
  mcx::tensor_id output;
};

plan_bundle
make_chain(usize nodes, usize elements, opcode code = opcode::no_op)
{
  mcx::compute_graph<registry> builder;
  const auto input = builder.add_input(descriptor(elements));
  auto current = input;
  for ( usize node = 0; node < nodes; ++node ) {
    const mcx::tensor_id inputs[]{ current };
    current = builder.add_one({ code }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
  }
  builder.retain(current);
  return { builder.finalize(), input, current };
}

plan_bundle
make_parallel(usize nodes, usize elements)
{
  mcx::compute_graph<registry> builder;
  const auto input = builder.add_input(descriptor(elements));
  mcx::tensor_id last{};
  const mcx::tensor_id inputs[]{ input };
  for ( usize node = 0; node < nodes; ++node ) {
    last = builder.add_one({ opcode::no_op }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    builder.retain(last);
  }
  return { builder.finalize(), input, last };
}

};      // namespace

int
main()
{
  constexpr usize elements = 128;
  constexpr usize chain_nodes = 256;
  constexpr usize parallel_nodes = 128;
  micron::vector<u64> input_values(elements, u64(7));

  auto chain = make_chain(chain_nodes, elements);
  mcx::compute_session<registry> chain_session(chain.plan);
  (void)chain_session.bind_borrowed(chain.input, input_values.data(), descriptor(elements));

  auto parallel = make_parallel(parallel_nodes, elements);
  mcx::compute_session<registry> parallel_session(parallel.plan);
  (void)parallel_session.bind_borrowed(parallel.input, input_values.data(), descriptor(elements));
  micron::concurrent_arena arena;
  (void)arena.create();
  (void)arena.create();
  (void)arena.create();
  (void)arena.create();
  mcx::thread_pool_executor executor(arena);

  mcx::compute_graph<registry> alias_builder;
  const auto alias_input = alias_builder.add_input(descriptor(elements));
  const mcx::tensor_id alias_inputs[]{ alias_input };
  const auto alias_output = alias_builder.add_one({ opcode::view }, micron::raw_slice<const mcx::tensor_id>(alias_inputs, 1));
  auto alias_plan = alias_builder.finalize();
  mcx::compute_session<registry> alias_session(alias_plan);
  (void)alias_session.bind_borrowed(alias_input, input_values.data(), descriptor(elements));

  mcx::compute_graph<registry> state_builder;
  const auto state = state_builder.add_state(descriptor(elements, mcx::tensor_access::read_write));
  const mcx::tensor_id state_inputs[]{ state };
  const auto state_output = state_builder.add_one({ opcode::increment }, micron::raw_slice<const mcx::tensor_id>(state_inputs, 1));
  auto state_plan = state_builder.finalize();
  mcx::compute_session<registry> state_session(state_plan);

  auto producer_bundle = make_chain(1, elements, opcode::copy);
  mcx::compute_session<registry> producer(producer_bundle.plan);
  (void)producer.bind_borrowed(producer_bundle.input, input_values.data(), descriptor(elements));
  (void)producer.execute();
  auto shared = producer.export_tensor(producer_bundle.output);
  mcx::compute_graph<registry> consumer_builder;
  const auto consumer_input = consumer_builder.add_input(descriptor(elements, mcx::tensor_access::read_write));
  const mcx::tensor_id consumer_inputs[]{ consumer_input };
  const auto consumer_output = consumer_builder.add_one({ opcode::view }, micron::raw_slice<const mcx::tensor_id>(consumer_inputs, 1));
  auto consumer_plan = consumer_builder.finalize();
  mcx::compute_session<registry> consumer(consumer_plan);

  micron::io::println("=== micron::math::compute BENCH ===");
  micron::io::println("  workload                       size       minimum");
  measure("finalize linear DAG", chain_nodes, 5, [] { return make_chain(chain_nodes, elements).plan.slots_count(); });
  measure("session arena setup", chain.plan.slots_count(), 20,
          [&] { return mcx::compute_session<registry>(chain.plan).context().calls.get(micron::memory_order_relaxed); });
  measure("inline no-op dispatch", chain_nodes, 40, [&] { return chain_session.execute().succeeded(); });
  measure("parallel independent nodes", parallel_nodes, 25, [&] { return parallel_session.execute(executor).succeeded(); });
  measure("read-only alias view", 1, 4000, [&] {
    const auto result = alias_session.execute();
    return result.succeeded() && alias_session.data(alias_output) == input_values.data();
  });
  measure("persistent state reuse", elements, 500, [&] {
    const auto result = state_session.execute();
    return result.succeeded() && state_session.data(state_output) == state_session.data(state);
  });
  measure("zero-copy session handoff", elements, 1000, [&] {
    const bool bound = consumer.bind_shared(consumer_input, shared) == mcx::compute_status::ok;
    const auto result = consumer.execute();
    return bound && result.succeeded() && consumer.data(consumer_output) == shared.data();
  });

  metric("chain arena allocations", chain.plan.slots_count());
  metric("chain arena peak bytes", chain.plan.peak_bytes());
  metric("parallel arena allocations", parallel.plan.slots_count());
  metric("parallel arena peak bytes", parallel.plan.peak_bytes());
  micron::io::println("sink=", sink);
  return 1;
}
