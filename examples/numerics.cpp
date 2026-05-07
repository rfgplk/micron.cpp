// numerics.cpp
// micron::numeric_limits<T> and numeric constants (src/numerics.hpp).
// Functionally equivalent to std::numeric_limits but lives in micron::,
// with no <limits> include.
//
// See also:
//   examples/types.cpp — fundamental type aliases (i8, u32, f64, ...)
//   examples/math.cpp  — math::constant_pi<T>, sqrt, log, ...
//
// Also covers:
//   micron::maximum::*bit  — typed maximum constants
//   micron::minimum::*bit  — typed minimum constants
//   micron::bits::byte     — bits per byte (8)
//   micron::pi, half_pi, pi64, half_pi64 — math constants

#include "../src/numerics.hpp"
#include "../src/io/console.hpp"

int
main()
{
  // ================================================================
  // numeric_limits<T> — type bounds and properties
  // ================================================================

  micron::io::println("--- numeric_limits<int> ---");
  micron::io::println("min=",     micron::numeric_limits<int>::min());
  micron::io::println("max=",     micron::numeric_limits<int>::max());
  micron::io::println("lowest=",  micron::numeric_limits<int>::lowest());   // same as min for integers
  micron::io::println("digits=",  micron::numeric_limits<int>::digits);      // 31
  micron::io::println("digits10=",micron::numeric_limits<int>::digits10);
  micron::io::println("is_signed=",micron::numeric_limits<int>::is_signed);
  micron::io::println("is_integer=",micron::numeric_limits<int>::is_integer);
  micron::io::println("is_modulo=",micron::numeric_limits<int>::is_modulo);

  micron::io::println("--- numeric_limits<u64> ---");
  micron::io::println("max=",     micron::numeric_limits<u64>::max());   // 2^64 - 1
  micron::io::println("is_signed=",micron::numeric_limits<u64>::is_signed);

  micron::io::println("--- numeric_limits<f32> ---");
  micron::io::println("min=",       micron::numeric_limits<f32>::min());       // smallest positive normal
  micron::io::println("max=",       micron::numeric_limits<f32>::max());
  micron::io::println("lowest=",    micron::numeric_limits<f32>::lowest());    // most negative
  micron::io::println("epsilon=",   micron::numeric_limits<f32>::epsilon());   // ULP at 1.0
  micron::io::println("infinity=",  micron::numeric_limits<f32>::infinity());
  micron::io::println("quiet_NaN=", micron::numeric_limits<f32>::quiet_NaN());
  micron::io::println("is_iec559=", micron::numeric_limits<f32>::is_iec559);
  micron::io::println("has_inf=",   micron::numeric_limits<f32>::has_infinity);
  micron::io::println("digits=",    micron::numeric_limits<f32>::digits);     // mantissa bits (24)
  micron::io::println("min_exp=",   micron::numeric_limits<f32>::min_exponent);
  micron::io::println("max_exp=",   micron::numeric_limits<f32>::max_exponent);

  micron::io::println("--- numeric_limits<f64> ---");
  micron::io::println("epsilon=",   micron::numeric_limits<f64>::epsilon());
  micron::io::println("max=",       micron::numeric_limits<f64>::max());
  micron::io::println("digits10=",  micron::numeric_limits<f64>::digits10);

  // --- Use in generic code ---
  micron::io::println("--- generic use ---");

  auto check_overflow = []<typename T>(T a, T b) {
    // Detect if a + b would overflow
    bool would_overflow = a > micron::numeric_limits<T>::max() - b;
    return would_overflow;
  };

  micron::io::println("int overflow(MAX-1 + 1)=",
      check_overflow(micron::numeric_limits<int>::max() - 1, int(1)));
  micron::io::println("int overflow(MAX + 1)=",
      check_overflow(micron::numeric_limits<int>::max(), int(1)));

  // ================================================================
  // maximum:: / minimum:: — typed constants
  // Prefer these over casting literals for clarity in bit manipulation
  // ================================================================

  micron::io::println("--- maximum:: constants ---");
  micron::io::println("u8bit=",  micron::maximum::u8bit);    // 0xFF
  micron::io::println("u16bit=", micron::maximum::u16bit);   // 0xFFFF
  micron::io::println("u32bit=", micron::maximum::u32bit);   // 0xFFFFFFFF
  micron::io::println("u64bit=", micron::maximum::u64bit);   // 0xFFFFFFFF...
  micron::io::println("i8bit=",  micron::maximum::i8bit);    // 127
  micron::io::println("i32bit=", micron::maximum::i32bit);   // 2^31 - 1
  micron::io::println("f32bit=", micron::maximum::f32bit);
  micron::io::println("f64bit=", micron::maximum::f64bit);

  micron::io::println("--- minimum:: constants ---");
  micron::io::println("i8bit=",  micron::minimum::i8bit);    // -128
  micron::io::println("i32bit=", micron::minimum::i32bit);
  micron::io::println("f32bit=", micron::minimum::f32bit);

  // Practical use: bitmask operations
  u32 masked = 0xDEADBEEF & micron::maximum::u16bit;   // keep low 16 bits
  micron::io::println("0xDEADBEEF & u16bit = ", masked);   // 0xBEEF

  // ================================================================
  // bits:: — bit counts per unit
  // ================================================================

  micron::io::println("bits::byte=", micron::bits::byte);     // 8
  micron::io::println("bits::_char=", micron::bits::_char);   // 8

  // ================================================================
  // Math constants
  // ================================================================

  micron::io::println("--- math constants ---");
  micron::io::println("pi=",       micron::pi);
  micron::io::println("half_pi=",  micron::half_pi);
  micron::io::println("pi64=",     micron::pi64);
  micron::io::println("half_pi64=",micron::half_pi64);

  // Use in computation
  f64 circumference = 2.0 * micron::pi64 * 5.0;
  micron::io::println("circumference of r=5: ", circumference);

  // ================================================================
  // npos / sentinel values via numeric_limits
  // ================================================================

  // max() is often used as npos (no-position sentinel)
  constexpr usize npos = micron::numeric_limits<usize>::max();
  micron::io::println("usize npos=", npos);   // all bits set

  usize search_result = npos;   // "not found"
  micron::io::println("search_result == npos: ", search_result == npos);

  return 0;
}
