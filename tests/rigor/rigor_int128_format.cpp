// rigor_int128_format.cpp -- the 128-bit format and parse entry points.
//
// four defects, one theme: 128-bit was bolted onto layers that assume 64.
//
//   1. fmt_u128_to_buf returned `off + to_chars(...)`. to_chars answers 0 for "did not fit", so a
//      too-small buffer reported the BASE PREFIX as a complete rendering: echof("{:#b}", ~u128(0))
//      printed "0b" and dropped all 128 digits, looking for all the world like a valid number.
//   2. formatter<i128> printed sign + magnitude in EVERY base, so {:x} of (i128)-1 was "-1" while
//      {:x} of (i64)-1 is "ffffffffffffffff". The same format string changed meaning purely by
//      widening its argument. The format layer's convention for a radix spec is the two's
//      complement bit pattern; micron::to_chars keeps the other one (sign in every base) because
//      that is the std::to_chars/arbint contract it was written against, so both are pinned here.
//   3. from_chars(i128&) consumed a sign and then delegated to the u128 overload, which consumes
//      a second '+' -- so "-+5" and "++5" both parsed, where from_chars(i64&, "-+5", 3) correctly
//      does not. The header's own contract says STRICT.
//   4. int_to_string<u128> was an amd64-only spelling: it is constrained on is_integral_v, and
//      type_traits.hpp registers the __int128 traits under __micron_arch_amd64 while
//      bits/__int128.hpp gives the TYPE on every 64-bit-width arch. tests/compiletests/strings.cpp
//      used it as if it were portable and broke every --arm/--arm64/--i386 cell of the matrix.
//
// oracle: a hand-rolled schoolbook base conversion over the hi/lo halves, independent of
// micron::to_chars and of the formatter.

#include "../../src/io/echo.hpp"
#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_FUZZ = 4000;
#else
constexpr static const usize N_FUZZ = 40000;
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// oracle: repeated divide by base, done on the u128 itself. no dependency on to_chars.

static micron::hstring<schar>
u128_oracle(u128 v, u32 base, bool upper)
{
  const char *tbl = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  char tmp[140];
  usize n = 0;
  if ( v == static_cast<u128>(0) ) {
    tmp[n++] = '0';
  } else {
    const u128 b = static_cast<u128>(base);
    while ( v != static_cast<u128>(0) ) {
      const u128 q = v / b;
      const u128 r = v - q * b;
      tmp[n++] = tbl[static_cast<u64>(r)];
      v = q;
    }
  }
  micron::hstring<schar> out;
  for ( usize i = n; i > 0; --i ) out += tmp[i - 1];
  return out;
}

