
#include "../src/heap/bloom.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"
#include "../src/vector/vector.hpp"


#include <vector>

int
main(void)
{
  mc::bloom_filter<u32, 524288> bf;
   for(size_t i = 0; i < 524288/3; i++) {
     bf.insert(i);
   }
  mc::console(bf.contains(7004567));
  mc::console(bf.contains(7304567));
  mc::console(bf.contains(13504567));
  return 1;
}
