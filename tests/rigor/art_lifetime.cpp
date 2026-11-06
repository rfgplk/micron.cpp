//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic error "-Wmaybe-uninitialized"
#endif

#include "../../src/string/strings.hpp"
#include "../../src/trees/art.hpp"

#include "../snowball/snowball.hpp"

namespace
{
struct destruction {
  usize count = 0;
  u64 sum = 0;

  void
  record(u32 id)
  {
    ++count;
    sum += id;
  }
};

destruction keys, values;

template<bool Collide> struct tracked_key {
  using pointer = const char *;
  using const_iterator = const char *;
  using iterator = const char *;

  u32 id;

  explicit tracked_key(u32 value) : id(value) { }

  ~tracked_key() { keys.record(id); }

  const char *
  data() const noexcept
  {
    return Collide ? "x" : reinterpret_cast<const char *>(&id);
  }

  const char *
  cbegin() const noexcept
  {
    return data();
  }

  const char *
  cend() const noexcept
  {
    return data() + size();
  }

  const char *
  begin() const noexcept
  {
    return data();
  }

  const char *
  end() const noexcept
  {
    return cend();
  }

  usize
  size() const noexcept
  {
    return Collide ? 1 : sizeof(id);
  }

  bool
  operator==(const tracked_key &other) const noexcept
  {
    return id == other.id;
  }
};

struct tracked_value {
  u32 id;

  ~tracked_value() { values.record(id); }
};

void
require_destroyed(destruction before_keys, destruction before_values, usize count, u64 sum)
{
  sb::require_true(keys.count - before_keys.count == count && keys.sum - before_keys.sum == sum);
  sb::require_true(values.count - before_values.count == count && values.sum - before_values.sum == sum);
}

template<bool Collide>
void
check_clear(u32 n)
{
  micron::art<tracked_key<Collide>, tracked_value> tree;
  for ( u32 i = 0; i < n; ++i ) sb::require_true(tree.insert(tracked_key<Collide>{ i }, tracked_value{ i }));
  sb::require_true(tree.size() == n);
  for ( u32 i = 0; i < n; ++i ) {
    const auto *value = tree.find(tracked_key<Collide>{ i });
    sb::require_true(value && value->id == i);
  }
  const auto before_keys = keys, before_values = values;
  tree.clear();
  require_destroyed(before_keys, before_values, n, u64(n) * (n ? n - 1 : 0) / 2);
  sb::require_true(tree.empty() && tree.blocks_allocated() == 0 && tree.pool_bytes() == 0);
  tree.clear();
  require_destroyed(before_keys, before_values, n, u64(n) * (n ? n - 1 : 0) / 2);
  sb::require_true(tree.insert(tracked_key<Collide>{ 17 }, tracked_value{ 17 }));
  sb::require_true(tree.size() == 1 && tree.find(tracked_key<Collide>{ 17 })->id == 17);
}
}      // namespace

int
main()
{
  sb::test_case("ART clear destroys every nontrivial collision-chain key and value exactly once");
  for ( const u32 n : { 0u, 1u, 2u, 7u, 16u, 65u, 257u } ) check_clear<true>(n);
  sb::end_test_case();

  sb::test_case("ART clear traverses sparse and dense inner nodes with nontrivial leaves");
  for ( const u32 n : { 1u, 4u, 16u, 48u, 256u, 4096u } ) check_clear<false>(n);
  sb::end_test_case();

  sb::test_case("ART move assignment releases the previous chain and transfers sole destruction ownership");
  {
    using tree_type = micron::art<tracked_key<true>, tracked_value>;
    tree_type source;
    destruction before_keys, before_values;
    {
      tree_type destination;
      for ( u32 i = 0; i < 33; ++i ) destination.insert(tracked_key<true>{ i }, tracked_value{ i });
      for ( u32 i = 0; i < 65; ++i ) source.insert(tracked_key<true>{ i }, tracked_value{ i });
      before_keys = keys;
      before_values = values;
      destination = micron::move(source);
      require_destroyed(before_keys, before_values, 33, 33 * 32 / 2);
      sb::require_true(source.empty() && destination.size() == 65);
      source.clear();
      require_destroyed(before_keys, before_values, 33, 33 * 32 / 2);
    }
    require_destroyed(before_keys, before_values, 33 + 65, 33 * 32 / 2 + 65 * 64 / 2);
  }
  sb::end_test_case();

  sb::test_case("ART string-key destruction preserves all mapped-value destructors");
  {
    destruction before;
    {
      micron::art<micron::string, tracked_value> tree;
      for ( u32 i = 0; i < 1000; ++i ) {
        micron::string key(sizeof(i), '\0');
        for ( usize byte = 0; byte < sizeof(i); ++byte ) key[byte] = static_cast<char>(i >> (byte * 8));
        sb::require_true(tree.insert(key, tracked_value{ i }));
      }
      before = values;
    }
    sb::require_true(values.count - before.count == 1000 && values.sum - before.sum == 999 * 1000 / 2);
  }
  sb::end_test_case();
  return 1;
}
