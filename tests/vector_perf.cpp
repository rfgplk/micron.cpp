#include <iostream>

#include "../src/vector/vector.hpp"
#include <vector>
int
main()
{
  micron::vector<int> buf = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  for ( size_t i = 0; i < 500000; i++ )
    buf.insert(buf.begin() + 5, 9999);
  return 0;
}
