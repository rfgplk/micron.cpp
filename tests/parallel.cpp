#include "../src/parallel/for.hpp"
#include "../src/std.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  mc::vector<u64> vec(1uLL << 30);
  mc::parallel_for<[](decltype(vec)::iterator x) { *x = 1; }>(vec);
  mc::ssleep(1);
  mc::console(vec[10]);
  mc::console(vec[100]);
  mc::console(vec[1000]);
  mc::console(vec[10000]);
  return 0;
}
