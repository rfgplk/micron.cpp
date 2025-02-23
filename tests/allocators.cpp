#include "../src/io/console.hpp"
#include "../src/std.h"
#include "../src/string/strings.h"

#include "../src/allocation/system_rs.hpp"

#include <string>

int
main()
{
  mc::hstring<char, micron::allocator_tiny<>> tstr;
  mc::console("Starting size: ", tstr.size());
  mc::console("Starting capacity: ", tstr.max_size());
  for ( u32 i = 0; i < 1000000000; i++ )
  {
    tstr += "qwertyuiop";
  }
  mc::console("Ending size: ", tstr.size());
  mc::console("Ending capacity: ", tstr.max_size());
  return 0;
}
