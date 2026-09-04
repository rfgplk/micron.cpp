//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::resources -- the sysinfo(2) snapshot abcmalloc's OOM policy reads
// (src/memory/allocation/abcmalloc/oom.hpp:43-44).

#include "../../src/memory/allocation/abcmalloc/oom.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  test_case("resources reports a live memory snapshot");
  {
    mc::resources rs;
    require_true(rs.total_memory > 0);
    require_true(rs.free_memory <= rs.total_memory);
    require_true(rs.memory <= rs.total_memory);
    require_true(rs.free_swap <= rs.total_swap);
    require_true(rs.procs > 0);
    require_true(rs.mem_unit > 0);
    sb::print("total=", rs.total_memory, " free=", rs.free_memory, " procs=", rs.procs);
  }
  end_test_case();

  test_case("a second snapshot agrees on the invariants");
  {
    mc::resources a;
    mc::resources b;
    // total ram and the unit scale do not move under us; free ram may
    require_true(a.total_memory == b.total_memory);
    require_true(a.mem_unit == b.mem_unit);
    require_true(b.free_memory <= b.total_memory);
  }
  end_test_case();

  sb::print("=== ALL RESOURCES TESTS PASSED ===");
  return 1;
}
