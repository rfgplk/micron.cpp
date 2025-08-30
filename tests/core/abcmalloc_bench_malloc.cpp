
#include "../../src/allocation/abcmalloc/arena.hpp"
#include "../../src/allocation/abcmalloc/book.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

void *volatile escaped;

#include <cstdlib>
#include <random>
int
main()
{
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    abc::__arena arena;
    for ( size_t n = 0; n < 100000; ++n ) {
      void *dont_optimize = std::malloc(dist(gen));
      escaped = dont_optimize;
    }
  }
  return 0;
}
