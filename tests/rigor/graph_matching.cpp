//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/graph.hpp"
#include "../snowball/snowball.hpp"

namespace mm = micron::math;
namespace mg = micron::math::graphs;

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

struct brute_matching {
  usize cardinality{};
  i64 weight{};
};

static u64
next_random(u64 &state)
{
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

static bool
better(const brute_matching &left, const brute_matching &right, mg::matching_objective objective)
{
  if ( objective == mg::matching_objective::maximum_cardinality_then_weight && left.cardinality != right.cardinality )
    return left.cardinality > right.cardinality;
  return left.weight > right.weight;
}

static brute_matching
brute_force(const bool *present, const i64 *weights, usize n, u32 remaining, mg::matching_objective objective)
{
  if ( remaining == 0 ) return {};
  usize u = 0;
  while ( (remaining & (u32(1) << u)) == 0 ) ++u;
  brute_matching best = brute_force(present, weights, n, remaining & ~(u32(1) << u), objective);
  for ( usize v = u + 1; v < n; ++v ) {
    if ( (remaining & (u32(1) << v)) == 0 || !present[u * n + v] ) continue;
    auto candidate = brute_force(present, weights, n, remaining & ~(u32(1) << u) & ~(u32(1) << v), objective);
    ++candidate.cardinality;
    candidate.weight += weights[u * n + v];
    if ( better(candidate, best, objective) ) best = candidate;
  }
  return best;
}

template<typename G, typename Result>
static bool
valid_matching(const G &graph, const Result &result)
{
  if ( result.status != mg::algorithm_status::ok || result.edges.size() != result.cardinality ) return false;
  usize pairs = 0;
  i64 sum = 0;
  for ( auto vertex : graph.vertices() ) {
    const auto mate = result.mate.data()[vertex.value];
    const auto edge = result.mate_edge.data()[vertex.value];
    if ( !mate.valid() ) {
      if ( edge.valid() ) return false;
      continue;
    }
    if ( !graph.has_vertex(mate) || !graph.has_edge(edge) || result.mate.data()[mate.value] != vertex
         || result.mate_edge.data()[mate.value] != edge )
      return false;
    const auto source = graph.source(edge);
    const auto target = graph.target(edge);
    if ( !((source == vertex && target == mate) || (source == mate && target == vertex)) ) return false;
    if ( vertex < mate ) {
      ++pairs;
      sum += graph.edge_property_unchecked(edge).weight;
    }
  }
  return pairs == result.cardinality && sum == result.total_weight;
}

int
main()
{
  test_case("weighted blossom matches exhaustive enumeration");
  {
    constexpr usize trials = 512;
    u64 random = 0x7e51'bc93'd648'2a0fULL;
    for ( usize trial = 0; trial < trials; ++trial ) {
      const usize n = 2 + next_random(random) % 9;
      using graph_type
          = mm::graph<mm::empty_property, mm::weighted_property<i64>, mm::empty_property, u32, mg::undirected_t, mg::parallel_t>;
      graph_type graph;
      (void)graph.add_vertices(n);
      bool present[100]{};
      i64 weights[100]{};
      for ( usize u = 0; u < n; ++u )
        for ( usize v = u + 1; v < n; ++v ) {
          if ( next_random(random) % 100 >= 43 ) continue;
          const i64 weight = static_cast<i64>(next_random(random) % 31) - 12;
          (void)graph.add_edge(static_cast<u32>(u), static_cast<u32>(v), weight);
          present[u * n + v] = present[v * n + u] = true;
          weights[u * n + v] = weights[v * n + u] = weight;
          if ( next_random(random) % 5 == 0 ) {
            const i64 parallel = static_cast<i64>(next_random(random) % 31) - 12;
            (void)graph.add_edge(static_cast<u32>(u), static_cast<u32>(v), parallel);
            if ( weights[u * n + v] < parallel ) weights[u * n + v] = weights[v * n + u] = parallel;
          }
        }

      const u32 all = (u32(1) << n) - 1;
      const auto expected_weight = brute_force(present, weights, n, all, mg::matching_objective::maximum_weight);
      const auto actual_weight = mg::maximum_weighted_matching(graph);
      require_true(valid_matching(graph, actual_weight));
      require_true(actual_weight.total_weight == expected_weight.weight);

      const auto expected_cardinality = brute_force(present, weights, n, all, mg::matching_objective::maximum_cardinality_then_weight);
      const auto actual_cardinality = mg::maximum_weighted_matching(graph, mg::matching_objective::maximum_cardinality_then_weight);
      require_true(valid_matching(graph, actual_cardinality));
      require_true(actual_cardinality.cardinality == expected_cardinality.cardinality);
      require_true(actual_cardinality.total_weight == expected_cardinality.weight);
    }
  }
  end_test_case();

  test_case("weighted blossom preserves the selected parallel descriptor");
  {
    using graph_type = mm::graph<mm::empty_property, mm::weighted_property<i64>, mm::empty_property, u32, mg::undirected_t, mg::parallel_t>;
    graph_type graph;
    const auto first = graph.add_edge(0u, 1u, i64(7)).id;
    (void)graph.add_edge(0u, 1u, i64(3));
    const auto equal = graph.add_edge(0u, 1u, i64(7)).id;
    const auto result = mg::maximum_weighted_matching(graph);
    require_true(result.status == mg::algorithm_status::ok && result.cardinality == 1 && result.total_weight == 7);
    require_true(result.edges.data()[0] == first && result.edges.data()[0] != equal);
  }
  end_test_case();

  test_case("unit-weight convenience and workspace support unweighted graphs");
  {
    mm::graph<> graph;
    (void)graph.add_edge(0, 1);
    (void)graph.add_edge(1, 2);
    (void)graph.add_edge(2, 3);
    mg::weighted_matching_workspace<u32, i32> workspace;
    const auto result = mg::maximum_cardinality_matching(graph, workspace);
    require_true(result.status == mg::algorithm_status::ok && result.cardinality == 2 && result.perfect());

    using loop_graph = mm::graph<mm::empty_property, mm::weighted_property<f64>, mm::empty_property, u32, mg::undirected_t, mg::simple_t,
                                 mg::allow_loops_t>;
    loop_graph invalid;
    volatile u64 infinity_bits = 0x7ff0000000000000ull;
    const auto inserted = invalid.add_edge(0, 0, micron::math::ieee::from_bits<f64>(infinity_bits));
    require_true(inserted.inserted());
    require_true(!micron::math::ieee::is_finite(invalid.edge_weight(inserted.id)));
    const auto invalid_result = mg::maximum_weighted_matching(invalid);
    require_true(invalid_result.status == mg::algorithm_status::invalid_weight);
  }
  end_test_case();

  return 1;
}
