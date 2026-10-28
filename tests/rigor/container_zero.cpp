//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/array/array.hpp"
#include "../../src/array/carray.hpp"
#include "../../src/array/conarray.hpp"
#include "../../src/array/farray.hpp"
#include "../../src/array/iarray.hpp"
#include "../../src/vector/convector.hpp"
#include "../../src/vector/fvector.hpp"
#include "../../src/vector/ivector.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__ubsan_default_options()
{
  return "halt_on_error=1:exitcode=86";
}

struct empty_source {
  using value_type = int;
  using pointer = int *;
  using iterator = int *;
  using const_iterator = const int *;
  static inline usize reads = 0;
  static inline int dummy = 0;

  constexpr usize
  size() const
  {
    return 0;
  }

  constexpr pointer
  data()
  {
    return nullptr;
  }

  constexpr iterator
  begin()
  {
    return nullptr;
  }

  constexpr iterator
  end()
  {
    return nullptr;
  }

  constexpr const_iterator
  begin() const
  {
    return nullptr;
  }

  constexpr const_iterator
  end() const
  {
    return nullptr;
  }

  constexpr const_iterator
  cbegin() const
  {
    return nullptr;
  }

  constexpr const_iterator
  cend() const
  {
    return nullptr;
  }

  const int &
  operator[](usize) const
  {
    ++reads;
    return dummy;
  }
};

struct static_empty_source: empty_source {
  static constexpr usize static_size = 0;
};

template<class Array>
void
empty_assignment()
{
  Array target(7);
  target = empty_source{};
  sb::require_true(empty_source::reads == 0);
  for ( const auto &value : target ) sb::require_true(value == 7);
  target = static_empty_source{};
  sb::require_true(empty_source::reads == 0);
  for ( const auto &value : target ) sb::require_true(value == 7);
  micron::vector<int> values{ 11, 13 };
  target = values;
  sb::require_true(target[0] == 11 && target[1] == 13 && target[2] == 7 && target[3] == 7);
}

int
main()
{
  sb::test_case("zero-count dynamic vector variants");
  {
    micron::fvector<int> empty(usize{ 0 });
    micron::fvector<int> fill(usize{ 0 }, 7);
    micron::fvector<int> generated(usize{ 0 }, [] {
      sb::require_true(false);
      return 1;
    });
    empty.assign(0, 8);
    empty.fill(9);
    empty.resize(0);
    empty.reserve(0);
    empty.append(fill);
    sb::require_true(empty.empty() && generated.empty());
    empty.push_back(42);
    sb::require_true(empty.size() == 1 && empty[0] == 42);
  }
  {
    micron::convector<int> empty(usize{ 0 }), fill(usize{ 0 }, 7);
    empty.fill(9);
    empty.assign(0, 8);
    empty.resize(0);
    empty.reserve(0);
    empty.append(fill);
    sb::require_true(empty.empty());
    empty.push_back(42);
    sb::require_true(empty.size() == 1 && empty[0] == 42);
  }
  {
    micron::ivector<int> empty(usize{ 0 }), fill(usize{ 0 }, 7);
    micron::ivector<int> generated(usize{ 0 }, [] {
      sb::require_true(false);
      return 1;
    });
    sb::require_true(empty.append(fill).empty() && empty.assign(0, 8).empty() && generated.empty());
    const auto one = empty.push_back(42);
    sb::require_true(one.size() == 1 && one[0] == 42);
    sb::require_true(one.pop_back().empty());
    const auto front = empty.push_front(17);
    sb::require_true(front.size() == 1 && front[0] == 17);
    sb::require_true(front.erase(usize{ 0 }).empty());
  }
  sb::end_test_case();
  sb::test_case("empty fixed-array assignment never indexes its source");
  empty_assignment<micron::array<int, 4>>();
  empty_assignment<micron::carray<int, 4>>();
  empty_assignment<micron::conarray<int, 4>>();
  empty_assignment<micron::farray<int, 4>>();
  empty_assignment<micron::iarray<int, 4>>();
  sb::end_test_case();
  return 1;
}
