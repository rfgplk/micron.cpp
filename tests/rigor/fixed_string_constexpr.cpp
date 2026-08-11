// fixed_string_constexpr.cpp
// Compile-time proof of the whole micron::fixed_string surface. Every assertion below is a
// static_assert, so the file failing to compile IS the failing test. The runtime main() only
// prints the banner.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/string/fixed_string.hpp"
#include "../../src/string/sstring.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;

constexpr static const mc::fixed_string kHello{ "Hello, World" };
constexpr static const mc::fixed_string kAbc{ "abc" };
constexpr static const mc::fixed_string kEmpty{ "" };

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the two lengths are DIFFERENT THINGS and must stay that way
//
// size() is the capacity N-1, fixed by the type. len() is strnlen(buf, N). they agree only when
// the buffer is full. regex.hpp sizes its VM arrays off size(); everything that reads characters
// uses len()

static_assert(kHello.size() == 12);
static_assert(kHello.len() == 12);
static_assert(kHello.capacity() == 12);
static_assert(mc::fixed_string<3>{}.size() == 2);
static_assert(mc::fixed_string<3>{}.len() == 0);
static_assert(mc::fixed_string<3>{}.empty());

// the (const char*, n) ctor is where the two part company -- this is the shape reflect.hpp builds
static_assert(mc::fixed_string<16>{ "abc", 3 }.size() == 15);
static_assert(mc::fixed_string<16>{ "abc", 3 }.len() == 3);
static_assert(!mc::fixed_string<16>{ "abc", 3 }.empty());
// the ctor caps at N-1 and never runs off the end
static_assert(mc::fixed_string<4>{ "abcdefgh", 8 }.len() == 3);
static_assert(mc::fixed_string<4>{ "abcdefgh", 8 } == "abc");

// N == 1 is a legal empty string, not a buffer underflow
static_assert(mc::fixed_string<1>{}.size() == 0);
static_assert(mc::fixed_string<1>{}.len() == 0);
static_assert(kEmpty.len() == 0);
static_assert(kEmpty.empty());

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// access

static_assert(kHello[0] == 'H');
static_assert(kHello.front() == 'H');
static_assert(kHello.back() == 'd');
static_assert(kHello.last() == 'd');
static_assert(kHello.at(1) == 'e');
static_assert(kHello.c_str()[12] == '\0');
static_assert(kHello.data()[4] == 'o');
static_assert(kHello.cdata()[4] == 'o');
// iterators walk the CONTENT, so end() is len() away and not N-1 away
static_assert(kHello.end() - kHello.begin() == 12);
static_assert(kHello.cend() - kHello.cbegin() == 12);
// a wide buffer holding short content: the iterator range is the CONTENT, not N-1. one named
// object, because two temporaries are two different objects and their pointers do not subtract
constexpr static const mc::fixed_string<16> kShortInWide{ "abc", 3 };
static_assert(kShortInWide.end() - kShortInWide.begin() == 3);
static_assert(kShortInWide.cend() - kShortInWide.cbegin() == 3);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the comparison armada
//
// only <=> and == are written; !=, <, <=, >, >= are synthesised, in both argument orders

static_assert(kAbc == "abc");
static_assert(kAbc != "abd");
static_assert(kAbc < "abd");
static_assert(kAbc <= "abc");
static_assert(kAbc > "abb");
static_assert(kAbc >= "abc");
// reversed operands come free off the member operators
static_assert("abc" == kAbc);
static_assert("abd" > kAbc);
static_assert("abb" < kAbc);

static_assert(mc::fixed_string{ "abc" } < mc::fixed_string{ "abd" });
static_assert(mc::fixed_string{ "abd" } > mc::fixed_string{ "abc" });
static_assert(mc::fixed_string{ "abc" } == mc::fixed_string{ "abc" });

// a prefix sorts before its extension
static_assert(mc::fixed_string{ "ab" } < mc::fixed_string{ "abc" });
static_assert(mc::fixed_string{ "abc" } > mc::fixed_string{ "ab" });

