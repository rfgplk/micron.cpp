#include "../src/slice.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"
#include "../src/hash/hash.hpp"
#include "../src/string/strings.h"
int
main(void)
{
  mc::string key_str = "uie5gwj89jdoibj90qu5go0isjgoijzec0ohgujb0 hjw4i5hjsro ibjsroihjsr ihjw49 05";
  const char *key = "uie5gwj89jdoibj90qu5go0isjgoijzec0ohgujb0 hjw4i5hjsro ibjsroihjsr ihjw49 05";
  auto h = mc::hash<mc::hash128_t>(key);
  auto hstr = mc::hash<mc::hash128_t>(key_str);
  u64 c = mc::hash(key);
  volatile int x = 0;
  for(size_t i = 0; i < 5e9; i++)
  {
    auto d = mc::hash(key);
    x++;
  }

  mc::console("128-bit murmur of string is: ", hstr.a, " ", hstr.b);
  mc::console("128-bit murmur is: ", h.a, " ", h.b);
  mc::console("64-bit xxhash is: ", c);
  return 1;
}
