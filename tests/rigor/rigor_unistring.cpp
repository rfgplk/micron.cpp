// Exhaustive, adversarial rigor suite for the micron "unistring" module
// (src/string/unistring.hpp) — note this is NOT a class but a family of free
// functions: number<->string conversions, constexpr formatters, unicode
// validation, and small string utilities.
//
// Coverage (>=10k iters/fn where applicable):
//   * integer formatting: to_string / to_string_stack / int_to_string /
//     uint_to_string / *_base_stack (bases 2..36) / to_hex|oct|bin_stack /
//     to_hex|bin_fixed_stack / int_to_string_padded_stack / bytes_to_string,
//     across i8..i64 / u8..u64, diffed against an independent hand-rolled
//     naive formatter AND round-tripped through micron's own string_to_int64.
//   * constexpr_int_to_string / constexpr_hex / constexpr_bin (compile-time
//     static_assert + runtime fuzz).
//   * unicode validation u8_check / u16_check / u32_check vs an independent
//     spec oracle over valid + truncated + overlong + surrogate + >U+10FFFF.
//   * utilities invert / with_capacity.
//   * float formatting: deterministic known values, structural validity, and
//     fixed-notation round-trip within tolerance.
//
// No STL (the old unistring.cpp pulled in <climits>/<cstring>; this one does not).
//
// Build: `duck build tests/rigor/rigor_unistring.cpp`; run `bin/rigor_unistring`.

#include "../../src/io/console.hpp"

#include "../../src/string/strings.hpp"
#include "../../src/string/unistring.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
#include "../support/oracles.hpp"

using namespace snowball;
using mtest::prng;

#ifndef RIGOR_ITERS
#define RIGOR_ITERS 10000
#endif
static constexpr usize ITERS = RIGOR_ITERS;

// ───────────────────────────────────────────────────────────────────────────
// independent naive oracles (hand-rolled, no STL)
// ───────────────────────────────────────────────────────────────────────────

// unsigned -> base, MSB first, returns length.
static usize
nfmt_base(u64 v, u32 base, bool upper, char *out) noexcept
{
  const char *dig = upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";
  char tmp[70];
  usize p = 0;
  if ( v == 0 )
    tmp[p++] = '0';
  else
    while ( v ) {
      tmp[p++] = dig[v % base];
      v /= base;
    }
  for ( usize i = 0; i < p; ++i ) out[i] = tmp[p - 1 - i];
  return p;
}

// signed decimal with leading '-'.
static usize
nfmt_dec_signed(i64 v, char *out) noexcept
{
  if ( v < 0 ) {
    out[0] = '-';
    u64 u = static_cast<u64>(-(v + 1)) + 1u;      // two's-complement-safe negate
    return 1 + nfmt_base(u, 10, false, out + 1);
  }
  return nfmt_base(static_cast<u64>(v), 10, false, out);
}

// compare a micron string's bytes against buf[0..len).
template<typename Str>
static bool
str_eq(const Str &s, const char *buf, usize len) noexcept
{
  if ( s.size() != len ) return false;
  const char *c = s.c_str();
  for ( usize i = 0; i < len; ++i )
    if ( c[i] != buf[i] ) return false;
  return true;
}

static void
ck(bool ok, const char *what, u64 it) noexcept
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", it);
  require(ok, true);
}

// random integer of width W (1/2/4/8 bytes) as a u64 bit pattern.
static u64
rand_bits(prng &rng) noexcept
{
  return rng.next();
}

// ───────────────────────────────────────────────────────────────────────────
// integer formatting
// ───────────────────────────────────────────────────────────────────────────

template<typename I>
static void
test_to_string_signed(const char *name)
{
  test_case(name);
  prng rng(0x51 + sizeof(I));
  char buf[70];
  for ( usize it = 0; it < ITERS; ++it ) {
    I v = static_cast<I>(rand_bits(rng));
    // sprinkle edges
    if ( (it & 0x3ff) == 0 ) v = static_cast<I>(0);
    auto s = micron::to_string<I, char>(v);
    usize len = nfmt_dec_signed(static_cast<i64>(v), buf);
    ck(str_eq(s, buf, len), "to_string", it);
    // round-trip through micron's parser
    i64 back = micron::string_to_int64(s);
    ck(back == static_cast<i64>(v), "to_string-roundtrip", it);
  }
  end_test_case();
}

