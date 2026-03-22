#include "../src/array/parray.hpp"
#include "../src/range.hpp"
#include "../src/std.hpp"

#include "../src/io/console.hpp"

int
main()
{
  mc::parray<int, 2, 2> parr(100);
  mc::console(parr.size());
  mc::console(parr.max_size());
  mc::console(parr[5]);
  mc::console(parr.set(1, 25));     // yields new arr
  mc::console(parr);
  for ( u64 i : mc::u64_range<1, 16>() )
    mc::console(parr.set(i, i * i));
  return 1;
}
