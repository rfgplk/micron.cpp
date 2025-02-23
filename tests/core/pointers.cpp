#include "../../src/io/console.hpp"
#include "../../src/pointer.hpp"
#include "../../src/std.h"

#include "../../src/string/strings.h"

int
main(void)
{
  mc::const_pointer<mc::wstr> cptr("你好嗎？");
  mc::console(*cptr);
  return 0;
}
