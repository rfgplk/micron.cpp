//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Build: duck build benches/graph_bench.cpp --perf --fp --no-ssp --no-lto -o bin/graph-bench -f
// Run:   taskset -c <quiet-cpu> bin/graph-bench/graph_bench

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/math/graph.hpp"

namespace mm = micron::math;
namespace mg = micron::math::graphs;

namespace
{

constexpr u32 measurements = 7;
constexpr u32 warmups = 2;
volatile u64 sink{};

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t time{};
  micron::clock_gettime(micron::clock_monotonic, time);
  return static_cast<u64>(time.tv_sec) * 1000000000ull + static_cast<u64>(time.tv_nsec);
}

f64
minimum(const f64 *values, usize count) noexcept
{
  f64 result = values[0];
  for ( usize i = 1; i < count; ++i )
    if ( values[i] < result ) result = values[i];
  return result;
}

void
row(const char *name, usize items, f64 nanoseconds)
{
  micron::io::print("  ", name);
  for ( usize i = micron::strlen(name); i < 31; ++i ) micron::io::print(" ");
  const u64 hundredths = static_cast<u64>(nanoseconds * 100.0 + 0.5);
  micron::io::print("N=", items, "  ", hundredths / 100, ".");
  if ( hundredths % 100 < 10 ) micron::io::print("0");
  micron::io::println(hundredths % 100, " ns/item");
}

template<typename Fn>
void
measure(const char *name, usize items, usize repetitions, Fn fn)
{
  f64 samples[measurements]{};
  for ( u32 sample = 0; sample < measurements + warmups; ++sample ) {
    const u64 begin = now_ns();
    u64 value = 0;
    for ( usize repetition = 0; repetition < repetitions; ++repetition ) value += static_cast<u64>(fn());
    const u64 elapsed = now_ns() - begin;
    sink = sink + value;
    if ( sample >= warmups ) samples[sample - warmups] = static_cast<f64>(elapsed) / static_cast<f64>(items * repetitions);
  }
  row(name, items, minimum(samples, measurements));
}

template<typename G = mm::graph<>>
G
make_topology_as(usize vertices)
{
  G graph;
  graph.reserve_vertices(vertices);
  graph.reserve_edges(vertices * 3);
  (void)graph.add_vertices(vertices);
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 1) % vertices));
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 17) % vertices));
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 101) % vertices));
  }
  return graph;
}

mm::graph<>
make_topology(usize vertices)
{
  return make_topology_as<>(vertices);
}

template<typename G>
usize
find_batch(const G &graph, usize vertices)
{
  usize found = 0;
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    found += graph.has_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 1) % vertices));
    found += graph.has_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 17) % vertices));
    found += graph.has_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 37) % vertices));
  }
  return found;
}

template<typename G>
usize
traverse_edges(const G &graph)
{
  usize sum = 0;
  for ( auto edge : graph.edges() ) sum += edge.source.value + edge.target.value;
  return sum;
}

template<typename G>
usize
erase_restore_batch(G &graph, usize vertices)
{
  usize restored = 0;
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    const u32 target = static_cast<u32>((vertex + 1) % vertices);
    restored += graph.remove_edge(static_cast<u32>(vertex), target);
    restored += graph.add_edge(static_cast<u32>(vertex), target).inserted();
  }
  return restored;
}

mm::weighted_digraph<u32>
make_weighted(usize vertices)
{
  mm::weighted_digraph<u32> graph;
  graph.reserve_vertices(vertices);
  graph.reserve_edges(vertices * 3);
  (void)graph.add_vertices(vertices);
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    if ( vertex + 1 < vertices ) (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>(vertex + 1), 1u);
    if ( vertex + 7 < vertices ) (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>(vertex + 7), 3u);
    if ( vertex + 31 < vertices ) (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>(vertex + 31), 9u);
  }
  return graph;
}

mm::weighted_digraph<u32>
make_flow_network(usize width)
{
  mm::weighted_digraph<u32> graph;
  const u32 sink_vertex = static_cast<u32>(width * 2 + 1);
  (void)graph.add_vertices(sink_vertex + 1);
  for ( usize i = 0; i < width; ++i ) {
    const u32 left = static_cast<u32>(1 + i);
    const u32 right = static_cast<u32>(1 + width + i);
    (void)graph.add_edge(0u, left, 8u);
    (void)graph.add_edge(left, right, 5u);
    (void)graph.add_edge(left, static_cast<u32>(1 + width + ((i + 1) % width)), 3u);
    (void)graph.add_edge(right, sink_vertex, 8u);
  }
  return graph;
}

