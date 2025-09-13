
#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/string/strings.hpp"
#include "../snowball/snowball.hpp"

#include <random>

struct s {
  int x;
  int y;
};
int
main()
{
  if constexpr ( true ) {
    byte *a = 0;
    byte *b = 0;
    byte *c = 0;
    byte *d = 0;
    byte *e = 0;
    for ( int i = 0; i < 100; i++ ) {
      a = abc::alloc(1025);
      d = abc::alloc(67);
      e = abc::alloc(444);
      sb::require(abc::within(a), true);
      sb::require(abc::within(d), true);
      sb::require(abc::within(e), true);
      sb::require_true(abc::is_present(a));
      sb::require_true(abc::is_present(d));
      sb::require_true(abc::is_present(e));
      sb::require_false(abc::is_present(b));
      sb::require_false(abc::is_present(c));
      b = abc::alloc(143564);
      c = abc::alloc(56730);
      sb::require_true(abc::is_present(b));
      sb::require_true(abc::is_present(c));
      sb::require(abc::within(c), true);
      sb::require(abc::within(b), true);
      abc::dealloc(e);
      abc::dealloc(c);
      abc::dealloc(b);
      abc::dealloc(d);
      abc::dealloc(a);
      sb::require_false(abc::is_present(a));
      sb::require_false(abc::is_present(d));
      sb::require_false(abc::is_present(e));
      sb::require_false(abc::is_present(b));
      sb::require_false(abc::is_present(c));
    }
    mc::infolog("Success");
  }
  return 0;
}
