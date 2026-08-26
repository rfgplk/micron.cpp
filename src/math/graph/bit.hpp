//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "graph.hpp"

namespace micron::math
{

template<class VertexProperty = empty_property, class GraphProperty = empty_property, micron::integral Index = u32,
         class Direction = graphs::undirected_t, class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
  requires graphs::direction_policy<Direction> && graphs::loop_policy<Loops>
class bit_adjacency_graph
{
  [[no_unique_address]] GraphProperty __property;
  micron::vector<VertexProperty, Alloc, false> __vertices;
  micron::vector<u64, Alloc, false> __bits;
  usize __edge_count{};

  [[nodiscard]] static constexpr usize
  __words(usize order) noexcept
  {
    return (order + 63) / 64;
  }

  [[nodiscard]] static constexpr usize
  __word(usize order, usize u, usize v) noexcept
  {
    return u * __words(order) + v / 64;
  }

  [[nodiscard]] static constexpr u64
  __mask(usize v) noexcept
  {
    return u64(1) << (v & 63);
  }

  [[nodiscard]] static constexpr bool
  __slot_fits(usize slot) noexcept
  {
    using UI = micron::make_unsigned_t<Index>;
    const UI maximum = static_cast<UI>(micron::numeric_limits<Index>::max());
    if constexpr ( sizeof(usize) > sizeof(UI) ) return slot < static_cast<usize>(maximum);
    return static_cast<UI>(slot) < maximum;
  }

  [[nodiscard]] static constexpr bool
  __matrix_fits(usize order) noexcept
  {
    return order == 0
           || (order <= micron::numeric_limits<usize>::max() / order && __slot_fits(order * order - 1)
               && order <= micron::numeric_limits<usize>::max() / __words(order));
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

  [[nodiscard]] bool
  __test(usize order, usize u, usize v) const noexcept
  {
    return (__bits.data()[__word(order, u, v)] & __mask(v)) != 0;
  }

  void
  __set(usize order, usize u, usize v) noexcept
  {
    __bits.data()[__word(order, u, v)] |= __mask(v);
  }

  void
  __clear(usize order, usize u, usize v) noexcept
  {
    __bits.data()[__word(order, u, v)] &= ~__mask(v);
  }

  void
  __rebuild(usize new_order, usize removed = micron::numeric_limits<usize>::max())
  {
    const usize old_order = __vertices.size();
    micron::vector<u64, Alloc, false> replacement(new_order * __words(new_order), u64(0));
    const usize last = old_order ? old_order - 1 : 0;
    const bool removing = removed < old_order;
    usize count = 0;
    for ( usize u = 0; u < old_order; ++u ) {
      for ( usize v = 0; v < old_order; ++v ) {
        if ( !__test(old_order, u, v) ) continue;
        if constexpr ( is_undirected )
          if ( v < u ) continue;
        if ( u == removed || v == removed ) continue;
        const usize nu = removing && u == last && removed != last ? removed : u;
        const usize nv = removing && v == last && removed != last ? removed : v;
        replacement.data()[nu * __words(new_order) + nv / 64] |= __mask(nv);
        if constexpr ( is_undirected ) replacement.data()[nv * __words(new_order) + nu / 64] |= __mask(nu);
        ++count;
      }
    }
    __bits = micron::move(replacement);
    __edge_count = count;
  }

  [[nodiscard]] edge_id<Index>
  __descriptor(usize u, usize v) const noexcept
  {
    if constexpr ( is_undirected )
      if ( v < u ) micron::swap(u, v);
    return edge_id<Index>(static_cast<Index>(u * __vertices.size() + v));
  }

  [[nodiscard]] edge_insert_result<Index>
  __insert_edge(Index source, Index target, bool auto_create)
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
    if ( __test(__vertices.size(), u, v) ) return { graphs::edge_insert_status::duplicate, __descriptor(u, v) };
    __set(__vertices.size(), u, v);
    if constexpr ( is_undirected ) __set(__vertices.size(), v, u);
    ++__edge_count;
    return { graphs::edge_insert_status::inserted, __descriptor(u, v) };
  }

public:
  using vertex_property_type = VertexProperty;
  using edge_property_type = empty_property;
  using graph_property_type = GraphProperty;
  using index_type = Index;
  using direction_type = Direction;
  using multiplicity_type = graphs::simple_t;
  using loop_type = Loops;
  using storage_type = graphs::bit_adjacency_t;
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
  static constexpr bool has_fast_in_adjacency = is_undirected;
  static constexpr bool has_matrix_adjacency = true;
  static constexpr bool has_bitset_neighbors = true;

