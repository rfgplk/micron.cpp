//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/string/unistring.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstring>

int
main(void)
{
  sb::print("=== UNISTRING / FORMATTING TESTS ===");

  // ── with_capacity ─────────────────────────────────────────────────────────

  sb::test_case("with_capacity(n) - returns hstring with at least n capacity");
  {
    auto s = micron::with_capacity(64);
    sb::require(s.max_size() >= 64ULL);
  }
  sb::end_test_case();

  sb::test_case("with_capacity<N>() - template overload returns correct capacity");
  {
    auto s = micron::with_capacity<128>();
    sb::require(s.max_size() >= 128ULL);
  }
  sb::end_test_case();

  // ── invert ────────────────────────────────────────────────────────────────

  sb::test_case("invert - reverses a simple string");
  {
    micron::hstring<char> s = "hello";
    micron::invert(s);
    sb::require(s == "olleh");
  }
  sb::end_test_case();

  sb::test_case("invert - even length");
  {
    micron::hstring<char> s = "abcd";
    micron::invert(s);
    sb::require(s == "dcba");
  }
  sb::end_test_case();

  sb::test_case("invert - single char: unchanged");
  {
    micron::hstring<char> s = "x";
    micron::invert(s);
    sb::require(s == "x");
  }
  sb::end_test_case();

  sb::test_case("invert - empty string: no crash");
  {
    micron::hstring<char> s = "";
    micron::invert(s);
    sb::require(s == "");
  }
  sb::end_test_case();

  sb::test_case("invert - palindrome: unchanged");
  {
    micron::hstring<char> s = "racecar";
    micron::invert(s);
    sb::require(s == "racecar");
  }
  sb::end_test_case();

  // ── constexpr_int_to_string ───────────────────────────────────────────────

  sb::test_case("constexpr_int_to_string - zero");
  {
    auto s = micron::constexpr_int_to_string<int>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_int_to_string - positive");
  {
    auto s = micron::constexpr_int_to_string<int>(12345);
    sb::require(std::strcmp(s.c_str(), "12345") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_int_to_string - negative");
  {
    auto s = micron::constexpr_int_to_string<int>(-9876);
    sb::require(std::strcmp(s.c_str(), "-9876") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_int_to_string - INT_MAX");
  {
    auto s = micron::constexpr_int_to_string<int>(INT_MAX);
    sb::require(std::strcmp(s.c_str(), "2147483647") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_int_to_string - INT_MIN");
  {
    auto s = micron::constexpr_int_to_string<int>(INT_MIN);
    sb::require(std::strcmp(s.c_str(), "-2147483648") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_int_to_string - u64 large value");
  {
    auto s = micron::constexpr_int_to_string<u64>(18446744073709551615ULL);
    sb::require(std::strcmp(s.c_str(), "18446744073709551615") == 0);
  }
  sb::end_test_case();

  // ── constexpr_hex ─────────────────────────────────────────────────────────

  sb::test_case("constexpr_hex - zero");
  {
    auto s = micron::constexpr_hex<int>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_hex - 255 lowercase");
  {
    auto s = micron::constexpr_hex<int>(255);
    sb::require(std::strcmp(s.c_str(), "ff") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_hex - 255 uppercase");
  {
    auto s = micron::constexpr_hex<int>(255, true);
    sb::require(std::strcmp(s.c_str(), "FF") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_hex - 0xDEAD");
  {
    auto s = micron::constexpr_hex<u32>(0xDEAD, true);
    sb::require(std::strcmp(s.c_str(), "DEAD") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_hex - u64 max");
  {
    auto s = micron::constexpr_hex<u64>(0xFFFFFFFFFFFFFFFFULL, false);
    sb::require(std::strcmp(s.c_str(), "ffffffffffffffff") == 0);
  }
  sb::end_test_case();

  // ── constexpr_bin ─────────────────────────────────────────────────────────

  sb::test_case("constexpr_bin - zero");
  {
    auto s = micron::constexpr_bin<int>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_bin - 1");
  {
    auto s = micron::constexpr_bin<int>(1);
    sb::require(std::strcmp(s.c_str(), "1") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_bin - 5 (101)");
  {
    auto s = micron::constexpr_bin<int>(5);
    sb::require(std::strcmp(s.c_str(), "101") == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_bin - 255 (8 ones)");
  {
    auto s = micron::constexpr_bin<u8>(255);
    sb::require(std::strcmp(s.c_str(), "11111111") == 0);
  }
  sb::end_test_case();

  // ── unicode validation ────────────────────────────────────────────────────

  sb::test_case("u8_check - pure ASCII: returns end pointer");
  {
    const char *str = "hello";
    const char *r = micron::u8_check(str, 5);
    sb::require(r != nullptr);
    sb::require(r == str + 5);
  }
  sb::end_test_case();

  sb::test_case("u8_check - valid 2-byte sequence (é)");
  {
    const char str[] = "\xC3\xA9";     // U+00E9 é
    const char *r = micron::u8_check(str, 2);
    sb::require(r != nullptr);
  }
  sb::end_test_case();

  sb::test_case("u8_check - valid 3-byte sequence (€)");
  {
    const char str[] = "\xE2\x82\xAC";     // U+20AC €
    const char *r = micron::u8_check(str, 3);
    sb::require(r != nullptr);
  }
  sb::end_test_case();

  sb::test_case("u8_check - valid 4-byte sequence (𐍈)");
  {
    const char str[] = "\xF0\x90\x8D\x88";     // U+10348
    const char *r = micron::u8_check(str, 4);
    sb::require(r != nullptr);
  }
  sb::end_test_case();

  sb::test_case("u8_check - invalid lead byte: returns nullptr");
  {
    const char str[] = "\xFF\xFE";
    const char *r = micron::u8_check(str, 2);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("u8_check - surrogate half (U+D800): returns nullptr");
  {
    const char str[] = "\xED\xA0\x80";     // U+D800 — illegal in UTF-8
    const char *r = micron::u8_check(str, 3);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("u8_check - overlong 2-byte encoding: returns nullptr");
  {
    const char str[] = "\xC0\x80";     // overlong NUL
    const char *r = micron::u8_check(str, 2);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("u8_check - empty string: returns str (no-op)");
  {
    const char *str = "";
    const char *r = micron::u8_check(str, 0);
    sb::require(r == str);
  }
  sb::end_test_case();

  sb::test_case("u16_check - valid BMP code points: returns end");
  {
    const char16_t str[] = { u'A', u'B', u'C' };
    const char16_t *r = micron::u16_check(str, 3);
    sb::require(r != nullptr);
    sb::require(r == str + 3);
  }
  sb::end_test_case();

  sb::test_case("u16_check - valid surrogate pair");
  {
    const char16_t str[] = { 0xD800, 0xDC00 };     // U+10000
    const char16_t *r = micron::u16_check(str, 2);
    sb::require(r != nullptr);
  }
  sb::end_test_case();

  sb::test_case("u16_check - unpaired high surrogate: returns nullptr");
  {
    const char16_t str[] = { 0xD800, u'A' };
    const char16_t *r = micron::u16_check(str, 2);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("u16_check - lone low surrogate: returns nullptr");
  {
    const char16_t str[] = { 0xDC00 };
    const char16_t *r = micron::u16_check(str, 1);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("micron::u32_check - valid code points: returns end");
  {
    const char32_t str[] = { U'A', U'€', U'𐍈' };
    const char32_t *r = micron::u32_check(str, 3);
    sb::require(r != nullptr);
    sb::require(r == str + 3);
  }
  sb::end_test_case();

  sb::test_case("micron::u32_check - above U+10FFFF: returns nullptr");
  {
    const char32_t str[] = { static_cast<char32_t>(0x110000) };
    const char32_t *r = micron::u32_check(str, 1);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  // ── to_string (heap) ──────────────────────────────────────────────────────

  sb::test_case("to_string<int> - zero");
  {
    auto s = micron::to_string<int>(0);
    sb::require(s == "0");
  }
  sb::end_test_case();

  sb::test_case("to_string<int> - positive");
  {
    auto s = micron::to_string<int>(42);
    sb::require(s == "42");
  }
  sb::end_test_case();

  sb::test_case("to_string<int> - negative");
  {
    auto s = micron::to_string<int>(-999);
    sb::require(s == "-999");
  }
  sb::end_test_case();

  sb::test_case("to_string<int> - INT_MAX");
  {
    auto s = micron::to_string<int>(INT_MAX);
    sb::require(s == "2147483647");
  }
  sb::end_test_case();

  sb::test_case("to_string<int> - INT_MIN");
  {
    auto s = micron::to_string<int>(INT_MIN);
    sb::require(s == "-2147483648");
  }
  sb::end_test_case();

  sb::test_case("to_string<u64> - large value");
  {
    auto s = micron::to_string<u64>(18446744073709551615ULL);
    sb::require(s == "18446744073709551615");
  }
  sb::end_test_case();

  sb::test_case("to_string - c-string literal");
  {
    auto s = micron::to_string<char>("hello");
    sb::require(s == "hello");
  }
  sb::end_test_case();

  sb::test_case("to_string - char array");
  {
    const char arr[] = "world";
    auto s = micron::to_string(arr);
    sb::require(s == "world");
  }
  sb::end_test_case();

  sb::test_case("to_string - f32: parses back to approximate value");
  {
    auto s = micron::to_string<char>(1.5f);
    sb::require(s.size() > 0ULL);
    sb::require(s[0] == '1');
  }
  sb::end_test_case();

  sb::test_case("to_string - f64: parses back to approximate value");
  {
    auto s = micron::to_string<char>(3.14);
    sb::require(s.size() > 0ULL);
    sb::require(s[0] == '3');
  }
  sb::end_test_case();

  sb::test_case("to_string - f32 with precision 2");
  {
    auto s = micron::to_string<char>(3.14159f, 2u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  sb::test_case("to_string - f64 with precision 4");
  {
    auto s = micron::to_string<char>(2.71828, 4u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  // ── to_string_stack ───────────────────────────────────────────────────────

  sb::test_case("to_string_stack<int> - zero");
  {
    auto s = micron::to_string_stack<int>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack<int> - positive");
  {
    auto s = micron::to_string_stack<int>(12345);
    sb::require(std::strcmp(s.c_str(), "12345") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack<int> - negative");
  {
    auto s = micron::to_string_stack<int>(-42);
    sb::require(std::strcmp(s.c_str(), "-42") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack<u64> - large value");
  {
    auto s = micron::to_string_stack<u64>(9999999999ULL);
    sb::require(std::strcmp(s.c_str(), "9999999999") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack - c-string");
  {
    auto s = micron::to_string_stack<32, char>("test");
    sb::require(std::strcmp(s.c_str(), "test") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack - c-string truncates to Sz-1");
  {
    auto s = micron::to_string_stack<4, char>("hello");     // only 3 chars fit + null
    sb::require(s.size() <= 3ULL);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack - f32 shortest");
  {
    auto s = micron::to_string_stack<32>(1.0f);
    sb::require(s.size() > 0ULL);
    sb::require(s[0] == '1');
  }
  sb::end_test_case();

  sb::test_case("to_string_stack - f64 shortest");
  {
    auto s = micron::to_string_stack<32>(2.5);
    sb::require(s.size() > 0ULL);
    sb::require(s[0] == '2');
  }
  sb::end_test_case();

  sb::test_case("to_string_stack - f32 with precision");
  {
    auto s = micron::to_string_stack<48>(3.14159f, 2u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  sb::test_case("to_string_stack - f64 with precision");
  {
    auto s = micron::to_string_stack<64>(2.71828, 3u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  // ── float_to_string_stack / double_to_string_stack ────────────────────────

  sb::test_case("float_to_string_stack - 0.0");
  {
    auto s = micron::float_to_string_stack(0.0f);
    sb::require(s.size() > 0ULL);
    sb::require(s[0] == '0');
  }
  sb::end_test_case();

  sb::test_case("float_to_string_stack - 1.5");
  {
    auto s = micron::float_to_string_stack(1.5f);
    sb::require(s[0] == '1');
  }
  sb::end_test_case();

  sb::test_case("float_to_string_stack - with precision");
  {
    auto s = micron::float_to_string_stack(3.14159f, 2u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  sb::test_case("double_to_string_stack - 0.0");
  {
    auto s = micron::double_to_string_stack(0.0);
    sb::require(s[0] == '0');
  }
  sb::end_test_case();

  sb::test_case("double_to_string_stack - 1234.5678");
  {
    auto s = micron::double_to_string_stack(1234.5678);
    sb::require(s[0] == '1');
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  sb::test_case("double_to_string_stack - with precision 3");
  {
    auto s = micron::double_to_string_stack(3.14159265, 3u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  // ── to_fixed_stack ────────────────────────────────────────────────────────

  sb::test_case("to_fixed_stack - 1.0 prec 2: '1.00'");
  {
    auto s = micron::to_fixed_stack(1.0, 2u);
    sb::require(std::strncmp(s.c_str(), "1.00", 4) == 0);
  }
  sb::end_test_case();

  sb::test_case("to_fixed_stack - 3.14159 prec 2: '3.14'");
  {
    auto s = micron::to_fixed_stack(3.14159, 2u);
    sb::require(std::strncmp(s.c_str(), "3.14", 4) == 0);
  }
  sb::end_test_case();

  sb::test_case("to_fixed_stack - negative value");
  {
    auto s = micron::to_fixed_stack(-1.5, 1u);
    sb::require(s[0] == '-');
  }
  sb::end_test_case();

  sb::test_case("to_fixed_stack - zero prec 4: '0.0000'");
  {
    auto s = micron::to_fixed_stack(0.0, 4u);
    sb::require(std::strncmp(s.c_str(), "0.0000", 6) == 0);
  }
  sb::end_test_case();

  // ── to_scientific_stack ───────────────────────────────────────────────────

  sb::test_case("to_scientific_stack - 1.0: contains 'e' or 'E'");
  {
    auto s = micron::to_scientific_stack(1.0, 2u);
    bool has_exp = false;
    for ( usize i = 0; i < s.size(); ++i )
      if ( s[i] == 'e' || s[i] == 'E' )
        has_exp = true;
    sb::require(has_exp);
  }
  sb::end_test_case();

  sb::test_case("to_scientific_stack - 12345.6789: non-empty");
  {
    auto s = micron::to_scientific_stack(12345.6789, 3u);
    sb::require(s.size() > 0ULL);
  }
  sb::end_test_case();

  // ── to_general_stack ──────────────────────────────────────────────────────

  sb::test_case("to_general_stack - small value uses fixed form");
  {
    auto s = micron::to_general_stack(1.5, 6u);
    // fixed form — should not contain 'e'
    bool has_exp = false;
    for ( usize i = 0; i < s.size(); ++i )
      if ( s[i] == 'e' || s[i] == 'E' )
        has_exp = true;
    sb::require(!has_exp);
  }
  sb::end_test_case();

  sb::test_case("to_general_stack - very small value uses scientific form");
  {
    auto s = micron::to_general_stack(0.0000001, 6u);
    bool has_exp = false;
    for ( usize i = 0; i < s.size(); ++i )
      if ( s[i] == 'e' || s[i] == 'E' )
        has_exp = true;
    sb::require(has_exp);
  }
  sb::end_test_case();

  sb::test_case("to_general_stack - very large value uses scientific form");
  {
    auto s = micron::to_general_stack(1e20, 6u);
    bool has_exp = false;
    for ( usize i = 0; i < s.size(); ++i )
      if ( s[i] == 'e' || s[i] == 'E' )
        has_exp = true;
    sb::require(has_exp);
  }
  sb::end_test_case();

  // ── to_hex_stack ──────────────────────────────────────────────────────────

  sb::test_case("to_hex_stack - 0");
  {
    auto s = micron::to_hex_stack<u32>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_hex_stack - 255 lowercase");
  {
    auto s = micron::to_hex_stack<u32>(255u);
    sb::require(std::strcmp(s.c_str(), "ff") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_hex_stack - 255 uppercase");
  {
    auto s = micron::to_hex_stack<u32>(255u, true);
    sb::require(std::strcmp(s.c_str(), "FF") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_hex_stack - 0xDEADBEEF");
  {
    auto s = micron::to_hex_stack<u32>(0xDEADBEEFu, true);
    sb::require(std::strcmp(s.c_str(), "DEADBEEF") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_hex_stack - u64 0xCAFEBABE");
  {
    auto s = micron::to_hex_stack<u64>(0xCAFEBABEULL, false);
    sb::require(std::strcmp(s.c_str(), "cafebabe") == 0);
  }
  sb::end_test_case();

  // ── to_oct_stack ──────────────────────────────────────────────────────────

  sb::test_case("to_oct_stack - 0");
  {
    auto s = micron::to_oct_stack<u32>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_oct_stack - 8 -> '10'");
  {
    auto s = micron::to_oct_stack<u32>(8u);
    sb::require(std::strcmp(s.c_str(), "10") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_oct_stack - 255 -> '377'");
  {
    auto s = micron::to_oct_stack<u32>(255u);
    sb::require(std::strcmp(s.c_str(), "377") == 0);
  }
  sb::end_test_case();

  // ── to_bin_stack ──────────────────────────────────────────────────────────

  sb::test_case("to_bin_stack - 0");
  {
    auto s = micron::to_bin_stack<u32>(0);
    sb::require(std::strcmp(s.c_str(), "0") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_bin_stack - 1");
  {
    auto s = micron::to_bin_stack<u32>(1u);
    sb::require(std::strcmp(s.c_str(), "1") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_bin_stack - 10 -> '1010'");
  {
    auto s = micron::to_bin_stack<u32>(10u);
    sb::require(std::strcmp(s.c_str(), "1010") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_bin_stack - 255 -> '11111111'");
  {
    auto s = micron::to_bin_stack<u8>(255u);
    sb::require(std::strcmp(s.c_str(), "11111111") == 0);
  }
  sb::end_test_case();

  // ── to_hex_fixed_stack ────────────────────────────────────────────────────

  sb::test_case("to_hex_fixed_stack - 0xff with 4 digits: '00ff'");
  {
    auto s = micron::to_hex_fixed_stack<u32, 8>(0xFF, 4, false);
    sb::require(std::strcmp(s.c_str(), "00ff") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_hex_fixed_stack - 0xff with 8 digits uppercase: '000000FF'");
  {
    auto s = micron::to_hex_fixed_stack<u32, 12>(0xFF, 8, true);
    sb::require(std::strcmp(s.c_str(), "000000FF") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_hex_fixed_stack - 0 with 4 digits: '0000'");
  {
    auto s = micron::to_hex_fixed_stack<u32, 8>(0, 4);
    sb::require(std::strcmp(s.c_str(), "0000") == 0);
  }
  sb::end_test_case();

  // ── to_bin_fixed_stack ────────────────────────────────────────────────────

  sb::test_case("to_bin_fixed_stack - 5 with 8 digits: '00000101'");
  {
    auto s = micron::to_bin_fixed_stack<u8, 12>(5, 8);
    sb::require(std::strcmp(s.c_str(), "00000101") == 0);
  }
  sb::end_test_case();

  sb::test_case("to_bin_fixed_stack - 0 with 4 digits: '0000'");
  {
    auto s = micron::to_bin_fixed_stack<u8, 8>(0, 4);
    sb::require(std::strcmp(s.c_str(), "0000") == 0);
  }
  sb::end_test_case();

  // ── int_to_string_padded_stack ────────────────────────────────────────────

  sb::test_case("int_to_string_padded_stack - 5 padded to width 4: '0005'");
  {
    auto s = micron::int_to_string_padded_stack<int, 24>(5, 4);
    sb::require(std::strcmp(s.c_str(), "0005") == 0);
  }
  sb::end_test_case();

  sb::test_case("int_to_string_padded_stack - 1234 padded to width 4: '1234' (no pad)");
  {
    auto s = micron::int_to_string_padded_stack<int, 24>(1234, 4);
    sb::require(std::strcmp(s.c_str(), "1234") == 0);
  }
  sb::end_test_case();

  sb::test_case("int_to_string_padded_stack - 99 padded to width 6: '000099'");
  {
    auto s = micron::int_to_string_padded_stack<int, 24>(99, 6);
    sb::require(std::strcmp(s.c_str(), "000099") == 0);
  }
  sb::end_test_case();

  sb::test_case("int_to_string_padded_stack - negative: minus sign then padding");
  {
    auto s = micron::int_to_string_padded_stack<int, 24>(-7, 4);
    sb::require(s[0] == '-');
  }
  sb::end_test_case();

  sb::test_case("int_to_string_padded_stack - width 0: no padding, just digits");
  {
    auto s = micron::int_to_string_padded_stack<int, 24>(42, 0);
    sb::require(std::strcmp(s.c_str(), "42") == 0);
  }
  sb::end_test_case();

  // ── int_to_string_base_stack ──────────────────────────────────────────────

  sb::test_case("int_to_string_base_stack - base 10 matches to_string");
  {
    auto s = micron::int_to_string_base_stack<int>(12345, 10);
    sb::require(std::strcmp(s.c_str(), "12345") == 0);
  }
  sb::end_test_case();

  sb::test_case("int_to_string_base_stack - base 16 matches to_hex");
  {
    auto s = micron::int_to_string_base_stack<u32>(255, 16, false);
    sb::require(std::strcmp(s.c_str(), "ff") == 0);
  }
  sb::end_test_case();

  sb::test_case("int_to_string_base_stack - base 2 matches to_bin");
  {
    auto s = micron::int_to_string_base_stack<u32>(10, 2);
    sb::require(std::strcmp(s.c_str(), "1010") == 0);
  }
  sb::end_test_case();

  sb::test_case("int_to_string_base_stack - negative base 10");
  {
    auto s = micron::int_to_string_base_stack<int>(-1, 10);
    sb::require(std::strcmp(s.c_str(), "-1") == 0);
  }
  sb::end_test_case();

  // ── bytes_to_string ───────────────────────────────────────────────────────

  sb::test_case("bytes_to_string - same as to_string<int>");
  {
    auto s = micron::bytes_to_string<int>(256);
    sb::require(s == "256");
  }
  sb::end_test_case();

  sb::test_case("bytes_to_string_stack - same as to_string_stack<int>");
  {
    auto s = micron::bytes_to_string_stack<int>(256);
    sb::require(std::strcmp(s.c_str(), "256") == 0);
  }
  sb::end_test_case();

  // ── round-trip consistency checks ─────────────────────────────────────────

  sb::test_case("round-trip - to_string vs to_string_stack agree for int");
  {
    for ( int v : { 0, 1, -1, 100, -100, INT_MAX, INT_MIN } ) {
      auto hs = micron::to_string<int>(v);
      auto ss = micron::to_string_stack<int>(v);
      sb::require(std::strcmp(hs.c_str(), ss.c_str()) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("round-trip - to_hex_stack vs uint_to_string_base_stack base 16");
  {
    for ( u32 v : { 0u, 1u, 255u, 0xDEADu, 0xFFFFFFFFu } ) {
      auto h1 = micron::to_hex_stack<u32>(v, false);
      auto h2 = micron::uint_to_string_base_stack<u32>(v, 16, false);
      sb::require(std::strcmp(h1.c_str(), h2.c_str()) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("round-trip - to_bin_stack vs uint_to_string_base_stack base 2");
  {
    for ( u32 v : { 0u, 1u, 5u, 255u } ) {
      auto b1 = micron::to_bin_stack<u32>(v);
      auto b2 = micron::uint_to_string_base_stack<u32>(v, 2, false);
      sb::require(std::strcmp(b1.c_str(), b2.c_str()) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("round-trip - constexpr_hex matches to_hex_stack");
  {
    auto c = micron::constexpr_hex<u32>(0xABCDu, true);
    auto r = micron::to_hex_stack<u32>(0xABCDu, true);
    sb::require(std::strcmp(c.c_str(), r.c_str()) == 0);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}
