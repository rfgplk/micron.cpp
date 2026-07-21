// rigor_format_int.cpp — snowball suite for
// src/string/conversions/integral.hpp
//
// Coverage (in micron:: namespace):
//   int_to_string / uint_to_string                    (heap hstring)
//   int_to_string_stack / uint_to_string_stack        (sstring<N>)
//   int_to_string_base / uint_to_string_base          (arbitrary base)
//   to_hex / to_oct / to_bin                          (convenience)
//   to_hex_fixed / to_bin_fixed                       (zero-padded)
//   int_to_string_padded                              (zero-padded width)
//   string_to_int64 / string_to_uint64                (parse decimal)
//   hex_string_to_int64 / hex_string_to_uint64        (parse hex)
//   oct_string_to_uint64 / bin_string_to_uint64       (parse oct/bin)
//   string_to_int32 / string_to_uint32 / 16 / etc.    (narrower variants)
//   string_to_int_base / string_to_uint_base          (arbitrary base)
//   try_string_to_int64 / try_string_to_uint64        (non-throwing)
//   parse_int / parse_uint / parse_hex                (cursor-advancing)
//
// Property tests round-trip through both directions (int → str → int).

#include "../../src/string/conversions/integral.hpp"
#include "../../src/string/format.hpp"

#include "../support/format_rigor.hpp"

