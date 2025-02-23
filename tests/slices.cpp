#include "../src/slice.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"
#include "../src/hash/murmur.hpp"
int
main(void)
{
  mc::slice<float> sl;
  auto b = sl[1000, 2000];
  auto c = sl[];
  mc::console(sl[0]);
  return 1;
}
