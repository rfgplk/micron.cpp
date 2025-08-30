#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include <random>
int
main()
{
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    for ( size_t n = 0; n < 100000; ++n ) {
      abc::malloc(dist(gen));
    }
  }
  return 0;
}