// CROSS-N COMPARISON IS BY CONTENT. before 2026-08-11 the free operator== short-circuited A != B
// to a compile-time false, so this whole block answered the wrong way
static_assert(mc::fixed_string<16>{ "abc", 3 } == mc::fixed_string{ "abc" });
static_assert(mc::fixed_string<16>{ "abc", 3 } == kAbc);
static_assert(mc::fixed_string<32>{ "abc", 3 } == mc::fixed_string<16>{ "abc", 3 });
static_assert(mc::fixed_string<16>{ "abb", 3 } < mc::fixed_string{ "abc" });

// characters compare as UNSIGNED -- 0x80 sorts above 'a', which a signed char compare gets backwards
static_assert(mc::fixed_string<2>{ "\x80", 1 } > mc::fixed_string<2>{ "a", 1 });

// <=> itself yields a real ordering value
static_assert((kAbc <=> mc::fixed_string{ "abd" }) < 0);
static_assert((kAbc <=> mc::fixed_string{ "abc" }) == 0);
static_assert(__is_same(decltype(kAbc <=> kAbc), std::strong_ordering));

// compare() is the general (pointer, length) entry point -- this is how meta::identifier or any
// other ptr+len view compares without fixed_string having to know the type
static_assert(kAbc.compare("abc", 3) == 0);
static_assert(kAbc.compare("abd", 3) < 0);
static_assert(kAbc.compare("abc") == 0);
static_assert(kAbc.compare(mc::fixed_string{ "abd" }) < 0);

// against the other micron string types, through is_string
static_assert(kAbc == mc::sstring<8, char>("abc"));
static_assert(kAbc < mc::sstring<8, char>("abd"));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// search

static_assert(kHello.find('H') == 0);
static_assert(kHello.find(',') == 5);
static_assert(kHello.find('l') == 2);
static_assert(kHello.find('l', 3) == 3);
static_assert(kHello.find('z') == mc::fixed_string<13>::npos);
static_assert(kHello.find("World") == 7);
static_assert(kHello.find("Hello") == 0);
static_assert(kHello.find("nope") == mc::fixed_string<13>::npos);
static_assert(kHello.find(mc::fixed_string{ "World" }) == 7);
// an empty needle matches at pos
static_assert(kHello.find_substr("", 0, 3) == 3);

static_assert(kHello.rfind('l') == 10);
static_assert(kHello.rfind('l', 9) == 3);
static_assert(kHello.rfind('H') == 0);
static_assert(kHello.rfind('z') == mc::fixed_string<13>::npos);
static_assert(kHello.rfind("l") == 10);
static_assert(kHello.rfind("Hello") == 0);

static_assert(kHello.find_first_of("oW") == 4);
static_assert(kHello.find_last_of("oW") == 8);
static_assert(kHello.find_first_of("z") == mc::fixed_string<13>::npos);
static_assert(kHello.find_first_not_of("Hel") == 4);
// "Hello, World": 11 d, 10 l, 9 r, 8 o are all in the set; 7 'W' is the first that is not
static_assert(kHello.find_last_not_of("dlro") == 7);

static_assert(kHello.contains('W'));
static_assert(!kHello.contains('z'));
static_assert(kHello.contains("lo, W"));
static_assert(!kHello.contains("lo,W"));
static_assert(kHello.contains(mc::fixed_string{ "World" }));

static_assert(kHello.starts_with('H'));
static_assert(kHello.starts_with("Hello"));
static_assert(kHello.starts_with("Hello, World"));
static_assert(!kHello.starts_with("Hello, World!"));
static_assert(!kHello.starts_with("ello"));
static_assert(kHello.starts_with(mc::fixed_string{ "Hell" }));

static_assert(kHello.ends_with('d'));
static_assert(kHello.ends_with("World"));
static_assert(kHello.ends_with("Hello, World"));
static_assert(!kHello.ends_with("Worl"));
static_assert(kHello.ends_with(mc::fixed_string{ "rld" }));

