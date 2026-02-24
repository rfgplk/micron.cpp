//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/filter.hpp"
#include "../src/algorithm/math.hpp"
#include "../src/algorithm/data.hpp"

#include "../src/io/console.hpp"
#include "../src/iterator.hpp"
#include "../src/numerics.hpp"
#include "../src/std.hpp"
#include "../src/sync/promises.hpp"
#include "../src/vector/vector.hpp"

int
main(void)
{
  mc::vector<u64> v(10);
  u64 i = 0;
  mc::generate(v, [](u64& x) { return x+=20; }, i);
  mc::make_heap(v);
  mc::console(mc::is_heap(v));
  mc::console(v);
  mc::pop_heap(v);
  mc::console(mc::is_heap(v));
  mc::console(v);
  return 1;
}
