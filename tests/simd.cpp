#include "../src/simd/simd.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "snowball/snowball.hpp"

int
main()
{
  mc::simd::v128<mc::simd::i128> v(2, 5, 7, 11);
  mc::simd::v128<mc::simd::i128> u(8, 12, 24, 45);
  sb::require(v[0], 11);
  sb::require(v[1], 7);
  sb::require(v[2], 5);
  sb::require(v[3], 2);
  v = { 8, 12, 24, 45 };
  sb::require(v[3], 8);
  sb::require(v[2], 12);
  sb::require(v[1], 24);
  sb::require(v[0], 45);
  sb::print(v[0]);
  sb::print(v[1]);
  sb::print(v[2]);
  sb::print(v[3]);
  sb::require(v == u);
  v.to_zero();
  sb::require(v.all_zeroes());
  return 0;
}
