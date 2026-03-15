//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"
#include "../snowball/snowball.hpp"

#include "../../src/io/console.hpp"

using namespace snowball;
namespace fmt = micron::format;

// ============================================================
// Convenience aliases
// ============================================================
using hstr = micron::hstring<schar>;
template <usize N> using sstr = micron::sstring<N, schar>;

// ============================================================
// Helper: build an hstring from a raw literal
// ============================================================
static hstr
h(const char *s)
{
  return hstr(s);
}

// ============================================================
int
main(int, char **)
{
  sb::print("=== micron::format TEST SUITE ===");

  // ===========================================================
  // SECTION 1 – CHARACTER PREDICATES (fundamental overload)
  // ===========================================================
  sb::print("--- Character predicates (fundamental) ---");

  test_case("isupper / islower – basic ASCII boundaries");
  {
    require_true(fmt::isupper('A'));
    require_true(fmt::isupper('Z'));
    require_false(fmt::isupper('a'));
    require_false(fmt::isupper('z'));
    require_false(fmt::isupper('0'));
    require_false(fmt::isupper(' '));

    require_true(fmt::islower('a'));
    require_true(fmt::islower('z'));
    require_false(fmt::islower('A'));
    require_false(fmt::islower('Z'));
    require_false(fmt::islower('0'));
  }
  end_test_case();

  test_case("isupper / islower – boundary values (one past end)");
  {
    // 0x40 '@' is just below 'A'
    require_false(fmt::isupper('\x40'));
    // 0x5B '[' is just above 'Z'
    require_false(fmt::isupper('\x5B'));
    // 0x60 '`' is just below 'a'
    require_false(fmt::islower('\x60'));
    // 0x7B '{' is just above 'z'
    require_false(fmt::islower('\x7B'));
  }
  end_test_case();

  test_case("isdigit – all digit chars and edges");
  {
    for ( char c = '0'; c <= '9'; ++c )
      require_true(fmt::isdigit(c));
    require_false(fmt::isdigit('/'));     // 0x2F, one below '0'
    require_false(fmt::isdigit(':'));     // 0x3A, one above '9'
    require_false(fmt::isdigit('a'));
    require_false(fmt::isdigit(' '));
  }
  end_test_case();

  test_case("isalpha – letters and non-letters");
  {
    require_true(fmt::isalpha('a'));
    require_true(fmt::isalpha('Z'));
    require_false(fmt::isalpha('0'));
    require_false(fmt::isalpha('!'));
    require_false(fmt::isalpha('\x00'));
  }
  end_test_case();

  test_case("isalnum – mixed");
  {
    require_true(fmt::isalnum('0'));
    require_true(fmt::isalnum('9'));
    require_true(fmt::isalnum('A'));
    require_true(fmt::isalnum('z'));
    require_false(fmt::isalnum(' '));
    require_false(fmt::isalnum('!'));
    require_false(fmt::isalnum('\x7F'));
  }
  end_test_case();

  test_case("isspace – whitespace chars");
  {
    require_true(fmt::isspace(' '));
    require_true(fmt::isspace('\t'));
    require_true(fmt::isspace('\n'));
    require_true(fmt::isspace('\r'));
    require_true(fmt::isspace('\v'));
    require_true(fmt::isspace('\f'));
    require_false(fmt::isspace('a'));
    require_false(fmt::isspace('0'));
  }
  end_test_case();

  test_case("isblank – only space and tab");
  {
    require_true(fmt::isblank(' '));
    require_true(fmt::isblank('\t'));
    require_false(fmt::isblank('\n'));
    require_false(fmt::isblank('\r'));
    require_false(fmt::isblank('a'));
  }
  end_test_case();

  test_case("ispunct – punctuation boundaries");
  {
    require_true(fmt::ispunct('!'));
    require_true(fmt::ispunct('/'));
    require_true(fmt::ispunct(':'));
    require_true(fmt::ispunct('@'));
    require_true(fmt::ispunct('['));
    require_true(fmt::ispunct('`'));
    require_true(fmt::ispunct('{'));
    require_true(fmt::ispunct('~'));
    require_false(fmt::ispunct('a'));
    require_false(fmt::ispunct('0'));
    require_false(fmt::ispunct(' '));
  }
  end_test_case();

  test_case("iscntrl");
  {
    require_true(fmt::iscntrl('\x00'));
    require_true(fmt::iscntrl('\x1F'));
    require_true(fmt::iscntrl('\x7F'));
    require_false(fmt::iscntrl(' '));
    require_false(fmt::iscntrl('a'));
  }
  end_test_case();

  test_case("isxdigit – hex digits");
  {
    for ( char c = '0'; c <= '9'; ++c )
      require_true(fmt::isxdigit(c));
    for ( char c = 'a'; c <= 'f'; ++c )
      require_true(fmt::isxdigit(c));
    for ( char c = 'A'; c <= 'F'; ++c )
      require_true(fmt::isxdigit(c));
    require_false(fmt::isxdigit('g'));
    require_false(fmt::isxdigit('G'));
    require_false(fmt::isxdigit('x'));
    require_false(fmt::isxdigit(' '));
  }
  end_test_case();

  test_case("isascii – full ASCII range");
  {
    require_true(fmt::isascii('\x00'));
    require_true(fmt::isascii('\x7F'));
    // Signed char: values ≥ 128 are negative when cast – test using unsigned
    // Just confirm boundaries
    require_true(fmt::isascii(' '));
    require_true(fmt::isascii('z'));
  }
  end_test_case();

  test_case("isgraph / isprint");
  {
    require_true(fmt::isgraph('!'));
    require_true(fmt::isgraph('~'));
    require_false(fmt::isgraph(' '));     // space not in isgraph
    require_false(fmt::isgraph('\n'));
    require_true(fmt::isprint(' '));     // space IS printable
    require_true(fmt::isprint('~'));
    require_false(fmt::isprint('\n'));
    require_false(fmt::isprint('\x7F'));
  }
  end_test_case();

  // ===========================================================
  // SECTION 2 – to_upper / to_lower (char value)
  // ===========================================================
  sb::print("--- to_upper / to_lower (char value) ---");

  test_case("to_upper / to_lower – every letter");
  {
    for ( char c = 'a'; c <= 'z'; ++c ) {
      char upper = fmt::to_upper(c);
      require(upper, static_cast<char>(c - 32));
    }
    for ( char c = 'A'; c <= 'Z'; ++c ) {
      char lower = fmt::to_lower(c);
      require(lower, static_cast<char>(c + 32));
    }
  }
  end_test_case();

  test_case("to_upper / to_lower – non-alpha pass-through");
  {
    require(fmt::to_upper('0'), '0');
    require(fmt::to_upper('!'), '!');
    require(fmt::to_upper('A'), 'A');     // already upper
    require(fmt::to_lower('a'), 'a');     // already lower
    require(fmt::to_lower('0'), '0');
  }
  end_test_case();

  // ===========================================================
  // SECTION 3 – casefold / upper  (string overloads)
  // ===========================================================
  sb::print("--- casefold / upper (string overloads) ---");

  test_case("casefold – hstring mutable ref (in-place)");
  {
    hstr s("Hello World 123!");
    fmt::casefold(s);
    require(s, h("hello world 123!"));
  }
  end_test_case();

  test_case("casefold – const hstring → new string");
  {
    const hstr s("ABCXYZ");
    hstr r = fmt::casefold(s);
    require(r, h("abcxyz"));
    require(s, h("ABCXYZ"));     // original unchanged
  }
  end_test_case();

  test_case("casefold – already lowercase");
  {
    hstr s("already lower");
    fmt::casefold(s);
    require(s, h("already lower"));
  }
  end_test_case();

  test_case("casefold – empty string");
  {
    hstr s("");
    fmt::casefold(s);
    require(s.empty(), true);
  }
  end_test_case();

  test_case("casefold – const char* + length");
  {
    hstr r = fmt::casefold("HELLO", 5u);
    require(r, h("hello"));
  }
  end_test_case();

  test_case("casefold – const char[N]");
  {
    hstr r = fmt::casefold("MICRON");
    require(r, h("micron"));
  }
  end_test_case();

  test_case("casefold – sstring mutable ref (in-place)");
  {
    sstr<32> s("SSTRING");
    fmt::casefold(s);
    require(s == h("sstring"), true);
  }
  end_test_case();

  test_case("upper – hstring mutable ref (in-place)");
  {
    hstr s("hello world");
    fmt::upper(s);
    require(s, h("HELLO WORLD"));
  }
  end_test_case();

  test_case("upper – const hstring → new string");
  {
    const hstr s("abcdef");
    hstr r = fmt::upper(s);
    require(r, h("ABCDEF"));
    require(s, h("abcdef"));
  }
  end_test_case();

  test_case("upper – empty string");
  {
    hstr s("");
    fmt::upper(s);
    require(s.empty(), true);
  }
  end_test_case();

  test_case("upper – const char* + length");
  {
    hstr r = fmt::upper("hello", 5u);
    require(r, h("HELLO"));
  }
  end_test_case();

  test_case("upper – const char[N]");
  {
    hstr r = fmt::upper("world");
    require(r, h("WORLD"));
  }
  end_test_case();

  test_case("upper – sstring mutable ref");
  {
    sstr<32> s("lowercase");
    fmt::upper(s);
    require(s == h("LOWERCASE"), true);
  }
  end_test_case();

  test_case("upper – raw char* pointer in-place");
  {
    char buf[] = "mutate me";
    fmt::upper(buf);
    require(hstr(buf), h("MUTATE ME"));
  }
  end_test_case();

  test_case("casefold – raw char* pointer in-place");
  {
    char buf[] = "MUTATE ME";
    fmt::casefold(buf);
    require(hstr(buf), h("mutate me"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 4 – isXXX_all / isXXX_any
  // ===========================================================
  sb::print("--- isXXX_all / isXXX_any ---");

  test_case("isdigit_all – all digits");
  {
    hstr s("1234567890");
    require_true(fmt::isdigit_all(s));
  }
  end_test_case();

  test_case("isdigit_all – mixed");
  {
    hstr s("123abc");
    require_false(fmt::isdigit_all(s));
  }
  end_test_case();

  test_case("isdigit_all – empty → false");
  {
    hstr s("");
    require_false(fmt::isdigit_all(s));
  }
  end_test_case();

  test_case("isalpha_all – all letters");
  {
    hstr s("HelloWorld");
    require_true(fmt::isalpha_all(s));
  }
  end_test_case();

  test_case("isalpha_all – with digit");
  {
    hstr s("Hello1");
    require_false(fmt::isalpha_all(s));
  }
  end_test_case();

  test_case("isalnum_all – letters + digits");
  {
    hstr s("abc123");
    require_true(fmt::isalnum_all(s));
  }
  end_test_case();

  test_case("isalnum_all – with punctuation");
  {
    hstr s("abc!");
    require_false(fmt::isalnum_all(s));
  }
  end_test_case();

  test_case("isspace_all / isspace_any");
  {
    hstr all_ws("   \t\n  ");
    hstr mixed("  hello ");
    hstr none("hello");
    require_true(fmt::isspace_all(all_ws));
    require_false(fmt::isspace_all(mixed));
    require_true(fmt::isspace_any(mixed));
    require_false(fmt::isspace_any(none));
  }
  end_test_case();

  test_case("isupper_all / islower_all");
  {
    hstr upper_s("HELLO");
    hstr lower_s("hello");
    hstr mixed("Hello");
    require_true(fmt::isupper_all(upper_s));
    require_false(fmt::isupper_all(mixed));
    require_true(fmt::islower_all(lower_s));
    require_false(fmt::islower_all(mixed));
  }
  end_test_case();

  test_case("isxdigit_all – valid and invalid hex strings");
  {
    hstr valid("0123456789abcdefABCDEF");
    hstr invalid("0x1A");     // '0x' has 'x' which is not xdigit
    require_true(fmt::isxdigit_all(valid));
    require_false(fmt::isxdigit_all(invalid));
  }
  end_test_case();

  test_case("isascii_all – pure ASCII");
  {
    hstr s("Hello World!\n");
    require_true(fmt::isascii_all(s));
  }
  end_test_case();

  test_case("isblank_all – spaces and tabs only");
  {
    hstr s("   \t\t ");
    require_true(fmt::isblank_all(s));
    hstr ns("  \n ");
    require_false(fmt::isblank_all(ns));
  }
  end_test_case();

  test_case("ispunct_all / ispunct_any");
  {
    hstr all_p("!@#$%");
    hstr with_alpha("!abc");
    require_true(fmt::ispunct_all(all_p));
    require_false(fmt::ispunct_all(with_alpha));
    require_true(fmt::ispunct_any(with_alpha));
    hstr no_p("abc");
    require_false(fmt::ispunct_any(no_p));
  }
  end_test_case();

  // ===========================================================
  // SECTION 5 – strip
  // ===========================================================
  sb::print("--- strip ---");

  test_case("strip – leading and trailing spaces (hstring)");
  {
    hstr s("  hello  ");
    fmt::strip(s);
    require(s, h("hello"));
  }
  end_test_case();

  test_case("strip – no leading/trailing (hstring)");
  {
    hstr s("hello");
    fmt::strip(s);
    require(s, h("hello"));
  }
  end_test_case();

  test_case("strip – all spaces");
  {
    hstr s("    ");
    // strip until empty
    fmt::strip(s);
    require(s.empty(), true);
  }
  end_test_case();

  test_case("strip – single char (not token)");
  {
    hstr s("x");
    fmt::strip(s);
    require(s, h("x"));
  }
  end_test_case();

  test_case("strip – custom token (comma)");
  {
    hstr s(",,hello,,");
    fmt::strip<','>(s);
    require(s, h("hello"));
  }
  end_test_case();

  test_case("strip – const hstring → new string");
  {
    const hstr s("  world  ");
    hstr r = fmt::strip(s);
    require(r, h("world"));
    require(s, h("  world  "));     // original unchanged
  }
  end_test_case();

  test_case("strip – const char[N]");
  {
    hstr r = fmt::strip("  trimmed  ");
    require(r, h("trimmed"));
  }
  end_test_case();

  test_case("strip – sstring mutable ref");
  {
    sstr<64> s("  sstack  ");
    fmt::strip(s);
    require(s == h("sstack"), true);
  }
  end_test_case();

  // ===========================================================
  // SECTION 6 – starts_with / ends_with
  // ===========================================================
  sb::print("--- starts_with / ends_with ---");

  test_case("starts_with – hstring + const char*");
  {
    hstr s("hello world");
    require_true(fmt::starts_with(s, "hello"));
    require_false(fmt::starts_with(s, "world"));
    require_false(fmt::starts_with(s, "hello world extra"));
  }
  end_test_case();

  test_case("starts_with – hstring + hstring");
  {
    hstr s("micron string");
    hstr pfx("micron");
    hstr bad("string");
    require_true(fmt::starts_with(s, pfx));
    require_false(fmt::starts_with(s, bad));
  }
  end_test_case();

  test_case("starts_with – empty prefix always true");
  {
    hstr s("anything");
    require_true(fmt::starts_with(s, ""));
  }
  end_test_case();

  test_case("starts_with – const char* + length");
  {
    require_true(fmt::starts_with("hello world", 11u, "hello"));
    require_false(fmt::starts_with("hello world", 11u, "world"));
  }
  end_test_case();

  test_case("starts_with – const char[N] + const char[M]");
  {
    require_true(fmt::starts_with("foobar", "foo"));
    require_false(fmt::starts_with("foobar", "bar"));
  }
  end_test_case();

  test_case("ends_with – hstring + const char*");
  {
    hstr s("hello world");
    require_true(fmt::ends_with(s, "world"));
    require_false(fmt::ends_with(s, "hello"));
    require_false(fmt::ends_with(s, "hello world extra"));
  }
  end_test_case();

  test_case("ends_with – hstring + hstring");
  {
    hstr s("hello world");
    hstr sfx("world");
    hstr bad("hello");
    require_true(fmt::ends_with(s, sfx));
    require_false(fmt::ends_with(s, bad));
  }
  end_test_case();

  test_case("ends_with – const char* + length");
  {
    require_true(fmt::ends_with("hello world", 11u, "world"));
    require_false(fmt::ends_with("hello world", 11u, "hello"));
  }
  end_test_case();

  test_case("ends_with – const char[N] + const char[M]");
  {
    require_true(fmt::ends_with("foobar", "bar"));
    require_false(fmt::ends_with("foobar", "foo"));
  }
  end_test_case();

  test_case("starts_with – sstring");
  {
    sstr<32> s("sstring test");
    require_true(fmt::starts_with(s, "sstring"));
    require_false(fmt::starts_with(s, "test"));
  }
  end_test_case();

  test_case("ends_with – sstring");
  {
    sstr<32> s("sstring test");
    require_true(fmt::ends_with(s, "test"));
    require_false(fmt::ends_with(s, "sstring"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 7 – contains
  // ===========================================================
  sb::print("--- contains ---");

  test_case("contains – hstring + char");
  {
    hstr s("hello");
    require_true(fmt::contains(s, 'e'));
    require_true(fmt::contains(s, 'o'));
    require_false(fmt::contains(s, 'z'));
  }
  end_test_case();

  test_case("contains – hstring + const char*");
  {
    hstr s("hello world");
    require_true(fmt::contains(s, "world"));
    require_true(fmt::contains(s, "hello"));
    require_true(fmt::contains(s, "lo wo"));
    require_false(fmt::contains(s, "xyz"));
    require_false(fmt::contains(s, "hello world extra"));
  }
  end_test_case();

  test_case("contains – hstring + hstring");
  {
    hstr s("abcdef");
    hstr needle("cde");
    hstr absent("xyz");
    require_true(fmt::contains(s, needle));
    require_false(fmt::contains(s, absent));
  }
  end_test_case();

  test_case("contains – empty haystack");
  {
    hstr s("");
    require_false(fmt::contains(s, 'a'));
    require_false(fmt::contains(s, "a"));
  }
  end_test_case();

  test_case("contains – needle longer than haystack");
  {
    hstr s("abc");
    require_false(fmt::contains(s, "abcdef"));
  }
  end_test_case();

  test_case("contains – raw pointer + length + char");
  {
    require_true(fmt::contains("hello", 5u, 'e'));
    require_false(fmt::contains("hello", 5u, 'z'));
  }
  end_test_case();

  test_case("contains – raw pointer + length + const char*");
  {
    require_true(fmt::contains("hello world", 11u, "world"));
    require_false(fmt::contains("hello world", 11u, "xyz"));
  }
  end_test_case();

  test_case("contains – const char[N] + const char[M]");
  {
    require_true(fmt::contains("abcdef", "cde"));
    require_false(fmt::contains("abcdef", "xyz"));
  }
  end_test_case();

  test_case("contains – sstring");
  {
    sstr<32> s("sstack contains");
    require_true(fmt::contains(s, "contains"));
    require_false(fmt::contains(s, "absent"));
  }
  end_test_case();

  test_case("contains – from-iterator variants");
  {
    hstr s("abcabc");
    auto from = s.begin() + 3;     // second half
    require_true(fmt::contains(s, from, 'a'));
    require_false(fmt::contains(s, from, 'x'));
    require_true(fmt::contains(s, from, "abc"));
    require_false(fmt::contains(s, from, "xyz"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 8 – find / fast_find
  // ===========================================================
  sb::print("--- find / fast_find ---");

  test_case("find – hstring + char");
  {
    hstr s("hello");
    auto it = fmt::find(s, 'l');
    require(it, s.begin() + 2);
    auto none = fmt::find(s, 'z');
    require(none, (hstr::iterator) nullptr);
  }
  end_test_case();

  test_case("find – hstring + const char*");
  {
    hstr s("hello world");
    auto it = fmt::find(s, "world");
    require(it, s.begin() + 6);
    auto none = fmt::find(s, "xyz");
    require(none, (hstr::const_iterator) nullptr);
  }
  end_test_case();

  test_case("find – hstring + hstring");
  {
    hstr s("abcdef");
    hstr needle("cde");
    auto it = fmt::find(s, needle);
    require(it, s.begin() + 2);
    hstr absent("xyz");
    require(fmt::find(s, absent), (hstr::iterator) nullptr);
  }
  end_test_case();

  test_case("find – empty haystack");
  {
    hstr s("");
    require(fmt::find(s, 'a'), (hstr::iterator) nullptr);
  }
  end_test_case();

  test_case("find – needle at beginning");
  {
    hstr s("prefix_suffix");
    auto it = fmt::find(s, "prefix");
    require(it, s.begin());
  }
  end_test_case();

  test_case("find – needle at end");
  {
    hstr s("prefix_suffix");
    auto it = fmt::find(s, "suffix");
    require(it, s.begin() + 7);
  }
  end_test_case();

  test_case("find – from-iterator + const char*");
  {
    hstr s("abcabc");
    auto from = s.begin() + 1;
    auto it = fmt::find(s, from, "bc");
    require(it, s.begin() + 1);
    from = s.begin() + 3;
    it = fmt::find(s, from, "bc");
    require(it, s.begin() + 4);
  }
  end_test_case();

  test_case("find – raw pointer + length + char");
  {
    const char *data = "hello";
    const char *res = fmt::find(data, 5u, 'l');
    require(res, data + 2);
    require(fmt::find(data, 5u, 'z'), (const char *)nullptr);
  }
  end_test_case();

  test_case("find – raw pointer + length + const char*");
  {
    const char *data = "hello world";
    const char *res = fmt::find(data, 11u, "world");
    require(res, data + 6);
    require(fmt::find(data, 11u, "xyz"), (const char *)nullptr);
  }
  end_test_case();

  test_case("find – const char[N] + char");
  {
    const char *res = fmt::find("hello", 'l');
    require(res, static_cast<const char *>("hello") + 2);
  }
  end_test_case();

  test_case("find – sstring + const char*");
  {
    sstr<64> s("stack find test");
    auto it = fmt::find(s, "find");
    require(it, s.begin() + 6);
  }
  end_test_case();

  test_case("fast_find – hstring + const char*");
  {
    hstr s("abcdefgh");
    auto it = fmt::fast_find(s, "def");
    require(it, s.begin() + 3);
    require(fmt::fast_find(s, "xyz"), s.end());
  }
  end_test_case();

  test_case("fast_find – hstring + hstring");
  {
    hstr s("the quick brown fox");
    hstr needle("brown");
    auto it = fmt::fast_find(s, needle);
    require(it, s.begin() + 10);
  }
  end_test_case();

  test_case("fast_find – raw pointer + length");
  {
    const char *data = "hello world";
    const char *res = fmt::fast_find(data, 11u, "world");
    require(res, data + 6);
    require(fmt::fast_find(data, 11u, "xyz"), (const char *)nullptr);
  }
  end_test_case();

  test_case("fast_find – const char[N] + const char[M]");
  {
    const char *res = fmt::fast_find("abcdef", "cde");
    require_true(res != nullptr);
  }
  end_test_case();

  // ===========================================================
  // SECTION 9 – find_reverse
  // ===========================================================
  sb::print("--- find_reverse ---");

  test_case("find_reverse – hstring + const_iterator + const char*");
  {
    hstr s("abcabc");
    auto from = s.cend() - 1;
    auto it = fmt::find_reverse(s, from, "abc");
    require(it, s.cbegin() + 3);
  }
  end_test_case();
  test_case("find_reverse – hstring + iterator + const char*");
  {
    hstr s("xyzxyz");
    auto from = s.end() - 1;
    auto it = fmt::find_reverse(s, from, "xyz");
    require(it, s.begin() + 3);
  }
  end_test_case();

  test_case("find_reverse – raw pointer + length + offset + char");
  {
    const char *data = "abcabc";
    const char *res = fmt::find_reverse(data, 6u, 5u, 'a');
    require(res, data + 3);
  }
  end_test_case();

  test_case("find_reverse – raw pointer, char not found");
  {
    const char *data = "hello";
    require(fmt::find_reverse(data, 5u, 4u, 'z'), (const char *)nullptr);
  }
  end_test_case();

  test_case("find_reverse – const char[N] + offset + char");
  {
    const char *res = fmt::find_reverse("abcabc", 5u, 'a');
    require_true(res != nullptr);
  }
  end_test_case();

  // ===========================================================
  // SECTION 10 – is_in / is_not_in
  // ===========================================================
  sb::print("--- is_in / is_not_in ---");

  test_case("is_in – hstring");
  {
    hstr s("hello world");
    require_true(fmt::is_in(s, "world"));
    require_false(fmt::is_in(s, "xyz"));
    require_true(fmt::is_in("world", s));
    require_false(fmt::is_in("xyz", s));
    require_true(fmt::is_in(s, h("world")));
    require_false(fmt::is_in(s, h("xyz")));
  }
  end_test_case();

  test_case("is_not_in – hstring");
  {
    hstr s("hello world");
    require_false(fmt::is_not_in(s, "world"));
    require_true(fmt::is_not_in(s, "xyz"));
    require_false(fmt::is_not_in("world", s));
    require_true(fmt::is_not_in("xyz", s));
  }
  end_test_case();

  test_case("is_in – raw pointer + length");
  {
    require_true(fmt::is_in("hello world", 11u, "world"));
    require_false(fmt::is_in("hello world", 11u, "xyz"));
    require_true(fmt::is_in("hello world", 11u, 'w'));
    require_false(fmt::is_in("hello world", 11u, 'z'));
  }
  end_test_case();

  test_case("is_not_in – raw pointer + length");
  {
    require_false(fmt::is_not_in("hello world", 11u, "world"));
    require_true(fmt::is_not_in("hello world", 11u, "xyz"));
  }
  end_test_case();

  test_case("is_in – const char[N] + const char[M]");
  {
    require_true(fmt::is_in("foobar", "bar"));
    require_false(fmt::is_in("foobar", "baz"));
  }
  end_test_case();

  test_case("is_in – sstring");
  {
    sstr<64> s("sstack is in test");
    require_true(fmt::is_in(s, "in"));
    require_false(fmt::is_in(s, "absent"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 11 – replace / replace_all
  // ===========================================================
  sb::print("--- replace / replace_all ---");

  test_case("replace – hstring, same length");
  {
    hstr s("hello world");
    fmt::replace(s, "world", "earth");
    require(s, h("hello earth"));
  }
  end_test_case();

  test_case("replace – hstring, shorter replacement");
  {
    hstr s("hello world");
    fmt::replace(s, "world", "hi");
    require(s, h("hello hi"));
  }
  end_test_case();

  test_case("replace – hstring, longer replacement");
  {
    hstr s("hello world");
    fmt::replace(s, "world", "everyone");
    require(s, h("hello everyone"));
  }
  end_test_case();

  test_case("replace – needle not found (no change)");
  {
    hstr s("hello world");
    fmt::replace(s, "xyz", "abc");
    require(s, h("hello world"));
  }
  end_test_case();

  test_case("replace – only first occurrence");
  {
    hstr s("aaa");
    fmt::replace(s, "a", "b");
    require(s, h("baa"));
  }
  end_test_case();

  test_case("replace – const hstring → new string");
  {
    const hstr s("hello world");
    hstr r = fmt::replace(s, "world", "earth");
    require(r, h("hello earth"));
    require(s, h("hello world"));     // original unchanged
  }
  end_test_case();

  test_case("replace – const char[N]");
  {
    hstr r = fmt::replace("hello world", "world", "earth");
    require(r, h("hello earth"));
  }
  end_test_case();

  test_case("replace – sstring");
  {
    sstr<64> s("stack replace");
    fmt::replace(s, "replace", "done");
    require(s == h("stack done"), true);
  }
  end_test_case();

  test_case("replace_all – replaces every occurrence");
  {
    hstr s("aababaa");
    fmt::replace_all(s, "a", "x");
    require(s, h("xxbxbxx"));
  }
  end_test_case();

  test_case("replace_all – no occurrences");
  {
    hstr s("hello");
    fmt::replace_all(s, "z", "x");
    require(s, h("hello"));
  }
  end_test_case();

  test_case("replace_all – replace with longer string");
  {
    hstr s("ab ab ab");
    fmt::replace_all(s, "ab", "xyz");
    require(s, h("xyz xyz xyz"));
  }
  end_test_case();

  test_case("replace_all – replace with shorter string");
  {
    hstr s("aabbcc");
    fmt::replace_all(s, "bb", "b");
    require(s, h("aabcc"));
    // note: just confirm it shortened
    require_smaller(s.size(), 7u);
  }
  end_test_case();

  test_case("replace_all – const hstring → new string");
  {
    const hstr s("xxx");
    hstr r = fmt::replace_all(s, "x", "ab");
    require(r, h("ababab"));
    require(s, h("xxx"));
  }
  end_test_case();

  test_case("replace_all – const char[N]");
  {
    hstr r = fmt::replace_all("xox", "x", "ab");
    require(r, h("aboab"));
  }
  end_test_case();

  test_case("replace_all – sstring");
  {
    sstr<64> s("aaaa");
    fmt::replace_all(s, "aa", "b");
    require(s == h("bb"), true);
  }
  end_test_case();

  // ===========================================================
  // SECTION 12 – concat
  // ===========================================================
  sb::print("--- concat ---");

  test_case("concat – two const char* → hstring");
  {
    hstr r = fmt::concat("hello", " world");
    require(r, h("hello world"));
  }
  end_test_case();

  test_case("concat – const char[N] + const char[M]");
  {
    hstr r = fmt::concat("foo", "bar");
    require(r, h("foobar"));
  }
  end_test_case();

  test_case("concat – two const char* with lengths");
  {
    hstr r = fmt::concat("hello", 5u, " world", 6u);
    require(r, h("hello world"));
  }
  end_test_case();

  test_case("concat – hstring + hstring → new hstring");
  {
    hstr a("left ");
    hstr b("right");
    hstr r = fmt::concat(a, b);
    require(r, h("left right"));
    require(a, h("left "));     // originals unchanged
    require(b, h("right"));
  }
  end_test_case();

  test_case("concat – variadic is_string pack");
  {
    hstr a("a"), b("b"), c("c");
    hstr r = fmt::concat(a, b, c);
    require(r, h("abc"));
  }
  end_test_case();

  test_case("concat – empty strings");
  {
    hstr r = fmt::concat("", "hello");
    require(r, h("hello"));
    hstr r2 = fmt::concat("hello", "");
    require(r2, h("hello"));
  }
  end_test_case();

  test_case("concat – sstring prepend (const char*)");
  {
    sstr<64> s("world");
    fmt::concat("hello ", s);
    require(s == h("hello world"), true);
  }
  end_test_case();

  // ===========================================================
  // SECTION 13 – split
  // ===========================================================
  sb::print("--- split ---");

  test_case("split – hstring at halfway (default)");
  {
    hstr s("abcdef");
    hstr r = fmt::split(s);
    require(r, h("def"));
  }
  end_test_case();

  test_case("split – hstring at explicit position");
  {
    hstr s("hello world");
    hstr r = fmt::split(s, 6u);
    require(r, h("world"));
  }
  end_test_case();

  test_case("split – hstring out of bounds throws");
  {
    hstr s("hi");

    require_throw([&]() { fmt::split(s, 100u); });
  }
  end_test_case();

  test_case("split – hstring const_iterator");
  {
    hstr s("abcdef");
    auto mid = s.cbegin() + 3;
    // returns head (data.begin() .. itr)
    hstr r = fmt::split(s, mid);
    require(r, h("def"));
  }
  end_test_case();

  test_case("split – raw pointer + length + offset");
  {
    hstr r = fmt::split("hello world", 11u, 6u);
    require(r, h("world"));
  }
  end_test_case();

  test_case("split – raw pointer + length + delimiter char");
  {
    hstr head = fmt::split_delim("key=value", 9u, '=');
    require(head, h("key"));
  }
  end_test_case();

  test_case("split – raw pointer out of bounds throws");
  {
    require_throw([&] { fmt::split("hi", 2u, 100u); });
  }
  end_test_case();

  test_case("split – const char[N]");
  {
    hstr r = fmt::split("abcdef");
    require(r, h("def"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 14 – to_integer / to_long
  // ===========================================================
  sb::print("--- to_integer / to_long ---");

  test_case("to_integer – hstring basic");
  {
    hstr s("12345");
    require(fmt::to_integer(s), 12345);
  }
  end_test_case();

  test_case("to_integer – leading spaces");
  {
    hstr s("  42");
    require(fmt::to_integer(s), 42);
  }
  end_test_case();

  test_case("to_integer – zero");
  {
    hstr s("0");
    require(fmt::to_integer(s), 0);
  }
  end_test_case();

  test_case("to_integer – raw pointer");
  {
    require(fmt::to_integer("999"), 999);
    require(fmt::to_integer("-42"), -42);
    require(fmt::to_integer("+7"), 7);
  }
  end_test_case();

  test_case("to_integer – const char* + length");
  {
    // only read first 3 chars of "1234"
    require(fmt::to_integer("1234", 3u), 123);
  }
  end_test_case();

  test_case("to_integer – const char[N]");
  {
    require(fmt::to_integer("256"), 256);
  }
  end_test_case();

  test_case("to_long – hstring");
  {
    hstr s("1000000");
    require(fmt::to_long(s), (i64)1000000);
  }
  end_test_case();

  test_case("to_long – raw pointer negative");
  {
    require(fmt::to_long("-99999"), (i64)-99999);
  }
  end_test_case();

  test_case("to_long – const char* + length");
  {
    require(fmt::to_long("123456", 4u), (i64)1234);
  }
  end_test_case();

  test_case("to_long – const char[N]");
  {
    require(fmt::to_long("9876"), (i64)9876);
  }
  end_test_case();

  test_case("to_long – sstring");
  {
    sstr<32> s("42");
    require(fmt::to_long(s), (i64)42);
  }
  end_test_case();

  // ===========================================================
  // SECTION 15 – to_float / to_double
  // ===========================================================
  sb::print("--- to_float / to_double ---");

  test_case("to_float – integer part only");
  {
    hstr s("123");
    f32 v = fmt::to_float(s);
    require(v > 122.9f, true);
    require(v < 123.1f, true);
  }
  end_test_case();

  test_case("to_float – decimal part");
  {
    hstr s("3.14");
    f32 v = fmt::to_float(s);
    require(v > 3.13f, true);
    require(v < 3.15f, true);
  }
  end_test_case();

  test_case("to_float – raw pointer");
  {
    f32 v = fmt::to_float("2.5");
    require(v > 2.4f, true);
    require(v < 2.6f, true);
  }
  end_test_case();

  test_case("to_float – negative raw pointer");
  {
    f32 v = fmt::to_float("-1.5");
    require(v < -1.4f, true);
    require(v > -1.6f, true);
  }
  end_test_case();

  test_case("to_float – const char* + length");
  {
    // "3.14159" truncated to 4 chars → "3.14"
    f32 v = fmt::to_float("3.14159", 4u);
    require(v > 3.13f, true);
    require(v < 3.15f, true);
  }
  end_test_case();

  test_case("to_float – const char[N]");
  {
    f32 v = fmt::to_float("1.0");
    require(v > 0.99f, true);
    require(v < 1.01f, true);
  }
  end_test_case();

  test_case("to_double – hstring");
  {
    hstr s("2.71828");
    f64 v = fmt::to_double(s);
    require(v > 2.71, true);
    require(v < 2.72, true);
  }
  end_test_case();

  test_case("to_double – raw pointer negative");
  {
    f64 v = fmt::to_double("-3.14");
    require(v < -3.13, true);
    require(v > -3.15, true);
  }
  end_test_case();

  test_case("to_double – const char* + length");
  {
    f64 v = fmt::to_double("9.9999", 3u);     // "9.9"
    require(v > 9.8, true);
    require(v < 10.0, true);
  }
  end_test_case();

  test_case("to_double – const char[N]");
  {
    f64 v = fmt::to_double("0.5");
    require(v > 0.49, true);
    require(v < 0.51, true);
  }
  end_test_case();

  test_case("to_double – sstring");
  {
    sstr<32> s("1.23");
    f64 v = fmt::to_double(s);
    require(v > 1.22, true);
    require(v < 1.24, true);
  }
  end_test_case();

  // ===========================================================
  // SECTION 16 – OOB / edge-case guards
  // ===========================================================
  sb::print("--- OOB and edge-case guards ---");

  test_case("find – from-iterator OOB (before begin) returns null");
  {
    hstr s("hello");
    auto bad = s.begin() - 1;
    auto result = fmt::find(s, bad, "he");
    require(result, (hstr::iterator) nullptr);
  }
  end_test_case();
  test_case("find – from-iterator OOB (at end) returns null");
  {
    hstr s("hello");
    auto bad = s.end();
    auto result = fmt::find(s, bad, "he");
    require(result, (hstr::iterator) nullptr);
  }
  end_test_case();

  test_case("find_reverse – from-iterator OOB (before begin) returns null");
  {
    hstr s("hello");
    auto bad = s.cbegin() - 1;
    auto result = fmt::find_reverse(s, bad, "he");
    require(result, (hstr::const_iterator) nullptr);
  }
  end_test_case();

  test_case("contains – from-iterator OOB returns false");
  {
    hstr s("hello");
    auto bad_before = s.cbegin() - 1;
    auto bad_after = s.cend();
    require_false(fmt::contains(s, bad_before, 'h'));
    require_false(fmt::contains(s, bad_after, 'h'));
  }
  end_test_case();

  test_case("split – out-of-bounds const_iterator throws");
  {
    hstr s("hello");
    auto bad = s.cend() + 1;
    require_throw([&] { fmt::split(s, bad); });
  }
  end_test_case();

  test_case("strip – raw pointer null-safe (should not crash)");
  {
    // The raw char* + len overload should handle len == 0 gracefully
    char buf[] = "  x  ";
    usize len = 5u;
    fmt::strip(buf, len);
    require(buf, h("x"));
  }
  end_test_case();

  test_case("replace – needle not present, string unchanged");
  {
    hstr s("nothing to replace here");
    hstr orig(s);
    fmt::replace(s, "zzz", "aaa");
    require(s == orig, true);
  }
  end_test_case();

  test_case("replace_all – needle equals replacement (idempotent)");
  {
    hstr s("hello");
    hstr orig(s);
    fmt::replace_all(s, "hello", "hello");
    require(s == orig, true);
  }
  end_test_case();

  test_case("to_integer – empty string returns 0");
  {
    require(fmt::to_integer(""), 0);
  }
  end_test_case();

  test_case("to_long – null pointer returns 0");
  {
    require(fmt::to_long((const char *)nullptr), (i64)0);
  }
  end_test_case();

  test_case("to_float – null pointer returns 0.0f");
  {
    f32 v = fmt::to_float((const char *)nullptr);
    require(v > -0.001f, true);
    require(v < 0.001f, true);
  }
  end_test_case();

  test_case("to_double – null pointer returns 0.0");
  {
    f64 v = fmt::to_double((const char *)nullptr);
    require(v > -0.001, true);
    require(v < 0.001, true);
  }
  end_test_case();

  test_case("starts_with – prefix longer than string returns false");
  {
    hstr s("hi");
    require_false(fmt::starts_with(s, "hello world"));
  }
  end_test_case();

  test_case("ends_with – suffix longer than string returns false");
  {
    hstr s("hi");
    require_false(fmt::ends_with(s, "hello world"));
  }
  end_test_case();

  test_case("find – needle longer than haystack returns null");
  {
    hstr s("hi");
    require(fmt::find(s, "hello world"), (hstr::const_iterator) nullptr);
  }
  end_test_case();

  test_case("fast_find – needle longer than haystack returns end");
  {
    hstr s("hi");
    require(fmt::fast_find(s, "hello world"), s.end());
  }
  end_test_case();

  // ===========================================================
  // SECTION 17 – sstring-specific paths
  // ===========================================================
  sb::print("--- sstring-specific ---");

  test_case("sstring casefold + upper round trip");
  {
    sstr<32> s("Hello World");
    fmt::upper(s);
    require(s == h("HELLO WORLD"), true);
    fmt::casefold(s);
    require(s == h("hello world"), true);
  }
  end_test_case();

  test_case("sstring strip leading/trailing");
  {
    sstr<32> s("  trim  ");
    fmt::strip(s);
    require(s == h("trim"), true);
  }
  end_test_case();

  test_case("sstring replace single occurrence");
  {
    sstr<64> s("stack is nice");
    fmt::replace(s, "nice", "great");
    require(s == h("stack is great"), true);
  }
  end_test_case();

  test_case("sstring replace_all");
  {
    sstr<64> s("aababaa");
    fmt::replace_all(s, "a", "x");
    require(s == h("xxbxbxx"), true);
  }
  end_test_case();

  test_case("sstring find char and pointer");
  {
    sstr<32> s("sstack");
    auto it = fmt::find(s, 's');
    require(it, s.begin());
    auto it2 = fmt::find(s, "tack");
    require(it2, s.begin() + 2);
  }
  end_test_case();

  test_case("sstring contains");
  {
    sstr<32> s("unit tests");
    require_true(fmt::contains(s, "tests"));
    require_false(fmt::contains(s, "absent"));
    require_true(fmt::contains(s, 't'));
    require_false(fmt::contains(s, 'z'));
  }
  end_test_case();

  test_case("sstring starts_with / ends_with");
  {
    sstr<32> s("snowball");
    require_true(fmt::starts_with(s, "snow"));
    require_false(fmt::starts_with(s, "ball"));
    require_true(fmt::ends_with(s, "ball"));
    require_false(fmt::ends_with(s, "snow"));
  }
  end_test_case();

  test_case("sstring to_integer / to_long / to_float / to_double");
  {
    sstr<32> si("42");
    sstr<32> sl("100000");
    sstr<32> sf("3.14");
    sstr<32> sd("2.71");
    require(fmt::to_integer(si), 42);
    require(fmt::to_long(sl), (i64)100000);
    f32 fv = fmt::to_float(sf);
    require(fv > 3.13f, true);
    require(fv < 3.15f, true);
    f64 dv = fmt::to_double(sd);
    require(dv > 2.70, true);
    require(dv < 2.72, true);
  }
  end_test_case();

  sb::print("=== ALL FORMAT TESTS PASSED ===");
  return 1;
}
