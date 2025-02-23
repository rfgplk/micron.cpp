#include "../src/errno.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"

int
main(void)
{
  mc::console(mc::error::const_get_errno<EPERM>());
  for ( int i = 0; i < 39; i++ )
    mc::console(mc::error::get_errno(i));
  return 0;
}
