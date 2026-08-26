//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../numerics.hpp"
#include "../../vector/vector.hpp"
#include "graph.hpp"

namespace micron::math
{

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>, class Storage = graphs::csr_t>
  requires graphs::direction_policy<Direction> && graphs::multiplicity_policy<Multiplicity> && graphs::loop_policy<Loops>
           && (micron::is_same_v<Storage, graphs::csr_t> || micron::is_same_v<Storage, graphs::bidirectional_csr_t>)
class basic_csr_graph
{
  struct edge_slot {
    Index source;
    Index target;
    [[no_unique_address]] EdgeProperty property;

    template<typename P> edge_slot(Index u, Index v, P &&value) : source(u), target(v), property(micron::forward<P>(value)) { }
  };

  struct arc {
    Index edge{};
    Index neighbor{};
  };

  [[no_unique_address]] GraphProperty __property;
  micron::vector<VertexProperty, Alloc, false> __vertices;
  micron::vector<edge_slot, Alloc, false> __edges;
  micron::vector<usize, Alloc, false> __out_outer;
  micron::vector<arc, Alloc, false> __out_arcs;
  micron::vector<usize, Alloc, false> __in_outer;
  micron::vector<arc, Alloc, false> __in_arcs;

  static constexpr bool __has_in_index
      = micron::is_same_v<Direction, graphs::directed_t> && micron::is_same_v<Storage, graphs::bidirectional_csr_t>;

  using mapping_vertices = micron::vector<vertex_id<Index>, micron::allocator_serial<>, false>;
  using mapping_edges = micron::vector<edge_id<Index>, micron::allocator_serial<>, false>;

  template<graphs::graph_model G>
  void
  __build(const G &source, mapping_vertices &dense_to_vertex, mapping_vertices &vertex_remap, mapping_edges &edge_remap)
  {
    dense_to_vertex.clear();
    dense_to_vertex.reserve(source.vertices_count());
    vertex_remap = mapping_vertices(source.vertex_slots(), vertex_descriptor::invalid());
    edge_remap = mapping_edges(source.edge_slots(), edge_descriptor::invalid());
    __vertices.reserve(source.vertices_count());
    __edges.reserve(source.edges_count());

    for ( auto vertex : source.vertices() ) {
      const vertex_descriptor dense(static_cast<Index>(__vertices.size()));
      vertex_remap.data()[static_cast<usize>(vertex.value)] = dense;
      dense_to_vertex.push_back(vertex_descriptor(static_cast<Index>(vertex.value)));
      __vertices.push_back(source.vertex_property_unchecked(vertex));
    }
    for ( auto edge : source.edges() ) {
      const vertex_descriptor u = vertex_remap.data()[static_cast<usize>(edge.source.value)];
      const vertex_descriptor v = vertex_remap.data()[static_cast<usize>(edge.target.value)];
      const edge_descriptor dense(static_cast<Index>(__edges.size()));
      edge_remap.data()[static_cast<usize>(edge.id.value)] = dense;
      __edges.emplace_back(u.value, v.value, edge.property);
    }

    const usize vertices = __vertices.size();
    __out_outer.resize(vertices + 1, usize(0));
    if constexpr ( __has_in_index ) __in_outer.resize(vertices + 1, usize(0));
    for ( usize edge = 0; edge < __edges.size(); ++edge ) {
      const edge_slot &slot = __edges.data()[edge];
      ++__out_outer.data()[static_cast<usize>(slot.source) + 1];
      if constexpr ( __has_in_index )
        ++__in_outer.data()[static_cast<usize>(slot.target) + 1];
      else if constexpr ( is_undirected )
        ++__out_outer.data()[static_cast<usize>(slot.target) + 1];
    }
    for ( usize vertex = 1; vertex <= vertices; ++vertex ) __out_outer.data()[vertex] += __out_outer.data()[vertex - 1];
    __out_arcs.resize(__out_outer.data()[vertices]);
    micron::vector<usize, Alloc, false> out_cursor(vertices, usize(0));
    for ( usize vertex = 0; vertex < vertices; ++vertex ) out_cursor.data()[vertex] = __out_outer.data()[vertex];

    micron::vector<usize, Alloc, false> in_cursor;
    if constexpr ( __has_in_index ) {
      for ( usize vertex = 1; vertex <= vertices; ++vertex ) __in_outer.data()[vertex] += __in_outer.data()[vertex - 1];
      __in_arcs.resize(__in_outer.data()[vertices]);
      in_cursor.resize(vertices, usize(0));
      for ( usize vertex = 0; vertex < vertices; ++vertex ) in_cursor.data()[vertex] = __in_outer.data()[vertex];
    }

    for ( usize edge = 0; edge < __edges.size(); ++edge ) {
      const edge_slot &slot = __edges.data()[edge];
      const usize u = static_cast<usize>(slot.source);
      const usize v = static_cast<usize>(slot.target);
      __out_arcs.data()[out_cursor.data()[u]++] = arc{ static_cast<Index>(edge), slot.target };
      if constexpr ( __has_in_index )
        __in_arcs.data()[in_cursor.data()[v]++] = arc{ static_cast<Index>(edge), slot.source };
      else if constexpr ( is_undirected )
        __out_arcs.data()[out_cursor.data()[v]++] = arc{ static_cast<Index>(edge), slot.source };
    }

    auto sort_rows = [](auto &arcs, const auto &outer, usize row_count) {
      for ( usize row = 0; row < row_count; ++row ) {
        const usize begin = outer.data()[row];
        const usize end = outer.data()[row + 1];
        const usize count = end - begin;
        auto less = [](const arc &left, const arc &right) {
          if ( left.neighbor != right.neighbor ) return left.neighbor < right.neighbor;
          return static_cast<Index>(left.edge) < static_cast<Index>(right.edge);
        };
        auto sift = [&](usize root, usize length) {
          for ( ;; ) {
            usize child = root * 2 + 1;
            if ( child >= length ) break;
            if ( child + 1 < length && less(arcs.data()[begin + child], arcs.data()[begin + child + 1]) ) ++child;
            if ( !less(arcs.data()[begin + root], arcs.data()[begin + child]) ) break;
            micron::swap(arcs.data()[begin + root], arcs.data()[begin + child]);
            root = child;
          }
        };
        for ( usize start = count / 2; start-- > 0; ) sift(start, count);
        for ( usize length = count; length > 1; ) {
          --length;
          micron::swap(arcs.data()[begin], arcs.data()[begin + length]);
          sift(0, length);
        }
      }
    };
    sort_rows(__out_arcs, __out_outer, vertices);
    if constexpr ( __has_in_index ) sort_rows(__in_arcs, __in_outer, vertices);
  }

public:
  using vertex_property_type = VertexProperty;
  using edge_property_type = EdgeProperty;
  using graph_property_type = GraphProperty;
  using index_type = Index;
  using direction_type = Direction;
  using multiplicity_type = Multiplicity;
  using loop_type = Loops;
  using storage_type = Storage;
  using allocator_type = Alloc;
  using vertex_descriptor = vertex_id<Index>;
  using edge_descriptor = edge_id<Index>;
  using micron_graph_tag = void;
  using micron_printable_tag = void;

