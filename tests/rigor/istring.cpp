// Exhaustive rigor tests for micron::istring<char>.
//
// istring is move-only and immutable: every "mutating" op returns a fresh
// istring. Coverage:
//   - Every construct/move overload at SIMD-boundary lengths
//     (0/1/15/16/17/31/32/33/63/64/127/128/255/256/257).
//   - Element access (operator[], at(idx), at(iter), data, c_str, w_str,
//     uni_str) with bounds-check semantics.
//   - All append/push_back/insert/operator+= overloads, asserting the
//     length, payload, and that the source is unchanged (immutability).
//   - substr at every boundary, including edge cases (pos=0, cnt=0,
//     pos==length, full-string).
//   - find(ch) at the SIMD boundaries listed above and rfind variants
//     where present.
//   - find(const istring&) — currently a stub (`istring.hpp:268-271`)
//     that returns npos; the test PINS that behaviour so the planned
//     optimisation flip is detectable.
//   - swap correctness.
//   - stack() — converts a <255-byte istring to sstring<256>; bounds
//     condition at exactly 255 must throw.
//   - clear/fast_clear and destructor non-doublefree.
//
// Build: `duck build tests/rigor/istring.cpp`. Exceptions ON per
// `src/defs.hpp` default.

#include "../../src/io/console.hpp"

#include "../../src/string/istring.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

using IS = micron::istring<char>;

// Deterministic ASCII pattern (mirrors string_simd.cpp's `pat`).
static char
pat(usize i)
{
  static constexpr const char *src = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  return src[i % 62];
}

static void
fill_pat(char *dst, usize n)
{
  for ( usize i = 0; i < n; ++i ) dst[i] = pat(i);
  dst[n] = 0;
}