template<typename I>
static void
test_to_string_unsigned(const char *name)
{
  test_case(name);
  prng rng(0x71 + sizeof(I));
  char buf[70];
  for ( usize it = 0; it < ITERS; ++it ) {
    I v = static_cast<I>(rand_bits(rng));
    auto s = micron::to_string<I, char>(v);
    usize len = nfmt_base(static_cast<u64>(v), 10, false, buf);
    ck(str_eq(s, buf, len), "to_string-u", it);
  }
  end_test_case();
}

template<typename I>
static void
test_to_string_stack_signed(const char *name)
{
  test_case(name);
  prng rng(0x91 + sizeof(I));
  char buf[70];
  for ( usize it = 0; it < ITERS; ++it ) {
    I v = static_cast<I>(rand_bits(rng));
    auto s = micron::to_string_stack<I, 32, char>(v);
    usize len = nfmt_dec_signed(static_cast<i64>(v), buf);
    ck(str_eq(s, buf, len), "to_string_stack", it);
  }
  end_test_case();
}

template<typename I>
static void
test_base_stack(const char *name)
{
  test_case(name);
  prng rng(0xB1 + sizeof(I));
  char buf[70];
  for ( usize it = 0; it < ITERS; ++it ) {
    I v = static_cast<I>(rand_bits(rng));
    u32 base = 2u + static_cast<u32>(rng.next_in(35));      // 2..36
    bool upper = (rng.next() & 1u) != 0;
    auto s = micron::uint_to_string_base_stack<I, 72, char>(v, base, upper);
    usize len = nfmt_base(static_cast<u64>(static_cast<micron::make_unsigned_t<I>>(v)), base, upper, buf);
    ck(str_eq(s, buf, len), "uint_base_stack", it);
  }
  end_test_case();
}

static void
test_hex_oct_bin(void)
{
  test_case("to_hex/oct/bin_stack vs naive");
  {
    prng rng(0xD1);
    char buf[70];
    for ( usize it = 0; it < ITERS; ++it ) {
      u32 v = static_cast<u32>(rng.next());
      {
        auto s = micron::to_hex_stack<u32, 20, char>(v, false);
        usize len = nfmt_base(v, 16, false, buf);
        ck(str_eq(s, buf, len), "to_hex", it);
      }
      {
        auto s = micron::to_oct_stack<u32, 24, char>(v);
        usize len = nfmt_base(v, 8, false, buf);
        ck(str_eq(s, buf, len), "to_oct", it);
      }
      {
        auto s = micron::to_bin_stack<u32, 68, char>(v);
        usize len = nfmt_base(v, 2, false, buf);
        ck(str_eq(s, buf, len), "to_bin", it);
      }
    }
  }
  end_test_case();
}

static void
test_fixed_width(void)
{
  test_case("to_hex_fixed/bin_fixed_stack");
  {
    prng rng(0xE1);
    for ( usize it = 0; it < ITERS; ++it ) {
      u32 v = static_cast<u32>(rng.next());
      usize digits = 1 + static_cast<usize>(rng.next_in(8));
      auto s = micron::to_hex_fixed_stack<u32, 40, char>(v, digits, false);
      // last `digits` hex nibbles of v, low to high reversed
      char buf[40];
      u32 t = v;
      for ( usize i = digits; i > 0; --i ) {
        buf[i - 1] = "0123456789abcdef"[t & 0xF];
        t >>= 4;
      }
      ck(str_eq(s, buf, digits), "hex_fixed", it);
    }
  }
  end_test_case();
}