static bool
same(const micron::hstring<schar> &a, const micron::hstring<schar> &b)
{
  if ( a.size() != b.size() ) return false;
  for ( usize i = 0; i < a.size(); ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

static u128
rand_u128(prng &rng) noexcept
{
  return (static_cast<u128>(rng.next()) << 64) | static_cast<u128>(rng.next());
}

int
main()
{
  prng rng(0xd1ce5eed12800abcULL);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("a too-small buffer answers 0, not the base prefix");
  {
    char buf[200];
    const u128 all = ~static_cast<u128>(0);

    // the reproducer: 2 prefix bytes + 128 digits. anything short of the whole thing is a refusal.
    micron::format::__impl::fmt_spec spec{};
    spec.type = 'b';
    spec.alt = true;
    for ( usize cap = 0; cap < 130; ++cap ) require_true(micron::format::formatter<u128>::write(buf, cap, all, spec) == 0);
    require_true(micron::format::formatter<u128>::write(buf, 130, all, spec) == 130);

    spec.type = 'x';
    for ( usize cap = 0; cap < 34; ++cap ) require_true(micron::format::formatter<u128>::write(buf, cap, all, spec) == 0);
    require_true(micron::format::formatter<u128>::write(buf, 34, all, spec) == 34);

    // through the porcelain, where buf_size is large enough, the digits are all there
    require_true(micron::format::format("{:#b}", all).size() == 130);
    require_true(micron::format::format("{:#x}", all).size() == 34);

    // the signed twin: a negative magnitude that does not fit must not report the '-' alone
    micron::format::__impl::fmt_spec dspec{};
    const i128 neg = -static_cast<i128>(1);
    for ( usize cap = 0; cap < 2; ++cap ) require_true(micron::format::formatter<i128>::write(buf, cap, neg, dspec) == 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("a radix spec prints the two's-complement bit pattern, as the narrower types do");
  {
    require_true(same(micron::format::format("{:x}", static_cast<i128>(-1)), micron::hstring<schar>("ffffffffffffffffffffffffffffffff")));
    require_true(same(micron::format::format("{:x}", static_cast<i64>(-1)), micron::hstring<schar>("ffffffffffffffff")));
    require_true(same(micron::format::format("{:X}", static_cast<i128>(-1)), micron::hstring<schar>("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")));
    require_true(micron::format::format("{:b}", static_cast<i128>(-1)).size() == 128);
    require_true(same(micron::format::format("{:x}", static_cast<i128>(-256)), micron::hstring<schar>("ffffffffffffffffffffffffffffff00")));
    // base 10 still carries the sign
    require_true(same(micron::format::format("{}", static_cast<i128>(-1)), micron::hstring<schar>("-1")));
    require_true(same(micron::format::format("{:d}", static_cast<i128>(-12345)), micron::hstring<schar>("-12345")));

    // to_chars deliberately keeps the OTHER convention -- sign in every base
    char b[80];
    const usize n = micron::to_chars(b, sizeof(b), static_cast<i128>(-1), 16u, false);
    require_true(n == 2 && b[0] == '-' && b[1] == '1');
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: u128 formatting against a schoolbook oracle, every base");
  {
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      const u128 v = rand_u128(rng);
      require_true(same(micron::format::format("{}", v), u128_oracle(v, 10, false)));
      require_true(same(micron::format::format("{:x}", v), u128_oracle(v, 16, false)));
      require_true(same(micron::format::format("{:X}", v), u128_oracle(v, 16, true)));
      require_true(same(micron::format::format("{:o}", v), u128_oracle(v, 8, false)));
      require_true(same(micron::format::format("{:b}", v), u128_oracle(v, 2, false)));

      // i128: the bit pattern in a radix spec, sign+magnitude in base 10
#if defined(__micron_arch_width_64)
      const i128 s = static_cast<i128>(v);
      const bool neg = s < 0;
      const u128 mag = neg ? (static_cast<u128>(0) - static_cast<u128>(s)) : static_cast<u128>(s);
#else
      const i128 s = i128(v);
      const bool neg = s.__is_negative();
      const u128 mag = s.__abs();
#endif
      require_true(same(micron::format::format("{:x}", s), u128_oracle(v, 16, false)));
      micron::hstring<schar> want10 = u128_oracle(mag, 10, false);
      if ( neg ) {
        micron::hstring<schar> t("-");
        t.append(want10.c_str(), want10.size());
        want10 = t;
      }
      require_true(same(micron::format::format("{}", s), want10));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("from_chars is strict about signs at 128 bits, as it is at 64");
  {
    i128 v{};
    u128 u{};
    require_true(!micron::from_chars(v, "-+5", 3));
    require_true(!micron::from_chars(v, "++5", 3));
    require_true(!micron::from_chars(v, "+-5", 3));
    require_true(!micron::from_chars(v, "--5", 3));
    require_true(micron::from_chars(v, "-5", 2));
    require_true(micron::from_chars(v, "+5", 2));
    require_true(micron::from_chars(v, "5", 1));
    require_true(!micron::from_chars(u, "-5", 2));      // unsigned takes no '-'
    require_true(micron::from_chars(u, "+5", 2));
    // the narrower type this was supposed to match
    i64 n64{};
    require_true(!micron::from_chars(n64, "-+5", 3));
    require_true(!micron::from_chars(n64, "++5", 3));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: 128-bit round-trip through to_chars / from_chars");
  {
    char b[160];
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      const u128 v = rand_u128(rng);
      for ( u32 base : { 2u, 8u, 10u, 16u } ) {
        const usize n = micron::to_chars(b, sizeof(b), v, base, false);
        require_true(n != 0);
        u128 back{};
        require_true(micron::from_chars(back, b, n, base));
        require_true(back == v);
        // a sign spliced in anywhere past the first character is still a rejection
        if ( n + 1 < sizeof(b) ) {
          char tmp[162];
          tmp[0] = '+';
          tmp[1] = '+';
          for ( usize k = 0; k < n; ++k ) tmp[k + 2] = b[k];
          u128 bad{};
          require_true(!micron::from_chars(bad, tmp, n + 2, base));
        }
      }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("int_to_string is type-keyed at 128 bits, not is_integral_v-keyed");
  {
    // this spelling has to build and answer on EVERY arch -- it is the one the compile matrix
    // uses, and off amd64 is_integral_v<u128> is false so the constrained template never matched
    const u128 big = (static_cast<u128>(1) << 100) + static_cast<u128>(7);
    require_true(same(micron::hstring<schar>(micron::int_to_string<u128>(big).c_str()), u128_oracle(big, 10, false)));
    require_true(same(micron::hstring<schar>(micron::to_string<u128>(big).c_str()), u128_oracle(big, 10, false)));
    require_true(same(micron::hstring<schar>(micron::int_to_string<i128>(static_cast<i128>(-5)).c_str()), micron::hstring<schar>("-5")));
    require_true(same(micron::hstring<schar>(micron::int_to_string<i128>(static_cast<i128>(5)).c_str()), micron::hstring<schar>("5")));

    for ( usize i = 0; i < N_FUZZ / 8; ++i ) {
      const u128 v = rand_u128(rng);
      require_true(same(micron::hstring<schar>(micron::int_to_string<u128>(v).c_str()), u128_oracle(v, 10, false)));
    }
  }
  end_test_case();

  print("=== INT128 FORMAT RIGOR SUITE PASSED ===");
  return 1;
}
