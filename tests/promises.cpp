//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/sync/promises.hpp"
#include "../src/algorithm/algorithm.hpp"
#include "../src/io/console.hpp"
#include "../src/iterator.hpp"
#include "../src/vector/vector.hpp"
#include "../src/numerics.hpp"
#include "../src/std.hpp"

void success(long x){
  mc::console("The result of mc::mean(1..89) was ", x);
}

int
main(void)
{
  mc::console( mc::maximum::u32bit );
  mc::vector<long> vc_l(89);
  mc::vector<long> vc_s(50);
  mc::vector<long> vc(25);
  for ( size_t i = 0; i < vc_l.size(); i++ )
    vc_l[i] = i + 1;
  for ( size_t i = 0; i < vc_s.size(); i++ )
    vc_s[i] = i + 1;
  for ( size_t i = 0; i < vc.size(); i++ )
    vc[i] = i + 1;
  mc::console(vc);
  mc::console("The arith. mean of (1..25) is expected to be 13: ", mc::expect(mc::mean(vc), 13.0));
  mc::console("The arith. mean of (1..25) is not expected to be 12: ", mc::expect(mc::mean(vc), 12.0));
  mc::expect(mc::mean(vc_l), 45, success, 45);
  mc::console("The geo. mean of (1..50) is: ", mc::geomean(vc_s));
}
