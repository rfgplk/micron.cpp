#include "../src/io/ftw.hpp"
#include "../src/std.h"

int
main(void)
{
  auto r = mc::io::ftw_all(".");
  for(auto& n : r)
    mc::console(n);
  return 0;
}
