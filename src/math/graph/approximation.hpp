//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "matching.hpp"
#include "paths.hpp"
#include "trees.hpp"

namespace micron::math::graphs
{

template<micron::integral I> struct coloring_result {
  algorithm_status status{ algorithm_status::ok };
  usize colors{};
  micron::vector<I, micron::allocator_serial<>, false> color;
};

template<graph_model G, typename Ordering>
[[nodiscard]] coloring_result<typename G::index_type>
greedy_coloring(const G &graph, const Ordering &ordering)
{
  using I = typename G::index_type;
  const I none = vertex_id<I>::invalid_value();
  coloring_result<I> result{ algorithm_status::ok, 0, micron::vector<I, micron::allocator_serial<>, false>(graph.vertex_slots(), none) };
  micron::vector<u8, micron::allocator_serial<>, false> forbidden(graph.vertices_count() + 1, u8(0));
  for ( auto vertex : ordering ) {
    if ( !graph.has_vertex(vertex) ) continue;
    forbidden.fill(u8(0));
    for ( auto neighbor : graph.out_neighbors(vertex) ) {
      const I color = result.color.data()[static_cast<usize>(neighbor.value)];
      if ( color != none && static_cast<usize>(color) < forbidden.size() ) forbidden.data()[static_cast<usize>(color)] = 1;
    }
    if constexpr ( G::is_directed )
      for ( auto neighbor : graph.in_neighbors(vertex) ) {
        const I color = result.color.data()[static_cast<usize>(neighbor.value)];
        if ( color != none && static_cast<usize>(color) < forbidden.size() ) forbidden.data()[static_cast<usize>(color)] = 1;
      }
    usize color = 0;
    while ( color < forbidden.size() && forbidden.data()[color] ) ++color;
    result.color.data()[static_cast<usize>(vertex.value)] = static_cast<I>(color);
    if ( color + 1 > result.colors ) result.colors = color + 1;
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
greedy_coloring(const G &graph)
{
  auto ordering = smallest_last_ordering(graph);
  return greedy_coloring(graph, ordering);
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
maximal_independent_set(const G &graph)
{
  auto ordering = smallest_last_ordering(graph);
  micron::vector<u8, micron::allocator_serial<>, false> blocked(graph.vertex_slots(), u8(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> result;
  for ( auto vertex : ordering ) {
    if ( blocked.data()[static_cast<usize>(vertex.value)] ) continue;
    result.push_back(vertex);
    blocked.data()[static_cast<usize>(vertex.value)] = 1;
    for ( auto neighbor : graph.out_neighbors(vertex) ) blocked.data()[static_cast<usize>(neighbor.value)] = 1;
    if constexpr ( G::is_directed )
      for ( auto neighbor : graph.in_neighbors(vertex) ) blocked.data()[static_cast<usize>(neighbor.value)] = 1;
  }
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
greedy_dominating_set(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  micron::vector<u8, micron::allocator_serial<>, false> covered(graph.vertex_slots(), u8(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> result;
  usize covered_count = 0;
  while ( covered_count < graph.vertices_count() ) {
    vertex_descriptor best = vertex_descriptor::invalid();
    usize best_gain = 0;
    for ( auto vertex : graph.vertices() ) {
      usize gain = covered.data()[static_cast<usize>(vertex.value)] ? 0 : 1;
      for ( auto neighbor : graph.out_neighbors(vertex) )
        if ( !covered.data()[static_cast<usize>(neighbor.value)] ) ++gain;
      if ( !best.valid() || gain > best_gain ) {
        best = vertex;
        best_gain = gain;
      }
    }
    if ( !best.valid() ) break;
    result.push_back(best);
    const usize bs = static_cast<usize>(best.value);
    if ( !covered.data()[bs] ) {
      covered.data()[bs] = 1;
      ++covered_count;
    }
    for ( auto neighbor : graph.out_neighbors(best) ) {
      const usize slot = static_cast<usize>(neighbor.value);
      if ( !covered.data()[slot] ) {
        covered.data()[slot] = 1;
        ++covered_count;
      }
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
approximate_vertex_cover(const G &graph)
{
  auto matching = greedy_maximal_matching(graph);
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> result;
  result.reserve(matching.cardinality * 2);
  for ( auto edge : matching.edges ) {
    result.push_back(graph.source(edge));
    result.push_back(graph.target(edge));
  }
  return result;
}

template<micron::integral I> struct cut_heuristic_result {
  usize cut_edges{};
  micron::vector<u8, micron::allocator_serial<>, false> side;
};

template<graph_model G>
[[nodiscard]] cut_heuristic_result<typename G::index_type>
max_cut(const G &graph)
{
  cut_heuristic_result<typename G::index_type> result{ 0,
                                                       micron::vector<u8, micron::allocator_serial<>, false>(graph.vertex_slots(), u8(0)) };
  usize ordinal = 0;
  for ( auto vertex : graph.vertices() ) result.side.data()[static_cast<usize>(vertex.value)] = static_cast<u8>(ordinal++ & 1u);
  bool improved = true;
  while ( improved ) {
    improved = false;
    for ( auto vertex : graph.vertices() ) {
      usize same = 0;
      usize different = 0;
      for ( auto neighbor : graph.out_neighbors(vertex) ) {
        if ( result.side.data()[static_cast<usize>(neighbor.value)] == result.side.data()[static_cast<usize>(vertex.value)] )
          ++same;
        else
          ++different;
      }
      if ( same > different ) {
        result.side.data()[static_cast<usize>(vertex.value)] ^= 1u;
        improved = true;
      }
    }
  }
  for ( auto edge : graph.edges() )
    if ( result.side.data()[static_cast<usize>(edge.source.value)] != result.side.data()[static_cast<usize>(edge.target.value)] )
      ++result.cut_edges;
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
maximum_clique_heuristic(const G &graph)
{
  auto order = smallest_last_ordering(graph);
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> clique;
  for ( auto candidate : order ) {
    bool adjacent = true;
    for ( auto member : clique )
      if ( !graph.has_edge(candidate, member) && !graph.has_edge(member, candidate) ) {
        adjacent = false;
        break;
      }
    if ( adjacent ) clique.push_back(candidate);
  }
  return clique;
}

template<graph_model G>
[[nodiscard]] usize
treewidth_min_degree_upper_bound(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  micron::vector<u8, micron::allocator_serial<>, false> active(graph.vertex_slots(), u8(0));
  for ( auto vertex : graph.vertices() ) active.data()[static_cast<usize>(vertex.value)] = 1;
  usize bound = 0;
  for ( usize removed = 0; removed < graph.vertices_count(); ++removed ) {
    vertex_descriptor selected = vertex_descriptor::invalid();
    usize selected_degree = 0;
    for ( auto vertex : graph.vertices() ) {
      if ( !active.data()[static_cast<usize>(vertex.value)] ) continue;
      usize degree = 0;
      for ( auto neighbor : graph.out_neighbors(vertex) )
        if ( active.data()[static_cast<usize>(neighbor.value)] ) ++degree;
      if ( !selected.valid() || degree < selected_degree ) {
        selected = vertex;
        selected_degree = degree;
      }
    }
    if ( !selected.valid() ) break;
    if ( selected_degree > bound ) bound = selected_degree;
    active.data()[static_cast<usize>(selected.value)] = 0;
  }
  return bound;
}

};      // namespace micron::math::graphs
