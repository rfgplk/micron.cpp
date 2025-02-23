
#include "../src/io/print.hpp"

#include "../src/std.h"
#include "../src/string/strings.h"
#include "../src/tuple.hpp"
#include "../src/vector/vector.hpp"

int
main(void)
{
  mc::pair<int, int> p = { (int)1, (int)2 };
  mc::io::println("New pair test of <int,int>");
  mc::io::println("First value is: ", p.a);
  mc::io::println("Second value is: ", p.b);
  mc::pair<int, float> r = { 1, 4.4f };
  mc::io::println("New pair test of <int,float>");
  mc::io::println("First value is: ", r.a);
  mc::io::println("Second value is: ", r.b);
  mc::pair<int, float> f = { 100, 55.65f };
  mc::io::println("New pair test of <int,float>");
  mc::io::println("First value is: ", f.a);
  mc::io::println("Second value is: ", f.b);
  f = 70.1f;
  f = 20;
  mc::io::println("First value after assign. is: ", f.a);
  mc::io::println("Second value after assign. is: ", f.b);
  mc::io::println("Copy pair test of <int,float>");
  mc::pair<int, float> c(f);
  mc::io::println("First value after copy. is: ", c.a);
  mc::io::println("Second value after copy. is: ", c.b);
  mc::io::println("First value after org. copy. is: ", f.a);
  mc::io::println("Second value after org. copy. is: ", f.b);
  mc::io::println("Move pair test of <int,float>");
  mc::pair<int, float> m(mc::move(f));
  mc::io::println("First value after org. move. is: ", f.a);
  mc::io::println("Second value after org. move. is: ", f.b);
  mc::io::println("First value after move. is: ", m.a);
  mc::io::println("Second value after move. is: ", m.b);
  mc::pair<int, float> n(m.get());
  mc::io::println("First value after get() is: ", n.a);
  mc::io::println("Second value after get() is: ", n.b);
  mc::pair<mc::string, mc::string> s = { "Element A!", "Element B!" };
  mc::io::println("First value is: ", s.a);
  mc::io::println("Second value is: ", s.b);
  mc::vector<mc::pair<float, bool>> vecpair;
  for ( auto i = 0; i < 20; i++ )
    vecpair.emplace_back(mc::tie(5.33f, false));
  for ( auto rtt : vecpair )
    mc::io::println(rtt.a, ", ", rtt.b);
};
