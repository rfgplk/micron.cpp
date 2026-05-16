#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "../src/vector/vector.hpp"

int
main()
{
  mc::vector<int> bla(202, 10);
  mc::console(1.1f);
  mc::console((double)1.1f);
  for ( int i{}; i < 3; ++i ) micron::console(i);
  micron::console("...");
  for ( int i{}; i < 3; i++ ) micron::console(i);
  mc::console(bla);
}
