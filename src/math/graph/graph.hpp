//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../memory/actions.hpp"
#include "../../memory/allocation/resources.hpp"
#include "../../numerics.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "descriptors.hpp"
#include "tags.hpp"

namespace micron::math
{

namespace graphs
{

template<typename G> [[nodiscard]] auto freeze(const G &graph);

template<typename G> [[nodiscard]] auto freeze_bidirectional(const G &graph);

template<typename G> [[nodiscard]] auto thaw(const G &graph);

template<typename G> [[nodiscard]] auto thaw_stable(const G &graph);

template<micron::integral I> struct bulk_insert_result {
  usize inserted{};
  usize duplicate{};
  usize self_loop{};
  usize invalid_vertex{};
  usize property_required{};
  usize index_overflow{};

  constexpr void
  account(edge_insert_status status) noexcept
  {
    switch ( status ) {
    case edge_insert_status::inserted:
      ++inserted;
      break;
    case edge_insert_status::duplicate:
      ++duplicate;
      break;
    case edge_insert_status::self_loop:
      ++self_loop;
      break;
    case edge_insert_status::invalid_vertex:
      ++invalid_vertex;
      break;
    case edge_insert_status::property_required:
      ++property_required;
      break;
    case edge_insert_status::index_overflow:
      ++index_overflow;
      break;
    }
  }

  [[nodiscard]] constexpr bool
  all_inserted() const noexcept
  {
    return duplicate == 0 && self_loop == 0 && invalid_vertex == 0 && property_required == 0 && index_overflow == 0;
  }
};

template<micron::integral I> struct __graph_arc {
  I edge{};
  I neighbor{};
};

struct __dead_graph_slot_t {
};

template<typename P, micron::integral I, class Alloc> struct __graph_vertex_slot {
  using arc_type = __graph_arc<I>;
  using arc_vector = micron::vector<arc_type, Alloc, false>;

  bool live{ true };
  bool property_live{ true };

  union {
    P property;
  };

  arc_vector out;
  arc_vector in;

  __graph_vertex_slot()
    requires micron::is_default_constructible_v<P>
      : live(true), property_live(true), property(), out(), in()
  {
  }

  explicit __graph_vertex_slot(__dead_graph_slot_t) : live(false), property_live(false), out(), in() { }

  template<typename Q>
    requires micron::is_constructible_v<P, Q &&>
  explicit __graph_vertex_slot(Q &&p) : live(true), property_live(true), property(micron::forward<Q>(p)), out(), in()
  {
  }

  __graph_vertex_slot(const __graph_vertex_slot &other) : live(other.live), property_live(other.property_live), out(other.out), in(other.in)
  {
    if ( property_live ) micron::construct_at(micron::addressof(property), other.property);
  }

  __graph_vertex_slot(__graph_vertex_slot &&other) noexcept(micron::is_nothrow_move_constructible_v<P>)
      : live(other.live), property_live(other.property_live), out(micron::move(other.out)), in(micron::move(other.in))
  {
    if ( property_live ) micron::construct_at(micron::addressof(property), micron::move(other.property));
  }

  __graph_vertex_slot &
  operator=(const __graph_vertex_slot &other)
  {
    if ( this == micron::addressof(other) ) return *this;
    if ( property_live && other.property_live )
      property = other.property;
    else if ( property_live ) {
      micron::destroy_at(micron::addressof(property));
      property_live = false;
    } else if ( other.property_live ) {
      micron::construct_at(micron::addressof(property), other.property);
      property_live = true;
    }
    live = other.live;
    out = other.out;
    in = other.in;
    return *this;
  }

  __graph_vertex_slot &
  operator=(__graph_vertex_slot &&other) noexcept(micron::is_nothrow_move_assignable_v<P> && micron::is_nothrow_move_constructible_v<P>)
  {
    if ( this == micron::addressof(other) ) return *this;
    if ( property_live && other.property_live )
      property = micron::move(other.property);
    else if ( property_live ) {
      micron::destroy_at(micron::addressof(property));
      property_live = false;
    } else if ( other.property_live ) {
      micron::construct_at(micron::addressof(property), micron::move(other.property));
      property_live = true;
    }
    live = other.live;
    out = micron::move(other.out);
    in = micron::move(other.in);
    return *this;
  }

  ~__graph_vertex_slot()
  {
    if ( property_live ) micron::destroy_at(micron::addressof(property));
  }

  void
  make_dead() noexcept
  {
    live = false;
    if ( property_live ) {
      micron::destroy_at(micron::addressof(property));
      property_live = false;
    }
    out.clear();
    in.clear();
  }
};

template<typename P, micron::integral I> struct __graph_edge_slot {
  bool live{ true };
  bool property_live{ true };
  I source{};
  I target{};

  union {
    P property;
  };

  __graph_edge_slot()
    requires micron::is_default_constructible_v<P>
      : live(true), property_live(true), source(), target(), property()
  {
  }

  explicit __graph_edge_slot(__dead_graph_slot_t) : live(false), property_live(false), source(), target() { }

  template<typename Q>
    requires micron::is_constructible_v<P, Q &&>
  __graph_edge_slot(I u, I v, Q &&p) : live(true), property_live(true), source(u), target(v), property(micron::forward<Q>(p))
  {
  }

  __graph_edge_slot(const __graph_edge_slot &other)
      : live(other.live), property_live(other.property_live), source(other.source), target(other.target)
  {
    if ( property_live ) micron::construct_at(micron::addressof(property), other.property);
  }

  __graph_edge_slot(__graph_edge_slot &&other) noexcept(micron::is_nothrow_move_constructible_v<P>)
      : live(other.live), property_live(other.property_live), source(other.source), target(other.target)
  {
    if ( property_live ) micron::construct_at(micron::addressof(property), micron::move(other.property));
  }

  __graph_edge_slot &
  operator=(const __graph_edge_slot &other)
  {
    if ( this == micron::addressof(other) ) return *this;
    if ( property_live && other.property_live )
      property = other.property;
    else if ( property_live ) {
      micron::destroy_at(micron::addressof(property));
      property_live = false;
    } else if ( other.property_live ) {
      micron::construct_at(micron::addressof(property), other.property);
      property_live = true;
    }
    live = other.live;
    source = other.source;
    target = other.target;
    return *this;
  }

  __graph_edge_slot &
  operator=(__graph_edge_slot &&other) noexcept(micron::is_nothrow_move_assignable_v<P> && micron::is_nothrow_move_constructible_v<P>)
  {
    if ( this == micron::addressof(other) ) return *this;
    if ( property_live && other.property_live )
      property = micron::move(other.property);
    else if ( property_live ) {
      micron::destroy_at(micron::addressof(property));
      property_live = false;
    } else if ( other.property_live ) {
      micron::construct_at(micron::addressof(property), micron::move(other.property));
      property_live = true;
    }
    live = other.live;
    source = other.source;
    target = other.target;
    return *this;
  }

  ~__graph_edge_slot()
  {
    if ( property_live ) micron::destroy_at(micron::addressof(property));
  }

  void
  make_dead() noexcept
  {
    live = false;
    if ( property_live ) {
      micron::destroy_at(micron::addressof(property));
      property_live = false;
    }
  }
};

};      // namespace graphs

template<class VertexProperty, class EdgeProperty, class GraphProperty, micron::integral Index, class Direction, class Multiplicity,
         class Loops, class Storage, class Alloc>
  requires graphs::direction_policy<Direction> && graphs::multiplicity_policy<Multiplicity> && graphs::loop_policy<Loops>
           && (micron::is_same_v<Storage, graphs::stable_adjacency_t> || micron::is_same_v<Storage, graphs::compact_adjacency_t>
               || micron::is_same_v<Storage, graphs::edge_list_t>)
class __basic_adjacency_graph
{
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
  static constexpr bool has_contiguous_slots = !micron::is_same_v<Storage, graphs::stable_adjacency_t>;
  static constexpr bool has_fast_in_adjacency = !micron::is_same_v<Storage, graphs::edge_list_t>;

private:
  static constexpr bool __stable = micron::is_same_v<Storage, graphs::stable_adjacency_t>;
  static constexpr bool __edge_list = micron::is_same_v<Storage, graphs::edge_list_t>;
  using vertex_slot = graphs::__graph_vertex_slot<VertexProperty, Index, Alloc>;
  using edge_slot = graphs::__graph_edge_slot<EdgeProperty, Index>;
  using arc_type = graphs::__graph_arc<Index>;
  using vertex_store = micron::vector<vertex_slot, Alloc, false>;
  using edge_store = micron::vector<edge_slot, Alloc, false>;

