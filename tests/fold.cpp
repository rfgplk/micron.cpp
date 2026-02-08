//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/filter.hpp"
#include "../src/algorithm/arith.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/io/console.hpp"
#include "../src/iterator.hpp"
#include "../src/numerics.hpp"
#include "../src/std.hpp"
#include "../src/sync/promises.hpp"
#include "../src/vector/vector.hpp"

int
main(void)
{
  mc::vector<u64> v(10, 2);
  mc::console(mc::sum(v));
  mc::console(mc::fold_left(v, 0, [](u64 x, const u64* val) { return x + *val; } ));
  mc::console(mc::fold(v, 100, [](u64 x, const u64* val) { return x + *val; } ));
  mc::console(mc::mul(v));
  mc::console(mc::fold_left(v, 0, [](u64 x, const u64* val) { return x * *val; } ));
  mc::console(mc::fold(v, 1, [](u64 x, const u64* val) { return x * *val; } ));
  mc::console(mc::fold(v, 2, [](u64 x, const u64* val) { return x * *val; } ));
}
