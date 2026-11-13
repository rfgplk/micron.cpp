//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../support/mock_allocators.hpp"
#include <micron/io/__serial_core.hpp>
#include <micron/io/canonical.hpp>
#include <micron/math/decimal.hpp>
#include <micron/string/conversions/parse_float.hpp>
#include <micron/vector/vector.hpp>
#include <snowball/snowball.hpp>
#if !defined(__micron_arch_wasm_any)
#include <micron/memory/mman.hpp>
#endif

namespace digits = micron::__impl::__digits;
namespace dec = micron::__impl::__dec;
namespace wire = micron::io::serialize::canonical;

namespace
{
u64 seed = 0x83a57f2d1946bec0ull;

u64
next_random()
{
  seed = seed * 6364136223846793005ull + 1442695040888963407ull;
  return seed;
}

void
decimal_oracle(u64 bits)
{
  const u64 exponent = (bits >> 52) & 2047;
  if ( exponent == 2047 ) return;
  u64 mantissa = bits & 0xfffffffffffffull;
  const i32 power = exponent ? static_cast<i32>(exponent) - 1075 : -1074;
  if ( exponent ) mantissa |= u64{ 1 } << 52;
  dec::__decimal original{}, actual{};
  dec::__dec_load_u64(original, mantissa);
  dec::__dec_shift(original, power);
  micron::math::__impl::__binary64_digits(actual, mantissa, power);
  sb::require(original.ndigits, actual.ndigits);
  sb::require(original.point, actual.point);
  sb::require_false(actual.truncated);
  for ( u32 i = 0; i < original.ndigits; ++i ) sb::require(original.d[i], actual.d[i]);
}
}      // namespace

