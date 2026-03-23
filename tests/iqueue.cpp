//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/queue/iqueue.hpp"
#include "../src/std.hpp"

int
main()
{
  sb::verify_debug();
  enable_scope()
  {
    mc::iqueue<int> qu;
    mc::iqueue<int> rst;
    for (u64 i : mc::u64_range<1, (u64)1e8>() )
      rst = mc::move(qu.push(i));
    mc::console(qu.empty());
  };
  return 0;
}
