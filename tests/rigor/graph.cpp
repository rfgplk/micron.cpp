//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/graph.hpp"
#include "../../src/math/graph.hpp"
#include "../../src/string/format.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace mm = micron::math;
namespace mg = micron::math::graphs;
namespace mig = micron::io::graph;

struct required_vertex_property {
  int value;
  required_vertex_property() = delete;

  explicit required_vertex_property(int input) : value(input) { }
};

struct required_edge_property {
  int value;
  required_edge_property() = delete;

  explicit required_edge_property(int input) : value(input) { }
};

static u64
next_random(u64 &state)
{
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

struct dfs_trace_visitor {
  micron::vector<u32> *discovered{};
  micron::vector<u32> *finished{};
  usize *examined{};

  bool
  discover_vertex(const mm::graph<> &, mm::vertex_id<u32> vertex)
  {
    discovered->push_back(vertex.value);
    return true;
  }

  bool
  examine_edge(const mm::graph<> &, mm::edge_id<u32>)
  {
    ++*examined;
    return true;
  }

  void
  finish_vertex(const mm::graph<> &, mm::vertex_id<u32> vertex)
  {
    finished->push_back(vertex.value);
  }
};

int
main()
{
  sb::print("=== GRAPH TESTS ===");

  test_case("default graph insertion policy and porcelain");
  {
    mm::graph<> graph;
    auto inserted = graph.add_edge(0, 2);
    require_true(inserted.inserted());
    require_true(inserted.id == mm::edge_id<u32>(0));
    require_true(graph.vertices_count() == 3);
    require_true(graph.edges_count() == 1);
    require_true(graph.has_edge(2, 0));
    require_true(graph.add_edge(2, 0).status == mg::edge_insert_status::duplicate);
    require_true(graph.add_edge(1, 1).status == mg::edge_insert_status::self_loop);
    require_true(graph.add_edge(-1, 2).status == mg::edge_insert_status::invalid_vertex);
    require_true(graph.add_edge(static_cast<u64>(mm::vertex_id<u32>::invalid_value()), 2u).status
                 == mg::edge_insert_status::index_overflow);
    graph += mm::edge{ 1u, 2u };
    require_true(graph.edges_count() == 2);
    require_true(graph.degree(mm::vertex_id<u32>(2)) == 2);
    require_true(graph.neighbors(mm::vertex_id<u32>::invalid()).begin() == graph.neighbors(mm::vertex_id<u32>::invalid()).end());

    using tiny_graph = mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u8>;
    tiny_graph tiny;
    require_true(tiny.add_vertices(255).size() == 255);
    require_true(!tiny.add_vertex().valid());
    require_true(tiny.add_edge(u8(0), u8(254)).inserted());
    auto tiny_bytes = mig::binary(tiny);
    auto tiny_round_trip = mig::parse_binary<tiny_graph>(tiny_bytes);
    require_true(tiny_round_trip && tiny_round_trip.value.vertices_count() == 255);

    mm::graph<> overflow;
    require_true(overflow.add_vertices(micron::numeric_limits<usize>::max()).empty());
    require_true(overflow.vertex_slots() == 0);

    mm::graph<mm::empty_property, required_edge_property> required_edge;
    require_true(required_edge.add_edge(0u, 1u).status == mg::edge_insert_status::property_required);
    require_true(required_edge.vertex_slots() == 0 && required_edge.edge_slots() == 0);
    require_true(required_edge.add_edge(0u, 1u, required_edge_property(3)).inserted());

    mm::graph<required_vertex_property> required_vertex;
    require_true(required_vertex.add_edge(0u, 1u).status == mg::edge_insert_status::property_required);
    require_true(required_vertex.vertex_slots() == 0 && required_vertex.edge_slots() == 0);
    auto first = required_vertex.add_vertex(required_vertex_property(1));
    auto second = required_vertex.add_vertex(required_vertex_property(2));
    require_true(required_vertex.add_edge(first, second).inserted());
  }
  end_test_case();

  test_case("tombstones preserve descriptors until compact");
  {
    mm::stable_adjacency_graph<int, int> graph;
    auto v0 = graph.add_vertex(10);
    auto v1 = graph.add_vertex(20);
    auto v2 = graph.add_vertex(30);
    auto e0 = graph.add_edge(v0, v1, 4).id;
    auto e1 = graph.add_edge(v1, v2, 5).id;
    require_true(graph.remove_edge(e0));
    require_true(!graph.has_edge(e0));
    require_true(graph.has_edge(e1));
    require_true(graph.edge_slots() == 2 && graph.edges_count() == 1);
    require_true(graph.remove_vertex(v1));
    require_true(!graph.has_vertex(v1));
    require_true(graph.vertex_slots() == 3 && graph.vertices_count() == 2);
    auto e2 = graph.add_edge(v0, v2, 9).id;
    require_true(e2 == mm::edge_id<u32>(2));
    auto remap = graph.compact();
    require_true(graph.vertex_slots() == 2 && graph.edge_slots() == 1);
    require_true(!remap.vertex_remap[1].valid());
    require_true(remap.vertex_remap[2] == mm::vertex_id<u32>(1));
    require_true(remap.edge_remap[2] == mm::edge_id<u32>(0));
    require_true(graph.edge_property(mm::edge_id<u32>(0)) == 9);
  }
  end_test_case();

  test_case("directed, parallel, loop, weighted, and labeled policies");
  {
    mm::digraph<> directed;
    (void)directed.add_edge(0, 1);
    (void)directed.add_edge(2, 1);
    require_true(directed.out_degree(mm::vertex_id<u32>(0)) == 1);
    require_true(directed.in_degree(mm::vertex_id<u32>(1)) == 2);
    require_true(!directed.has_edge(1, 0));

    mm::multigraph<mm::empty_property, int, mm::empty_property, u32, mg::allow_loops_t> multi;
    require_true(multi.add_edge(0, 0, 1).inserted());
    require_true(multi.add_edge(0, 0, 2).inserted());
    require_true(multi.degree(mm::vertex_id<u32>(0)) == 4);

    mm::weighted_digraph<i32> weighted;
    auto edge = weighted.add_edge(0, 1, 17);
    require_true(edge.inserted());
    require_true(weighted.edge_weight(edge.id) == 17);

    mm::labeled_graph<micron::string, int> labeled;
    auto alice = labeled.add_labeled_vertex(micron::string("alice"), 7);
    auto bob = labeled.add_labeled_vertex(micron::string("bob"), 8);
    (void)labeled.add_edge(alice, bob);
    require_true(labeled.find_vertex(micron::string("bob")) == bob);
    require_true(labeled.vertex_property(bob).property == 8);
    auto rendered = micron::format::format("{:e}", labeled);
    require_true(micron::format::find(rendered, "alice") != nullptr);
    require_true(micron::format::find(rendered, " @ ") != nullptr);
    auto labeled_frozen = labeled.freeze();
    auto frozen_rendered = micron::format::format("{:e}", labeled_frozen.value);
    require_true(micron::format::find(frozen_rendered, "bob") != nullptr);
  }
  end_test_case();

  test_case("traversal, connectivity, and SCC certificates");
  {
    auto graph = mg::path_graph<>(6);
    auto bfs = mg::bfs(graph, 0u);
    require_true(bfs.status == mg::algorithm_status::ok);
    require_true(bfs.order.size() == 6);
    require_true(bfs.depth[5] == 5);
    require_true(mg::reachable(graph, 0u, 5u));
    require_true(mg::connected_components(graph).count == 1);
    require_true(mg::bridges(graph).size() == 5);
    require_true(mg::articulation_points(graph).size() == 4);
    auto blocks = mg::biconnected_components(graph);
    require_true(blocks.status == mg::algorithm_status::ok && blocks.blocks.size() == 5);
    require_true(mg::is_bipartite(graph));

    micron::vector<u32> discovered;
    micron::vector<u32> finished;
    usize examined = 0;
    auto depth_first
        = mg::dfs(graph, mm::vertex_id<u32>(0), dfs_trace_visitor{ micron::addressof(discovered), micron::addressof(finished), &examined });
    require_true(depth_first.order.size() == 6 && discovered.size() == 6 && finished.size() == 6);
    for ( usize i = 0; i < 6; ++i ) {
      require_true(discovered[i] == i);
      require_true(finished[i] == 5 - i);
    }
    require_true(examined == graph.edges_count() * 2);

    mm::digraph<> directed;
    (void)directed.add_edge(0, 1);
    (void)directed.add_edge(1, 0);
    (void)directed.add_edge(1, 2);
    (void)directed.add_edge(2, 3);
    (void)directed.add_edge(3, 2);
    auto scc = mg::strongly_connected_components(directed);
    auto tarjan = mg::tarjan_scc(directed);
    require_true(scc.count == 2);
    require_true(tarjan.count == scc.count);
    require_true(scc[mm::vertex_id<u32>(0)] == scc[mm::vertex_id<u32>(1)]);
    require_true(scc[mm::vertex_id<u32>(2)] == scc[mm::vertex_id<u32>(3)]);
    for ( auto a : directed.vertices() )
      for ( auto b : directed.vertices() ) require_true((scc[a] == scc[b]) == (tarjan[a] == tarjan[b]));
  }
  end_test_case();

  test_case("shortest paths never depend on infinity sentinels");
  {
    mm::weighted_digraph<i32> graph;
    (void)graph.add_edge(0, 1, 4);
    (void)graph.add_edge(0, 2, 10);
    (void)graph.add_edge(1, 2, 3);
    (void)graph.add_edge(2, 3, 2);
    auto paths = mg::dijkstra(graph, 0u);
    require_true(paths.status == mg::algorithm_status::ok);
    require_true(paths.distance[3] == 9);
    auto path = paths.path_to(mm::vertex_id<u32>(3));
    require_true(path.size() == 4 && path[1].value == 1 && path[2].value == 2);

    mm::weighted_digraph<i32> negative;
    (void)negative.add_edge(0, 1, 2);
    (void)negative.add_edge(1, 2, -5);
    (void)negative.add_edge(0, 2, 1);
    auto bf = mg::bellman_ford(negative, mm::vertex_id<u32>(0));
    require_true(bf.status == mg::algorithm_status::ok && bf.distance[2] == -3);
    (void)negative.add_edge(2, 1, 1);
    require_true(mg::bellman_ford(negative, mm::vertex_id<u32>(0)).status == mg::algorithm_status::negative_cycle);

    mm::digraph<> dag;
    (void)dag.add_edge(0, 2);
    (void)dag.add_edge(1, 2);
    (void)dag.add_edge(2, 3);
    auto order = mg::topological_sort(dag);
    require_true(order.status == mg::algorithm_status::ok && order.order.size() == 4);
  }
  end_test_case();

  test_case("fixed-seed Dijkstra heap agrees with Bellman-Ford");
  {
    constexpr usize vertices = 32;
    mm::weighted_digraph<u32> graph;
    (void)graph.add_vertices(vertices);
    u64 state = 0xe7037ed1a0b428dbull;
    for ( usize edge = 0; edge < 180; ++edge ) {
      const u32 u = static_cast<u32>(next_random(state) % vertices);
      const u32 v = static_cast<u32>(next_random(state) % vertices);
      if ( u != v ) (void)graph.add_edge(u, v, static_cast<u32>(1 + next_random(state) % 100));
    }
    for ( u32 source = 0; source < vertices; ++source ) {
      auto fast = mg::dijkstra(graph, source);
      auto oracle = mg::bellman_ford(graph, mm::vertex_id<u32>(source));
      require_true(fast.status == oracle.status);
      for ( u32 target = 0; target < vertices; ++target ) {
        const mm::vertex_id<u32> vertex(target);
        require_true(fast.contains(vertex) == oracle.contains(vertex));
        if ( fast.contains(vertex) ) require_true(fast.distance[target] == oracle.distance[target]);
      }
    }
  }
  end_test_case();

  test_case("representations, views, matrices, and constexpr storage");
  {
    mm::stable_adjacency_graph<> graph;
    (void)graph.add_edge(0, 1);
    (void)graph.add_edge(1, 2);
    (void)graph.add_edge(2, 3);
    (void)graph.remove_vertex(1u);
    (void)graph.add_edge(2, 3);      // duplicate: no mutation
    auto frozen = graph.freeze();
    require_true(frozen.value.vertices_count() == 3);
    require_true(!frozen.vertex_remap[1].valid());
    require_true(mg::bfs(frozen.value, mm::vertex_id<u32>(1)).order.size() == 2);
    auto thawed = frozen.value.thaw();
    require_true(thawed.value.vertices_count() == frozen.value.vertices_count());
    require_true(thawed.value.edges_count() == frozen.value.edges_count());

    auto matrix = mg::adjacency_matrix<>(graph);
    require_true(matrix.value.rows == 3 && matrix.value.cols == 3);
    auto view = mm::as_graph_view(matrix.value);
    require_true(view.vertices_count() == 3);
    auto reverse = mg::reverse(graph);
    require_true(mg::bfs(reverse, 3u).order.size() >= 1);
    auto filtered = mg::induced_subgraph(graph, [](auto vertex) { return vertex.value >= 2; });
    require_true(filtered.vertices_count() == 2);

    mm::digraph<> directed;
    (void)directed.add_edge(0, 1);
    (void)directed.add_edge(2, 1);
    auto directed_csr = directed.freeze();
    auto bidirectional_csr = directed.freeze_bidirectional();
    static_assert(!decltype(directed_csr.value)::has_in_index);
    static_assert(decltype(bidirectional_csr.value)::has_in_index);
    require_true(directed_csr.value.out_degree(mm::vertex_id<u32>(0)) == 1);
    require_true(directed_csr.value.in_degree(mm::vertex_id<u32>(1)) == 2);
    require_true(bidirectional_csr.value.in_degree(mm::vertex_id<u32>(1)) == 2);
    require_true(mg::bfs(mg::reverse(directed_csr.value), mm::vertex_id<u32>(1)).order.size() == 3);

    constexpr auto fixed_ok = [] {
      mm::fixed_graph<4, 4> fixed;
      auto result = fixed.add_edge(0u, 3u);
      return result.inserted() && fixed.vertices_count() == 4 && fixed.edges_count() == 1;
    }();
    static_assert(fixed_ok);
    require_true(fixed_ok);
    mm::fixed_graph<4, 4> fixed_printable;
    (void)fixed_printable.add_edge(0u, 3u);
    auto fixed_summary = micron::format::format("{:s}", fixed_printable);
    require_true(micron::format::find(fixed_summary, "vertices: 4") != nullptr);
  }
  end_test_case();

  test_case("spanning tree, max flow, and bipartite matching certificates");
  {
    mm::weighted_graph<u32> weighted;
    (void)weighted.add_edge(0, 1, 1);
    (void)weighted.add_edge(1, 2, 2);
    (void)weighted.add_edge(0, 2, 9);
    auto tree = mg::kruskal(weighted);
    require_true(tree.status == mg::algorithm_status::ok);
    require_true(tree.edges.size() == 2 && tree.weight == 3);

    mm::weighted_digraph<u32> network;
    (void)network.add_edge(0, 1, 3);
    (void)network.add_edge(0, 2, 2);
    (void)network.add_edge(1, 2, 1);
    (void)network.add_edge(1, 3, 2);
    (void)network.add_edge(2, 3, 4);
    auto flow = mg::dinic(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3));
    require_true(flow.status == mg::algorithm_status::ok && flow.value == 5);
    require_true(mg::verify_flow(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), flow, mg::intrinsic_edge_weight{}));
    auto augmenting = mg::edmonds_karp(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3));
    require_true(augmenting.status == mg::algorithm_status::ok && augmenting.value == flow.value);
    require_true(mg::verify_flow(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), augmenting, mg::intrinsic_edge_weight{}));
    auto preflow = mg::push_relabel(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3));
    require_true(preflow.status == mg::algorithm_status::ok && preflow.value == flow.value);
    require_true(mg::verify_flow(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), preflow, mg::intrinsic_edge_weight{}));

    auto path = mg::path_graph<>(8);
    auto matching = mg::hopcroft_karp(path);
    require_true(matching.status == mg::algorithm_status::ok && matching.cardinality == 4);
    require_true(mg::validate_matching(path, matching));
    require_true(mg::edge_connectivity(path) == 1);
    require_true(mg::node_connectivity(path) == 1);
    require_true(mg::edge_connectivity(mg::cycle_graph<>(6)) == 2);
    require_true(mg::node_connectivity(mg::complete_graph<>(4)) == 3);
  }
  end_test_case();

  test_case("fixed-seed max-flow implementations agree");
  {
    u64 state = 0x8ebc6af09c88c6e3ull;
    for ( usize sample = 0; sample < 64; ++sample ) {
      mm::weighted_digraph<u32> network;
      (void)network.add_vertices(10);
      for ( usize attempt = 0; attempt < 80; ++attempt ) {
        const u32 u = static_cast<u32>(next_random(state) % 10);
        const u32 v = static_cast<u32>(next_random(state) % 10);
        if ( u != v ) (void)network.add_edge(u, v, static_cast<u32>(1 + next_random(state) % 31));
      }
      const auto source = mm::vertex_id<u32>(0);
      const auto sink = mm::vertex_id<u32>(9);
      auto level = mg::dinic(network, source, sink);
      auto augmenting = mg::edmonds_karp(network, source, sink);
      auto preflow = mg::push_relabel(network, source, sink);
      require_true(level.status == mg::algorithm_status::ok);
      require_true(augmenting.status == mg::algorithm_status::ok && augmenting.value == level.value);
      require_true(preflow.status == mg::algorithm_status::ok && preflow.value == level.value);
      require_true(mg::verify_flow(network, source, sink, augmenting, mg::intrinsic_edge_weight{}));
      require_true(mg::verify_flow(network, source, sink, preflow, mg::intrinsic_edge_weight{}));
    }
  }
  end_test_case();

  test_case("structure and analytics");
  {
    auto cycle = mg::cycle_graph<>(5);
    require_true(mg::has_cycle(cycle));
    auto euler = mg::eulerian_path(cycle);
    require_true(euler.status == mg::algorithm_status::ok && euler.circuit && euler.edges.size() == 5);
    require_true(mg::triangles(mg::complete_graph<>(4)) == 4);
    require_true(mg::is_chordal(mg::complete_graph<>(5)));
    require_true(!mg::is_chordal(cycle));
    auto rank = mg::pagerank(cycle, 0.85, 1e-12, 200);
    require_true(rank.status == mg::algorithm_status::ok);
    auto eigenvector = mg::eigenvector_centrality(mg::complete_graph<>(4));
    require_true(eigenvector.status == mg::algorithm_status::ok);
    auto katz = mg::katz_centrality(mg::complete_graph<>(4), 0.1, 1.0, 1e-10, 200);
    require_true(katz.status == mg::algorithm_status::ok);
    require_true(mg::degree_assortativity(cycle) == 0.0);
    require_true(mg::average_clustering(mg::complete_graph<>(4)) == 1.0);
    require_true(mg::k_core_vertices(mg::complete_graph<>(4), 3).size() == 4);
    require_true(mg::rich_club_coefficient(mg::complete_graph<>(4), 2) == 1.0);
    require_true(mg::preferential_attachment_score(cycle, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1)) == 4);
    require_true(mg::resource_allocation_index(cycle, mm::vertex_id<u32>(0), mm::vertex_id<u32>(2)) == 0.5);
    require_true(mg::distance_extrema(cycle).diameter == 2);
    require_true(mg::maximum_clique_heuristic(mg::complete_graph<>(4)).size() == 4);
    require_true(mg::weisfeiler_lehman_hash(cycle) != 0);
  }
  end_test_case();

  test_case("advanced path, ordering, cut, and dominator routines");
  {
    mm::weighted_digraph<i32> dag;
    (void)dag.add_edge(0, 1, 2);
    (void)dag.add_edge(0, 2, 7);
    (void)dag.add_edge(1, 2, -5);
    (void)dag.add_edge(2, 3, 4);
    auto shortest = mg::dag_shortest_paths(dag, mm::vertex_id<u32>(0));
    require_true(shortest.status == mg::algorithm_status::ok && shortest.distance[3] == 1);
    auto longest = mg::dag_longest_paths(dag, mm::vertex_id<u32>(0));
    require_true(longest.status == mg::algorithm_status::ok && longest.distance[3] == 11);
    auto queue_paths = mg::spfa(dag, mm::vertex_id<u32>(0));
    require_true(queue_paths.status == mg::algorithm_status::ok && queue_paths.distance[3] == 1);
    auto all_pairs = mg::floyd_warshall(dag);
    require_true(all_pairs.status == mg::algorithm_status::ok);
    require_true(*all_pairs.try_distance(mm::vertex_id<u32>(0), mm::vertex_id<u32>(3)) == 1);
    auto best = mg::astar(dag, mm::vertex_id<u32>(0), mm::vertex_id<u32>(3), [](auto, auto) { return i32(0); });
    require_true(best.status == mg::algorithm_status::invalid_weight);

    mm::digraph<> topology;
    (void)topology.add_edge(0, 1);
    (void)topology.add_edge(0, 2);
    (void)topology.add_edge(1, 2);
    (void)topology.add_edge(2, 3);
    auto generations = mg::topological_generations(topology);
    require_true(generations.size() == 4);
    auto closure = mg::transitive_closure(topology);
    require_true(closure.has_edge(0, 3));
    mg::algorithm_status reduction_status{};
    auto reduction = mg::transitive_reduction(topology, &reduction_status);
    require_true(reduction_status == mg::algorithm_status::ok && !reduction.has_edge(0, 2));
    auto dom = mg::dominators(topology, mm::vertex_id<u32>(0));
    require_true(dom.status == mg::algorithm_status::ok);
    require_true(dom.dominates(mm::vertex_id<u32>(0), mm::vertex_id<u32>(3)));
    require_true(dom.immediate[3] == mm::vertex_id<u32>(2));

    mm::weighted_graph<u32> weighted;
    (void)weighted.add_edge(0, 1, 1);
    (void)weighted.add_edge(1, 2, 2);
    (void)weighted.add_edge(0, 2, 9);
    require_true(mg::prim(weighted).weight == 3);
    require_true(mg::boruvka(weighted).weight == 3);
    auto minimum_cut = mg::stoer_wagner(weighted);
    require_true(minimum_cut.status == mg::algorithm_status::ok && minimum_cut.value == 3);
    require_true(mg::reverse_cuthill_mckee_ordering(weighted).size() == 3);
  }
  end_test_case();

  test_case("generators, operators, communities, and approximation certificates");
  {
    u64 state = 0xa0761d6478bd642full;
    auto rng = [&state]() { return next_random(state); };
    auto random = mg::erdos_renyi<>(20, 0.2, rng);
    require_true(random.vertices_count() == 20);
    require_true(mg::barabasi_albert<>(20, 2, rng).vertices_count() == 20);
    require_true(mg::watts_strogatz<>(20, 4, 0.25, rng).vertices_count() == 20);
    require_true(mg::rmat<>(5, 40, rng).vertices_count() == 32);

    auto path = mg::path_graph<>(6);
    auto cycle = mg::cycle_graph<>(6);
    require_true(mg::graph_union(path, cycle).edges_count() == 6);
    require_true(mg::graph_intersection(path, cycle).edges_count() == 5);
    require_true(mg::graph_difference(cycle, path).edges_count() == 1);
    require_true(mg::complement(mg::complete_graph<>(5)).edges_count() == 0);
    require_true(mg::line_graph(path).vertices_count() == path.edges_count());
    require_true(mg::ego_graph(path, mm::vertex_id<u32>(2), 1).vertices_count() == 3);
    auto product_a = mg::path_graph<>(2);
    auto product_b = mg::path_graph<>(3);
    require_true(mg::cartesian_product(product_a, product_b).edges_count() == 7);
    require_true(mg::tensor_product(product_a, product_b).edges_count() == 4);
    require_true(mg::strong_product(product_a, product_b).edges_count() == 11);

    mm::stable_adjacency_graph<> sparse_slots;
    (void)sparse_slots.add_vertices(3);
    (void)sparse_slots.add_edge(0, 2);
    (void)sparse_slots.remove_vertex(1u);
    auto topology_copy = mg::topology_copy(sparse_slots);
    require_true(topology_copy.vertex_slots() == 3 && topology_copy.vertices_count() == 2);
    require_true(!topology_copy.has_vertex(1u));

    auto complete = mg::complete_graph<>(5);
    auto coloring = mg::greedy_coloring(complete);
    require_true(coloring.colors == 5);
    require_true(mg::maximal_independent_set(complete).size() == 1);
    auto dominating = mg::greedy_dominating_set(complete);
    require_true(dominating.size() == 1);
    require_true(mg::approximate_vertex_cover(path).size() <= path.vertices_count());
    require_true(mg::max_cut(cycle).cut_edges == cycle.edges_count());
    require_true(mg::treewidth_min_degree_upper_bound(path) == 1);

    mm::graph<> clusters;
    for ( u32 base : { 0u, 3u } ) {
      (void)clusters.add_edge(base, base + 1);
      (void)clusters.add_edge(base + 1, base + 2);
      (void)clusters.add_edge(base, base + 2);
    }
    auto partition = mg::label_propagation(clusters);
    require_true(partition.status == mg::algorithm_status::ok && partition.communities == 2);
    require_true(mg::partition_coverage(clusters, partition.community) == 1.0);
    require_true(mg::modularity(clusters, partition.community) > 0.0);
    auto louvain = mg::louvain(clusters);
    auto leiden = mg::leiden(clusters);
    auto greedy_communities = mg::greedy_modularity_communities(clusters);
    require_true(louvain.communities == 2 && leiden.communities == 2 && greedy_communities.communities == 2);
    require_true(mg::partition_coverage(clusters, louvain.community) == 1.0);
    require_true(mg::partition_coverage(clusters, leiden.community) == 1.0);
    require_true(mg::partition_coverage(clusters, greedy_communities.community) == 1.0);
  }
  end_test_case();

  test_case("all matrix and text representations round trip");
  {
    auto source = mg::path_graph<>(5);
    auto adjacency = mg::adjacency_matrix<>(source);
    auto degree = mg::degree_matrix<>(source);
    auto laplacian = mg::laplacian_matrix<>(source);
    auto incidence = mg::incidence_matrix<>(source);
    auto sparse = mg::sparse_adjacency_matrix<>(source);
    auto sparse_columns = mg::sparse_adjacency_matrix_csc<>(source);
    auto restored = mg::from_adjacency_matrix(adjacency.value);
    auto restored_sparse = mg::from_sparse_adjacency_matrix<>(sparse.value);
    auto restored_columns = mg::from_sparse_adjacency_matrix<>(sparse_columns.value);
    require_true(restored.vertices_count() == 5 && restored.edges_count() == 4);
    require_true(restored_sparse.vertices_count() == 5 && restored_sparse.edges_count() == 4);
    require_true(restored_columns.vertices_count() == 5 && restored_columns.edges_count() == 4);
    require_true(degree.value.at(2, 2) == 2);
    require_true(laplacian.value.at(2, 2) == 2 && laplacian.value.at(2, 1) == -1);
    require_true(incidence.value.rows == 5 && incidence.value.cols == 4);
    require_true(sparse.value.inner.size() == 8);

    mm::weighted_digraph<i32> weighted;
    (void)weighted.add_edge(0, 1, -4);
    (void)weighted.add_edge(1, 2, 7);
    auto weighted_sparse = mg::sparse_weighted_adjacency_matrix<i32>(weighted);
    auto weighted_restored = mg::from_sparse_adjacency_matrix<mm::weighted_digraph<i32>>(weighted_sparse.value);
    require_true(weighted_restored.edge_weight(mm::edge_id<u32>(0)) == -4);
    require_true(weighted_restored.edge_weight(mm::edge_id<u32>(1)) == 7);

    auto adjacency_text = mig::adjacency_list(source);
    auto adjacency_round_trip = mig::parse_adjacency_list<>(adjacency_text);
    require_true(adjacency_round_trip && adjacency_round_trip.value.edges_count() == 4);
    auto market_text = mig::matrix_market(source);
    auto market_round_trip = mig::parse_matrix_market<>(market_text);
    require_true(market_round_trip && market_round_trip.value.edges_count() == 4);
    auto dimacs_text = mig::dimacs(source);
    auto dimacs_round_trip = mig::parse_dimacs<>(dimacs_text);
    require_true(dimacs_round_trip && dimacs_round_trip.value.edges_count() == 4);

    micron::vector<i32> exterior(source.vertex_slots(), i32(0));
    auto map = mg::property_map(exterior);
    map[mm::vertex_id<u32>(3)] = 19;
    require_true(exterior[3] == 19);
    auto pointer_map = mg::property_map(exterior.data(), exterior.size());
    require_true(pointer_map.try_get(mm::vertex_id<u32>(3)) == exterior.data() + 3);
    require_true(pointer_map.try_get(mm::vertex_id<u32>::invalid()) == nullptr);
    auto row_map = mg::row_property_map(adjacency.value, 2);
    auto column_map = mg::column_property_map(adjacency.value, 2);
    require_true(row_map[mm::vertex_id<u32>(1)] == adjacency.value.at(2, 1));
    require_true(column_map[mm::vertex_id<u32>(1)] == adjacency.value.at(1, 2));
  }
  end_test_case();

  test_case("text, format, and binary round trips reject malformed data");
  {
    mm::stable_adjacency_graph<> graph;
    (void)graph.add_edge(0, 1);
    auto removed = graph.add_edge(1, 2).id;
    (void)graph.add_edge(2, 3);
    require_true(graph.remove_edge(removed));
    require_true(graph.remove_vertex(1u));

    auto bytes = mig::binary(graph);
    auto decoded = mig::parse_binary<mm::stable_adjacency_graph<>>(bytes);
    require_true(static_cast<bool>(decoded));
    require_true(decoded.value.vertex_slots() == graph.vertex_slots());
    require_true(decoded.value.edge_slots() == graph.edge_slots());
    require_true(!decoded.value.has_vertex(1u));
    require_true(decoded.value.has_edge(mm::edge_id<u32>(2)));
    auto packed = mig::parse_binary<>(bytes);
    require_true(static_cast<bool>(packed));
    require_true(packed.value.vertex_slots() == graph.vertices_count());
    require_true(packed.value.edge_slots() == graph.edges_count());
    require_true(!packed.vertex_remap[1].valid());
    require_true(packed.vertex_remap[2] == mm::vertex_id<u32>(1));
    require_true(packed.edge_remap[2] == mm::edge_id<u32>(0));
    auto trailing = bytes;
    trailing.push_back(0);
    require_true(mig::parse_binary<>(trailing).status.code == mig::parse_code::corrupt);
    auto reserved = bytes;
    reserved[9] = 1;
    require_true(mig::parse_binary<>(reserved).status.code == mig::parse_code::corrupt);
    for ( usize prefix = 0; prefix < bytes.size(); ++prefix ) {
      auto partial = mig::parse_binary<>(bytes.data(), prefix);
      require_true(!partial && partial.value.empty());
    }

    auto text = mig::edge_list(mg::path_graph<>(5));
    auto parsed = mig::parse_edge_list<>(text);
    require_true(static_cast<bool>(parsed) && parsed.value.edges_count() == 4);
    auto bad = mig::parse_edge_list<>("0 nope\n", 7);
    require_true(!static_cast<bool>(bad) && bad.value.empty());
    bytes.pop_back();
    require_true(mig::parse_binary<>(bytes).status.code == mig::parse_code::truncated);

    auto summary = micron::format::format("{:s}", graph);
    require_true(micron::format::find(summary, "vertices: 3") != nullptr);
    auto edges = micron::format::format("{:e}", graph);
    require_true(micron::format::find(edges, "edges:") != nullptr);

    mm::weighted_digraph<i32> weighted;
    (void)weighted.add_edge(0, 1, -7);
    (void)weighted.add_edge(1, 2, 11);
    mig::native_property_codec codec;
    auto weighted_bytes = mig::binary(weighted, codec);
    auto weighted_decoded = mig::parse_binary<mm::weighted_digraph<i32>>(weighted_bytes, codec);
    require_true(static_cast<bool>(weighted_decoded));
    require_true(weighted_decoded.value.edge_weight(mm::edge_id<u32>(0)) == -7);
    require_true(weighted_decoded.value.edge_weight(mm::edge_id<u32>(1)) == 11);
    for ( usize prefix = 0; prefix < weighted_bytes.size(); ++prefix ) {
      auto partial = mig::parse_binary<mm::weighted_digraph<i32>>(weighted_bytes.data(), prefix, codec);
      require_true(!partial && partial.value.empty());
    }
    weighted_bytes.pop_back();
    require_true(mig::parse_binary<mm::weighted_digraph<i32>>(weighted_bytes, codec).status.code == mig::parse_code::truncated);
  }
  end_test_case();

  test_case("architecture-selected bitset kernels agree with scalar truth");
  {
    u64 a[5] = { 0xf0f0u, 0xaaaa5555u, 0, ~u64(0), 7 };
    u64 b[5] = { 0x0ff0u, 0x5555aaaau, 3, 0, 5 };
    u64 out[5]{};
    mg::kernels::set_union(out, a, b, 5);
    for ( usize i = 0; i < 5; ++i ) require_true(out[i] == (a[i] | b[i]));
    mg::kernels::set_intersection(out, a, b, 5);
    for ( usize i = 0; i < 5; ++i ) require_true(out[i] == (a[i] & b[i]));
    require_true(mg::kernels::common_neighbor_count(a, b, 5) == mg::kernels::popcount_reduce(out, 5));
  }
  end_test_case();

  test_case("fixed-seed mutation fuzz preserves the simple-graph oracle");
  {
    constexpr usize count = 24;
    bool live[count]{};
    bool adjacency[count][count]{};
    mm::stable_adjacency_graph<> graph;
    (void)graph.add_vertices(count);
    for ( usize i = 0; i < count; ++i ) live[i] = true;
    u64 state = 0xd1b54a32d192ed03ull;
    for ( usize step = 0; step < 4000; ++step ) {
      const usize u = static_cast<usize>(next_random(state) % count);
      const usize v = static_cast<usize>(next_random(state) % count);
      const u64 operation = next_random(state) % 5;
      if ( operation < 3 && u != v && live[u] && live[v] ) {
        auto inserted = graph.add_edge(static_cast<u32>(u), static_cast<u32>(v));
        if ( adjacency[u][v] )
          require_true(inserted.status == mg::edge_insert_status::duplicate);
        else {
          require_true(inserted.inserted());
          adjacency[u][v] = adjacency[v][u] = true;
        }
      } else if ( operation == 3 && u != v ) {
        const bool removed = graph.remove_edge(static_cast<u32>(u), static_cast<u32>(v));
        require_true(removed == adjacency[u][v]);
        adjacency[u][v] = adjacency[v][u] = false;
      } else if ( operation == 4 && live[u] ) {
        require_true(graph.remove_vertex(static_cast<u32>(u)));
        live[u] = false;
        for ( usize i = 0; i < count; ++i ) adjacency[u][i] = adjacency[i][u] = false;
      }
      usize oracle_edges = 0;
      for ( usize i = 0; i < count; ++i )
        for ( usize j = i + 1; j < count; ++j ) {
          require_true(graph.has_edge(static_cast<u32>(i), static_cast<u32>(j)) == adjacency[i][j]);
          oracle_edges += adjacency[i][j] ? 1 : 0;
        }
      require_true(graph.edges_count() == oracle_edges);
    }
  }
  end_test_case();

  sb::print("=== GRAPH TESTS COMPLETE ===");
  return 1;
}
