#include <iostream>

#include "../src/vector/vector.hpp"

int
main()
{
  for ( size_t j = 0; j < 5e8; j++ ) {
    micron::vector<int> buf = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  }
  return 0;
}
