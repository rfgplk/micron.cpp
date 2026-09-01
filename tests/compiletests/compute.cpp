//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/compute.hpp"
#include "../../src/math/compute/thread_pool.hpp"

namespace mcx = micron::math::compute;

enum class dtype : u8 { byte_v = 0 };

struct operation {
};

struct context {
};

struct registry {
  using operation_type = operation;
  using dtype_type = dtype;
  using context_type = context;

  static constexpr usize
  dtype_size(dtype_type) noexcept
  {
    return 1;
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
    if ( inputs.size() != 1 || outputs.size() != 1 ) return mcx::compute_status::invalid_plan;
    outputs[0] = inputs[0];
    return mcx::compute_status::ok;
  }

  static mcx::compute_status
  validate(const operation_type &, micron::raw_slice<const mcx::tensor_descriptor<dtype_type>>,
           micron::raw_slice<mcx::tensor_descriptor<dtype_type>>) noexcept
  {
    return mcx::compute_status::ok;
  }

  static mcx::compute_status
  execute(const operation_type &, context_type &, micron::raw_slice<const mcx::tensor_view<dtype_type>>,
          micron::raw_slice<mcx::tensor_view<dtype_type>>) noexcept
  {
    return mcx::compute_status::ok;
  }
};

static_assert(mcx::operation_registry<registry>);
static_assert(mcx::domain_provider<mcx::host_domains>);

int
main()
{
  mcx::compute_graph<registry> builder;
  mcx::tensor_descriptor<dtype> descriptor;
  descriptor.dimensions.push_back(16);
  const auto input = builder.add_input(descriptor);
  const mcx::tensor_id inputs[]{ input };
  const auto output = builder.add_one({}, micron::raw_slice<const mcx::tensor_id>(inputs, 1));
  builder.retain(output);
  auto plan = builder.finalize();
  mcx::compute_session<registry> session(plan);
  (void)session;
  return 1;
}
