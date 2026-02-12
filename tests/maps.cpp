//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/map.hpp"
#include "../src/maps/hopscotch.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"

#include "snowball/snowball.hpp"

int
main(void)
{
  micron::exc<mc::except::memory_error>("test error");
  mc::stack_swiss_map<mc::string, int, 128> ms;
  ms.insert("bla", 1);
  mc::console(*ms.find("bla"));
  // std::unordered_map<mc::hash64_t, int> hmp;
  mc::hopscotch_map<mc::hash64_t, int> hmp(25000);
  for ( size_t i = 0; i < 10000; i++ ) {
    try {
      hmp.insert(i, 5 * i);
    } catch ( mc::except::library_error &e ) {
    }
  }
  mc::console(hmp[500]);
  mc::console("Done");
  mc::console("Done");
  mc::bmap<mc::string, float> bm;
  bm.insert("hi", 2.0f);
  bm.insert("bye", 5.0f);
  mc::console(bm.find("bb"));
  mc::console(*bm.find("hi"));
  return 1;
  mc::map<mc::string, int> mp;
  mc::console(mp.size());
  mc::console(mp.max_size());
  mp.emplace("hello", 1);
  mp.insert("hello", 1);
  mp.insert("hello", 1);
  mp.insert("cat", 1);
  mp.insert("cat", 1);
  mc::console(mp.count("hello"));
  mc::console(mp.count("cat"));
  return 0;
}
