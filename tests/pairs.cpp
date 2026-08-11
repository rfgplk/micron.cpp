//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/stdout.hpp"

#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/tuple.hpp"
#include "../src/vector/vector.hpp"

int
main(void)
{
  mc::pair<int, int> p = { (int)1, (int)2 };
  mc::io::print("New pair test of <int,int>", '\n');
  mc::io::print("First value is: ", p.a, '\n');
  mc::io::print("Second value is: ", p.b, '\n');
  mc::pair<int, float> r = { 1, 4.4f };
  mc::io::print("New pair test of <int,float>", '\n');
  mc::io::print("First value is: ", r.a, '\n');
  mc::io::print("Second value is: ", r.b, '\n');
  mc::pair<int, float> f = { 100, 55.65f };
  mc::io::print("New pair test of <int,float>", '\n');
  mc::io::print("First value is: ", f.a, '\n');
  mc::io::print("Second value is: ", f.b, '\n');
  f = 70.1f;
  f = 20;
  mc::io::print("First value after assign. is: ", f.a, '\n');
  mc::io::print("Second value after assign. is: ", f.b, '\n');
  mc::io::print("Copy pair test of <int,float>", '\n');
  mc::pair<int, float> c(f);
  mc::io::print("First value after copy. is: ", c.a, '\n');
  mc::io::print("Second value after copy. is: ", c.b, '\n');
  mc::io::print("First value after org. copy. is: ", f.a, '\n');
  mc::io::print("Second value after org. copy. is: ", f.b, '\n');
  mc::io::print("Move pair test of <int,float>", '\n');
  mc::pair<int, float> m(mc::move(f));
  mc::io::print("First value after org. move. is: ", f.a, '\n');
  mc::io::print("Second value after org. move. is: ", f.b, '\n');
  mc::io::print("First value after move. is: ", m.a, '\n');
  mc::io::print("Second value after move. is: ", m.b, '\n');
  mc::pair<int, float> n(m.get());
  mc::io::print("First value after get() is: ", n.a, '\n');
  mc::io::print("Second value after get() is: ", n.b, '\n');
  mc::pair<mc::string, mc::string> s = { "Element A!", "Element B!" };
  mc::io::print("First value is: ", s.a, '\n');
  mc::io::print("Second value is: ", s.b, '\n');
  mc::vector<mc::pair<float, bool>> vecpair;
  // was mc::tie(5.33f, false): tie() takes an initializer_list or lvalue refs, and returns a
  // tuple, not a pair -- this line had not compiled in a long time
  for ( auto i = 0; i < 20; i++ ) vecpair.emplace_back(mc::pair<float, bool>(5.33f, false));
  for ( auto rtt : vecpair ) mc::io::print(rtt.a, ", ", rtt.b, '\n');
  return 1;
};
