//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: fast_clear() must retain trivial storage, destroy owning values,
// and keep capacity reusable across vector variants. The old reversed
// trait test wiped byte buffers and silently skipped nontrivial destructors.

#include "../../src/std.hpp"
#include "../../src/vector/convector.hpp"
#include "../../src/vector/fvector.hpp"
#include "../../src/vector/svector.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../support/tracked_types.hpp"

namespace
{

usize failures = 0;

void
verify(bool ok)
{
  if ( !ok ) ++failures;
  sb::check(ok);
}

struct move_only {
  static inline usize constructed = 0;
  static inline usize destroyed = 0;
  int value;

  explicit move_only(int v = 0) : value(v) { ++constructed; }

  move_only(const move_only &) = delete;
  move_only &operator=(const move_only &) = delete;

  move_only(move_only &&o) noexcept : value(o.value) { ++constructed; }

  move_only &
  operator=(move_only &&o) noexcept
  {
    value = o.value;
    return *this;
  }

  ~move_only() { ++destroyed; }
};

struct trivial_class {
  u32 value;

  bool operator==(const trivial_class &) const = default;
};

constexpr usize counts[] = { 0, 1, 3, 4, 7, 65 };

template<typename V>
void
verify_trivial_fast_clear(const typename V::value_type &value)
{
  static_assert(micron::is_trivially_destructible_v<typename V::value_type>);
  for ( usize count : counts ) {
    V v;
    for ( usize i = 0; i < count; ++i ) v.emplace_back(value);
    const auto *const storage = v.data();
    const usize capacity = v.max_size();
    v.fast_clear();
    verify(v.empty());
    verify(v.data() == storage && v.max_size() == capacity);
    for ( usize i = 0; i < count; ++i ) verify(storage[i] == value);
    v.fast_clear();
    v.emplace_back(value);
    verify(v.size() == usize(1) && v[0] == value);
    if ( count ) verify(v.data() == storage && v.max_size() == capacity);
  }
}

template<typename V>
void
verify_tracked_fast_clear()
{
  using tracked = typename V::value_type;
  for ( usize count : counts ) {
    tracked::reset();
    usize destroyed = 0;
    {
      V v;
      for ( usize i = 0; i < count; ++i ) v.emplace_back(static_cast<int>(i));
      verify(tracked::live() == count);
      const auto *const storage = v.data();
      const usize capacity = v.max_size();
      destroyed = tracked::dtor;
      v.fast_clear();
      verify(v.empty() && tracked::live() == usize(0));
      verify(tracked::dtor == destroyed + count);
      verify(v.data() == storage && v.max_size() == capacity);
      v.fast_clear();
      verify(tracked::dtor == destroyed + count);
      v.emplace_back(42);
      verify(v.size() == usize(1) && v[0].v == 42);
      verify(tracked::live() == usize(1));
      if ( count ) verify(v.data() == storage && v.max_size() == capacity);
    }
    verify(tracked::dtor == destroyed + count + 1 && tracked::live() == usize(0));
  }
}

}      // namespace

int
main()
{
  sb::test_case("fast_clear retains byte storage and capacity; clear still wipes");
  {
    micron::vector<u8> v(65, u8(0xa5));
    u8 *const storage = v.data();
    const usize capacity = v.max_size();
    v.fast_clear();
    verify(v.empty());
    verify(v.data() == storage && v.max_size() == capacity);
    for ( usize i = 0; i < 65; ++i ) verify(storage[i] == u8(0xa5));
    v.fast_clear();
    v.resize(65, u8(0x7c));
    verify(v.data() == storage && v.size() == usize(65));
    v.clear();
    for ( usize i = 0; i < 65; ++i ) verify(storage[i] == u8(0));
    verify(v.max_size() == capacity);
  }
  sb::end_test_case();

  sb::test_case("fast_clear does not wipe trivially destructible class values");
  {
    struct pod {
      u32 value;
    };

    micron::vector<pod> v(3, pod{ 0x12345678 });
    const pod *const storage = v.data();
    v.fast_clear();
    verify(v.empty());
    for ( usize i = 0; i < 3; ++i ) verify(storage[i].value == u32(0x12345678));
  }
  sb::end_test_case();

  sb::test_case("fast_clear destroys copyable values once before reuse");
  {
    using tracked = mtest::Tracked<87>;
    tracked::reset();
    {
      micron::vector<tracked> v;
      for ( int i = 0; i < 7; ++i ) v.emplace_back(i);
      const usize capacity = v.max_size();
      const tracked *const storage = v.data();
      v.fast_clear();
      verify(v.empty() && tracked::live() == usize(0));
      verify(tracked::dtor == usize(7));
      verify(v.data() == storage && v.max_size() == capacity);
      v.fast_clear();
      verify(tracked::dtor == usize(7));
      v.emplace_back(42);
      verify(v.size() == usize(1) && v[0].v == 42);
      verify(tracked::live() == usize(1));
    }
    verify(tracked::dtor == usize(8) && tracked::live() == usize(0));
  }
  sb::end_test_case();

  sb::test_case("fast_clear destroys move-only values once before reuse");
  {
    move_only::constructed = 0;
    move_only::destroyed = 0;
    {
      micron::vector<move_only> v;
      for ( int i = 0; i < 7; ++i ) v.emplace_back(i);
      const usize capacity = v.max_size();
      const move_only *const storage = v.data();
      v.fast_clear();
      verify(v.empty() && move_only::destroyed == usize(7));
      verify(v.data() == storage && v.max_size() == capacity);
      v.fast_clear();
      verify(move_only::destroyed == usize(7));
      v.emplace_back(42);
      verify(v.size() == usize(1) && v[0].value == 42);
    }
    verify(move_only::constructed == usize(8) && move_only::destroyed == usize(8));
  }
  sb::end_test_case();

  sb::test_case("drop remains a nonzeroing byte reset");
  {
    micron::vector<u8> v(33, u8(0x6d));
    const u8 *const storage = v.data();
    const usize capacity = v.max_size();
    v.drop();
    verify(v.empty() && v.max_size() == capacity);
    for ( usize i = 0; i < 33; ++i ) verify(storage[i] == u8(0x6d));
  }
  sb::end_test_case();

  sb::test_case("fvector fast_clear retains trivial values and destroys nontrivial values");
  verify_trivial_fast_clear<micron::fvector<u8>>(u8(0xa5));
  verify_trivial_fast_clear<micron::fvector<trivial_class>>(trivial_class{ 0x12345678 });
  verify_tracked_fast_clear<micron::fvector<mtest::Tracked<88>>>();
  sb::end_test_case();

  sb::test_case("svector fast_clear retains trivial values and destroys nontrivial values");
  verify_trivial_fast_clear<micron::svector<u8, 96>>(u8(0xa5));
  verify_trivial_fast_clear<micron::svector<trivial_class, 96>>(trivial_class{ 0x12345678 });
  verify_tracked_fast_clear<micron::svector<mtest::Tracked<89>, 96>>();
  sb::end_test_case();

  sb::test_case("convector fast_clear retains trivial values and destroys nontrivial values");
  verify_trivial_fast_clear<micron::convector<u8>>(u8(0xa5));
  verify_trivial_fast_clear<micron::convector<trivial_class>>(trivial_class{ 0x12345678 });
  verify_tracked_fast_clear<micron::convector<mtest::Tracked<90>>>();
  sb::end_test_case();

  sb::require(failures, usize(0));
  sb::print("=== VECTOR FAST_CLEAR REGRESSIONS PASSED ===");
  return 1;
}
