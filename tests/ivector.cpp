
#include "../src/vector/ivector.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"
int
main(void)
{
  mc::ivector<float> f(500, 1.0f);
  for(auto i = 0; i < 50; i++)
    mc::console(f[i]);
}
