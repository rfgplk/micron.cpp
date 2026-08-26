//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../numerics.hpp"
#include "../../type_traits.hpp"
#include "descriptors.hpp"
#include "tags.hpp"

namespace micron::math
{

template<usize VertexCapacity, usize EdgeCapacity, class VertexProperty = empty_property, class EdgeProperty = empty_property,
         class GraphProperty = empty_property, micron::integral Index = u32, class Direction = graphs::undirected_t,
         class Multiplicity = graphs::simple_t, class Loops = graphs::no_loops_t>
  requires graphs::direction_policy<Direction> && graphs::multiplicity_policy<Multiplicity> && graphs::loop_policy<Loops>
           && micron::is_default_constructible_v<VertexProperty> && micron::is_default_constructible_v<EdgeProperty>
           && micron::is_default_constructible_v<GraphProperty>
class fixed_graph
{
  static constexpr usize __vertex_capacity = VertexCapacity ? VertexCapacity : 1;
  static constexpr usize __edge_capacity = EdgeCapacity ? EdgeCapacity : 1;

  struct vertex_slot {
    bool live{};
    [[no_unique_address]] VertexProperty property{};
  };

  struct edge_slot {
    bool live{};
    Index source{};
    Index target{};
    [[no_unique_address]] EdgeProperty property{};
  };

  [[no_unique_address]] GraphProperty __property{};
  vertex_slot __vertices[__vertex_capacity]{};
  edge_slot __edges[__edge_capacity]{};
  usize __vertex_high{};
  usize __edge_high{};
  usize __live_vertices{};
  usize __live_edges{};

public:
  using vertex_property_type = VertexProperty;
  using edge_property_type = EdgeProperty;
  using graph_property_type = GraphProperty;
  using index_type = Index;
  using direction_type = Direction;
  using multiplicity_type = Multiplicity;
  using loop_type = Loops;
  using storage_type = graphs::stable_adjacency_t;
  using allocator_type = micron::allocator_serial<>;
  using vertex_descriptor = vertex_id<Index>;
  using edge_descriptor = edge_id<Index>;
  using micron_graph_tag = void;
  using micron_printable_tag = void;

  static constexpr bool is_directed = micron::is_same_v<Direction, graphs::directed_t>;
  static constexpr bool is_undirected = !is_directed;
  static constexpr bool is_simple = micron::is_same_v<Multiplicity, graphs::simple_t>;
  static constexpr bool allows_parallel_edges = !is_simple;
  static constexpr bool allows_loops = micron::is_same_v<Loops, graphs::allow_loops_t>;

  static_assert(VertexCapacity < static_cast<usize>(micron::numeric_limits<Index>::max()));
  static_assert(EdgeCapacity < static_cast<usize>(micron::numeric_limits<Index>::max()));

  constexpr fixed_graph() noexcept = default;

  constexpr explicit fixed_graph(const GraphProperty &property) noexcept : __property(property) { }

  [[nodiscard]] constexpr usize
  vertices_count() const noexcept
  {
    return __live_vertices;
  }

  [[nodiscard]] constexpr usize
  edges_count() const noexcept
  {
    return __live_edges;
  }

  [[nodiscard]] constexpr usize
  vertex_slots() const noexcept
  {
    return __vertex_high;
  }

  [[nodiscard]] constexpr usize
  edge_slots() const noexcept
  {
    return __edge_high;
  }

  [[nodiscard]] constexpr bool
  empty() const noexcept
  {
    return __live_vertices == 0;
  }

  [[nodiscard]] constexpr GraphProperty &
  property() noexcept
  {
    return __property;
  }

  [[nodiscard]] constexpr const GraphProperty &
  property() const noexcept
  {
    return __property;
  }

  [[nodiscard]] constexpr GraphProperty &
  graph_property() noexcept
  {
    return __property;
  }

  [[nodiscard]] constexpr const GraphProperty &
  graph_property() const noexcept
  {
    return __property;
  }

  [[nodiscard]] constexpr vertex_descriptor
  add_vertex(const VertexProperty &property = VertexProperty{}) noexcept
  {
    if ( __vertex_high >= VertexCapacity ) return vertex_descriptor::invalid();
    const usize slot = __vertex_high++;
    __vertices[slot].live = true;
    __vertices[slot].property = property;
    ++__live_vertices;
    return vertex_descriptor(static_cast<Index>(slot));
  }

