#include "linux/std.hpp"
#include "std.hpp"
// 232323 (why is this here???)
#include "recipe.hh"

#include "commands.hh"

int
main(int argc, char **argv)
{
  try {
    return parse_main(argc, argv);
  } catch ( ... ) {
    return 2;
  }
}
