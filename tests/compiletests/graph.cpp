//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/graph.hpp"
#include "../../src/io/graph.hpp"
#include "../../src/string/format.hpp"

struct non_default_vertex {
  int value;
  non_default_vertex() = delete;

  constexpr explicit non_default_vertex(int v) : value(v) { }
};

struct non_default_edge {
  int value;
  non_default_edge() = delete;

  constexpr explicit non_default_edge(int v) : value(v) { }
};

static_assert(micron::math::graphs::graph_model<micron::math::graph<>>);
static_assert(micron::is_same_v<typename micron::math::graph<>::storage_type, micron::math::graphs::compact_adjacency_t>);
static_assert(micron::is_same_v<typename micron::math::stable_adjacency_graph<>::storage_type, micron::math::graphs::stable_adjacency_t>);
static_assert(micron::math::graphs::contiguous_slot_graph<micron::math::graph<>>);
static_assert(!micron::math::graphs::contiguous_slot_graph<micron::math::stable_adjacency_graph<>>);
static_assert(micron::math::graphs::matrix_adjacency_graph<micron::math::dense_adjacency_graph<>>);
static_assert(micron::math::graphs::bitset_neighbor_graph<micron::math::bit_adjacency_graph<>>);
static_assert(micron::is_trivially_copyable_v<micron::math::vertex_id<u32>>);
static_assert(micron::is_trivially_copyable_v<micron::math::edge_id<u32>>);

constexpr bool
fixed_graph_works()
{
  micron::math::fixed_graph<4, 4> graph;
  auto edge = graph.add_edge(0u, 2u);
  return edge.inserted() && graph.vertices_count() == 3 && graph.edges_count() == 1 && graph.has_edge(0u, 2u);
}

static_assert(fixed_graph_works());

