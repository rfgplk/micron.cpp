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
  mc::io::stdoutln("New pair test of <int,int>");
  mc::io::stdoutln("First value is: ", p.a);
  mc::io::stdoutln("Second value is: ", p.b);
  mc::pair<int, float> r = { 1, 4.4f };
  mc::io::stdoutln("New pair test of <int,float>");
  mc::io::stdoutln("First value is: ", r.a);
  mc::io::stdoutln("Second value is: ", r.b);
  mc::pair<int, float> f = { 100, 55.65f };
  mc::io::stdoutln("New pair test of <int,float>");
  mc::io::stdoutln("First value is: ", f.a);
  mc::io::stdoutln("Second value is: ", f.b);
  f = 70.1f;
  f = 20;
  mc::io::stdoutln("First value after assign. is: ", f.a);
  mc::io::stdoutln("Second value after assign. is: ", f.b);
  mc::io::stdoutln("Copy pair test of <int,float>");
  mc::pair<int, float> c(f);
  mc::io::stdoutln("First value after copy. is: ", c.a);
  mc::io::stdoutln("Second value after copy. is: ", c.b);
  mc::io::stdoutln("First value after org. copy. is: ", f.a);
  mc::io::stdoutln("Second value after org. copy. is: ", f.b);
  mc::io::stdoutln("Move pair test of <int,float>");
  mc::pair<int, float> m(mc::move(f));
  mc::io::stdoutln("First value after org. move. is: ", f.a);
  mc::io::stdoutln("Second value after org. move. is: ", f.b);
  mc::io::stdoutln("First value after move. is: ", m.a);
  mc::io::stdoutln("Second value after move. is: ", m.b);
  mc::pair<int, float> n(m.get());
  mc::io::stdoutln("First value after get() is: ", n.a);
  mc::io::stdoutln("Second value after get() is: ", n.b);
  mc::pair<mc::string, mc::string> s = { "Element A!", "Element B!" };
  mc::io::stdoutln("First value is: ", s.a);
  mc::io::stdoutln("Second value is: ", s.b);
  mc::vector<mc::pair<float, bool>> vecpair;
  for ( auto i = 0; i < 20; i++ )
    vecpair.emplace_back(mc::tie(5.33f, false));
  for ( auto rtt : vecpair )
    mc::io::stdoutln(rtt.a, ", ", rtt.b);
};
