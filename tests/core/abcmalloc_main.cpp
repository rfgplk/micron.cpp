
#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.h"

#include <random>
int
main()
{
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    for ( size_t n = 0; n < (2 << 8); ++n ) {
      abc::malloc(dist(gen));
    }
    mc::infolog("Success");
  }
  return 0;
}
