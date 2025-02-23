#include "../src/string/strings.h"
#include "../src/io/console.hpp"
#include "../src/std.h"

#include <iostream>

int
main(void)
{
  mc::ustr8 str = "Another message";
  for(size_t i = 0; i < (size_t)1e7; i++)
    mc::console(str);
}
