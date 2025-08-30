#include "../../src/allocation/abcmalloc/oom.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"
int
main(void)
{
  mc::resources rs;
  mc::console(rs.free_memory);
  mc::console(rs.total_memory);
  return 0;
}