  static constexpr bool is_directed = micron::is_same_v<Direction, graphs::directed_t>;
  static constexpr bool is_undirected = !is_directed;
  static constexpr bool is_simple = micron::is_same_v<Multiplicity, graphs::simple_t>;
  static constexpr bool allows_parallel_edges = !is_simple;
  static constexpr bool allows_loops = micron::is_same_v<Loops, graphs::allow_loops_t>;
  static constexpr bool has_in_index = __has_in_index;
  static constexpr bool has_contiguous_slots = true;
  static constexpr bool has_fast_in_adjacency = is_undirected || __has_in_index;
  static constexpr bool has_sorted_adjacency = true;

  basic_csr_graph()
    requires micron::is_default_constructible_v<GraphProperty>
      : __property()
  {
  }

  explicit basic_csr_graph(const GraphProperty &property) : __property(property) { }

  explicit basic_csr_graph(GraphProperty &&property) : __property(micron::move(property)) { }

  template<graphs::graph_model G>
    requires micron::is_same_v<typename G::index_type, Index>
  basic_csr_graph(const G &source, mapping_vertices &dense_to_vertex, mapping_vertices &vertex_remap, mapping_edges &edge_remap)
      : __property(source.graph_property())
  {
    __build(source, dense_to_vertex, vertex_remap, edge_remap);
  }

  [[nodiscard]] usize
  vertices_count() const noexcept
  {
    return __vertices.size();
  }

  [[nodiscard]] usize
  edges_count() const noexcept
  {
    return __edges.size();
  }

  [[nodiscard]] usize
  vertex_slots() const noexcept
  {
    return __vertices.size();
  }

  [[nodiscard]] usize
  edge_slots() const noexcept
  {
    return __edges.size();
  }

