//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/bipartite.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/max_cardinality_matching.hpp>
#include <boost/graph/maximum_weighted_matching.hpp>
#include <boost/graph/page_rank.hpp>
#include <boost/graph/push_relabel_max_flow.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include "../../src/math/graph.hpp"

namespace mm = micron::math;
namespace mg = micron::math::graphs;

namespace
{

struct oracle_edge {
  std::int64_t weight{};
};

using oracle_digraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, boost::no_property, oracle_edge>;
using oracle_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, oracle_edge>;
using oracle_weighted_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property,
                                                    boost::property<boost::edge_weight_t, std::int64_t>>;
using oracle_planar_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property,
                                                  boost::property<boost::edge_index_t, std::size_t>>;

struct fixed_rng {
  std::uint64_t state;

  std::uint64_t
  next()
  {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }

  std::size_t
  bounded(std::size_t bound)
  {
    return static_cast<std::size_t>(next() % bound);
  }
};

int failures = 0;

void
check(bool condition, std::string_view family, std::uint64_t seed, std::size_t a = 0, std::size_t b = 0)
{
  if ( condition ) return;
  if ( failures < 24 )
    std::cerr << "graph oracle mismatch: " << family << " seed=0x" << std::hex << seed << std::dec << " a=" << a << " b=" << b << '\n';
  ++failures;
}

template<class Subject>
oracle_digraph
make_directed_oracle(const Subject &subject)
{
  oracle_digraph oracle(subject.vertex_slots());
  for ( auto edge : subject.edges() )
    boost::add_edge(static_cast<std::size_t>(edge.source.value), static_cast<std::size_t>(edge.target.value),
                    oracle_edge{ edge.property.weight }, oracle);
  return oracle;
}

template<class Subject>
oracle_graph
make_undirected_oracle(const Subject &subject)
{
  oracle_graph oracle(subject.vertex_slots());
  for ( auto edge : subject.edges() )
    boost::add_edge(static_cast<std::size_t>(edge.source.value), static_cast<std::size_t>(edge.target.value),
                    oracle_edge{ edge.property.weight }, oracle);
  return oracle;
}

template<class Oracle> struct bfs_distance_visitor: boost::default_bfs_visitor {
  std::vector<std::size_t> *distance;

  explicit bfs_distance_visitor(std::vector<std::size_t> &values) : distance(&values) { }

  template<class Edge>
  void
  tree_edge(Edge edge, const Oracle &graph) const
  {
    const auto source = static_cast<std::size_t>(boost::source(edge, graph));
    const auto target = static_cast<std::size_t>(boost::target(edge, graph));
    (*distance)[target] = (*distance)[source] + 1;
  }
};

template<class A, class B>
bool
same_partition(const A &a, const B &b, std::size_t count)
{
  for ( std::size_t u = 0; u < count; ++u )
    for ( std::size_t v = 0; v < count; ++v )
      if ( (a[u] == a[v]) != (b[u] == b[v]) ) return false;
  return true;
}

void
directed_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 2 + random.bounded(17);
  mm::weighted_digraph<i64> subject;
  (void)subject.add_vertices(n);

