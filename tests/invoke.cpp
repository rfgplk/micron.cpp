
#include "../src/io/console.hpp"
#include "../src/sync/invoke.hpp"
#include "../src/std.h"

void fn(int x)
{
  mc::console(x);
}

int
main(void)
{
  mc::invoke(fn, 2);
  return 0;
}
