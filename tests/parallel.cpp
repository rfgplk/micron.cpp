#include "../src/io/console.hpp"
#include "../src/parallel/for.hpp"
#include "../src/std.hpp"
#include "../src/vector/vector.hpp"

#include "snowball/snowball.hpp"

int
main()
{
  enable_scope()
  {
    mc::vector<u64> vec(1uLL << 16);
    for ( auto &n : vec )
      n = 1;
    mc::console(vec[10]);
    mc::console(vec[100]);
    mc::console(vec[1000]);
    mc::console(vec[10000]);
    for ( auto &n : vec )
      if ( n != 1 )
        mc::cerror("Wasn't equal to one");
  };
  disable_scope()
  {
    mc::vector<u64> vec(1uLL << 24);
    // mc::parallel_for<[](decltype(vec)::iterator x) { *x = 1; }>(vec);
    mc::fork_for<[](decltype(vec)::iterator x) { *x = 1; }>(vec);
    for ( auto &n : vec )
      if ( n != 1 )
        mc::cerror("Wasn't equal to one");
    mc::fork_for<[](decltype(vec)::iterator x) { *x = 5; }>(vec);
    for ( auto &n : vec )
      if ( n != 5 )
        mc::cerror("Wasn't equal to 5");
     mc::fork_for<[](decltype(vec)::iterator x) { *x = 10; }>(vec);
    for ( auto &n : vec )
      if ( n != 10 )
        mc::cerror("Wasn't equal to 10");
 
  };
  return 0;
}