int
main()
{
  sb::test_case("SIMD digit kernels match decimal arithmetic at every alignment");
  for ( usize alignment = 0; alignment < 16; ++alignment ) {
    byte output[40];
    char input[40];
    for ( usize iteration = 0; iteration < 2048; ++iteration ) {
      const u32 number = iteration < 100 ? static_cast<u32>(iteration) * 1000000 : static_cast<u32>(next_random() % 100000000);
      for ( byte &b : output ) b = 0xa5;
      digits::write8(output + alignment + 1, number);
      u32 oracle = number;
      for ( usize i = 8; i-- > 0; ) {
        sb::require(output[alignment + i + 1], static_cast<byte>(oracle % 10));
        oracle /= 10;
      }
      for ( usize i = 0; i <= alignment; ++i ) sb::require(output[i], byte{ 0xa5 });
      for ( usize i = alignment + 9; i < sizeof(output); ++i ) sb::require(output[i], byte{ 0xa5 });
      digits::ascii(input + alignment, output + alignment + 1, 8);
      u32 parsed = ~u32{ 0 };
      sb::require_true(digits::read8(input + alignment, parsed));
      sb::require(parsed, number);
    }
  }
  sb::end_test_case();

  sb::test_case("sixteen-byte prefixes compact a decimal point in every lane");
  {
    char text[32];
    for ( usize dot = 0; dot <= 16; ++dot )
      for ( usize alignment = 0; alignment < 16; ++alignment ) {
        u64 expected = 0;
        for ( usize j = 0; j < 16; ++j ) {
          text[alignment + j] = j == dot ? '.' : static_cast<char>('0' + next_random() % 10);
          if ( j != dot ) expected = expected * 10 + (text[alignment + j] - '0');
        }
        digits::__prefix16 actual{};
        sb::require_true(digits::read16(text + alignment, actual));
        sb::require(actual.mantissa, expected);
        sb::require(actual.point, static_cast<u32>(dot));
        for ( usize position = 0; position < 16; ++position ) {
          const char saved = text[alignment + position];
          for ( u32 byte = 0; byte < 256; ++byte ) {
            text[alignment + position] = static_cast<char>(byte);
            digits::__prefix16 oracle{ 0xdeadbeef, 77 }, result = oracle;
            const bool wanted = digits::__scalar16(text + alignment, oracle);
            sb::require(digits::read16(text + alignment, result), wanted);
            sb::require(result.mantissa, oracle.mantissa);
            sb::require(result.point, oracle.point);
          }
          text[alignment + position] = saved;
        }
      }
  }
  sb::end_test_case();

  sb::test_case("eight-byte validation rejects every non-digit in every lane without publishing output");
  char validation_input[8] = { '1', '2', '3', '4', '5', '6', '7', '8' };
  for ( usize position = 0; position < 8; ++position ) {
    const char saved = validation_input[position];
    for ( u32 value = 0; value < 256; ++value ) {
      validation_input[position] = static_cast<char>(value);
      u32 result = 0xdeadbeef;
      const bool ok = digits::read8(validation_input, result);
      sb::require(ok, value >= '0' && value <= '9');
      if ( !ok ) sb::require(result, u32{ 0xdeadbeef });
    }
    validation_input[position] = saved;
  }
  sb::end_test_case();

  sb::test_case("bounded SIMD parsing agrees with the original unbounded scalar scanner");
  {
    namespace pf = micron::__impl::__pf;
    char text[801];
    for ( usize i = 0; i < 4096; ++i ) {
      const u64 bits = next_random();
      if ( ((bits >> 52) & 2047) == 2047 ) continue;
      const auto result = micron::math::binary64_to_decimal(text, 800, __builtin_bit_cast(f64, bits));
      sb::require_true(static_cast<bool>(result));
      text[result.size] = 0;
      for ( usize iteration = 0; iteration < 3; ++iteration ) {
        const usize position = static_cast<usize>(next_random() % result.size);
        const char saved = text[position];
        if ( iteration ) text[position] = iteration == 1 ? '.' : 'e';
        f64 bounded = 0, scalar = 0;
        const char *bounded_stop, *scalar_stop;
        const auto actual = pf::__parse<f64, true>(text, text + result.size, bounded, bounded_stop);
        const auto expected = pf::__parse<f64, false>(text, static_cast<const char *>(nullptr), scalar, scalar_stop);
        sb::require(actual, expected);
        sb::require(__builtin_bit_cast(u64, bounded), __builtin_bit_cast(u64, scalar));
        sb::require(bounded_stop - text, scalar_stop - text);
        text[position] = saved;
      }
    }
  }
  sb::end_test_case();

  sb::test_case("binary64 block conversion agrees with the original binary shift oracle");
  for ( u64 exponent = 0; exponent < 2047; ++exponent ) {
    decimal_oracle(exponent << 52);
    decimal_oracle((exponent << 52) | 1);
    decimal_oracle((exponent << 52) | 0xfffffffffffffull);
    decimal_oracle((exponent << 52) | (next_random() & 0xfffffffffffffull));
  }
  for ( usize i = 0; i < 4096; ++i ) decimal_oracle(next_random());
  sb::end_test_case();

  sb::test_case("format failures preserve every destination byte and decimal text parses bit-exactly");
  for ( usize i = 0; i < 1024; ++i ) {
    const u64 bits = next_random();
    if ( ((bits >> 52) & 2047) == 2047 ) continue;
    char text[832];
    for ( char &c : text ) c = '~';
    const auto value = __builtin_bit_cast(f64, bits);
    const usize offset = i % 16;
    const auto result = micron::math::binary64_to_decimal(text + offset, 800, value);
    sb::require_true(static_cast<bool>(result));
    f64 parsed = 0;
    sb::require_true(micron::try_parse_double(text + offset, result.size, parsed));
    sb::require(__builtin_bit_cast(u64, parsed), bits);
    for ( usize j = 0; j < offset; ++j ) sb::require(text[j], '~');
    for ( usize j = offset + result.size; j < sizeof(text); ++j ) sb::require(text[j], '~');
    for ( char &c : text ) c = '~';
    const auto failed = micron::math::binary64_to_decimal(text + offset, result.size - 1, value);
    sb::require(failed.error, micron::math::decimal_error::short_output);
    for ( const char c : text ) sb::require(c, '~');
  }
  sb::end_test_case();

  sb::test_case("repeated binary deserialization retains capacity and performs no allocations");
  using alloc = mtest::tracking_allocator<191>;
  using vector = micron::vector<u64, alloc>;
  vector input, output;
  input.reserve(256);
  output.reserve(256);
  for ( usize i = 0; i < 256; ++i ) input.push_back(next_random());
  byte frame[2064];
  for ( usize size : { usize{ 0 }, usize{ 1 }, usize{ 7 }, usize{ 256 }, usize{ 16 }, usize{ 0 }, usize{ 256 } } ) {
    input.set_size(size);
    const auto length = micron::io::serialize::frame_into(frame, sizeof(frame), input);
    sb::require_true(length >= 0);
    const usize allocations = alloc::allocations;
    sb::require(micron::io::serialize::unframe_from(frame, static_cast<usize>(length), output), length);
    sb::require(alloc::allocations, allocations);
    sb::require(output.size(), size);
    for ( usize i = 0; i < size; ++i ) sb::require(output[i], input[i]);
  }
  sb::end_test_case();

#if !defined(__micron_arch_wasm_any)
  sb::test_case("digit parsing rendering and canonical reads stop at inaccessible page boundaries");
  constexpr usize page = 65536;
  auto *mapping
      = reinterpret_cast<byte *>(micron::mmap(nullptr, page * 3, micron::prot_none, micron::map_private | micron::map_anonymous, -1, 0));
  sb::require_true(!micron::mmap_failed(mapping));
  sb::require(micron::mprotect(mapping + page, page, micron::prot_read | micron::prot_write), 0);
  byte *end = mapping + page * 2;
  digits::write8(end - 8, 12345678);
  digits::ascii(reinterpret_cast<char *>(end - 8), end - 8, 8);
  u32 parsed = 0;
  sb::require_true(digits::read8(reinterpret_cast<char *>(end - 8), parsed));
  sb::require(parsed, u32{ 12345678 });
  for ( usize count = 1; count <= 40; ++count ) {
    for ( usize i = 0; i < count; ++i ) end[-static_cast<ssize_t>(i) - 1] = '1';
    f64 value = 0;
    sb::require_true(micron::try_parse_double(reinterpret_cast<char *>(end - count), count, value));
    if ( count >= 16 ) {
      digits::__prefix16 prefix;
      sb::require_true(digits::read16(reinterpret_cast<char *>(end - 16), prefix));
    }
    wire::reader reader(end - count, count);
    u64 scalar = 0xdeadbeef;
    sb::require(reader.get_u64(scalar), count >= 8);
    if ( count < 8 ) sb::require(scalar, u64{ 0xdeadbeef });
  }
  sb::require(micron::munmap(reinterpret_cast<addr_t *>(mapping), page * 3), 0);
  sb::end_test_case();
#endif
  return 1;
}
