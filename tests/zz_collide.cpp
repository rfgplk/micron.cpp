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

#include "../src/sort/quick.hpp"

double
count_equiv(mc::slice<u64> &vec)
{
  mc::sort::quick<mc::slice<u64>>(vec.begin(), vec.end());

  u64 total_pairs = 0;
  u64 run = 1;
  for ( size_t i = 1; i < vec.size(); i++ ) {
    if ( vec[i] == vec[i - 1] ) {
      run++;
    } else {
      total_pairs += run * (run - 1) / 2;
      run = 1;
    }
  }
  total_pairs += run * (run - 1) / 2;

  u64 n = vec.size();
  u64 total_possible_pairs = n * (n - 1) / 2;
  return static_cast<double>(total_pairs) / total_possible_pairs * 100.0;
}

int
main()
{
  constexpr u64 iters = 100'000;
  enable_scope()
  {
    mc::slice<u64> hashes(iters);
    {
      alignas(8) byte str[32] = {};
      for ( usize i = 0; i < iters; ++i ) {
        hashes[i] = mc::zz64(reinterpret_cast<const byte *>(str), 123, 32);
        for ( i32 j = 0; j < 8; ++j ) {
          str[j] = ((i) >> (j * 8)) & 0xFF;
        }
      }
      hashes.mark(iters);
      mc::console("for zeroed = ", count_equiv(hashes));
    }
    for ( u64 k = 1; k < 65; ++k ) {
      alignas(8) byte str[32] = {};
      for ( usize i = 0; i < iters; ++i ) {
        hashes[i] = mc::zz64(reinterpret_cast<const byte *>(str), 123, 32);
        // mc::console(hashes[i]);
        for ( i32 j = 0; j < 8; ++j ) {
          str[j] = ((i * k) >> (j * 8)) & 0xFF;
          str[j + 8] = ((i * k) >> (j * 8)) & 0xFF;
          str[j + 16] = ((i * k) >> (j * 8)) & 0xFF;
          str[j + 24] = ((i * k) >> (j * 8)) & 0xFF;
        }
      }
      hashes.mark(iters);
      mc::console("for k: ", k, " = ", count_equiv(hashes));
    }
  };
}
