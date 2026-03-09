//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/slice.hpp"

#include "../src/hash/xx.hpp"
#include "../src/hash/zzz.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "../src/io/posix/file.hpp"

int
main()
{
  constexpr u64 iters = 100'000;
  enable_scope()
  {
    auto rand = mc::io::virtual_file("/dev/random");
    mc::string str(1<<20);
    str.set_size(rand.read(str));

    alignas(32) u64 scratch[4] = {};
    for ( usize i = 0; i < iters; ++i ) {
      mc::zzz(&str, 1234 + i, str.size(), scratch);
    }
    mc::console(scratch);
  };
}