static_assert(kHello.count('l') == 3);
static_assert(kHello.count('z') == 0);
static_assert(mc::fixed_string{ "aaaa" }.count("aa") == 2);      // non-overlapping
static_assert(mc::fixed_string{ "abcabc" }.count("abc") == 2);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// transforms -- every one returns a new value, because N is part of the type

static_assert(kHello.to_lower() == "hello, world");
static_assert(kHello.to_upper() == "HELLO, WORLD");
static_assert(kHello.reverse() == "dlroW ,olleH");
static_assert(kAbc.reverse().reverse() == kAbc);

static_assert(kHello.substr<0, 5>() == "Hello");
static_assert(kHello.substr<7, 5>() == "World");
static_assert(kHello.substr<0, 0>() == "");
// the result WIDTH is Cnt+1 regardless of what content lands in it
static_assert(kHello.substr<7, 5>().size() == 5);
static_assert(kHello.substr<7, 5>().len() == 5);

static_assert(mc::fixed_string{ "  hi  " }.trim() == "hi");
static_assert(mc::fixed_string{ "  hi  " }.trim_left() == "hi  ");
static_assert(mc::fixed_string{ "  hi  " }.trim_right() == "  hi");
static_assert(mc::fixed_string{ "  hi  " }.trim().len() == 2);
// a trimmed value keeps its width and zeroes the tail; that is what makes len() the right reader
static_assert(mc::fixed_string{ "  hi  " }.trim().size() == 6);
static_assert(mc::fixed_string{ "    " }.trim().empty());
static_assert(mc::fixed_string{ "hi" }.trim() == "hi");
static_assert(mc::fixed_string{ "\t\n x \r\v\f" }.trim() == "x");

static_assert((mc::fixed_string{ "ab" } + mc::fixed_string{ "cd" }) == "abcd");
static_assert((mc::fixed_string{ "ab" } + mc::fixed_string{ "cd" }).len() == 4);
static_assert((kEmpty + kAbc) == "abc");
static_assert((kAbc + kEmpty) == "abc");
// concatenating short content into wide buffers still lands contiguously
static_assert((mc::fixed_string<16>{ "ab", 2 } + mc::fixed_string<16>{ "cd", 2 }) == "abcd");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the invariants that let the type exist at all
//
// adding <=>, a member operator==, typedefs and static members must NOT cost fixed_string its
// structural-ness -- regex.hpp's cmatch<"pattern"> is nothing but this

template<mc::fixed_string P> struct keyed {
  static constexpr auto key = P;
};

static_assert(keyed<mc::fixed_string{ "xy" }>::key.len() == 2);
static_assert(keyed<mc::fixed_string{ "xy" }>::key == "xy");
static_assert(__is_same(decltype(keyed<mc::fixed_string{ "xy" }>::key), const mc::fixed_string<3>));
// NTTP identity is template-argument equivalence over buf[], NOT operator==. two widths that
// compare equal are still distinct template arguments
static_assert(__is_same(keyed<mc::fixed_string{ "xy" }>, keyed<mc::fixed_string<3>{ "xy", 2 }>));
static_assert(!__is_same(keyed<mc::fixed_string<3>{ "xy", 2 }>, keyed<mc::fixed_string<8>{ "xy", 2 }>));
static_assert(mc::fixed_string<3>{ "xy", 2 } == mc::fixed_string<8>{ "xy", 2 });

// it models is_string now, which is what puts it in reach of every template<is_string S> overload
static_assert(mc::is_string<mc::fixed_string<4>>);

// and it is still trivially copyable, an aggregate-sized literal type
static_assert(mc::is_trivially_copyable_v<mc::fixed_string<8>>);
static_assert(sizeof(mc::fixed_string<8>) == 8);

int
main()
{
  sb::print("=== FIXED_STRING CONSTEXPR PROOFS ===");
  sb::print("=== ALL FIXED_STRING CONSTEXPR TESTS PASSED (at compile time) ===");
  return 1;
}
