#include <micron/io/console.hpp>
#include <micron/std.hpp>

int
main()
{
  mc::console(1.1f);
  mc::console((double)1.1f);
  for ( int i{}; i < 3; ++i ) micron::console(i);
  micron::console("...");
  for ( int i{}; i < 3; i++ ) micron::console(i);
}
