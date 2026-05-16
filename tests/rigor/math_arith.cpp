// math_arith.cpp
// Rigorous snowball test suite for math::arith — overflow-aware
// integer arithmetic.  Covers: checked, saturating, wrapping, widening,
// carrying.

#include "../../src/math/arith.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math::arith;

int
main()
{
  print("=== MATH::ARITH TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // checked::add
  test_case("checked::add — non-overflowing");
  {
    auto r = checked::add<i32>(2, 3);
    require_true(static_cast<bool>(r));
    require(r.value, 5);
    require_false(r.overflow);
  }
  end_test_case();

  test_case("checked::add — overflow on i32 max");
  {
    auto r = checked::add<i32>(numeric_limits<i32>::max(), 1);
    require_true(r.overflow);
    require_false(static_cast<bool>(r));
  }
  end_test_case();

  test_case("checked::add — unsigned overflow");
  {
    auto r = checked::add<u8>(255, 1);
    require_true(r.overflow);
    auto r2 = checked::add<u32>(0xFFFFFFFFu, 1);
    require_true(r2.overflow);
  }
  end_test_case();

  test_case("checked::sub — borrow on unsigned");
  {
    auto r = checked::sub<u32>(0u, 1u);
    require_true(r.overflow);
  }
  end_test_case();

  test_case("checked::mul — multiplication overflow");
  {
    auto r = checked::mul<i32>(numeric_limits<i32>::max(), 2);
    require_true(r.overflow);
    auto r2 = checked::mul<i32>(2, 3);
    require_false(r2.overflow);
    require(r2.value, 6);
  }
  end_test_case();

  test_case("checked::neg — i32 min cannot negate");
  {
    auto r = checked::neg<i32>(numeric_limits<i32>::min());
    require_true(r.overflow);
    auto r2 = checked::neg<i32>(5);
    require_false(r2.overflow);
    require(r2.value, -5);
  }
  end_test_case();

  test_case("checked::div — i32 min / -1 detected");
  {
    auto r = checked::div<i32>(numeric_limits<i32>::min(), -1);
    require_true(r.overflow);
    auto r2 = checked::div<i32>(10, 2);
    require_false(r2.overflow);
    require(r2.value, 5);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // saturating
  test_case("saturating::add — clamps to i32 max");
  {
    require(saturating::add<i32>(numeric_limits<i32>::max(), 1), numeric_limits<i32>::max());
    require(saturating::add<i32>(numeric_limits<i32>::min(), -1), numeric_limits<i32>::min());
    require(saturating::add<i32>(5, 7), 12);
  }
  end_test_case();

  test_case("saturating::sub — unsigned floor");
  {
    require(saturating::sub<u32>(0u, 1u), 0u);
    require(saturating::sub<i32>(numeric_limits<i32>::min(), 1), numeric_limits<i32>::min());
  }
  end_test_case();

  test_case("saturating::mul — clamps signs correctly");
  {
    require(saturating::mul<i32>(1 << 30, 4), numeric_limits<i32>::max());
    require(saturating::mul<i32>(-(1 << 30), 4), numeric_limits<i32>::min());
    require(saturating::mul<u32>(0xFFFF'FFFFu, 2u), numeric_limits<u32>::max());
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // wrapping
  test_case("wrapping::add — modular u8");
  {
    require(wrapping::add<u8>(200u, 100u), u8(44));      // 300 mod 256
    require(wrapping::add<u32>(0xFFFFFFFFu, 1u), 0u);
  }
  end_test_case();

  test_case("wrapping::mul — defined for signed");
  {
    require(wrapping::mul<i32>(1 << 30, 4), 0);
    require(wrapping::mul<i64>(0x100000000LL, 0x100000000LL), 0LL);
  }
  end_test_case();

  test_case("wrapping::neg — i32 min round-trips");
  {
    require(wrapping::neg<i32>(numeric_limits<i32>::min()), numeric_limits<i32>::min());
    require(wrapping::neg<i32>(7), -7);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // widening
  test_case("widening::mul — promotes to wider");
  {
    require(widening::mul<i32>(1 << 30, 4), i64(1) << 32);
    require(widening::mul<u32>(0xFFFFFFFFu, 0xFFFFFFFFu), u64(0xFFFFFFFE00000001ULL));
    require(widening::mul<i16>(1000, 1000), i32(1'000'000));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // carrying
  test_case("carrying::adc — multi-precision add chain");
  {
    auto p = carrying::adc<u64>(0xFFFFFFFFFFFFFFFFULL, 1ULL, 0ULL);
    require(get<0>(p), 0ULL);
    require(get<1>(p), 1ULL);

    auto p2 = carrying::adc<u64>(0xFFFFFFFFFFFFFFFFULL, 0ULL, 1ULL);
    require(get<0>(p2), 0ULL);
    require(get<1>(p2), 1ULL);
  }
  end_test_case();

  test_case("carrying::mul64 — schoolbook 128-bit product");
  {
    auto p = carrying::mul64(0xFFFFFFFFFFFFFFFFULL, 2ULL);
    require(get<0>(p), 0xFFFFFFFFFFFFFFFEULL);
    require(get<1>(p), 1ULL);

    auto p2 = carrying::mul64(0x100000000ULL, 0x100000000ULL);
    require(get<0>(p2), 0ULL);
    require(get<1>(p2), 1ULL);
  }
  end_test_case();

  test_case("carrying::sbb — multi-precision subtract chain");
  {
    auto p = carrying::sbb<u64>(0ULL, 1ULL, 0ULL);
    require(get<0>(p), 0xFFFFFFFFFFFFFFFFULL);
    require(get<1>(p), 1ULL);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // size contracts
  test_case("checked_result fits two registers");
  {
    static_assert(sizeof(checked_result<u64>) <= 16);
    static_assert(sizeof(checked_result<i32>) <= 8);
  }
  end_test_case();

  print("=== MATH::ARITH TESTS PASSED ===");
  return 1;
}
