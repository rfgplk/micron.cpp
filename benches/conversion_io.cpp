//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/__serial_core.hpp"
#include "../src/io/canonical.hpp"
#include "../src/io/console.hpp"
#if defined(__micron_arch_wasm_any)
#include "../src/bits/wasm/time.hpp"
#else
#include "../src/linux/sys/time.hpp"
#endif
#include "../src/math/decimal.hpp"
#if !defined(__micron_arch_wasm_any)
#include "../src/math/symbolic.hpp"
#endif
#include "../src/string/conversions/parse_float.hpp"
#include "../src/vector/vector.hpp"
#include "../tests/support/mock_allocators.hpp"

namespace
{
namespace wire = micron::io::serialize::canonical;
constexpr usize count = 256;
f64 numbers[count];
char texts[count][800];
usize lengths[count];
alignas(64) byte payload[65536], frame[65552];
char decimal_output[800];
volatile u64 checksum = 0;
int control_fd = -1, acknowledge_fd = -1;
using alloc = mtest::tracking_allocator<190>;
using sequence = micron::vector<u64, alloc>;

bool
equal(const char *a, const char *b)
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

u64
now()
{
  micron::timespec_t value{};
  micron::clock_gettime(micron::clock_monotonic, value);
  return static_cast<u64>(value.tv_sec) * 1000000000ull + value.tv_nsec;
}

u64
cpu_now()
{
#if defined(__micron_arch_wasm_any)
  return now();
#else
  micron::timespec_t value{};
  micron::clock_gettime(micron::clock_thread_cputime_id, value);
  return static_cast<u64>(value.tv_sec) * 1000000000ull + value.tv_nsec;
#endif
}

usize
allocation_count()
{
#if defined(MICRON_ABC_STATS) && !defined(__micron_arch_wasm_any)
  return abc::get_stats().alloc_requests;
#else
  return alloc::allocations;
#endif
}

void
counters(bool enabled)
{
#if !defined(__micron_arch_wasm_any)
  if ( control_fd < 0 ) return;
  const char *command = enabled ? "enable\n" : "disable\n";
  micron::posix::write(control_fd, command, enabled ? 7 : 8);
  char acknowledgement[4];
  usize received = 0;
  while ( received < sizeof(acknowledgement) ) {
    const auto n = micron::posix::read(acknowledge_fd, acknowledgement + received, sizeof(acknowledgement) - received);
    if ( n <= 0 ) micron::abort();
    received += static_cast<usize>(n);
  }
#else
  (void)enabled;
#endif
}

template<typename Fn>
void
measure(const char *name, usize repeats, usize bytes, Fn fn)
{
  for ( usize i = 0; i < count; ++i ) checksum = checksum ^ fn(i);
  const usize allocations = allocation_count();
  u64 sum = 0;
  counters(true);
  const u64 start = now();
  const u64 cpu_start = cpu_now();
  for ( usize r = 0; r < repeats; ++r )
    for ( usize i = 0; i < count; ++i ) {
      asm volatile("" ::: "memory");
      sum += fn(i);
      asm volatile("" ::: "memory");
    }
  const u64 cpu_elapsed = cpu_now() - cpu_start;
  const u64 elapsed = now() - start;
  counters(false);
  checksum = sum;
  micron::io::println(name, ",", repeats * count, ",", repeats * bytes, ",", elapsed, ",", allocation_count() - allocations, ",", sum, ",",
                      cpu_elapsed);
}

#if !defined(__micron_arch_wasm_any)
template<typename Owner, typename Sink>
auto
symbolic_with_scratch(const Owner &owner, micron::math::symbolic::node_id id, Sink &&sink, char *scratch, usize capacity)
{
  if constexpr ( requires { owner.write_csexp(id, sink, scratch, capacity); } )
    return owner.write_csexp(id, sink, scratch, capacity);
  else
    return owner.write_csexp(id, sink);
}
#endif

void
initialize(bool wide)
{
  u64 seed = 0x83a57f2d1946bec0ull;
  for ( usize i = 0; i < count; ++i ) {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    const u64 exponent = wide ? (i * 2046 / (count - 1)) : 1010 + i % 30;
    numbers[i] = __builtin_bit_cast(f64, (seed & 0x800fffffffffffffull) | (exponent << 52));
  }
  for ( usize i = 0; i < sizeof(payload); ++i ) payload[i] = static_cast<byte>(i * 73 + 17);
}
}      // namespace