  [[nodiscard]] constexpr edge_descriptor
  find_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    if ( !has_vertex(u) || !has_vertex(v) ) return edge_descriptor::invalid();
    for ( usize i = 0; i < __edge_high; ++i ) {
      const edge_slot &edge = __edges[i];
      if ( !edge.live ) continue;
      if ( edge.source == u.value && edge.target == v.value ) return edge_descriptor(static_cast<Index>(i));
      if constexpr ( !is_directed )
        if ( edge.source == v.value && edge.target == u.value ) return edge_descriptor(static_cast<Index>(i));
    }
    return edge_descriptor::invalid();
  }

  [[nodiscard]] constexpr edge_insert_result<Index>
  add_edge(vertex_descriptor u, vertex_descriptor v, const EdgeProperty &property = EdgeProperty{}) noexcept
  {
    if ( !u.valid() || !v.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    const usize us = static_cast<usize>(u.value);
    const usize vs = static_cast<usize>(v.value);
    const usize high = us > vs ? us : vs;
    if ( high >= VertexCapacity || __edge_high >= EdgeCapacity )
      return { graphs::edge_insert_status::index_overflow, edge_descriptor::invalid() };
    if constexpr ( !allows_loops )
      if ( u == v ) return { graphs::edge_insert_status::self_loop, edge_descriptor::invalid() };
    if ( us < __vertex_high && !__vertices[us].live ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if ( vs < __vertex_high && !__vertices[vs].live ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if constexpr ( is_simple ) {
      const edge_descriptor duplicate = find_edge(u, v);
      if ( duplicate.valid() ) return { graphs::edge_insert_status::duplicate, duplicate };
    }
    while ( __vertex_high <= high ) (void)add_vertex();
    const usize slot = __edge_high++;
    __edges[slot] = edge_slot{ true, u.value, v.value, property };
    ++__live_edges;
    return { graphs::edge_insert_status::inserted, edge_descriptor(static_cast<Index>(slot)) };
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] constexpr edge_insert_result<Index>
  add_edge(U u, V v, const EdgeProperty &property = EdgeProperty{}) noexcept
  {
    if constexpr ( micron::is_signed_v<U> )
      if ( u < U(0) ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if constexpr ( micron::is_signed_v<V> )
      if ( v < V(0) ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if ( static_cast<umax_t>(u) >= VertexCapacity || static_cast<umax_t>(v) >= VertexCapacity )
      return { graphs::edge_insert_status::index_overflow, edge_descriptor::invalid() };
    return add_edge(vertex_descriptor(static_cast<Index>(u)), vertex_descriptor(static_cast<Index>(v)), property);
  }

  [[nodiscard]] constexpr bool
  has_vertex(vertex_descriptor vertex) const noexcept
  {
    return vertex.valid() && static_cast<usize>(vertex.value) < __vertex_high && __vertices[static_cast<usize>(vertex.value)].live;
  }

  [[nodiscard]] constexpr bool
  has_edge(edge_descriptor edge) const noexcept
  {
    return edge.valid() && static_cast<usize>(edge.value) < __edge_high && __edges[static_cast<usize>(edge.value)].live;
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] constexpr bool
  has_edge(U u, V v) const noexcept
  {
    return find_edge(vertex_descriptor(static_cast<Index>(u)), vertex_descriptor(static_cast<Index>(v))).valid();
  }

  [[nodiscard]] constexpr bool
  remove_edge(edge_descriptor edge) noexcept
  {
    if ( !has_edge(edge) ) return false;
    __edges[static_cast<usize>(edge.value)].live = false;
    --__live_edges;
    return true;
  }

  [[nodiscard]] constexpr bool
  remove_vertex(vertex_descriptor vertex) noexcept
  {
    if ( !has_vertex(vertex) ) return false;
    for ( usize i = 0; i < __edge_high; ++i )
      if ( __edges[i].live && (__edges[i].source == vertex.value || __edges[i].target == vertex.value) )
        (void)remove_edge(edge_descriptor(static_cast<Index>(i)));
    __vertices[static_cast<usize>(vertex.value)].live = false;
    --__live_vertices;
    return true;
  }

  [[nodiscard]] constexpr VertexProperty &
  vertex_property_unchecked(vertex_descriptor vertex) noexcept
  {
    return __vertices[static_cast<usize>(vertex.value)].property;
  }

  [[nodiscard]] constexpr const VertexProperty &
  vertex_property_unchecked(vertex_descriptor vertex) const noexcept
  {
    return __vertices[static_cast<usize>(vertex.value)].property;
  }

  [[nodiscard]] constexpr EdgeProperty &
  edge_property_unchecked(edge_descriptor edge) noexcept
  {
    return __edges[static_cast<usize>(edge.value)].property;
  }

  [[nodiscard]] constexpr const EdgeProperty &
  edge_property_unchecked(edge_descriptor edge) const noexcept
  {
    return __edges[static_cast<usize>(edge.value)].property;
  }

  [[nodiscard]] constexpr vertex_descriptor
  source(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(__edges[static_cast<usize>(edge.value)].source);
  }

  [[nodiscard]] constexpr vertex_descriptor
  target(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(__edges[static_cast<usize>(edge.value)].target);
  }

  [[nodiscard]] constexpr vertex_descriptor
  opposite(edge_descriptor edge, vertex_descriptor vertex) const noexcept
  {
    const edge_slot &slot = __edges[static_cast<usize>(edge.value)];
    return vertex_descriptor(slot.source == vertex.value ? slot.target : slot.source);
  }

private:
  class vertex_iterator
  {
    const fixed_graph *__graph{};
    usize __slot{};

    constexpr void
    skip() noexcept
    {
      while ( __slot < __graph->__vertex_high && !__graph->__vertices[__slot].live ) ++__slot;
    }

  public:
    constexpr vertex_iterator(const fixed_graph *graph, usize slot) noexcept : __graph(graph), __slot(slot) { skip(); }

    [[nodiscard]] constexpr vertex_descriptor
    operator*() const noexcept
    {
      return vertex_descriptor(static_cast<Index>(__slot));
    }

    constexpr vertex_iterator &
    operator++() noexcept
    {
      ++__slot;
      skip();
      return *this;
    }

    friend constexpr bool
    operator==(vertex_iterator a, vertex_iterator b) noexcept
    {
      return a.__graph == b.__graph && a.__slot == b.__slot;
    }

    friend constexpr bool
    operator!=(vertex_iterator a, vertex_iterator b) noexcept
    {
      return !(a == b);
    }
  };

public:
  struct const_edge_reference {
    edge_descriptor id;
    vertex_descriptor source;
    vertex_descriptor target;
    const EdgeProperty &property;
  };

private:
  class edge_iterator
  {
    const fixed_graph *__graph{};
    usize __slot{};

    constexpr void
    skip() noexcept
    {
      while ( __slot < __graph->__edge_high && !__graph->__edges[__slot].live ) ++__slot;
    }

  public:
    constexpr edge_iterator(const fixed_graph *graph, usize slot) noexcept : __graph(graph), __slot(slot) { skip(); }

    [[nodiscard]] constexpr const_edge_reference
    operator*() const noexcept
    {
      const edge_slot &slot = __graph->__edges[__slot];
      return { edge_descriptor(static_cast<Index>(__slot)), vertex_descriptor(slot.source), vertex_descriptor(slot.target), slot.property };
    }

    constexpr edge_iterator &
    operator++() noexcept
    {
      ++__slot;
      skip();
      return *this;
    }

    friend constexpr bool
    operator==(edge_iterator a, edge_iterator b) noexcept
    {
      return a.__graph == b.__graph && a.__slot == b.__slot;
    }

    friend constexpr bool
    operator!=(edge_iterator a, edge_iterator b) noexcept
    {
      return !(a == b);
    }
  };

  class incident_iterator
  {
    const fixed_graph *__graph{};
    vertex_descriptor __vertex{};
    usize __slot{};
    bool __incoming{};

    constexpr bool
    matches() const noexcept
    {
      if ( !__graph->__edges[__slot].live ) return false;
      if ( __incoming && is_directed ) return __graph->__edges[__slot].target == __vertex.value;
      return __graph->__edges[__slot].source == __vertex.value || (!is_directed && __graph->__edges[__slot].target == __vertex.value);
    }

    constexpr void
    skip() noexcept
    {
      while ( __slot < __graph->__edge_high && !matches() ) ++__slot;
    }

  public:
    constexpr incident_iterator(const fixed_graph *graph, vertex_descriptor vertex, usize slot, bool incoming) noexcept
        : __graph(graph), __vertex(vertex), __slot(slot), __incoming(incoming)
    {
      skip();
    }

    [[nodiscard]] constexpr edge_descriptor
    operator*() const noexcept
    {
      return edge_descriptor(static_cast<Index>(__slot));
    }

    constexpr incident_iterator &
    operator++() noexcept
    {
      ++__slot;
      skip();
      return *this;
    }

    friend constexpr bool
    operator==(incident_iterator a, incident_iterator b) noexcept
    {
      return a.__graph == b.__graph && a.__vertex == b.__vertex && a.__slot == b.__slot && a.__incoming == b.__incoming;
    }

    friend constexpr bool
    operator!=(incident_iterator a, incident_iterator b) noexcept
    {
      return !(a == b);
    }
  };

  class neighbor_iterator
  {
    incident_iterator __edge;
    incident_iterator __end;
    const fixed_graph *__graph{};
    vertex_descriptor __vertex{};

  public:
    constexpr neighbor_iterator(const fixed_graph *graph, vertex_descriptor vertex, usize slot, bool incoming) noexcept
        : __edge(graph, vertex, slot, incoming), __end(graph, vertex, graph->__edge_high, incoming), __graph(graph), __vertex(vertex)
    {
    }

    [[nodiscard]] constexpr vertex_descriptor
    operator*() const noexcept
    {
      const edge_descriptor edge = *__edge;
      if constexpr ( is_directed ) {
        if ( __graph->target(edge) == __vertex ) return __graph->source(edge);
        return __graph->target(edge);
      }
      return __graph->opposite(edge, __vertex);
    }

    constexpr neighbor_iterator &
    operator++() noexcept
    {
      ++__edge;
      return *this;
    }

    friend constexpr bool
    operator==(const neighbor_iterator &a, const neighbor_iterator &b) noexcept
    {
      return a.__edge == b.__edge;
    }

    friend constexpr bool
    operator!=(const neighbor_iterator &a, const neighbor_iterator &b) noexcept
    {
      return !(a == b);
    }
  };

public:
  struct vertex_range {
    const fixed_graph *owner;

    [[nodiscard]] constexpr auto
    begin() const noexcept
    {
      return vertex_iterator(owner, 0);
    }

    [[nodiscard]] constexpr auto
    end() const noexcept
    {
      return vertex_iterator(owner, owner->__vertex_high);
    }
  };

  struct edge_range {
    const fixed_graph *owner;

    [[nodiscard]] constexpr auto
    begin() const noexcept
    {
      return edge_iterator(owner, 0);
    }

    [[nodiscard]] constexpr auto
    end() const noexcept
    {
      return edge_iterator(owner, owner->__edge_high);
    }
  };

  struct incident_range {
    const fixed_graph *owner;
    vertex_descriptor vertex;
    bool incoming;

    [[nodiscard]] constexpr auto
    begin() const noexcept
    {
      return incident_iterator(owner, vertex, 0, incoming);
    }

    [[nodiscard]] constexpr auto
    end() const noexcept
    {
      return incident_iterator(owner, vertex, owner->__edge_high, incoming);
    }
  };

  struct neighbor_range {
    const fixed_graph *owner;
    vertex_descriptor vertex;
    bool incoming;

    [[nodiscard]] constexpr auto
    begin() const noexcept
    {
      return neighbor_iterator(owner, vertex, 0, incoming);
    }

    [[nodiscard]] constexpr auto
    end() const noexcept
    {
      return neighbor_iterator(owner, vertex, owner->__edge_high, incoming);
    }
  };

  [[nodiscard]] constexpr vertex_range
  vertices() const noexcept
  {
    return { this };
  }

  [[nodiscard]] constexpr edge_range
  edges() const noexcept
  {
    return { this };
  }

  [[nodiscard]] constexpr incident_range
  out_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, false };
  }

  [[nodiscard]] constexpr incident_range
  in_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, true };
  }

  [[nodiscard]] constexpr neighbor_range
  out_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, false };
  }

  [[nodiscard]] constexpr neighbor_range
  in_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, true };
  }

  [[nodiscard]] constexpr neighbor_range
  neighbors(vertex_descriptor vertex) const noexcept
  {
    return out_neighbors(vertex);
  }

  [[nodiscard]] constexpr usize
  out_degree(vertex_descriptor vertex) const noexcept
  {
    usize count = 0;
    for ( auto edge : out_edges(vertex) ) {
      (void)edge;
      ++count;
    }
    return count;
  }

  [[nodiscard]] constexpr usize
  in_degree(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( !is_directed ) return out_degree(vertex);
    usize count = 0;
    for ( auto edge : in_edges(vertex) ) {
      (void)edge;
      ++count;
    }
    return count;
  }

  [[nodiscard]] constexpr usize
  degree(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( is_directed ) return in_degree(vertex) + out_degree(vertex);
    return out_degree(vertex);
  }

  template<typename Out>
  void
  __micron_print(Out &out) const
  {
    char mode = '\0';
    if constexpr ( requires { out.mode(); } ) mode = out.mode();
    auto print_vertex = [&](vertex_descriptor vertex) {
      if constexpr ( graphs::labeled_bundle<VertexProperty> ) {
        out.elem(vertex_property_unchecked(vertex).label);
        out.raw(" @ ", 3);
      }
      out.elem(vertex);
    };
    if ( mode == 's' ) {
      out.raw("graph{ directed: ", 17);
      out.raw(is_directed ? "true" : "false", is_directed ? 4 : 5);
      out.raw(", simple: ", 10);
      out.raw(is_simple ? "true" : "false", is_simple ? 4 : 5);
      out.raw(", vertices: ", 12);
      out.num(static_cast<u64>(vertices_count()));
      out.raw(", edges: ", 9);
      out.num(static_cast<u64>(edges_count()));
      out.raw(" }", 2);
      return;
    }
    if ( mode == 'a' ) {
      out.raw("graph{ ", 7);
      bool first_vertex = true;
      for ( auto vertex : vertices() ) {
        if ( !first_vertex ) out.raw(", ", 2);
        first_vertex = false;
        print_vertex(vertex);
        out.raw(": { ", 4);
        bool first_neighbor = true;
        for ( auto neighbor : out_neighbors(vertex) ) {
          if ( !first_neighbor ) out.raw(", ", 2);
          first_neighbor = false;
          print_vertex(neighbor);
        }
        out.raw(" }", 2);
      }
      out.raw(" }", 2);
      return;
    }
    out.raw("graph{ directed: ", 17);
    out.raw(is_directed ? "true" : "false", is_directed ? 4 : 5);
    out.raw(", simple: ", 10);
    out.raw(is_simple ? "true" : "false", is_simple ? 4 : 5);
    if ( mode != 'e' ) {
      out.raw(", vertices: { ", 14);
      bool first_vertex = true;
      for ( auto vertex : vertices() ) {
        if ( !first_vertex ) out.raw(", ", 2);
        first_vertex = false;
        print_vertex(vertex);
        out.raw(": ", 2);
        if constexpr ( graphs::labeled_bundle<VertexProperty> )
          out.elem(vertex_property_unchecked(vertex).property);
        else
          out.elem(vertex_property_unchecked(vertex));
      }
      out.raw(" }", 2);
    }
    out.raw(", edges: { ", 11);
    bool first_edge = true;
    for ( auto edge : edges() ) {
      if ( !first_edge ) out.raw(", ", 2);
      first_edge = false;
      print_vertex(edge.source);
      out.raw(is_directed ? " -> " : " -- ", 4);
      print_vertex(edge.target);
      out.raw(": ", 2);
      out.elem(edge.property);
    }
    out.raw(" } }", 4);
  }
};

};      // namespace micron::math
