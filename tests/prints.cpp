#include <micron/std.hpp>
#include <micron/io/console.hpp>

int
main()
{
  for ( int i{}; i < 3; ++i )
    micron::console(i);
  micron::console("...");
  for ( int i{}; i < 3; i++ )
    micron::console(i);
}
