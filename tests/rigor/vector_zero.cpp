//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/graph/connectivity.hpp"
#include "../../src/vector/vector.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1:print_stacktrace=1";
}

struct copy_construct_only {
  static inline usize live = 0;
  u32 value;

  explicit copy_construct_only(u32 v = 0) : value(v) { ++live; }

  copy_construct_only(const copy_construct_only &o) noexcept : value(o.value) { ++live; }

  copy_construct_only(copy_construct_only &&o) noexcept : value(o.value) { ++live; }

  copy_construct_only &operator=(const copy_construct_only &) = delete;

  copy_construct_only &
  operator=(copy_construct_only &&o) noexcept
  {
    value = o.value;
    return *this;
  }

  ~copy_construct_only() { --live; }
};

static_assert(micron::movable<copy_construct_only> && !micron::copyable<copy_construct_only>);

template<typename T, bool Safe>
void
scalars()
{
  using V = micron::vector<T, micron::allocator_serial<>, Safe>;
  V empty_list(std::initializer_list<T>{});
  sb::require_true(empty_list.empty());
  for ( usize n : { usize(0), usize(1), usize(2), usize(7), usize(64), usize(0) } ) {
    V zeroed(n), filled(n, T(37)), copied(filled), assigned(0);
    assigned = copied;
    sb::require(zeroed.size(), n);
    sb::require(assigned.size(), n);
    for ( usize i = 0; i < n; ++i ) {
      sb::require(zeroed[i], T(0));
      sb::require(assigned[i], T(37));
    }
    usize generated_count = 0, transformed_count = 0;
    V generated(n, [&]() {
      ++generated_count;
      return T(11);
    });
    V transformed(n, [&](T v) {
      ++transformed_count;
      return T(v + 19);
    });
    sb::require(generated_count, n);
    sb::require(transformed_count, n);
    for ( usize i = 0; i < n; ++i ) {
      sb::require(generated[i], T(11));
      sb::require(transformed[i], T(19));
    }
    V empty(0), moved(micron::move(copied));
    assigned.assign(0, T(1));
    assigned.append(empty).weld(micron::move(copied));
    assigned.clear();
    assigned.drop();
    assigned.fill(T(3));
    sb::require_true(assigned.empty());
    if ( n == 0 ) sb::require_true(zeroed.data() == nullptr && moved.data() == nullptr);
  }
}

int
main()
{
  sb::test_case("empty and nonempty vector constructors, copies and callbacks");
  scalars<u8, true>();
  scalars<u32, false>();
  sb::end_test_case();

  sb::test_case("the move-only vector specialization preserves element lifetimes");
  for ( usize n : { usize(0), usize(1), usize(7), usize(0) } ) {
    {
      const copy_construct_only value(42);
      micron::vector<copy_construct_only> zeroed(n), filled(n, value);
      sb::require(copy_construct_only::live, 2 * n + 1);
      for ( usize i = 0; i < n; ++i ) {
        sb::require(zeroed[i].value, u32(0));
        sb::require(filled[i].value, u32(42));
      }
      zeroed.clear();
      filled.drop();
      sb::require(copy_construct_only::live, usize(1));
    }
    sb::require(copy_construct_only::live, usize(0));
  }
  sb::end_test_case();

  sb::test_case("zero-count container helpers accept null storage");
  u32 *empty = nullptr;
  u32 *source_empty = nullptr;
  const u32 *const_empty = nullptr;
  micron::__impl_container::construct(empty, u32(1), 0);
  micron::__impl_container::construct<0>(empty, u32(1));
  micron::__impl_container::copy(empty, const_empty, 0);
  micron::__impl_container::copy<0>(empty, const_empty);
  micron::__impl_container::move(empty, source_empty, 0);
  micron::__impl_container::move<0>(empty, source_empty);
  micron::__impl_container::destroy(empty, 0);
  micron::__impl_container::destroy<0>(empty);
  micron::__impl_container::set(empty, u32(1), 0);
  micron::__impl_container::set<0>(empty, u32(1));
  micron::__impl_container::zero<0>(empty);
  sb::end_test_case();

  sb::test_case("SCC scratch vectors handle empty graphs, chains and cycles");
  namespace graphs = micron::math::graphs;
  using G = micron::math::graph<micron::math::empty_property, micron::math::empty_property, micron::math::empty_property, u32,
                                graphs::directed_t, graphs::simple_t, graphs::allow_loops_t>;
  for ( u32 n = 0; n < 65; ++n ) {
    G graph;
    for ( u32 i = 0; i < n; ++i ) (void)graph.add_vertex();
    for ( u32 i = 1; i < n; ++i ) (void)graph.add_edge(i - 1, i);
    const auto chain = graphs::strongly_connected_components(graph);
    sb::require(chain.count, usize(n));
    sb::require(chain.component.size(), usize(n));
    if ( n == 0 ) continue;
    for ( u32 i = 1; i < n; ++i ) sb::require_true(chain.component[i - 1] != chain.component[i]);
    (void)graph.add_edge(n - 1, 0);
    const auto cycle = graphs::strongly_connected_components(graph);
    sb::require(cycle.count, usize(1));
    for ( u32 i = 1; i < n; ++i ) sb::require(cycle.component[i], cycle.component[0]);
  }
  sb::end_test_case();
  return 1;
}