int
main(int argc, char **argv)
{
  const char *selected = argc > 1 ? argv[1] : "all";
  usize repeats = 32;
  if ( argc > 2 ) {
    repeats = 0;
    for ( const char *p = argv[2]; *p >= '0' && *p <= '9'; ++p ) repeats = repeats * 10 + (*p - '0');
    if ( !repeats || repeats > 1000000 ) return 2;
  }
  if ( argc > 4 ) {
    control_fd = acknowledge_fd = 0;
    for ( const char *p = argv[3]; *p >= '0' && *p <= '9'; ++p ) control_fd = control_fd * 10 + (*p - '0');
    for ( const char *p = argv[4]; *p >= '0' && *p <= '9'; ++p ) acknowledge_fd = acknowledge_fd * 10 + (*p - '0');
  }
  const auto selected_case = [&](const char *name) { return equal(selected, "all") || equal(selected, name); };
  for ( bool wide : { false, true } ) {
    initialize(wide);
    const char *exact = wide ? "decimal_exact_wide" : "decimal_exact_common";
    const char *precision = wide ? "decimal_17_wide" : "decimal_17_common";
    if ( selected_case(exact) )
      measure(exact, repeats, 0, [&](usize i) {
        const auto r = micron::math::binary64_to_decimal(decimal_output, sizeof(decimal_output), numbers[i]);
        return r.size + static_cast<u8>(decimal_output[r.size ? r.size - 1 : 0]);
      });
    if ( selected_case(precision) )
      measure(precision, repeats, 0, [&](usize i) {
        const auto r = micron::math::binary64_to_decimal(decimal_output, sizeof(decimal_output), numbers[i], 17);
        return r.size + static_cast<u8>(decimal_output[r.size ? r.size - 1 : 0]);
      });
    usize bytes = 0;
    for ( usize i = 0; i < count; ++i ) {
      lengths[i] = micron::math::binary64_to_decimal(texts[i], sizeof(texts[i]), numbers[i], 17).size;
      bytes += lengths[i];
    }
    const char *parse = wide ? "parse_17_wide" : "parse_17_common";
    if ( selected_case(parse) )
      measure(parse, repeats, bytes, [&](usize i) {
        f64 value = 0;
        const bool ok = micron::try_parse_double(texts[i], lengths[i], value);
        return __builtin_bit_cast(u64, value) ^ static_cast<u64>(ok);
      });
    bytes = 0;
    for ( usize i = 0; i < count; ++i ) {
      lengths[i] = micron::math::binary64_to_decimal(texts[i], sizeof(texts[i]), numbers[i]).size;
      bytes += lengths[i];
    }
    parse = wide ? "parse_exact_wide" : "parse_exact_common";
    if ( selected_case(parse) )
      measure(parse, repeats, bytes, [&](usize i) {
        f64 value = 0;
        const bool ok = micron::try_parse_double(texts[i], lengths[i], value);
        return __builtin_bit_cast(u64, value) ^ static_cast<u64>(ok);
      });
  }
  for ( usize size : { usize{ 64 }, usize{ 4096 }, usize{ 65536 } } ) {
    const char *name = size == 64 ? "frame_64" : size == 4096 ? "frame_4096" : "frame_65536";
    if ( selected_case(name) )
      measure(name, repeats, count * size, [&](usize i) {
        const auto r = wire::frame_bytes(frame, sizeof(frame), payload, size);
        return static_cast<u64>(r) + frame[16 + i % size];
      });
    (void)wire::frame_bytes(frame, sizeof(frame), payload, size);
    name = size == 64 ? "view_64" : size == 4096 ? "view_4096" : "view_65536";
    if ( selected_case(name) )
      measure(name, repeats, count * size, [&](usize i) {
        wire::byte_view view;
        const auto r = wire::unframe_bytes(frame, size + 16, view);
        return static_cast<u64>(r) + view.data[i % size];
      });
  }
  if ( selected_case("scalar_write") )
    measure("scalar_write", repeats, count * 2048, [&](usize i) {
      wire::writer out(frame + i % 16, 2048);
      for ( usize j = 0; j < 256; ++j ) out.put_u64(j + i);
      return out.position() + frame[i % 16 + i];
    });
  if ( selected_case("scalar_read") )
    measure("scalar_read", repeats, count * 2048, [&](usize i) {
      wire::reader in(payload + i % 16, 2048);
      u64 sum = 0, value;
      for ( usize j = 0; j < 256; ++j ) {
        in.get_u64(value);
        sum += value;
      }
      return sum;
    });
  sequence input, output;
  input.reserve(512);
  for ( usize i = 0; i < 512; ++i ) input.push_back(i * 0x9e3779b97f4a7c15ull);
  const auto framed = micron::io::serialize::frame_into(frame, sizeof(frame), input);
  if ( selected_case("vector_frame") )
    measure("vector_frame", repeats, count * 4096,
            [&](usize i) { return static_cast<u64>(micron::io::serialize::frame_into(frame, sizeof(frame), input)) + frame[i]; });
  if ( selected_case("vector_unframe") )
    measure("vector_unframe", repeats, count * 4096, [&](usize i) {
      const auto r = micron::io::serialize::unframe_from(frame, static_cast<usize>(framed), output);
      return static_cast<u64>(r) + output[i];
    });
  using text = micron::hstring<char, true, alloc>;
  text string_input, string_output;
  string_input.reserve(4097);
  string_output.reserve(4097);
  for ( usize i = 0; i < 4096; ++i ) string_input.push_back(static_cast<char>('a' + i % 26));
  const auto string_frame = micron::io::serialize::frame_into(frame, sizeof(frame), string_input);
  if ( selected_case("string_unframe") )
    measure("string_unframe", repeats, count * 4096, [&](usize i) {
      return static_cast<u64>(micron::io::serialize::unframe_from(frame, static_cast<usize>(string_frame), string_output))
             + static_cast<u8>(string_output[i]);
    });
  micron::vector<text, alloc> nested_input, nested_output;
  nested_input.resize(16);
  nested_output.resize(16);
  for ( usize i = 0; i < 16; ++i )
    for ( usize j = 0; j < 64; ++j ) nested_input[i].push_back(static_cast<char>('a' + (i + j) % 26));
  const auto nested_frame = micron::io::serialize::frame_into(frame, sizeof(frame), nested_input);
  if ( selected_case("nested_unframe") )
    measure("nested_unframe", repeats, count * 1024, [&](usize i) {
      return static_cast<u64>(micron::io::serialize::unframe_from(frame, static_cast<usize>(nested_frame), nested_output))
             + static_cast<u8>(nested_output[i % 16][i % 64]);
    });
#if !defined(__micron_arch_wasm_any)
  if ( selected_case("symbolic_write") || selected_case("symbolic_write_scratch") ) {
    namespace sym = micron::math::symbolic;
    sym::limits limits;
    limits.max_nodes = 128;
    limits.max_symbols = 16;
    limits.max_text_bytes = 256;
    sym::arena arena(limits);
    const auto x = arena.symbol({ "coordinate", 10 });
    const auto expression = arena.add(arena.power(x.id, 7).id, arena.integer(123456789).id);
    const auto emit_expression = [&](bool reuse) {
      usize used = 0;
      auto sink = [&](const char *p, usize n) {
        if ( n > sizeof(decimal_output) - used ) return false;
        for ( usize j = 0; j < n; ++j ) decimal_output[used++] = p[j];
        return true;
      };
      char scratch[256];
      const auto code
          = reuse ? symbolic_with_scratch(arena, expression.id, sink, scratch, sizeof(scratch)) : arena.write_csexp(expression.id, sink);
      return used + static_cast<u64>(code);
    };
    if ( selected_case("symbolic_write") ) measure("symbolic_write", repeats, 0, [&](usize) { return emit_expression(false); });
    if ( selected_case("symbolic_write_scratch") )
      measure("symbolic_write_scratch", repeats, 0, [&](usize) { return emit_expression(true); });
  }
#endif
#if !defined(__micron_arch_wasm_any)
  micron::io::fflush(micron::io::stdout);
#endif
  return 1;
}
