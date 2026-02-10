//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/filter.hpp"
#include "../src/algorithm/math.hpp"
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
  v.fill(100);
  mc::console(mc::sum(v));
  mc::console(mc::find(v, 100));
  mc::console(mc::clear(v));
  mc::console(mc::find(v, 100));
  mc::console(mc::generate(v, [](int x) { return x; }, 5));
  mc::vector<float> vf(10);
  float i = 0;
  mc::console(mc::generate(vf, [](float& x) { return ++x; }, i));
  mc::console(mc::sin(vf));
}