  [[no_unique_address]] GraphProperty __property;
  vertex_store __vertices;
  edge_store __edges;
  usize __live_vertices{};
  usize __live_edges{};

  template<typename U>
  [[nodiscard]] static constexpr bool
  __endpoint(U value, Index &out) noexcept
  {
    static_assert(micron::is_integral_v<U>);
    if constexpr ( micron::is_signed_v<U> )
      if ( value < U(0) ) return false;

    using UU = micron::make_unsigned_t<U>;
    using UI = micron::make_unsigned_t<Index>;
    const UU u = static_cast<UU>(value);
    const UI imax = static_cast<UI>(micron::numeric_limits<Index>::max());
    if constexpr ( sizeof(UU) > sizeof(UI) ) {
      if ( u >= static_cast<UU>(imax) ) return false;
    } else {
      if ( static_cast<UI>(u) >= imax ) return false;
    }
    if constexpr ( sizeof(UU) > sizeof(usize) ) {
      if ( u > static_cast<UU>(micron::numeric_limits<usize>::max()) ) return false;
    }
    out = static_cast<Index>(u);
    return true;
  }

  template<typename U, typename V>
  [[nodiscard]] static constexpr graphs::edge_insert_status
  __endpoint_failure(U u, V v) noexcept
  {
    if constexpr ( micron::is_signed_v<U> )
      if ( u < U(0) ) return graphs::edge_insert_status::invalid_vertex;
    if constexpr ( micron::is_signed_v<V> )
      if ( v < V(0) ) return graphs::edge_insert_status::invalid_vertex;
    return graphs::edge_insert_status::index_overflow;
  }

  [[nodiscard]] static constexpr bool
  __slot_fits(usize slot) noexcept
  {
    using UI = micron::make_unsigned_t<Index>;
    const UI imax = static_cast<UI>(micron::numeric_limits<Index>::max());
    if constexpr ( sizeof(usize) > sizeof(UI) ) return slot < static_cast<usize>(imax);
    return static_cast<UI>(slot) < imax;
  }

  [[nodiscard]] bool
  __live_vertex(usize slot) const noexcept
  {
    return slot < __vertices.size() && __vertices.data()[slot].live;
  }

  [[nodiscard]] bool
  __live_edge(usize slot) const noexcept
  {
    return slot < __edges.size() && __edges.data()[slot].live;
  }

  [[nodiscard]] edge_descriptor
  __find_edge(usize u, usize v) const noexcept
  {
    if ( !__live_vertex(u) || !__live_vertex(v) ) return edge_descriptor::invalid();
    if constexpr ( __edge_list ) {
      for ( usize i = 0; i < __edges.size(); ++i ) {
        const edge_slot &slot = __edges.data()[i];
        if ( !slot.live ) continue;
        if ( static_cast<usize>(slot.source) == u && static_cast<usize>(slot.target) == v ) return edge_descriptor(static_cast<Index>(i));
        if constexpr ( is_undirected )
          if ( static_cast<usize>(slot.source) == v && static_cast<usize>(slot.target) == u ) return edge_descriptor(static_cast<Index>(i));
      }
      return edge_descriptor::invalid();
    }
    const auto &arcs = __vertices.data()[u].out;
    for ( usize i = 0; i < arcs.size(); ++i ) {
      const arc_type &arc = arcs.data()[i];
      if ( static_cast<usize>(arc.neighbor) == v && __live_edge(static_cast<usize>(arc.edge)) ) return edge_descriptor(arc.edge);
    }
    return edge_descriptor::invalid();
  }

  template<typename V>
  static void
  __reserve_append(V &values, usize count)
  {
    const usize needed = values.size() + count;
    if ( needed <= values.max_size() ) return;
    usize capacity = values.max_size();
    usize grown = capacity < 4 ? 4 : capacity;
    if ( grown < needed ) {
      if ( grown <= micron::numeric_limits<usize>::max() / 2 ) grown *= 2;
      if ( grown < needed ) grown = needed;
    }
    values.reserve(grown);
  }

  template<typename Arcs>
  static void
  __erase_arc(Arcs &arcs, Index edge) noexcept
  {
    for ( usize i = 0; i < arcs.size(); ) {
      if ( arcs.data()[i].edge != edge ) {
        ++i;
        continue;
      }
      if ( i + 1 != arcs.size() ) arcs.data()[i] = micron::move(arcs.data()[arcs.size() - 1]);
      arcs.pop_back();
    }
  }

  template<typename Arcs>
  static void
  __replace_arc_edge(Arcs &arcs, Index from, Index to) noexcept
  {
    for ( usize i = 0; i < arcs.size(); ++i )
      if ( arcs.data()[i].edge == from ) arcs.data()[i].edge = to;
  }

  void
  __detach_edge(Index id, const edge_slot &slot) noexcept
  {
    if constexpr ( __edge_list ) return;
    const usize u = static_cast<usize>(slot.source);
    const usize v = static_cast<usize>(slot.target);
    if constexpr ( is_directed ) {
      __erase_arc(__vertices.data()[u].out, id);
      __erase_arc(__vertices.data()[v].in, id);
    } else {
      __erase_arc(__vertices.data()[u].out, id);
      if ( u != v ) __erase_arc(__vertices.data()[v].out, id);
    }
  }

  void
  __retag_edge_arcs(Index from, Index to, const edge_slot &slot) noexcept
  {
    if constexpr ( __edge_list ) return;
    const usize u = static_cast<usize>(slot.source);
    const usize v = static_cast<usize>(slot.target);
    if constexpr ( is_directed ) {
      __replace_arc_edge(__vertices.data()[u].out, from, to);
      __replace_arc_edge(__vertices.data()[v].in, from, to);
    } else {
      __replace_arc_edge(__vertices.data()[u].out, from, to);
      if ( u != v ) __replace_arc_edge(__vertices.data()[v].out, from, to);
    }
  }