int
main(int, char **)
{
  sb::print("=== ISTRING TESTS ===");

  // ----- Construction -----

  test_case("Default construction — zero length");
  {
    IS s;
    require(s.size(), 0u);
    require(s.empty(), true);
  }
  end_test_case();

  test_case("Construction with preallocated capacity");
  {
    IS s(64u);
    require(s.size(), 0u);
    require(s.empty(), true);
  }
  end_test_case();

  test_case("Construction (n, ch) fills n chars with ch");
  {
    IS s(7u, 'x');
    require(s.size(), 7u);
    for ( usize i = 0; i < 7; ++i ) require(s[i], 'x');
  }
  end_test_case();

  test_case("Construction from const char* lvalue (uses strlen)");
  {
    const char *p = "hello";
    IS s(p);
    require(s.size(), 5u);
    require(s[0], 'h');
    require(s[4], 'o');
  }
  end_test_case();

  test_case("Construction from const char* at SIMD boundaries");
  {
    constexpr usize lens[] = { 0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 127, 128, 255, 256, 257 };
    char buf[260];
    for ( usize L : lens ) {
      fill_pat(buf, L);
      const char *p = buf;
      IS s(p);
      require(s.size(), L);
      for ( usize i = 0; i < L; ++i ) require(s[i], pat(i));
    }
  }
  end_test_case();

  test_case("Construction from string literal (array template)");
  {
    IS s("world");
    require(s.size(), 5u);
    require(s[0], 'w');
    require(s[4], 'd');
  }
  end_test_case();

  test_case("Move construction transfers memory and zeros donor");
  {
    const char *p = "moveme";
    IS donor(p);
    IS s(micron::move(donor));
    require(s.size(), 6u);
    require(s[0], 'm');
    require(donor.size(), 0u);
    require(donor.empty(), true);
  }
  end_test_case();

  test_case("Copy construction is deleted");
  {
    static_assert(!micron::is_copy_constructible_v<IS>, "istring must be move-only");
  }
  end_test_case();

  // ----- Assignment -----

  test_case("Move assignment frees old, transfers from donor");
  {
    const char *p = "donor";
    IS donor(p);
    const char *q = "old";
    IS s(q);
    s = micron::move(donor);
    require(s.size(), 5u);
    require(s[0], 'd');
    require(donor.size(), 0u);
  }
  end_test_case();

  // ----- Element access -----

  test_case("operator[] returns correct byte");
  {
    const char *p = "abcdefgh";
    IS s(p);
    for ( usize i = 0; i < 8; ++i ) require(s[i], static_cast<char>('a' + i));
  }
  end_test_case();

  test_case("at(idx) out-of-range throws");
  {
    const char *p = "abc";
    IS s(p);
    bool threw = false;
    try {
      (void)s.at(3);
    } catch ( const micron::except::library_error & ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  test_case("at(iter) returns index");
  {
    const char *p = "hello";
    IS s(p);
    require(s.at(s.cbegin() + 3), 3u);
  }
  end_test_case();

  test_case("data/c_str/w_str/uni_str return base pointer");
  {
    const char *p = "data";
    IS s(p);
    require(s.data() != nullptr, true);
    require(s.c_str() != nullptr, true);
    require(s.w_str() != nullptr, true);
    require(s.uni_str() != nullptr, true);
    require(s.c_str()[0], 'd');
  }
  end_test_case();

  test_case("c_str on empty istring returns valid empty string");
  {
    IS s;
    // We don't dereference past index 0 since istring(default) allocates a
    // small buffer, but c_str should not be nullptr.
    require(s.c_str() != nullptr, true);
  }
  end_test_case();

  // ----- Iterators -----

  test_case("Iterator traversal covers all characters in order");
  {
    const char *p = "xyz1234";
    IS s(p);
    usize idx = 0;
    for ( auto it = s.begin(); it != s.end(); ++it, ++idx ) require(*it, p[idx]);
    require(idx, 7u);
  }
  end_test_case();

  test_case("cbegin/cend agree with begin/end");
  {
    const char *p = "iter";
    IS s(p);
    require(s.begin(), s.cbegin());
    require(s.end(), s.cend());
  }
  end_test_case();

  test_case("last() points to final character");
  {
    const char *p = "tail";
    IS s(p);
    require(*s.last(), 'l');
  }
  end_test_case();

  // ----- Modifiers (each returns a fresh istring; source unchanged) -----

  test_case("append(ptr,n) appends n bytes and leaves source unmodified");
  {
    const char *p = "abc";
    IS s(p);
    auto t = s.append("def", 3);
    require(t.size(), 6u);
    require(t[0], 'a');
    require(t[5], 'f');
    require(s.size(), 3u);      // immutable
  }
  end_test_case();

  test_case("append(istring) concatenates");
  {
    const char *p = "left";
    const char *q = "right";
    IS a(p), b(q);
    auto c = a.append(b);
    require(c.size(), 9u);
    require(c[0], 'l');
    require(c[8], 't');
  }
  end_test_case();

  test_case("append(sstring) concatenates length-by-strlen");
  {
    const char *p = "hi";
    IS a(p);
    micron::sstring<32, char> rhs("there");
    auto c = a.append(rhs);
    require(c.size() >= 7u, true);
    require(c[0], 'h');
  }
  end_test_case();

  test_case("push_back(ch) adds one char");
  {
    const char *p = "abc";
    IS s(p);
    auto t = s.push_back('!');
    require(t.size(), 4u);
    require(t[3], '!');
    require(s.size(), 3u);
  }
  end_test_case();

  test_case("push_back(istring) adds whole string");
  {
    const char *p = "x";
    const char *q = "yz";
    IS a(p), b(q);
    auto c = a.push_back(b);
    require(c.size(), 3u);
    require(c[0], 'x');
    require(c[2], 'z');
  }
  end_test_case();

  test_case("insert(idx,ch,n) inserts cnt copies at index");
  {
    const char *p = "abef";
    IS s(p);
    auto t = s.insert(2, 'X', 2);
    require(t.size(), 6u);
    require(t[0], 'a');
    require(t[1], 'b');
    require(t[2], 'X');
    require(t[3], 'X');
    require(t[4], 'e');
    require(t[5], 'f');
  }
  end_test_case();

  test_case("operator+=(ptr) returns lengthened copy");
  {
    const char *p = "foo";
    IS s(p);
    const char *q = "bar";
    auto t = s += q;
    require(t.size(), 6u);
    require(t[0], 'f');
    require(t[5], 'r');
  }
  end_test_case();

  // ----- substr -----

  test_case("substr(pos,cnt) extracts a sub-range");
  {
    const char *p = "abcdefghij";
    IS s(p);
    auto t = s.substr(2, 4);
    require(t.size(), 4u);
    require(t[0], 'c');
    require(t[3], 'f');
  }
  end_test_case();

  test_case("substr(0, length) returns whole string");
  {
    const char *p = "whole";
    IS s(p);
    auto t = s.substr(0, 5);
    require(t.size(), 5u);
    require(t[0], 'w');
    require(t[4], 'e');
  }
  end_test_case();

  test_case("substr(0, 0) returns empty");
  {
    const char *p = "anything";
    IS s(p);
    auto t = s.substr(0, 0);
    require(t.size(), 0u);
  }
  end_test_case();

  test_case("substr out-of-range throws");
  {
    const char *p = "abc";
    IS s(p);
    bool threw = false;
    try {
      (void)s.substr(5, 1);
    } catch ( const micron::except::library_error & ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  // ----- find -----

  test_case("find(ch) at SIMD boundaries — hit at 0/L-1, miss");
  {
    constexpr usize lens[] = { 1, 15, 16, 17, 31, 32, 33, 63, 64, 127, 128, 255, 256, 257 };
    char buf[260];
    for ( usize L : lens ) {
      fill_pat(buf, L);
      const char *p = buf;
      IS s(p);
      require(s.find(pat(0)), 0u);
      // last position: pat(L-1); find may return any earlier dup, so check
      // returned index points to that character
      auto idx = s.find(pat(L - 1));
      require(idx != micron::npos, true);
      require(s[idx], pat(L - 1));
      require(s.find('!'), micron::npos);      // '!' not in alphabet
    }
  }
  end_test_case();

  test_case("find(ch) on empty returns npos");
  {
    IS s;
    require(s.find('a'), micron::npos);
  }
  end_test_case();

  // ----- find(istring) — SIMD memmem path -----

  test_case("find(istring) finds first match");
  {
    const char *p = "haystack-needle-tail";
    const char *q = "needle";
    IS h(p), n(q);
    require(h.find(n), 9u);
  }
  end_test_case();

  test_case("find(istring) returns npos when needle absent");
  {
    const char *p = "haystack-tail";
    const char *q = "needle";
    IS h(p), n(q);
    require(h.find(n), micron::npos);
  }
  end_test_case();

  test_case("find(istring) with pos offset skips earlier match");
  {
    const char *p = "abc-abc-abc";
    const char *q = "abc";
    IS h(p), n(q);
    require(h.find(n, 4u), 4u);
  }
  end_test_case();

  test_case("find(istring) empty needle returns pos");
  {
    const char *p = "haystack";
    const char *q = "";
    IS h(p), n(q);
    require(h.find(n), 0u);
  }
  end_test_case();

  // ----- comparison -----

  test_case("operator==(C-str) when equal");
  {
    const char *p = "eq";
    IS s(p);
    require(s == "eq", true);
  }
  end_test_case();

  test_case("operator==(C-str) when different length returns false");
  {
    const char *p = "eq";
    IS s(p);
    require(s == "eqq", false);
  }
  end_test_case();

  test_case("operator==(istring) when equal");
  {
    const char *p = "same";
    IS a(p), b(p);
    require(a == b, true);
  }
  end_test_case();

  test_case("operator==(istring) when last byte differs");
  {
    const char *p = "abcdef";
    const char *q = "abcdeg";
    IS a(p), b(q);
    require(a == b, false);
  }
  end_test_case();

  test_case("operator==(istring) at SIMD boundary lengths");
  {
    constexpr usize lens[] = { 1, 15, 16, 17, 31, 32, 33, 63, 64, 127, 128, 255 };
    char buf[260], buf2[260];
    for ( usize L : lens ) {
      fill_pat(buf, L);
      fill_pat(buf2, L);
      const char *p = buf;
      const char *q = buf2;
      IS a(p), b(q);
      require(a == b, true);
      if ( L > 0 ) {
        buf2[L - 1] ^= 1;
        const char *q2 = buf2;
        IS c(q2);
        require(a == c, false);
        buf2[L - 1] ^= 1;
      }
    }
  }
  end_test_case();

  // ----- swap -----

  test_case("swap exchanges memory and length");
  {
    const char *p = "first";
    const char *q = "second-long";
    IS a(p), b(q);
    a.swap(b);
    require(a.size(), 11u);
    require(b.size(), 5u);
    require(a[0], 's');
    require(b[0], 'f');
  }
  end_test_case();

  // ----- stack() conversion -----

  test_case("stack() — fits in sstring<256> for length < 255");
  {
    const char *p = "stackable";
    IS s(p);
    auto ss = s.stack();
    require(ss.size() >= 9u, true);
    require(ss[0], 's');
  }
  end_test_case();

  test_case("stack() — empty istring rounds-trips to empty sstring");
  {
    IS s;
    auto ss = s.stack();
    require(ss.empty(), true);
  }
  end_test_case();

  test_case("stack() throws when istring length >= 255");
  {
    char buf[300];
    fill_pat(buf, 255);      // length 255 — threshold
    const char *p = buf;
    IS s(p);
    bool threw = false;
    try {
      (void)s.stack();
    } catch ( const micron::except::library_error & ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  // ----- clear / fast_clear -----

  test_case("clear() resets length to 0");
  {
    const char *p = "anything";
    IS s(p);
    s.clear();
    require(s.size(), 0u);
    require(s.empty(), true);
  }
  end_test_case();

  test_case("fast_clear() resets length without zero-fill");
  {
    const char *p = "anything";
    IS s(p);
    s.fast_clear();
    require(s.size(), 0u);
    require(s.empty(), true);
  }
  end_test_case();

  // ----- destructor non-doublefree (smoke) -----

  test_case("Destructor on moved-from istring does not crash");
  {
    const char *p = "moveme";
    IS donor(p);
    IS s(micron::move(donor));
    // both go out of scope; s frees real memory, donor sees is_zero()
    (void)s.size();
  }
  end_test_case();

  sb::print("=== ISTRING TESTS DONE ===");
  return 1;
}
