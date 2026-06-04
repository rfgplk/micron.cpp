// simd_shifts_arm32.cpp

#include "../../src/simd/arch/shifts_arm32.hpp"
#include "../../src/simd/simd.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace ms = micron::simd;
namespace nb = micron::simd::__bits;

template<typename Flag> struct pk {
  using bit_width = nb::int32x4_t;
  using lane_width = Flag;
  nb::int32x4_t v;
  pk() = default;

  pk(nb::int32x4_t x) : v(x) { }

  operator nb::int32x4_t() const { return v; }
};

using pk16 = pk<ms::__v16>;
using pk32 = pk<ms::__v32>;
using pk64 = pk<ms::__v64>;

static nb::int32x4_t
mk32(int a, int b, int c, int d)
{
  nb::int32x4_t r = { a, b, c, d };
  return r;
}

static nb::int32x4_t
mk64(long long a, long long b)
{
  nb::int64x2_t r = { a, b };
  return nb::vreinterpretq_s32_s64(r);
}

static unsigned
lu32(nb::int32x4_t x, int i)
{
  return (unsigned)x[i];
}

static int
ls32(nb::int32x4_t x, int i)
{
  return x[i];
}

static unsigned long long
lu64(nb::int32x4_t x, int i)
{
  nb::uint64x2_t s = nb::vreinterpretq_u64_s32(x);
  return s[i];
}

int
main()
{
  print("=== SIMD SHIFTS ARM32 (fixed-path) TESTS ===");

  test_case("sra (-16>>2==-4 signed, sign fill)");
  {
    pk32 a{ mk32(-16, 16, -1, -1024) };
    nb::int32x4_t r = ms::sra(a, mk32(2, 0, 0, 0));
    require_true(ls32(r, 0) == -4 && ls32(r, 1) == 4 && ls32(r, 2) == -1 && ls32(r, 3) == -256);
  }
  end_test_case();

  test_case("srai (immediate arithmetic right)");
  {
    pk32 a{ mk32(-16, 256, -1, 1024) };
    nb::int32x4_t r = ms::srai(a, 2);
    require_true(ls32(r, 0) == -4 && ls32(r, 1) == 64 && ls32(r, 2) == -1 && ls32(r, 3) == 256);
  }
  end_test_case();

  test_case("srl (logical right zero-fill, (u)-16>>2==0x3FFFFFFC)");
  {
    pk32 a{ mk32(-16, 16, -1, 1024) };
    nb::int32x4_t r = ms::srl(a, mk32(2, 0, 0, 0));
    require_true(lu32(r, 0) == 0x3FFFFFFCu && lu32(r, 1) == 4u && lu32(r, 2) == 0x3FFFFFFFu && lu32(r, 3) == 256u);
  }
  end_test_case();

  test_case("srli (immediate logical right zero-fill)");
  {
    pk32 a{ mk32(-1, 16, -16, 1024) };
    nb::int32x4_t r = ms::srli(a, 4);
    require_true(lu32(r, 0) == 0x0FFFFFFFu && lu32(r, 1) == 1u && lu32(r, 2) == 0x0FFFFFFFu && lu32(r, 3) == 64u);
  }
  end_test_case();

  test_case("srav (per-lane arithmetic right)");
  {
    pk32 a{ mk32(-16, -16, -16, -16) };
    pk32 c{ mk32(1, 2, 3, 4) };
    nb::int32x4_t r = ms::srav(a, c);
    require_true(ls32(r, 0) == -8 && ls32(r, 1) == -4 && ls32(r, 2) == -2 && ls32(r, 3) == -1);
  }
  end_test_case();

  test_case("srlv (per-lane logical right)");
  {
    pk32 a{ mk32(-1, -1, -1, -1) };
    pk32 c{ mk32(28, 29, 30, 31) };
    nb::int32x4_t r = ms::srlv(a, c);
    require_true(lu32(r, 0) == 0xFu && lu32(r, 1) == 0x7u && lu32(r, 2) == 0x3u && lu32(r, 3) == 0x1u);
  }
  end_test_case();

  test_case("srli_64 (64-bit logical right)");
  {
    pk64 a{ mk64(-1LL, 1024LL) };
    nb::int32x4_t r = ms::srli(a, 4);
    require_true(lu64(r, 0) == 0x0FFFFFFFFFFFFFFFULL && lu64(r, 1) == 64ULL);
  }
  end_test_case();

  test_case("maskz_srli_64 (k=0b01 keeps lane0, zeros lane1) -> __expand_mask_u64");
  {
    pk64 src{ mk64(0x7777777777777777LL, 0x7777777777777777LL) };
    pk64 a{ mk64(-1LL, -1LL) };
    nb::int32x4_t r = ms::maskz_srli_64((unsigned)0x1, a, 4);

    require_true(lu64(r, 0) == 0x0FFFFFFFFFFFFFFFULL && lu64(r, 1) == 0ULL);
    (void)src;
  }
  end_test_case();

  test_case("mask_srli_64 (k=0b10 keeps lane1 result, lane0 from src) -> __expand_mask_u64");
  {
    pk64 src{ mk64(0x1111111111111111LL, 0x2222222222222222LL) };
    pk64 a{ mk64(-1LL, 256LL) };
    nb::int32x4_t r = ms::mask_srli_64(src, (unsigned)0x2, a, 4);
    require_true(lu64(r, 0) == 0x1111111111111111ULL && lu64(r, 1) == 16ULL);
  }
  end_test_case();

  print("=== ALL SIMD SHIFTS ARM32 TESTS PASSED ===");
  return 1;
}
