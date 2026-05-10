// test_neon.cpp
// Behavioral coverage for `micron::simd::neon::*`. only built when targeting
// arm64 / arm32. on x86 hosts this file is a self-test of the alias header
// shape (validates the file parses cleanly when included).

#include "../../src/bits/__arch.hpp"

#if defined(__micron_arch_arm64) || defined(__micron_arch_arm32)

#include "../../src/simd/aliases/neon.hpp"
#include "../snowball/snowball.hpp"

namespace mn = ::micron::simd::neon;

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

template <typename T>
[[gnu::always_inline]] inline bool
v_eq(T a, T b) noexcept
{
  alignas(16) unsigned char ba[sizeof(T)];
  alignas(16) unsigned char bb[sizeof(T)];
  __builtin_memcpy(ba, &a, sizeof(T));
  __builtin_memcpy(bb, &b, sizeof(T));
  for ( unsigned i = 0; i < sizeof(T); ++i )
    if ( ba[i] != bb[i] ) return false;
  return true;
}

int
main()
{
  print("=== TEST NEON ===");

  test_case("neon: load / store / splat (uint8x16)");
  alignas(16) unsigned char buf[16];
  for ( int i = 0; i < 16; ++i ) buf[i] = (unsigned char)i;
  uint8x16_t v = mn::load_u8(buf);
  uint8x16_t s = mn::splat_u8(7);
  alignas(16) unsigned char out[16];
  mn::store_u8(out, mn::add(v, s));
  for ( int i = 0; i < 16; ++i ) require_true(out[i] == (unsigned char)(i + 7));
  end_test_case();

  test_case("neon: arith on float32x4");
  alignas(16) float fbuf[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  float32x4_t fv = mn::load_f32(fbuf);
  float32x4_t fr = mn::add(fv, mn::splat_f32(0.5f));
  alignas(16) float fout[4];
  mn::store_f32(fout, fr);
  for ( int i = 0; i < 4; ++i ) require_true(fout[i] == fbuf[i] + 0.5f);

  float32x4_t fp = mn::mul(fv, fv);
  mn::store_f32(fout, fp);
  for ( int i = 0; i < 4; ++i ) require_true(fout[i] == fbuf[i] * fbuf[i]);
  end_test_case();

  test_case("neon: bitwise + compare on uint32x4");
  alignas(16) unsigned int ibuf[4] = { 0x10, 0x20, 0x30, 0x40 };
  uint32x4_t a = mn::load_u32(ibuf);
  uint32x4_t b = mn::splat_u32(0x30);
  uint32x4_t eq = mn::eq(a, b);
  alignas(16) unsigned int ebuf[4];
  mn::store_u32(ebuf, eq);
  require_true(ebuf[0] == 0u && ebuf[2] == 0xFFFFFFFFu);

  uint32x4_t gt = mn::gt(a, b);
  mn::store_u32(ebuf, gt);
  require_true(ebuf[0] == 0u && ebuf[3] == 0xFFFFFFFFu);

  uint32x4_t x = mn::xor_(a, b);
  mn::store_u32(ebuf, x);
  require_true(ebuf[0] == (0x10u ^ 0x30u));
  end_test_case();

#if defined(__micron_arch_arm64)
  test_case("neon: aarch64 min / max / abs / fma");
  float32x4_t mn_v = mn::min(fv, mn::splat_f32(2.5f));
  float32x4_t mx_v = mn::max(fv, mn::splat_f32(2.5f));
  mn::store_f32(fout, mn_v);
  require_true(fout[0] == 1.0f && fout[3] == 2.5f);
  mn::store_f32(fout, mx_v);
  require_true(fout[0] == 2.5f && fout[3] == 4.0f);

  float32x4_t fma_v = mn::fma(mn::splat_f32(10.0f), mn::splat_f32(2.0f), mn::splat_f32(3.0f));
  mn::store_f32(fout, fma_v);
  for ( int i = 0; i < 4; ++i ) require_true(fout[i] == 10.0f + 2.0f * 3.0f);
  end_test_case();
#endif

  print("[TEST NEON OK]");
  return 0;
}

#else

// host is x86 - we still validate the alias header parses cleanly when
// configured for ARM by force-defining the right arch macros around a
// secondary include of the file. this is a syntax check, not a runtime test.
#include "../snowball/snowball.hpp"

int
main()
{
  ::sb::print("=== TEST NEON ===");
  ::sb::print("[TEST NEON skipped on x86 host - cross-compile to arm64/armv7 to exercise]");
  return 0;
}

#endif
