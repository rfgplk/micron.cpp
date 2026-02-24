//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/array/arrays.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "snowball/snowball.hpp"

int
main()
{
  mc::carray<float, 2450> fl;
  mc::array<int, 20> arr(5);
  sb::require(arr[10] == 5);
  sb::require(arr.all(5) == true);
  //for ( auto &n : arr )
  //  mc::console(n);
  mc::conarray<float, 10> farr(4.4f);
  //for ( auto &n : farr )
  //  mc::console(n);
  sb::require(farr.view()[0] == 4.4f);
  farr += 10;
  sb::require(farr.view()[0] == 14.4f);
  sb::require(farr.all(0) == false);
  
  mc::bisect_array<int, 64> barr;
  barr.insert(10);
  barr.insert(5);
  barr.insert(22);
  barr.insert(4356);
  barr.insert(4);
  barr.insert(9);
  barr.insert(0);

  sb::require(barr[0] == 0);
  sb::require(barr[1] == 4);
  sb::require(barr[2] == 5);
  sb::require(barr[3] == 9);
  sb::require(barr[4] == 10);
  sb::require(barr[5] == 22);
  //for ( auto &n : barr )
  //  mc::console(n);
  return 0;
};
