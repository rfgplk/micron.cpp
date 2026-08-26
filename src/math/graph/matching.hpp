//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "connectivity.hpp"

namespace micron::math::graphs
{

template<micron::integral I> struct matching_result {
  algorithm_status status{ algorithm_status::ok };
  usize cardinality{};
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> mate;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
};

template<graph_model G>
[[nodiscard]] matching_result<typename G::index_type>
hopcroft_karp(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  using I = typename G::index_type;
  matching_result<I> result{ algorithm_status::ok,
                             0,
                             micron::vector<vertex_descriptor, micron::allocator_serial<>, false>(graph.vertex_slots(),
                                                                                                  vertex_descriptor::invalid()),
                             {} };
  auto partition = bipartite_test(graph);
  if ( !partition.bipartite ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  micron::vector<usize, micron::allocator_serial<>, false> distance(graph.vertex_slots(), usize(0));
  micron::vector<u8, micron::allocator_serial<>, false> active(graph.vertex_slots(), u8(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());

  auto each_neighbor = [&](vertex_descriptor u, auto &&fn) {
    for ( auto v : graph.out_neighbors(u) ) fn(v);
    if constexpr ( G::is_directed )
      for ( auto v : graph.in_neighbors(u) ) fn(v);
  };

  auto build_layers = [&]() {
    queue.clear();
    active.fill(u8(0));
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( partition.color.data()[slot] == 0 && !result.mate.data()[slot].valid() ) {
        active.data()[slot] = 1;
        distance.data()[slot] = 0;
        queue.push_back(vertex);
      }
    }
    bool augmenting = false;
    usize head = 0;
    while ( head < queue.size() ) {
      const vertex_descriptor u = queue.data()[head++];
      each_neighbor(u, [&](vertex_descriptor v) {
        const vertex_descriptor mate = result.mate.data()[static_cast<usize>(v.value)];
        if ( !mate.valid() ) {
          augmenting = true;
        } else {
          const usize ms = static_cast<usize>(mate.value);
          if ( !active.data()[ms] ) {
            active.data()[ms] = 1;
            distance.data()[ms] = distance.data()[static_cast<usize>(u.value)] + 1;
            queue.push_back(mate);
          }
        }
      });
    }
    return augmenting;
  };

  auto augment = [&](auto &&self, vertex_descriptor u) -> bool {
    bool success = false;
    each_neighbor(u, [&](vertex_descriptor v) {
      if ( success ) return;
      const vertex_descriptor mate = result.mate.data()[static_cast<usize>(v.value)];
      if ( !mate.valid()
           || (distance.data()[static_cast<usize>(mate.value)] == distance.data()[static_cast<usize>(u.value)] + 1 && self(self, mate)) ) {
        result.mate.data()[static_cast<usize>(u.value)] = v;
        result.mate.data()[static_cast<usize>(v.value)] = u;
        success = true;
      }
    });
    if ( !success ) active.data()[static_cast<usize>(u.value)] = 0;
    return success;
  };

  while ( build_layers() ) {
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( partition.color.data()[slot] == 0 && !result.mate.data()[slot].valid() && augment(augment, vertex) ) ++result.cardinality;
    }
  }
  result.edges.reserve(result.cardinality);
  for ( auto vertex : graph.vertices() ) {
    const usize slot = static_cast<usize>(vertex.value);
    if ( partition.color.data()[slot] != 0 || !result.mate.data()[slot].valid() ) continue;
    auto edge = graph.find_edge(vertex, result.mate.data()[slot]);
    if ( !edge.valid() && G::is_directed ) edge = graph.find_edge(result.mate.data()[slot], vertex);
    if ( edge.valid() ) result.edges.push_back(edge);
  }
  return result;
}

template<graph_model G>
[[nodiscard]] matching_result<typename G::index_type>
greedy_maximal_matching(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  matching_result<typename G::index_type> result{ algorithm_status::ok,
                                                  0,
                                                  micron::vector<vertex_descriptor, micron::allocator_serial<>, false>(
                                                      graph.vertex_slots(), vertex_descriptor::invalid()),
                                                  {} };
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    if ( edge.source == edge.target || result.mate.data()[u].valid() || result.mate.data()[v].valid() ) continue;
    result.mate.data()[u] = edge.target;
    result.mate.data()[v] = edge.source;
    result.edges.push_back(edge.id);
    ++result.cardinality;
  }
  return result;
}

template<graph_model G>
[[nodiscard]] bool
validate_matching(const G &graph, const matching_result<typename G::index_type> &matching)
{
  micron::vector<u8, micron::allocator_serial<>, false> used(graph.vertex_slots(), u8(0));
  for ( auto edge : matching.edges ) {
    if ( !graph.has_edge(edge) ) return false;
    const auto u = graph.source(edge);
    const auto v = graph.target(edge);
    if ( u == v || used.data()[static_cast<usize>(u.value)] || used.data()[static_cast<usize>(v.value)] ) return false;
    used.data()[static_cast<usize>(u.value)] = 1;
    used.data()[static_cast<usize>(v.value)] = 1;
  }
  return matching.edges.size() == matching.cardinality;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::edge_descriptor, micron::allocator_serial<>, false>
minimum_edge_cover(const G &graph)
{
  auto matching = hopcroft_karp(graph);
  micron::vector<typename G::edge_descriptor, micron::allocator_serial<>, false> result = micron::move(matching.edges);
  micron::vector<u8, micron::allocator_serial<>, false> covered(graph.vertex_slots(), u8(0));
  for ( auto edge : result ) {
    covered.data()[static_cast<usize>(graph.source(edge).value)] = 1;
    covered.data()[static_cast<usize>(graph.target(edge).value)] = 1;
  }
  for ( auto vertex : graph.vertices() ) {
    const usize slot = static_cast<usize>(vertex.value);
    if ( covered.data()[slot] ) continue;
    auto range = graph.out_edges(vertex);
    auto iterator = range.begin();
    if ( iterator == range.end() && G::is_directed ) {
      auto incoming = graph.in_edges(vertex);
      if ( incoming.begin() != incoming.end() ) {
        const auto edge = *incoming.begin();
        result.push_back(edge);
        covered.data()[slot] = 1;
        covered.data()[static_cast<usize>(graph.source(edge).value)] = 1;
      }
    } else if ( iterator != range.end() ) {
      const auto edge = *iterator;
      result.push_back(edge);
      covered.data()[slot] = 1;
      covered.data()[static_cast<usize>(graph.opposite(edge, vertex).value)] = 1;
    }
  }
  return result;
}

};      // namespace micron::math::graphs
