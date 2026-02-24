
#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/string/strings.hpp"
#include "../snowball/snowball.hpp"

struct s {
  int x;
  int y;
};

int
main()
{
  byte *a = 0;
  for ( ;; )
    for ( int i = 1; i < 500; i++ ) {
      a = abc::alloc(1025);
      abc::dealloc(a);
    }

  return 0;
}