  for ( std::size_t u = 0; u < n; ++u )
    (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>((u + 1) % n), i64(1 + random.bounded(13)));
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = 0; v < n; ++v )
      if ( u != v && random.bounded(100) < 22 )
        (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v), i64(1 + random.bounded(31)));

  for ( auto edge : subject.edges() )
    if ( static_cast<std::size_t>(edge.id.value) >= n && edge.id.value % 11 == 7 ) (void)subject.remove_edge(edge.id);

  const oracle_digraph oracle = make_directed_oracle(subject);
  const auto weight = boost::get(&oracle_edge::weight, oracle);
  const auto vertex_index = boost::get(boost::vertex_index, oracle);
  const i64 infinity = std::numeric_limits<i64>::max() / 4;
  const std::size_t unreachable = std::numeric_limits<std::size_t>::max();
  const auto floyd = mg::floyd_warshall(subject);

  check(floyd.status == mg::algorithm_status::ok, "floyd status", seed);
  for ( std::size_t source = 0; source < n; ++source ) {
    std::vector<i64> oracle_distance(n, infinity);
    boost::dijkstra_shortest_paths(oracle, boost::vertex(source, oracle),
                                   boost::weight_map(weight)
                                       .distance_map(boost::make_iterator_property_map(oracle_distance.begin(), vertex_index))
                                       .distance_inf(infinity)
                                       .distance_zero(i64(0)));

    std::vector<std::size_t> oracle_depth(n, unreachable);
    oracle_depth[source] = 0;
    boost::breadth_first_search(oracle, boost::vertex(source, oracle), boost::visitor(bfs_distance_visitor<oracle_digraph>(oracle_depth)));

    const auto dijkstra = mg::dijkstra(subject, static_cast<u32>(source));
    const auto bellman = mg::bellman_ford(subject, mm::vertex_id<u32>(static_cast<u32>(source)));
    const auto spfa = mg::spfa(subject, mm::vertex_id<u32>(static_cast<u32>(source)));
    const auto breadth = mg::unweighted_shortest_paths(subject, static_cast<u32>(source));
    check(dijkstra.status == mg::algorithm_status::ok, "dijkstra status", seed, source);
    check(bellman.status == mg::algorithm_status::ok, "bellman-ford status", seed, source);
    check(spfa.status == mg::algorithm_status::ok, "spfa status", seed, source);

    for ( std::size_t target = 0; target < n; ++target ) {
      const bool oracle_reached = oracle_distance[target] != infinity;
      const auto vertex = mm::vertex_id<u32>(static_cast<u32>(target));
      check(dijkstra.contains(vertex) == oracle_reached, "dijkstra reachability", seed, source, target);
      check(bellman.contains(vertex) == oracle_reached, "bellman-ford reachability", seed, source, target);
      check(spfa.contains(vertex) == oracle_reached, "spfa reachability", seed, source, target);
      check(floyd.contains(mm::vertex_id<u32>(static_cast<u32>(source)), vertex) == oracle_reached, "floyd reachability", seed, source,
            target);
      if ( oracle_reached ) {
        check(dijkstra.distance.data()[target] == oracle_distance[target], "dijkstra distance", seed, source, target);
        check(bellman.distance.data()[target] == oracle_distance[target], "bellman-ford distance", seed, source, target);
        check(spfa.distance.data()[target] == oracle_distance[target], "spfa distance", seed, source, target);
        check(floyd.distance.data()[source * n + target] == oracle_distance[target], "floyd distance", seed, source, target);
      }
      const bool breadth_reached = oracle_depth[target] != unreachable;
      check(breadth.contains(vertex) == breadth_reached, "bfs reachability", seed, source, target);
      if ( breadth_reached ) check(breadth.distance.data()[target] == oracle_depth[target], "bfs depth", seed, source, target);
    }
  }

  std::vector<int> oracle_component(n);
  const auto oracle_count = static_cast<std::size_t>(boost::strong_components(oracle, oracle_component.data()));
  const auto kosaraju = mg::kosaraju_scc(subject);
  const auto tarjan = mg::tarjan_scc(subject);
  check(kosaraju.count == oracle_count, "kosaraju count", seed);
  check(tarjan.count == oracle_count, "tarjan count", seed);
  check(same_partition(kosaraju.component, oracle_component, n), "kosaraju partition", seed);
  check(same_partition(tarjan.component, oracle_component, n), "tarjan partition", seed);

  std::vector<double> oracle_rank(n);
  auto rank_map = boost::make_iterator_property_map(oracle_rank.begin(), vertex_index);
  boost::graph::page_rank(oracle, rank_map, boost::graph::n_iterations(400), 0.85);
  double rank_sum = 0;
  for ( double value : oracle_rank ) rank_sum += value;
  const auto rank = mg::pagerank(subject, 0.85, 1e-12, 1000);
  check(rank.status == mg::algorithm_status::ok, "pagerank convergence", seed);
  for ( std::size_t vertex = 0; vertex < n; ++vertex )
    check(std::abs(rank.score.data()[vertex] - oracle_rank[vertex] / rank_sum) < 1e-8, "pagerank", seed, vertex);
}

