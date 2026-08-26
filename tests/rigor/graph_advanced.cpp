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

template<typename Path>
static bool
path_less(const Path &left, const Path &right) noexcept
{
  if ( left.cost != right.cost ) return left.cost < right.cost;
  const usize vertices = left.vertices.size() < right.vertices.size() ? left.vertices.size() : right.vertices.size();
  for ( usize i = 0; i < vertices; ++i ) {
    if ( left.vertices[i] != right.vertices[i] ) return left.vertices[i] < right.vertices[i];
  }
  if ( left.vertices.size() != right.vertices.size() ) return left.vertices.size() < right.vertices.size();
  for ( usize i = 0; i < left.edges.size(); ++i )
    if ( left.edges[i] != right.edges[i] ) return left.edges[i] < right.edges[i];
  return false;
}

template<typename G>
static auto
all_simple_paths(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target)
{
  using path_type = mg::weighted_path<typename G::index_type, i32>;
  micron::vector<path_type> paths;
  path_type current;
  micron::vector<u8> visited(graph.vertex_slots(), u8(0));
  current.vertices.push_back(source);
  visited[source.value] = 1;
  auto walk = [&](auto &&self, typename G::vertex_descriptor vertex, i32 cost) -> void {
    if ( vertex == target ) {
      current.cost = cost;
      paths.push_back(current);
      return;
    }
    for ( auto edge : graph.out_edges(vertex) ) {
      const auto next = graph.target(edge);
      if ( visited[next.value] ) continue;
      visited[next.value] = 1;
      current.vertices.push_back(next);
      current.edges.push_back(edge);
      self(self, next, cost + graph.edge_weight(edge));
      current.edges.pop_back();
      current.vertices.pop_back();
      visited[next.value] = 0;
    }
  };
  walk(walk, source, 0);
  for ( usize i = 1; i < paths.size(); ++i ) {
    path_type value = micron::move(paths[i]);
    usize at = i;
    while ( at != 0 && path_less(value, paths[at - 1]) ) {
      paths[at] = micron::move(paths[at - 1]);
      --at;
    }
    paths[at] = micron::move(value);
  }
  return paths;
}

using cost_network
    = mm::graph<mm::empty_property, mm::capacity_cost_property<u32, i32>, mm::empty_property, u32, mg::directed_t, mg::parallel_t>;

struct brute_flow_result {
  bool feasible{};
  u32 maximum_flow{};
  i32 minimum_cost{};
};

static brute_flow_result
brute_flow(const cost_network &graph, u32 source, u32 sink, bool fixed, u32 requested, const i32 *supplies = nullptr)
{
  brute_flow_result best{};
  micron::vector<u32> flow(graph.edge_slots(), u32(0));
  auto enumerate = [&](auto &&self, usize edge_slot) -> void {
    if ( edge_slot != graph.edge_slots() ) {
      const auto edge = mm::edge_id<u32>(static_cast<u32>(edge_slot));
      if ( !graph.has_edge(edge) ) {
        self(self, edge_slot + 1);
        return;
      }
      const u32 capacity = graph.edge_property_unchecked(edge).capacity;
      for ( u32 value = 0; value <= capacity; ++value ) {
        flow[edge_slot] = value;
        self(self, edge_slot + 1);
      }
      return;
    }
    i32 balance[8]{};
    i32 cost = 0;
    for ( auto edge : graph.edges() ) {
      const u32 amount = flow[edge.id.value];
      balance[edge.source.value] += static_cast<i32>(amount);
      balance[edge.target.value] -= static_cast<i32>(amount);
      cost += static_cast<i32>(amount) * edge.property.cost;
    }
    u32 achieved = 0;
    if ( supplies ) {
      for ( usize vertex = 0; vertex < graph.vertex_slots(); ++vertex )
        if ( balance[vertex] != supplies[vertex] ) return;
    } else {
      for ( usize vertex = 0; vertex < graph.vertex_slots(); ++vertex ) {
        if ( vertex == source ) {
          if ( balance[vertex] < 0 ) return;
          achieved = static_cast<u32>(balance[vertex]);
        } else if ( vertex == sink ) {
          if ( balance[vertex] != -static_cast<i32>(achieved) ) return;
        } else if ( balance[vertex] != 0 ) {
          return;
        }
      }
      if ( fixed && achieved != requested ) return;
    }
    if ( !best.feasible || (!fixed && !supplies && achieved > best.maximum_flow)
         || ((!fixed && !supplies ? achieved == best.maximum_flow : true) && cost < best.minimum_cost) ) {
      best.feasible = true;
      best.maximum_flow = achieved;
      best.minimum_cost = cost;
    }
  };
  enumerate(enumerate, 0);
  return best;
}

