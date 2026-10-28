//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/vector/reserved_vector.hpp"
#include "../../src/vector/segmented_vector.hpp"
#include "../snowball/snowball.hpp"

struct alignas(64) record {
  u64 value = 42, other = 17;
};

template<class Container>
void
verify(Container &values)
{
  sb::require_true(values.try_resize(1));
  record *first = &values[0];
  sb::require_true(first->value == 42 && first->other == 17);
  u64 random = 0x5120041;
  usize size = 1;
  for ( u32 iteration = 0; iteration < 10000; ++iteration ) {
    random ^= random << 13;
    random ^= random >> 7;
    random ^= random << 17;
    if ( size < 65536 ) {
      const usize next = micron::min(size + (random & 31) + 1, usize{ 65536 });
      sb::require_true(values.try_resize(next));
      for ( usize i = size; i < next; ++i ) {
        sb::require_true(values[i].value == 42 && values[i].other == 17);
        values[i].value = i * 17;
      }
      size = next;
    }
    const usize index = 1 + random % (size - 1);
    sb::require_true(values[index].value == index * 17);
    sb::require_true(&values[0] == first && first->value == 42);
    sb::require_true((reinterpret_cast<uintptr_t>(&values[index]) & 63) == 0);
  }
  values.clear();
  sb::require_true(values.size() == 0 && values.try_resize(3));
  sb::require_true(values[1].value == 42 && &values[0] == first);
}

struct counted {
  static inline int live = 0, attempts = 0, fail_at = -1;
  u64 value = 17;

  counted()
  {
    if ( attempts++ == fail_at ) throw 7;
    ++live;
  }

  ~counted() { --live; }
};

int
main()
{
  {
    micron::reserved_vector<counted> values;
    sb::require_true(values.initialize(32) && values.try_resize(3));
    counted *first = values.data();
    counted::fail_at = 5;
    sb::require_true(!values.try_resize(8) && values.size() == 3 && counted::live == 3);
    counted::fail_at = -1;
    sb::require_true(values.try_resize(8) && values.data() == first && counted::live == 8);
    values[0].value = 42;
    micron::reserved_vector<counted> copy(values);
    sb::require_true(copy.data() != first && copy[0].value == 42 && counted::live == 16);
    micron::reserved_vector<counted> moved(static_cast<micron::reserved_vector<counted> &&>(copy));
    sb::require_true(!copy.data() && moved[0].value == 42);
    values = moved;
    sb::require_true(values.data() == first && values.size() == 8 && counted::live == 16);
    sb::require_true(values.try_resize(2) && counted::live == 10);
    values.clear();
    sb::require_true(counted::live == 8);
  }
  sb::require_true(counted::live == 0);
  counted::attempts = 0;
  {
    micron::segmented_vector<counted, 4> values;
    sb::require_true(values.try_resize(3));
    counted *first = &values[0];
    counted::fail_at = 5;
    sb::require_true(!values.try_resize(9) && values.size() == 3 && counted::live == 3);
    counted::fail_at = -1;
    sb::require_true(values.try_resize(9) && counted::live == 9 && &values[0] == first);
    sb::require_true(values.try_resize(1) && counted::live == 1);
    values.release();
    sb::require_true(counted::live == 0 && values.try_resize(2));
  }
  sb::require_true(counted::live == 0);
  micron::reserved_vector<record> contiguous;
  sb::require_true(!contiguous.initialize(~usize{ 0 }));
  sb::require_true(contiguous.initialize(65536));
  verify(contiguous);
  sb::require_true(!contiguous.try_resize(65537));
  sb::require_true(contiguous.size() == 3 && contiguous[0].value == 42);
  contiguous.release();
  sb::require_true(contiguous.initialize(4) && contiguous.try_resize(4));
  sb::require_true(contiguous[3].value == 42);
  micron::segmented_vector<record, 64> segmented;
  verify(segmented);
  sb::require_true(!segmented.try_resize(~usize{ 0 }));
  sb::require_true(segmented.size() == 3);
  return 1;
}
