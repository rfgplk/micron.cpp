#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "../src/math/rng.hpp"
#include "../src/range.hpp"

int
main()
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(0x9E3779B97F4A7C15ULL);

  for ( u64 i : mc::u64_range<100, 1000>() ) mc::console(micron::math::rng::dist::uniform_int<u32>(rng, 0u, i));
  return 1;
}
