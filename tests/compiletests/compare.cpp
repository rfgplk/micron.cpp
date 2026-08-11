//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: micron's std:: comparison-category port. Not run -- every assertion is a
// static_assert, so failing to compile IS the failing test.
//
// this has to hold in the freestanding cells too (-k / -ke). a compiler-required-header port that
// only works hosted is the failure mode that matters, and verify_compile.duck sweeps this
// directory under both.

#include "../../src/compare.hpp"

namespace mc = micron;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the built-in <=> has to find OUR types by name -- this is the whole point of the port

static_assert(__is_same(decltype(1 <=> 2), std::strong_ordering));
static_assert(__is_same(decltype(1.0 <=> 2.0), std::partial_ordering));

static_assert((1 <=> 2) < 0);
static_assert((2 <=> 1) > 0);
static_assert((2 <=> 2) == 0);
static_assert(!((1 <=> 2) > 0));

// literal-zero comparison in both argument orders
static_assert(0 < (2 <=> 1));
static_assert(0 > (1 <=> 2));
static_assert(0 == (2 <=> 2));
static_assert(0 <= (2 <=> 2));
static_assert(0 >= (2 <=> 2));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// partial_ordering's unordered state answers false to ALL FOUR relations

static_assert(!(std::partial_ordering::unordered < 0));
static_assert(!(std::partial_ordering::unordered > 0));
static_assert(!(std::partial_ordering::unordered <= 0));
static_assert(!(std::partial_ordering::unordered >= 0));
static_assert(!(std::partial_ordering::unordered == 0));

static_assert(std::partial_ordering::less < 0);
static_assert(std::partial_ordering::greater > 0);
static_assert(std::partial_ordering::equivalent == 0);

// a NaN compare is where that state actually comes from. classify through a noinline volatile
// round-trip -- duck defaults to -Ofast, under which a literal NaN comparison folds (CLAUDE.md,
// the isnan/-ffinite-math-only hazard)
[[gnu::noinline]] static bool
__unordered_at_runtime(void)
{
  volatile double a = __builtin_nan("");
  volatile double b = 1.0;
  const double x = a, y = b;
  return (x <=> y) == std::partial_ordering::unordered;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// conversions run strong -> weak -> partial, never the other way

static_assert(static_cast<std::weak_ordering>(std::strong_ordering::less) < 0);
static_assert(static_cast<std::partial_ordering>(std::strong_ordering::greater) > 0);
static_assert(static_cast<std::partial_ordering>(std::weak_ordering::equivalent) == 0);
static_assert(std::strong_ordering::equal == std::strong_ordering::equivalent);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// common_comparison_category -- the weakest of the pack; the empty pack is strong_ordering

static_assert(__is_same(std::common_comparison_category_t<>, std::strong_ordering));
static_assert(__is_same(std::common_comparison_category_t<std::strong_ordering>, std::strong_ordering));
static_assert(__is_same(std::common_comparison_category_t<std::strong_ordering, std::weak_ordering>, std::weak_ordering));
static_assert(__is_same(std::common_comparison_category_t<std::strong_ordering, std::partial_ordering>, std::partial_ordering));
static_assert(__is_same(std::common_comparison_category_t<std::weak_ordering, std::partial_ordering>, std::partial_ordering));
static_assert(
    __is_same(std::common_comparison_category_t<std::strong_ordering, std::weak_ordering, std::partial_ordering>, std::partial_ordering));
// anything that is not a comparison category poisons the whole pack to void
static_assert(__is_same(std::common_comparison_category_t<std::strong_ordering, int>, void));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a DEFAULTED <=> over more than one member -- this is the shape at __special/meta:474, which was
// implicitly deleted before this header existed (declaring it was fine; using it was a hard error)

struct member_offset {
  usize bytes;
  usize bits;

  auto operator<=>(const member_offset &) const = default;
};

static_assert(__is_same(decltype(member_offset{} <=> member_offset{}), std::strong_ordering));
static_assert(member_offset{ 1, 2 } < member_offset{ 1, 3 });
static_assert(member_offset{ 1, 2 } == member_offset{ 1, 2 });
static_assert(member_offset{ 2, 0 } > member_offset{ 1, 9 });
// the synthesised relations come for free off the defaulted <=>
static_assert(member_offset{ 1, 2 } <= member_offset{ 1, 2 });
static_assert(member_offset{ 1, 3 } >= member_offset{ 1, 2 });
static_assert(member_offset{ 1, 2 } != member_offset{ 1, 3 });

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a hand-written <=> on a literal class template, the fixed_string shape

template<usize N> struct probe {
  char buf[N]{};

  constexpr probe() noexcept = default;

  constexpr probe(const char (&s)[N]) noexcept
  {
    for ( usize i = 0; i < N; ++i ) buf[i] = s[i];
  }

  template<usize M>
  constexpr std::strong_ordering
  operator<=>(const probe<M> &o) const noexcept
  {
    const usize c = (N < M ? N : M);
    for ( usize i = 0; i < c; ++i )
      if ( buf[i] != o.buf[i] )
        return static_cast<unsigned char>(buf[i]) < static_cast<unsigned char>(o.buf[i]) ? std::strong_ordering::less
                                                                                         : std::strong_ordering::greater;
    return N < M ? std::strong_ordering::less : (N > M ? std::strong_ordering::greater : std::strong_ordering::equal);
  }

  template<usize M>
  constexpr bool
  operator==(const probe<M> &o) const noexcept
  {
    return (*this <=> o) == 0;
  }
};

template<usize N> probe(const char (&)[N]) -> probe<N>;

static_assert(probe{ "abc" } == probe{ "abc" });
static_assert(probe{ "abc" } < probe{ "abd" });
static_assert(probe{ "abd" } > probe{ "abc" });
static_assert(probe{ "abc" } <= probe{ "abc" });
static_assert(probe{ "abc" } != probe{ "abd" });

// adding <=> and a member operator== must NOT cost the type its structural-ness -- it still has to
// be usable as a non-type template parameter, which is the only reason fixed_string exists
template<probe P> struct keyed {
  static constexpr auto key = P;
};

static_assert(keyed<probe{ "xy" }>::key.buf[0] == 'x');
static_assert(__is_same(decltype(keyed<probe{ "xy" }>::key), const probe<3>));

int
main()
{
  return __unordered_at_runtime() ? 1 : 0;
}
