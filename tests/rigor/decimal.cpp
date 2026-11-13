//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <micron/math/decimal.hpp>
#include <micron/string/conversions/parse_float.hpp>
#include <snowball/snowball.hpp>

namespace
{

constexpr bool
literal_format() noexcept
{
  char text[8]{};
  const auto result = micron::math::binary64_to_decimal(text, sizeof(text), f64{ 1.5 });
  return result && result.size == 3 && text[0] == '1' && text[1] == '.' && text[2] == '5';
}

void
roundtrip(u64 bits, u32 significant = 0)
{
  char text[micron::math::binary64_decimal_capacity]{};
  const auto number = __builtin_bit_cast(f64, bits);
  const auto result = significant ? micron::math::binary64_to_decimal(text, sizeof(text), number, significant)
                                  : micron::math::binary64_to_decimal(text, sizeof(text), number);
  sb::require_true(static_cast<bool>(result));
  sb::require_true(result.size && result.size <= sizeof(text));
  f64 parsed = 0;
  sb::require_true(micron::try_parse_double(text, result.size, parsed));
  sb::require(__builtin_bit_cast(u64, parsed), bits);
}

template<usize N>
void
spelling(f64 value, u32 digits, const char (&expected)[N])
{
  char output[micron::math::binary64_significant_decimal_capacity]{};
  const auto result = micron::math::binary64_to_decimal(output, sizeof(output), value, digits);
  sb::require_true(static_cast<bool>(result));
  sb::require(result.size, usize{ N - 1 });
  for ( usize i = 0; i < result.size; ++i ) sb::require(output[i], expected[i]);
}

constexpr bool
significant_constexpr() noexcept
{
  char text[8]{};
  const auto result = micron::math::binary64_to_decimal(text, sizeof(text), f64{ 1.25 }, 2);
  return result && result.size == 3 && text[0] == '1' && text[1] == '.' && text[2] == '2';
}

}      // namespace

static_assert(literal_format());
static_assert(significant_constexpr());

