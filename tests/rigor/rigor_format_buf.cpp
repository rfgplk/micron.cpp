// rigor_format_buf.cpp — snowball suite for low-level buffer functions in
// src/string/format.hpp (namespace micron::format::__impl) plus parse
// utilities.
//
// Coverage:
//   __impl::bool_to_buf            (bool -> "true"/"false", buf-bounded)
//   __impl::ptr_to_buf             (void* -> "0x...", buf-bounded)
//   __impl::fmt_uint_to_buf        (u64 in arbitrary base)
//   __impl::fmt_int_to_buf         (i64 in arbitrary base)
//   __impl::fmt_float_to_buf       (f64 + precision)
//   __impl::fmt_float_to_buf_typed (f64 + precision + type char)
//   __impl::parse_spec             (fmt-spec parser)
//   __impl::parse_decimal/hex/octal (cursor-advancing parse)
//   __impl::parse_hex_byte
//   __impl::xdigit_to_val / to_hex_char
//
// Buffer-bounded: every helper writes at most buf_sz bytes and returns
// the number written. Tests verify both the written content and the
// boundary behavior (returning 0 / truncated count when buf is too small).

#include "../../src/string/format.hpp"

#include "../support/format_rigor.hpp"

using namespace mtest::format_rigor;
using mtest::prng;
namespace fmt = micron::format;
namespace fimpl = micron::format::__impl;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== FORMAT/BUF RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // bool_to_buf
  // ════════════════════════════════════════════════════════════════════

  test_case("bool_to_buf(true) writes 'true'");
  {
    char buf[8] = {};
    usize n = fimpl::bool_to_buf(buf, 8, true);
    require(n, usize(4));
    require_true(bytes_equal(buf, 4, "true", 4));
  }
  end_test_case();

  test_case("bool_to_buf(false) writes 'false'");
  {
    char buf[8] = {};
    usize n = fimpl::bool_to_buf(buf, 8, false);
    require(n, usize(5));
    require_true(bytes_equal(buf, 5, "false", 5));
  }
  end_test_case();

  test_case("bool_to_buf returns 0 when buffer too small for 'true'");
  {
    char buf[3] = {};
    usize n = fimpl::bool_to_buf(buf, 3, true);
    require(n, usize(0));
  }
  end_test_case();

  test_case("bool_to_buf returns 0 when buffer too small for 'false'");
  {
    char buf[4] = {};
    usize n = fimpl::bool_to_buf(buf, 4, false);
    require(n, usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // ptr_to_buf
  // ════════════════════════════════════════════════════════════════════

  test_case("ptr_to_buf(nullptr) writes '0x0'");
  {
    char buf[16] = {};
    usize n = fimpl::ptr_to_buf(buf, 16, nullptr);
    require(n, usize(3));
    require_true(bytes_equal(buf, 3, "0x0", 3));
  }
  end_test_case();

  test_case("ptr_to_buf(non-null) starts with '0x'");
  {
    char buf[32] = {};
    int x = 0;
    usize n = fimpl::ptr_to_buf(buf, 32, &x);
    require_true(n >= 3);
    require(buf[0], '0');
    require(buf[1], 'x');
  }
  end_test_case();

  test_case("ptr_to_buf returns 0 on too-small buffer");
  {
    char buf[3] = {};
    int x = 0;
    usize n = fimpl::ptr_to_buf(buf, 3, &x);
    require(n, usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fmt_uint_to_buf
  // ════════════════════════════════════════════════════════════════════

  test_case("fmt_uint_to_buf(0) base 10");
  {
    char buf[16] = {};
    usize n = fimpl::fmt_uint_to_buf(buf, 16, 0u, 10, false);
    require(n, usize(1));
    require(buf[0], '0');
  }
  end_test_case();

  test_case("fmt_uint_to_buf(12345) base 10");
  {
    char buf[16] = {};
    usize n = fimpl::fmt_uint_to_buf(buf, 16, 12345u, 10, false);
    require(n, usize(5));
    require_true(bytes_equal(buf, 5, "12345", 5));
  }
  end_test_case();

  test_case("fmt_uint_to_buf(0xDEAD) base 16 lower");
  {
    char buf[16] = {};
    usize n = fimpl::fmt_uint_to_buf(buf, 16, 0xDEADu, 16, false);
    require(n, usize(4));
    require_true(bytes_equal(buf, 4, "dead", 4));
  }
  end_test_case();

  test_case("fmt_uint_to_buf(0xDEAD) base 16 UPPER");
  {
    char buf[16] = {};
    usize n = fimpl::fmt_uint_to_buf(buf, 16, 0xDEADu, 16, true);
    require(n, usize(4));
    require_true(bytes_equal(buf, 4, "DEAD", 4));
  }
  end_test_case();

  test_case("fmt_uint_to_buf adversarial sizes (boundary u64)");
  {
    for ( usize i = 0; i < kUnsignedBoundariesCount; ++i ) {
      u64 v = kUnsignedBoundaries[i];
      char buf[32] = {};
      usize n = fimpl::fmt_uint_to_buf(buf, 32, v, 10, false);
      char ref_buf[32];
      usize rn = ref::naive_u64_to_dec(v, ref_buf, sizeof(ref_buf));
      require(n, rn);
      require_true(bytes_equal(buf, n, ref_buf, rn));
    }
  }
  end_test_case();

  property_test(
      "fmt_uint_to_buf base=10 vs naive (10k)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        char buf[32] = {};
        usize n = fimpl::fmt_uint_to_buf(buf, 32, v, 10, false);
        char ref_buf[32];
        usize rn = ref::naive_u64_to_dec(v, ref_buf, sizeof(ref_buf));
        require(n, rn);
        require_true(bytes_equal(buf, n, ref_buf, rn));
      },
      10000);

  property_test(
      "fmt_uint_to_buf base=16 vs naive (10k)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        char buf[32] = {};
        usize n = fimpl::fmt_uint_to_buf(buf, 32, v, 16, false);
        char ref_buf[32];
        usize rn = ref::naive_u64_to_hex(v, ref_buf, sizeof(ref_buf), false);
        require(n, rn);
        require_true(bytes_equal(buf, n, ref_buf, rn));
      },
      10000);

  property_test(
      "fmt_uint_to_buf base=2 vs naive (10k)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        char buf[72] = {};
        usize n = fimpl::fmt_uint_to_buf(buf, 72, v, 2, false);
        char ref_buf[72];
        usize rn = ref::naive_u64_to_bin(v, ref_buf, sizeof(ref_buf));
        require(n, rn);
        require_true(bytes_equal(buf, n, ref_buf, rn));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // fmt_int_to_buf
  // ════════════════════════════════════════════════════════════════════

  test_case("fmt_int_to_buf(0)");
  {
    char buf[16] = {};
    usize n = fimpl::fmt_int_to_buf(buf, 16, 0, 10, false);
    require(n, usize(1));
    require(buf[0], '0');
  }
  end_test_case();

  test_case("fmt_int_to_buf(-12345)");
  {
    char buf[16] = {};
    usize n = fimpl::fmt_int_to_buf(buf, 16, -12345, 10, false);
    require(n, usize(6));
    require_true(bytes_equal(buf, 6, "-12345", 6));
  }
  end_test_case();

  test_case("fmt_int_to_buf(INT64_MIN)");
  {
    char buf[32] = {};
    i64 v = static_cast<i64>(-0x7fffffffffffffffLL - 1);
    usize n = fimpl::fmt_int_to_buf(buf, 32, v, 10, false);
    require_true(bytes_equal(buf, n, "-9223372036854775808", 20));
  }
  end_test_case();

  test_case("fmt_int_to_buf hex base ignores sign");
  {
    char buf[16] = {};
    // hex/oct/bin do not prepend '-' (base != 10)
    usize n = fimpl::fmt_int_to_buf(buf, 16, -1, 16, false);
    require_true(n > 0);
    require(buf[0], 'f');      // -1 cast to u64 is all-ones
  }
  end_test_case();

  property_test(
      "fmt_int_to_buf base=10 vs naive (10k)",
      [](u32 raw_hi, u32 raw_lo) {
        i64 v = (static_cast<i64>(raw_hi) << 32) | static_cast<i64>(raw_lo);
        char buf[32] = {};
        usize n = fimpl::fmt_int_to_buf(buf, 32, v, 10, false);
        char ref_buf[32];
        usize rn = ref::naive_i64_to_dec(v, ref_buf, sizeof(ref_buf));
        require(n, rn);
        require_true(bytes_equal(buf, n, ref_buf, rn));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // fmt_float_to_buf / fmt_float_to_buf_typed
  // ════════════════════════════════════════════════════════════════════

  test_case("fmt_float_to_buf(0.0)");
  {
    char buf[32] = {};
    usize n = fimpl::fmt_float_to_buf(buf, 32, 0.0, 6);
    require_true(n > 0);
    require(buf[0], '0');
  }
  end_test_case();

  test_case("fmt_float_to_buf(3.5)");
  {
    char buf[32] = {};
    usize n = fimpl::fmt_float_to_buf(buf, 32, 3.5, 6);
    require_true(n > 0);
    require(buf[0], '3');
  }
  end_test_case();

  test_case("fmt_float_to_buf(-1.5) negative starts with '-'");
  {
    char buf[32] = {};
    usize n = fimpl::fmt_float_to_buf(buf, 32, -1.5, 6);
    require_true(n > 0);
    require(buf[0], '-');
  }
  end_test_case();

  test_case("fmt_float_to_buf_typed type='e' uses scientific notation");
  {
    char buf[64] = {};
    usize n = fimpl::fmt_float_to_buf_typed(buf, 64, 1.0, 2, 'e', true);
    require_true(n > 0);
    bool has_e = false;
    for ( usize i = 0; i < n; ++i )
      if ( buf[i] == 'e' || buf[i] == 'E' ) {
        has_e = true;
        break;
      }
    require_true(has_e);
  }
  end_test_case();

  test_case("fmt_float_to_buf_typed type='f' uses fixed");
  {
    char buf[64] = {};
    usize n = fimpl::fmt_float_to_buf_typed(buf, 64, 1.5, 3, 'f', true);
    require_true(n > 0);
    bool has_e = false;
    for ( usize i = 0; i < n; ++i )
      if ( buf[i] == 'e' || buf[i] == 'E' ) {
        has_e = true;
        break;
      }
    require_false(has_e);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // parse_decimal / parse_hex / parse_octal (cursor-advancing parsers)
  // ════════════════════════════════════════════════════════════════════

  test_case("parse_decimal reads digits, stops at non-digit");
  {
    const char *s = "12345abc";
    const char *p = s;
    u32 v = fmt::parse_decimal(p);
    require(v, u32(12345));
    require_true(p == s + 5);
  }
  end_test_case();

  test_case("parse_decimal on empty / no-digit string returns 0");
  {
    const char *s = "";
    const char *p = s;
    u32 v = fmt::parse_decimal(p);
    require(v, u32(0));
    require_true(p == s);
  }
  end_test_case();

  test_case("parse_hex reads hex digits");
  {
    const char *s = "deadbeefXYZ";
    const char *p = s;
    u32 v = fmt::parse_hex(p);
    require(v, u32(0xdeadbeef));
    require_true(p == s + 8);
  }
  end_test_case();

  test_case("parse_hex case-insensitive");
  {
    const char *s = "AbCdEf!!!";
    const char *p = s;
    u32 v = fmt::parse_hex(p);
    require(v, u32(0xAbCdEf));
    require_true(p == s + 6);
  }
  end_test_case();

  test_case("parse_octal stops at 8/9");
  {
    const char *s = "777890";
    const char *p = s;
    u32 v = fmt::parse_octal(p);
    require(v, u32(0777));
    require_true(p == s + 3);      // stopped at '8'
  }
  end_test_case();

  test_case("parse_hex_byte two-char hex");
  {
    const char *s = "abcd";
    const char *p = s;
    int v = fmt::parse_hex_byte(p);
    require(v, int(0xab));
    require_true(p == s + 2);
  }
  end_test_case();

  test_case("parse_hex_byte single-char hex");
  {
    const char *s = "a!";
    const char *p = s;
    int v = fmt::parse_hex_byte(p);
    require(v, int(0xa));
    require_true(p == s + 1);
  }
  end_test_case();

  test_case("parse_hex_byte non-hex returns -1");
  {
    const char *s = "xyz";
    const char *p = s;
    int v = fmt::parse_hex_byte(p);
    require(v, int(-1));
    require_true(p == s);
  }
  end_test_case();

  property_test(
      "parse_decimal round-trips through naive (10k)",
      [](u32 raw) {
        u32 v = raw & 0x7fffffffu;      // u32 range that fits in our generator
        char buf[16];
        usize n = ref::naive_u64_to_dec(v, buf, sizeof(buf));
        buf[n] = '\0';
        const char *p = buf;
        u32 out = fmt::parse_decimal(p);
        require(out, v);
        require_true(p == buf + n);
      },
      10000);

  property_test(
      "parse_hex round-trips through naive (10k)",
      [](u32 raw) {
        u32 v = raw;
        char buf[16];
        usize n = ref::naive_u64_to_hex(v, buf, sizeof(buf), false);
        buf[n] = '\0';
        const char *p = buf;
        u32 out = fmt::parse_hex(p);
        require(out, v);
        require_true(p == buf + n);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // parse_spec — the format-spec parser
  // ════════════════════════════════════════════════════════════════════

  test_case("parse_spec empty -> default");
  {
    const char *s = "";
    auto spec = fimpl::parse_spec(s, s);
    require(spec.width, u32(0));
    require(spec.prec, u32(6));      // fmt_spec default prec is 6
    require_false(spec.has_prec);
    require(spec.fill, ' ');      // default fill is space
    require(spec.align, '\0');
    require(spec.type, '\0');
    require_false(spec.alt);
  }
  end_test_case();

  test_case("parse_spec '10' -> width=10");
  {
    const char *s = "10";
    auto spec = fimpl::parse_spec(s, s + 2);
    require(spec.width, u32(10));
    require_false(spec.has_prec);
  }
  end_test_case();

  test_case("parse_spec '.5' -> precision=5");
  {
    const char *s = ".5";
    auto spec = fimpl::parse_spec(s, s + 2);
    require_true(spec.has_prec);
    require(spec.prec, u32(5));
  }
  end_test_case();

  test_case("parse_spec '10.3f' -> width, precision, type");
  {
    const char *s = "10.3f";
    auto spec = fimpl::parse_spec(s, s + 5);
    require(spec.width, u32(10));
    require_true(spec.has_prec);
    require(spec.prec, u32(3));
    require(spec.type, 'f');
  }
  end_test_case();

  test_case("parse_spec '>10' -> right align");
  {
    const char *s = ">10";
    auto spec = fimpl::parse_spec(s, s + 3);
    require(spec.align, '>');
    require(spec.width, u32(10));
  }
  end_test_case();

  test_case("parse_spec '*<10' -> fill='*', left align");
  {
    const char *s = "*<10";
    auto spec = fimpl::parse_spec(s, s + 4);
    require(spec.fill, '*');
    require(spec.align, '<');
    require(spec.width, u32(10));
  }
  end_test_case();

  test_case("parse_spec '0^4d' -> center fill='0' width=4 type='d'");
  {
    const char *s = "0^4d";
    auto spec = fimpl::parse_spec(s, s + 4);
    require(spec.fill, '0');
    require(spec.align, '^');
    require(spec.width, u32(4));
    require(spec.type, 'd');
  }
  end_test_case();

  test_case("parse_spec '#x' -> alt + hex");
  {
    const char *s = "#x";
    auto spec = fimpl::parse_spec(s, s + 2);
    require_true(spec.alt);
    require(spec.type, 'x');
  }
  end_test_case();

  sb::print("=== FORMAT/BUF RIGOR SUITE PASSED ===");
  return 1;
}