mm::weighted_digraph<i32>
make_signed_paths(usize vertices)
{
  mm::weighted_digraph<i32> graph;
  (void)graph.add_vertices(vertices);
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    if ( vertex + 1 < vertices ) (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>(vertex + 1), i32(2));
    if ( vertex + 7 < vertices ) (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>(vertex + 7), i32(-1));
    if ( vertex + 19 < vertices ) (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>(vertex + 19), i32(4));
  }
  return graph;
}

mm::weighted_graph<u32>
make_cut_graph(usize vertices)
{
  mm::weighted_graph<u32> graph;
  (void)graph.add_vertices(vertices);
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 1) % vertices), u32(3 + vertex % 11));
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 7) % vertices), u32(1 + vertex % 7));
  }
  return graph;
}

using cost_network
    = mm::graph<mm::empty_property, mm::capacity_cost_property<u32, i32>, mm::empty_property, u32, mg::directed_t, mg::parallel_t>;

cost_network
make_cost_network(usize width)
{
  cost_network graph;
  const u32 target = static_cast<u32>(width * 2 + 1);
  (void)graph.add_vertices(target + 1);
  for ( usize index = 0; index < width; ++index ) {
    const u32 left = static_cast<u32>(index + 1);
    const u32 right = static_cast<u32>(width + index + 1);
    (void)graph.add_edge(0u, left, mm::capacity_cost_property<u32, i32>(4, static_cast<i32>(index % 5) - 2));
    (void)graph.add_edge(left, right, mm::capacity_cost_property<u32, i32>(3, static_cast<i32>(index % 7) - 3));
    (void)graph.add_edge(left, static_cast<u32>(width + ((index + 1) % width) + 1), mm::capacity_cost_property<u32, i32>(2, 2));
    (void)graph.add_edge(right, target, mm::capacity_cost_property<u32, i32>(4, 1));
  }
  return graph;
}

mm::weighted_graph<i32>
make_blossom_graph(usize vertices)
{
  mm::weighted_graph<i32> graph;
  (void)graph.add_vertices(vertices);
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 1) % vertices), i32(5 + vertex % 13));
    (void)graph.add_edge(static_cast<u32>(vertex), static_cast<u32>((vertex + 9) % vertices), i32(1 + vertex % 17));
  }
  return graph;
}

};      // namespace

