#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "../src/math/trig.hpp"
#include "../src/tasks/coroutine/routine.hpp"

#include "../src/tasks/tasks.hpp"

int
main(void)
{
  int v = 100;
  for ( ; v > 0; v-- ) {
    auto r = mc::coro::spin([](int x) { return mc::math::sin((float)x); }, v);
    mc::console(r.result());
  }

  return 1;
}
