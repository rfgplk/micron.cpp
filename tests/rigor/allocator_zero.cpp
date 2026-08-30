//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/allocator.hpp"
#include "../../src/memory/allocation/allocator_types/constrained_allocator.hpp"

#include "../snowball/snowball.hpp"

namespace
{

template<typename Alloc>
void
require_zero_allocation()
{
  micron::chunk<byte> memory = Alloc::create(0);
  snowball::require(memory.ptr, static_cast<byte *>(nullptr));
  snowball::require(memory.len, usize{ 0 });
  Alloc::destroy(memory);
}

template<typename Alloc>
void
require_nonzero_allocation(usize requested, usize expected)
{
  micron::chunk<byte> memory = Alloc::create(requested);
  snowball::require(memory.ptr != nullptr, true);
  snowball::require(memory.len >= expected, true);
  Alloc::destroy(memory);
}

};      // namespace

int
main()
{
  using namespace snowball;

  test_case("zero requests remain zero through granularity rounding");
  {
    require(micron::to_granularity<512>(0), usize{ 0 });
    require(micron::to_granularity<512>(1), usize{ 512 });
    require(micron::to_granularity<512>(511), usize{ 512 });
    require(micron::to_granularity<512>(512), usize{ 512 });
    require(micron::to_granularity<512>(513), usize{ 1024 });
  }
  end_test_case();

  test_case("zero allocator requests return the canonical empty chunk");
  {
    require_zero_allocation<micron::allocator_serial<>>();
    require_zero_allocation<micron::allocator_small<>>();
    require_zero_allocation<micron::allocator_constrained<>>();
    require_zero_allocation<micron::map_allocator<>>();
  }
  end_test_case();

  test_case("nonzero allocator requests retain their sizing policy");
  {
    require_nonzero_allocation<micron::allocator_serial<>>(1, micron::page_size);
    require_nonzero_allocation<micron::allocator_small<>>(1, micron::small_allocation_policy::granularity);
    require_nonzero_allocation<micron::allocator_constrained<>>(1, micron::constrained_allocation_policy::granularity);
    require_nonzero_allocation<micron::map_allocator<>>(1, micron::page_size);
  }
  end_test_case();

  sb::print("=== ALLOCATOR ZERO-SIZE TESTS PASSED ===");
  return 1;
}
