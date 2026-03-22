#include "../src/maps/itable.hpp"
#include "../src/std.hpp"

#include "../src/io/console.hpp"

int
main()
{
  mc::immutable_table<u32, u32> tab;
  auto __t = tab.insert(1, 0);
  for ( u64 i = 0; i < (1 << 24); ++i )
    __t = tab.insert(1, i);
  mc::console(*__t.find(1));
  return 1;
}