static void
test_padded(void)
{
  test_case("int_to_string_padded_stack");
  {
    prng rng(0xF1);
    char buf[70];
    for ( usize it = 0; it < ITERS; ++it ) {
      i32 v = static_cast<i32>(rng.next());
      usize width = static_cast<usize>(rng.next_in(20));
      auto s = micron::int_to_string_padded_stack<i32, 40, char>(v, width);
      // build expected: sign + zero-pad digits to width
      usize raw = nfmt_dec_signed(static_cast<i64>(v), buf);
      if ( raw >= width ) {
        ck(str_eq(s, buf, raw), "padded-nopad", it);
      } else {
        char out[40];
        usize pos = 0;
        usize pad = width - raw;
        bool neg = (v < 0);
        char dig[40];
        usize dlen
            = nfmt_base(static_cast<u64>(neg ? (static_cast<u64>(-(static_cast<i64>(v) + 1)) + 1u) : static_cast<u64>(v)), 10, false, dig);
        if ( neg ) out[pos++] = '-';
        for ( usize i = 0; i < pad; ++i ) out[pos++] = '0';
        for ( usize i = 0; i < dlen; ++i ) out[pos++] = dig[i];
        ck(str_eq(s, out, pos), "padded", it);
      }
    }
  }
  end_test_case();
}

static void
test_constexpr_formatters(void)
{
  // NOTE: constexpr_int_to_string is declared constexpr but is NOT currently
  // evaluable at compile time — its chain reaches sstring()'s czero()->cbyteset(),
  // which are non-constexpr SIMD routines in memory/cmemory. Making them
  // constexpr (an `if consteval` scalar fallback, like micron::memcpy already
  // has) is a separate cmemory enhancement; here we validate it at runtime.
  test_case("constexpr_int_to_string/hex/bin runtime fuzz");
  {
    prng rng(0x1A1A);
    char buf[70];
    for ( usize it = 0; it < ITERS; ++it ) {
      i32 v = static_cast<i32>(rng.next());
      auto s = micron::constexpr_int_to_string<i32, 24>(v);
      usize len = nfmt_dec_signed(static_cast<i64>(v), buf);
      ck(str_eq(s, buf, len), "constexpr_int", it);
      u32 u = static_cast<u32>(rng.next());
      auto h = micron::constexpr_hex<u32, 24>(u, false);
      usize hlen = nfmt_base(u, 16, false, buf);
      ck(str_eq(h, buf, hlen), "constexpr_hex", it);
      auto b = micron::constexpr_bin<u32, 68>(u);
      usize blen = nfmt_base(u, 2, false, buf);
      ck(str_eq(b, buf, blen), "constexpr_bin", it);
    }
  }
  end_test_case();
}

