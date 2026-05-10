// test_sse.cpp
// Behavioral coverage for `micron::simd::sse::*` (SSE/SSE2/SSE3/SSSE3/
// SSE4.1/SSE4.2). every test computes a known-good reference scalar value
// and compares against the shim. runs from snowball.

#include "../../src/simd/aliases/sse.hpp"
#include "../snowball/snowball.hpp"

namespace ms = ::micron::simd::sse;

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

template <typename T>
[[gnu::always_inline]] inline bool
v_lane_eq(T v, unsigned idx, long long expect) noexcept
{
  alignas(16) unsigned char b[sizeof(T)];
  __builtin_memcpy(b, &v, sizeof(T));
  long long got = 0;
  __builtin_memcpy(&got, b + idx * (sizeof(T) / 16), sizeof(T) / 16);
  // mask to lane width
  unsigned bits = (unsigned)(8 * (sizeof(T) / 16));
  unsigned long long mask = bits == 64 ? ~0ull : ((1ull << bits) - 1ull);
  return ((unsigned long long)got & mask) == ((unsigned long long)expect & mask);
}

// ---- set / splat / zero ---------------------------------------------------

static void
test_set_splat()
{
  test_case("sse: set / splat / zero");
  auto z32 = ms::zero_f32();
  auto zd = ms::zero_f64();
  auto zi = ms::zero_i128();
  require_true(ms::extract_low_f32(z32) == 0.0f);
  require_true(ms::extract_low_f64(zd) == 0.0);
  require_true(ms::movemask_i8(zi) == 0);

  auto sp_i8 = ms::splat_i8(0x77);
  auto sp_i32 = ms::splat_i32(-1);
  require_true(ms::movemask_i8(sp_i8) == 0);           // 0x77 = positive byte, movemask returns 0
  require_true(ms::movemask_i8(sp_i32) == 0xFFFF);     // all -1 bytes -> all-ones movemask
  require_true(v_eq(ms::splat_f32(3.14f), ms::set_f32(3.14f, 3.14f, 3.14f, 3.14f)));
  end_test_case();
}

// ---- load / store ---------------------------------------------------------