  template<typename P>
  edge_insert_result<Index>
  __insert_edge(Index ui, Index vi, P &&property, bool auto_create)
  {
    const usize u = static_cast<usize>(ui);
    const usize v = static_cast<usize>(vi);
    const usize high = u > v ? u : v;

    if ( !__slot_fits(high) || !__slot_fits(__edges.size()) )
      return { graphs::edge_insert_status::index_overflow, edge_descriptor::invalid() };
    if constexpr ( !allows_loops ) {
      if ( u == v ) return { graphs::edge_insert_status::self_loop, edge_descriptor::invalid() };
    }

    if ( u < __vertices.size() && !__vertices.data()[u].live )
      return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if ( v < __vertices.size() && !__vertices.data()[v].live )
      return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if ( high >= __vertices.size() && !auto_create ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if ( high >= __vertices.size() ) {
      if constexpr ( !micron::is_default_constructible_v<VertexProperty> )
        return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
    }

    if constexpr ( is_simple ) {
      if ( u < __vertices.size() && v < __vertices.size() ) {
        const edge_descriptor duplicate = __find_edge(u, v);
        if ( duplicate.valid() ) return { graphs::edge_insert_status::duplicate, duplicate };
      }
    }

    if ( high >= __vertices.size() ) {
      if constexpr ( micron::is_default_constructible_v<VertexProperty> ) {
        __vertices.reserve(high + 1);
        while ( __vertices.size() <= high ) {
          __vertices.emplace_back();
          ++__live_vertices;
        }
      } else {
        return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
      }
    }

    __reserve_append(__edges, 1);
    if constexpr ( !__edge_list ) {
      if constexpr ( is_directed ) {
        __reserve_append(__vertices.data()[u].out, 1);
        __reserve_append(__vertices.data()[v].in, 1);
      } else if ( u == v ) {
        __reserve_append(__vertices.data()[u].out, 2);
      } else {
        __reserve_append(__vertices.data()[u].out, 1);
        __reserve_append(__vertices.data()[v].out, 1);
      }
    }

    const Index raw_id = static_cast<Index>(__edges.size());
    __edges.emplace_back(ui, vi, micron::forward<P>(property));
    if constexpr ( !__edge_list ) {
      if constexpr ( is_directed ) {
        __vertices.data()[u].out.push_back(arc_type{ raw_id, vi });
        __vertices.data()[v].in.push_back(arc_type{ raw_id, ui });
      } else {
        __vertices.data()[u].out.push_back(arc_type{ raw_id, vi });
        __vertices.data()[v].out.push_back(arc_type{ raw_id, ui });
      }
    }
    ++__live_edges;
    return { graphs::edge_insert_status::inserted, edge_descriptor(raw_id) };
  }

  template<bool Const> class __vertex_iterator
  {
    using graph_pointer = micron::conditional_t<Const, const __basic_adjacency_graph *, __basic_adjacency_graph *>;
    graph_pointer __graph{};
    usize __slot{};

    constexpr void
    __skip() noexcept
    {
      while ( __slot < __graph->__vertices.size() && !__graph->__vertices.data()[__slot].live ) ++__slot;
    }

  public:
    constexpr __vertex_iterator() noexcept = default;

    constexpr __vertex_iterator(graph_pointer g, usize slot) noexcept : __graph(g), __slot(slot) { __skip(); }

    [[nodiscard]] constexpr vertex_descriptor
    operator*() const noexcept
    {
      return vertex_descriptor(static_cast<Index>(__slot));
    }

    constexpr __vertex_iterator &
    operator++() noexcept
    {
      ++__slot;
      __skip();
      return *this;
    }

    friend constexpr bool
    operator==(const __vertex_iterator &a, const __vertex_iterator &b) noexcept
    {
      return a.__graph == b.__graph && a.__slot == b.__slot;
    }

    friend constexpr bool
    operator!=(const __vertex_iterator &a, const __vertex_iterator &b) noexcept
    {
      return !(a == b);
    }
  };

public:
  template<bool Const> struct basic_edge_reference {
    using property_reference = micron::conditional_t<Const, const EdgeProperty &, EdgeProperty &>;
    using micron_printable_tag = void;

    edge_descriptor id;
    vertex_descriptor source;
    vertex_descriptor target;
    property_reference property;

    template<typename Out>
    constexpr void
    __micron_print(Out &out) const
    {
      out.raw("edge{ ", 6);
      out.elem(id);
      out.raw(": ", 2);
      out.elem(source);
      out.raw(is_directed ? " -> " : " -- ", 4);
      out.elem(target);
      out.raw(", property: ", 12);
      out.elem(property);
      out.raw(" }", 2);
    }
  };

  using edge_reference = basic_edge_reference<false>;
  using const_edge_reference = basic_edge_reference<true>;

private:
  template<bool Const> class __edge_iterator
  {
    using graph_pointer = micron::conditional_t<Const, const __basic_adjacency_graph *, __basic_adjacency_graph *>;
    graph_pointer __graph{};
    usize __slot{};

    constexpr void
    __skip() noexcept
    {
      while ( __slot < __graph->__edges.size() && !__graph->__edges.data()[__slot].live ) ++__slot;
    }

  public:
    constexpr __edge_iterator() noexcept = default;

    constexpr __edge_iterator(graph_pointer g, usize slot) noexcept : __graph(g), __slot(slot) { __skip(); }

    [[nodiscard]] constexpr auto
    operator*() const noexcept
    {
      auto &slot = __graph->__edges.data()[__slot];
      return basic_edge_reference<Const>{ edge_descriptor(static_cast<Index>(__slot)), vertex_descriptor(slot.source),
                                          vertex_descriptor(slot.target), slot.property };
    }

    constexpr __edge_iterator &
    operator++() noexcept
    {
      ++__slot;
      __skip();
      return *this;
    }

    friend constexpr bool
    operator==(const __edge_iterator &a, const __edge_iterator &b) noexcept
    {
      return a.__graph == b.__graph && a.__slot == b.__slot;
    }

    friend constexpr bool
    operator!=(const __edge_iterator &a, const __edge_iterator &b) noexcept
    {
      return !(a == b);
    }
  };

  enum class __arc_direction : u8 { out = 0, in };

  class __neighbor_iterator
  {
    const __basic_adjacency_graph *__graph{};
    usize __vertex{};
    usize __arc{};
    __arc_direction __direction{};
    bool __repeat_loop{};

    [[nodiscard]] const auto &
    __arcs() const noexcept
    {
      if constexpr ( is_directed ) {
        if ( __direction == __arc_direction::in ) return __graph->__vertices.data()[__vertex].in;
      }
      return __graph->__vertices.data()[__vertex].out;
    }

    void
    __skip() noexcept
    {
      if constexpr ( __edge_list ) {
        __repeat_loop = false;
        while ( __arc < __graph->__edges.size() ) {
          const edge_slot &slot = __graph->__edges.data()[__arc];
          if ( slot.live ) {
            const bool match
                = __direction == __arc_direction::in && is_directed
                      ? static_cast<usize>(slot.target) == __vertex
                      : static_cast<usize>(slot.source) == __vertex || (is_undirected && static_cast<usize>(slot.target) == __vertex);
            if ( match ) {
              if constexpr ( is_undirected && allows_loops ) __repeat_loop = slot.source == slot.target;
              break;
            }
          }
          ++__arc;
        }
        return;
      }
      const auto &arcs = __arcs();
      while ( __arc < arcs.size() ) {
        const arc_type &a = arcs.data()[__arc];
        if ( __graph->__live_edge(static_cast<usize>(a.edge)) && __graph->__live_vertex(static_cast<usize>(a.neighbor)) ) break;
        ++__arc;
      }
    }

  public:
    __neighbor_iterator() noexcept = default;

    __neighbor_iterator(const __basic_adjacency_graph *g, usize vertex, usize arc, __arc_direction direction) noexcept
        : __graph(g), __vertex(vertex), __arc(arc), __direction(direction)
    {
      __skip();
    }

    [[nodiscard]] vertex_descriptor
    operator*() const noexcept
    {
      if constexpr ( __edge_list ) {
        const edge_slot &slot = __graph->__edges.data()[__arc];
        if constexpr ( is_directed )
          return vertex_descriptor(__direction == __arc_direction::in ? slot.source : slot.target);
        else
          return vertex_descriptor(slot.source == static_cast<Index>(__vertex) ? slot.target : slot.source);
      }
      return vertex_descriptor(__arcs().data()[__arc].neighbor);
    }

    __neighbor_iterator &
    operator++() noexcept
    {
      if constexpr ( __edge_list && is_undirected && allows_loops )
        if ( __repeat_loop ) {
          __repeat_loop = false;
          return *this;
        }
      ++__arc;
      __skip();
      return *this;
    }

    friend bool
    operator==(const __neighbor_iterator &a, const __neighbor_iterator &b) noexcept
    {
      return a.__graph == b.__graph && a.__vertex == b.__vertex && a.__arc == b.__arc && a.__direction == b.__direction
             && a.__repeat_loop == b.__repeat_loop;
    }

    friend bool
    operator!=(const __neighbor_iterator &a, const __neighbor_iterator &b) noexcept
    {
      return !(a == b);
    }
  };

  class __incident_iterator
  {
    const __basic_adjacency_graph *__graph{};
    usize __vertex{};
    usize __arc{};
    __arc_direction __direction{};
    bool __repeat_loop{};

    [[nodiscard]] const auto &
    __arcs() const noexcept
    {
      if constexpr ( is_directed ) {
        if ( __direction == __arc_direction::in ) return __graph->__vertices.data()[__vertex].in;
      }
      return __graph->__vertices.data()[__vertex].out;
    }

    void
    __skip() noexcept
    {
      if constexpr ( __edge_list ) {
        __repeat_loop = false;
        while ( __arc < __graph->__edges.size() ) {
          const edge_slot &slot = __graph->__edges.data()[__arc];
          if ( slot.live ) {
            const bool match
                = __direction == __arc_direction::in && is_directed
                      ? static_cast<usize>(slot.target) == __vertex
                      : static_cast<usize>(slot.source) == __vertex || (is_undirected && static_cast<usize>(slot.target) == __vertex);
            if ( match ) {
              if constexpr ( is_undirected && allows_loops ) __repeat_loop = slot.source == slot.target;
              break;
            }
          }
          ++__arc;
        }
        return;
      }
      const auto &arcs = __arcs();
      while ( __arc < arcs.size() && !__graph->__live_edge(static_cast<usize>(arcs.data()[__arc].edge)) ) ++__arc;
    }

  public:
    __incident_iterator() noexcept = default;

    __incident_iterator(const __basic_adjacency_graph *g, usize vertex, usize arc, __arc_direction direction) noexcept
        : __graph(g), __vertex(vertex), __arc(arc), __direction(direction)
    {
      __skip();
    }

    [[nodiscard]] edge_descriptor
    operator*() const noexcept
    {
      if constexpr ( __edge_list ) return edge_descriptor(static_cast<Index>(__arc));
      return edge_descriptor(__arcs().data()[__arc].edge);
    }

    __incident_iterator &
    operator++() noexcept
    {
      if constexpr ( __edge_list && is_undirected && allows_loops )
        if ( __repeat_loop ) {
          __repeat_loop = false;
          return *this;
        }
      ++__arc;
      __skip();
      return *this;
    }

    friend bool
    operator==(const __incident_iterator &a, const __incident_iterator &b) noexcept
    {
      return a.__graph == b.__graph && a.__vertex == b.__vertex && a.__arc == b.__arc && a.__direction == b.__direction
             && a.__repeat_loop == b.__repeat_loop;
    }

    friend bool
    operator!=(const __incident_iterator &a, const __incident_iterator &b) noexcept
    {
      return !(a == b);
    }
  };

public:
  template<bool Const> struct vertex_range {
    using graph_pointer = micron::conditional_t<Const, const __basic_adjacency_graph *, __basic_adjacency_graph *>;
    graph_pointer owner{};

    [[nodiscard]] auto
    begin() const noexcept
    {
      return __vertex_iterator<Const>(owner, 0);
    }

    [[nodiscard]] auto
    end() const noexcept
    {
      return __vertex_iterator<Const>(owner, owner->__vertices.size());
    }
  };

  template<bool Const> struct edge_range {
    using graph_pointer = micron::conditional_t<Const, const __basic_adjacency_graph *, __basic_adjacency_graph *>;
    graph_pointer owner{};

    [[nodiscard]] auto
    begin() const noexcept
    {
      return __edge_iterator<Const>(owner, 0);
    }

    [[nodiscard]] auto
    end() const noexcept
    {
      return __edge_iterator<Const>(owner, owner->__edges.size());
    }
  };

  struct neighbor_range {
    const __basic_adjacency_graph *owner{};
    usize vertex{};
    __arc_direction direction{};
    bool valid{};

    [[nodiscard]] auto
    begin() const noexcept
    {
      return valid ? __neighbor_iterator(owner, vertex, 0, direction) : __neighbor_iterator();
    }

    [[nodiscard]] auto
    end() const noexcept
    {
      if ( !valid ) return __neighbor_iterator();
      if constexpr ( __edge_list ) return __neighbor_iterator(owner, vertex, owner->__edges.size(), direction);
      const auto &arcs
          = direction == __arc_direction::in && is_directed ? owner->__vertices.data()[vertex].in : owner->__vertices.data()[vertex].out;
      return __neighbor_iterator(owner, vertex, arcs.size(), direction);
    }
  };

  struct incident_range {
    const __basic_adjacency_graph *owner{};
    usize vertex{};
    __arc_direction direction{};
    bool valid{};

    [[nodiscard]] auto
    begin() const noexcept
    {
      return valid ? __incident_iterator(owner, vertex, 0, direction) : __incident_iterator();
    }

    [[nodiscard]] auto
    end() const noexcept
    {
      if ( !valid ) return __incident_iterator();
      if constexpr ( __edge_list ) return __incident_iterator(owner, vertex, owner->__edges.size(), direction);
      const auto &arcs
          = direction == __arc_direction::in && is_directed ? owner->__vertices.data()[vertex].in : owner->__vertices.data()[vertex].out;
      return __incident_iterator(owner, vertex, arcs.size(), direction);
    }
  };

  __basic_adjacency_graph()
    requires micron::is_default_constructible_v<GraphProperty>
      : __property(), __vertices(), __edges()
  {
  }

  explicit __basic_adjacency_graph(const GraphProperty &property) : __property(property), __vertices(), __edges() { }

  explicit __basic_adjacency_graph(GraphProperty &&property) : __property(micron::move(property)), __vertices(), __edges() { }

  __basic_adjacency_graph(const __basic_adjacency_graph &) = default;
  __basic_adjacency_graph(__basic_adjacency_graph &&) = default;
  __basic_adjacency_graph &operator=(const __basic_adjacency_graph &) = default;
  __basic_adjacency_graph &operator=(__basic_adjacency_graph &&) = default;
  ~__basic_adjacency_graph() = default;

  [[nodiscard]] usize
  vertices_count() const noexcept
  {
    return __live_vertices;
  }

  [[nodiscard]] usize
  edges_count() const noexcept
  {
    return __live_edges;
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
    return __live_vertices == 0;
  }

  void
  reserve_vertices(usize count)
  {
    __vertices.reserve(count);
  }

  void
  reserve_edges(usize count)
  {
    __edges.reserve(count);
  }

  void
  reserve_adjacency(vertex_descriptor vertex, usize outgoing, usize incoming = 0)
  {
    if constexpr ( __edge_list ) return;
    if ( !has_vertex(vertex) ) return;
    auto &slot = __vertices.data()[static_cast<usize>(vertex.value)];
    slot.out.reserve(outgoing);
    if constexpr ( is_directed ) slot.in.reserve(incoming);
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

  [[nodiscard]] vertex_descriptor
  add_vertex()
    requires micron::is_default_constructible_v<VertexProperty>
  {
    if ( !__slot_fits(__vertices.size()) ) return vertex_descriptor::invalid();
    const Index id = static_cast<Index>(__vertices.size());
    __vertices.emplace_back();
    ++__live_vertices;
    return vertex_descriptor(id);
  }

  [[nodiscard]] vertex_descriptor
  add_vertex(const VertexProperty &property)
  {
    if ( !__slot_fits(__vertices.size()) ) return vertex_descriptor::invalid();
    const Index id = static_cast<Index>(__vertices.size());
    __vertices.emplace_back(property);
    ++__live_vertices;
    return vertex_descriptor(id);
  }

  [[nodiscard]] vertex_descriptor
  add_vertex(VertexProperty &&property)
  {
    if ( !__slot_fits(__vertices.size()) ) return vertex_descriptor::invalid();
    const Index id = static_cast<Index>(__vertices.size());
    __vertices.emplace_back(micron::move(property));
    ++__live_vertices;
    return vertex_descriptor(id);
  }

  template<typename... Args>
  [[nodiscard]] vertex_descriptor
  emplace_vertex(Args &&...args)
  {
    if ( !__slot_fits(__vertices.size()) ) return vertex_descriptor::invalid();
    const Index id = static_cast<Index>(__vertices.size());
    VertexProperty property(micron::forward<Args>(args)...);
    __vertices.emplace_back(micron::move(property));
    ++__live_vertices;
    return vertex_descriptor(id);
  }

  template<typename Label>
    requires graphs::labeled_bundle<VertexProperty> && micron::is_constructible_v<typename VertexProperty::label_type, Label &&>
  [[nodiscard]] vertex_descriptor
  add_labeled_vertex(Label &&label)
  {
    return add_vertex(VertexProperty(micron::forward<Label>(label)));
  }

  template<typename Label, typename P>
    requires graphs::labeled_bundle<VertexProperty> && micron::is_constructible_v<typename VertexProperty::label_type, Label &&>
             && micron::is_constructible_v<typename VertexProperty::property_type, P &&>
  [[nodiscard]] vertex_descriptor
  add_labeled_vertex(Label &&label, P &&property)
  {
    return add_vertex(VertexProperty(micron::forward<Label>(label), micron::forward<P>(property)));
  }

  [[nodiscard]] micron::vector<vertex_descriptor, Alloc, false>
  add_vertices(usize count)
    requires micron::is_default_constructible_v<VertexProperty>
  {
    micron::vector<vertex_descriptor, Alloc, false> result;
    if ( count == 0 ) return result;
    if ( count > micron::numeric_limits<usize>::max() - __vertices.size() ) return result;
    if ( !__slot_fits(__vertices.size()) || !__slot_fits(__vertices.size() + count - 1) ) return result;
    result.reserve(count);
    __vertices.reserve(__vertices.size() + count);
    for ( usize i = 0; i < count; ++i ) result.push_back(add_vertex());
    return result;
  }

  [[nodiscard]] bool
  has_vertex(vertex_descriptor id) const noexcept
  {
    return id.valid() && __live_vertex(static_cast<usize>(id.value));
  }

  template<micron::integral U>
  [[nodiscard]] bool
  has_vertex(U id) const noexcept
  {
    Index value{};
    return __endpoint(id, value) && __live_vertex(static_cast<usize>(value));
  }

  [[nodiscard]] bool
  has_edge(edge_descriptor id) const noexcept
  {
    return id.valid() && __live_edge(static_cast<usize>(id.value));
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] bool
  has_edge(U u, V v) const noexcept
  {
    Index ui{}, vi{};
    return __endpoint(u, ui) && __endpoint(v, vi) && __find_edge(static_cast<usize>(ui), static_cast<usize>(vi)).valid();
  }

  [[nodiscard]] bool
  has_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    return u.valid() && v.valid() && __find_edge(static_cast<usize>(u.value), static_cast<usize>(v.value)).valid();
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_descriptor
  find_edge(U u, V v) const noexcept
  {
    Index ui{}, vi{};
    if ( !__endpoint(u, ui) || !__endpoint(v, vi) ) return edge_descriptor::invalid();
    return __find_edge(static_cast<usize>(ui), static_cast<usize>(vi));
  }

  [[nodiscard]] edge_descriptor
  find_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    if ( !u.valid() || !v.valid() ) return edge_descriptor::invalid();
    return __find_edge(static_cast<usize>(u.value), static_cast<usize>(v.value));
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U u, V v)
  {
    Index ui{}, vi{};
    if ( !__endpoint(u, ui) || !__endpoint(v, vi) ) return { __endpoint_failure(u, v), edge_descriptor::invalid() };
    if constexpr ( micron::is_default_constructible_v<EdgeProperty> )
      return __insert_edge(ui, vi, EdgeProperty{}, true);
    else
      return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
  }

  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor u, vertex_descriptor v)
  {
    if ( !u.valid() || !v.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    if constexpr ( micron::is_default_constructible_v<EdgeProperty> )
      return __insert_edge(u.value, v.value, EdgeProperty{}, false);
    else
      return { graphs::edge_insert_status::property_required, edge_descriptor::invalid() };
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U u, V v, const EdgeProperty &property)
  {
    Index ui{}, vi{};
    if ( !__endpoint(u, ui) || !__endpoint(v, vi) ) return { __endpoint_failure(u, v), edge_descriptor::invalid() };
    return __insert_edge(ui, vi, property, true);
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U u, V v, EdgeProperty &&property)
  {
    Index ui{}, vi{};
    if ( !__endpoint(u, ui) || !__endpoint(v, vi) ) return { __endpoint_failure(u, v), edge_descriptor::invalid() };
    return __insert_edge(ui, vi, micron::move(property), true);
  }

  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor u, vertex_descriptor v, const EdgeProperty &property)
  {
    if ( !u.valid() || !v.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    return __insert_edge(u.value, v.value, property, false);
  }

  [[nodiscard]] edge_insert_result<Index>
  add_edge(vertex_descriptor u, vertex_descriptor v, EdgeProperty &&property)
  {
    if ( !u.valid() || !v.valid() ) return { graphs::edge_insert_status::invalid_vertex, edge_descriptor::invalid() };
    return __insert_edge(u.value, v.value, micron::move(property), false);
  }

  template<micron::integral U, micron::integral V, typename W>
    requires graphs::weighted_bundle<EdgeProperty> && (!micron::is_same_v<micron::remove_cvref_t<W>, EdgeProperty>)
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U u, V v, W &&weight)
  {
    return add_edge(u, v, EdgeProperty(micron::forward<W>(weight)));
  }

