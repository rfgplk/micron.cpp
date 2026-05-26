#include "../src/math/sqrt.hpp"

#include "../src/io/console.hpp"

#include "../src/std.hpp"

int
main(void)
{
  mc::console(mc::math::sqrt((f32)16.0f));
  mc::console(mc::math::sqrt((float)64.0f));
  mc::console(mc::math::sqrt((double)81.0f));
  mc::console(mc::math::sqrt((f64)121.0f));
  return 0;
}
