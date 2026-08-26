//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/actions.hpp"
#include "../../type_traits.hpp"
#include "../../vector/vector.hpp"
#include "graph.hpp"

namespace micron::math
{

namespace graphs
{

template<typename P> class __dense_cell
{
  bool __live{};
  micron::aligned_storage_t<sizeof(P), alignof(P)> __storage;

  [[nodiscard]] P *
  __value() noexcept
  {
    return reinterpret_cast<P *>(micron::addressof(__storage));
  }

  [[nodiscard]] const P *
  __value() const noexcept
  {
    return reinterpret_cast<const P *>(micron::addressof(__storage));
  }

public:
  __dense_cell() noexcept = default;

  __dense_cell(const __dense_cell &other) : __live(other.__live)
  {
    if ( __live ) micron::construct_at(__value(), *other.__value());
  }

  __dense_cell(__dense_cell &&other) noexcept(micron::is_nothrow_move_constructible_v<P>) : __live(other.__live)
  {
    if ( __live ) {
      micron::construct_at(__value(), micron::move(*other.__value()));
      other.reset();
    }
  }

  __dense_cell &
  operator=(const __dense_cell &other)
  {
    if ( this == micron::addressof(other) ) return *this;
    reset();
    if ( other.__live ) {
      micron::construct_at(__value(), *other.__value());
      __live = true;
    }
    return *this;
  }

  __dense_cell &
  operator=(__dense_cell &&other) noexcept(micron::is_nothrow_move_constructible_v<P>)
  {
    if ( this == micron::addressof(other) ) return *this;
    reset();
    if ( other.__live ) {
      micron::construct_at(__value(), micron::move(*other.__value()));
      __live = true;
      other.reset();
    }
    return *this;
  }

  ~__dense_cell() { reset(); }

  [[nodiscard]] bool
  live() const noexcept
  {
    return __live;
  }

  template<typename Q>
  void
  emplace(Q &&value)
  {
    reset();
    micron::construct_at(__value(), micron::forward<Q>(value));
    __live = true;
  }

  void
  reset() noexcept
  {
    if ( !__live ) return;
    __value()->~P();
    __live = false;
  }

  [[nodiscard]] P &
  get() noexcept
  {
    return *__value();
  }

  [[nodiscard]] const P &
  get() const noexcept
  {
    return *__value();
  }
};

};      // namespace graphs

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Loops = graphs::no_loops_t,
         class Alloc = micron::allocator_serial<>>
  requires graphs::direction_policy<Direction> && graphs::loop_policy<Loops>
class dense_adjacency_graph
{
  using cell_type = graphs::__dense_cell<EdgeProperty>;
  using vertex_store = micron::vector<VertexProperty, Alloc, false>;
  using cell_store = micron::vector<cell_type, Alloc, false>;

  [[no_unique_address]] GraphProperty __property;
  vertex_store __vertices;
  cell_store __cells;
  usize __edge_count{};

  [[nodiscard]] static constexpr bool
  __slot_fits(usize slot) noexcept
  {
    using UI = micron::make_unsigned_t<Index>;
    const UI maximum = static_cast<UI>(micron::numeric_limits<Index>::max());
    if constexpr ( sizeof(usize) > sizeof(UI) ) return slot < static_cast<usize>(maximum);
    return static_cast<UI>(slot) < maximum;
  }

  [[nodiscard]] static constexpr bool
  __matrix_fits(usize count) noexcept
  {
    return count == 0 || (count <= micron::numeric_limits<usize>::max() / count && __slot_fits(count * count - 1));
  }

  [[nodiscard]] static constexpr usize
  __position(usize order, usize u, usize v) noexcept
  {
    if constexpr ( is_undirected )
      if ( v < u ) micron::swap(u, v);
    return u * order + v;
  }

  template<typename U>
  [[nodiscard]] static constexpr bool
  __endpoint(U input, Index &value) noexcept
  {
    if constexpr ( micron::is_signed_v<U> )
      if ( input < U{} ) return false;
    const umax_t converted = static_cast<umax_t>(input);
    if ( converted >= static_cast<umax_t>(vertex_id<Index>::invalid_value()) ) return false;
    value = static_cast<Index>(input);
    return true;
  }