  template<micron::integral U, micron::integral V, typename W, typename P>
    requires graphs::weighted_bundle<EdgeProperty>
  [[nodiscard]] edge_insert_result<Index>
  add_edge(U u, V v, W &&weight, P &&property)
  {
    return add_edge(u, v, EdgeProperty(micron::forward<W>(weight), micron::forward<P>(property)));
  }

  template<typename Range>
  [[nodiscard]] graphs::bulk_insert_result<Index>
  add_edges(const Range &range)
  {
    graphs::bulk_insert_result<Index> result{};
    for ( const auto &item : range ) {
      edge_insert_result<Index> inserted;
      if constexpr ( requires { item.property; } && !micron::is_same_v<EdgeProperty, empty_property> )
        inserted = add_edge(item.source, item.target, item.property);
      else
        inserted = add_edge(item.source, item.target);
      result.account(inserted.status);
    }
    return result;
  }

  template<micron::integral I, typename P>
  __basic_adjacency_graph &
  operator+=(const edge<I, P> &item)
  {
    if constexpr ( micron::is_same_v<P, empty_property> )
      (void)add_edge(item.source, item.target);
    else if constexpr ( micron::is_constructible_v<EdgeProperty, const P &> )
      (void)add_edge(item.source, item.target, EdgeProperty(item.property));
    return *this;
  }