void
negative_path_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 3 + random.bounded(15);
  mm::weighted_digraph<i64> subject;
  (void)subject.add_vertices(n);
  for ( std::size_t u = 0; u + 1 < n; ++u )
    (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(u + 1), i64(random.bounded(24)) - 8);
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = u + 2; v < n; ++v )
      if ( random.bounded(100) < 23 ) (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v), i64(random.bounded(31)) - 10);

  oracle_digraph oracle = make_directed_oracle(subject);
  const auto weight = boost::get(&oracle_edge::weight, oracle);
  const auto vertex_index = boost::get(boost::vertex_index, oracle);
  const i64 infinity = std::numeric_limits<i64>::max();
  const auto floyd = mg::floyd_warshall(subject);
  check(floyd.status == mg::algorithm_status::ok, "signed floyd status", seed);
  for ( std::size_t source = 0; source < n; ++source ) {
    std::vector<i64> oracle_distance(n, infinity);
    const bool oracle_ok
        = boost::bellman_ford_shortest_paths(oracle, n,
                                             boost::weight_map(weight)
                                                 .distance_map(boost::make_iterator_property_map(oracle_distance.begin(), vertex_index))
                                                 .root_vertex(boost::vertex(source, oracle)));
    const auto bellman = mg::bellman_ford(subject, mm::vertex_id<u32>(static_cast<u32>(source)));
    const auto spfa = mg::spfa(subject, mm::vertex_id<u32>(static_cast<u32>(source)));
    check(oracle_ok, "boost signed bellman-ford", seed, source);
    check(bellman.status == mg::algorithm_status::ok, "signed bellman-ford status", seed, source);
    check(spfa.status == mg::algorithm_status::ok, "signed spfa status", seed, source);
    for ( std::size_t target = 0; target < n; ++target ) {
      const bool oracle_reached = oracle_distance[target] != infinity;
      const auto vertex = mm::vertex_id<u32>(static_cast<u32>(target));
      check(bellman.contains(vertex) == oracle_reached, "signed bellman-ford reachability", seed, source, target);
      check(spfa.contains(vertex) == oracle_reached, "signed spfa reachability", seed, source, target);
      check(floyd.contains(mm::vertex_id<u32>(static_cast<u32>(source)), vertex) == oracle_reached, "signed floyd reachability", seed,
            source, target);
      if ( oracle_reached ) {
        check(bellman.distance.data()[target] == oracle_distance[target], "signed bellman-ford distance", seed, source, target);
        check(spfa.distance.data()[target] == oracle_distance[target], "signed spfa distance", seed, source, target);
        check(floyd.distance.data()[source * n + target] == oracle_distance[target], "signed floyd distance", seed, source, target);
      }
    }
  }
}