  void
  __rebuild(usize new_order, usize removed = micron::numeric_limits<usize>::max())
  {
    const usize old_order = __vertices.size();
    cell_store replacement(new_order * new_order);
    usize count = 0;
    const bool removing = removed < old_order;
    for ( usize u = 0; u < old_order; ++u ) {
      for ( usize v = 0; v < old_order; ++v ) {
        const usize old_position = __position(old_order, u, v);
        if ( !__cells.data()[old_position].live() ) continue;
        if constexpr ( is_undirected )
          if ( v < u ) continue;
        if ( u == removed || v == removed ) continue;
        const usize last = old_order - 1;
        const usize nu = removing && u == last && removed != last ? removed : u;
        const usize nv = removing && v == last && removed != last ? removed : v;
        replacement.data()[__position(new_order, nu, nv)].emplace(micron::move(__cells.data()[old_position].get()));
        ++count;
      }
    }
    __cells = micron::move(replacement);
    __edge_count = count;
  }

  template<typename P>
  [[nodiscard]] edge_insert_result<Index>
  __insert_edge(Index source, Index target, P &&property, bool auto_create)
  {
    const usize u = static_cast<usize>(source);
    const usize v = static_cast<usize>(target);
    const usize high = u > v ? u : v;
    if constexpr ( !allows_loops )
      if ( u == v ) return { graphs::edge_insert_status::self_loop, edge_descriptor::invalid() };
    if ( high >= __vertices.size() && !auto_create ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if ( high >= __vertices.size() ) {
      if constexpr ( !micron::is_default_constructible_v<VertexProperty> )
        return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
      else {
        const usize needed = high + 1;
        if ( !__matrix_fits(needed) ) return { graphs::edge_insert_status::index_overflow, edge_descriptor::invalid() };
        __rebuild(needed);
        while ( __vertices.size() < needed ) __vertices.emplace_back();
      }
    }
    const usize position = __position(__vertices.size(), u, v);
    if ( __cells.data()[position].live() ) return { graphs::edge_insert_status::duplicate, edge_descriptor(static_cast<Index>(position)) };
    __cells.data()[position].emplace(micron::forward<P>(property));
    ++__edge_count;
    return { graphs::edge_insert_status::inserted, edge_descriptor(static_cast<Index>(position)) };
  }

public:
  using vertex_property_type = VertexProperty;
  using edge_property_type = EdgeProperty;
  using graph_property_type = GraphProperty;
  using index_type = Index;
  using direction_type = Direction;
  using multiplicity_type = graphs::simple_t;
  using loop_type = Loops;
  using storage_type = graphs::dense_adjacency_t;
  using allocator_type = Alloc;
  using vertex_descriptor = vertex_id<Index>;
  using edge_descriptor = edge_id<Index>;
  using micron_graph_tag = void;

  static constexpr bool is_directed = micron::is_same_v<Direction, graphs::directed_t>;
  static constexpr bool is_undirected = !is_directed;
  static constexpr bool is_simple = true;
  static constexpr bool allows_parallel_edges = false;
  static constexpr bool allows_loops = micron::is_same_v<Loops, graphs::allow_loops_t>;
  static constexpr bool has_contiguous_slots = true;
  static constexpr bool has_fast_in_adjacency = true;
  static constexpr bool has_matrix_adjacency = true;

  dense_adjacency_graph()
    requires micron::is_default_constructible_v<GraphProperty>
      : __property()
  {
  }

  explicit dense_adjacency_graph(const GraphProperty &property) : __property(property) { }

  explicit dense_adjacency_graph(GraphProperty &&property) : __property(micron::move(property)) { }

  [[nodiscard]] usize
  vertices_count() const noexcept
  {
    return __vertices.size();
  }

  [[nodiscard]] usize
  edges_count() const noexcept
  {
    return __edge_count;
  }

  [[nodiscard]] usize
  vertex_slots() const noexcept
  {
    return __vertices.size();
  }

  [[nodiscard]] usize
  edge_slots() const noexcept
  {
    return __cells.size();
  }

  [[nodiscard]] bool
  empty() const noexcept
  {
    return __vertices.empty();
  }

  [[nodiscard]] usize
  matrix_order() const noexcept
  {
    return __vertices.size();
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

  void
  reserve_vertices(usize)
  {
  }

  void
  reserve_edges(usize)
  {
  }

  void
  reserve_adjacency(vertex_descriptor, usize, usize = 0)
  {
  }

  [[nodiscard]] vertex_descriptor
  add_vertex()
    requires micron::is_default_constructible_v<VertexProperty>
  {
    const usize next = __vertices.size() + 1;
    if ( !__matrix_fits(next) ) return vertex_descriptor::invalid();
    __rebuild(next);
    __vertices.emplace_back();
    return vertex_descriptor(static_cast<Index>(next - 1));
  }

  template<typename P>
    requires micron::is_constructible_v<VertexProperty, P &&>
  [[nodiscard]] vertex_descriptor
  add_vertex(P &&property)
  {
    const usize next = __vertices.size() + 1;
    if ( !__matrix_fits(next) ) return vertex_descriptor::invalid();
    __rebuild(next);
    __vertices.emplace_back(micron::forward<P>(property));
    return vertex_descriptor(static_cast<Index>(next - 1));
  }

  template<typename... Args>
  [[nodiscard]] vertex_descriptor
  emplace_vertex(Args &&...args)
  {
    return add_vertex(VertexProperty(micron::forward<Args>(args)...));
  }

  [[nodiscard]] micron::vector<vertex_descriptor, Alloc, false>
  add_vertices(usize count)
    requires micron::is_default_constructible_v<VertexProperty>
  {
    micron::vector<vertex_descriptor, Alloc, false> result;
    if ( count > micron::numeric_limits<usize>::max() - __vertices.size() || !__matrix_fits(__vertices.size() + count) ) return result;
    result.reserve(count);
    for ( usize i = 0; i < count; ++i ) result.push_back(add_vertex());
    return result;
  }

  [[nodiscard]] bool
  has_vertex(vertex_descriptor vertex) const noexcept
  {
    return vertex.valid() && static_cast<usize>(vertex.value) < __vertices.size();
  }

  template<micron::integral U>
  [[nodiscard]] bool
  has_vertex(U vertex) const noexcept
  {
    Index value{};
    return __endpoint(vertex, value) && static_cast<usize>(value) < __vertices.size();
  }

  [[nodiscard]] bool
  has_edge(edge_descriptor edge) const noexcept
  {
    return edge.valid() && static_cast<usize>(edge.value) < __cells.size() && __cells.data()[static_cast<usize>(edge.value)].live();
  }

  [[nodiscard]] edge_descriptor
  find_edge(vertex_descriptor source, vertex_descriptor target) const noexcept
  {
    if ( !has_vertex(source) || !has_vertex(target) ) return edge_descriptor::invalid();
    const usize position = __position(__vertices.size(), static_cast<usize>(source.value), static_cast<usize>(target.value));
    return __cells.data()[position].live() ? edge_descriptor(static_cast<Index>(position)) : edge_descriptor::invalid();
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_descriptor
  find_edge(U source, V target) const noexcept
  {
    Index u{}, v{};
    if ( !__endpoint(source, u) || !__endpoint(target, v) ) return edge_descriptor::invalid();
    return find_edge(vertex_descriptor(u), vertex_descriptor(v));
  }

  template<typename U, typename V>
  [[nodiscard]] bool
  has_edge(U source, V target) const noexcept
  {
    return find_edge(source, target).valid();
  }

  [[nodiscard]] bool
  matrix_has_edge(vertex_descriptor source, vertex_descriptor target) const noexcept
  {
    return has_edge(source, target);
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U source, V target)
  {
    Index u{}, v{};
    if ( !__endpoint(source, u) || !__endpoint(target, v) )
      return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if constexpr ( micron::is_default_constructible_v<EdgeProperty> )
      return __insert_edge(u, v, EdgeProperty{}, true);
    else
      return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
  }

  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor source, vertex_descriptor target)
  {
    if ( !source.valid() || !target.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if constexpr ( micron::is_default_constructible_v<EdgeProperty> )
      return __insert_edge(source.value, target.value, EdgeProperty{}, false);
    else
      return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
  }

  template<micron::integral U, micron::integral V, typename P>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U source, V target, P &&property)
  {
    Index u{}, v{};
    if ( !__endpoint(source, u) || !__endpoint(target, v) )
      return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    return __insert_edge(u, v, EdgeProperty(micron::forward<P>(property)), true);
  }

  template<typename P>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor source, vertex_descriptor target, P &&property)
  {
    if ( !source.valid() || !target.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    return __insert_edge(source.value, target.value, EdgeProperty(micron::forward<P>(property)), false);
  }

  [[nodiscard]] bool
  remove_edge(edge_descriptor edge) noexcept
  {
    if ( !has_edge(edge) ) return false;
    __cells.data()[static_cast<usize>(edge.value)].reset();
    --__edge_count;
    return true;
  }

  template<typename U, typename V>
  [[nodiscard]] bool
  remove_edge(U source, V target) noexcept
  {
    return remove_edge(find_edge(source, target));
  }

  [[nodiscard]] usize
  remove_edges(vertex_descriptor source, vertex_descriptor target) noexcept
  {
    return remove_edge(find_edge(source, target)) ? 1 : 0;
  }

  [[nodiscard]] bool
  remove_vertex(vertex_descriptor vertex)
  {
    if ( !has_vertex(vertex) ) return false;
    const usize erased = static_cast<usize>(vertex.value);
    const usize last = __vertices.size() - 1;
    __rebuild(last, erased);
    if ( erased != last ) __vertices.data()[erased] = micron::move(__vertices.data()[last]);
    __vertices.pop_back();
    return true;
  }

  template<micron::integral U>
  [[nodiscard]] bool
  remove_vertex(U vertex)
  {
    Index value{};
    return __endpoint(vertex, value) && remove_vertex(vertex_descriptor(value));
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
    return __cells.data()[static_cast<usize>(edge.value)].get();
  }

  [[nodiscard]] const EdgeProperty &
  edge_property_unchecked(edge_descriptor edge) const noexcept
  {
    return __cells.data()[static_cast<usize>(edge.value)].get();
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
    return has_edge(edge) ? micron::addressof(__cells.data()[static_cast<usize>(edge.value)].get()) : nullptr;
  }

  [[nodiscard]] const EdgeProperty *
  try_edge_property(edge_descriptor edge) const noexcept
  {
    return has_edge(edge) ? micron::addressof(__cells.data()[static_cast<usize>(edge.value)].get()) : nullptr;
  }

  [[nodiscard]] vertex_descriptor
  source(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(static_cast<Index>(static_cast<usize>(edge.value) / __vertices.size()));
  }

  [[nodiscard]] vertex_descriptor
  target(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(static_cast<Index>(static_cast<usize>(edge.value) % __vertices.size()));
  }

  [[nodiscard]] vertex_descriptor
  opposite(edge_descriptor edge, vertex_descriptor vertex) const noexcept
  {
    const vertex_descriptor u = source(edge);
    const vertex_descriptor v = target(edge);
    return u == vertex ? v : u;
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
    explicit vertex_iterator(Index value) : __value(value) { }

    [[nodiscard]] vertex_descriptor
    operator*() const noexcept
    {
      return vertex_descriptor(__value);
    }

    vertex_iterator &
    operator++() noexcept
    {
      ++__value;
      return *this;
    }

    friend bool
    operator==(vertex_iterator a, vertex_iterator b) noexcept
    {
      return a.__value == b.__value;
    }
  };

public:
  template<bool Const> struct basic_edge_reference {
    using property_reference = micron::conditional_t<Const, const EdgeProperty &, EdgeProperty &>;
    edge_descriptor id;
    vertex_descriptor source;
    vertex_descriptor target;
    property_reference property;
  };

  using edge_reference = basic_edge_reference<false>;
  using const_edge_reference = basic_edge_reference<true>;

private:
  template<bool Const> class edge_iterator
  {
    using owner_type = micron::conditional_t<Const, const dense_adjacency_graph, dense_adjacency_graph>;
    owner_type *__owner{};
    usize __position{};

    void
    __skip() noexcept
    {
      while ( __position < __owner->__cells.size() && !__owner->__cells.data()[__position].live() ) ++__position;
    }

  public:
    edge_iterator(owner_type *owner, usize position) : __owner(owner), __position(position) { __skip(); }

    [[nodiscard]] auto
    operator*() const noexcept
    {
      return basic_edge_reference<Const>{ edge_descriptor(static_cast<Index>(__position)),
                                          vertex_descriptor(static_cast<Index>(__position / __owner->__vertices.size())),
                                          vertex_descriptor(static_cast<Index>(__position % __owner->__vertices.size())),
                                          __owner->__cells.data()[__position].get() };
    }

    edge_iterator &
    operator++() noexcept
    {
      ++__position;
      __skip();
      return *this;
    }

    friend bool
    operator==(const edge_iterator &a, const edge_iterator &b) noexcept
    {
      return a.__owner == b.__owner && a.__position == b.__position;
    }
  };

  template<bool Neighbors> class incidence_iterator
  {
    const dense_adjacency_graph *__owner{};
    usize __vertex{};
    usize __position{};
    bool __incoming{};

    void
    __skip() noexcept
    {
      const usize order = __owner->__vertices.size();
      const usize end = order + (is_undirected && allows_loops ? 1 : 0);
      while ( __position < end ) {
        const usize other = __position < order ? __position : __vertex;
        if ( __owner->has_edge(vertex_descriptor(static_cast<Index>(__incoming && is_directed ? other : __vertex)),
                               vertex_descriptor(static_cast<Index>(__incoming && is_directed ? __vertex : other))) )
          break;
        ++__position;
      }
    }

  public:
    incidence_iterator(const dense_adjacency_graph *owner, usize vertex, usize position, bool incoming)
        : __owner(owner), __vertex(vertex), __position(position), __incoming(incoming)
    {
      if ( owner ) __skip();
    }

    [[nodiscard]] auto
    operator*() const noexcept
    {
      const usize order = __owner->__vertices.size();
      const usize other = __position < order ? __position : __vertex;
      const vertex_descriptor u(static_cast<Index>(__incoming && is_directed ? other : __vertex));
      const vertex_descriptor v(static_cast<Index>(__incoming && is_directed ? __vertex : other));
      if constexpr ( Neighbors )
        return vertex_descriptor(static_cast<Index>(other));
      else
        return __owner->find_edge(u, v);
    }

    incidence_iterator &
    operator++() noexcept
    {
      ++__position;
      __skip();
      return *this;
    }

    friend bool
    operator==(const incidence_iterator &a, const incidence_iterator &b) noexcept
    {
      return a.__owner == b.__owner && a.__vertex == b.__vertex && a.__position == b.__position && a.__incoming == b.__incoming;
    }
  };

public:
  struct vertex_range {
    usize count{};

    [[nodiscard]] vertex_iterator
    begin() const noexcept
    {
      return vertex_iterator(Index{});
    }

    [[nodiscard]] vertex_iterator
    end() const noexcept
    {
      return vertex_iterator(static_cast<Index>(count));
    }
  };

  template<bool Const> struct basic_edge_range {
    using owner_type = micron::conditional_t<Const, const dense_adjacency_graph, dense_adjacency_graph>;
    owner_type *owner{};

    [[nodiscard]] edge_iterator<Const>
    begin() const noexcept
    {
      return edge_iterator<Const>(owner, 0);
    }

    [[nodiscard]] edge_iterator<Const>
    end() const noexcept
    {
      return edge_iterator<Const>(owner, owner->__cells.size());
    }
  };

  template<bool Neighbors> struct incidence_range {
    const dense_adjacency_graph *owner{};
    usize vertex{};
    bool incoming{};
    bool valid{};

    [[nodiscard]] incidence_iterator<Neighbors>
    begin() const noexcept
    {
      return valid ? incidence_iterator<Neighbors>(owner, vertex, 0, incoming) : incidence_iterator<Neighbors>(nullptr, 0, 0, false);
    }

    [[nodiscard]] incidence_iterator<Neighbors>
    end() const noexcept
    {
      return valid ? incidence_iterator<Neighbors>(owner, vertex, owner->__vertices.size() + (is_undirected && allows_loops ? 1 : 0),
                                                   incoming)
                   : incidence_iterator<Neighbors>(nullptr, 0, 0, false);
    }
  };

  [[nodiscard]] vertex_range
  vertices() const noexcept
  {
    return { __vertices.size() };
  }

  [[nodiscard]] basic_edge_range<false>
  edges() noexcept
  {
    return { this };
  }

  [[nodiscard]] basic_edge_range<true>
  edges() const noexcept
  {
    return { this };
  }

  [[nodiscard]] incidence_range<true>
  out_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, static_cast<usize>(vertex.value), false, has_vertex(vertex) };
  }

