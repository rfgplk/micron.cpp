

#include "../../src/bits/__arch.hpp"
#include "../../src/simd/simd.hpp"

#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#if defined(__micron_arch_x86_any)

namespace ms = ::micron::simd;

alignas(64) static int g_a[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
alignas(64) static float g_f[16] = { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f };

static void
test_v128_construct(void)
{
  test_case("v128: ctors, operator[], size");

  ms::v32 v(2, 5, 7, 11);
  require_true(v.size() == 4);
  require_true(v[0] == 2);
  require_true(v[1] == 5);
  require_true(v[2] == 7);
  require_true(v[3] == 11);

  v = { 8, 12, 24, 45 };
  require_true(v[0] == 8);
  require_true(v[1] == 12);
  require_true(v[2] == 24);
  require_true(v[3] == 45);

  ms::v32 b(7);
  require_true(b[0] == 7 && b[1] == 7 && b[2] == 7 && b[3] == 7);

  end_test_case();
}

static void
test_v128_zero_ones(void)
{
  test_case("v128: to_zero / to_ones / all_zeroes / all_ones");

  ms::v32 v(1, 2, 3, 4);
  require_true(!v.all_zeroes());

  v.to_zero();
  require_true(v.all_zeroes());
  require_true(v[0] == 0 && v[3] == 0);

  v.to_ones();
  require_true(v.all_ones());
  require_true(v[0] == -1 && v[3] == -1);

  end_test_case();
}

static void
test_v128_load_store(void)
{
  test_case("v128: load / uload / get / get_aligned");

  ms::v32 a;
  a.load(g_a);
  require_true(a[0] == 1 && a[1] == 2 && a[2] == 3 && a[3] == 4);

  ms::v32 u;
  u.uload(g_a + 1);
  require_true(u[0] == 2 && u[1] == 3 && u[2] == 4 && u[3] == 5);

  int out[4] = { 0, 0, 0, 0 };
  a.get(out);
  require_true(out[0] == 1 && out[1] == 2 && out[2] == 3 && out[3] == 4);

  alignas(64) int outa[4] = { 0, 0, 0, 0 };
  a.get_aligned(outa);
  require_true(outa[0] == 1 && outa[1] == 2 && outa[2] == 3 && outa[3] == 4);

  end_test_case();
}

static void
test_v128_arith(void)
{
  test_case("v128: += -= *= &= |= ^=");

  ms::v32 v(10, 20, 30, 40);
  ms::v32 w(1, 2, 3, 4);

  v += w;
  require_true(v[0] == 11 && v[3] == 44);

  v -= w;
  require_true(v[0] == 10 && v[3] == 40);

  ms::v32 m(2, 3, 4, 5);
  m *= ms::v32(2, 2, 2, 2);
  require_true(m[0] == 4 && m[1] == 6 && m[2] == 8 && m[3] == 10);

  ms::v32 bits(0b1100, 0b1100, 0b1100, 0b1100);
  bits &= ms::v32(0b1010, 0b1010, 0b1010, 0b1010);
  require_true(bits[0] == 0b1000);

  bits |= ms::v32(0b0001, 0b0001, 0b0001, 0b0001);
  require_true(bits[0] == 0b1001);

  bits ^= ms::v32(0b1111, 0b1111, 0b1111, 0b1111);
  require_true(bits[0] == 0b0110);

  end_test_case();
}

static void
test_v128_compare(void)
{
  test_case("v128: operator== / operator!=  (movemask, not bool)");

  ms::v32 a(1, 2, 3, 4);
  ms::v32 b(1, 2, 3, 4);
  ms::v32 c(1, 2, 3, 9);

  const int all = (a == b);
  const int some = (a == c);
  require_true(all != 0);
  require_true(some != all);

  end_test_case();
}

static void
test_v128_float(void)
{
  test_case("v128: float lanes (vfloat)");

  ms::vfloat v;
  v.uload(g_f);
  require_true(v.size() == 4);
  require_true(v[0] == 1.0f && v[3] == 4.0f);

  v += v;
  require_true(v[0] == 2.0f && v[3] == 8.0f);

  ms::vfloat a;
  a.load(g_f);
  require_true(a[0] == 1.0f && a[3] == 4.0f);

  end_test_case();
}

// v256 needs AVX2, exactly as v512 below needs AVX-512F: there is no narrower form of a 256-bit
// vector class, so below AVX2 ms::w* is not declared at all (see simd/simd.hpp). this used to be
// ungated only because the build recipe forced -mavx2 on every x86 compile.
#if defined(__micron_x86_avx2)

static void
test_v256(void)
{
  test_case("v256: ctors, load, arith, lanes");

  ms::w32 v;
  v.load(g_a);
  require_true(v.size() == 8);
  require_true(v[0] == 1 && v[7] == 8);

  ms::w32 w;
  w.uload(g_a + 1);
  require_true(w[0] == 2 && w[7] == 9);

  v += ms::w32(1);
  require_true(v[0] == 2 && v[7] == 9);

  v.to_zero();
  require_true(v.all_zeroes());
  v.to_ones();
  require_true(v.all_ones());

  ms::wfloat f;
  f.load(g_f);
  require_true(f[0] == 1.0f && f[7] == 8.0f);

  int out[8] = {};
  ms::w32 g;
  g.load(g_a);
  g.get(out);
  require_true(out[0] == 1 && out[7] == 8);

  end_test_case();
}

#endif      // __micron_x86_avx2

#if defined(__AVX512F__)

static void
test_v512(void)
{
  test_case("v512: ctors, load, lanes");

  ms::z32 v;
  v.load(g_a);
  require_true(v.size() == 16);
  require_true(v[0] == 1 && v[15] == 16);

  v.to_zero();
  require_true(v.all_zeroes());
  v.to_ones();
  require_true(v.all_ones());

  ms::zfloat f;
  f.load(g_f);
  require_true(f[0] == 1.0f && f[15] == 16.0f);

  end_test_case();
}
#endif

int
main()
{
  print("=== SIMD CLASS (v128/v256/v512) ===");

  test_v128_construct();
  test_v128_zero_ones();
  test_v128_load_store();
  test_v128_arith();
  test_v128_compare();
  test_v128_float();
#if defined(__micron_x86_avx2)
  test_v256();
#else
  print("[v256 needs AVX2 - skipped at this ISA level]");
#endif
#if defined(__AVX512F__)
  test_v512();
#else
  print("[v512 body compiled; execution needs -mavx512f - skipped]");
#endif

  print("[SIMD CLASS OK]");
  return 1;
}

#else

int
main()
{
  print("=== SIMD CLASS (v128/v256/v512) ===");
  print("[skipped: x86-only; the NEON v128 class is covered by neon_v128_classfix.cpp]");
  return 1;
}

#endif

#pragma GCC diagnostic pop