int
main()
{
  constexpr usize vertices = 4096;
  constexpr usize edge_list_vertices = 512;
  constexpr usize matrix_vertices = 256;
  auto topology = make_topology(vertices);
  auto edge_list = make_topology_as<mm::edge_list_graph<>>(edge_list_vertices);
  auto dense = make_topology_as<mm::dense_adjacency_graph<>>(matrix_vertices);
  auto bit = make_topology_as<mm::bit_adjacency_graph<>>(matrix_vertices);
  auto frozen = topology.freeze();
  auto weighted = make_weighted(vertices);
  auto matching_graph = mg::path_graph<>(vertices);
  auto matrix_graph = make_topology(matrix_vertices);
  auto network = make_flow_network(64);
  auto signed_paths = make_signed_paths(96);
  auto cut_graph = make_cut_graph(32);
  auto costs = make_cost_network(20);
  auto blossom = make_blossom_graph(64);
  auto planar = mg::wheel_graph<>(512);

  micron::io::println("=== micron::math::graph BENCH ===");
  micron::io::println("  workload                       size       minimum");

  measure("construct packed adjacency", vertices * 3, 8, [] { return make_topology(vertices).edges_count(); });
  measure("construct edge list", edge_list_vertices * 3, 8,
          [] { return make_topology_as<mm::edge_list_graph<>>(edge_list_vertices).edges_count(); });
  measure("construct dense adjacency", matrix_vertices * 3, 12,
          [] { return make_topology_as<mm::dense_adjacency_graph<>>(matrix_vertices).edges_count(); });
  measure("construct bit adjacency", matrix_vertices * 3, 12,
          [] { return make_topology_as<mm::bit_adjacency_graph<>>(matrix_vertices).edges_count(); });
  measure("freeze packed to CSR", topology.edges_count(), 12, [&] { return topology.freeze().value.edges_count(); });

  measure("find packed adjacency", vertices * 3, 200, [&] { return find_batch(topology, vertices); });
  measure("find edge list", edge_list_vertices * 3, 5, [&] { return find_batch(edge_list, edge_list_vertices); });
  measure("find CSR", vertices * 3, 200, [&] { return find_batch(frozen.value, vertices); });
  measure("find dense adjacency", matrix_vertices * 3, 600, [&] { return find_batch(dense, matrix_vertices); });
  measure("find bit adjacency", matrix_vertices * 3, 600, [&] { return find_batch(bit, matrix_vertices); });

  measure("iterate packed edges", topology.edges_count(), 250, [&] { return traverse_edges(topology); });
  measure("iterate edge-list edges", edge_list.edges_count(), 250, [&] { return traverse_edges(edge_list); });
  measure("iterate CSR edges", frozen.value.edges_count(), 250, [&] { return traverse_edges(frozen.value); });
  measure("iterate dense edges", dense.edges_count(), 250, [&] { return traverse_edges(dense); });
  measure("iterate bit edges", bit.edges_count(), 250, [&] { return traverse_edges(bit); });

  measure("erase/restore packed", vertices, 7, [&] { return erase_restore_batch(topology, vertices); });
  measure("erase/restore edge list", edge_list_vertices, 2, [&] { return erase_restore_batch(edge_list, edge_list_vertices); });
  measure("erase/restore dense", matrix_vertices, 30, [&] { return erase_restore_batch(dense, matrix_vertices); });
  measure("erase/restore bit", matrix_vertices, 30, [&] { return erase_restore_batch(bit, matrix_vertices); });
  measure("BFS", vertices, 40, [&] { return mg::bfs(topology, 0u).order.size(); });
  measure("BFS/CSR", vertices, 40, [&] { return mg::bfs(frozen.value, mm::vertex_id<u32>(0)).order.size(); });
  measure("DFS", vertices, 40, [&] { return mg::dfs(topology, 0u).order.size(); });
  measure("connected components", vertices, 30, [&] { return mg::connected_components(topology).count; });
  measure("Dijkstra", vertices, 10, [&] {
    auto result = mg::dijkstra(weighted, 0u);
    return result.distance[vertices - 1];
  });
  measure("PageRank/20 iterations", vertices * 20, 4, [&] {
    auto result = mg::pagerank(topology, 0.85, 0.0, 20);
    return result.iterations;
  });
  measure("common neighbors", 1, 20000, [&] { return mg::common_neighbors(topology, mm::vertex_id<u32>(17), mm::vertex_id<u32>(118)); });
  measure("dense adjacency conversion", matrix_graph.vertices_count() * matrix_graph.vertices_count(), 8,
          [&] { return mg::adjacency_matrix<>(matrix_graph).value.rows; });
  measure("Hopcroft-Karp", matching_graph.vertices_count(), 10, [&] { return mg::hopcroft_karp(matching_graph).cardinality; });
  measure("Dinic", network.edges_count(), 10, [&] { return mg::dinic(network, mm::vertex_id<u32>(0), mm::vertex_id<u32>(129)).value; });

  measure("Johnson signed APSP", signed_paths.vertices_count() * signed_paths.vertices_count(), 3,
          [&] { return mg::johnson(signed_paths).distance.size(); });
  measure("Floyd-Warshall signed APSP", signed_paths.vertices_count() * signed_paths.vertices_count(), 3,
          [&] { return mg::floyd_warshall(signed_paths).distance.size(); });
  measure("Yen 16 shortest paths", 16, 3,
          [&] { return mg::yen(signed_paths, mm::vertex_id<u32>(0), mm::vertex_id<u32>(95), 16).paths.size(); });
  measure("Gomory-Hu", cut_graph.vertices_count() - 1, 3, [&] { return mg::gomory_hu(cut_graph).tree.edges_count(); });
  measure("min-cost max-flow", costs.edges_count(), 5,
          [&] { return mg::min_cost_max_flow(costs, mm::vertex_id<u32>(0), mm::vertex_id<u32>(41)).achieved_flow; });
  measure("weighted blossom", blossom.vertices_count(), 3, [&] { return mg::maximum_weighted_matching(blossom).cardinality; });
  measure("planarity + rotation", planar.edges_count(), 8, [&] {
    auto result = mg::boyer_myrvold_planarity(planar);
    return result.planar ? result.rotation.size() : usize(0);
  });

  micron::vector<u64> left(4096, u64(0xf0f0f0f0f0f0f0f0ull));
  micron::vector<u64> right(4096, u64(0x5555555555555555ull));
  measure("SIMD common-neighbor kernel", left.size(), 2000,
          [&] { return mg::kernels::common_neighbor_count(left.data(), right.data(), left.size()); });

  micron::io::println("sink=", sink);
  return 1;
}
