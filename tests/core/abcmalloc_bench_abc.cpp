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
    std::uniform_int_distribution<int> dist(1, 1e6);
    for ( size_t n = 0; n < 5000; ++n ) { //1-5gb
      void *dont_optimize = abc::malloc(dist(gen));
      escaped = dont_optimize;
    }
  }
  return 0;
}