template<typename Tree>
static u32
tree_min_cut(const Tree &tree, u32 source, u32 target)
{
  bool visited[16]{};
  auto walk = [&](auto &&self, u32 vertex, u32 minimum) -> u32 {
    if ( vertex == target ) return minimum;
    visited[vertex] = true;
    for ( auto edge : tree.out_edges(mm::vertex_id<u32>(vertex)) ) {
      const u32 next = tree.opposite(edge, mm::vertex_id<u32>(vertex)).value;
      if ( visited[next] ) continue;
      const u32 capacity = tree.edge_weight(edge);
      const u32 result = self(self, next, minimum < capacity ? minimum : capacity);
      if ( result != micron::numeric_limits<u32>::max() ) return result;
    }
    return micron::numeric_limits<u32>::max();
  };
  return walk(walk, source, micron::numeric_limits<u32>::max());
}

int
main()
{
  test_case("Johnson agrees with Floyd-Warshall on signed sparse DAGs");
  {
    u64 random = 0x243f6a8885a308d3ull;
    mg::johnson_workspace<u32, i32> workspace;
    for ( usize trial = 0; trial < 128; ++trial ) {
      mm::weighted_digraph<i32> graph;
      const usize vertices = 2 + next_random(random) % 9;
      (void)graph.add_vertices(vertices);
      for ( u32 u = 0; u < vertices; ++u )
        for ( u32 v = u + 1; v < vertices; ++v )
          if ( next_random(random) % 4 == 0 ) (void)graph.add_edge(u, v, static_cast<i32>(next_random(random) % 31) - 10);
      const auto expected = mg::floyd_warshall(graph);
      const auto actual = mg::johnson(graph, mg::intrinsic_edge_weight{}, workspace);
      require_true(actual.status == expected.status);
      for ( usize i = 0; i < vertices * vertices; ++i ) {
        require_true(actual.reached[i] == expected.reached[i]);
        if ( actual.reached[i] ) require_true(actual.distance[i] == expected.distance[i]);
      }
    }
    mm::weighted_digraph<i32> cycle;
    (void)cycle.add_edge(0, 1, -2);
    (void)cycle.add_edge(1, 0, 1);
    require_true(mg::johnson(cycle).status == mg::algorithm_status::negative_cycle);

    volatile u64 infinity_bits = 0x7ff0000000000000ull;
    const f64 infinity = micron::math::ieee::from_bits<f64>(infinity_bits);
    mm::weighted_digraph<f64> invalid;
    (void)invalid.add_edge(0, 1, infinity);
    require_true(mg::johnson(invalid).status == mg::algorithm_status::invalid_weight);
    require_true(mg::yen(invalid, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1), 1).status == mg::algorithm_status::invalid_weight);
  }
  end_test_case();

  test_case("Yen agrees with exhaustive simple paths including parallel edges");
  {
    using graph_type = mm::weighted_digraph<i32, mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::parallel_t>;
    u64 random = 0x13198a2e03707344ull;
    mg::yen_workspace<u32, i32> workspace;
    for ( usize trial = 0; trial < 96; ++trial ) {
      graph_type graph;
      constexpr u32 vertices = 6;
      (void)graph.add_vertices(vertices);
      for ( u32 u = 0; u < vertices; ++u )
        for ( u32 v = u + 1; v < vertices; ++v ) {
          const usize copies = static_cast<usize>(next_random(random) % 3);
          for ( usize copy = 0; copy < copies; ++copy ) (void)graph.add_edge(u, v, static_cast<i32>(next_random(random) % 13) - 4);
        }
      const auto expected = all_simple_paths(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(vertices - 1));
      const usize wanted = static_cast<usize>(next_random(random) % 12);
      const auto actual
          = mg::yen(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(vertices - 1), wanted, workspace, mg::intrinsic_edge_weight{});
      if ( wanted == 0 ) {
        require_true(actual.status == mg::algorithm_status::ok && actual.paths.empty());
        continue;
      }
      if ( expected.empty() ) {
        require_true(actual.status == mg::algorithm_status::unreachable && actual.paths.empty());
        continue;
      }
      require_true(actual.status == mg::algorithm_status::ok);
      const usize count = wanted < expected.size() ? wanted : expected.size();
      require_true(actual.paths.size() == count);
      for ( usize path = 0; path < count; ++path ) {
        require_true(actual.paths[path].cost == expected[path].cost);
        require_true(actual.paths[path].vertices.size() == expected[path].vertices.size());
        require_true(actual.paths[path].edges.size() == expected[path].edges.size());
        for ( usize i = 0; i < actual.paths[path].vertices.size(); ++i )
          require_true(actual.paths[path].vertices[i] == expected[path].vertices[i]);
        for ( usize i = 0; i < actual.paths[path].edges.size(); ++i ) require_true(actual.paths[path].edges[i] == expected[path].edges[i]);
      }
    }
  }
  end_test_case();

  test_case("Gomory-Hu matches every pairwise flow and certifies each cut side");
  {
    using graph_type = mm::stable_adjacency_graph<mm::empty_property, mm::weighted_property<u32>, mm::empty_property, u32, mg::undirected_t,
                                                  mg::parallel_t>;
    graph_type graph;
    (void)graph.add_vertices(8);
    require_true(graph.remove_vertex(3u));
    const u32 path[][3] = { { 0, 1, 4 }, { 1, 2, 7 }, { 2, 4, 3 }, { 4, 5, 8 }, { 5, 6, 2 }, { 6, 7, 6 } };
    for ( const auto &edge : path ) (void)graph.add_edge(edge[0], edge[1], edge[2]);
    (void)graph.add_edge(0, 4, 5);
    (void)graph.add_edge(1, 5, 2);
    (void)graph.add_edge(2, 6, 4);
    (void)graph.add_edge(0, 1, 3);
    mg::gomory_hu_workspace<u32, u32> workspace;
    const auto cuts = mg::gomory_hu(graph, mg::intrinsic_edge_weight{}, workspace);
    require_true(cuts.status == mg::algorithm_status::ok && cuts.tree.edges_count() == graph.vertices_count() - 1);

    mm::weighted_digraph<u32, mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::parallel_t> network;
    (void)network.add_vertices(graph.vertices_count());
    for ( auto edge : graph.edges() ) {
      const u32 u = cuts.vertex_to_dense[edge.source.value];
      const u32 v = cuts.vertex_to_dense[edge.target.value];
      (void)network.add_edge(u, v, edge.property.weight);
      (void)network.add_edge(v, u, edge.property.weight);
    }
    for ( u32 u = 0; u < graph.vertices_count(); ++u )
      for ( u32 v = u + 1; v < graph.vertices_count(); ++v ) {
        const auto flow = mg::dinic(network, mm::vertex_id<u32>(u), mm::vertex_id<u32>(v));
        require_true(flow.status == mg::algorithm_status::ok);
        require_true(tree_min_cut(cuts.tree, u, v) == flow.value);
      }
    for ( usize child = 1; child < graph.vertices_count(); ++child ) {
      require_true(cuts.side_contains(child, child));
      require_true(!cuts.side_contains(child, cuts.parent[child].value));
      u32 capacity = 0;
      for ( auto edge : graph.edges() ) {
        const usize u = cuts.vertex_to_dense[edge.source.value];
        const usize v = cuts.vertex_to_dense[edge.target.value];
        if ( cuts.side_contains(child, u) != cuts.side_contains(child, v) ) capacity += edge.property.weight;
      }
      require_true(capacity == cuts.cut[child]);
    }
  }
  end_test_case();

  test_case("minimum-cost flow agrees with exhaustive feasible flows");
  {
    cost_network graph;
    (void)graph.add_vertices(4);
    (void)graph.add_edge(0, 1, mm::capacity_cost_property<u32, i32>(2, 2));
    (void)graph.add_edge(0, 2, mm::capacity_cost_property<u32, i32>(2, 1));
    (void)graph.add_edge(1, 3, mm::capacity_cost_property<u32, i32>(2, 1));
    (void)graph.add_edge(2, 3, mm::capacity_cost_property<u32, i32>(2, 3));
    (void)graph.add_edge(1, 2, mm::capacity_cost_property<u32, i32>(1, -3));
    (void)graph.add_edge(2, 1, mm::capacity_cost_property<u32, i32>(1, 0));
    mg::min_cost_flow_workspace<u32, u32, i32> workspace;
    for ( u32 requested = 0; requested <= 4; ++requested ) {
      const auto expected = brute_flow(graph, 0, 3, true, requested);
      const auto actual = mg::min_cost_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), requested, mg::intrinsic_edge_capacity{},
                                            mg::intrinsic_edge_cost{}, workspace);
      require_true(expected.feasible && actual.status == mg::algorithm_status::ok);
      require_true(actual.achieved_flow == requested && actual.total_cost == expected.minimum_cost);
      require_true(mg::verify_min_cost_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), actual));
    }
    const auto infeasible = mg::min_cost_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), u32(5));
    require_true(infeasible.status == mg::algorithm_status::infeasible);
    const auto expected_max = brute_flow(graph, 0, 3, false, 0);
    const auto maximum = mg::min_cost_max_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3));
    require_true(maximum.status == mg::algorithm_status::ok && maximum.achieved_flow == expected_max.maximum_flow);
    require_true(maximum.total_cost == expected_max.minimum_cost);

    micron::vector<i32> supplies{ 2, 0, 0, -2 };
    const auto expected_circulation = brute_flow(graph, 0, 3, true, 0, supplies.data());
    const auto circulation = mg::min_cost_circulation(graph, supplies);
    require_true(circulation.status == mg::algorithm_status::ok && expected_circulation.feasible);
    require_true(circulation.achieved_flow == 2 && circulation.total_cost == expected_circulation.minimum_cost);
    require_true(mg::verify_min_cost_flow(graph, supplies, circulation));

    using wide_capacity_network
        = mm::graph<mm::empty_property, mm::capacity_cost_property<u16, i32>, mm::empty_property, u32, mg::directed_t, mg::parallel_t>;
    wide_capacity_network wide_capacity;
    (void)wide_capacity.add_edge(0, 1, mm::capacity_cost_property<u16, i32>(300, 1));
    require_true(mg::min_cost_flow(wide_capacity, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1), u8(1)).status
                 == mg::algorithm_status::overflow);

    using tiny_index_network
        = mm::graph<mm::empty_property, mm::capacity_cost_property<u8, i32>, mm::empty_property, u8, mg::directed_t, mg::parallel_t>;
    tiny_index_network tiny_index;
    (void)tiny_index.add_vertices(255);
    micron::vector<i16> zero_supplies(255, i16(0));
    require_true(mg::min_cost_circulation(tiny_index, zero_supplies).status == mg::algorithm_status::overflow);
  }
  end_test_case();

  test_case("minimum-cost flow fuzzes signed residual cycles against enumeration");
  {
    u64 random = 0xa4093822299f31d0ull;
    mg::min_cost_flow_workspace<u32, u32, i32> workspace;
    for ( usize trial = 0; trial < 128; ++trial ) {
      cost_network graph;
      (void)graph.add_vertices(4);
      const u32 endpoints[][2] = { { 0, 1 }, { 1, 3 }, { 0, 2 }, { 2, 3 } };
      for ( const auto &endpoints_pair : endpoints )
        (void)graph.add_edge(
            endpoints_pair[0], endpoints_pair[1],
            mm::capacity_cost_property<u32, i32>(static_cast<u32>(next_random(random) % 3), static_cast<i32>(next_random(random) % 9) - 4));
      for ( usize extra = 0; extra < 2; ++extra ) {
        u32 u = static_cast<u32>(next_random(random) % 4);
        u32 v = static_cast<u32>(next_random(random) % 4);
        if ( u == v ) v = (v + 1) % 4;
        (void)graph.add_edge(
            u, v,
            mm::capacity_cost_property<u32, i32>(static_cast<u32>(next_random(random) % 3), static_cast<i32>(next_random(random) % 9) - 4));
      }

      const u32 requested = static_cast<u32>(next_random(random) % 4);
      const auto expected = brute_flow(graph, 0, 3, true, requested);
      const auto actual = mg::min_cost_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), requested, mg::intrinsic_edge_capacity{},
                                            mg::intrinsic_edge_cost{}, workspace);
      if ( expected.feasible ) {
        require_true(actual.status == mg::algorithm_status::ok && actual.achieved_flow == requested);
        require_true(actual.total_cost == expected.minimum_cost);
        require_true(mg::verify_min_cost_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), actual));
      } else {
        require_true(actual.status == mg::algorithm_status::infeasible);
      }

      const auto expected_max = brute_flow(graph, 0, 3, false, 0);
      const auto maximum = mg::min_cost_max_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), mg::intrinsic_edge_capacity{},
                                                 mg::intrinsic_edge_cost{}, workspace);
      require_true(maximum.status == mg::algorithm_status::ok && expected_max.feasible);
      require_true(maximum.achieved_flow == expected_max.maximum_flow && maximum.total_cost == expected_max.minimum_cost);
      require_true(mg::verify_min_cost_flow(graph, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), maximum));

      micron::vector<i32> supplies(4, i32(0));
      for ( auto edge : graph.edges() ) {
        const u32 flow = static_cast<u32>(next_random(random) % (edge.property.capacity + 1));
        supplies[edge.source.value] += static_cast<i32>(flow);
        supplies[edge.target.value] -= static_cast<i32>(flow);
      }
      const auto expected_circulation = brute_flow(graph, 0, 3, true, 0, supplies.data());
      const auto circulation
          = mg::min_cost_circulation(graph, supplies, mg::intrinsic_edge_capacity{}, mg::intrinsic_edge_cost{}, workspace);
      require_true(expected_circulation.feasible && circulation.status == mg::algorithm_status::ok);
      require_true(circulation.total_cost == expected_circulation.minimum_cost);
      require_true(mg::verify_min_cost_flow(graph, supplies, circulation));
    }

    cost_network graph;
    (void)graph.add_edge(0, 1, mm::capacity_cost_property<u32, i32>(1, 0));
    micron::vector<i32> unbalanced{ 1, 0 };
    require_true(mg::min_cost_circulation(graph, unbalanced).status == mg::algorithm_status::infeasible);

    using floating_cost_network
        = mm::graph<mm::empty_property, mm::capacity_cost_property<u32, f64>, mm::empty_property, u32, mg::directed_t, mg::parallel_t>;
    volatile u64 infinity_bits = 0x7ff0000000000000ull;
    floating_cost_network invalid;
    (void)invalid.add_edge(0, 1, mm::capacity_cost_property<u32, f64>(1, micron::math::ieee::from_bits<f64>(infinity_bits)));
    require_true(mg::min_cost_flow(invalid, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1), u32(1)).status
                 == mg::algorithm_status::invalid_weight);
  }
  end_test_case();

  return 1;
}
