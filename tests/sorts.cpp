//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/console.hpp"
#include "../src/sort/sorts.hpp"
#include "../src/vector/fvector.hpp"

#include "../src/std.hpp"

int
main(void)
{
  mc::fvector<float> vf = { 932421.0f, 2345.0f, 5.0f, 3452.0f, 0.1f, -3453.233f, 355.0f, 17.18f, 17.99f, 17.19f, 0.0f };
  mc::sort::fradix(vf);
  mc::console(vf);
  mc::fvector<u32> vec(10);
  u32 i = 3453463438;
  mc::generate(
      vec,
      [](u32 &x) {
        return x /= 2;
      },
      i);
  mc::sort::quick<mc::fvector<u32>>(vec.begin(), vec.end());
  mc::console(vec);
  return 1;
}