  struct bit_row {
    const u64 *data{};
    usize words{};
    usize bits{};
  };

private:
  static empty_property &
  __empty() noexcept
  {
    static empty_property property;
    return property;
  }

public:
  bit_adjacency_graph()
    requires micron::is_default_constructible_v<GraphProperty>
      : __property()
  {
  }

  explicit bit_adjacency_graph(const GraphProperty &property) : __property(property) { }

  explicit bit_adjacency_graph(GraphProperty &&property) : __property(micron::move(property)) { }

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
    return __vertices.size() * __vertices.size();
  }

  [[nodiscard]] usize
  matrix_order() const noexcept
  {
    return __vertices.size();
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
    if ( !edge.valid() || static_cast<usize>(edge.value) >= edge_slots() || __vertices.empty() ) return false;
    const usize u = static_cast<usize>(edge.value) / __vertices.size();
    const usize v = static_cast<usize>(edge.value) % __vertices.size();
    if constexpr ( is_undirected )
      if ( v < u ) return false;
    return __test(__vertices.size(), u, v);
  }

  [[nodiscard]] edge_descriptor
  find_edge(vertex_descriptor source, vertex_descriptor target) const noexcept
  {
    if ( !has_vertex(source) || !has_vertex(target) ) return edge_descriptor::invalid();
    return __test(__vertices.size(), static_cast<usize>(source.value), static_cast<usize>(target.value))
               ? __descriptor(static_cast<usize>(source.value), static_cast<usize>(target.value))
               : edge_descriptor::invalid();
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

  [[nodiscard]] bit_row
  neighbor_words(vertex_descriptor vertex) const noexcept
  {
    if ( !has_vertex(vertex) ) return {};
    const usize words = __words(__vertices.size());
    return { __bits.data() + static_cast<usize>(vertex.value) * words, words, __vertices.size() };
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U source, V target)
  {
    Index u{}, v{};
    if ( !__endpoint(source, u) || !__endpoint(target, v) )
      return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    return __insert_edge(u, v, true);
  }

  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor source, vertex_descriptor target)
  {
    if ( !source.valid() || !target.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    return __insert_edge(source.value, target.value, false);
  }

  template<typename P>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor source, vertex_descriptor target, P &&)
  {
    return add_edge(source, target);
  }

