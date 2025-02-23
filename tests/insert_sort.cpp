#include "../src/vector/fvector.hpp"
#include "../src/vector/vector.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"

#include <random>

int
main()
{

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_int_distribution<u32> distr_int(1, 10000);
  int random_integer = distr_int(eng);
  mc::fvector<u32> vec;
  for ( u64 i = 0; i < 50; i++ )
    vec.insert_sort(distr_int(eng));
  for ( auto &n : vec )
    mc::console(n);
  return 0;
}
