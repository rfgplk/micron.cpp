//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/hash/xx.hpp"
#include "../src/hash/zzz.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

int
main()
{
  enable_scope()
  {
    alignas(8) byte str[32] = {};
    for ( u64 i = 0; i < 32; ++i )
      str[i] = i;
    volatile u64 out = 0;
    // 32GiB
    for ( usize i = 0; i < 1'000'000'000; ++i )
      out = mc::hashes::z64(reinterpret_cast<const byte *>(&str), i, 32);
    mc::console(static_cast<u64>(out));
  };
}
