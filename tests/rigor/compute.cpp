//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/compute.hpp"
#include "../../src/math/compute/thread_pool.hpp"
#include "../snowball/snowball.hpp"

namespace mcx = micron::math::compute;

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

enum class test_dtype : u8 { i32 = 0 };
enum class test_opcode : u8 { add = 0, copy, view, increment, fail };

struct test_operation {
  test_opcode opcode{};
  usize elements{};
  usize offset{};
};

struct test_context {
  micron::atomic_token<u32> calls{ 0 };
};

struct test_registry {
  using operation_type = test_operation;
  using dtype_type = test_dtype;
  using context_type = test_context;

  static constexpr usize
  dtype_size(dtype_type) noexcept
  {
    return sizeof(i32);
  }

  static constexpr usize
  output_count(const operation_type &) noexcept
  {
    return 1;
  }

  static mcx::compute_status
  infer(const operation_type &operation, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>> inputs,
        micron::raw_slice<mcx::tensor_descriptor<dtype_type>> outputs) noexcept
  {
    if ( inputs.is_empty() || outputs.size() != 1 ) return mcx::compute_status::invalid_plan;
    outputs[0] = inputs[0];
    if ( operation.elements != 0 ) {
      outputs[0].dimensions[0] = operation.elements;
      outputs[0].byte_size = 0;
      outputs[0].alignment = alignof(i32);
    }
    outputs[0].access = mcx::tensor_access::read_write;
    return mcx::compute_status::ok;
  }

  static mcx::compute_status
  validate(const operation_type &operation, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>> inputs,
           micron::raw_slice<mcx::tensor_descriptor<dtype_type>> outputs) noexcept
  {
    const usize wanted = operation.opcode == test_opcode::add ? 2 : 1;
    return inputs.size() == wanted && outputs.size() == 1 ? mcx::compute_status::ok : mcx::compute_status::invalid_plan;
  }

  static mcx::compute_status
  aliases(const operation_type &operation, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>>,
          micron::raw_slice<const mcx::tensor_descriptor<dtype_type>>, micron::raw_slice<mcx::alias_declaration> aliases) noexcept
  {
    if ( operation.opcode == test_opcode::view ) aliases[0] = { mcx::alias_mode::read_only_view, 0, operation.offset };
    if ( operation.opcode == test_opcode::increment ) aliases[0] = { mcx::alias_mode::destructive_in_place, 0, operation.offset };
    return mcx::compute_status::ok;
  }

  static mcx::compute_status
  execute(const operation_type &operation, context_type &context, micron::raw_slice<const mcx::tensor_view<dtype_type>> inputs,
          micron::raw_slice<mcx::tensor_view<dtype_type>> outputs) noexcept
  {
    context.calls.fetch_add(1, micron::memory_order_relaxed);
    if ( operation.opcode == test_opcode::fail ) return mcx::compute_status::kernel_failure;
    const usize count = outputs[0].descriptor->byte_size / sizeof(i32);
    const i32 *left = inputs[0].template as<i32>();
    i32 *output = outputs[0].template as<i32>();
    if ( operation.opcode == test_opcode::view )
      return reinterpret_cast<const byte *>(output) == reinterpret_cast<const byte *>(left) + operation.offset
                 ? mcx::compute_status::ok
                 : mcx::compute_status::kernel_failure;
    if ( operation.opcode == test_opcode::increment ) {
      for ( usize i = 0; i < count; ++i ) ++output[i];
      return mcx::compute_status::ok;
    }
    if ( operation.opcode == test_opcode::copy ) {
      for ( usize i = 0; i < count; ++i ) output[i] = left[i];
      return mcx::compute_status::ok;
    }
    const i32 *right = inputs[1].template as<i32>();
    for ( usize i = 0; i < count; ++i ) output[i] = left[i] + right[i];
    return mcx::compute_status::ok;
  }
};

static mcx::tensor_descriptor<test_dtype>
descriptor(mcx::tensor_access access = mcx::tensor_access::read_only)
{
  mcx::tensor_descriptor<test_dtype> result;
  result.dtype = test_dtype::i32;
  result.dimensions.push_back(4);
  result.access = access;
  return result;
}

