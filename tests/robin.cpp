
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/maps/robin.hpp"
#include "../src/io/console.hpp"
#include "../src/map.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/string/unistring.hpp"

#include "snowball/snowball.hpp"

int
main(void)
{
  mc::robin_map<mc::string, int> robin;
  robin.insert("hello", 100);
  robin.insert("world!", 200);
  mc::console(*robin.find("hello"));

  mc::robin_map<mc::string, int> robin_stress(10000);
  for ( u64 i; i < 1000; ++i ){
    mc::console(i);
    robin_stress.insert(mc::to_string(i + 1143), i * 2 >> 2);
  }
  mc::console(*robin_stress.find("1144"));
  mc::console(*robin_stress.find("1145"));
  mc::console(*robin_stress.find("1146"));
  return 0;
}