  [[nodiscard]] bool
  remove_edge(edge_descriptor id) noexcept
  {
    if ( !has_edge(id) ) return false;
    const usize erased = static_cast<usize>(id.value);
    if constexpr ( __stable ) {
      __edges.data()[erased].make_dead();
      --__live_edges;
      return true;
    }

    const usize last = __edges.size() - 1;
    __detach_edge(id.value, __edges.data()[erased]);
    if ( erased != last ) {
      const Index old_id = static_cast<Index>(last);
      __edges.data()[erased] = micron::move(__edges.data()[last]);
      __retag_edge_arcs(old_id, id.value, __edges.data()[erased]);
    }
    __edges.pop_back();
    --__live_edges;
    return true;
  }

  template<micron::integral U, micron::integral V>
  [[nodiscard]] bool
  remove_edge(U u, V v) noexcept
  {
    const edge_descriptor id = find_edge(u, v);
    return remove_edge(id);
  }

  [[nodiscard]] usize
  remove_edges(vertex_descriptor u, vertex_descriptor v) noexcept
  {
    if ( !has_vertex(u) || !has_vertex(v) ) return 0;
    usize removed = 0;
    for ( usize i = 0; i < __edges.size(); ) {
      const edge_slot &slot = __edges.data()[i];
      const bool matches = is_directed
                               ? slot.source == u.value && slot.target == v.value
                               : (slot.source == u.value && slot.target == v.value) || (slot.source == v.value && slot.target == u.value);
      if ( matches && remove_edge(edge_descriptor(static_cast<Index>(i))) ) {
        ++removed;
        if constexpr ( __stable ) ++i;
      } else {
        ++i;
      }
    }
    return removed;
  }

