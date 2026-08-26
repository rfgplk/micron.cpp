//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "bit.hpp"
#include "csr.hpp"
#include "dense.hpp"
#include "graph.hpp"

namespace micron::math
{

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using compact_adjacency_graph = graph<VertexProperty, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using csr_graph = basic_csr_graph<VertexProperty, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::directed_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using bidirectional_csr_graph = basic_csr_graph<VertexProperty, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops, Alloc,
                                                graphs::bidirectional_csr_t>;

namespace graphs
{

template<typename Out, micron::integral I> struct representation_result {
  Out value;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> vertex_remap;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edge_remap;
  algorithm_status status{ algorithm_status::ok };

  [[nodiscard]] bool
  succeeded() const noexcept
  {
    return status == algorithm_status::ok;
  }

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return succeeded();
  }

  [[nodiscard]] Out *
  operator->() noexcept
  {
    return micron::addressof(value);
  }

  [[nodiscard]] const Out *
  operator->() const noexcept
  {
    return micron::addressof(value);
  }

  [[nodiscard]] Out &
  operator*() noexcept
  {
    return value;
  }

  [[nodiscard]] const Out &
  operator*() const noexcept
  {
    return value;
  }
};

template<typename G>
using frozen_graph_t
    = csr_graph<typename G::vertex_property_type, typename G::edge_property_type, typename G::graph_property_type, typename G::index_type,
                typename G::direction_type, typename G::multiplicity_type, typename G::loop_type, typename G::allocator_type>;

template<typename G>
using bidirectional_frozen_graph_t
    = bidirectional_csr_graph<typename G::vertex_property_type, typename G::edge_property_type, typename G::graph_property_type,
                              typename G::index_type, typename G::direction_type, typename G::multiplicity_type, typename G::loop_type,
                              typename G::allocator_type>;

template<typename G>
using thawed_graph_t
    = graph<typename G::vertex_property_type, typename G::edge_property_type, typename G::graph_property_type, typename G::index_type,
            typename G::direction_type, typename G::multiplicity_type, typename G::loop_type, typename G::allocator_type>;

template<typename G>
using stable_thawed_graph_t = stable_adjacency_graph<typename G::vertex_property_type, typename G::edge_property_type,
                                                     typename G::graph_property_type, typename G::index_type, typename G::direction_type,
                                                     typename G::multiplicity_type, typename G::loop_type, typename G::allocator_type>;

template<typename G>
[[nodiscard]] auto
__copy_representation(const G &source)
{
  using I = typename G::index_type;
  using Out = thawed_graph_t<G>;
  representation_result<Out, I> result{
    Out(source.graph_property()),
    {},
    micron::vector<vertex_id<I>, micron::allocator_serial<>, false>(source.vertex_slots(), vertex_id<I>::invalid()),
    micron::vector<edge_id<I>, micron::allocator_serial<>, false>(source.edge_slots(), edge_id<I>::invalid())
  };
  result.value.reserve_vertices(source.vertices_count());
  result.value.reserve_edges(source.edges_count());
  result.dense_to_vertex.reserve(source.vertices_count());
  for ( auto vertex : source.vertices() ) {
    const vertex_id<I> dense = result.value.add_vertex(source.vertex_property_unchecked(vertex));
    if ( !dense.valid() ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    result.vertex_remap.data()[static_cast<usize>(vertex.value)] = dense;
    result.dense_to_vertex.push_back(vertex);
  }
  for ( auto edge : source.edges() ) {
    const auto u = result.vertex_remap.data()[static_cast<usize>(edge.source.value)];
    const auto v = result.vertex_remap.data()[static_cast<usize>(edge.target.value)];
    auto inserted = result.value.add_edge(u, v, edge.property);
    if ( !inserted.inserted() ) {
      result.status = inserted.status == edge_insert_status::index_overflow ? algorithm_status::overflow : algorithm_status::invalid_graph;
      return result;
    }
    result.edge_remap.data()[static_cast<usize>(edge.id.value)] = inserted.id;
  }
  return result;
}

template<typename G>
[[nodiscard]] auto
freeze(const G &source)
{
  using I = typename G::index_type;
  using Out = frozen_graph_t<G>;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> vertex_remap;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edge_remap;
  Out value(source, dense_to_vertex, vertex_remap, edge_remap);
  return representation_result<Out, I>{ micron::move(value), micron::move(dense_to_vertex), micron::move(vertex_remap),
                                        micron::move(edge_remap) };
}

template<typename G>
[[nodiscard]] auto
freeze_bidirectional(const G &source)
{
  using I = typename G::index_type;
  using Out = bidirectional_frozen_graph_t<G>;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> vertex_remap;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edge_remap;
  Out value(source, dense_to_vertex, vertex_remap, edge_remap);
  return representation_result<Out, I>{ micron::move(value), micron::move(dense_to_vertex), micron::move(vertex_remap),
                                        micron::move(edge_remap) };
}

template<typename G>
[[nodiscard]] auto
thaw(const G &source)
{
  return __copy_representation(source);
}

template<typename G>
[[nodiscard]] auto
thaw_stable(const G &source)
{
  using I = typename G::index_type;
  using Out = stable_thawed_graph_t<G>;
  representation_result<Out, I> result{
    Out(source.graph_property()),
    {},
    micron::vector<vertex_id<I>, micron::allocator_serial<>, false>(source.vertex_slots(), vertex_id<I>::invalid()),
    micron::vector<edge_id<I>, micron::allocator_serial<>, false>(source.edge_slots(), edge_id<I>::invalid())
  };
  result.value.reserve_vertices(source.vertex_slots());
  result.value.reserve_edges(source.edge_slots());
  result.dense_to_vertex.reserve(source.vertices_count());
  for ( usize slot = 0; slot < source.vertex_slots(); ++slot ) {
    const vertex_id<I> original(static_cast<I>(slot));
    if ( source.has_vertex(original) ) {
      const vertex_id<I> inserted = result.value.add_vertex(source.vertex_property_unchecked(original));
      if ( !inserted.valid() ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      result.vertex_remap.data()[slot] = inserted;
      result.dense_to_vertex.push_back(original);
    } else {
      if ( !result.value.__import_dead_vertex_slot() ) {
        result.status = algorithm_status::overflow;
        return result;
      }
    }
  }
  for ( usize slot = 0; slot < source.edge_slots(); ++slot ) {
    const edge_id<I> original(static_cast<I>(slot));
    if ( source.has_edge(original) ) {
      const auto u = result.vertex_remap.data()[static_cast<usize>(source.source(original).value)];
      const auto v = result.vertex_remap.data()[static_cast<usize>(source.target(original).value)];
      auto inserted = result.value.add_edge(u, v, source.edge_property_unchecked(original));
      if ( !inserted.inserted() ) {
        result.status
            = inserted.status == edge_insert_status::index_overflow ? algorithm_status::overflow : algorithm_status::invalid_graph;
        return result;
      }
      result.edge_remap.data()[slot] = inserted.id;
    } else {
      if ( !result.value.__import_dead_edge_slot() ) {
        result.status = algorithm_status::overflow;
        return result;
      }
    }
  }
  return result;
}

template<typename G>
[[nodiscard]] auto
to_edge_list(const G &source)
{
  using I = typename G::index_type;
  using Out = edge_list_graph<typename G::vertex_property_type, typename G::edge_property_type, typename G::graph_property_type, I,
                              typename G::direction_type, typename G::multiplicity_type, typename G::loop_type, typename G::allocator_type>;
  auto stable = __copy_representation(source);
  representation_result<Out, I> result{ Out(stable.value.graph_property()), micron::move(stable.dense_to_vertex),
                                        micron::move(stable.vertex_remap), micron::move(stable.edge_remap), stable.status };
  if ( !stable ) return result;
  result.value.reserve_vertices(stable.value.vertices_count());
  result.value.reserve_edges(stable.value.edges_count());
  for ( auto v : stable.value.vertices() )
    if ( !result.value.add_vertex(stable.value.vertex_property_unchecked(v)).valid() ) {
      result.status = algorithm_status::overflow;
      return result;
    }
  for ( auto e : stable.value.edges() ) {
    const auto inserted = result.value.add_edge(e.source, e.target, e.property);
    if ( !inserted.inserted() ) {
      result.status = inserted.status == edge_insert_status::index_overflow ? algorithm_status::overflow : algorithm_status::invalid_graph;
      return result;
    }
  }
  return result;
}

template<typename Out, typename G>
[[nodiscard]] auto
__convert_packed_representation(const G &source)
{
  using I = typename G::index_type;
  representation_result<Out, I> result{
    Out(source.graph_property()),
    {},
    micron::vector<vertex_id<I>, micron::allocator_serial<>, false>(source.vertex_slots(), vertex_id<I>::invalid()),
    micron::vector<edge_id<I>, micron::allocator_serial<>, false>(source.edge_slots(), edge_id<I>::invalid())
  };
  result.dense_to_vertex.reserve(source.vertices_count());
  for ( auto vertex : source.vertices() ) {
    const auto dense = result.value.add_vertex(source.vertex_property_unchecked(vertex));
    if ( !dense.valid() ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    result.vertex_remap.data()[static_cast<usize>(vertex.value)] = dense;
    result.dense_to_vertex.push_back(vertex);
  }
  for ( auto edge : source.edges() ) {
    const auto u = result.vertex_remap.data()[static_cast<usize>(edge.source.value)];
    const auto v = result.vertex_remap.data()[static_cast<usize>(edge.target.value)];
    auto inserted = result.value.add_edge(u, v, edge.property);
    if ( !inserted.inserted() ) {
      result.status = inserted.status == edge_insert_status::index_overflow ? algorithm_status::overflow : algorithm_status::invalid_graph;
      return result;
    }
    result.edge_remap.data()[static_cast<usize>(edge.id.value)] = inserted.id;
  }
  return result;
}

template<typename G>
[[nodiscard]] auto
to_compact_adjacency(const G &source)
{
  return thaw(source);
}

template<typename G>
[[nodiscard]] auto
to_stable_adjacency(const G &source)
{
  return thaw_stable(source);
}

template<typename G>
[[nodiscard]] auto
to_dense_adjacency(const G &source)
{
  using Out = dense_adjacency_graph<typename G::vertex_property_type, typename G::edge_property_type, typename G::graph_property_type,
                                    typename G::index_type, typename G::direction_type, typename G::loop_type, typename G::allocator_type>;
  return __convert_packed_representation<Out>(source);
}

template<typename G>
  requires micron::is_same_v<typename G::edge_property_type, empty_property>
[[nodiscard]] auto
to_bit_adjacency(const G &source)
{
  using Out = bit_adjacency_graph<typename G::vertex_property_type, typename G::graph_property_type, typename G::index_type,
                                  typename G::direction_type, typename G::loop_type, typename G::allocator_type>;
  return __convert_packed_representation<Out>(source);
}

};      // namespace graphs
};      // namespace micron::math