static void
test_load_store()
{
  test_case("sse: load / store / loadu");
  alignas(16) float buf[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  alignas(16) float out[4] = {};
  alignas(16) char ibuf[16];
  alignas(16) char oibuf[16];
  for ( int i = 0; i < 16; ++i ) {
    ibuf[i] = (char)(i + 1);
    oibuf[i] = 0;
  }

  auto v = ms::load_f32(buf);
  auto vu = ms::loadu_f32(buf);
  ms::store_f32(out, v);
  require_true(out[0] == 1.0f && out[3] == 4.0f);
  require_true(v_eq(v, vu));

  auto iv = ms::load_i128((const __m128i *)ibuf);
  auto ivu = ms::loadu_i128((const __m128i_u *)ibuf);
  require_true(v_eq(iv, ivu));
  ms::storeu_i128((__m128i_u *)oibuf, iv);
  for ( int i = 0; i < 16; ++i ) require_true(oibuf[i] == ibuf[i]);
  end_test_case();
}

// ---- arith integer --------------------------------------------------------

static void
test_arith_int()
{
  test_case("sse: int arith / saturating / avg / sad");
  auto a = ms::set_i32(4, 3, 2, 1);
  auto b = ms::set_i32(40, 30, 20, 10);
  auto s = ms::add_i32(a, b);
  alignas(16) int sb[4];
  ms::storeu_i128((__m128i_u *)sb, s);
  require_true(sb[0] == 11 && sb[3] == 44);

  auto d = ms::sub_i32(b, a);
  ms::storeu_i128((__m128i_u *)sb, d);
  require_true(sb[0] == 9 && sb[3] == 36);

  auto sat_a = ms::splat_i8(120);
  auto sat_b = ms::splat_i8(50);
  auto sat_r = ms::add_sat_i8(sat_a, sat_b);     // saturates to 127
  alignas(16) signed char sb8[16];
  ms::storeu_i128((__m128i_u *)sb8, sat_r);
  require_true(sb8[0] == 127);

  auto avg = ms::avg_u8(ms::splat_i8(0x40), ms::splat_i8(0x60));
  alignas(16) unsigned char ub8[16];
  ms::storeu_i128((__m128i_u *)ub8, avg);
  require_true(ub8[0] == 0x50);

  auto sad = ms::sad_u8(ms::splat_i8(0xA0), ms::splat_i8(0x10));
  alignas(16) unsigned long long sad_lanes[2];
  ms::storeu_i128((__m128i_u *)sad_lanes, sad);
  require_true(sad_lanes[0] == 0x90ull * 8);
  end_test_case();
}

// ---- arith float ----------------------------------------------------------

static void
test_arith_fp()
{
  test_case("sse: fp arith / sqrt / min / max");
  auto a = ms::setr_f32(1.0f, 4.0f, 9.0f, 16.0f);
  auto s = ms::sqrt_f32(a);
  alignas(16) float sb[4];
  ms::storeu_f32(sb, s);
  require_true(sb[0] == 1.0f && sb[1] == 2.0f && sb[2] == 3.0f && sb[3] == 4.0f);

  auto b = ms::splat_f32(2.5f);
  auto m = ms::min_f32(a, b);
  ms::storeu_f32(sb, m);
  require_true(sb[0] == 1.0f && sb[1] == 2.5f);

  auto x = ms::splat_f32(3.0f);
  auto y = ms::splat_f32(4.0f);
  auto sum = ms::add_f32(x, y);
  auto prod = ms::mul_f32(x, y);
  ms::storeu_f32(sb, sum);
  require_true(sb[0] == 7.0f);
  ms::storeu_f32(sb, prod);
  require_true(sb[0] == 12.0f);
  end_test_case();
}

// ---- bitwise --------------------------------------------------------------

static void
test_bitwise()
{
  test_case("sse: bitwise");
  auto a = ms::splat_i32(0x0F0F0F0F);
  auto b = ms::splat_i32(0xF0F0F0F0);
  auto x = ms::xor_i128(a, b);
  alignas(16) int sb[4];
  ms::storeu_i128((__m128i_u *)sb, x);
  require_true(sb[0] == int(0xFFFFFFFFu));

  auto an = ms::and_i128(a, b);
  ms::storeu_i128((__m128i_u *)sb, an);
  require_true(sb[0] == 0);

  auto orr = ms::or_i128(a, b);
  ms::storeu_i128((__m128i_u *)sb, orr);
  require_true(sb[0] == int(0xFFFFFFFFu));
  end_test_case();
}

// ---- compare --------------------------------------------------------------

static void
test_compare()
{
  test_case("sse: cmp / movemask");
  // set_i32(w, z, y, x) lays out lanes as { x, y, z, w } so lane 0 = -4.
  auto a = ms::set_i32(1, -2, 3, -4);
  auto b = ms::splat_i32(0);
  auto pos_mask = ms::gt_i32(a, b);
  alignas(16) int sb[4];
  ms::storeu_i128((__m128i_u *)sb, pos_mask);
  require_true(sb[0] == 0 && sb[1] == int(0xFFFFFFFFu) && sb[2] == 0 && sb[3] == int(0xFFFFFFFFu));

  auto eq = ms::eq_i32(a, b);
  ms::storeu_i128((__m128i_u *)sb, eq);
  require_true(sb[0] == 0 && sb[1] == 0 && sb[2] == 0 && sb[3] == 0);

  auto fa = ms::setr_f32(1.0f, 2.0f, 3.0f, 4.0f);
  auto fb = ms::splat_f32(2.5f);
  auto fcmp = ms::lt_f32(fa, fb);
  int mask = ms::movemask_f32(fcmp);
  require_true(mask == 0b0011);     // first two lanes < 2.5
  end_test_case();
}

// ---- shifts ---------------------------------------------------------------

static void
test_shifts()
{
  test_case("sse: shifts");
  auto a = ms::splat_i32(0x12345678);
  auto l = ms::shl_i32(a, 4);
  auto r = ms::shr_i32(a, 8);
  auto ar = ms::shr_arith_i32(ms::splat_i32(-128), 1);
  alignas(16) int sb[4];
  ms::storeu_i128((__m128i_u *)sb, l);
  require_true(sb[0] == 0x23456780);
  ms::storeu_i128((__m128i_u *)sb, r);
  require_true(sb[0] == 0x00123456);
  ms::storeu_i128((__m128i_u *)sb, ar);
  require_true(sb[0] == -64);
  end_test_case();
}

// ---- pack / unpack -------------------------------------------------------

static void
test_pack_unpack()
{
  test_case("sse: pack / unpack");
  auto a = ms::set_i32(0x10, 0x20, 0x30, 0x40);
  auto b = ms::set_i32(0x100, 0x200, 0x300, 0x400);
  auto unp_lo = ms::unpack_lo_i32(a, b);
  auto unp_hi = ms::unpack_hi_i32(a, b);
  alignas(16) int sb[4];
  ms::storeu_i128((__m128i_u *)sb, unp_lo);
  // unpack_lo_i32: { a[0], b[0], a[1], b[1] } = { 0x40, 0x400, 0x30, 0x300 }
  require_true(sb[0] == 0x40 && sb[1] == 0x400 && sb[2] == 0x30 && sb[3] == 0x300);
  ms::storeu_i128((__m128i_u *)sb, unp_hi);
  require_true(sb[0] == 0x20 && sb[1] == 0x200);

  auto sa = ms::set_i32(70000, -70000, 100, -100);
  auto packed = ms::pack_sat_i32(sa, sa);     // saturates to int16 range
  alignas(16) short ps[8];
  ms::storeu_i128((__m128i_u *)ps, packed);
  require_true(ps[0] == -100 && ps[1] == 100 && ps[2] == -32768 && ps[3] == 32767);
  end_test_case();
}

// ---- min / max (integer) -------------------------------------------------

static void
test_minmax()
{
  test_case("sse: min / max integer");
  auto a = ms::splat_i32(10);
  auto b = ms::splat_i32(-5);
  auto mn = ms::min_i32(a, b);
  auto mx = ms::max_i32(a, b);
  alignas(16) int sb[4];
  ms::storeu_i128((__m128i_u *)sb, mn);
  require_true(sb[0] == -5);
  ms::storeu_i128((__m128i_u *)sb, mx);
  require_true(sb[0] == 10);

  auto u_a = ms::splat_i8((char)0xC0);
  auto u_b = ms::splat_i8((char)0x40);
  auto u_mn = ms::min_u8(u_a, u_b);
  auto u_mx = ms::max_u8(u_a, u_b);
  alignas(16) unsigned char ub[16];
  ms::storeu_i128((__m128i_u *)ub, u_mn);
  require_true(ub[0] == 0x40);
  ms::storeu_i128((__m128i_u *)ub, u_mx);
  require_true(ub[0] == 0xC0);
  end_test_case();
}

// ---- shuffle (variable, SSSE3) ------------------------------------------

static void
test_shuffle()
{
  test_case("sse: shuffle / abs / sign / madd");
  auto a = ms::set_i32(0xAA, 0xBB, 0xCC, 0xDD);
  auto idx = ms::set_i32(0x0F0E0D0C, 0x0B0A0908, 0x07060504, 0x03020100);
  auto rev = ms::shuffle_v_i8(a, idx);
  // identity index -> output equals input
  require_true(v_eq(a, rev));

  auto neg = ms::splat_i16(-7);
  auto babs = ms::abs_i16(neg);
  alignas(16) short sb[8];
  ms::storeu_i128((__m128i_u *)sb, babs);
  require_true(sb[0] == 7);

  auto sgn = ms::sign_i16(ms::splat_i16(5), ms::splat_i16(-3));
  ms::storeu_i128((__m128i_u *)sb, sgn);
  require_true(sb[0] == -5);

  // madd: pairs of i16 multiplied + summed -> i32 lanes. set_i32 reverses,
  // so lane[0]=8, lane[1]=7 in the lo dword; madd_lane0 = 8*8 + 7*7 = 113.
  auto u = ms::set_i32(0x00010002, 0x00030004, 0x00050006, 0x00070008);
  auto v = ms::set_i32(0x00010002, 0x00030004, 0x00050006, 0x00070008);
  auto md = ms::madd_i16(u, v);
  alignas(16) int mi[4];
  ms::storeu_i128((__m128i_u *)mi, md);
  require_true(mi[0] == 8 * 8 + 7 * 7);
  end_test_case();
}

// ---- convert + extract ---------------------------------------------------

static void
test_convert()
{
  test_case("sse: convert");
  auto i = ms::set_i32(7, 6, 5, 4);
  auto f = ms::convert_i32_to_f32(i);
  alignas(16) float sb[4];
  ms::storeu_f32(sb, f);
  require_true(sb[0] == 4.0f && sb[3] == 7.0f);

  auto fp = ms::setr_f32(1.4f, 2.6f, -0.5f, 3.7f);
  auto ti = ms::convert_trunc_f32_to_i32(fp);
  alignas(16) int ib[4];
  ms::storeu_i128((__m128i_u *)ib, ti);
  require_true(ib[0] == 1 && ib[1] == 2 && ib[2] == 0 && ib[3] == 3);

  auto w = ms::splat_i8((char)-1);
  auto wide = ms::widen_i8_to_i16(w);
  alignas(16) short ws[8];
  ms::storeu_i128((__m128i_u *)ws, wide);
  require_true(ws[0] == -1 && ws[7] == -1);
  end_test_case();
}

// ---- testz / testc / testnzc (SSE4.1) ------------------------------------

static void
test_testz()
{
  test_case("sse: testz / testc / testnzc");
  auto a = ms::splat_i32(0);
  auto b = ms::splat_i32(-1);
  require_true(ms::testz_i128(a, b) == 1);     // a & b == 0
  require_true(ms::testc_i128(b, a) == 1);     // (~b & a) == 0
  end_test_case();
}

int
main()
{
  print("=== TEST SSE ===");
  test_set_splat();
  test_load_store();
  test_arith_int();
  test_arith_fp();
  test_bitwise();
  test_compare();
  test_shifts();
  test_pack_unpack();
  test_minmax();
  test_shuffle();
  test_convert();
  test_testz();
  print("[TEST SSE OK]");
  return 0;
}
