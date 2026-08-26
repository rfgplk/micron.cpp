//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "graph.hpp"
#include "matrix.hpp"
#include "traversal.hpp"

namespace micron::math::graphs
{

template<typename G = graph<>>
[[nodiscard]] G
empty_graph(usize vertices)
{
  G result;
  (void)result.add_vertices(vertices);
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
path_graph(usize vertices)
{
  G result;
  (void)result.add_vertices(vertices);
  for ( usize i = 1; i < vertices; ++i )
    (void)result.add_edge(static_cast<typename G::index_type>(i - 1), static_cast<typename G::index_type>(i));
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
cycle_graph(usize vertices)
{
  G result = path_graph<G>(vertices);
  if ( vertices > 2 ) (void)result.add_edge(static_cast<typename G::index_type>(vertices - 1), typename G::index_type(0));
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
complete_graph(usize vertices)
{
  G result;
  (void)result.add_vertices(vertices);
  for ( usize i = 0; i < vertices; ++i ) {
    const usize begin = G::is_directed ? 0 : i + 1;
    for ( usize j = begin; j < vertices; ++j ) {
      if ( i == j && !G::allows_loops ) continue;
      (void)result.add_edge(static_cast<typename G::index_type>(i), static_cast<typename G::index_type>(j));
    }
  }
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
star_graph(usize leaves)
{
  G result;
  (void)result.add_vertices(leaves + 1);
  for ( usize i = 1; i <= leaves; ++i ) (void)result.add_edge(typename G::index_type(0), static_cast<typename G::index_type>(i));
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
wheel_graph(usize vertices)
{
  if ( vertices < 2 ) return empty_graph<G>(vertices);
  G result;
  (void)result.add_vertices(vertices);
  for ( usize i = 1; i < vertices; ++i ) {
    (void)result.add_edge(typename G::index_type(0), static_cast<typename G::index_type>(i));
    if ( vertices > 3 )
      (void)result.add_edge(static_cast<typename G::index_type>(i), static_cast<typename G::index_type>(i + 1 < vertices ? i + 1 : 1));
  }
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
grid_graph(usize rows, usize columns, bool periodic_rows = false, bool periodic_columns = false)
{
  G result;
  if ( rows == 0 || columns == 0 ) return result;
  (void)result.add_vertices(rows * columns);
  auto id = [columns](usize row, usize column) { return static_cast<typename G::index_type>(row * columns + column); };
  for ( usize row = 0; row < rows; ++row ) {
    for ( usize column = 0; column < columns; ++column ) {
      if ( column + 1 < columns )
        (void)result.add_edge(id(row, column), id(row, column + 1));
      else if ( periodic_columns && columns > 2 )
        (void)result.add_edge(id(row, column), id(row, 0));
      if ( row + 1 < rows )
        (void)result.add_edge(id(row, column), id(row + 1, column));
      else if ( periodic_rows && rows > 2 )
        (void)result.add_edge(id(row, column), id(0, column));
    }
  }
  return result;
}

template<typename G = graph<>>
[[nodiscard]] G
balanced_tree(usize branching, usize height)
{
  G result;
  (void)result.add_vertex();
  usize level_begin = 0;
  usize level_size = 1;
  for ( usize depth = 0; depth < height; ++depth ) {
    const usize child_begin = result.vertices_count();
    (void)result.add_vertices(level_size * branching);
    usize child = child_begin;
    for ( usize parent = level_begin; parent < level_begin + level_size; ++parent )
      for ( usize branch = 0; branch < branching; ++branch )
        (void)result.add_edge(static_cast<typename G::index_type>(parent), static_cast<typename G::index_type>(child++));
    level_begin = child_begin;
    level_size *= branching;
  }
  return result;
}

namespace __impl
{
template<typename Rng>
[[nodiscard]] inline f64
unit_random(Rng &rng)
{
  const u64 raw = static_cast<u64>(micron::invoke(rng));
  return static_cast<f64>(raw >> 11u) * static_cast<f64>(1.0 / 9007199254740992.0);
}
};      // namespace __impl

template<typename G = graph<>, typename Rng>
[[nodiscard]] G
erdos_renyi(usize vertices, f64 probability, Rng &&rng)
{
  G result;
  (void)result.add_vertices(vertices);
  if ( probability <= f64(0) ) return result;
  if ( probability > f64(1) ) probability = f64(1);
  for ( usize i = 0; i < vertices; ++i ) {
    const usize begin = G::is_directed ? 0 : i + 1;
    for ( usize j = begin; j < vertices; ++j ) {
      if ( i == j && !G::allows_loops ) continue;
      if ( __impl::unit_random(rng) < probability )
        (void)result.add_edge(static_cast<typename G::index_type>(i), static_cast<typename G::index_type>(j));
    }
  }
  return result;
}

template<typename G = graph<>, typename Rng>
[[nodiscard]] G
barabasi_albert(usize vertices, usize attachments, Rng &&rng)
{
  G result;
  if ( vertices == 0 || attachments == 0 ) return result;
  const usize initial = attachments + 1 < vertices ? attachments + 1 : vertices;
  result = complete_graph<G>(initial);
  micron::vector<usize, micron::allocator_serial<>, false> degree(vertices, usize(0));
  for ( usize i = 0; i < initial; ++i )
    degree.data()[i] = result.degree(typename G::vertex_descriptor(static_cast<typename G::index_type>(i)));
  for ( usize vertex = initial; vertex < vertices; ++vertex ) {
    (void)result.add_vertex();
    usize added = 0;
    usize attempts = 0;
    while ( added < attachments && attempts++ < attachments * vertices * 4 ) {
      usize total = 0;
      for ( usize i = 0; i < vertex; ++i ) total += degree.data()[i] + 1;
      usize pick = static_cast<usize>(micron::invoke(rng)) % total;
      usize target = 0;
      for ( ; target < vertex; ++target ) {
        const usize weight = degree.data()[target] + 1;
        if ( pick < weight ) break;
        pick -= weight;
      }
      auto inserted = result.add_edge(static_cast<typename G::index_type>(vertex), static_cast<typename G::index_type>(target));
      if ( inserted.inserted() ) {
        ++degree.data()[vertex];
        ++degree.data()[target];
        ++added;
      }
    }
  }
  return result;
}

template<typename G = graph<>, typename Rng>
[[nodiscard]] G
watts_strogatz(usize vertices, usize neighbors, f64 rewire_probability, Rng &&rng)
{
  G result;
  (void)result.add_vertices(vertices);
  if ( vertices < 2 ) return result;
  if ( neighbors >= vertices ) neighbors = vertices - 1;
  if ( neighbors & 1u ) --neighbors;
  const usize half = neighbors / 2;
  for ( usize u = 0; u < vertices; ++u ) {
    for ( usize offset = 1; offset <= half; ++offset ) {
      usize v = (u + offset) % vertices;
      if ( __impl::unit_random(rng) < rewire_probability ) {
        for ( usize attempt = 0; attempt < vertices * 2; ++attempt ) {
          const usize candidate = static_cast<usize>(micron::invoke(rng)) % vertices;
          if ( candidate != u
               && !result.has_edge(static_cast<typename G::index_type>(u), static_cast<typename G::index_type>(candidate)) ) {
            v = candidate;
            break;
          }
        }
      }
      (void)result.add_edge(static_cast<typename G::index_type>(u), static_cast<typename G::index_type>(v));
    }
  }
  return result;
}

template<typename G = graph<>, typename Rng>
[[nodiscard]] G
rmat(usize scale, usize edges, Rng &&rng, f64 a = 0.57, f64 b = 0.19, f64 c = 0.19)
{
  const usize vertices = usize(1) << scale;
  G result;
  (void)result.add_vertices(vertices);
  for ( usize e = 0; e < edges; ++e ) {
    usize u = 0;
    usize v = 0;
    for ( usize bit = scale; bit > 0; --bit ) {
      const f64 p = __impl::unit_random(rng);
      const usize mask = usize(1) << (bit - 1);
      if ( p < a ) {
      } else if ( p < a + b ) {
        v |= mask;
      } else if ( p < a + b + c ) {
        u |= mask;
      } else {
        u |= mask;
        v |= mask;
      }
    }
    (void)result.add_edge(static_cast<typename G::index_type>(u), static_cast<typename G::index_type>(v));
  }
  return result;
}

template<graph_model G>
using topology_graph_t = stable_adjacency_graph<empty_property, empty_property, empty_property, typename G::index_type,
                                                typename G::direction_type, simple_t, typename G::loop_type>;

template<graph_model G>
[[nodiscard]] topology_graph_t<G>
topology_vertices(const G &source)
{
  topology_graph_t<G> result;
  for ( usize slot = 0; slot < source.vertex_slots(); ++slot ) {
    const typename G::vertex_descriptor vertex(static_cast<typename G::index_type>(slot));
    if ( source.has_vertex(vertex) )
      (void)result.add_vertex();
    else
      (void)result.__import_dead_vertex_slot();
  }
  return result;
}

template<graph_model G>
[[nodiscard]] topology_graph_t<G>
topology_copy(const G &source)
{
  auto result = topology_vertices(source);
  for ( auto edge : source.edges() ) (void)result.add_edge(edge.source, edge.target);
  return result;
}

template<graph_model A, graph_model B>
[[nodiscard]] topology_graph_t<A>
graph_union(const A &a, const B &b)
{
  topology_graph_t<A> result;
  const usize slots = a.vertex_slots() > b.vertex_slots() ? a.vertex_slots() : b.vertex_slots();
  for ( usize slot = 0; slot < slots; ++slot ) {
    const bool in_a = slot < a.vertex_slots() && a.has_vertex(typename A::vertex_descriptor(static_cast<typename A::index_type>(slot)));
    const bool in_b = slot < b.vertex_slots() && b.has_vertex(typename B::vertex_descriptor(static_cast<typename B::index_type>(slot)));
    if ( in_a || in_b )
      (void)result.add_vertex();
    else
      (void)result.__import_dead_vertex_slot();
  }
  for ( auto edge : a.edges() ) (void)result.add_edge(edge.source, edge.target);
  for ( auto edge : b.edges() ) (void)result.add_edge(edge.source, edge.target);
  return result;
}

template<graph_model A, graph_model B>
[[nodiscard]] topology_graph_t<A>
graph_intersection(const A &a, const B &b)
{
  topology_graph_t<A> result;
  const usize slots = a.vertex_slots() > b.vertex_slots() ? a.vertex_slots() : b.vertex_slots();
  for ( usize slot = 0; slot < slots; ++slot ) {
    const bool in_a = slot < a.vertex_slots() && a.has_vertex(typename A::vertex_descriptor(static_cast<typename A::index_type>(slot)));
    const bool in_b = slot < b.vertex_slots() && b.has_vertex(typename B::vertex_descriptor(static_cast<typename B::index_type>(slot)));
    if ( in_a && in_b )
      (void)result.add_vertex();
    else
      (void)result.__import_dead_vertex_slot();
  }
  for ( auto edge : a.edges() )
    if ( b.has_edge(edge.source, edge.target) ) (void)result.add_edge(edge.source, edge.target);
  return result;
}

template<graph_model A, graph_model B>
[[nodiscard]] topology_graph_t<A>
graph_difference(const A &a, const B &b)
{
  topology_graph_t<A> result;
  for ( usize slot = 0; slot < a.vertex_slots(); ++slot ) {
    const typename A::vertex_descriptor vertex(static_cast<typename A::index_type>(slot));
    if ( a.has_vertex(vertex) )
      (void)result.add_vertex();
    else
      (void)result.__import_dead_vertex_slot();
  }
  for ( auto edge : a.edges() )
    if ( !b.has_edge(edge.source, edge.target) ) (void)result.add_edge(edge.source, edge.target);
  return result;
}

template<graph_model G>
[[nodiscard]] topology_graph_t<G>
complement(const G &graph)
{
  topology_graph_t<G> result;
  for ( usize slot = 0; slot < graph.vertex_slots(); ++slot ) {
    const typename G::vertex_descriptor vertex(static_cast<typename G::index_type>(slot));
    if ( graph.has_vertex(vertex) )
      (void)result.add_vertex();
    else
      (void)result.__import_dead_vertex_slot();
  }
  for ( auto u : graph.vertices() ) {
    for ( auto v : graph.vertices() ) {
      if ( u == v && !topology_graph_t<G>::allows_loops ) continue;
      if constexpr ( !G::is_directed )
        if ( v.value <= u.value ) continue;
      if ( !graph.has_edge(u, v) ) (void)result.add_edge(u, v);
    }
  }
  return result;
}

template<graph_model A, graph_model B>
[[nodiscard]] auto
compose(const A &a, const B &b)
{
  return graph_union(a, b);
}

template<graph_model G>
[[nodiscard]] graph<edge_id<typename G::index_type>, empty_property, empty_property, typename G::index_type>
line_graph(const G &source)
{
  using I = typename G::index_type;
  graph<edge_id<I>, empty_property, empty_property, I> result;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> edge_vertex(source.edge_slots(), vertex_id<I>::invalid());
  for ( auto edge : source.edges() ) edge_vertex.data()[static_cast<usize>(edge.id.value)] = result.add_vertex(edge.id);
  for ( auto a : source.edges() ) {
    for ( auto b : source.edges() ) {
      if ( b.id.value <= a.id.value ) continue;
      if ( a.source == b.source || a.source == b.target || a.target == b.source || a.target == b.target )
        (void)result.add_edge(edge_vertex.data()[static_cast<usize>(a.id.value)], edge_vertex.data()[static_cast<usize>(b.id.value)]);
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] topology_graph_t<G>
ego_graph(const G &source, typename G::vertex_descriptor center, usize radius = 1)
{
  auto reached = diffusion(source, center, radius);
  micron::vector<u8, micron::allocator_serial<>, false> include(source.vertex_slots(), u8(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> remap(source.vertex_slots(),
                                                                                         G::vertex_descriptor::invalid());
  topology_graph_t<G> result;
  for ( auto vertex : reached.order ) {
    const usize slot = static_cast<usize>(vertex.value);
    include.data()[slot] = 1;
    remap.data()[slot] = result.add_vertex();
  }
  for ( auto edge : source.edges() )
    if ( include.data()[static_cast<usize>(edge.source.value)] && include.data()[static_cast<usize>(edge.target.value)] )
      (void)result.add_edge(remap.data()[static_cast<usize>(edge.source.value)], remap.data()[static_cast<usize>(edge.target.value)]);
  return result;
}

template<graph_model A, graph_model B>
  requires micron::is_same_v<typename A::direction_type, typename B::direction_type>
using product_graph_t = graph<empty_property, empty_property, empty_property, usize, typename A::direction_type>;

template<graph_model A, graph_model B>
  requires micron::is_same_v<typename A::direction_type, typename B::direction_type>
[[nodiscard]] product_graph_t<A, B>
cartesian_product(const A &a, const B &b)
{
  product_graph_t<A, B> result;
  auto a_mapping = __dense_vertex_mapping(a);
  auto b_mapping = __dense_vertex_mapping(b);
  const usize an = a.vertices_count();
  const usize bn = b.vertices_count();
  (void)result.add_vertices(an * bn);
  auto id = [bn](usize av, usize bv) { return av * bn + bv; };
  for ( auto edge : a.edges() ) {
    const usize u = static_cast<usize>(a_mapping.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(a_mapping.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    for ( usize bv = 0; bv < bn; ++bv ) (void)result.add_edge(id(u, bv), id(v, bv));
  }
  for ( auto edge : b.edges() ) {
    const usize u = static_cast<usize>(b_mapping.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(b_mapping.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    for ( usize av = 0; av < an; ++av ) (void)result.add_edge(id(av, u), id(av, v));
  }
  return result;
}

template<graph_model A, graph_model B>
  requires micron::is_same_v<typename A::direction_type, typename B::direction_type>
[[nodiscard]] product_graph_t<A, B>
tensor_product(const A &a, const B &b)
{
  product_graph_t<A, B> result;
  auto a_mapping = __dense_vertex_mapping(a);
  auto b_mapping = __dense_vertex_mapping(b);
  const usize bn = b.vertices_count();
  (void)result.add_vertices(a.vertices_count() * bn);
  auto id = [bn](usize av, usize bv) { return av * bn + bv; };
  for ( auto ae : a.edges() ) {
    const usize au = static_cast<usize>(a_mapping.vertex_to_dense.data()[static_cast<usize>(ae.source.value)]);
    const usize av = static_cast<usize>(a_mapping.vertex_to_dense.data()[static_cast<usize>(ae.target.value)]);
    for ( auto be : b.edges() ) {
      const usize bu = static_cast<usize>(b_mapping.vertex_to_dense.data()[static_cast<usize>(be.source.value)]);
      const usize bv = static_cast<usize>(b_mapping.vertex_to_dense.data()[static_cast<usize>(be.target.value)]);
      (void)result.add_edge(id(au, bu), id(av, bv));
      if constexpr ( !A::is_directed ) (void)result.add_edge(id(au, bv), id(av, bu));
    }
  }
  return result;
}

template<graph_model A, graph_model B>
  requires micron::is_same_v<typename A::direction_type, typename B::direction_type>
[[nodiscard]] product_graph_t<A, B>
strong_product(const A &a, const B &b)
{
  auto result = cartesian_product(a, b);
  auto diagonal = tensor_product(a, b);
  for ( auto edge : diagonal.edges() ) (void)result.add_edge(edge.source, edge.target);
  return result;
}

};      // namespace micron::math::graphs