  [[nodiscard]] incidence_range<true>
  in_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, static_cast<usize>(vertex.value), is_directed, has_vertex(vertex) };
  }

  [[nodiscard]] incidence_range<true>
  neighbors(vertex_descriptor vertex) const noexcept
  {
    return out_neighbors(vertex);
  }

  [[nodiscard]] incidence_range<false>
  out_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, static_cast<usize>(vertex.value), false, has_vertex(vertex) };
  }

  [[nodiscard]] incidence_range<false>
  in_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, static_cast<usize>(vertex.value), is_directed, has_vertex(vertex) };
  }

  [[nodiscard]] usize
  out_degree(vertex_descriptor vertex) const noexcept
  {
    usize count = 0;
    for ( auto edge : out_edges(vertex) ) {
      (void)edge;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor vertex) const noexcept
  {
    usize count = 0;
    for ( auto edge : in_edges(vertex) ) {
      (void)edge;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  degree(vertex_descriptor vertex) const noexcept
  {
    return is_directed ? out_degree(vertex) + in_degree(vertex) : out_degree(vertex);
  }

  void
  clear()
  {
    __cells.clear();
    __vertices.clear();
    __edge_count = 0;
  }

  [[nodiscard]] auto
  freeze() const
  {
    return graphs::freeze(*this);
  }

  [[nodiscard]] auto
  freeze_bidirectional() const
  {
    return graphs::freeze_bidirectional(*this);
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
};

};      // namespace micron::math
