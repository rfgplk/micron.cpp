
#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include <random>
void *volatile escaped;
int
main()
{
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    for ( size_t n = 0; n < 10000; ++n ) {
      void *dont_optimize = abc::malloc(65536);
      escaped = dont_optimize;
    }
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    for ( size_t n = 0; n < (2 << 8); ++n ) {
      abc::malloc(dist(gen));     // oom checking
    }
    mc::infolog("Success");
  }
  return 0;
}
