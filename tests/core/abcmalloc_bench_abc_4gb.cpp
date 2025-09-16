#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

void *volatile escaped;
int
main()
{
  char *dont_optimize = reinterpret_cast<char*>(abc::malloc(1ULL << 32));
  escaped = dont_optimize;
  mc::console(escaped);
  //mc::io::print("\n");
  //mc::io::print((const char)dont_optimize[345456]);
  //mc::io::print("\n");
  //mc::io::print((const char)dont_optimize[456]);
  //mc::io::print("\n");
  return 0;
}
