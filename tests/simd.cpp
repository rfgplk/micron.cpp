#include "../src/simd/simd.hpp"
#include "../src/io/console.hpp"
#include "../src/simd/arith.hpp"
#include "../src/simd/dispatch.hpp"
#include "../src/simd/fma.hpp"
#include "../src/std.hpp"

#include "snowball/snowball.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
int
main()
{
  mc::console(mc::simd::__has_avx256(mc::simd::__get_runtime_features()));

  mc::simd::v128<mc::simd::i128, mc::simd::__v32> v(2, 5, 7, 11);
  mc::simd::v128<mc::simd::i128, mc::simd::__v32> u(8, 12, 24, 45);
  mc::console(v[0]);
  mc::console(v[1]);
  mc::console(v[2]);
  mc::console(v[3]);
  sb::require(v[0], 2);
  sb::require(v[1], 5);
  sb::require(v[2], 7);
  sb::require(v[3], 11);
  v = { 8, 12, 24, 45 };
  mc::console(v[0]);
  mc::console(v[1]);
  mc::console(v[2]);
  mc::console(v[3]);
  sb::require(v[0], 8);
  sb::require(v[1], 12);
  sb::require(v[2], 24);
  sb::require(v[3], 45);
  // sb::require(v, u);
  v.to_zero();
  sb::require(v.all_zeroes(), true);
  v.to_ones();
  sb::require(v.all_ones(), true);
  v = { 5, 10, 25, 50 };
  v += 10;
  sb::require(v[0], 15);
  sb::require(v[1], 20);
  sb::require(v[2], 35);
  sb::require(v[3], 60);

  v -= 1;
  sb::require(v[0], 14);
  sb::require(v[1], 19);
  sb::require(v[2], 34);
  sb::require(v[3], 59);
  v -= { 5, 10, 15, 20 };
  sb::require(v[0], 9);
  sb::require(v[1], 9);
  sb::require(v[2], 19);
  sb::require(v[3], 39);
  mc::simd::v128<mc::simd::d128, mc::simd::__vd> d(11.1f);
  d += (double)0.6f;
  mc::console(d[1]);
  mc::simd::v256<mc::simd::d256, mc::simd::__vd> e(346.2f);
  e += (double)112.4f;
  mc::console(e[3]);
  e.to_zero();
  sb::require(e[0], 0);
  mc::simd::v256<mc::simd::i256, mc::simd::__v32> f(5);
  for ( int i = 0; i < 10; ++i )
    f *= 2;     // (5*(2^10)) == 5120
  mc::console(f[5]);
  sb::require(f[5], 5120);

  mc::w32 arr32 = {};

  for ( int i = 0; i < 30; ++i )
    arr32 += (3 * i);
  i32 arr[8] = {};
  arr32.get(arr);
  for ( int i = 0; i < 8; ++i )
    sb::require(arr[i], 1305);
  arr32 = arr;
  for ( int i = 0; i < 8; ++i )
    sb::require(arr32[i], 1305);
  mc::console("Done");
  return 0;
}
#pragma GCC diagnostic pop