using namespace mtest::format_rigor;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== FORMAT/INT RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // int_to_string  (signed / decimal)
  // ════════════════════════════════════════════════════════════════════

  test_case("int_to_string boundary values");
  {
    for ( usize i = 0; i < kSignedBoundariesCount; ++i ) {
      i64 v = kSignedBoundaries[i];
      auto s = micron::int_to_string(v);
      char ref_buf[32];
      usize n = ref::naive_i64_to_dec(v, ref_buf, sizeof(ref_buf));
      require_true(hstr_equal_bytes(s, ref_buf, n));
    }
  }
  end_test_case();

  test_case("int_to_string zero");
  {
    auto s = micron::int_to_string<i32>(0);
    require_true(hstr_equal_cstr(s, "0"));
  }
  end_test_case();

  test_case("int_to_string positive / negative");
  {
    auto a = micron::int_to_string<i32>(12345);
    auto b = micron::int_to_string<i32>(-12345);
    require_true(hstr_equal_cstr(a, "12345"));
    require_true(hstr_equal_cstr(b, "-12345"));
  }
  end_test_case();

  test_case("int_to_string INT_MIN");
  {
    auto s = micron::int_to_string<i32>(static_cast<i32>(-2147483647 - 1));
    require_true(hstr_equal_cstr(s, "-2147483648"));
  }
  end_test_case();

  test_case("int_to_string INT64_MIN");
  {
    auto s = micron::int_to_string<i64>(static_cast<i64>(-0x7fffffffffffffffLL - 1));
    require_true(hstr_equal_cstr(s, "-9223372036854775808"));
  }
  end_test_case();

  property_test(
      "int_to_string vs naive (10k random i64)",
      [](u32 raw_hi, u32 raw_lo) {
        i64 v = (static_cast<i64>(raw_hi) << 32) | static_cast<i64>(raw_lo);
        auto s = micron::int_to_string(v);
        char ref_buf[32];
        usize n = ref::naive_i64_to_dec(v, ref_buf, sizeof(ref_buf));
        require_true(hstr_equal_bytes(s, ref_buf, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // uint_to_string  (unsigned / decimal)
  // ════════════════════════════════════════════════════════════════════

  test_case("uint_to_string boundary values");
  {
    for ( usize i = 0; i < kUnsignedBoundariesCount; ++i ) {
      u64 v = kUnsignedBoundaries[i];
      auto s = micron::uint_to_string(v);
      char ref_buf[32];
      usize n = ref::naive_u64_to_dec(v, ref_buf, sizeof(ref_buf));
      require_true(hstr_equal_bytes(s, ref_buf, n));
    }
  }
  end_test_case();

  test_case("uint_to_string UINT64_MAX");
  {
    auto s = micron::uint_to_string<u64>(0xffffffffffffffffULL);
    require_true(hstr_equal_cstr(s, "18446744073709551615"));
  }
  end_test_case();

  property_test(
      "uint_to_string vs naive (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string(v);
        char ref_buf[32];
        usize n = ref::naive_u64_to_dec(v, ref_buf, sizeof(ref_buf));
        require_true(hstr_equal_bytes(s, ref_buf, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // int_to_string_stack / uint_to_string_stack  (sstring<N>)
  // ════════════════════════════════════════════════════════════════════

  test_case("int_to_string_stack<32> matches int_to_string");
  {
    for ( usize i = 0; i < kSignedBoundariesCount; ++i ) {
      i64 v = kSignedBoundaries[i];
      auto a = micron::int_to_string<i64>(v);
      auto b = micron::int_to_string_stack<i64, char, 32>(v);
      require(a.size(), b.size());
      for ( usize j = 0; j < a.size(); ++j ) require(a[j], b[j]);
    }
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // base conversion
  // ════════════════════════════════════════════════════════════════════

  test_case("uint_to_string_base(16) lowercase hex");
  {
    auto s = micron::uint_to_string_base<u64>(0xdeadbeef, 16, false);
    require_true(hstr_equal_cstr(s, "deadbeef"));
  }
  end_test_case();

  test_case("uint_to_string_base(16) uppercase hex");
  {
    auto s = micron::uint_to_string_base<u64>(0xdeadbeef, 16, true);
    require_true(hstr_equal_cstr(s, "DEADBEEF"));
  }
  end_test_case();

  test_case("uint_to_string_base(8)");
  {
    auto s = micron::uint_to_string_base<u32>(8u, 8, false);
    require_true(hstr_equal_cstr(s, "10"));
    auto t = micron::uint_to_string_base<u32>(63u, 8, false);
    require_true(hstr_equal_cstr(t, "77"));
  }
  end_test_case();

  test_case("uint_to_string_base(2)");
  {
    auto s = micron::uint_to_string_base<u32>(0xAAu, 2, false);
    require_true(hstr_equal_cstr(s, "10101010"));
  }
  end_test_case();

  test_case("uint_to_string_base zero in every base");
  {
    auto h = micron::uint_to_string_base<u32>(0u, 16, false);
    auto o = micron::uint_to_string_base<u32>(0u, 8, false);
    auto b = micron::uint_to_string_base<u32>(0u, 2, false);
    auto d = micron::uint_to_string_base<u32>(0u, 10, false);
    require_true(hstr_equal_cstr(h, "0"));
    require_true(hstr_equal_cstr(o, "0"));
    require_true(hstr_equal_cstr(b, "0"));
    require_true(hstr_equal_cstr(d, "0"));
  }
  end_test_case();

  test_case("to_hex / to_oct / to_bin convenience aliases");
  {
    auto h = micron::to_hex<u32>(255u);
    auto o = micron::to_oct<u32>(8u);
    auto b = micron::to_bin<u32>(5u);
    require_true(hstr_equal_cstr(h, "ff"));
    require_true(hstr_equal_cstr(o, "10"));
    require_true(hstr_equal_cstr(b, "101"));
  }
  end_test_case();

  test_case("to_hex_fixed pads with leading zeros");
  {
    auto s = micron::to_hex_fixed<u32>(0xab, usize(8));
    require_true(hstr_equal_cstr(s, "000000ab"));
  }
  end_test_case();

  test_case("to_bin_fixed pads with leading zeros");
  {
    auto s = micron::to_bin_fixed<u32>(5u, usize(8));
    require_true(hstr_equal_cstr(s, "00000101"));
  }
  end_test_case();

  property_test(
      "uint_to_string_base(16) vs naive (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string_base<u64>(v, 16, false);
        char ref_buf[32];
        usize n = ref::naive_u64_to_hex(v, ref_buf, sizeof(ref_buf), false);
        require_true(hstr_equal_bytes(s, ref_buf, n));
      },
      10000);

  property_test(
      "uint_to_string_base(8) vs naive (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string_base<u64>(v, 8, false);
        char ref_buf[32];
        usize n = ref::naive_u64_to_oct(v, ref_buf, sizeof(ref_buf));
        require_true(hstr_equal_bytes(s, ref_buf, n));
      },
      10000);

  property_test(
      "uint_to_string_base(2) vs naive (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string_base<u64>(v, 2, false);
        char ref_buf[80];
        usize n = ref::naive_u64_to_bin(v, ref_buf, sizeof(ref_buf));
        require_true(hstr_equal_bytes(s, ref_buf, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // int_to_string_padded
  // ════════════════════════════════════════════════════════════════════

  test_case("int_to_string_padded zero-pads positive");
  {
    auto s = micron::int_to_string_padded<i32>(42, usize(6));
    require_true(hstr_equal_cstr(s, "000042"));
  }
  end_test_case();

  test_case("int_to_string_padded handles negative");
  {
    auto s = micron::int_to_string_padded<i32>(-42, usize(6));
    require_true(hstr_equal_cstr(s, "-00042"));
  }
  end_test_case();

  test_case("int_to_string_padded no-op when wide enough");
  {
    auto s = micron::int_to_string_padded<i32>(123456, usize(3));
    require_true(hstr_equal_cstr(s, "123456"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // string_to_int64 / string_to_uint64 — parse
  // ════════════════════════════════════════════════════════════════════

  test_case("string_to_int64 boundary values");
  {
    for ( usize i = 0; i < kSignedBoundariesCount; ++i ) {
      i64 v = kSignedBoundaries[i];
      auto s = micron::int_to_string(v);
      i64 parsed = micron::string_to_int64(s);
      require(parsed, v);
    }
  }
  end_test_case();

  test_case("string_to_uint64 boundary values");
  {
    for ( usize i = 0; i < kUnsignedBoundariesCount; ++i ) {
      u64 v = kUnsignedBoundaries[i];
      auto s = micron::uint_to_string(v);
      u64 parsed = micron::string_to_uint64(s);
      require(parsed, v);
    }
  }
  end_test_case();

  test_case("string_to_int64 with leading minus");
  {
    auto s = mk_hstring("-12345");
    require(micron::string_to_int64(s), i64(-12345));
  }
  end_test_case();

  test_case("string_to_int64 with leading plus");
  {
    auto s = mk_hstring("+12345");
    require(micron::string_to_int64(s), i64(12345));
  }
  end_test_case();

  test_case("string_to_int64 empty string returns 0");
  {
    auto s = mk_hstring("");
    require(micron::string_to_int64(s), i64(0));
  }
  end_test_case();

  property_test(
      "int → str → int round-trip (10k random i64)",
      [](u32 raw_hi, u32 raw_lo) {
        i64 v = (static_cast<i64>(raw_hi) << 32) | static_cast<i64>(raw_lo);
        auto s = micron::int_to_string(v);
        i64 parsed = micron::string_to_int64(s);
        require(parsed, v);
      },
      10000);

  property_test(
      "uint → str → uint round-trip (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string(v);
        u64 parsed = micron::string_to_uint64(s);
        require(parsed, v);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // hex / oct / bin string parsing
  // ════════════════════════════════════════════════════════════════════

  test_case("hex_string_to_uint64 basic");
  {
    auto s = mk_hstring("deadbeef");
    require(micron::hex_string_to_uint64(s), u64(0xdeadbeefu));
  }
  end_test_case();

  test_case("hex_string_to_uint64 with 0x prefix");
  {
    auto s = mk_hstring("0xdeadbeef");
    require(micron::hex_string_to_uint64(s), u64(0xdeadbeefu));
  }
  end_test_case();

  test_case("hex_string_to_uint64 uppercase");
  {
    auto s = mk_hstring("DEADBEEF");
    require(micron::hex_string_to_uint64(s), u64(0xdeadbeefu));
  }
  end_test_case();

  test_case("oct_string_to_uint64");
  {
    auto s = mk_hstring("77");
    require(micron::oct_string_to_uint64(s), u64(63));
  }
  end_test_case();

  test_case("bin_string_to_uint64");
  {
    auto s = mk_hstring("10101010");
    require(micron::bin_string_to_uint64(s), u64(0xAA));
  }
  end_test_case();

  property_test(
      "uint → hex_str → uint round-trip (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string_base<u64>(v, 16, false);
        u64 parsed = micron::hex_string_to_uint64(s);
        require(parsed, v);
      },
      10000);

  property_test(
      "uint → oct_str → uint round-trip (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string_base<u64>(v, 8, false);
        u64 parsed = micron::oct_string_to_uint64(s);
        require(parsed, v);
      },
      10000);

  property_test(
      "uint → bin_str → uint round-trip (10k random u64)",
      [](u32 raw_hi, u32 raw_lo) {
        u64 v = (static_cast<u64>(raw_hi) << 32) | static_cast<u64>(raw_lo);
        auto s = micron::uint_to_string_base<u64>(v, 2, false);
        u64 parsed = micron::bin_string_to_uint64(s);
        require(parsed, v);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // try_string_to_int64 / try_string_to_uint64 (non-throwing)
  // ════════════════════════════════════════════════════════════════════

  test_case("try_string_to_int64 success");
  {
    auto s = mk_hstring("12345");
    i64 out = 0;
    bool ok = micron::try_string_to_int64(s, out);
    require_true(ok);
    require(out, i64(12345));
  }
  end_test_case();

  test_case("try_string_to_int64 negative");
  {
    auto s = mk_hstring("-9999");
    i64 out = 0;
    bool ok = micron::try_string_to_int64(s, out);
    require_true(ok);
    require(out, i64(-9999));
  }
  end_test_case();

  test_case("try_string_to_int64 invalid");
  {
    auto s = mk_hstring("not a number");
    i64 out = 12345;
    bool ok = micron::try_string_to_int64(s, out);
    require_false(ok);
  }
  end_test_case();

  test_case("try_string_to_uint64 invalid");
  {
    auto s = mk_hstring("xyz");
    u64 out = 42;
    bool ok = micron::try_string_to_uint64(s, out);
    require_false(ok);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // parse_int / parse_uint / parse_hex (cursor-advancing)
  // ════════════════════════════════════════════════════════════════════

  test_case("parse_int advances cursor past digits");
  {
    const char *s = "12345abc";
    const char *p = s;
    i64 v = micron::parse_int(p, s + 8);
    require(v, i64(12345));
    require_true(p == s + 5);
  }
  end_test_case();

  test_case("parse_int handles negative");
  {
    const char *s = "-999end";
    const char *p = s;
    i64 v = micron::parse_int(p, s + 7);
    require(v, i64(-999));
    require_true(p == s + 4);
  }
  end_test_case();

  test_case("parse_uint advances cursor");
  {
    const char *s = "42!@#";
    const char *p = s;
    u64 v = micron::parse_uint(p, s + 5);
    require(v, u64(42));
    require_true(p == s + 2);
  }
  end_test_case();

  test_case("parse_hex advances cursor through hex digits");
  {
    const char *s = "feedZ";
    const char *p = s;
    i64 v = micron::parse_hex(p, s + 5);
    require(v, i64(0xfeed));
    require_true(p == s + 4);
  }
  end_test_case();

  property_test(
      "parse_int matches naive (10k random)",
      [](u32 raw) {
        i32 v = static_cast<i32>(raw);
        char buf[16];
        usize n = ref::naive_i64_to_dec(v, buf, sizeof(buf));
        const char *p = buf;
        i64 parsed = micron::parse_int(p, buf + n);
        require(parsed, static_cast<i64>(v));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // string_to_int_base / string_to_uint_base
  // ════════════════════════════════════════════════════════════════════

  test_case("string_to_int_base(16) on 'cafe'");
  {
    auto s = mk_hstring("cafe");
    require(micron::string_to_int_base(s, 16u), i64(0xcafe));
  }
  end_test_case();

  test_case("string_to_uint_base(2) on '110011'");
  {
    auto s = mk_hstring("110011");
    require(micron::string_to_uint_base(s, 2u), u64(51));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Narrower string_to_intN variants
  // ════════════════════════════════════════════════════════════════════

  test_case("string_to_int32 / int16");
  {
    require(micron::string_to_int32(mk_hstring("32767")), i32(32767));
    require(micron::string_to_int32(mk_hstring("-32768")), i32(-32768));
    require(micron::string_to_int16(mk_hstring("100")), i16(100));
    require(micron::string_to_int16(mk_hstring("-100")), i16(-100));
  }
  end_test_case();

  test_case("string_to_uint32 / uint16");
  {
    require(micron::string_to_uint32(mk_hstring("65535")), u32(65535));
    require(micron::string_to_uint16(mk_hstring("256")), u16(256));
  }
  end_test_case();

  test_case("try_parse_uint64 basics");
  {
    u64 out = 0;
    require_true(micron::try_parse_uint64("12345", 5, out));
    require(out, u64(12345));
    require_true(micron::try_parse_uint64("0", 1, out));
    require(out, u64(0));
    require_true(micron::try_parse_uint64("18446744073709551615", 20, out));
    require(out, u64(0xFFFFFFFFFFFFFFFFull));
    require_true(micron::try_parse_uint64("  42  ", 6, out));      // padding tolerated
    require(out, u64(42));
  }
  end_test_case();

  test_case("try_parse_uint64 rejects");
  {
    u64 out = 7;
    require_false(micron::try_parse_uint64("xyz", 3, out));
    require_false(micron::try_parse_uint64("-1", 2, out));                       // no sign
    require_false(micron::try_parse_uint64("12a", 3, out));                      // trailing junk
    require_false(micron::try_parse_uint64("", 0, out));                         // empty
    require_false(micron::try_parse_uint64(static_cast<const char *>(nullptr), 4, out));
    require_false(micron::try_parse_uint64("18446744073709551616", 20, out));    // overflow
  }
  end_test_case();

  test_case("try_parse_uint64 honours the length (no NUL needed)");
  {
    u64 out = 0;
    const char win[] = "1234abcd";
    require_true(micron::try_parse_uint64(win, 4, out));      // borrowed window, not a C string
    require(out, u64(1234));
    require_false(micron::try_parse_uint64(win, 5, out));      // 'a' is now in range
  }
  end_test_case();

  test_case("try_parse_int64 basics + rejects");
  {
    i64 out = 0;
    require_true(micron::try_parse_int64("12345", 5, out));
    require(out, i64(12345));
    require_true(micron::try_parse_int64("-9999", 5, out));
    require(out, i64(-9999));
    require_true(micron::try_parse_int64("-9223372036854775808", 20, out));
    require(out, i64(-0x7FFFFFFFFFFFFFFFLL - 1));
    require_false(micron::try_parse_int64("not a number", 12, out));
    require_false(micron::try_parse_int64("9223372036854775808", 19, out));      // +max overflow
    require_false(micron::try_parse_int64("-", 1, out));
    require_false(micron::try_parse_int64("", 0, out));
  }
  end_test_case();

  test_case("try_parse_hex64");
  {
    u64 out = 0;
    require_true(micron::try_parse_hex64("1a2b", 4, out));
    require(out, u64(0x1a2b));
    require_true(micron::try_parse_hex64("0xDEAD", 6, out));      // prefix tolerated
    require(out, u64(0xDEAD));
    require_true(micron::try_parse_hex64("FFFFFFFFFFFFFFFF", 16, out));
    require(out, u64(0xFFFFFFFFFFFFFFFFull));
    require_false(micron::try_parse_hex64("1g", 2, out));                     // bad digit
    require_false(micron::try_parse_hex64("0x", 2, out));                     // prefix only
    require_false(micron::try_parse_hex64("", 0, out));
    require_false(micron::try_parse_hex64("10000000000000000", 17, out));     // overflow
  }
  end_test_case();

  test_case("try_parse_hex_bytes");
  {
    u8 buf[4] = { 0, 0, 0, 0 };
    require_true(micron::try_parse_hex_bytes("0a1bff00", 8, buf, 4));
    require(buf[0], u8(0x0a));
    require(buf[1], u8(0x1b));
    require(buf[2], u8(0xff));
    require(buf[3], u8(0x00));
    require_true(micron::try_parse_hex_bytes("0A1BFF00", 8, buf, 4));      // case insensitive
    require(buf[1], u8(0x1b));
    require_false(micron::try_parse_hex_bytes("0a1bff", 6, buf, 4));       // too short
    require_false(micron::try_parse_hex_bytes("0a1bff0000", 10, buf, 4));  // too long
    require_false(micron::try_parse_hex_bytes("0a1bffzz", 8, buf, 4));     // bad digit
    require(buf[3], u8(0x00));                                             // untouched on reject
  }
  end_test_case();

  test_case("try_parse_* agree with the hstring spellings");
  {
    const char *cases[] = { "0", "1", "42", "-1", "-42", "12345", "18446744073709551615", "x", "", "  9  ", "1a" };
    for ( usize i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i ) {
      usize n = 0;
      while ( cases[i][n] ) ++n;
      auto h = mk_hstring(cases[i]);

      i64 a = 0, b = 0;
      bool oka = micron::try_string_to_int64(h, a);
      bool okb = micron::try_parse_int64(cases[i], n, b);
      require(oka, okb);
      if ( oka ) require(a, b);

      u64 c = 0, d = 0;
      bool okc = micron::try_string_to_uint64(h, c);
      bool okd = micron::try_parse_uint64(cases[i], n, d);
      require(okc, okd);
      if ( okc ) require(c, d);
    }
  }
  end_test_case();

  sb::print("=== FORMAT/INT RIGOR SUITE PASSED ===");
  return 1;
}