int
main()
{
  test_case("borrowed inputs and retained output");
  {
    mcx::compute_graph<test_registry> builder;
    const auto left = builder.add_input(descriptor());
    const auto right = builder.add_input(descriptor());
    const mcx::tensor_id inputs[]{ left, right };
    const auto output = builder.add_one({ test_opcode::add }, micron::raw_slice<const mcx::tensor_id>(inputs, 2));
    builder.retain(output);
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 a[]{ 1, 2, 3, 4 };
    alignas(micron::max_align_t) i32 b[]{ 5, 6, 7, 8 };
    require_true(session.bind_borrowed(left, a, descriptor()) == mcx::compute_status::ok);
    require_true(session.bind_borrowed(right, b, descriptor()) == mcx::compute_status::ok);
    const auto execution = session.execute();
    require_true(execution.succeeded());
    auto *values = static_cast<i32 *>(session.data(output));
    require_true(values && values[0] == 6 && values[1] == 8 && values[2] == 10 && values[3] == 12);
    auto exported = session.export_tensor(output);
    require_true(exported && exported.data() == values);
  }
  end_test_case();

  test_case("borrowed view aliases preserve pointer identity");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    const mcx::tensor_id inputs[]{ input };
    const auto output = builder.add_one({ test_opcode::view }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    builder.retain(output);
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 1, 2, 3, 4 };
    require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
    require_true(session.execute().succeeded());
    require_true(session.data(output) == values);
    require_true(!session.export_tensor(output));
  }
  end_test_case();

  test_case("alias views carry offset and static stride metadata");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    const mcx::tensor_id inputs[]{ input };
    const auto output = builder.add_one({ test_opcode::view, 2, sizeof(i32) }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    auto plan = builder.finalize();
    require_true(plan.descriptor(output)->dimensions[0] == 2);
    require_true(plan.descriptor(output)->strides[0] == static_cast<ssize_t>(sizeof(i32)));
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 41, 43, 47, 53 };
    require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
    require_true(session.execute().succeeded());
    require_true(session.data(output) == values + 1);
  }
  end_test_case();

  test_case("destructive aliases wait for earlier readers");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor(mcx::tensor_access::read_write));
    const mcx::tensor_id inputs[]{ input };
    const auto snapshot = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    const auto incremented = builder.add_one({ test_opcode::increment }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    const mcx::tensor_id follower_inputs[]{ incremented };
    const auto follower = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(follower_inputs, 1));
    builder.retain(snapshot);
    builder.retain(follower);
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 3, 5, 7, 9 };
    require_true(session.bind_borrowed(input, values, descriptor(mcx::tensor_access::read_write)) == mcx::compute_status::ok);
    require_true(session.execute().succeeded());
    const auto *copied = static_cast<const i32 *>(session.data(snapshot));
    require_true(copied && copied[0] == 3 && copied[1] == 5 && copied[2] == 7 && copied[3] == 9);
    require_true(session.data(incremented) == values);
    require_true(values[0] == 4 && values[1] == 6 && values[2] == 8 && values[3] == 10);
    const auto *followed = static_cast<const i32 *>(session.data(follower));
    require_true(followed && followed[0] == 4 && followed[1] == 6 && followed[2] == 8 && followed[3] == 10);
  }
  end_test_case();

  test_case("persistent state survives reset and hard reset clears it");
  {
    mcx::compute_graph<test_registry> builder;
    const auto state = builder.add_state(descriptor(mcx::tensor_access::read_write));
    const mcx::tensor_id inputs[]{ state };
    const auto updated = builder.add_one({ test_opcode::increment }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    require_true(session.execute().succeeded());
    auto *values = static_cast<i32 *>(session.data(state));
    require_true(values && values[0] == 1 && session.data(updated) == values);
    session.reset();
    require_true(session.execute().succeeded());
    require_true(values[0] == 2);
    auto exported = session.export_tensor(state);
    require_true(exported && exported.data() == values);
    session.hard_reset();
    require_true(session.execute().succeeded());
    values = static_cast<i32 *>(session.data(state));
    require_true(values && values[0] == 1);
  }
  end_test_case();

  test_case("arena slots are reused after the last consumer");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    const mcx::tensor_id first_inputs[]{ input };
    const auto first = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(first_inputs, 1));
    const mcx::tensor_id second_inputs[]{ first };
    const auto second = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(second_inputs, 1));
    const mcx::tensor_id third_inputs[]{ second };
    const auto third = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(third_inputs, 1));
    const mcx::tensor_id fourth_inputs[]{ third };
    const auto output = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(fourth_inputs, 1));
    builder.retain(output);
    auto plan = builder.finalize();
    require_true(plan.slots_count() == 3);
    require_true(plan.peak_bytes() == sizeof(i32) * 12);
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 8, 6, 4, 2 };
    require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
    require_true(session.execute().succeeded());
    const auto *copied = static_cast<const i32 *>(session.data(output));
    require_true(copied && copied[0] == 8 && copied[1] == 6 && copied[2] == 4 && copied[3] == 2);
  }
  end_test_case();

  test_case("owned outputs bind across sessions without copying");
  {
    mcx::compute_graph<test_registry> producer_builder;
    const auto producer_input = producer_builder.add_input(descriptor());
    const mcx::tensor_id producer_inputs[]{ producer_input };
    const auto producer_output
        = producer_builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(producer_inputs, 1));
    producer_builder.retain(producer_output);
    auto producer_plan = producer_builder.finalize();
    mcx::compute_session<test_registry> producer(producer_plan);
    alignas(micron::max_align_t) i32 values[]{ 11, 13, 17, 19 };
    require_true(producer.bind_borrowed(producer_input, values, descriptor()) == mcx::compute_status::ok);
    require_true(producer.execute().succeeded());
    auto shared = producer.export_tensor(producer_output);
    require_true(shared && shared.data() == producer.data(producer_output));

    mcx::compute_graph<test_registry> consumer_builder;
    const auto consumer_input = consumer_builder.add_input(descriptor());
    const mcx::tensor_id consumer_inputs[]{ consumer_input };
    const auto consumer_output
        = consumer_builder.add_one({ test_opcode::view }, micron::raw_slice<const mcx::tensor_id>(consumer_inputs, 1));
    auto consumer_plan = consumer_builder.finalize();
    mcx::compute_session<test_registry> consumer(consumer_plan);
    require_true(consumer.bind_shared(consumer_input, shared) == mcx::compute_status::ok);
    require_true(consumer.execute().succeeded());
    require_true(consumer.data(consumer_input) == shared.data());
    require_true(consumer.data(consumer_output) == shared.data());
  }
  end_test_case();

  test_case("exported storage outlives its producing session");
  {
    mcx::shared_tensor<test_dtype> exported;
    const void *address = nullptr;
    {
      mcx::compute_graph<test_registry> builder;
      const auto input = builder.add_input(descriptor());
      const mcx::tensor_id inputs[]{ input };
      const auto output = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
      builder.retain(output);
      auto plan = builder.finalize();
      mcx::compute_session<test_registry> session(plan);
      alignas(micron::max_align_t) i32 values[]{ 23, 29, 31, 37 };
      require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
      require_true(session.execute().succeeded());
      exported = session.export_tensor(output);
      address = exported.data();
      require_true(exported && address != nullptr);
    }
    const auto *values = static_cast<const i32 *>(exported.data());
    require_true(exported.data() == address && values[0] == 23 && values[3] == 37);
  }
  end_test_case();

  test_case("bindings require exact canonical shape and strides");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 1, 2, 3, 4 };
    auto incompatible = descriptor();
    incompatible.dimensions[0] = 2;
    require_true(session.bind_borrowed(input, values, incompatible) == mcx::compute_status::incompatible_binding);
    incompatible = descriptor();
    incompatible.strides.push_back(sizeof(i32) * 2);
    incompatible.byte_size = sizeof(i32) * 7;
    require_true(session.bind_borrowed(input, values, incompatible) == mcx::compute_status::incompatible_binding);
  }
  end_test_case();

  test_case("shared tensors preserve negative-stride logical origins");
  {
    auto reversed = descriptor(mcx::tensor_access::read_write);
    reversed.strides.push_back(-static_cast<ssize_t>(sizeof(i32)));
    reversed.byte_size = sizeof(i32) * 4;
    reversed.alignment = alignof(i32);

    mcx::compute_graph<test_registry> producer_builder;
    const auto state = producer_builder.add_state(reversed);
    auto producer_plan = producer_builder.finalize();
    mcx::compute_session<test_registry> producer(producer_plan);
    auto shared = producer.export_tensor(state);
    require_true(shared && shared.storage.size() == sizeof(i32) * 4);
    require_true(shared.offset_bytes == sizeof(i32) * 3);

    mcx::compute_graph<test_registry> consumer_builder;
    const auto input = consumer_builder.add_input(reversed);
    auto consumer_plan = consumer_builder.finalize();
    mcx::compute_session<test_registry> consumer(consumer_plan);
    require_true(consumer.bind_shared(input, shared) == mcx::compute_status::ok);
    require_true(consumer.data(input) == shared.data());
    require_true(consumer.execute().succeeded());
  }
  end_test_case();

  test_case("zero-sized owned tensors export and bind without pointer arithmetic");
  {
    auto empty = descriptor(mcx::tensor_access::read_write);
    empty.dimensions[0] = 0;
    mcx::compute_graph<test_registry> producer_builder;
    const auto state = producer_builder.add_state(empty);
    auto producer_plan = producer_builder.finalize();
    mcx::compute_session<test_registry> producer(producer_plan);
    auto shared = producer.export_tensor(state);
    require_true(shared && shared.storage.size() == 0 && shared.data() == nullptr);

    mcx::compute_graph<test_registry> consumer_builder;
    const auto input = consumer_builder.add_input(empty);
    auto consumer_plan = consumer_builder.finalize();
    mcx::compute_session<test_registry> consumer(consumer_plan);
    require_true(consumer.bind_shared(input, shared) == mcx::compute_status::ok);
    require_true(consumer.data(input) == nullptr);
    require_true(consumer.execute().succeeded());
  }
  end_test_case();

  test_case("concurrent arena executes fan-out and fan-in plans repeatedly");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    const mcx::tensor_id source[]{ input };
    mcx::tensor_id branch[4];
    for ( usize i = 0; i < 4; ++i ) branch[i] = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(source, 1));
    const mcx::tensor_id left_inputs[]{ branch[0], branch[1] };
    const mcx::tensor_id right_inputs[]{ branch[2], branch[3] };
    const auto left = builder.add_one({ test_opcode::add }, micron::raw_slice<const mcx::tensor_id>(left_inputs, 2));
    const auto right = builder.add_one({ test_opcode::add }, micron::raw_slice<const mcx::tensor_id>(right_inputs, 2));
    const mcx::tensor_id final_inputs[]{ left, right };
    const auto output = builder.add_one({ test_opcode::add }, micron::raw_slice<const mcx::tensor_id>(final_inputs, 2));
    builder.retain(output);
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 2, 3, 5, 7 };
    require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
    micron::concurrent_arena arena;
    (void)arena.create();
    (void)arena.create();
    mcx::thread_pool_executor executor(arena);
    for ( usize iteration = 0; iteration < 64; ++iteration ) {
      require_true(session.execute(executor).succeeded());
      const auto *sum = static_cast<const i32 *>(session.data(output));
      require_true(sum && sum[0] == 8 && sum[1] == 12 && sum[2] == 20 && sum[3] == 28);
      session.reset();
    }
    require_true(session.context().calls.get(micron::memory_order_relaxed) == 64 * 7);
  }
  end_test_case();

  test_case("parallel failures select the earliest plan operation");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    const mcx::tensor_id inputs[]{ input };
    const auto first = builder.add_one({ test_opcode::fail }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    const auto second = builder.add_one({ test_opcode::fail }, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
    builder.retain(first);
    builder.retain(second);
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 0, 0, 0, 0 };
    require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
    micron::concurrent_arena arena;
    (void)arena.create();
    (void)arena.create();
    mcx::thread_pool_executor executor(arena);
    const auto execution = session.execute(executor);
    require_true(execution.status == mcx::compute_status::kernel_failure);
    require_true(execution.failing_operation == mcx::operation_id{ 0 });
    require_true(session.context().calls.get(micron::memory_order_relaxed) == 2);
  }
  end_test_case();

  test_case("kernel failure stops dependent work and reports the first operation");
  {
    mcx::compute_graph<test_registry> builder;
    const auto input = builder.add_input(descriptor());
    const mcx::tensor_id failing_inputs[]{ input };
    const auto failed = builder.add_one({ test_opcode::fail }, micron::raw_slice<const mcx::tensor_id>(failing_inputs, 1));
    const mcx::tensor_id dependent_inputs[]{ failed };
    const auto dependent = builder.add_one({ test_opcode::copy }, micron::raw_slice<const mcx::tensor_id>(dependent_inputs, 1));
    builder.retain(dependent);
    auto plan = builder.finalize();
    mcx::compute_session<test_registry> session(plan);
    alignas(micron::max_align_t) i32 values[]{ 1, 1, 1, 1 };
    require_true(session.bind_borrowed(input, values, descriptor()) == mcx::compute_status::ok);
    const auto execution = session.execute();
    require_true(execution.status == mcx::compute_status::kernel_failure);
    require_true(execution.failing_operation == mcx::operation_id{ 0 });
    require_true(session.context().calls.get(micron::memory_order_relaxed) == 1);
    require_true(session.data(dependent) == nullptr);
  }
  end_test_case();

  return 1;
}