  [[nodiscard]] bool
  empty() const noexcept
  {
    return __vertices.empty();
  }

  [[nodiscard]] GraphProperty &
  property() noexcept
  {
    return __property;
  }

  [[nodiscard]] const GraphProperty &
  property() const noexcept
  {
    return __property;
  }

  [[nodiscard]] GraphProperty &
  graph_property() noexcept
  {
    return __property;
  }

  [[nodiscard]] const GraphProperty &
  graph_property() const noexcept
  {
    return __property;
  }

  [[nodiscard]] bool
  has_vertex(vertex_descriptor vertex) const noexcept
  {
    return vertex.valid() && static_cast<usize>(vertex.value) < __vertices.size();
  }

  [[nodiscard]] bool
  has_edge(edge_descriptor edge) const noexcept
  {
    return edge.valid() && static_cast<usize>(edge.value) < __edges.size();
  }

  [[nodiscard]] edge_descriptor
  find_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    if ( !has_vertex(u) || !has_vertex(v) ) return edge_descriptor::invalid();
    usize begin = __out_outer.data()[static_cast<usize>(u.value)];
    usize end = __out_outer.data()[static_cast<usize>(u.value) + 1];
    while ( begin < end ) {
      const usize middle = begin + (end - begin) / 2;
      if ( __out_arcs.data()[middle].neighbor < v.value )
        begin = middle + 1;
      else
        end = middle;
    }
    const usize row_end = __out_outer.data()[static_cast<usize>(u.value) + 1];
    if ( begin < row_end && __out_arcs.data()[begin].neighbor == v.value ) return edge_descriptor(__out_arcs.data()[begin].edge);
    return edge_descriptor::invalid();
  }