  [[nodiscard]] bool
  remove_vertex(vertex_descriptor id) noexcept
  {
    if ( !has_vertex(id) ) return false;
    const Index raw = id.value;
    if constexpr ( __stable ) {
      for ( usize i = 0; i < __edges.size(); ++i ) {
        edge_slot &slot = __edges.data()[i];
        if ( slot.live && (slot.source == raw || slot.target == raw) ) {
          slot.make_dead();
          --__live_edges;
        }
      }
      __vertices.data()[static_cast<usize>(raw)].make_dead();
      --__live_vertices;
      return true;
    }

    for ( usize i = 0; i < __edges.size(); ) {
      const edge_slot &slot = __edges.data()[i];
      if ( slot.source == raw || slot.target == raw )
        (void)remove_edge(edge_descriptor(static_cast<Index>(i)));
      else
        ++i;
    }

    const usize erased = static_cast<usize>(raw);
    const usize last = __vertices.size() - 1;
    if ( erased != last ) {
      const Index old_vertex = static_cast<Index>(last);
      __vertices.data()[erased] = micron::move(__vertices.data()[last]);
      for ( usize i = 0; i < __edges.size(); ++i ) {
        edge_slot &slot = __edges.data()[i];
        if ( slot.source == old_vertex ) slot.source = raw;
        if ( slot.target == old_vertex ) slot.target = raw;
      }
      if constexpr ( !__edge_list ) {
        for ( usize i = 0; i < __vertices.size(); ++i ) {
          auto &out = __vertices.data()[i].out;
          for ( usize j = 0; j < out.size(); ++j )
            if ( out.data()[j].neighbor == old_vertex ) out.data()[j].neighbor = raw;
          if constexpr ( is_directed ) {
            auto &in = __vertices.data()[i].in;
            for ( usize j = 0; j < in.size(); ++j )
              if ( in.data()[j].neighbor == old_vertex ) in.data()[j].neighbor = raw;
          }
        }
      }
    }
    __vertices.pop_back();
    --__live_vertices;
    return true;
  }

  // Binary/topology builders use these two hooks to reproduce high-water
  // descriptor space without inventing live objects or adjacency arcs.
  [[nodiscard]] bool
  __import_dead_vertex_slot()
    requires(__stable)
  {
    if ( !__slot_fits(__vertices.size()) ) return false;
    __vertices.emplace_back(graphs::__dead_graph_slot_t{});
    return true;
  }

  [[nodiscard]] bool
  __import_dead_edge_slot()
    requires(__stable)
  {
    if ( !__slot_fits(__edges.size()) ) return false;
    __edges.emplace_back(graphs::__dead_graph_slot_t{});
    return true;
  }

  template<micron::integral U>
  [[nodiscard]] bool
  remove_vertex(U value) noexcept
  {
    Index id{};
    return __endpoint(value, id) && remove_vertex(vertex_descriptor(id));
  }

  [[nodiscard]] VertexProperty *
  try_vertex_property(vertex_descriptor id) noexcept
  {
    return has_vertex(id) ? micron::addressof(__vertices.data()[static_cast<usize>(id.value)].property) : nullptr;
  }