void
negative_cycle_oracle()
{
  mm::weighted_digraph<i64> subject;
  (void)subject.add_edge(0u, 1u, i64(2));
  (void)subject.add_edge(1u, 2u, i64(-5));
  (void)subject.add_edge(2u, 1u, i64(1));
  (void)subject.add_edge(2u, 3u, i64(2));
  oracle_digraph oracle = make_directed_oracle(subject);
  std::vector<i64> distance(subject.vertices_count(), std::numeric_limits<i64>::max());
  const bool oracle_ok = boost::bellman_ford_shortest_paths(
      oracle, subject.vertices_count(),
      boost::weight_map(boost::get(&oracle_edge::weight, oracle))
          .distance_map(boost::make_iterator_property_map(distance.begin(), boost::get(boost::vertex_index, oracle)))
          .root_vertex(boost::vertex(0, oracle)));
  check(!oracle_ok, "boost negative-cycle detection", 0xc1c1'e5f0'0d12'beefULL);
  check(mg::bellman_ford(subject, mm::vertex_id<u32>(0)).status == mg::algorithm_status::negative_cycle,
        "bellman-ford negative-cycle detection", 0xc1c1'e5f0'0d12'beefULL);
  check(mg::spfa(subject, mm::vertex_id<u32>(0)).status == mg::algorithm_status::negative_cycle, "spfa negative-cycle detection",
        0xc1c1'e5f0'0d12'beefULL);
  check(mg::floyd_warshall(subject).status == mg::algorithm_status::negative_cycle, "floyd negative-cycle detection",
        0xc1c1'e5f0'0d12'beefULL);
}

void
undirected_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 2 + random.bounded(19);
  mm::weighted_graph<i64> subject;
  (void)subject.add_vertices(n);
  if ( seed & 1 )
    for ( std::size_t u = 1; u < n; ++u ) (void)subject.add_edge(static_cast<u32>(u - 1), static_cast<u32>(u), i64(1 + random.bounded(31)));
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = u + 1; v < n; ++v )
      if ( random.bounded(100) < 18 ) (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v), i64(1 + random.bounded(47)));

  const oracle_graph oracle = make_undirected_oracle(subject);
  std::vector<int> oracle_component(n);
  const auto oracle_count = static_cast<std::size_t>(boost::connected_components(oracle, oracle_component.data()));
  const auto components = mg::connected_components(subject);
  check(components.count == oracle_count, "connected component count", seed);
  check(same_partition(components.component, oracle_component, n), "connected component partition", seed);

  std::vector<oracle_graph::edge_descriptor> oracle_tree;
  boost::kruskal_minimum_spanning_tree(oracle, std::back_inserter(oracle_tree),
                                       boost::weight_map(boost::get(&oracle_edge::weight, oracle)));
  i64 oracle_weight = 0;
  for ( auto edge : oracle_tree ) oracle_weight += boost::get(&oracle_edge::weight, oracle, edge);
  const auto kruskal = mg::kruskal(subject);
  const auto prim = mg::prim(subject);
  const auto boruvka = mg::boruvka(subject);
  check(kruskal.weight == oracle_weight, "kruskal weight", seed);
  check(prim.weight == oracle_weight, "prim weight", seed);
  check(boruvka.weight == oracle_weight, "boruvka weight", seed);
  check(kruskal.components == oracle_count, "kruskal components", seed);
  check(prim.components == oracle_count, "prim components", seed);
  check(boruvka.components == oracle_count, "boruvka components", seed);

  const bool oracle_bipartite = boost::is_bipartite(oracle);
  check(mg::is_bipartite(subject) == oracle_bipartite, "bipartite", seed);

  std::vector<oracle_graph::vertex_descriptor> oracle_articulation;
  boost::articulation_points(oracle, std::back_inserter(oracle_articulation));
  std::sort(oracle_articulation.begin(), oracle_articulation.end());
  const auto articulation = mg::articulation_points(subject);
  check(articulation.size() == oracle_articulation.size(), "articulation count", seed);
  if ( articulation.size() == oracle_articulation.size() )
    for ( std::size_t i = 0; i < oracle_articulation.size(); ++i )
      check(static_cast<std::size_t>(articulation.data()[i].value) == oracle_articulation[i], "articulation vertex", seed, i);
}

void
matching_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t left = 1 + random.bounded(10);
  const std::size_t right = 1 + random.bounded(10);
  const std::size_t n = left + right;
  mm::graph<> subject;
  (void)subject.add_vertices(n);
  oracle_graph oracle(n);
  for ( std::size_t u = 0; u < left; ++u )
    for ( std::size_t v = left; v < n; ++v )
      if ( random.bounded(100) < 37 ) {
        (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v));
        boost::add_edge(u, v, oracle_edge{}, oracle);
      }

  std::vector<oracle_graph::vertex_descriptor> mate(n);
  auto mate_map = boost::make_iterator_property_map(mate.begin(), boost::get(boost::vertex_index, oracle));
  const bool oracle_valid = boost::checked_edmonds_maximum_cardinality_matching(oracle, mate_map);
  const auto oracle_cardinality = static_cast<std::size_t>(boost::matching_size(oracle, mate_map));
  const auto matching = mg::hopcroft_karp(subject);
  check(oracle_valid, "boost matching certificate", seed);
  check(matching.status == mg::algorithm_status::ok, "hopcroft-karp status", seed);
  check(mg::validate_matching(subject, matching), "hopcroft-karp certificate", seed);
  check(matching.cardinality == oracle_cardinality, "hopcroft-karp cardinality", seed);
}