  [[nodiscard]] bool
  remove_edge(edge_descriptor edge) noexcept
  {
    if ( !has_edge(edge) ) return false;
    const usize order = __vertices.size();
    const usize u = static_cast<usize>(edge.value) / order;
    const usize v = static_cast<usize>(edge.value) % order;
    __clear(order, u, v);
    if constexpr ( is_undirected ) __clear(order, v, u);
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

  [[nodiscard]] empty_property &
  edge_property_unchecked(edge_descriptor) noexcept
  {
    return __empty();
  }

  [[nodiscard]] const empty_property &
  edge_property_unchecked(edge_descriptor) const noexcept
  {
    return __empty();
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

  [[nodiscard]] empty_property *
  try_edge_property(edge_descriptor edge) noexcept
  {
    return has_edge(edge) ? micron::addressof(__empty()) : nullptr;
  }

  [[nodiscard]] const empty_property *
  try_edge_property(edge_descriptor edge) const noexcept
  {
    return has_edge(edge) ? micron::addressof(__empty()) : nullptr;
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
    const auto u = source(edge);
    const auto v = target(edge);
    return u == vertex ? v : u;
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
  struct const_edge_reference {
    edge_descriptor id;
    vertex_descriptor source;
    vertex_descriptor target;
    const empty_property &property;
  };

private:
  class edge_iterator
  {
    const bit_adjacency_graph *__owner{};
    usize __position{};

    void
    __skip() noexcept
    {
      while ( __position < __owner->edge_slots() && !__owner->has_edge(edge_descriptor(static_cast<Index>(__position))) ) ++__position;
    }

  public:
    edge_iterator(const bit_adjacency_graph *owner, usize position) : __owner(owner), __position(position) { __skip(); }

    [[nodiscard]] const_edge_reference
    operator*() const noexcept
    {
      const edge_descriptor edge(static_cast<Index>(__position));
      return { edge, __owner->source(edge), __owner->target(edge), __empty() };
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
    const bit_adjacency_graph *__owner{};
    usize __vertex{};
    usize __position{};
    bool __incoming{};
    usize __next_word{};
    u64 __remaining{};
    bool __repeat_loop{};

    [[nodiscard]] bool
    __row_access() const noexcept
    {
      return !__incoming || !is_directed;
    }

    void
    __load_word() noexcept
    {
      const usize order = __owner->__vertices.size();
      const usize words = __words(order);
      while ( __next_word < words ) {
        const usize word = __next_word++;
        __remaining = __owner->__bits.data()[__word(order, __vertex, word * 64)];
        if ( __remaining ) {
          __position = word * 64 + static_cast<usize>(__builtin_ctzll(__remaining));
          if ( __position < order ) {
            if constexpr ( is_undirected && allows_loops ) __repeat_loop = __position == __vertex;
            return;
          }
          __remaining = 0;
        }
      }
      __position = order;
      __repeat_loop = false;
    }

    void
    __skip() noexcept
    {
      const usize order = __owner->__vertices.size();
      if ( __row_access() ) {
        __load_word();
        return;
      }
      while ( __position < order ) {
        const bool present = __owner->__test(order, __position, __vertex);
        if ( present ) break;
        ++__position;
      }
    }

  public:
    incidence_iterator(const bit_adjacency_graph *owner, usize vertex, usize position, bool incoming)
        : __owner(owner), __vertex(vertex), __position(position), __incoming(incoming)
    {
      if ( owner && position < owner->__vertices.size() ) __skip();
    }

    [[nodiscard]] auto
    operator*() const noexcept
    {
      if constexpr ( Neighbors )
        return vertex_descriptor(static_cast<Index>(__position));
      else {
        const usize u = __incoming && is_directed ? __position : __vertex;
        const usize v = __incoming && is_directed ? __vertex : __position;
        return __owner->__descriptor(u, v);
      }
    }

    incidence_iterator &
    operator++() noexcept
    {
      if ( __row_access() ) {
        if constexpr ( is_undirected && allows_loops )
          if ( __repeat_loop ) {
            __repeat_loop = false;
            return *this;
          }
        __remaining &= __remaining - 1;
        if ( __remaining ) {
          __position = (__next_word - 1) * 64 + static_cast<usize>(__builtin_ctzll(__remaining));
          if constexpr ( is_undirected && allows_loops ) __repeat_loop = __position == __vertex;
        } else
          __load_word();
      } else {
        ++__position;
        __skip();
      }
      return *this;
    }

    friend bool
    operator==(const incidence_iterator &a, const incidence_iterator &b) noexcept
    {
      return a.__owner == b.__owner && a.__vertex == b.__vertex && a.__position == b.__position && a.__incoming == b.__incoming
             && a.__repeat_loop == b.__repeat_loop;
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

  struct edge_range {
    const bit_adjacency_graph *owner{};

    [[nodiscard]] edge_iterator
    begin() const noexcept
    {
      return edge_iterator(owner, 0);
    }

    [[nodiscard]] edge_iterator
    end() const noexcept
    {
      return edge_iterator(owner, owner->edge_slots());
    }
  };

  template<bool Neighbors> struct incidence_range {
    const bit_adjacency_graph *owner{};
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
      return valid ? incidence_iterator<Neighbors>(owner, vertex, owner->__vertices.size(), incoming)
                   : incidence_iterator<Neighbors>(nullptr, 0, 0, false);
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
    const bit_row row = neighbor_words(vertex);
    for ( usize i = 0; i < row.words; ++i ) count += static_cast<usize>(__builtin_popcountll(row.data[i]));
    if constexpr ( is_undirected && allows_loops )
      if ( has_edge(vertex, vertex) ) ++count;
    return count;
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor vertex) const noexcept
  {
    if constexpr ( is_undirected ) return out_degree(vertex);
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
    __bits.clear();
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
