//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/doublelist.hpp"
#include "../../src/list.hpp"
#include "../../src/slice.hpp"
#include "../../src/span.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__ubsan_default_options()
{
  return "halt_on_error=1:exitcode=77";
}

struct tracked {
  static inline int live = 0;
  int value;

  tracked(int v = 0) : value(v) { ++live; }

  tracked(const tracked &o) : value(o.value) { ++live; }

  tracked(tracked &&o) noexcept : value(o.value) { ++live; }

  tracked &operator=(const tracked &) = default;
  tracked &operator=(tracked &&) = default;

  ~tracked() { --live; }
};

int
main()
{
  sb::test_case("zero-sized slice storage does not form element references");
  {
    micron::slice<int> sized(usize{ 0 }), filled(usize{ 0 }, 42), null(nullptr);
    const int *p = nullptr;
    micron::slice<int> range(p, p);
    for ( auto *empty : { micron::addr(sized), micron::addr(filled), micron::addr(null), micron::addr(range) } ) {
      sb::require_true(empty->size() == 0 && empty->data() == nullptr);
      const auto &constant = *empty;
      sb::require_true(empty->begin() == empty->end());
      sb::require_true(empty->addr() == nullptr && constant.addr() == nullptr && constant.data() == nullptr);
      *empty = byte{ 0 };
      empty->reset();
      empty->copy_from_slice(micron::raw_slice<int>{});
      empty->copy_within(0, 0, 0);
      auto repeated = empty->repeat(3);
      auto concatenated = empty->concat(micron::raw_slice<int>{});
      auto copied = empty->operator[]();
      auto ranged = empty->operator[](0, 0);
      sb::require_true(repeated.size() == 0 && concatenated.size() == 0 && copied.size() == 0 && ranged.size() == 0);
      auto joined = empty->join(micron::raw_slice<int>{}, 7);
      sb::require_true(joined.size() == 1 && joined[0] == 7);
    }
  }
  sb::end_test_case();

  sb::test_case("zero-sized slice lifetime operations leave no live elements");
  {
    tracked fill(7);
    {
      micron::slice<tracked> sized(usize{ 0 }), filled(usize{ 0 }, fill);
      sized.reset();
      auto repeated = filled.repeat(2);
      auto concatenated = sized.concat(micron::raw_slice<tracked>{});
      sb::require_true(tracked::live == 1);
      auto joined = sized.join(micron::raw_slice<tracked>{}, fill);
      sb::require_true(joined.size() == 1 && joined[0].value == 7 && tracked::live == 2);
    }
    sb::require_true(tracked::live == 1);
  }
  sb::require_true(tracked::live == 0);
  sb::end_test_case();

  sb::test_case("empty stack and linked containers retain empty ranges");
  {
    micron::span<int> span(micron::raw_slice<int>{});
    micron::list<int> list;
    micron::double_list<int> double_list;
    sb::require_true(span.size() == 0 && span.begin() == span.end());
    sb::require_true(list.begin() == list.end() && double_list.begin() == double_list.end());
    auto list_copy = list;
    auto double_copy = double_list;
    sb::require_true(list_copy.begin() == list_copy.end() && double_copy.begin() == double_copy.end());
  }
  sb::end_test_case();

  sb::test_case("nonempty slice copies preserve values and independent storage");
  {
    int input[] = { 1, 2, 3 };
    micron::slice<int> source(input, input + 3);
    auto copied = source.operator[]();
    source[1] = 9;
    sb::require_true(copied.size() == 3 && copied[0] == 1 && copied[1] == 2 && copied[2] == 3);
    auto concatenated = source.concat(micron::raw_slice<int>(input, 3));
    sb::require_true(concatenated.size() == 6 && concatenated[1] == 9 && concatenated[4] == 2);
  }
  sb::end_test_case();
  return 1;
}
