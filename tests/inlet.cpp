#include "../src/sync/inlet.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/vector/vector.hpp"

#include "../src/io/console.hpp"

#include "snowball/snowball.hpp"

int
main()
{
  enable_scope()
  {
    mc::vector<int> a { 1, 2, 3, 4 };
    mc::spin_inlet<mc::vector<int>> inlet("test");
    mc::console(inlet.tag());
    inlet.store(a);
    mc::console(inlet.access().get());
  };
  return 0;
}