void
weighted_matching_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 8 + random.bounded(25);
  mm::weighted_graph<i64> subject;
  (void)subject.add_vertices(n);
  oracle_weighted_graph oracle(n);
  std::vector<i64> weights(n * n);
  i64 largest_magnitude = 0;
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = u + 1; v < n; ++v ) {
      if ( random.bounded(100) >= 31 ) continue;
      const i64 value = static_cast<i64>(random.bounded(101)) - 45;
      (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v), value);
      const auto inserted = boost::add_edge(u, v, oracle).first;
      boost::put(boost::edge_weight, oracle, inserted, value);
      weights[u * n + v] = weights[v * n + u] = value;
      const i64 magnitude = value < 0 ? -value : value;
      if ( largest_magnitude < magnitude ) largest_magnitude = magnitude;
    }

  std::vector<oracle_weighted_graph::vertex_descriptor> mate(n);
  auto mate_map = boost::make_iterator_property_map(mate.begin(), boost::get(boost::vertex_index, oracle));
  boost::maximum_weighted_matching(oracle, mate_map);
  i64 oracle_weight = 0;
  std::size_t oracle_cardinality = 0;
  for ( std::size_t vertex = 0; vertex < n; ++vertex ) {
    const auto partner = mate[vertex];
    if ( partner == boost::graph_traits<oracle_weighted_graph>::null_vertex() ) continue;
    check(partner < n && mate[partner] == vertex, "Boost weighted matching certificate", seed, vertex);
    if ( vertex < partner ) {
      ++oracle_cardinality;
      oracle_weight += weights[vertex * n + partner];
    }
  }
  const auto actual = mg::maximum_weighted_matching(subject);
  check(actual.status == mg::algorithm_status::ok, "weighted blossom status", seed);
  check(actual.total_weight == oracle_weight, "weighted blossom weight", seed);

  const i64 bonus = largest_magnitude * static_cast<i64>(n + 1) + 1;
  for ( auto edge : boost::make_iterator_range(boost::edges(oracle)) ) {
    const std::size_t u = boost::source(edge, oracle);
    const std::size_t v = boost::target(edge, oracle);
    boost::put(boost::edge_weight, oracle, edge, weights[u * n + v] + bonus);
  }
  boost::maximum_weighted_matching(oracle, mate_map);
  oracle_weight = 0;
  oracle_cardinality = 0;
  for ( std::size_t vertex = 0; vertex < n; ++vertex ) {
    const auto partner = mate[vertex];
    if ( partner != boost::graph_traits<oracle_weighted_graph>::null_vertex() && vertex < partner ) {
      ++oracle_cardinality;
      oracle_weight += weights[vertex * n + partner];
    }
  }
  const auto cardinality_first = mg::maximum_weighted_matching(subject, mg::matching_objective::maximum_cardinality_then_weight);
  check(cardinality_first.status == mg::algorithm_status::ok, "cardinality-first blossom status", seed);
  check(cardinality_first.cardinality == oracle_cardinality, "cardinality-first blossom cardinality", seed);
  check(cardinality_first.total_weight == oracle_weight, "cardinality-first blossom weight", seed);
}

