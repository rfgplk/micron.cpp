
#include "../src/string/strings.h"
#include "../src/string/format.h"
#include "../src/attributes.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"

int
main(void)
{
  mc::sstr<256> hello = "Hello, World World World!";
  mc::format::replace(hello, "World", "Cat");
  mc::console(hello);
  return 0;
}
