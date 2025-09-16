#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

void *volatile escaped;
#include <random>
int
main()
{
  if constexpr ( true ) {
    for ( size_t n = 0; n < 1000; ++n ) {
      void *dont_optimize = abc::malloc(1024);
      escaped = dont_optimize;
      abc::free(dont_optimize);
    }
  }
  return 0;
}