void
planarity_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 3 + random.bounded(22);
  using subject_type
      = mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t, mg::parallel_t, mg::allow_loops_t>;
  subject_type subject;
  (void)subject.add_vertices(n);
  oracle_planar_graph oracle(n);
  std::size_t edge_index = 0;
  auto add = [&](std::size_t u, std::size_t v) {
    (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v));
    const auto edge = boost::add_edge(u, v, oracle).first;
    boost::put(boost::edge_index, oracle, edge, edge_index++);
  };
  const std::size_t density = 10 + random.bounded(36);
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = u + 1; v < n; ++v )
      if ( random.bounded(100) < density ) add(u, v);
  if ( (seed & 3) == 0 ) {
    const std::size_t u = random.bounded(n);
    const std::size_t v = random.bounded(n);
    add(u, u);
    if ( u != v ) {
      add(u, v);
      add(u, v);
    }
  }

  using edge_type = oracle_planar_graph::edge_descriptor;
  using embedding_storage = std::vector<std::vector<edge_type>>;
  embedding_storage embedding_rows(n);
  auto embedding = boost::make_iterator_property_map(embedding_rows.begin(), boost::get(boost::vertex_index, oracle));
  std::vector<edge_type> witness;
  const bool oracle_planar
      = boost::boyer_myrvold_planarity_test(boost::boyer_myrvold_params::graph = oracle, boost::boyer_myrvold_params::embedding = embedding,
                                            boost::boyer_myrvold_params::kuratowski_subgraph = std::back_inserter(witness));
  const auto actual = mg::boyer_myrvold_planarity(subject);
  check(actual.status == mg::algorithm_status::ok, "planarity status", seed);
  check(actual.planar == oracle_planar, "planarity decision", seed, n, subject.edges_count());
  if ( oracle_planar ) {
    std::size_t oracle_incidences = 0;
    for ( const auto &row : embedding_rows ) oracle_incidences += row.size();
    check(oracle_incidences == edge_index * 2, "Boost embedding incidences", seed);
    check(mg::validate_planar_embedding(subject, actual), "micron embedding certificate", seed);
  } else {
    check(!witness.empty(), "Boost Kuratowski certificate", seed);
    check(mg::validate_kuratowski_witness(subject, actual.kuratowski_edges), "micron Kuratowski certificate", seed);
  }
}

using flow_traits = boost::adjacency_list_traits<boost::vecS, boost::vecS, boost::directedS>;
using flow_property = boost::property<
    boost::edge_capacity_t, i64,
    boost::property<boost::edge_residual_capacity_t, i64, boost::property<boost::edge_reverse_t, flow_traits::edge_descriptor>>>;
using oracle_flow_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, flow_property>;

void
add_flow_arc(oracle_flow_graph &graph, std::size_t source, std::size_t target, i64 capacity)
{
  auto capacity_map = boost::get(boost::edge_capacity, graph);
  auto reverse_map = boost::get(boost::edge_reverse, graph);
  const auto forward = boost::add_edge(source, target, graph).first;
  const auto reverse = boost::add_edge(target, source, graph).first;
  capacity_map[forward] = capacity;
  capacity_map[reverse] = 0;
  reverse_map[forward] = reverse;
  reverse_map[reverse] = forward;
}

void
flow_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 3 + random.bounded(12);
  mm::weighted_digraph<i64> subject;
  (void)subject.add_vertices(n);
  for ( std::size_t u = 1; u < n; ++u ) (void)subject.add_edge(static_cast<u32>(u - 1), static_cast<u32>(u), i64(1 + random.bounded(29)));
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = 0; v < n; ++v )
      if ( u != v && random.bounded(100) < 17 )
        (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v), i64(1 + random.bounded(37)));

  oracle_flow_graph oracle(n);
  for ( auto edge : subject.edges() )
    add_flow_arc(oracle, static_cast<std::size_t>(edge.source.value), static_cast<std::size_t>(edge.target.value), edge.property.weight);
  const i64 oracle_value = boost::push_relabel_max_flow(oracle, 0, n - 1);
  const auto source = mm::vertex_id<u32>(0);
  const auto sink = mm::vertex_id<u32>(static_cast<u32>(n - 1));
  const auto dinic = mg::dinic(subject, source, sink);
  const auto edmonds_karp = mg::edmonds_karp(subject, source, sink);
  const auto push_relabel = mg::push_relabel(subject, source, sink);
  check(dinic.value == oracle_value, "dinic value", seed);
  check(edmonds_karp.value == oracle_value, "edmonds-karp value", seed);
  check(push_relabel.value == oracle_value, "push-relabel value", seed);
  check(mg::verify_flow(subject, source, sink, dinic, mg::intrinsic_edge_weight{}), "dinic certificate", seed);
  check(mg::verify_flow(subject, source, sink, edmonds_karp, mg::intrinsic_edge_weight{}), "edmonds-karp certificate", seed);
  check(mg::verify_flow(subject, source, sink, push_relabel, mg::intrinsic_edge_weight{}), "push-relabel certificate", seed);
}

