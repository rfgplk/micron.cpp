//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../support/mock_allocators.hpp"
#include <micron/io/__serial_core.hpp>
#include <micron/io/canonical.hpp>
#if !defined(__micron_arch_wasm_any)
#include <micron/math/symbolic.hpp>
#endif
#include <snowball/snowball.hpp>

namespace ser = micron::io::serialize;
#if !defined(__micron_arch_wasm_any)
namespace sym = micron::math::symbolic;
#endif

namespace
{
struct sink {
  char bytes[8192]{};
  usize size = 0;

  bool
  operator()(const char *p, usize n)
  {
    if ( n > sizeof(bytes) - size ) return false;
    for ( usize i = 0; i < n; ++i ) bytes[size++] = p[i];
    return true;
  }
};

void
same(const sink &a, const sink &b)
{
  sb::require(a.size, b.size);
  for ( usize i = 0; i < a.size; ++i ) sb::require(a.bytes[i], b.bytes[i]);
}
}      // namespace

int
main()
{
  sb::test_case("strings and nested string vectors reuse destination storage");
  {
    using alloc = mtest::tracking_allocator<192>;
    using text = micron::hstring<char, true, alloc>;
    using list = micron::vector<text, alloc>;
    list input, output;
    input.resize(8);
    output.resize(8);
    for ( usize i = 0; i < 8; ++i ) {
      input[i].reserve(513);
      output[i].reserve(513);
    }
    byte frame[8192];
    for ( usize length : { usize{ 0 }, usize{ 1 }, usize{ 511 }, usize{ 16 }, usize{ 512 }, usize{ 0 } } ) {
      for ( usize i = 0; i < 8; ++i ) {
        input[i].fast_clear();
        for ( usize j = 0; j < length; ++j ) input[i].push_back(static_cast<char>(i * 17 + j));
      }
      const auto bytes = ser::frame_into(frame, sizeof(frame), input);
      sb::require(bytes, static_cast<max_t>(16 + 8 * (8 + length)));
      const usize allocations = alloc::allocations;
      sb::require(ser::unframe_from(frame, static_cast<usize>(bytes), output), bytes);
      sb::require(alloc::allocations, allocations);
      sb::require(output.size(), input.size());
      for ( usize i = 0; i < output.size(); ++i ) {
        sb::require(output[i].size(), length);
        sb::require(output[i].c_str()[length], '\0');
        for ( usize j = 0; j < length; ++j ) sb::require(output[i][j], input[i][j]);
      }
    }
    const usize allocations = alloc::allocations;
    for ( usize i = 8; i < 16; ++i ) frame[i] = 255;
    sb::require(ser::unframe_from(frame, sizeof(frame), output), -static_cast<max_t>(micron::error::invalid_arg));
    sb::require(alloc::allocations, allocations);
    byte padded[528]{};
    for ( usize i = 0; i < 8; ++i ) padded[i] = frame[i];
    ser::canonical::writer hostile(padded + 8, sizeof(padded) - 8);
    sb::require_true(hostile.put_u64(512));
    sb::require_true(hostile.put_u64(~u64{ 0 }));
    const usize original_size = output.size();
    sb::require(ser::unframe_from(padded, sizeof(padded), output), -static_cast<max_t>(micron::error::invalid_arg));
    sb::require(alloc::allocations, allocations);
    sb::require(output.size(), original_size);
  }
  sb::end_test_case();

  sb::test_case("canonical scalars preserve little endian encoding at every byte alignment");
  {
    byte memory[80];
    for ( usize alignment = 0; alignment < 32; ++alignment ) {
      for ( byte &b : memory ) b = 0xa5;
      ser::canonical::writer out(memory + alignment, 14);
      sb::require_true(out.put_u16(0x1234));
      sb::require_true(out.put_u32(0x89abcdef));
      sb::require_true(out.put_u64(0x0123456789abcdefull));
      constexpr byte expected[14] = { 0x34, 0x12, 0xef, 0xcd, 0xab, 0x89, 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 };
      for ( usize i = 0; i < sizeof(memory); ++i )
        sb::require(memory[i], i >= alignment && i < alignment + 14 ? expected[i - alignment] : byte{ 0xa5 });
      for ( usize length = 0; length <= 14; ++length ) {
        ser::canonical::reader in(memory + alignment, length);
        u16 a = 0;
        u32 b = 0;
        u64 c = 0xcafe;
        sb::require(in.get_u16(a), length >= 2);
        if ( length < 2 ) continue;
        sb::require(in.get_u32(b), length >= 6);
        if ( length < 6 ) continue;
        sb::require(in.get_u64(c), length == 14);
        if ( length == 14 )
          sb::require(c, u64{ 0x0123456789abcdefull });
        else
          sb::require(c, u64{ 0xcafe });
      }
    }
  }
  sb::end_test_case();

#if !defined(__micron_arch_wasm_any)
  sb::test_case("symbolic output uses local or caller scratch and handles large coefficients");
  {
    sym::limits limits;
    limits.max_nodes = 64;
    limits.max_symbols = 8;
    limits.max_text_bytes = 64;
    sym::arena arena(limits);
    const auto x = arena.symbol({ "x", 1 });
    const auto expression = arena.add(arena.power(x.id, 9).id, arena.integer(-123456789).id);
    sb::require_true(static_cast<bool>(expression));
    sink original;
    sb::require(arena.write_csexp(expression.id, original), sym::error::none);
    char scratch[8192];
    sink external;
    sb::require(arena.write_csexp(expression.id, external, scratch, sizeof(scratch)), sym::error::none);
    same(original, external);
    for ( usize capacity = 0; capacity < 65; ++capacity ) {
      sink small;
      sb::require(arena.write_csexp(expression.id, small, scratch, capacity), sym::error::output_limit);
    }
    sink invalid;
    sb::require(arena.write_csexp(expression.id, invalid, nullptr, 10), sym::error::output_limit);
    sb::require(arena.write_csexp(sym::invalid_node, invalid, scratch, sizeof(scratch)), sym::error::invalid_node);

#if defined(MICRON_ABC_STATS) && !defined(__micron_arch_wasm_any)
    const auto before = abc::get_stats();
    for ( usize i = 0; i < 32; ++i ) {
      external.size = 0;
      const auto code = arena.write_csexp(expression.id, external, scratch, sizeof(scratch));
      sb::require(code, sym::error::none);
    }
    const auto after = abc::get_stats();
    sb::require_true(before.enabled && after.enabled);
    sb::require(after.alloc_requests, before.alloc_requests);
#endif

    micron::math::arbint<> numerator(1);
    numerator <<= 4096;
    const auto rational = micron::math::make_rational(numerator, micron::math::arbint<>(3));
    sb::require_true(static_cast<bool>(rational));
    const auto big = arena.constant(rational.value);
    sb::require_true(static_cast<bool>(big));
    sink a, b;
    sb::require(arena.write_csexp(big.id, a), sym::error::none);
    sb::require(arena.write_csexp(big.id, b, scratch, sizeof(scratch)), sym::error::none);
    same(a, b);
    sink short_buffer;
    sb::require(arena.write_csexp(big.id, short_buffer, scratch, 128), sym::error::output_limit);
    limits.rational.max_limbs = ~usize{ 0 };
    limits.rational.max_work_limbs = ~usize{ 0 };
    sym::arena overflow_limits(limits);
    const auto symbol = overflow_limits.symbol({ "x", 1 });
    sb::require_true(static_cast<bool>(symbol));
    sink bounded;
    sb::require(overflow_limits.write_csexp(symbol.id, bounded), sym::error::limb_limit);
    sb::require(overflow_limits.write_csexp(symbol.id, bounded, scratch, sizeof(scratch)), sym::error::limb_limit);
    sb::require(bounded.size, usize{ 0 });
  }
  sb::end_test_case();
#endif
  return 1;
}
