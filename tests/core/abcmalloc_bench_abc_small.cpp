#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

void *volatile escaped;
#include <random>
int
main()
{
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 255);
    for ( size_t n = 0; n < 2e7; ++n ) { 
      void *dont_optimize = abc::malloc(dist(gen));
      escaped = dont_optimize;
    }
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1e4);
    for ( size_t n = 0; n < 1e7; ++n ) {
      void *dont_optimize = abc::launder(dist(gen));
      escaped = dont_optimize;
    }
  }
  return 0;
}