void
topological_oracles(std::uint64_t seed)
{
  fixed_rng random{ seed };
  const std::size_t n = 2 + random.bounded(20);
  mm::digraph<> subject;
  (void)subject.add_vertices(n);
  oracle_digraph oracle(n);
  for ( std::size_t u = 0; u < n; ++u )
    for ( std::size_t v = u + 1; v < n; ++v )
      if ( v == u + 1 || random.bounded(100) < 15 ) {
        (void)subject.add_edge(static_cast<u32>(u), static_cast<u32>(v));
        boost::add_edge(u, v, oracle_edge{}, oracle);
      }

  std::vector<oracle_digraph::vertex_descriptor> oracle_order;
  boost::topological_sort(oracle, std::back_inserter(oracle_order));
  const auto order = mg::topological_sort(subject);
  check(oracle_order.size() == n, "boost topological size", seed);
  check(order.status == mg::algorithm_status::ok && order.order.size() == n, "topological status", seed);
  std::vector<std::size_t> position(n);
  for ( std::size_t i = 0; i < order.order.size(); ++i ) position[order.order.data()[i].value] = i;
  for ( auto edge : subject.edges() )
    check(position[edge.source.value] < position[edge.target.value], "topological order", seed, edge.source.value, edge.target.value);

  (void)subject.add_edge(static_cast<u32>(n - 1), u32(0));
  boost::add_edge(n - 1, 0, oracle_edge{}, oracle);
  bool oracle_cycle = false;
  try {
    oracle_order.clear();
    boost::topological_sort(oracle, std::back_inserter(oracle_order));
  } catch ( const boost::not_a_dag & ) {
    oracle_cycle = true;
  }
  check(oracle_cycle, "boost cycle detection", seed);
  check(mg::topological_sort(subject).status == mg::algorithm_status::not_a_dag, "cycle detection", seed);
}

};      // namespace

int
main()
{
  constexpr std::uint64_t directed_seed = 0x6f4a'31d8'a105'9bc3ULL;
  constexpr std::uint64_t signed_path_seed = 0x1b83'c6a4'fe92'705dULL;
  constexpr std::uint64_t undirected_seed = 0x95ce'0742'bb68'e1f9ULL;
  constexpr std::uint64_t matching_seed = 0x42f8'c91d'737a'065bULL;
  constexpr std::uint64_t weighted_matching_seed = 0xe372'419b'ca58'0f6dULL;
  constexpr std::uint64_t planarity_seed = 0x871d'0ae4'5b36'c29fULL;
  constexpr std::uint64_t flow_seed = 0xa734'09ed'5bc1'286fULL;
  constexpr std::uint64_t dag_seed = 0xd6b2'7a81'40ec'f395ULL;
  for ( std::uint64_t trial = 0; trial < 64; ++trial ) {
    directed_oracles(directed_seed ^ (trial * 0x9e37'79b9'7f4a'7c15ULL));
    negative_path_oracles(signed_path_seed ^ (trial * 0xa24b'aed4'963e'e407ULL));
    undirected_oracles(undirected_seed ^ (trial * 0xbf58'476d'1ce4'e5b9ULL));
    matching_oracles(matching_seed ^ (trial * 0x94d0'49bb'1331'11ebULL));
    weighted_matching_oracles(weighted_matching_seed ^ (trial * 0xc6bc'2796'92b5'cc83ULL));
    planarity_oracles(planarity_seed ^ (trial * 0xdb4f'0b91'75ae'2165ULL));
    flow_oracles(flow_seed ^ (trial * 0xd134'2543'de82'ef95ULL));
    topological_oracles(dag_seed ^ (trial * 0xda94'2042'e4dd'58b5ULL));
  }
  negative_cycle_oracle();
  if ( failures ) {
    std::cerr << failures << " Boost.Graph differential checks failed\n";
    return 6;
  }
  std::cout << "PASS: 64 fixed-seed Boost.Graph differential trials across traversal, signed paths, connectivity, MST, "
               "weighted/general matching, flow, planarity, PageRank, articulation points, and DAG ordering\n";
  return 1;
}