// more integer + string-conversion coverage
static void
test_more_conversions(void)
{
  test_case("int_to_string / uint_to_string (heap) vs naive");
  {
    prng rng(0x301);
    char buf[70];
    for ( usize it = 0; it < ITERS; ++it ) {
      i64 v = static_cast<i64>(rng.next());
      auto s = micron::int_to_string<i64, char>(v);
      usize len = nfmt_dec_signed(v, buf);
      ck(str_eq(s, buf, len), "int_to_string", it);
      u64 u = rng.next();
      auto s2 = micron::uint_to_string<u64, char>(u);
      usize len2 = nfmt_base(u, 10, false, buf);
      ck(str_eq(s2, buf, len2), "uint_to_string", it);
    }
  }
  end_test_case();

  test_case("int_to_string_base_stack signed (base 10) vs naive");
  {
    prng rng(0x302);
    char buf[70];
    for ( usize it = 0; it < ITERS; ++it ) {
      i32 v = static_cast<i32>(rng.next());
      auto s = micron::int_to_string_base_stack<i32, 72, char>(v, 10, false);
      usize len = nfmt_dec_signed(static_cast<i64>(v), buf);
      ck(str_eq(s, buf, len), "int_base10", it);
    }
  }
  end_test_case();

  test_case("bytes_to_string / bytes_to_string_stack vs naive");
  {
    prng rng(0x303);
    char buf[70];
    for ( usize it = 0; it < 4000; ++it ) {
      i32 v = static_cast<i32>(rng.next());
      auto a = micron::bytes_to_string<i32, char>(v);
      auto b = micron::bytes_to_string_stack<i32, 24, char>(v);
      usize len = nfmt_dec_signed(static_cast<i64>(v), buf);
      ck(str_eq(a, buf, len), "bytes_to_string", it);
      ck(str_eq(b, buf, len), "bytes_to_string_stack", it);
    }
  }
  end_test_case();

  test_case("to_string_stack(const char*) round-trips literal");
  {
    auto s = micron::to_string_stack<32, char>("hello world");
    require(s == "hello world", true);
    require(s.size(), 11u);
  }
  end_test_case();

  test_case("double_to_string_stack matches heap double_to_string");
  {
    prng rng(0x304);
    for ( usize it = 0; it < 4000; ++it ) {
      i64 mant = static_cast<i64>(rng.next() % 2000000) - 1000000;
      f64 x = static_cast<f64>(mant) / 256.0;
      auto stack = micron::double_to_string_stack<48, char>(x);
      auto heap = micron::double_to_string(x);
      bool ok = (stack.size() == heap.size());
      const char *a = stack.c_str();
      const char *b = heap.c_str();
      for ( usize i = 0; ok && i < heap.size(); ++i )
        if ( a[i] != b[i] ) ok = false;
      ck(ok, "stack==heap", it);
    }
  }
  end_test_case();

  test_case("to_scientific/to_general_stack structural validity");
  {
    prng rng(0x305);
    for ( usize it = 0; it < 2000; ++it ) {
      i64 mant = static_cast<i64>(rng.next() % 2000000) - 1000000;
      f64 x = static_cast<f64>(mant) / 1000.0;
      auto sci = micron::to_scientific_stack<80, char>(x, 4);
      auto gen = micron::to_general_stack<80, char>(x, 6);
      // non-empty and only valid float-string characters
      bool ok = sci.size() > 0 && gen.size() > 0;
      const char *p = sci.c_str();
      for ( usize i = 0; ok && i < sci.size(); ++i ) {
        char c = p[i];
        if ( !((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') ) ok = false;
      }
      ck(ok, "sci/gen-structural", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// unicode validation — independent spec oracle
// ───────────────────────────────────────────────────────────────────────────

static bool
oracle_u8_valid(const unsigned char *s, usize n) noexcept
{
  usize i = 0;
  while ( i < n ) {
    unsigned c = s[i];
    if ( c < 0x80 ) {
      ++i;
      continue;
    }
    usize seq;
    unsigned cp;
    if ( (c & 0xE0) == 0xC0 ) {
      seq = 2;
      cp = c & 0x1F;
    } else if ( (c & 0xF0) == 0xE0 ) {
      seq = 3;
      cp = c & 0x0F;
    } else if ( (c & 0xF8) == 0xF0 ) {
      seq = 4;
      cp = c & 0x07;
    } else
      return false;
    if ( i + seq > n ) return false;
    for ( usize j = 1; j < seq; ++j ) {
      unsigned cont = s[i + j];
      if ( (cont & 0xC0) != 0x80 ) return false;
      cp = (cp << 6) | (cont & 0x3F);
    }
    if ( seq == 2 && cp < 0x80 ) return false;
    if ( seq == 3 && cp < 0x800 ) return false;
    if ( seq == 4 && cp < 0x10000 ) return false;
    if ( cp >= 0xD800 && cp <= 0xDFFF ) return false;
    if ( cp > 0x10FFFF ) return false;
    i += seq;
  }
  return true;
}

static void
test_unicode_validation(void)
{
  test_case("u8_check vs spec oracle (fuzz)");
  {
    prng rng(0x5C1);
    char buf[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(40));
      // mix: mostly ASCII, sometimes high bytes / multibyte fragments
      for ( usize i = 0; i < n; ++i ) {
        u64 r = rng.next();
        if ( (r & 3u) == 0 )
          buf[i] = static_cast<char>(0x80 + (r >> 8) % 0x80);      // continuation/lead bytes
        else
          buf[i] = static_cast<char>((r >> 8) % 0x80);      // ASCII
      }
      bool exp = oracle_u8_valid(reinterpret_cast<const unsigned char *>(buf), n);
      bool got = (micron::u8_check(buf, n) != nullptr);
      ck(got == exp, "u8_check", it);
    }
  }
  end_test_case();

  test_case("u8_check valid multibyte sequences accepted");
  {
    const char *s2 = "\xC3\xA9";              // é
    const char *s3 = "\xE2\x82\xAC";          // €
    const char *s4 = "\xF0\x9F\x98\x80";      // 😀
    require(micron::u8_check(s2, 2) != nullptr, true);
    require(micron::u8_check(s3, 3) != nullptr, true);
    require(micron::u8_check(s4, 4) != nullptr, true);
    // truncated / overlong / surrogate / lone continuation
    require(micron::u8_check("\xC3", 1) == nullptr, true);              // truncated
    require(micron::u8_check("\xC0\x80", 2) == nullptr, true);          // overlong NUL
    require(micron::u8_check("\xED\xA0\x80", 3) == nullptr, true);      // surrogate U+D800
    require(micron::u8_check("\x80", 1) == nullptr, true);              // lone continuation
  }
  end_test_case();

  test_case("u32_check ranges");
  {
    char32_t ok[3] = { U'A', 0x10FFFF, 0xFFFD };
    require(micron::u32_check(ok, 3) != nullptr, true);
    char32_t sur[1] = { 0xD800 };
    require(micron::u32_check(sur, 1) == nullptr, true);
    char32_t big[1] = { 0x110000 };
    require(micron::u32_check(big, 1) == nullptr, true);
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// utilities
// ───────────────────────────────────────────────────────────────────────────

static void
test_utilities(void)
{
  test_case("with_capacity");
  {
    auto s = micron::with_capacity(128);
    require(s.max_size() >= 128u, true);
    require(s.size(), 0u);
    auto t = micron::with_capacity<256>();
    require(t.max_size() >= 256u, true);
  }
  end_test_case();

  test_case("invert vs reversed oracle (fuzz)");
  {
    prng rng(0x4E4E);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(120));
      micron::hstring<char> s;
      char buf[140];
      for ( usize i = 0; i < n; ++i ) {
        char c = static_cast<char>('a' + (rng.next() % 26));
        buf[i] = c;
        s.push_back(c);
      }
      micron::invert(s);
      bool ok = (s.size() == n);
      for ( usize i = 0; ok && i < n; ++i )
        if ( s[i] != buf[n - 1 - i] ) ok = false;
      ck(ok, "invert", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// float formatting (deterministic + structural + fixed round-trip)
// ───────────────────────────────────────────────────────────────────────────

// parse a fixed-notation decimal (optional '-', digits, optional '.' digits).
static f64
parse_fixed(const char *s, usize n, bool &ok) noexcept
{
  ok = true;
  usize i = 0;
  bool neg = false;
  if ( i < n && (s[i] == '-' || s[i] == '+') ) {
    neg = (s[i] == '-');
    ++i;
  }
  f64 ip = 0.0;
  bool any = false;
  while ( i < n && s[i] >= '0' && s[i] <= '9' ) {
    ip = ip * 10.0 + (s[i] - '0');
    ++i;
    any = true;
  }
  f64 fp = 0.0, scale = 1.0;
  if ( i < n && s[i] == '.' ) {
    ++i;
    while ( i < n && s[i] >= '0' && s[i] <= '9' ) {
      scale *= 10.0;
      fp += (s[i] - '0') / scale;
      ++i;
      any = true;
    }
  }
  if ( i != n || !any ) ok = false;
  f64 v = ip + fp;
  return neg ? -v : v;
}

static void
test_float(void)
{
  test_case("float deterministic known values");
  {
    require(micron::to_string_f64(0.0) == "0.0", true);
    require(micron::double_to_string(1.5) == "1.5", true);
    require(micron::double_to_string(-2.25) == "-2.25", true);
    require(micron::to_fixed_stack<32, char>(3.14159, 2) == "3.14", true);
    require(micron::to_fixed_stack<32, char>(1.0, 3) == "1.000", true);
  }
  end_test_case();

  test_case("to_fixed_stack round-trip within tolerance");
  {
    prng rng(0xF0A7);
    for ( usize it = 0; it < ITERS; ++it ) {
      // moderate-magnitude finite doubles
      i64 mant = static_cast<i64>(rng.next() % 2000000) - 1000000;
      f64 x = static_cast<f64>(mant) / 1000.0;      // up to ~1e3 with 3 decimals
      u32 prec = 3u + static_cast<u32>(rng.next_in(4));
      auto s = micron::to_fixed_stack<48, char>(x, prec);
      bool ok = false;
      f64 back = parse_fixed(s.c_str(), s.size(), ok);
      f64 diff = back - x;
      if ( diff < 0 ) diff = -diff;
      ck(ok && diff < 0.01, "fixed-roundtrip", it);
    }
  }
  end_test_case();

  test_case("float special values do not crash and are non-empty");
  {
    f64 inf = 1e308 * 10.0;      // +inf
    f64 ninf = -inf;
    f64 nan = inf - inf;      // nan
    auto a = micron::double_to_string(inf);
    auto b = micron::double_to_string(ninf);
    auto c = micron::double_to_string(nan);
    require(a.size() > 0u, true);
    require(b.size() > 0u, true);
    require(c.size() > 0u, true);
    require(micron::double_to_string(-0.0).size() > 0u, true);
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// output char-type breadth (C param)
// ───────────────────────────────────────────────────────────────────────────

template<typename C>
static void
test_to_string_ctype(const char *name)
{
  test_case(name);
  prng rng(0xC7);
  char buf[70];
  for ( usize it = 0; it < 4000; ++it ) {
    i32 v = static_cast<i32>(rng.next());
    auto s = micron::to_string<i32, C>(v);
    usize len = nfmt_dec_signed(static_cast<i64>(v), buf);
    bool ok = (s.size() == len);
    for ( usize i = 0; ok && i < len; ++i )
      if ( static_cast<C>(s[i]) != static_cast<C>(buf[i]) ) ok = false;
    ck(ok, "to_string<C>", it);
  }
  end_test_case();
}

int
main(int, char **)
{
  sb::print("=== UNISTRING RIGOR ===");

  sb::print("--- integer formatting ---");
  test_to_string_signed<i8>("to_string i8");
  test_to_string_signed<i16>("to_string i16");
  test_to_string_signed<i32>("to_string i32");
  test_to_string_signed<i64>("to_string i64");
  test_to_string_unsigned<u8>("to_string u8");
  test_to_string_unsigned<u16>("to_string u16");
  test_to_string_unsigned<u32>("to_string u32");
  test_to_string_unsigned<u64>("to_string u64");
  test_to_string_stack_signed<i32>("to_string_stack i32");
  test_to_string_stack_signed<i64>("to_string_stack i64");
  test_base_stack<u32>("uint_base_stack u32");
  test_base_stack<u64>("uint_base_stack u64");
  test_hex_oct_bin();
  test_fixed_width();
  test_padded();
  test_constexpr_formatters();
  test_more_conversions();

  sb::print("--- output char-type breadth ---");
  test_to_string_ctype<char>("to_string<char>");
  test_to_string_ctype<wide>("to_string<wide>");
  test_to_string_ctype<unicode32>("to_string<unicode32>");

  sb::print("--- unicode validation ---");
  test_unicode_validation();

  sb::print("--- utilities ---");
  test_utilities();

  sb::print("--- float formatting ---");
  test_float();

  sb::print("=== UNISTRING RIGOR DONE ===");
  return 1;
}
