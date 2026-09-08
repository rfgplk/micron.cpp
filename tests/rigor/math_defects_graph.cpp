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

static u64
next_random(u64 &state) noexcept
{
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exact treewidth by exhaustive elimination -- the oracle the heuristic must not undercut

static constexpr unsigned __oracle_max = 8;

struct treewidth_oracle {
  unsigned order{};
  unsigned best{};

  void
  descend(const unsigned *adjacency, unsigned remaining, unsigned width) noexcept
  {
    if ( width >= best ) return;
    if ( remaining == 0 ) {
      best = width;
      return;
    }
    for ( unsigned v = 0; v < order; ++v ) {
      if ( !(remaining & (1u << v)) ) continue;
      const unsigned clique = adjacency[v] & remaining & ~(1u << v);
      const unsigned degree = static_cast<unsigned>(micron::bitcount(clique));
      const unsigned next_width = degree > width ? degree : width;
      if ( next_width >= best ) continue;
      unsigned next[__oracle_max];
      for ( unsigned i = 0; i < order; ++i ) next[i] = adjacency[i];
      for ( unsigned a = 0; a < order; ++a )
        if ( clique & (1u << a) ) next[a] |= clique & ~(1u << a);
      descend(next, remaining & ~(1u << v), next_width);
    }
  }

  [[nodiscard]] unsigned
  operator()(unsigned n, const unsigned *adjacency) noexcept
  {
    order = n;
    best = n;
    descend(adjacency, (1u << n) - 1u, 0);
    return best;
  }
};

template<typename Range>
static void
collect_sorted(Range range, micron::vector<u32> &out)
{
  out.clear();
  for ( auto neighbor : range ) out.push_back(static_cast<u32>(neighbor.value));
  for ( usize i = 1; i < out.size(); ++i )
    for ( usize j = i; j > 0 && out[j] < out[j - 1]; --j ) micron::swap(out[j], out[j - 1]);
}

int
main()
{
  test_case("dijkstra refuses a negative floating-point weight exactly as it refuses a negative integer one");
  {
    volatile f64 opaque = -2.0;
    const f64 laundered = opaque;
    require_true(mg::__impl::invalid_weight(laundered));
    require_true(mg::__impl::invalid_weight(-2.0));
    require_true(mg::__impl::invalid_weight(static_cast<i64>(-2)));
    require_true(!mg::__impl::invalid_weight(2.0) && !mg::__impl::invalid_weight(0.0));
    require_true(!mg::__impl::invalid_weight(static_cast<i64>(2)) && !mg::__impl::invalid_weight(static_cast<u32>(2)));

    mm::weighted_digraph<f64> floating;
    (void)floating.add_vertices(4);
    (void)floating.add_edge(0, 1, 1.0);
    (void)floating.add_edge(0, 2, 2.0);
    (void)floating.add_edge(2, 1, laundered);
    (void)floating.add_edge(1, 3, 1.0);
    require_true(mg::dijkstra(floating, mm::vertex_id<u32>(0)).status == mg::algorithm_status::invalid_weight);
    const auto reference = mg::bellman_ford(floating, mm::vertex_id<u32>(0));
    require_true(reference.status == mg::algorithm_status::ok && reference.distance.data()[3] == 1.0);

    mm::weighted_digraph<i64> integral;
    (void)integral.add_vertices(4);
    (void)integral.add_edge(0, 1, i64(1));
    (void)integral.add_edge(0, 2, i64(2));
    (void)integral.add_edge(2, 1, i64(-2));
    (void)integral.add_edge(1, 3, i64(1));
    require_true(mg::dijkstra(integral, mm::vertex_id<u32>(0)).status == mg::algorithm_status::invalid_weight);

    mm::weighted_digraph<f64> positive;
    (void)positive.add_vertices(4);
    (void)positive.add_edge(0, 1, 1.0);
    (void)positive.add_edge(0, 2, 2.0);
    (void)positive.add_edge(2, 1, 0.5);
    (void)positive.add_edge(1, 3, 1.0);
    const auto ok = mg::dijkstra(positive, mm::vertex_id<u32>(0));
    require_true(ok.status == mg::algorithm_status::ok && ok.distance.data()[3] == 2.0);
  }
  end_test_case();

  test_case("filtered_graph_view::in_neighbors yields predecessors, not the queried vertex");
  {
    using digraph
        = mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::directed_t, mg::simple_t, mg::allow_loops_t>;
    digraph fixed_case;
    (void)fixed_case.add_vertices(3);
    (void)fixed_case.add_edge(0, 1);
    (void)fixed_case.add_edge(2, 1);
    auto view = mg::filtered(fixed_case, mg::accept_all{}, mg::accept_all{});
    micron::vector<u32> expected, actual;
    collect_sorted(fixed_case.in_neighbors(mm::vertex_id<u32>(1)), expected);
    collect_sorted(view.in_neighbors(mm::vertex_id<u32>(1)), actual);
    require_true(expected.size() == 2 && expected[0] == 0 && expected[1] == 2);
    require_true(actual.size() == expected.size() && actual[0] == expected[0] && actual[1] == expected[1]);

    u64 seed = 0x9E3779B97F4A7C15ull;
    for ( int trial = 0; trial < 96; ++trial ) {
      digraph graph;
      const u32 order = 3 + static_cast<u32>(next_random(seed) % 6);
      (void)graph.add_vertices(order);
      const u32 edges = static_cast<u32>(next_random(seed) % (order * order));
      for ( u32 e = 0; e < edges; ++e )
        (void)graph.add_edge(static_cast<u32>(next_random(seed) % order), static_cast<u32>(next_random(seed) % order));
      auto filtered = mg::filtered(graph, mg::accept_all{}, mg::accept_all{});
      for ( u32 v = 0; v < order; ++v ) {
        collect_sorted(graph.in_neighbors(mm::vertex_id<u32>(v)), expected);
        collect_sorted(filtered.in_neighbors(mm::vertex_id<u32>(v)), actual);
        require_true(expected.size() == actual.size());
        for ( usize i = 0; i < expected.size(); ++i ) require_true(expected[i] == actual[i]);
        collect_sorted(graph.out_neighbors(mm::vertex_id<u32>(v)), expected);
        collect_sorted(filtered.out_neighbors(mm::vertex_id<u32>(v)), actual);
        require_true(expected.size() == actual.size());
        for ( usize i = 0; i < expected.size(); ++i ) require_true(expected[i] == actual[i]);
      }
    }
  }
  end_test_case();

  test_case("treewidth_min_degree_upper_bound never lands below the treewidth");
  {
    for ( u32 side = 2; side <= 5; ++side ) {
      mm::graph<> grid;
      (void)grid.add_vertices(side * side);
      for ( u32 row = 0; row < side; ++row )
        for ( u32 column = 0; column < side; ++column ) {
          const u32 id = row * side + column;
          if ( column + 1 < side ) (void)grid.add_edge(id, id + 1);
          if ( row + 1 < side ) (void)grid.add_edge(id, id + side);
        }
      require_true(mg::treewidth_min_degree_upper_bound(grid) >= side);
    }

    mm::graph<> path;
    (void)path.add_vertices(6);
    for ( u32 i = 0; i + 1 < 6; ++i ) (void)path.add_edge(i, i + 1);
    require_true(mg::treewidth_min_degree_upper_bound(path) == 1);

    treewidth_oracle oracle;
    u64 seed = 0x0FEEDFACECAFEB0Dull;
    for ( int trial = 0; trial < 160; ++trial ) {
      const unsigned order = 3 + static_cast<unsigned>(next_random(seed) % 5);
      unsigned adjacency[__oracle_max] = {};
      mm::graph<> graph;
      (void)graph.add_vertices(order);
      for ( unsigned a = 0; a < order; ++a )
        for ( unsigned b = a + 1; b < order; ++b )
          if ( next_random(seed) % 100 < 45 ) {
            (void)graph.add_edge(a, b);
            adjacency[a] |= 1u << b;
            adjacency[b] |= 1u << a;
          }
      require_true(mg::treewidth_min_degree_upper_bound(graph) >= oracle(order, adjacency));
    }
  }
  end_test_case();

  test_case("fixed_graph counts an undirected self-loop twice, as every other storage does");
  {
    using packed_type
        = mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t, mg::simple_t, mg::allow_loops_t>;
    using fixed_type = mm::fixed_graph<8, 8, mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t,
                                       mg::simple_t, mg::allow_loops_t>;
    packed_type packed;
    (void)packed.add_vertices(2);
    (void)packed.add_edge(0, 1);
    (void)packed.add_edge(0, 0);
    fixed_type fixed;
    for ( int i = 0; i < 2; ++i ) (void)fixed.add_vertex();
    (void)fixed.add_edge(0, 1);
    (void)fixed.add_edge(0, 0);

    const mm::vertex_id<u32> loop_vertex(0);
    usize packed_edges = 0, fixed_edges = 0;
    for ( auto edge : packed.out_edges(loop_vertex) ) {
      (void)edge;
      ++packed_edges;
    }
    for ( auto edge : fixed.out_edges(loop_vertex) ) {
      (void)edge;
      ++fixed_edges;
    }
    micron::vector<u32> expected, actual;
    collect_sorted(packed.out_neighbors(loop_vertex), expected);
    collect_sorted(fixed.out_neighbors(loop_vertex), actual);
    require_true(packed.degree(loop_vertex) == 3 && fixed.degree(loop_vertex) == packed.degree(loop_vertex));
    require_true(packed.out_degree(loop_vertex) == 3 && fixed.out_degree(loop_vertex) == packed.out_degree(loop_vertex));
    require_true(packed_edges == 3 && fixed_edges == packed_edges);
    require_true(expected.size() == 3 && actual.size() == expected.size());
    for ( usize i = 0; i < expected.size(); ++i ) require_true(expected[i] == actual[i]);
    require_true(fixed.degree(mm::vertex_id<u32>(1)) == 1);

    packed_type packed_circuit;
    (void)packed_circuit.add_vertices(3);
    fixed_type fixed_circuit;
    for ( int i = 0; i < 3; ++i ) (void)fixed_circuit.add_vertex();
    const u32 triangle[][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 0 } };
    for ( const auto &edge : triangle ) {
      (void)packed_circuit.add_edge(edge[0], edge[1]);
      (void)fixed_circuit.add_edge(edge[0], edge[1]);
    }
    const auto packed_tour = mg::eulerian_path(packed_circuit);
    const auto fixed_tour = mg::eulerian_path(fixed_circuit);
    require_true(packed_tour.status == mg::algorithm_status::ok && packed_tour.circuit && packed_tour.edges.size() == 4);
    require_true(fixed_tour.status == packed_tour.status && fixed_tour.circuit == packed_tour.circuit);
    require_true(fixed_tour.edges.size() == packed_tour.edges.size());
  }
  end_test_case();

  test_case("a negative-cost self-loop is a negative cycle min_cost_max_flow cancels once");
  {
    using network = mm::graph<mm::empty_property, mm::capacity_cost_property<i32, i32>, mm::empty_property, u32, mg::directed_t,
                              mg::simple_t, mg::allow_loops_t>;
    network looped;
    (void)looped.add_vertices(3);
    (void)looped.add_edge(0, 1, mm::capacity_cost_property<i32, i32>(3, 1));
    (void)looped.add_edge(1, 2, mm::capacity_cost_property<i32, i32>(3, 1));
    (void)looped.add_edge(1, 1, mm::capacity_cost_property<i32, i32>(2, -5));
    const auto looped_result = mg::min_cost_max_flow(looped, mm::vertex_id<u32>(0), mm::vertex_id<u32>(2));

    network expanded;
    (void)expanded.add_vertices(4);
    (void)expanded.add_edge(0, 1, mm::capacity_cost_property<i32, i32>(3, 1));
    (void)expanded.add_edge(1, 2, mm::capacity_cost_property<i32, i32>(3, 1));
    (void)expanded.add_edge(1, 3, mm::capacity_cost_property<i32, i32>(2, -5));
    (void)expanded.add_edge(3, 1, mm::capacity_cost_property<i32, i32>(2, 0));
    const auto expanded_result = mg::min_cost_max_flow(expanded, mm::vertex_id<u32>(0), mm::vertex_id<u32>(2));

    require_true(looped_result.status == mg::algorithm_status::ok);
    require_true(expanded_result.status == mg::algorithm_status::ok);
    require_true(looped_result.achieved_flow == expanded_result.achieved_flow);
    require_true(looped_result.total_cost == expanded_result.total_cost && looped_result.total_cost == -4);
    require_true(mg::verify_min_cost_flow(looped, mm::vertex_id<u32>(0), mm::vertex_id<u32>(2), looped_result));

    micron::vector<i32> balanced{ 0, 0, 0 };
    const auto circulation = mg::min_cost_circulation(looped, balanced);
    require_true(circulation.status == mg::algorithm_status::ok && circulation.total_cost == -10);
    require_true(mg::verify_min_cost_flow(looped, balanced, circulation));

    network positive;
    (void)positive.add_vertices(3);
    (void)positive.add_edge(0, 1, mm::capacity_cost_property<i32, i32>(3, 1));
    (void)positive.add_edge(1, 2, mm::capacity_cost_property<i32, i32>(3, 1));
    (void)positive.add_edge(1, 1, mm::capacity_cost_property<i32, i32>(2, 5));
    const auto untouched = mg::min_cost_max_flow(positive, mm::vertex_id<u32>(0), mm::vertex_id<u32>(2));
    require_true(untouched.status == mg::algorithm_status::ok && untouched.total_cost == 6);
  }
  end_test_case();

  // the 64-bit capacity/supply instantiation below is the i386 and armv7-a gate: the range checks in
  // build_cost_residual and supply_capacity used to route a `long long` through uint128_t, whose 32-bit
  // software form has (u64), (u32) and (int) constructors and no unambiguous winner.
  test_case("min-cost flow instantiates for 64-bit capacities on a 32-bit target");
  {
    using wide_network = mm::graph<mm::empty_property, mm::capacity_cost_property<i64, i64>, mm::empty_property, u32, mg::directed_t,
                                   mg::simple_t, mg::allow_loops_t>;
    wide_network graph;
    (void)graph.add_vertices(3);
    (void)graph.add_edge(0, 1, mm::capacity_cost_property<i64, i64>(3, 1));
    (void)graph.add_edge(1, 2, mm::capacity_cost_property<i64, i64>(3, 1));
    (void)graph.add_edge(1, 1, mm::capacity_cost_property<i64, i64>(2, -5));
    const auto result = mg::min_cost_max_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(2));
    require_true(result.status == mg::algorithm_status::ok && result.achieved_flow == 3 && result.total_cost == -4);
    micron::vector<i64> supplies{ 2, 0, -2 };
    const auto circulation = mg::min_cost_circulation(graph, supplies);
    require_true(circulation.status == mg::algorithm_status::ok && circulation.total_cost == -6);
  }
  end_test_case();

  return 1;
}
