//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "connectivity.hpp"
#include "generators.hpp"
#include "paths.hpp"

namespace micron::math::graphs
{

template<graph_model G>
[[nodiscard]] bool
has_cycle(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  micron::vector<u8, micron::allocator_serial<>, false> color(graph.vertex_slots(), u8(0));
  auto visit_directed = [&](auto &&self, vertex_descriptor u) -> bool {
    color.data()[static_cast<usize>(u.value)] = 1;
    for ( auto v : graph.out_neighbors(u) ) {
      const u8 state = color.data()[static_cast<usize>(v.value)];
      if ( state == 1 || (state == 0 && self(self, v)) ) return true;
    }
    color.data()[static_cast<usize>(u.value)] = 2;
    return false;
  };
  auto visit_undirected = [&](auto &&self, vertex_descriptor u, edge_descriptor parent) -> bool {
    color.data()[static_cast<usize>(u.value)] = 1;
    for ( auto edge : graph.out_edges(u) ) {
      if ( edge == parent ) continue;
      const vertex_descriptor v = graph.opposite(edge, u);
      if ( color.data()[static_cast<usize>(v.value)] || self(self, v, edge) ) return true;
    }
    return false;
  };
  for ( auto root : graph.vertices() ) {
    if ( color.data()[static_cast<usize>(root.value)] ) continue;
    if constexpr ( G::is_directed ) {
      if ( visit_directed(visit_directed, root) ) return true;
    } else if ( visit_undirected(visit_undirected, root, edge_descriptor::invalid()) ) {
      return true;
    }
  }
  return false;
}

template<micron::integral I> struct euler_result {
  algorithm_status status{ algorithm_status::ok };
  bool circuit{};
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> vertices;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
};

template<graph_model G>
[[nodiscard]] euler_result<typename G::index_type>
eulerian_path(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  euler_result<typename G::index_type> result;
  if ( graph.edges_count() == 0 ) {
    auto vertices = graph.vertices();
    if ( vertices.begin() != vertices.end() ) result.vertices.push_back(*vertices.begin());
    result.circuit = true;
    return result;
  }

  vertex_descriptor start = vertex_descriptor::invalid();
  if constexpr ( G::is_directed ) {
    usize plus = 0;
    usize minus = 0;
    for ( auto vertex : graph.vertices() ) {
      const usize out = graph.out_degree(vertex);
      const usize in = graph.in_degree(vertex);
      if ( out == in + 1 ) {
        ++plus;
        start = vertex;
      } else if ( in == out + 1 ) {
        ++minus;
      } else if ( in != out ) {
        result.status = algorithm_status::invalid_graph;
        return result;
      }
      if ( !start.valid() && out ) start = vertex;
    }
    if ( !((plus == 0 && minus == 0) || (plus == 1 && minus == 1)) ) {
      result.status = algorithm_status::invalid_graph;
      return result;
    }
    result.circuit = plus == 0;
  } else {
    usize odd = 0;
    for ( auto vertex : graph.vertices() ) {
      if ( graph.degree(vertex) & 1u ) {
        ++odd;
        start = vertex;
      } else if ( !start.valid() && graph.degree(vertex) ) {
        start = vertex;
      }
    }
    if ( odd != 0 && odd != 2 ) {
      result.status = algorithm_status::invalid_graph;
      return result;
    }
    result.circuit = odd == 0;
  }

  micron::vector<u8, micron::allocator_serial<>, false> used(graph.edge_slots(), u8(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> vertex_stack;
  micron::vector<edge_descriptor, micron::allocator_serial<>, false> edge_stack;
  vertex_stack.push_back(start);
  while ( !vertex_stack.empty() ) {
    const vertex_descriptor u = vertex_stack.data()[vertex_stack.size() - 1];
    edge_descriptor next_edge = edge_descriptor::invalid();
    for ( auto edge : graph.out_edges(u) )
      if ( !used.data()[static_cast<usize>(edge.value)] ) {
        next_edge = edge;
        break;
      }
    if ( next_edge.valid() ) {
      used.data()[static_cast<usize>(next_edge.value)] = 1;
      const vertex_descriptor v = G::is_directed ? graph.target(next_edge) : graph.opposite(next_edge, u);
      vertex_stack.push_back(v);
      edge_stack.push_back(next_edge);
    } else {
      result.vertices.push_back(u);
      vertex_stack.pop_back();
      if ( !edge_stack.empty() ) {
        result.edges.push_back(edge_stack.data()[edge_stack.size() - 1]);
        edge_stack.pop_back();
      }
    }
  }
  if ( result.edges.size() != graph.edges_count() ) {
    result.status = algorithm_status::disconnected;
    result.vertices.clear();
    result.edges.clear();
    return result;
  }
  for ( usize a = 0, b = result.vertices.size() ? result.vertices.size() - 1 : 0; a < b; ++a, --b )
    micron::swap(result.vertices.data()[a], result.vertices.data()[b]);
  for ( usize a = 0, b = result.edges.size() ? result.edges.size() - 1 : 0; a < b; ++a, --b )
    micron::swap(result.edges.data()[a], result.edges.data()[b]);
  return result;
}

template<graph_model G>
[[nodiscard]] bool
is_eulerian(const G &graph)
{
  auto result = eulerian_path(graph);
  return result.status == algorithm_status::ok && result.circuit;
}

template<graph_model G>
[[nodiscard]] topology_graph_t<G>
transitive_closure(const G &graph)
{
  auto result = topology_vertices(graph);
  for ( auto source : graph.vertices() ) {
    auto reached = bfs(graph, source);
    for ( auto target : reached.order ) {
      if ( source == target && !topology_graph_t<G>::allows_loops ) continue;
      (void)result.add_edge(source, target);
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] topology_graph_t<G>
transitive_reduction(const G &graph, algorithm_status *status = nullptr)
{
  auto result = topology_vertices(graph);
  auto order = topological_sort(graph);
  if ( order.status != algorithm_status::ok ) {
    if ( status ) *status = algorithm_status::not_a_dag;
    return result;
  }
  micron::vector<u8, micron::allocator_serial<>, false> seen(graph.vertex_slots(), u8(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> queue;
  for ( auto candidate : graph.edges() ) {
    seen.fill(u8(0));
    queue.clear();
    queue.push_back(candidate.source);
    seen.data()[static_cast<usize>(candidate.source.value)] = 1;
    usize head = 0;
    bool alternate = false;
    while ( head < queue.size() && !alternate ) {
      const auto u = queue.data()[head++];
      for ( auto edge : graph.out_edges(u) ) {
        if ( edge == candidate.id ) continue;
        const auto v = graph.target(edge);
        if ( v == candidate.target ) {
          alternate = true;
          break;
        }
        const usize slot = static_cast<usize>(v.value);
        if ( !seen.data()[slot] ) {
          seen.data()[slot] = 1;
          queue.push_back(v);
        }
      }
    }
    if ( !alternate ) (void)result.add_edge(candidate.source, candidate.target);
  }
  if ( status ) *status = algorithm_status::ok;
  return result;
}

template<graph_model G>
[[nodiscard]] bool
is_chordal(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  if constexpr ( G::is_directed ) return false;
  const usize n = graph.vertices_count();
  micron::vector<usize, micron::allocator_serial<>, false> label(graph.vertex_slots(), usize(0));
  micron::vector<usize, micron::allocator_serial<>, false> number(graph.vertex_slots(), usize(0));
  micron::vector<u8, micron::allocator_serial<>, false> selected(graph.vertex_slots(), u8(0));
  for ( usize step = n; step > 0; --step ) {
    vertex_descriptor best = vertex_descriptor::invalid();
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( selected.data()[slot] ) continue;
      if ( !best.valid() || label.data()[static_cast<usize>(best.value)] < label.data()[slot] ) best = vertex;
    }
    const usize bs = static_cast<usize>(best.value);
    selected.data()[bs] = 1;
    number.data()[bs] = step;
    for ( auto neighbor : graph.out_neighbors(best) )
      if ( !selected.data()[static_cast<usize>(neighbor.value)] ) ++label.data()[static_cast<usize>(neighbor.value)];
  }
  for ( auto vertex : graph.vertices() ) {
    vertex_descriptor parent = vertex_descriptor::invalid();
    for ( auto neighbor : graph.out_neighbors(vertex) ) {
      if ( number.data()[static_cast<usize>(neighbor.value)] <= number.data()[static_cast<usize>(vertex.value)] ) continue;
      if ( !parent.valid() || number.data()[static_cast<usize>(neighbor.value)] < number.data()[static_cast<usize>(parent.value)] )
        parent = neighbor;
    }
    if ( !parent.valid() ) continue;
    for ( auto neighbor : graph.out_neighbors(vertex) ) {
      if ( neighbor == parent || number.data()[static_cast<usize>(neighbor.value)] <= number.data()[static_cast<usize>(vertex.value)] )
        continue;
      if ( !graph.has_edge(parent, neighbor) ) return false;
    }
  }
  return true;
}

template<typename Range>
[[nodiscard]] bool
is_graphical_degree_sequence(const Range &input)
{
  micron::vector<usize, micron::allocator_serial<>, false> degrees;
  for ( auto value : input ) degrees.push_back(static_cast<usize>(value));

  struct descending {
    usize value;

    [[nodiscard]] bool
    operator<(const descending &other) const
    {
      return value > other.value;
    }
  };

  while ( !degrees.empty() ) {
    micron::vector<descending, micron::allocator_serial<>, false> sorted;
    sorted.reserve(degrees.size());
    for ( usize value : degrees )
      if ( value ) sorted.push_back({ value });
    if ( sorted.empty() ) return true;
    sorted.sort();
    const usize degree = sorted.data()[0].value;
    if ( degree >= sorted.size() ) return false;
    degrees.clear();
    for ( usize i = 1; i < sorted.size(); ++i ) {
      usize value = sorted.data()[i].value;
      if ( i <= degree ) {
        if ( value == 0 ) return false;
        --value;
      }
      degrees.push_back(value);
    }
  }
  return true;
}

namespace __impl
{
[[nodiscard]] constexpr u64
mix_color(u64 value) noexcept
{
  value ^= value >> 30;
  value *= 0xbf58476d1ce4e5b9ull;
  value ^= value >> 27;
  value *= 0x94d049bb133111ebull;
  return value ^ (value >> 31);
}
};      // namespace __impl

template<graph_model G>
[[nodiscard]] u64
weisfeiler_lehman_hash(const G &graph, usize iterations = 3)
{
  micron::vector<u64, micron::allocator_serial<>, false> color(graph.vertex_slots(), u64(0));
  micron::vector<u64, micron::allocator_serial<>, false> next(graph.vertex_slots(), u64(0));
  for ( auto vertex : graph.vertices() ) color.data()[static_cast<usize>(vertex.value)] = __impl::mix_color(graph.degree(vertex) + 1);
  for ( usize iteration = 0; iteration < iterations; ++iteration ) {
    for ( auto vertex : graph.vertices() ) {
      micron::vector<u64, micron::allocator_serial<>, false> neighbors;
      neighbors.reserve(graph.degree(vertex));
      for ( auto neighbor : graph.out_neighbors(vertex) ) neighbors.push_back(color.data()[static_cast<usize>(neighbor.value)]);
      if constexpr ( G::is_directed )
        for ( auto neighbor : graph.in_neighbors(vertex) )
          neighbors.push_back(__impl::mix_color(color.data()[static_cast<usize>(neighbor.value)] ^ 0x9e3779b97f4a7c15ull));
      neighbors.sort();
      u64 hash = __impl::mix_color(color.data()[static_cast<usize>(vertex.value)] ^ static_cast<u64>(neighbors.size()));
      for ( u64 value : neighbors ) hash = __impl::mix_color(hash ^ value);
      next.data()[static_cast<usize>(vertex.value)] = hash;
    }
    color.swap(next);
  }
  micron::vector<u64, micron::allocator_serial<>, false> sorted;
  sorted.reserve(graph.vertices_count());
  for ( auto vertex : graph.vertices() ) sorted.push_back(color.data()[static_cast<usize>(vertex.value)]);
  sorted.sort();
  u64 result = __impl::mix_color(static_cast<u64>(graph.vertices_count()) ^ (static_cast<u64>(graph.edges_count()) << 32u));
  for ( u64 value : sorted ) result = __impl::mix_color(result ^ value);
  return result;
}

template<graph_model A, graph_model B>
[[nodiscard]] bool
trees_isomorphic(const A &a, const B &b)
{
  if ( a.vertices_count() != b.vertices_count() || a.edges_count() != b.edges_count() ) return false;
  if ( a.vertices_count() && (a.edges_count() + 1 != a.vertices_count() || !is_connected(a) || !is_connected(b)) ) return false;
  return weisfeiler_lehman_hash(a, a.vertices_count()) == weisfeiler_lehman_hash(b, b.vertices_count());
}

template<micron::integral I> struct dominator_result {
  algorithm_status status{ algorithm_status::ok };
  usize words{};
  micron::vector<u64, micron::allocator_serial<>, false> sets;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> immediate;

  [[nodiscard]] bool
  dominates(vertex_id<I> dominator, vertex_id<I> vertex) const noexcept
  {
    return (sets.data()[static_cast<usize>(vertex.value) * words + (static_cast<usize>(dominator.value) >> 6u)]
            & (u64(1) << (static_cast<usize>(dominator.value) & 63u)))
           != 0;
  }
};

template<graph_model G>
[[nodiscard]] dominator_result<typename G::index_type>
dominators(const G &graph, typename G::vertex_descriptor start)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  dominator_result<typename G::index_type> result;
  if ( !graph.has_vertex(start) ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }
  result.words = (graph.vertex_slots() + 63u) / 64u;
  result.sets = micron::vector<u64, micron::allocator_serial<>, false>(graph.vertex_slots() * result.words, u64(0));
  result.immediate
      = micron::vector<vertex_descriptor, micron::allocator_serial<>, false>(graph.vertex_slots(), vertex_descriptor::invalid());
  micron::vector<u64, micron::allocator_serial<>, false> universe(result.words, u64(0));
  for ( auto vertex : graph.vertices() )
    universe.data()[static_cast<usize>(vertex.value) >> 6u] |= u64(1) << (static_cast<usize>(vertex.value) & 63u);
  for ( auto vertex : graph.vertices() ) {
    u64 *set = result.sets.data() + static_cast<usize>(vertex.value) * result.words;
    if ( vertex == start )
      set[static_cast<usize>(start.value) >> 6u] |= u64(1) << (static_cast<usize>(start.value) & 63u);
    else
      for ( usize word = 0; word < result.words; ++word ) set[word] = universe.data()[word];
  }
  bool changed = true;
  while ( changed ) {
    changed = false;
    for ( auto vertex : graph.vertices() ) {
      if ( vertex == start ) continue;
      micron::vector<u64, micron::allocator_serial<>, false> intersection = universe;
      bool have_predecessor = false;
      for ( auto predecessor : graph.in_neighbors(vertex) ) {
        have_predecessor = true;
        const u64 *set = result.sets.data() + static_cast<usize>(predecessor.value) * result.words;
        for ( usize word = 0; word < result.words; ++word ) intersection.data()[word] &= set[word];
      }
      if ( !have_predecessor ) intersection.fill(u64(0));
      intersection.data()[static_cast<usize>(vertex.value) >> 6u] |= u64(1) << (static_cast<usize>(vertex.value) & 63u);
      u64 *current = result.sets.data() + static_cast<usize>(vertex.value) * result.words;
      for ( usize word = 0; word < result.words; ++word )
        if ( current[word] != intersection.data()[word] ) {
          current[word] = intersection.data()[word];
          changed = true;
        }
    }
  }
  result.immediate.data()[static_cast<usize>(start.value)] = start;
  for ( auto vertex : graph.vertices() ) {
    if ( vertex == start ) continue;
    vertex_descriptor best = vertex_descriptor::invalid();
    for ( auto candidate : graph.vertices() ) {
      if ( candidate == vertex || !result.dominates(candidate, vertex) ) continue;
      if ( !best.valid() || result.dominates(best, candidate) ) best = candidate;
    }
    result.immediate.data()[static_cast<usize>(vertex.value)] = best;
  }
  return result;
}

};      // namespace micron::math::graphs