  [[nodiscard]] const VertexProperty *
  try_vertex_property(vertex_descriptor id) const noexcept
  {
    return has_vertex(id) ? micron::addressof(__vertices.data()[static_cast<usize>(id.value)].property) : nullptr;
  }

  [[nodiscard]] EdgeProperty *
  try_edge_property(edge_descriptor id) noexcept
  {
    return has_edge(id) ? micron::addressof(__edges.data()[static_cast<usize>(id.value)].property) : nullptr;
  }

  [[nodiscard]] const EdgeProperty *
  try_edge_property(edge_descriptor id) const noexcept
  {
    return has_edge(id) ? micron::addressof(__edges.data()[static_cast<usize>(id.value)].property) : nullptr;
  }

  [[nodiscard]] VertexProperty &
  vertex_property_unchecked(vertex_descriptor id) noexcept
  {
    return __vertices.data()[static_cast<usize>(id.value)].property;
  }

  [[nodiscard]] const VertexProperty &
  vertex_property_unchecked(vertex_descriptor id) const noexcept
  {
    return __vertices.data()[static_cast<usize>(id.value)].property;
  }

  [[nodiscard]] EdgeProperty &
  edge_property_unchecked(edge_descriptor id) noexcept
  {
    return __edges.data()[static_cast<usize>(id.value)].property;
  }

  [[nodiscard]] const EdgeProperty &
  edge_property_unchecked(edge_descriptor id) const noexcept
  {
    return __edges.data()[static_cast<usize>(id.value)].property;
  }

  [[nodiscard]] vertex_descriptor
  source(edge_descriptor id) const noexcept
  {
    return vertex_descriptor(__edges.data()[static_cast<usize>(id.value)].source);
  }

  [[nodiscard]] vertex_descriptor
  target(edge_descriptor id) const noexcept
  {
    return vertex_descriptor(__edges.data()[static_cast<usize>(id.value)].target);
  }

  [[nodiscard]] vertex_descriptor
  opposite(edge_descriptor id, vertex_descriptor vertex) const noexcept
  {
    const edge_slot &slot = __edges.data()[static_cast<usize>(id.value)];
    return vertex_descriptor(slot.source == vertex.value ? slot.target : slot.source);
  }

  [[nodiscard]] VertexProperty &
  vertex_property(vertex_descriptor id)
  {
    VertexProperty *result = try_vertex_property(id);
    if ( result == nullptr ) __builtin_trap();
    return *result;
  }

  [[nodiscard]] const VertexProperty &
  vertex_property(vertex_descriptor id) const
  {
    const VertexProperty *result = try_vertex_property(id);
    if ( result == nullptr ) __builtin_trap();
    return *result;
  }

  [[nodiscard]] EdgeProperty &
  edge_property(edge_descriptor id)
  {
    EdgeProperty *result = try_edge_property(id);
    if ( result == nullptr ) __builtin_trap();
    return *result;
  }

  [[nodiscard]] const EdgeProperty &
  edge_property(edge_descriptor id) const
  {
    const EdgeProperty *result = try_edge_property(id);
    if ( result == nullptr ) __builtin_trap();
    return *result;
  }

  [[nodiscard]] VertexProperty &
  operator[](vertex_descriptor id)
  {
    return vertex_property(id);
  }

  [[nodiscard]] const VertexProperty &
  operator[](vertex_descriptor id) const
  {
    return vertex_property(id);
  }

  template<typename T = EdgeProperty>
    requires graphs::weighted_bundle<T>
  [[nodiscard]] decltype(auto)
  edge_weight(edge_descriptor id) noexcept
  {
    return (edge_property_unchecked(id).weight);
  }

  template<typename T = EdgeProperty>
    requires graphs::weighted_bundle<T>
  [[nodiscard]] decltype(auto)
  edge_weight(edge_descriptor id) const noexcept
  {
    return (edge_property_unchecked(id).weight);
  }

  template<typename T = VertexProperty>
    requires graphs::labeled_bundle<T>
  [[nodiscard]] decltype(auto)
  label(vertex_descriptor id) noexcept
  {
    return (vertex_property_unchecked(id).label);
  }

  template<typename T = VertexProperty>
    requires graphs::labeled_bundle<T>
  [[nodiscard]] decltype(auto)
  label(vertex_descriptor id) const noexcept
  {
    return (vertex_property_unchecked(id).label);
  }

  template<typename Label, typename T = VertexProperty>
    requires graphs::labeled_bundle<T>
  [[nodiscard]] vertex_descriptor
  find_vertex(const Label &label_value) const noexcept
  {
    for ( auto id : vertices() )
      if ( vertex_property_unchecked(id).label == label_value ) return id;
    return vertex_descriptor::invalid();
  }

  [[nodiscard]] vertex_range<false>
  vertices() noexcept
  {
    return { this };
  }

  [[nodiscard]] vertex_range<true>
  vertices() const noexcept
  {
    return { this };
  }

  [[nodiscard]] edge_range<false>
  edges() noexcept
  {
    return { this };
  }

  [[nodiscard]] edge_range<true>
  edges() const noexcept
  {
    return { this };
  }

  [[nodiscard]] neighbor_range
  out_neighbors(vertex_descriptor id) const noexcept
  {
    return { this, static_cast<usize>(id.value), __arc_direction::out, has_vertex(id) };
  }

  [[nodiscard]] neighbor_range
  in_neighbors(vertex_descriptor id) const noexcept
  {
    return { this, static_cast<usize>(id.value), is_directed ? __arc_direction::in : __arc_direction::out, has_vertex(id) };
  }

  [[nodiscard]] neighbor_range
  neighbors(vertex_descriptor id) const noexcept
  {
    return out_neighbors(id);
  }

  template<micron::integral U>
  [[nodiscard]] neighbor_range
  out_neighbors(U id) const noexcept
  {
    return out_neighbors(vertex_descriptor(static_cast<Index>(id)));
  }

  template<micron::integral U>
  [[nodiscard]] neighbor_range
  in_neighbors(U id) const noexcept
  {
    return in_neighbors(vertex_descriptor(static_cast<Index>(id)));
  }

  template<micron::integral U>
  [[nodiscard]] neighbor_range
  neighbors(U id) const noexcept
  {
    return out_neighbors(id);
  }

  [[nodiscard]] incident_range
  out_edges(vertex_descriptor id) const noexcept
  {
    return { this, static_cast<usize>(id.value), __arc_direction::out, has_vertex(id) };
  }

  [[nodiscard]] incident_range
  in_edges(vertex_descriptor id) const noexcept
  {
    return { this, static_cast<usize>(id.value), is_directed ? __arc_direction::in : __arc_direction::out, has_vertex(id) };
  }

  [[nodiscard]] usize
  out_degree(vertex_descriptor id) const noexcept
  {
    if ( !has_vertex(id) ) return 0;
    usize result = 0;
    for ( auto ignored : out_edges(id) ) {
      (void)ignored;
      ++result;
    }
    return result;
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor id) const noexcept
  {
    if constexpr ( is_undirected ) return out_degree(id);
    if ( !has_vertex(id) ) return 0;
    usize result = 0;
    for ( auto ignored : in_edges(id) ) {
      (void)ignored;
      ++result;
    }
    return result;
  }

