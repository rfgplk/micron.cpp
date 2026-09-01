#include "../src/io.hpp"
#include "../src/std.hpp"

#include "../src/string/strings.hpp"

int
main()
{
  u64 *sentinel = (u64*)abc::alloc(sizeof(u64));
  *sentinel = 0xAAAABBBB;
  mc::string a = { "Whatever " };
  mc::string b = { "May " };
  mc::string c = { "Change!" };
  while ( *sentinel == 0xAAAABBBB ) {
    mc::echo(a, b, c);
  }
  return 0;
}
