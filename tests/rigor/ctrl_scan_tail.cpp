// ctrl_scan_tail.cpp
// Unit test for micron::__impl::ctrl_scan (src/maps/robin.hpp:90-115). The
// function returns the first index i in a 32-byte (AVX2) or 16-byte (SSE2/
// NEON) window where ctrl_bytes[i] > saturating_add_u8(plen, i). Verifies
// the SIMD implementation matches a scalar reference across every position
// and edge cases (all-equal, plen near saturation, boundary index W-1).

#include "../../src/std.hpp"

#include "../../src/maps/robin.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

// scalar reference impl of ctrl_scan. Semantics (per robin.hpp:33-49):
// builds saturating threshold = plen + i (clamped to 255) per lane, returns
// the first lane where p[i] is NOT strictly greater than thres (i.e. the
// first probe-termination point). Returns W if every lane exceeds its
// threshold.
usize
scan_scalar(const u8 *p, usize plen)
{
  constexpr usize W = micron::__impl::__ctrl_window;
  for ( usize i = 0; i < W; ++i ) {
    usize sum = plen + i;
    u8 thres = sum <= 255u ? static_cast<u8>(sum) : 255u;
    if ( !(p[i] > thres) ) return i;
  }
  return W;
}

}      // namespace

int
main()
{
  print("=== CTRL_SCAN TAIL TESTS ===");

  constexpr usize W = micron::__impl::__ctrl_window;
  if constexpr ( W == 0 ) {
    print("ctrl_scan not available on this build (no SIMD); skipping.");
    return 1;
  } else {
    // ------------------------------------------------------------ //
    test_case("all-zeros window with plen=0: SIMD == scalar");
    {
      alignas(32) u8 buf[64] = { 0 };
      const usize got = micron::__impl::ctrl_scan(buf, 0);
      const usize exp = scan_scalar(buf, 0);
      require(got, exp);
    }
    end_test_case();

    // ------------------------------------------------------------ //
    test_case("controlled first-termination at every position 0..W-1");
    for ( usize stop_at = 0; stop_at < W; ++stop_at ) {
      // make every byte exceed its threshold up to index stop_at-1, then
      // have stop_at fall AT or below threshold so it terminates there.
      // With plen=0, thres[i] = i. To make p[i] > thres[i] we set p[i]=255;
      // at stop_at we set p[stop_at] = 0 which is <= thres[stop_at].
      alignas(32) u8 buf[64];
      for ( usize i = 0; i < 64; ++i ) buf[i] = 255;
      buf[stop_at] = 0;
      const usize got = micron::__impl::ctrl_scan(buf, 0);
      const usize exp = scan_scalar(buf, 0);
      require(got, exp);
      require(got, stop_at);
    }
    end_test_case();

    // ------------------------------------------------------------ //
    test_case("non-W-aligned plen (1, 7, 31, 63, 128, 254, 255)");
    {
      const usize plens[] = { 1, 7, 31, 63, 128, 254, 255 };
      for ( const usize plen : plens ) {
        alignas(32) u8 buf[64];
        // fill with varying pattern
        for ( usize i = 0; i < 64; ++i ) buf[i] = static_cast<u8>((i * 17 + 3) & 0xff);
        const usize got = micron::__impl::ctrl_scan(buf, plen);
        const usize exp = scan_scalar(buf, plen);
        require(got, exp);
      }
    }
    end_test_case();

    // ------------------------------------------------------------ //
    test_case("plen at saturation boundary (plen=255): SIMD == scalar");
    {
      alignas(32) u8 buf[64];
      for ( usize i = 0; i < 64; ++i ) buf[i] = static_cast<u8>(i);      // 0..63
      // with plen=255, thres saturates to 255 at every position; nothing > 255
      // so every position satisfies "!(p[i] > thres)" -> return 0.
      const usize got = micron::__impl::ctrl_scan(buf, 255);
      const usize exp = scan_scalar(buf, 255);
      require(got, exp);
    }
    end_test_case();

    // ------------------------------------------------------------ //
    test_case("monotone-increasing buffer matches scalar at multiple plens");
    {
      alignas(32) u8 buf[64];
      for ( usize i = 0; i < 64; ++i ) buf[i] = static_cast<u8>(i & 0xff);
      for ( usize plen = 0; plen < 256; plen += 5 ) {
        const usize got = micron::__impl::ctrl_scan(buf, plen);
        const usize exp = scan_scalar(buf, plen);
        require(got, exp);
      }
    }
    end_test_case();

    // ------------------------------------------------------------ //
    test_case("randomized buffers vs scalar (xorshift)");
    {
      u64 seed = 0xdeadbeef'cafef00dULL;
      for ( int iter = 0; iter < 200; ++iter ) {
        alignas(32) u8 buf[64];
        // mix the seed and fill
        for ( usize i = 0; i < 64; ++i ) {
          seed ^= seed << 13;
          seed ^= seed >> 7;
          seed ^= seed << 17;
          buf[i] = static_cast<u8>(seed & 0xff);
        }
        const usize plen = (seed >> 32) & 0xff;
        const usize got = micron::__impl::ctrl_scan(buf, plen);
        const usize exp = scan_scalar(buf, plen);
        require(got, exp);
      }
    }
    end_test_case();
  }

  print("=== ALL CTRL_SCAN TESTS PASSED ===");
  return 1;
}
