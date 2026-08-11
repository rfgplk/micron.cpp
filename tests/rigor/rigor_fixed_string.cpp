// Randomized rigor suite for micron::fixed_string<N>, diffed per operation against the flat
// ref_string oracle in tests/support/string_rigor.hpp.
//
// fixed_string is a compile-time-first type, and tests/rigor/fixed_string_constexpr.cpp already
// proves the surface by static_assert. This file exists for the half that a static_assert cannot
// reach: the RUNTIME instantiations. Every member here is `constexpr`, which means each one is
// compiled twice -- once folded by the front end, once as real code under -Ofast with SIMD in
// micron::memcmp's `if !consteval` arm. A search loop can agree with the oracle at compile time
// and still be wrong when it is actually executed, and only this file would see that.
//
// N is part of fixed_string's type, so the content length is varied through the (const char*, n)
// constructor against a single fixed capacity rather than by varying N.
//
// Seeds are fixed hex literals, never time-based.
//
// Build: `duck test tests/rigor/rigor_fixed_string.cpp -o bin/t --timeout 120`

#include "../../src/io/console.hpp"

#include "../../src/string/fixed_string.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
#include "../support/oracles.hpp"
#include "../support/string_rigor.hpp"

using namespace snowball;
using mtest::alpha;
using mtest::prng;
using mtest::REF_NPOS;
using mtest::ref_string;

#ifndef RIGOR_ITERS
#define RIGOR_ITERS 10000
#endif

// one capacity for the whole suite; content length is what varies
constexpr static const usize CAP = 64;
constexpr static const usize MAXLEN = CAP - 1;

using fs = micron::fixed_string<CAP>;
using ref = ref_string<char, CAP>;

constexpr static const usize FS_NPOS = fs::npos;

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