int
main()
{
  micron::math::graph<> g;
  auto a = g.add_edge(0, 1);
  g += micron::math::edge{ 1u, 2u };
  (void)a;
  for ( auto v : g.vertices() ) (void)v;
  for ( auto e : g.edges() ) (void)e;
  auto traversal = micron::math::graphs::bfs(g, 0u);
  auto depth_first = micron::math::graphs::dfs(g, 0u);
  auto components = micron::math::graphs::connected_components(g);
  auto weak = micron::math::graphs::weakly_connected_components(g);
  auto tarjan = micron::math::graphs::tarjan_scc(g);
  auto cuts = micron::math::graphs::cut_structure(g);
  auto blocks = micron::math::graphs::biconnected_components(g);
  auto bipartite = micron::math::graphs::bipartite_test(g);
  auto paths = micron::math::graphs::dijkstra(g, 0u);
  auto frozen = g.freeze();
  auto bidirectional_frozen = g.freeze_bidirectional();
  auto frozen_bfs = micron::math::graphs::bfs(frozen.value, micron::math::vertex_id<u32>(0));
  auto thawed = frozen.value.thaw();
  auto adjacency = micron::math::graphs::adjacency_matrix<>(g);
  auto reverse = micron::math::graphs::reverse(g);
  auto filtered = micron::math::graphs::induced_subgraph(g, [](auto vertex) { return vertex.value != 1u; });
  auto matrix_view = micron::math::as_graph_view(adjacency.value);
  auto reverse_bfs = micron::math::graphs::bfs(reverse, 0u);
  auto filtered_bfs = micron::math::graphs::bfs(filtered, 0u);
  auto matrix_bfs = micron::math::graphs::bfs(matrix_view, 0u);
  auto rendered = micron::format::format("{:s}", g);
  auto generated = micron::math::graphs::path_graph<>(8);
  auto product = micron::math::graphs::cartesian_product(generated, generated);
  micron::math::weighted_digraph<u32> network;
  (void)network.add_edge(0, 1, 4u);
  (void)network.add_edge(1, 2, 3u);
  auto flow = micron::math::graphs::dinic(network, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(2));
  auto augmenting_flow = micron::math::graphs::edmonds_karp(network, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(2));
  auto preflow = micron::math::graphs::push_relabel(network, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(2));
  auto matching = micron::math::graphs::hopcroft_karp(generated);
  auto rank = micron::math::graphs::pagerank(generated);
  auto authority = micron::math::graphs::hits(generated);
  auto centrality = micron::math::graphs::betweenness_centrality(generated);
  auto eigenvector = micron::math::graphs::eigenvector_centrality(generated);
  auto katz = micron::math::graphs::katz_centrality(generated);
  auto cores = micron::math::graphs::k_core_vertices(generated, 1);
  auto coloring = micron::math::graphs::greedy_coloring(generated);
  auto independent = micron::math::graphs::maximal_independent_set(generated);
  auto dominating = micron::math::graphs::greedy_dominating_set(generated);
  auto cover = micron::math::graphs::approximate_vertex_cover(generated);
  auto cut = micron::math::graphs::max_cut(generated);
  auto communities = micron::math::graphs::label_propagation(generated);
  auto louvain = micron::math::graphs::louvain(generated);
  auto leiden = micron::math::graphs::leiden(generated);
  auto greedy_communities = micron::math::graphs::greedy_modularity_communities(generated);
  auto quality = micron::math::graphs::modularity(generated, communities.community);
  auto euler = micron::math::graphs::eulerian_path(generated);
  auto hash = micron::math::graphs::weisfeiler_lehman_hash(generated);
  auto edge_text = micron::io::graph::edge_list(generated);
  auto parsed = micron::io::graph::parse_edge_list<>(edge_text);
  auto adjacency_text = micron::io::graph::adjacency_list(generated);
  auto parsed_adjacency = micron::io::graph::parse_adjacency_list<>(adjacency_text.data(), adjacency_text.size());
  auto market_text = micron::io::graph::matrix_market(generated);
  auto parsed_market = micron::io::graph::parse_matrix_market<>(market_text.data(), market_text.size());
  auto dimacs_text = micron::io::graph::dimacs(generated);
  auto parsed_dimacs = micron::io::graph::parse_dimacs<>(dimacs_text.data(), dimacs_text.size());
  auto binary = micron::io::graph::binary(generated);
  auto decoded = micron::io::graph::parse_binary<>(binary);
  micron::io::graph::native_property_codec native_codec;
  auto weighted_binary = micron::io::graph::binary(network, native_codec);
  auto weighted_decoded = micron::io::graph::parse_binary<micron::math::weighted_digraph<u32>>(weighted_binary, native_codec);
  if ( false ) {
    micron::io::path_t path;
    (void)micron::io::graph::write_edge_list(path, generated);
    (void)micron::io::graph::write_adjacency_list(path, generated);
    (void)micron::io::graph::write_matrix_market(path, generated);
    (void)micron::io::graph::write_dimacs(path, generated);
    (void)micron::io::graph::write_binary(path, generated);
    (void)micron::io::graph::read_edge_list<>(path);
    (void)micron::io::graph::read_adjacency_list<>(path);
    (void)micron::io::graph::read_matrix_market<>(path);
    (void)micron::io::graph::read_dimacs<>(path);
    (void)micron::io::graph::read_binary<>(path);
  }
  (void)traversal;
  (void)depth_first;
  (void)components;
  (void)weak;
  (void)tarjan;
  (void)cuts;
  (void)blocks;
  (void)bipartite;
  (void)paths;
  (void)frozen;
  (void)bidirectional_frozen;
  (void)frozen_bfs;
  (void)thawed;
  (void)adjacency;
  (void)reverse;
  (void)filtered;
  (void)matrix_view;
  (void)reverse_bfs;
  (void)filtered_bfs;
  (void)matrix_bfs;
  (void)rendered;
  (void)generated;
  (void)product;
  (void)flow;
  (void)augmenting_flow;
  (void)preflow;
  (void)matching;
  (void)rank;
  (void)authority;
  (void)centrality;
  (void)eigenvector;
  (void)katz;
  (void)cores;
  (void)coloring;
  (void)independent;
  (void)dominating;
  (void)cover;
  (void)cut;
  (void)communities;
  (void)louvain;
  (void)leiden;
  (void)greedy_communities;
  (void)quality;
  (void)euler;
  (void)hash;
  (void)parsed;
  (void)parsed_adjacency;
  (void)parsed_market;
  (void)parsed_dimacs;
  (void)decoded;
  (void)weighted_decoded;

  micron::math::weighted_digraph<int> weighted;
  (void)weighted.add_edge(0, 1, 7);
  auto weighted_paths = micron::math::graphs::dijkstra(weighted, 0u);
  auto astar = micron::math::graphs::astar(weighted, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(1),
                                           [](auto, auto) { return 0; });
  auto spfa = micron::math::graphs::spfa(weighted, micron::math::vertex_id<u32>(0));
  auto all_pairs = micron::math::graphs::floyd_warshall(weighted);
  micron::math::graphs::johnson_workspace<u32, int> johnson_storage;
  auto johnson = micron::math::graphs::johnson(weighted, micron::math::graphs::intrinsic_edge_weight{}, johnson_storage);
  micron::math::graphs::yen_workspace<u32, int> yen_storage;
  auto yen = micron::math::graphs::yen(weighted, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(1), 3, yen_storage,
                                       micron::math::graphs::intrinsic_edge_weight{});
  auto forest = micron::math::graphs::kruskal(weighted);
  auto prim = micron::math::graphs::prim(weighted);
  auto boruvka = micron::math::graphs::boruvka(weighted);
  auto min_cut = micron::math::graphs::stoer_wagner(weighted);
  auto edge_connectivity = micron::math::graphs::edge_connectivity(weighted);
  auto node_connectivity = micron::math::graphs::node_connectivity(weighted);
  (void)weighted_paths;
  (void)astar;
  (void)spfa;
  (void)all_pairs;
  (void)johnson;
  (void)yen;
  (void)forest;
  (void)prim;
  (void)boruvka;
  (void)min_cut;
  (void)edge_connectivity;
  (void)node_connectivity;

  micron::math::digraph<> dag;
  (void)dag.add_edge(0, 1);
  (void)dag.add_edge(1, 2);
  auto generations = micron::math::graphs::topological_generations(dag);
  auto dag_shortest = micron::math::graphs::dag_shortest_paths(dag, micron::math::vertex_id<u32>(0));
  auto dag_longest = micron::math::graphs::dag_longest_paths(dag, micron::math::vertex_id<u32>(0));
  auto closure = micron::math::graphs::transitive_closure(dag);
  auto reduction = micron::math::graphs::transitive_reduction(dag);
  auto dominators = micron::math::graphs::dominators(dag, micron::math::vertex_id<u32>(0));
  auto condensation = micron::math::graphs::condensation(dag);
  auto arborescence
      = micron::math::graphs::directed_arborescence(dag, micron::math::vertex_id<u32>(0), micron::math::graphs::intrinsic_edge_weight{});
  (void)generations;
  (void)dag_shortest;
  (void)dag_longest;
  (void)closure;
  (void)reduction;
  (void)dominators;
  (void)condensation;
  (void)arborescence;

  auto degree = micron::math::graphs::degree_matrix<>(generated);
  auto laplacian = micron::math::graphs::laplacian_matrix<>(generated);
  auto incidence = micron::math::graphs::incidence_matrix<>(generated);
  auto sparse = micron::math::graphs::sparse_adjacency_matrix<>(generated);
  auto sparse_columns = micron::math::graphs::sparse_adjacency_matrix_csc<>(generated);
  auto matrix_round_trip = micron::math::graphs::from_adjacency_matrix(adjacency.value);
  auto sparse_round_trip = micron::math::graphs::from_sparse_adjacency_matrix<>(sparse.value);
  auto sparse_column_round_trip = micron::math::graphs::from_sparse_adjacency_matrix<>(sparse_columns.value);
  (void)degree;
  (void)laplacian;
  (void)incidence;
  (void)sparse;
  (void)sparse_columns;
  (void)matrix_round_trip;
  (void)sparse_round_trip;
  (void)sparse_column_round_trip;

  micron::vector<int> exterior(g.vertex_slots(), 0);
  auto exterior_map = micron::math::graphs::property_map(exterior);
  exterior_map[micron::math::vertex_id<u32>(0)] = 3;
  auto projection = micron::math::graphs::project_property([](auto id) { return id.value; });
  (void)projection[micron::math::vertex_id<u32>(0)];

  micron::math::compact_adjacency_graph<> compact_storage;
  micron::math::edge_list_graph<> edge_storage;
  micron::math::csr_graph<> csr_storage;
  micron::math::bidirectional_csr_graph<> bidirectional_storage;
  micron::math::dense_adjacency_graph<> dense_storage;
  micron::math::bit_adjacency_graph<> bit_storage;
  auto edge_conversion = micron::math::graphs::to_edge_list(generated);
  auto dense_conversion = micron::math::graphs::to_dense_adjacency(generated);
  auto bit_conversion = micron::math::graphs::to_bit_adjacency(generated);
  auto stable_conversion = micron::math::graphs::to_stable_adjacency(frozen.value);
  auto compact_conversion = micron::math::graphs::to_compact_adjacency(stable_conversion.value);
  (void)compact_storage;
  (void)edge_storage;
  (void)csr_storage;
  (void)bidirectional_storage;
  (void)dense_storage;
  (void)bit_storage;
  (void)edge_conversion;
  (void)dense_conversion;
  (void)bit_conversion;
  (void)stable_conversion;
  (void)compact_conversion;

  micron::math::weighted_graph<u32> capacities;
  (void)capacities.add_edge(0, 1, 4u);
  (void)capacities.add_edge(1, 2, 3u);
  (void)capacities.add_edge(0, 2, 2u);
  micron::math::graphs::gomory_hu_workspace<u32, u32> cut_storage;
  auto cut_tree = micron::math::graphs::gomory_hu(capacities, micron::math::graphs::intrinsic_edge_weight{}, cut_storage);
  micron::math::graphs::weighted_matching_workspace<u32, u32> blossom_storage;
  auto weighted_matching = micron::math::graphs::maximum_weighted_matching(
      capacities, blossom_storage, micron::math::graphs::matching_objective::maximum_cardinality_then_weight);
  micron::math::graphs::weighted_matching_workspace<u32, i32> cardinality_storage;
  auto cardinality_matching = micron::math::graphs::maximum_cardinality_matching(generated, cardinality_storage);
  micron::math::graphs::planarity_workspace<u32> planar_storage;
  auto embedding = micron::math::graphs::boyer_myrvold_planarity(generated, planar_storage);
  auto valid_embedding = micron::math::graphs::validate_planar_embedding(generated, embedding);
  (void)cut_tree;
  (void)weighted_matching;
  (void)cardinality_matching;
  (void)valid_embedding;

  using cost_network
      = micron::math::graph<micron::math::empty_property, micron::math::capacity_cost_property<u32, i32>, micron::math::empty_property, u32,
                            micron::math::graphs::directed_t, micron::math::graphs::parallel_t>;
  cost_network costs;
  (void)costs.add_edge(0, 1, micron::math::capacity_cost_property<u32, i32>(3, -1));
  (void)costs.add_edge(1, 2, micron::math::capacity_cost_property<u32, i32>(3, 2));
  micron::math::graphs::min_cost_flow_workspace<u32, u32, i32> cost_storage;
  auto requested_flow = micron::math::graphs::min_cost_flow(costs, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(2), u32(2),
                                                            micron::math::graphs::intrinsic_edge_capacity{},
                                                            micron::math::graphs::intrinsic_edge_cost{}, cost_storage);
  auto maximum_cost_flow = micron::math::graphs::min_cost_max_flow(costs, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(2),
                                                                   micron::math::graphs::intrinsic_edge_capacity{},
                                                                   micron::math::graphs::intrinsic_edge_cost{}, cost_storage);
  micron::vector<i32> supplies{ 1, 0, -1 };
  auto circulation = micron::math::graphs::min_cost_circulation(costs, supplies, micron::math::graphs::intrinsic_edge_capacity{},
                                                                micron::math::graphs::intrinsic_edge_cost{}, cost_storage);
  const bool requested_verified
      = micron::math::graphs::verify_min_cost_flow(costs, micron::math::vertex_id<u32>(0), micron::math::vertex_id<u32>(2), requested_flow);
  const bool circulation_verified = micron::math::graphs::verify_min_cost_flow(costs, supplies, circulation);
  (void)maximum_cost_flow;
  (void)requested_verified;
  (void)circulation_verified;

  micron::math::graph<non_default_vertex> explicit_vertices;
  auto v = explicit_vertices.add_vertex(non_default_vertex(4));
  (void)v;
  micron::math::graph<micron::math::empty_property, non_default_edge> explicit_edges;
  (void)explicit_edges.add_vertices(2);
  auto required = explicit_edges.add_edge(0u, 1u);
  (void)required;
  return 1;
}