int
main()
{
  sb::test_case("exact binary64 decimal has independent literal and bounded output contracts");
  {
    char text[800]{};
    const auto result = micron::math::binary64_to_decimal(text, sizeof(text), 0.1);
    constexpr char expected[] = "1.000000000000000055511151231257827021181583404541015625e-1";
    sb::require(result.size, usize{ sizeof(expected) - 1 });
    for ( usize i = 0; i < result.size; ++i ) sb::require(text[i], expected[i]);

    constexpr struct {
      f64 number;
      u64 bits;
    } literals[] = {
      { 0.0, 0 },
      { -0.0, 0x8000000000000000ull },
      { 1.5, 0x3ff8000000000000ull },
      { 0.1, 0x3fb999999999999aull },
      { 2.2250738585072014e-308, 0x0010000000000000ull },
      { 4.9406564584124654e-324, 1 },
      { 1.7976931348623157e308, 0x7fefffffffffffffull },
    };

    for ( const auto &literal : literals ) {
      sb::require(__builtin_bit_cast(u64, literal.number), literal.bits);
      roundtrip(literal.bits);
    }
    text[0] = 'x';
    sb::require(micron::math::binary64_to_decimal(text, 1, 1.5).error, micron::math::decimal_error::short_output);
    sb::require(text[0], 'x');
    sb::require(micron::math::binary64_to_decimal(nullptr, 800, 1).error, micron::math::decimal_error::short_output);
    sb::require(micron::math::binary64_to_decimal(text, sizeof(text), __builtin_bit_cast(f64, u64{ 0x7ff0000000000000ull })).error,
                micron::math::decimal_error::nonfinite);
    sb::require(micron::math::binary64_to_decimal(text, sizeof(text), __builtin_bit_cast(f64, u64{ 0x7ff8000000000001ull })).error,
                micron::math::decimal_error::nonfinite);
    sb::require(text[0], 'x');
  }
  sb::end_test_case();

  sb::test_case("binary64 decimal covers every binade signs subnormals and formatter regression region");
  {
    u64 random = 0x41e20b738acd593full;
    for ( u64 exponent = 0; exponent < 2047; ++exponent ) {
      random = random * 6364136223846793005ull + 1442695040888963407ull;
      const auto bits = (exponent << 52) | (random & 0xfffffffffffffull);
      roundtrip(bits);
      roundtrip(bits | 0x8000000000000000ull);
    }
    for ( u64 i = 0; i < 65; ++i ) {
      roundtrip(0x2db34076d9def587ull + i);
      roundtrip(i);
      roundtrip(0xfffffffffffffull - i);
    }
  }
  sb::end_test_case();

  sb::test_case("significant decimal rounds exact ties to even carries and preserves failed outputs");
  {
    spelling(1.25, 2, "1.2");
    spelling(1.75, 2, "1.8");
    spelling(12.5, 2, "12");
    spelling(17.5, 2, "18");
    spelling(125.0, 2, "120");
    spelling(175.0, 2, "180");
    spelling(8.5, 1, "8");
    spelling(9.5, 1, "10");
    spelling(-1.25, 2, "-1.2");
    spelling(-1.75, 2, "-1.8");
    spelling(0.125, 2, "0.12");
    spelling(0.375, 2, "0.38");
    spelling(0.96875, 1, "1");
    spelling(0.96875, 2, "0.97");
    spelling(0.96875, 3, "0.969");
    spelling(0.96875, 4, "0.9688");
    spelling(0.1, 15, "0.1");
    spelling(0.1, 17, "0.10000000000000001");
    spelling(-0.0, 1, "-0");
    spelling(4.9406564584124654e-324, 17, "4.9406564584124654e-324");
    spelling(2.2250738585072014e-308, 17, "2.2250738585072014e-308");
    spelling(1.7976931348623157e308, 17, "1.7976931348623157e308");
    spelling(1.7976931348623157e308, 15, "1.79769313486232e308");
    char output[32]{};
    for ( char &byte : output ) byte = 'x';
    for ( u32 digits : { u32{ 0 }, u32{ 18 }, ~u32{ 0 } } ) {
      sb::require(micron::math::binary64_to_decimal(output, sizeof(output), 1.25, digits).error,
                  micron::math::decimal_error::invalid_precision);
      for ( const auto byte : output ) sb::require(byte, 'x');
    }
    sb::require(micron::math::binary64_to_decimal(output, 2, 1.25, 2).error, micron::math::decimal_error::short_output);
    sb::require(micron::math::binary64_to_decimal(nullptr, sizeof(output), 1.25, 2).error, micron::math::decimal_error::short_output);
    sb::require(micron::math::binary64_to_decimal(output, sizeof(output), __builtin_bit_cast(f64, u64{ 0x7ff0000000000000ull }), 17).error,
                micron::math::decimal_error::nonfinite);
    for ( const auto byte : output ) sb::require(byte, 'x');
  }
  sb::end_test_case();

  sb::test_case("seventeen significant decimal digits roundtrip all binades boundaries signs and regression values");
  {
    u64 random = 0xf16c0534b2e897daull;
    for ( u64 exponent = 0; exponent < 2047; ++exponent ) {
      random = random * 6364136223846793005ull + 1442695040888963407ull;
      const u64 points[] = { exponent << 52, (exponent << 52) | 1, (exponent << 52) | (random & 0xfffffffffffffull),
                             (exponent << 52) | 0xfffffffffffffull };
      for ( const auto bits : points ) {
        roundtrip(bits, 17);
        roundtrip(bits | 0x8000000000000000ull, 17);
      }
    }
    for ( u64 i = 0; i < 65; ++i ) {
      roundtrip(0x2db34076d9def587ull + i, 17);
      roundtrip(i, 17);
      roundtrip(0xfffffffffffffull - i, 17);
    }
    // Every permitted precision stays bounded and valid decimal syntax. Only
    // seventeen digits promise the original binary64 value after parsing.
    for ( u32 digits = 1; digits <= 17; ++digits ) {
      for ( f64 value : { 1.0, -0.0, 0.1, -0.000000123456789, 1e100, 1e-100 } ) {
        char output[micron::math::binary64_significant_decimal_capacity]{};
        const auto result = micron::math::binary64_to_decimal(output, sizeof(output), value, digits);
        sb::require_true(static_cast<bool>(result));
        sb::require_true(result.size <= sizeof(output));
        f64 parsed = 0;
        sb::require_true(micron::try_parse_double(output, result.size, parsed));
      }
    }
  }
  sb::end_test_case();
  return 1;
}