  [[nodiscard]] usize
  degree(vertex_descriptor id) const noexcept
  {
    if constexpr ( is_directed ) return in_degree(id) + out_degree(id);
    return out_degree(id);
  }

  [[nodiscard]] compact_result<Index>
  compact()
  {
    compact_result<Index> remap{
      micron::vector<vertex_descriptor, micron::allocator_serial<>, false>(__vertices.size(), vertex_descriptor::invalid()),
      micron::vector<edge_descriptor, micron::allocator_serial<>, false>(__edges.size(), edge_descriptor::invalid())
    };

    __basic_adjacency_graph replacement(micron::move(__property));
    replacement.reserve_vertices(__live_vertices);
    replacement.reserve_edges(__live_edges);
    for ( usize i = 0; i < __vertices.size(); ++i ) {
      if ( !__vertices.data()[i].live ) continue;
      remap.vertex_remap.data()[i] = replacement.add_vertex(micron::move(__vertices.data()[i].property));
    }
    for ( usize i = 0; i < __edges.size(); ++i ) {
      edge_slot &slot = __edges.data()[i];
      if ( !slot.live ) continue;
      const vertex_descriptor u = remap.vertex_remap.data()[static_cast<usize>(slot.source)];
      const vertex_descriptor v = remap.vertex_remap.data()[static_cast<usize>(slot.target)];
      auto inserted = replacement.add_edge(u, v, micron::move(slot.property));
      remap.edge_remap.data()[i] = inserted.id;
    }
    *this = micron::move(replacement);
    return remap;
  }

  void
  clear()
  {
    __vertices.clear();
    __edges.clear();
    __live_vertices = 0;
    __live_edges = 0;
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
      for ( auto v : vertices() ) {
        if ( !first_vertex ) out.raw(", ", 2);
        first_vertex = false;
        print_vertex(v);
        out.raw(": { ", 4);
        bool first_neighbor = true;
        for ( auto n : out_neighbors(v) ) {
          if ( !first_neighbor ) out.raw(", ", 2);
          first_neighbor = false;
          print_vertex(n);
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
      for ( auto v : vertices() ) {
        if ( !first_vertex ) out.raw(", ", 2);
        first_vertex = false;
        print_vertex(v);
        out.raw(": ", 2);
        if constexpr ( graphs::labeled_bundle<VertexProperty> )
          out.elem(vertex_property_unchecked(v).property);
        else
          out.elem(vertex_property_unchecked(v));
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// standard graph families

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using graph = __basic_adjacency_graph<VertexProperty, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops,
                                      graphs::compact_adjacency_t, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using stable_adjacency_graph = __basic_adjacency_graph<VertexProperty, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops,
                                                       graphs::stable_adjacency_t, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using edge_list_graph = __basic_adjacency_graph<VertexProperty, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops,
                                                graphs::edge_list_t, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Multiplicity = graphs::simple_t, class Loops = graphs::no_loops_t,
         class Alloc = micron::allocator_serial<>>
using digraph = graph<VertexProperty, EdgeProperty, GraphProperty, Index, graphs::directed_t, Multiplicity, Loops, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using multigraph = graph<VertexProperty, EdgeProperty, GraphProperty, Index, graphs::undirected_t, graphs::parallel_t, Loops, Alloc>;

template<class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using multidigraph = graph<VertexProperty, EdgeProperty, GraphProperty, Index, graphs::directed_t, graphs::parallel_t, Loops, Alloc>;

template<typename Weight = f64, class VertexProperty = empty_property, class EdgeProperty = empty_property,
         class GraphProperty = empty_property, micron::integral Index = u32, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using weighted_graph = graph<VertexProperty, weighted_property<Weight, EdgeProperty>, GraphProperty, Index, graphs::undirected_t,
                             Multiplicity, Loops, Alloc>;

template<typename Weight = f64, class VertexProperty = empty_property, class EdgeProperty = empty_property,
         class GraphProperty = empty_property, micron::integral Index = u32, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using weighted_digraph
    = graph<VertexProperty, weighted_property<Weight, EdgeProperty>, GraphProperty, Index, graphs::directed_t, Multiplicity, Loops, Alloc>;

template<typename Label, class VertexProperty = empty_property, class EdgeProperty = empty_property, class GraphProperty = empty_property,
         micron::integral Index = u32, class Direction = graphs::undirected_t, class Multiplicity = graphs::simple_t,
         class Loops = graphs::no_loops_t, class Alloc = micron::allocator_serial<>>
using labeled_graph
    = graph<labeled_property<Label, VertexProperty>, EdgeProperty, GraphProperty, Index, Direction, Multiplicity, Loops, Alloc>;

namespace graphs
{

template<typename G>
concept graph_model = requires(const G graph, typename G::vertex_descriptor vertex, typename G::edge_descriptor edge) {
  typename G::index_type;
  typename G::vertex_property_type;
  typename G::edge_property_type;
  typename G::direction_type;
  { graph.vertices_count() } -> micron::convertible_to<usize>;
  { graph.edges_count() } -> micron::convertible_to<usize>;
  { graph.vertex_slots() } -> micron::convertible_to<usize>;
  { graph.edge_slots() } -> micron::convertible_to<usize>;
  { graph.has_vertex(vertex) } -> micron::convertible_to<bool>;
  { graph.has_edge(edge) } -> micron::convertible_to<bool>;
  graph.vertices();
  graph.edges();
  graph.out_neighbors(vertex);
  graph.out_edges(vertex);
  graph.source(edge);
  graph.target(edge);
};

template<typename G>
concept mutable_graph_model = graph_model<G> && requires(G graph, typename G::vertex_descriptor vertex, typename G::edge_descriptor edge) {
  graph.remove_vertex(vertex);
  graph.remove_edge(edge);
};

template<typename G>
concept contiguous_slot_graph = graph_model<G> && requires { G::has_contiguous_slots; } && G::has_contiguous_slots;

template<typename G>
concept fast_incoming_graph = graph_model<G> && requires { G::has_fast_in_adjacency; } && G::has_fast_in_adjacency;

template<typename G>
concept matrix_adjacency_graph
    = graph_model<G> && requires(const G graph, typename G::vertex_descriptor u, typename G::vertex_descriptor v) {
        { G::has_matrix_adjacency } -> micron::convertible_to<bool>;
        requires G::has_matrix_adjacency;
        { graph.matrix_has_edge(u, v) } -> micron::convertible_to<bool>;
        { graph.matrix_order() } -> micron::convertible_to<usize>;
      };

template<typename G>
concept bitset_neighbor_graph = matrix_adjacency_graph<G> && requires(const G graph, typename G::vertex_descriptor vertex) {
  { G::has_bitset_neighbors } -> micron::convertible_to<bool>;
  requires G::has_bitset_neighbors;
  graph.neighbor_words(vertex);
};

template<typename G>
concept contiguous_slots_graph = contiguous_slot_graph<G>;
template<typename G>
concept fast_incoming_adjacency_graph = fast_incoming_graph<G>;
template<typename G>
concept matrix_adjacency = matrix_adjacency_graph<G>;
template<typename G>
concept bitset_neighbor_access = bitset_neighbor_graph<G>;

};      // namespace graphs

};      // namespace micron::math
