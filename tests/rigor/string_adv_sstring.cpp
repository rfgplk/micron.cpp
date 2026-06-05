

#include "../../src/io/console.hpp"

#include "../../src/slice.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

using namespace snowball;

template<typename A, typename B>
concept can_starts_with = requires(A a, B b) { a.starts_with(b); };
template<typename A, typename B>
concept can_contains = requires(A a, B b) { a.contains(b); };
template<typename A, typename B>
concept can_compare = requires(A a, B b) { a.compare(b); };
template<typename A, typename B>
concept can_eq = requires(A a, B b) { a == b; };

template<usize N, typename T, bool Sf>
static bool
nul_term_ok(const micron::sstring<N, T, Sf> &s)
{
  if ( s.size() >= N ) return false;
  return static_cast<T>(s[s.size()]) == static_cast<T>(0);
}

template<usize N, typename T, bool Sf>
static bool
nul_ok(const micron::sstring<N, T, Sf> &s)
{
  if ( !nul_term_ok(s) ) return false;
  return micron::strlen(s.c_str()) == s.size();
}

int
main(int, char **)
{
  sb::print("=== SSTRING ADV ===");

  test_case("replace(first,last) swapped iterators throw, no wild memmove");
  {
    micron::sstring<64, char> s("hello world");

    expect_throw_type<micron::except::library_error>([&] { s.replace(s.begin() + 5, s.begin() + 2, "XYZ"); });

    expect_throw_type<micron::except::library_error>([&] { s.replace(s.begin(), s.begin() + 64, "XYZ"); });

    expect_throw_type<micron::except::library_error>([&] { s.replace(s.begin() - 1, s.begin() + 1, "XYZ"); });

    require(s == "hello world", true);
    require(nul_ok(s), true);

    s.replace(s.begin(), s.begin() + 5, "HELLO");
    require(s == "HELLO world", true);
    require(nul_ok(s), true);
  }
  end_test_case();

  test_case("replace(first,last) swapped is a no-op under Sf=false");
  {
    micron::sstring<64, char, false> s("hello world");
    s.replace(s.begin() + 5, s.begin() + 2, "XYZ");
    require(s.size() == 11u, true);
    require(s[0] == 'h', true);
  }
  end_test_case();

  test_case("oversize is_string ctor/assign clamps to N-1, c_str terminated");
  {
    micron::string big;
    big = "ABCDEFGH";
    micron::sstring<8, char> sc(big);
    require(sc.size() == 7u, true);
    require(nul_ok(sc), true);
    require(micron::strlen(sc.c_str()) == 7u, true);

    micron::sstring<8, char> sa;
    sa = big;
    require(sa.size() == 7u, true);
    require(nul_ok(sa), true);

    micron::string fit;
    fit = "ABCDEFG";
    micron::sstring<8, char> se(fit);
    require(se.size() == 7u, true);
    require(nul_ok(se), true);
  }
  end_test_case();

  test_case("cross-capacity converting copy/move ctor clamps to N-1");
  {
    micron::sstring<64, char> big("0123456789ABCDEF");
    micron::sstring<8, char> small(big);
    require(small.size() == 7u, true);
    require(nul_ok(small), true);
    for ( usize i = 0; i < 7; ++i ) require(small[i] == big[i], true);

    micron::sstring<64, char> big2("0123456789ABCDEF");
    micron::sstring<8, char> smv(micron::move(big2));
    require(smv.size() == 7u, true);
    require(nul_ok(smv), true);
    require(big2.size() == 0u, true);
  }
  end_test_case();

  test_case("operator+=(empty slice) is a no-op, non-empty appends right");
  {
    micron::sstring<32, char> s("hi");
    micron::slice<char> empty(nullptr);
    s += empty;
    require(s == "hi", true);
    require(nul_ok(s), true);

    const char abc[4] = { 'a', 'b', 'c', 0 };
    micron::slice<char> sl(&abc[0], &abc[4]);
    s += sl;
    require(s == "hiabc", true);
    require(nul_ok(s), true);
  }
  end_test_case();

  test_case("overflow-safe size check on append/+= rejects absurd counts");
  {
    micron::sstring<16, char> s("abc");
    const char src[8] = { 'x', 'x', 'x', 'x', 'x', 'x', 'x', 0 };
    micron::slice<char> sl(&src[0], &src[8]);

    expect_throw_type<micron::except::library_error>([&] { s.append(sl, static_cast<usize>(-1)); });
    require(s == "abc", true);
    require(nul_ok(s), true);
  }
  end_test_case();

  test_case("array-insert pre-multiply overflow guard throws");
  {
    micron::sstring<32, char> s("seed");

    expect_throw_type<micron::except::library_error>([&] { s.insert(0u, "ab", micron::npos); });
    expect_throw_type<micron::except::library_error>([&] { s.insert(s.begin(), "ab", micron::npos); });
    require(s == "seed", true);
    require(nul_ok(s), true);

    s.insert(0u, "ab", 2u);
    require(s == "ababseed", true);
    require(nul_ok(s), true);
  }
  end_test_case();

  test_case("set_size/_buf_set_length clamp to N-1, keep NUL");
  {
    micron::sstring<64, char> s("payload");
    s.set_size(1000);
    require(s.size() < 64u, true);
    require(s.size() == 63u, true);
    require(nul_term_ok(s), true);

    micron::sstring<64, char> t("payload");
    t._buf_set_length(5000);
    require(t.size() < 64u, true);
    require(t.size() == 63u, true);
    require(nul_term_ok(t), true);

    micron::sstring<64, char> u("payload");
    u.set_size(3);
    require(u.size() == 3u, true);
    require(nul_ok(u), true);
  }
  end_test_case();

  test_case("same-width sstring compare/contains/starts_with still work");
  {
    micron::sstring<16, char> a("alpha");
    micron::sstring<32, char> b("alpha");
    require(a == b, true);
    require(a.compare(b) == 0, true);
    require(a.starts_with(b), true);
    require(a.contains(b), true);
    micron::sstring<16, char> ab("alphabet");
    require(ab.starts_with(a), true);
    require(ab.contains(a), true);
    require(ab.find(a) == 0u, true);
  }
  end_test_case();

  static_assert(!can_starts_with<micron::sstring<8, unicode32>, micron::sstring<8, unicode8>>,
                "cross-width starts_with must be ill-formed");
  static_assert(!can_contains<micron::sstring<8, unicode32>, micron::sstring<8, unicode8>>, "cross-width contains must be ill-formed");
  static_assert(!can_compare<micron::sstring<8, unicode32>, micron::sstring<8, unicode8>>, "cross-width compare must be ill-formed");
  static_assert(can_starts_with<micron::sstring<8, char>, micron::sstring<16, char>>, "same-width starts_with must remain well-formed");
  static_assert(can_eq<micron::sstring<8, char>, micron::sstring<16, char>>, "same-width operator== must remain well-formed");

  sb::print("ALL SSTRING ADV PASSED");
  return 1;
}
