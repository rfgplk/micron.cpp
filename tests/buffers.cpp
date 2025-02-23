
#include "../src/iterator.hpp"
#include "../src/memory_block.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"


byte increment( byte x ){
  return ++x;
}

int
main(void)
{
  mc::buffer b(4096);
  b = 0xF0;
  mc::iterator<mc::buffer::value_type> itr(b);
  itr.for_each(increment);
  mc::console("Size of the buffer is: ", b.size());
  b.resize(8192);
  for(auto& n : b) {
    mc::console(n);
  }
  mc::console("Size of the buffer after resizing is: ", b.size());
  b.recreate(4096);
  mc::console("Size of the buffer after recreating is: ", b.size());
}
