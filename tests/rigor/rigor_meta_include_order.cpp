// rigor_meta_include_order.cpp -- micron's <meta> port must survive micron's own headers.
//
// __special/meta declares std::allocator, std::vector and std::ranges::* itself, because gcc's
// reflection implementation hardcodes lookups of those names and micron ships no libstdc++ to get
// them from. It stands down and defers to the real <meta> when a libstdc++ header has already
// declared them -- that part is deliberate and documented.
//
// The detection used to be `#if defined(_GLIBCXX_RELEASE)`, i.e. "any libstdc++ header at all".
// That became wrong the moment micron itself started reaching one: string/fixed_string.hpp gained
// `#include "../compare.hpp"`, and compare.hpp in a HOSTED build defers to the real <compare>,
// which defines _GLIBCXX_RELEASE by way of <bits/c++config.h>. Since fixed_string is pulled by
// string_view.hpp -> strings.hpp -> std.hpp, EVERY hosted TU that touched the string layer before
// reflect.hpp silently retired the self-hosted branch -- the one tests/rigor/meta.cpp exists to
// cover -- even though <compare> declares none of the names in question and cannot collide.
//
// THIS FILE IS THE ORDERING THAT BROKE IT. strings.hpp first, reflect.hpp second. If the
// detection ever widens again, meta::self_hosted flips and this fails.
//
// It must be built with -freflection; tests/reflect/reflect.duck carries it.

#include "../../src/string/strings.hpp"      // <- deliberately FIRST. this is the whole point.

#include "../../src/compare.hpp"
#include "../../src/reflect.hpp"

#include "../support/oracles.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

struct point {
  int x;
  int y;
};

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("micron's <meta> is still self-hosted after the string layer");
  {
    // the assertion this file exists for. before the fix this was false, and every reflection TU
    // that included std.hpp was silently exercising libstdc++'s <meta> instead of micron's.
    static_assert(micron::meta::self_hosted, "strings.hpp must not retire the self-hosted <meta>");
    require_true(micron::meta::self_hosted);

    // <compare> is the one __special/ port that deliberately does NOT self-host when hosted, so
    // this is expected to be false here -- it is the asymmetry that caused the problem, and
    // pinning it keeps the two facts from being conflated later
    require_true(!micron::compare::self_hosted);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("and the self-hosted branch actually works in this TU");
  {
    // not just the flag: the port has to still function with the string layer present, which is
    // what proves the two are really coexisting rather than one having been quietly dropped
    constexpr usize n = micron::reflect::field_count<point>;
    static_assert(n == 2);
    require_true(n == 2);

    point p{ 3, 4 };
    int sum = 0;
    usize named = 0;
    micron::reflect::for_each_field(p, [&](auto name, auto &f) {
      sum += static_cast<int>(f);
      named += name.len();
    });
    require_true(sum == 7);
    require_true(named == 2);      // "x" + "y"

    // and the string layer is fully usable alongside it
    micron::fixed_string<8> fs("ok", 2);
    require_true(fs.len() == 2);
    require_true(micron::format::format("{}", fs).size() == 2);
    require_true(micron::to_fixed(1.5, 2).size() == 4);
  }
  end_test_case();

  print("=== META INCLUDE ORDER SUITE PASSED ===");
  return 1;
}
