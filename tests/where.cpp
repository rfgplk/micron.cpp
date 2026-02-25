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
  mc::vector<u32> w(10);
  mc::transform<[](u32 x) -> u32 { return x = 5; }>(w);
  mc::console(w);
  mc::transform<[](u32 *x) -> u32 { return *x = 10; }>(w);
  mc::console(w);
  mc::transform<[](u32 &x) -> u32 { return x *= 2; }>(w);
  mc::console(w);
  u64 y = 0;
  mc::vector<u64> v(100);
  mc::generate(v, [&]() -> u64 { return y++; });
  auto vec = mc::where(v, [](u64 x) -> bool { return x % 2 == 0; });
  mc::console(vec);
}