  [[nodiscard]] bool
  has_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    return find_edge(u, v).valid();
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] bool
  has_edge(U u, V v) const noexcept
  {
    if constexpr ( micron::is_signed_v<U> )
      if ( u < U(0) ) return false;
    if constexpr ( micron::is_signed_v<V> )
      if ( v < V(0) ) return false;
    if ( static_cast<umax_t>(u) >= __vertices.size() || static_cast<umax_t>(v) >= __vertices.size() ) return false;
    return has_edge(vertex_descriptor(static_cast<Index>(u)), vertex_descriptor(static_cast<Index>(v)));
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_descriptor
  find_edge(U u, V v) const noexcept
  {
    return has_edge(u, v) ? find_edge(vertex_descriptor(static_cast<Index>(u)), vertex_descriptor(static_cast<Index>(v)))
                          : edge_descriptor::invalid();
  }

  [[nodiscard]] VertexProperty &
  vertex_property_unchecked(vertex_descriptor vertex) noexcept
  {
    return __vertices.data()[static_cast<usize>(vertex.value)];
  }

  [[nodiscard]] const VertexProperty &
  vertex_property_unchecked(vertex_descriptor vertex) const noexcept
  {
    return __vertices.data()[static_cast<usize>(vertex.value)];
  }

  [[nodiscard]] EdgeProperty &
  edge_property_unchecked(edge_descriptor edge) noexcept
  {
    return __edges.data()[static_cast<usize>(edge.value)].property;
  }

  [[nodiscard]] const EdgeProperty &
  edge_property_unchecked(edge_descriptor edge) const noexcept
  {
    return __edges.data()[static_cast<usize>(edge.value)].property;
  }

  [[nodiscard]] VertexProperty *
  try_vertex_property(vertex_descriptor vertex) noexcept
  {
    return has_vertex(vertex) ? __vertices.data() + static_cast<usize>(vertex.value) : nullptr;
  }

  [[nodiscard]] const VertexProperty *
  try_vertex_property(vertex_descriptor vertex) const noexcept
  {
    return has_vertex(vertex) ? __vertices.data() + static_cast<usize>(vertex.value) : nullptr;
  }

  [[nodiscard]] EdgeProperty *
  try_edge_property(edge_descriptor edge) noexcept
  {
    return has_edge(edge) ? micron::addressof(__edges.data()[static_cast<usize>(edge.value)].property) : nullptr;
  }

  [[nodiscard]] const EdgeProperty *
  try_edge_property(edge_descriptor edge) const noexcept
  {
    return has_edge(edge) ? micron::addressof(__edges.data()[static_cast<usize>(edge.value)].property) : nullptr;
  }

  [[nodiscard]] vertex_descriptor
  source(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(__edges.data()[static_cast<usize>(edge.value)].source);
  }

  [[nodiscard]] vertex_descriptor
  target(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(__edges.data()[static_cast<usize>(edge.value)].target);
  }

  [[nodiscard]] vertex_descriptor
  opposite(edge_descriptor edge, vertex_descriptor vertex) const noexcept
  {
    const edge_slot &slot = __edges.data()[static_cast<usize>(edge.value)];
    return vertex_descriptor(slot.source == vertex.value ? slot.target : slot.source);
  }

  template<typename T = EdgeProperty>
    requires graphs::weighted_bundle<T>
  [[nodiscard]] decltype(auto)
  edge_weight(edge_descriptor edge) noexcept
  {
    return (edge_property_unchecked(edge).weight);
  }

  template<typename T = EdgeProperty>
    requires graphs::weighted_bundle<T>
  [[nodiscard]] decltype(auto)
  edge_weight(edge_descriptor edge) const noexcept
  {
    return (edge_property_unchecked(edge).weight);
  }

private:
  class vertex_iterator
  {
    Index __value{};

  public:
    constexpr explicit vertex_iterator(Index value) noexcept : __value(value) { }

    [[nodiscard]] constexpr vertex_descriptor
    operator*() const noexcept
    {
      return vertex_descriptor(__value);
    }

    constexpr vertex_iterator &
    operator++() noexcept
    {
      ++__value;
      return *this;
    }

    friend constexpr bool
    operator==(vertex_iterator a, vertex_iterator b) noexcept
    {
      return a.__value == b.__value;
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
    const basic_csr_graph *__graph{};
    usize __position{};

  public:
    edge_iterator(const basic_csr_graph *graph, usize position) noexcept : __graph(graph), __position(position) { }

    [[nodiscard]] const_edge_reference
    operator*() const noexcept
    {
      const edge_slot &edge = __graph->__edges.data()[__position];
      return { edge_descriptor(static_cast<Index>(__position)), vertex_descriptor(edge.source), vertex_descriptor(edge.target),
               edge.property };
    }

    edge_iterator &
    operator++() noexcept
    {
      ++__position;
      return *this;
    }

    friend bool
    operator==(edge_iterator a, edge_iterator b) noexcept
    {
      return a.__graph == b.__graph && a.__position == b.__position;
    }

    friend bool
    operator!=(edge_iterator a, edge_iterator b) noexcept
    {
      return !(a == b);
    }
  };

  template<bool Neighbors> class arc_iterator
  {
    const arc *__data{};
    usize __position{};

  public:
    arc_iterator(const arc *data, usize position) noexcept : __data(data), __position(position) { }

    [[nodiscard]] auto
    operator*() const noexcept
    {
      if constexpr ( Neighbors )
        return vertex_descriptor(__data[__position].neighbor);
      else
        return edge_descriptor(__data[__position].edge);
    }

    arc_iterator &
    operator++() noexcept
    {
      ++__position;
      return *this;
    }

    friend bool
    operator==(arc_iterator a, arc_iterator b) noexcept
    {
      return a.__data == b.__data && a.__position == b.__position;
    }

    friend bool
    operator!=(arc_iterator a, arc_iterator b) noexcept
    {
      return !(a == b);
    }
  };

  template<bool Neighbors> class incoming_scan_iterator
  {
    const basic_csr_graph *__graph{};
    usize __position{};
    Index __target{};

    void
    __skip() noexcept
    {
      while ( __position < __graph->__edges.size() && __graph->__edges.data()[__position].target != __target ) ++__position;
    }

  public:
    incoming_scan_iterator(const basic_csr_graph *graph, usize position, Index target) noexcept
        : __graph(graph), __position(position), __target(target)
    {
      __skip();
    }

    [[nodiscard]] auto
    operator*() const noexcept
    {
      if constexpr ( Neighbors )
        return vertex_descriptor(__graph->__edges.data()[__position].source);
      else
        return edge_descriptor(static_cast<Index>(__position));
    }

    incoming_scan_iterator &
    operator++() noexcept
    {
      ++__position;
      __skip();
      return *this;
    }

    friend bool
    operator==(incoming_scan_iterator a, incoming_scan_iterator b) noexcept
    {
      return a.__graph == b.__graph && a.__position == b.__position && a.__target == b.__target;
    }

    friend bool
    operator!=(incoming_scan_iterator a, incoming_scan_iterator b) noexcept
    {
      return !(a == b);
    }
  };

public:
  struct vertex_range {
    usize count{};

    [[nodiscard]] vertex_iterator
    begin() const noexcept
    {
      return vertex_iterator(Index(0));
    }

    [[nodiscard]] vertex_iterator
    end() const noexcept
    {
      return vertex_iterator(static_cast<Index>(count));
    }
  };

  struct edge_range {
    const basic_csr_graph *graph{};

    [[nodiscard]] edge_iterator
    begin() const noexcept
    {
      return edge_iterator(graph, 0);
    }

    [[nodiscard]] edge_iterator
    end() const noexcept
    {
      return edge_iterator(graph, graph->__edges.size());
    }
  };

  template<bool Neighbors> struct arc_range {
    const arc *data{};
    usize begin_position{};
    usize end_position{};

    [[nodiscard]] arc_iterator<Neighbors>
    begin() const noexcept
    {
      return arc_iterator<Neighbors>(data, begin_position);
    }

    [[nodiscard]] arc_iterator<Neighbors>
    end() const noexcept
    {
      return arc_iterator<Neighbors>(data, end_position);
    }
  };

  template<bool Neighbors> struct incoming_scan_range {
    const basic_csr_graph *graph{};
    Index target{};

    [[nodiscard]] incoming_scan_iterator<Neighbors>
    begin() const noexcept
    {
      return incoming_scan_iterator<Neighbors>(graph, 0, target);
    }

    [[nodiscard]] incoming_scan_iterator<Neighbors>
    end() const noexcept
    {
      return incoming_scan_iterator<Neighbors>(graph, graph ? graph->__edges.size() : 0, target);
    }
  };

  [[nodiscard]] vertex_range
  vertices() const noexcept
  {
    return { __vertices.size() };
  }

  [[nodiscard]] edge_range
  edges() const noexcept
  {
    return { this };
  }

private:
  template<bool Neighbors>
  [[nodiscard]] arc_range<Neighbors>
  __arc_range(vertex_descriptor vertex, bool incoming) const noexcept
  {
    if ( !has_vertex(vertex) ) return {};
    const usize slot = static_cast<usize>(vertex.value);
    if ( incoming && is_directed ) return { __in_arcs.data(), __in_outer.data()[slot], __in_outer.data()[slot + 1] };
    return { __out_arcs.data(), __out_outer.data()[slot], __out_outer.data()[slot + 1] };
  }

public:
  [[nodiscard]] auto
  out_neighbors(vertex_descriptor vertex) const noexcept
  {
    return __arc_range<true>(vertex, false);
  }

  [[nodiscard]] auto
  in_neighbors(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( is_undirected )
      return __arc_range<true>(vertex, false);
    else if constexpr ( has_in_index )
      return __arc_range<true>(vertex, true);
    else
      return incoming_scan_range<true>{ this, has_vertex(vertex) ? vertex.value : vertex_descriptor::invalid_value() };
  }

  [[nodiscard]] auto
  neighbors(vertex_descriptor vertex) const noexcept
  {
    return out_neighbors(vertex);
  }

  [[nodiscard]] auto
  out_edges(vertex_descriptor vertex) const noexcept
  {
    return __arc_range<false>(vertex, false);
  }

  [[nodiscard]] auto
  in_edges(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( is_undirected )
      return __arc_range<false>(vertex, false);
    else if constexpr ( has_in_index )
      return __arc_range<false>(vertex, true);
    else
      return incoming_scan_range<false>{ this, has_vertex(vertex) ? vertex.value : vertex_descriptor::invalid_value() };
  }

  [[nodiscard]] usize
  out_degree(vertex_descriptor vertex) const noexcept
  {
    if ( !has_vertex(vertex) ) return 0;
    const usize slot = static_cast<usize>(vertex.value);
    return __out_outer.data()[slot + 1] - __out_outer.data()[slot];
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( is_undirected ) return out_degree(vertex);
    if ( !has_vertex(vertex) ) return 0;
    if constexpr ( has_in_index ) {
      const usize slot = static_cast<usize>(vertex.value);
      return __in_outer.data()[slot + 1] - __in_outer.data()[slot];
    } else {
      usize result = 0;
      for ( auto edge : in_edges(vertex) ) {
        (void)edge;
        ++result;
      }
      return result;
    }
  }

  [[nodiscard]] usize
  degree(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( is_directed ) return out_degree(vertex) + in_degree(vertex);
    return out_degree(vertex);
  }

  [[nodiscard]] auto
  thaw() const
  {
    return graphs::thaw(*this);
  }

  [[nodiscard]] auto
  thaw_stable() const
  {
    return graphs::thaw_stable(*this);
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