// fixed_string::size() is the CAPACITY, so seq_eq()/cstr_eq() from the shared kit do not apply --
// they key off size(). the content length is len()
static bool
fs_eq(const fs &m, const ref &r) noexcept
{
  if ( m.len() != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( m.buf[i] != r.buf[i] ) return false;
  return m.buf[r.len] == '\0';
}

// draw a content length and its bytes. a generated NUL would truncate the fixed_string (len() is
// strnlen) while the oracle would keep counting, so the alphabets below never produce one
static usize
draw(prng &rng, char *out, usize cap, alpha a)
{
  const usize n = static_cast<usize>(rng.next() % (cap + 1));
  for ( usize i = 0; i < n; ++i ) {
    char c = mtest::gen_char<char>(rng, a);
    if ( c == '\0' ) c = 'x';
    out[i] = c;
  }
  out[n] = '\0';
  return n;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deterministic edges

static void
run_edges(void)
{
  test_case("fs: default is empty, capacity is N-1");
  {
    fs s;
    require(s.empty(), true);
    require(s.len(), 0u);
    require(s.size(), CAP - 1);
    require(s.capacity(), CAP - 1);
    require(s.c_str()[0], '\0');
  }
  end_test_case();

  test_case("fs: size() is the capacity, len() is the content");
  {
    fs s("abc", 3);
    require(s.size(), CAP - 1);
    require(s.len(), 3u);
    require(s.end() - s.begin(), 3l);
  }
  end_test_case();

  test_case("fs: (ptr,n) ctor caps at N-1 and never overruns");
  {
    char big[CAP * 2];
    for ( usize i = 0; i < CAP * 2; ++i ) big[i] = 'a';
    fs s(big, CAP * 2);
    require(s.len(), MAXLEN);
    require(s.buf[CAP - 1], '\0');
  }
  end_test_case();

  test_case("fs: empty-string edges answer npos, not a crash");
  {
    fs s;
    require(s.find('a') == FS_NPOS, true);
    require(s.rfind('a') == FS_NPOS, true);
    require(s.find_first_of("abc") == FS_NPOS, true);
    require(s.find_last_of("abc") == FS_NPOS, true);
    require(s.find_first_not_of("abc") == FS_NPOS, true);
    require(s.find_last_not_of("abc") == FS_NPOS, true);
    require(s.starts_with('a'), false);
    require(s.ends_with('a'), false);
    require(s.count('a'), 0u);
    require(s.trim().empty(), true);
    require(s.reverse().empty(), true);
  }
  end_test_case();

  test_case("fs: cross-N comparison is by content");
  {
    micron::fixed_string<8> a("abc", 3);
    micron::fixed_string<32> b("abc", 3);
    fs c("abc", 3);
    require(a == b, true);
    require(b == c, true);
    require(a == "abc", true);
    require(a < micron::fixed_string<8>("abd", 3), true);
    // a prefix sorts before its extension regardless of the two capacities
    require(micron::fixed_string<8>("ab", 2) < micron::fixed_string<32>("abc", 3), true);
  }
  end_test_case();

  test_case("fs: characters order as UNSIGNED bytes");
  {
    // signed-char ordering would put 0x80 below 'a'; memcmp semantics put it above
    const char hi[2] = { static_cast<char>(0x80), '\0' };
    const char lo[2] = { 'a', '\0' };
    fs a(hi, 1), b(lo, 1);
    require(a > b, true);
    require(b < a, true);
  }
  end_test_case();

  test_case("fs: at() throws past the content length");
  {
    fs s("abc", 3);
    bool threw = false;
    try {
      (void)s.at(3);
    } catch ( ... ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// randomized property loops

static void
run_props(alpha a, u64 seed, const char *label)
{
  char sbuf[CAP + 1];
  char nbuf[CAP + 1];

  test_case(label);
  {
    prng rng(seed);
    for ( usize it = 0; it < RIGOR_ITERS; ++it ) {
      const usize n = draw(rng, sbuf, MAXLEN, a);
      fs m(sbuf, n);
      ref r;
      r.assign(sbuf, n);

      ck(fs_eq(m, r), "content", it);
      ck(m.len() == r.size(), "len", it);
      ck(m.empty() == r.empty(), "empty", it);

      // single-character search, from both ends and from an offset
      const char c = mtest::gen_char<char>(rng, a);
      const usize pos = n ? static_cast<usize>(rng.next() % n) : 0;
      ck(m.find(c) == r.find(c), "find(ch)", it);
      ck(m.find(c, pos) == r.find(c, pos), "find(ch,pos)", it);
      ck(m.rfind(c) == r.rfind(c), "rfind(ch)", it);
      ck(m.count(c) == r.count_char(c), "count(ch)", it);
      ck(m.contains(c) == (r.find(c) != REF_NPOS), "contains(ch)", it);

      // substring search against a needle drawn from the same alphabet, and -- more usefully --
      // against a slice of the haystack itself, which is the case that actually hits
      const usize nn = draw(rng, nbuf, 4, a);
      ck(m.find_substr(nbuf, nn) == r.find_seq(nbuf, nn, 0), "find_substr", it);
      ck(m.rfind_substr(nbuf, nn) == r.rfind_seq(nbuf, nn, REF_NPOS), "rfind_substr", it);
      ck(m.count(nbuf, nn) == r.count_seq(nbuf, nn), "count(seq)", it);
      ck(m.contains(nbuf) == (nn == 0 || r.find_seq(nbuf, nn, 0) != REF_NPOS), "contains(seq)", it);

      if ( n >= 2 ) {
        const usize off = static_cast<usize>(rng.next() % (n - 1));
        const usize cnt = 1 + static_cast<usize>(rng.next() % (n - off));
        ck(m.find_substr(sbuf + off, cnt) == r.find_seq(sbuf + off, cnt, 0), "find_substr(self)", it);
        ck(m.rfind_substr(sbuf + off, cnt) == r.rfind_seq(sbuf + off, cnt, REF_NPOS), "rfind_substr(self)", it);
      }

      // character-set search
      const usize sn = draw(rng, nbuf, 6, a);
      ck(m.find_first_of_n(nbuf, sn) == r.find_first_of(nbuf, sn, 0), "find_first_of", it);
      ck(m.find_last_of_n(nbuf, sn) == r.find_last_of(nbuf, sn, REF_NPOS), "find_last_of", it);
      ck(m.find_first_not_of_n(nbuf, sn) == r.find_first_not_of(nbuf, sn, 0), "find_first_not_of", it);
      ck(m.find_last_not_of_n(nbuf, sn) == r.find_last_not_of(nbuf, sn, REF_NPOS), "find_last_not_of", it);

      // affixes, again both against a random needle and against a real prefix/suffix
      ck(m.starts_with(nbuf, sn) == r.starts_with(nbuf, sn), "starts_with", it);
      ck(m.ends_with(nbuf, sn) == r.ends_with(nbuf, sn), "ends_with", it);
      if ( n ) {
        const usize k = 1 + static_cast<usize>(rng.next() % n);
        ck(m.starts_with(sbuf, k), "starts_with(self)", it);
        ck(m.ends_with(sbuf + n - k, k), "ends_with(self)", it);
      }

      // comparison, against the oracle's own three-way
      const usize mn = draw(rng, nbuf, MAXLEN, a);
      const int got = m.compare(nbuf, mn);
      const int want = r.compare(nbuf, mn);
      ck((got < 0) == (want < 0) && (got > 0) == (want > 0) && (got == 0) == (want == 0), "compare", it);

      // and the operators built on it must agree with compare()'s sign
      fs other(nbuf, mn);
      ck((m == other) == (got == 0), "operator==", it);
      ck((m != other) == (got != 0), "operator!=", it);
      ck((m < other) == (got < 0), "operator<", it);
      ck((m > other) == (got > 0), "operator>", it);
      ck((m <= other) == (got <= 0), "operator<=", it);
      ck((m >= other) == (got >= 0), "operator>=", it);
      // reversed operands go through the synthesised forms
      ck((other > m) == (got < 0), "operator>(rev)", it);

      // transforms
      {
        ref t = r;
        t.to_lower();
        ck(fs_eq(m.to_lower(), t), "to_lower", it);
      }
      {
        ref t = r;
        t.to_upper();
        ck(fs_eq(m.to_upper(), t), "to_upper", it);
      }
      {
        ref t = r;
        t.reverse();
        ck(fs_eq(m.reverse(), t), "reverse", it);
      }
      {
        ref t = r;
        t.trim();
        ck(fs_eq(m.trim(), t), "trim", it);
      }
      {
        ref t = r;
        t.trim_left();
        ck(fs_eq(m.trim_left(), t), "trim_left", it);
      }
      {
        ref t = r;
        t.trim_right();
        ck(fs_eq(m.trim_right(), t), "trim_right", it);
      }

      // a transform must not change the type's width, only its content
      ck(m.trim().size() == CAP - 1, "trim width", it);
      ck(m.reverse().reverse().len() == m.len(), "reverse involutive", it);
    }
  }
  end_test_case();
}

// the same operation folded and executed must give the same answer. this is the property that the
// `if !consteval` split inside micron::memcmp could silently break -- the SIMD arm and the scalar
// arm are different code
[[gnu::noinline]] static int
opaque_compare(const fs &a, const fs &b)
{
  return a.compare(b);
}

static void
run_consteval_agreement(void)
{
  test_case("fs: folded and executed comparisons agree");
  {
    constexpr fs a("abc", 3);
    constexpr fs b("abd", 3);
    constexpr fs c("abc", 3);

    static_assert(a.compare(b) < 0);
    static_assert(a.compare(c) == 0);
    static_assert(b.compare(a) > 0);

    require(opaque_compare(a, b) < 0, true);
    require(opaque_compare(a, c) == 0, true);
    require(opaque_compare(b, a) > 0, true);

    // long enough to reach the SIMD arm at run time while the constant-evaluated arm stays scalar
    char lbuf[CAP];
    for ( usize i = 0; i < MAXLEN; ++i ) lbuf[i] = static_cast<char>('a' + (i % 26));
    lbuf[MAXLEN] = '\0';
    fs big(lbuf, MAXLEN);
    fs big2(lbuf, MAXLEN);
    require(opaque_compare(big, big2) == 0, true);
    lbuf[MAXLEN - 1] = 'Z';
    fs big3(lbuf, MAXLEN);
    require(opaque_compare(big, big3) > 0, true);
    require(opaque_compare(big3, big) < 0, true);
  }
  end_test_case();
}

int
main()
{
  sb::print("=== FIXED_STRING RIGOR ===");

  run_edges();
  run_consteval_agreement();

  run_props(alpha::small, 0xF11EDu, "fs props: small alphabet");
  run_props(alpha::ascii, 0xC0FFEEu, "fs props: ascii");
  run_props(alpha::case_mix, 0xBADC0DEu, "fs props: mixed case");
  run_props(alpha::ws_mix, 0x5EED1u, "fs props: whitespace-heavy");
  run_props(alpha::full, 0xA11CEu, "fs props: full byte range");

  sb::print("=== ALL FIXED_STRING RIGOR TESTS PASSED ===");
  return 1;
}
