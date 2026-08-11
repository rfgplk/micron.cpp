// string_view_constexpr.cpp
// Compile-time proof that micron::string_view<S> is a literal type and that its whole read
// surface constant-evaluates. Every assertion is a static_assert, so the file failing to compile
// IS the failing test; main() only prints the banner.
//
// Before 2026-08-11 none of this was reachable. Four things stood in the way, in order:
//   1. `~string_view() { }` was user-provided and not constexpr, so the type was not a literal
//      type and no constexpr object of it could exist, whatever else was marked
//   2. the eight read accessors carried no constexpr at all on the `cstring_view` copy
//   3. substr() was a non-const member, and a constexpr object is const
//   4. the cross-type constructors reinterpret_cast, which is illegal in constant evaluation --
//      including on the already-constexpr cstring_view, whose ctors were therefore landmines
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/string/string_view.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;

using sv8 = mc::string_view<mc::sstring<8, char>>;
using sv16 = mc::string_view<mc::sstring<16, char>>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a constexpr OBJECT can exist at all -- this is what the destructor blocked

constexpr static const sv8 kHello("hello, world", 12);

static_assert(kHello.size() == 12);
static_assert(!kHello.empty());
static_assert(kHello[0] == 'h');
static_assert(kHello.at(1) == 'e');
static_assert(kHello.front() == 'h');
static_assert(kHello.last() == 'd');
static_assert(*kHello.begin() == 'h');
static_assert(kHello.end() - kHello.begin() == 12);
static_assert(kHello.cend() - kHello.cbegin() == 12);
static_assert(*kHello.ptr(7) == 'w');
static_assert(kHello.data()[7] == 'w');

// the NUL-terminated constructor has to run micron::strlen at compile time
constexpr static const sv8 kLit("abc");
static_assert(kLit.size() == 3);
static_assert(kLit == "abc");

// substr, which needed the const
static_assert(kHello.substr(7, 12).size() == 5);
static_assert(kHello.substr(7, 12) == "world");
static_assert(kHello.substr(7).size() == 5);
static_assert(kHello.substr(0, 5) == "hello");
static_assert(kHello.substr(3, 3).empty());

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// over an sstring. this is the constructor that reinterpret_cast used to poison, and it only
// folds at all because sstring's own dtor stopped calling the non-constexpr cbyteset

constexpr static const mc::sstring<16, char> kStack("abcdef");
constexpr static const sv16 kOverStack(kStack);

static_assert(kOverStack.size() == 6);
static_assert(kOverStack == "abcdef");
static_assert(kOverStack.substr(2, 5) == "cde");
static_assert(kOverStack.front() == 'a');

// the bounded form
constexpr static const sv16 kBounded(kStack, 3);
static_assert(kBounded.size() == 3);
static_assert(kBounded == "abc");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rebinding is constexpr too

consteval bool
__rebinds(void)
{
  sv8 v("abcdef", 6);
  v.advance(2);
  if ( v.size() != 4 || v.front() != 'c' ) return false;
  v.__push(0);
  sv8 w("zz", 2);
  w = v;
  return w.size() == 4 && w.front() == 'c';
}
static_assert(__rebinds());

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// comparison -- the type had none before. only <=> and == are written; the rest is synthesised

static_assert(kLit == "abc");
static_assert(kLit != "abd");
static_assert(kLit < "abd");
static_assert(kLit <= "abc");
static_assert(kLit > "abb");
static_assert(kLit >= "abc");
static_assert("abc" == kLit);
static_assert("abd" > kLit);

static_assert(kLit == mc::fixed_string{ "abc" });
static_assert(kLit < mc::fixed_string{ "abd" });
static_assert(mc::fixed_string{ "abc" } == kLit);

static_assert(kLit == mc::sstring<8, char>("abc"));
static_assert(kLit.compare("abc", 3) == 0);
static_assert(kLit.compare("abd", 3) < 0);

// a prefix sorts before its extension, and equal-length wins on content
static_assert(sv8("ab", 2) < sv8("abc", 3));
static_assert(sv8("abc", 3) > sv8("ab", 2));
static_assert(sv8("abc", 3) == sv8("abc", 3));
static_assert(__is_same(decltype(kLit <=> kLit), std::strong_ordering));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the old spelling names the same type, so nothing downstream had to move

static_assert(mc::is_same_v<mc::cstring_view<mc::sstring<8, char>>, sv8>);
constexpr static const mc::cstring_view<mc::sstring<8, char>> kOld("xy", 2);
static_assert(kOld.size() == 2 && kOld == "xy");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// string_view<hstring> is the one that can NEVER fold -- hstring reaches the heap through an
// allocator base chain with no constexpr member in it. it still has to compile and run

[[gnu::noinline]] static bool
__over_hstring(void)
{
  mc::string owner("the quick brown fox");
  mc::string_view<mc::string> v(owner);
  if ( v.size() != 19 ) return false;
  if ( v.substr(4, 9) != "quick" ) return false;
  v.advance(4);
  if ( v.front() != 'q' ) return false;
  return v == "quick brown fox";
}

int
main()
{
  sb::print("=== STRING_VIEW CONSTEXPR PROOFS ===");
  if ( !__over_hstring() ) {
    sb::print("\033[31mruntime string_view<hstring> FAILED\033[0m");
    return 0;
  }
  sb::print("=== ALL STRING_VIEW CONSTEXPR TESTS PASSED (at compile time) ===");
  return 1;
}
