//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "graph.hpp"

namespace micron::math::graphs
{

namespace __impl
{
template<typename Pred, typename G, typename Item>
[[nodiscard]] constexpr bool
accept(Pred &predicate, const G &graph, Item item)
{
  if constexpr ( requires { micron::invoke(predicate, graph, item); } )
    return static_cast<bool>(micron::invoke(predicate, graph, item));
  else
    return static_cast<bool>(micron::invoke(predicate, item));
}
};      // namespace __impl

struct accept_all {
  template<typename... T>
  [[nodiscard]] constexpr bool
  operator()(T &&...) const noexcept
  {
    return true;
  }
};

template<graph_model G> class reverse_graph_view
{
  const G *__graph{};

public:
  using vertex_property_type = typename G::vertex_property_type;
  using edge_property_type = typename G::edge_property_type;
  using graph_property_type = typename G::graph_property_type;
  using index_type = typename G::index_type;
  using direction_type = typename G::direction_type;
  using multiplicity_type = typename G::multiplicity_type;
  using loop_type = typename G::loop_type;
  using storage_type = typename G::storage_type;
  using allocator_type = typename G::allocator_type;
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  using micron_graph_tag = void;

  static constexpr bool is_directed = G::is_directed;
  static constexpr bool is_undirected = G::is_undirected;
  static constexpr bool is_simple = G::is_simple;
  static constexpr bool allows_parallel_edges = G::allows_parallel_edges;
  static constexpr bool allows_loops = G::allows_loops;

  constexpr explicit reverse_graph_view(const G &graph) noexcept : __graph(micron::addressof(graph)) { }

  [[nodiscard]] usize
  vertices_count() const noexcept
  {
    return __graph->vertices_count();
  }

  [[nodiscard]] usize
  edges_count() const noexcept
  {
    return __graph->edges_count();
  }

  [[nodiscard]] usize
  vertex_slots() const noexcept
  {
    return __graph->vertex_slots();
  }

  [[nodiscard]] usize
  edge_slots() const noexcept
  {
    return __graph->edge_slots();
  }

  [[nodiscard]] bool
  has_vertex(vertex_descriptor vertex) const noexcept
  {
    return __graph->has_vertex(vertex);
  }

  [[nodiscard]] bool
  has_edge(edge_descriptor edge) const noexcept
  {
    return __graph->has_edge(edge);
  }

  [[nodiscard]] bool
  has_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    if constexpr ( is_directed ) return __graph->has_edge(v, u);
    return __graph->has_edge(u, v);
  }

  [[nodiscard]] auto
  vertices() const noexcept
  {
    return __graph->vertices();
  }

  [[nodiscard]] auto
  out_neighbors(vertex_descriptor vertex) const noexcept
  {
    return __graph->in_neighbors(vertex);
  }

  [[nodiscard]] auto
  in_neighbors(vertex_descriptor vertex) const noexcept
  {
    return __graph->out_neighbors(vertex);
  }

  [[nodiscard]] auto
  neighbors(vertex_descriptor vertex) const noexcept
  {
    return out_neighbors(vertex);
  }

  [[nodiscard]] auto
  out_edges(vertex_descriptor vertex) const noexcept
  {
    return __graph->in_edges(vertex);
  }

  [[nodiscard]] auto
  in_edges(vertex_descriptor vertex) const noexcept
  {
    return __graph->out_edges(vertex);
  }

  [[nodiscard]] usize
  out_degree(vertex_descriptor vertex) const noexcept
  {
    return __graph->in_degree(vertex);
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor vertex) const noexcept
  {
    return __graph->out_degree(vertex);
  }

  [[nodiscard]] usize
  degree(vertex_descriptor vertex) const noexcept
  {
    return __graph->degree(vertex);
  }

  [[nodiscard]] vertex_descriptor
  source(edge_descriptor edge) const noexcept
  {
    return __graph->target(edge);
  }

  [[nodiscard]] vertex_descriptor
  target(edge_descriptor edge) const noexcept
  {
    return __graph->source(edge);
  }

  [[nodiscard]] vertex_descriptor
  opposite(edge_descriptor edge, vertex_descriptor vertex) const noexcept
  {
    return __graph->opposite(edge, vertex);
  }

  [[nodiscard]] const vertex_property_type &
  vertex_property_unchecked(vertex_descriptor vertex) const noexcept
  {
    return __graph->vertex_property_unchecked(vertex);
  }

  [[nodiscard]] const edge_property_type &
  edge_property_unchecked(edge_descriptor edge) const noexcept
  {
    return __graph->edge_property_unchecked(edge);
  }

  [[nodiscard]] const graph_property_type &
  graph_property() const noexcept
  {
    return __graph->graph_property();
  }

  struct edge_reference {
    edge_descriptor id;
    vertex_descriptor source;
    vertex_descriptor target;
    const edge_property_type &property;
  };

  class edge_iterator
  {
    using base_iterator = decltype(micron::declval<const G &>().edges().begin());
    const reverse_graph_view *__view{};
    base_iterator __iterator;

  public:
    edge_iterator(const reverse_graph_view *view, base_iterator iterator) : __view(view), __iterator(iterator) { }

    [[nodiscard]] edge_reference
    operator*() const
    {
      auto edge = *__iterator;
      return { edge.id, __view->source(edge.id), __view->target(edge.id), edge.property };
    }

    edge_iterator &
    operator++()
    {
      ++__iterator;
      return *this;
    }

    friend bool
    operator==(const edge_iterator &a, const edge_iterator &b)
    {
      return a.__iterator == b.__iterator;
    }

    friend bool
    operator!=(const edge_iterator &a, const edge_iterator &b)
    {
      return !(a == b);
    }
  };

  struct edge_range {
    const reverse_graph_view *view;

    [[nodiscard]] auto
    begin() const
    {
      return edge_iterator(view, view->__graph->edges().begin());
    }

    [[nodiscard]] auto
    end() const
    {
      return edge_iterator(view, view->__graph->edges().end());
    }
  };

  [[nodiscard]] edge_range
  edges() const noexcept
  {
    return { this };
  }
};

template<graph_model G>
[[nodiscard]] constexpr reverse_graph_view<G>
reverse(const G &graph) noexcept
{
  return reverse_graph_view<G>(graph);
}

template<graph_model G>
[[nodiscard]] constexpr reverse_graph_view<G>
transpose(const G &graph) noexcept
{
  return reverse_graph_view<G>(graph);
}

template<graph_model G, typename VertexPredicate, typename EdgePredicate = accept_all> class filtered_graph_view
{
  const G *__graph{};
  [[no_unique_address]] VertexPredicate __vertex_predicate;
  [[no_unique_address]] EdgePredicate __edge_predicate;

  [[nodiscard]] bool
  __accept_vertex(typename G::vertex_descriptor vertex) const
  {
    auto &predicate = const_cast<VertexPredicate &>(__vertex_predicate);
    return __impl::accept(predicate, *__graph, vertex);
  }

  [[nodiscard]] bool
  __accept_edge(typename G::edge_descriptor edge) const
  {
    auto &predicate = const_cast<EdgePredicate &>(__edge_predicate);
    return __impl::accept(predicate, *__graph, edge) && __accept_vertex(__graph->source(edge)) && __accept_vertex(__graph->target(edge));
  }

public:
  using vertex_property_type = typename G::vertex_property_type;
  using edge_property_type = typename G::edge_property_type;
  using graph_property_type = typename G::graph_property_type;
  using index_type = typename G::index_type;
  using direction_type = typename G::direction_type;
  using multiplicity_type = typename G::multiplicity_type;
  using loop_type = typename G::loop_type;
  using storage_type = typename G::storage_type;
  using allocator_type = typename G::allocator_type;
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  using micron_graph_tag = void;

  static constexpr bool is_directed = G::is_directed;
  static constexpr bool is_undirected = G::is_undirected;
  static constexpr bool is_simple = G::is_simple;
  static constexpr bool allows_parallel_edges = G::allows_parallel_edges;
  static constexpr bool allows_loops = G::allows_loops;

  filtered_graph_view(const G &graph, VertexPredicate vertex_predicate, EdgePredicate edge_predicate = {})
      : __graph(micron::addressof(graph)), __vertex_predicate(micron::move(vertex_predicate)),
        __edge_predicate(micron::move(edge_predicate))
  {
  }

  [[nodiscard]] usize
  vertex_slots() const noexcept
  {
    return __graph->vertex_slots();
  }

  [[nodiscard]] usize
  edge_slots() const noexcept
  {
    return __graph->edge_slots();
  }

  [[nodiscard]] bool
  has_vertex(vertex_descriptor vertex) const
  {
    return __graph->has_vertex(vertex) && __accept_vertex(vertex);
  }

  [[nodiscard]] bool
  has_edge(edge_descriptor edge) const
  {
    return __graph->has_edge(edge) && __accept_edge(edge);
  }

  [[nodiscard]] vertex_descriptor
  source(edge_descriptor edge) const noexcept
  {
    return __graph->source(edge);
  }

  [[nodiscard]] vertex_descriptor
  target(edge_descriptor edge) const noexcept
  {
    return __graph->target(edge);
  }

  [[nodiscard]] vertex_descriptor
  opposite(edge_descriptor edge, vertex_descriptor vertex) const noexcept
  {
    return __graph->opposite(edge, vertex);
  }

  [[nodiscard]] const vertex_property_type &
  vertex_property_unchecked(vertex_descriptor vertex) const noexcept
  {
    return __graph->vertex_property_unchecked(vertex);
  }

  [[nodiscard]] const edge_property_type &
  edge_property_unchecked(edge_descriptor edge) const noexcept
  {
    return __graph->edge_property_unchecked(edge);
  }

  [[nodiscard]] const graph_property_type &
  graph_property() const noexcept
  {
    return __graph->graph_property();
  }

private:
  class vertex_iterator
  {
    using base_iterator = decltype(micron::declval<const G &>().vertices().begin());
    const filtered_graph_view *__view{};
    base_iterator __iterator;
    base_iterator __end;

    void
    skip()
    {
      while ( __iterator != __end && !__view->__accept_vertex(*__iterator) ) ++__iterator;
    }

  public:
    vertex_iterator(const filtered_graph_view *view, base_iterator iterator, base_iterator end)
        : __view(view), __iterator(iterator), __end(end)
    {
      skip();
    }

    [[nodiscard]] vertex_descriptor
    operator*() const
    {
      return *__iterator;
    }

    vertex_iterator &
    operator++()
    {
      ++__iterator;
      skip();
      return *this;
    }

    friend bool
    operator==(const vertex_iterator &a, const vertex_iterator &b)
    {
      return a.__iterator == b.__iterator;
    }

    friend bool
    operator!=(const vertex_iterator &a, const vertex_iterator &b)
    {
      return !(a == b);
    }
  };

  class edge_iterator
  {
    using base_iterator = decltype(micron::declval<const G &>().edges().begin());
    const filtered_graph_view *__view{};
    base_iterator __iterator;
    base_iterator __end;

    void
    skip()
    {
      while ( __iterator != __end && !__view->__accept_edge((*__iterator).id) ) ++__iterator;
    }

  public:
    edge_iterator(const filtered_graph_view *view, base_iterator iterator, base_iterator end)
        : __view(view), __iterator(iterator), __end(end)
    {
      skip();
    }

    [[nodiscard]] auto
    operator*() const
    {
      return *__iterator;
    }

    edge_iterator &
    operator++()
    {
      ++__iterator;
      skip();
      return *this;
    }

    friend bool
    operator==(const edge_iterator &a, const edge_iterator &b)
    {
      return a.__iterator == b.__iterator;
    }

    friend bool
    operator!=(const edge_iterator &a, const edge_iterator &b)
    {
      return !(a == b);
    }
  };

  class incident_iterator
  {
    using base_iterator = decltype(micron::declval<const G &>().out_edges(micron::declval<vertex_descriptor>()).begin());
    const filtered_graph_view *__view{};
    base_iterator __iterator;
    base_iterator __end;

    void
    skip()
    {
      while ( __iterator != __end && !__view->__accept_edge(*__iterator) ) ++__iterator;
    }

  public:
    incident_iterator(const filtered_graph_view *view, base_iterator iterator, base_iterator end)
        : __view(view), __iterator(iterator), __end(end)
    {
      skip();
    }

    [[nodiscard]] edge_descriptor
    operator*() const
    {
      return *__iterator;
    }

    incident_iterator &
    operator++()
    {
      ++__iterator;
      skip();
      return *this;
    }

    friend bool
    operator==(const incident_iterator &a, const incident_iterator &b)
    {
      return a.__iterator == b.__iterator;
    }

    friend bool
    operator!=(const incident_iterator &a, const incident_iterator &b)
    {
      return !(a == b);
    }
  };

  class neighbor_iterator
  {
    incident_iterator __iterator;
    const filtered_graph_view *__view{};
    vertex_descriptor __from{};

  public:
    neighbor_iterator(const filtered_graph_view *view, vertex_descriptor from, incident_iterator iterator)
        : __iterator(iterator), __view(view), __from(from)
    {
    }

    [[nodiscard]] vertex_descriptor
    operator*() const
    {
      const edge_descriptor edge = *__iterator;
      if constexpr ( is_directed ) return __view->target(edge);
      return __view->opposite(edge, __from);
    }

    neighbor_iterator &
    operator++()
    {
      ++__iterator;
      return *this;
    }

    friend bool
    operator==(const neighbor_iterator &a, const neighbor_iterator &b)
    {
      return a.__iterator == b.__iterator;
    }

    friend bool
    operator!=(const neighbor_iterator &a, const neighbor_iterator &b)
    {
      return !(a == b);
    }
  };

public:
  struct vertex_range {
    const filtered_graph_view *view;

    [[nodiscard]] auto
    begin() const
    {
      auto range = view->__graph->vertices();
      return vertex_iterator(view, range.begin(), range.end());
    }

    [[nodiscard]] auto
    end() const
    {
      auto range = view->__graph->vertices();
      return vertex_iterator(view, range.end(), range.end());
    }
  };

  struct edge_range {
    const filtered_graph_view *view;

    [[nodiscard]] auto
    begin() const
    {
      auto range = view->__graph->edges();
      return edge_iterator(view, range.begin(), range.end());
    }

    [[nodiscard]] auto
    end() const
    {
      auto range = view->__graph->edges();
      return edge_iterator(view, range.end(), range.end());
    }
  };

  struct incident_range {
    const filtered_graph_view *view;
    vertex_descriptor vertex;
    bool incoming;

    [[nodiscard]] auto
    begin() const
    {
      if ( incoming ) {
        auto range = view->__graph->in_edges(vertex);
        return incident_iterator(view, range.begin(), range.end());
      }
      auto range = view->__graph->out_edges(vertex);
      return incident_iterator(view, range.begin(), range.end());
    }

    [[nodiscard]] auto
    end() const
    {
      if ( incoming ) {
        auto range = view->__graph->in_edges(vertex);
        return incident_iterator(view, range.end(), range.end());
      }
      auto range = view->__graph->out_edges(vertex);
      return incident_iterator(view, range.end(), range.end());
    }
  };

  struct neighbor_range {
    const filtered_graph_view *view;
    vertex_descriptor vertex;
    bool incoming;

    [[nodiscard]] auto
    begin() const
    {
      incident_range range{ view, vertex, incoming };
      return neighbor_iterator(view, vertex, range.begin());
    }

    [[nodiscard]] auto
    end() const
    {
      incident_range range{ view, vertex, incoming };
      return neighbor_iterator(view, vertex, range.end());
    }
  };

  [[nodiscard]] vertex_range
  vertices() const noexcept
  {
    return { this };
  }

  [[nodiscard]] edge_range
  edges() const noexcept
  {
    return { this };
  }

  [[nodiscard]] incident_range
  out_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, false };
  }

  [[nodiscard]] incident_range
  in_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, true };
  }

  [[nodiscard]] neighbor_range
  out_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, false };
  }

  [[nodiscard]] neighbor_range
  in_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, true };
  }

  [[nodiscard]] neighbor_range
  neighbors(vertex_descriptor vertex) const noexcept
  {
    return out_neighbors(vertex);
  }

  [[nodiscard]] usize
  vertices_count() const
  {
    usize count = 0;
    for ( auto ignored : vertices() ) {
      (void)ignored;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  edges_count() const
  {
    usize count = 0;
    for ( auto ignored : edges() ) {
      (void)ignored;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  out_degree(vertex_descriptor vertex) const
  {
    usize count = 0;
    for ( auto ignored : out_edges(vertex) ) {
      (void)ignored;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor vertex) const
  {
    usize count = 0;
    for ( auto ignored : in_edges(vertex) ) {
      (void)ignored;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  degree(vertex_descriptor vertex) const
  {
    if constexpr ( is_directed ) return in_degree(vertex) + out_degree(vertex);
    return out_degree(vertex);
  }
};

template<graph_model G, typename VertexPredicate, typename EdgePredicate = accept_all>
[[nodiscard]] auto
filtered(const G &graph, VertexPredicate vertex_predicate, EdgePredicate edge_predicate = {})
{
  return filtered_graph_view<G, VertexPredicate, EdgePredicate>(graph, micron::move(vertex_predicate), micron::move(edge_predicate));
}

template<graph_model G, typename VertexPredicate>
[[nodiscard]] auto
induced_subgraph(const G &graph, VertexPredicate vertex_predicate)
{
  return filtered(graph, micron::move(vertex_predicate), accept_all{});
}

};      // namespace micron::math::graphs

namespace micron::math
{

template<typename Matrix, micron::integral Index = u32, class Direction = graphs::directed_t> class matrix_graph_view
{
  const Matrix *__matrix{};
  usize __size{};

  [[nodiscard]] bool
  __present(usize row, usize column) const noexcept
  {
    using value_type = typename Matrix::value_type;
    return __matrix->at(row, column) != value_type(0);
  }

public:
  using vertex_property_type = empty_property;
  using edge_property_type = typename Matrix::value_type;
  using graph_property_type = empty_property;
  using index_type = Index;
  using direction_type = Direction;
  using multiplicity_type = graphs::simple_t;
  using loop_type = graphs::allow_loops_t;
  using storage_type = graphs::dense_adjacency_t;
  using allocator_type = micron::allocator_serial<>;
  using vertex_descriptor = vertex_id<Index>;
  using edge_descriptor = edge_id<Index>;
  using micron_graph_tag = void;

  static constexpr bool is_directed = micron::is_same_v<Direction, graphs::directed_t>;
  static constexpr bool is_undirected = !is_directed;
  static constexpr bool is_simple = true;
  static constexpr bool allows_parallel_edges = false;
  static constexpr bool allows_loops = true;

  explicit matrix_graph_view(const Matrix &matrix) noexcept
      : __matrix(micron::addressof(matrix)), __size(matrix.rows < matrix.cols ? matrix.rows : matrix.cols)
  {
  }

  [[nodiscard]] usize
  vertices_count() const noexcept
  {
    return __size;
  }

  [[nodiscard]] usize
  vertex_slots() const noexcept
  {
    return __size;
  }

  [[nodiscard]] usize
  edge_slots() const noexcept
  {
    return __size * __size;
  }

  [[nodiscard]] bool
  has_vertex(vertex_descriptor vertex) const noexcept
  {
    return vertex.valid() && static_cast<usize>(vertex.value) < __size;
  }

  [[nodiscard]] bool
  has_edge(edge_descriptor edge) const noexcept
  {
    if ( !edge.valid() ) return false;
    const usize raw = static_cast<usize>(edge.value);
    if ( raw >= edge_slots() ) return false;
    return __present(raw / __size, raw % __size);
  }

  [[nodiscard]] bool
  has_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    return has_vertex(u) && has_vertex(v) && __present(static_cast<usize>(u.value), static_cast<usize>(v.value));
  }

  [[nodiscard]] edge_descriptor
  find_edge(vertex_descriptor u, vertex_descriptor v) const noexcept
  {
    return has_edge(u, v) ? edge_descriptor(static_cast<Index>(static_cast<usize>(u.value) * __size + static_cast<usize>(v.value)))
                          : edge_descriptor::invalid();
  }

  [[nodiscard]] vertex_descriptor
  source(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(static_cast<Index>(static_cast<usize>(edge.value) / __size));
  }

  [[nodiscard]] vertex_descriptor
  target(edge_descriptor edge) const noexcept
  {
    return vertex_descriptor(static_cast<Index>(static_cast<usize>(edge.value) % __size));
  }

  [[nodiscard]] vertex_descriptor
  opposite(edge_descriptor edge, vertex_descriptor vertex) const noexcept
  {
    const auto u = source(edge);
    const auto v = target(edge);
    return u == vertex ? v : u;
  }

  [[nodiscard]] const edge_property_type &
  edge_property_unchecked(edge_descriptor edge) const noexcept
  {
    return __matrix->at(static_cast<usize>(source(edge).value), static_cast<usize>(target(edge).value));
  }

  [[nodiscard]] const vertex_property_type &
  vertex_property_unchecked(vertex_descriptor) const noexcept
  {
    static constexpr empty_property property{};
    return property;
  }

  [[nodiscard]] const graph_property_type &
  graph_property() const noexcept
  {
    static constexpr empty_property property{};
    return property;
  }

private:
  class vertex_iterator
  {
    Index __vertex{};

  public:
    explicit vertex_iterator(Index vertex) : __vertex(vertex) { }

    [[nodiscard]] vertex_descriptor
    operator*() const noexcept
    {
      return vertex_descriptor(__vertex);
    }

    vertex_iterator &
    operator++() noexcept
    {
      ++__vertex;
      return *this;
    }

    friend bool
    operator==(vertex_iterator a, vertex_iterator b) noexcept
    {
      return a.__vertex == b.__vertex;
    }

    friend bool
    operator!=(vertex_iterator a, vertex_iterator b) noexcept
    {
      return !(a == b);
    }
  };

public:
  struct edge_reference {
    edge_descriptor id;
    vertex_descriptor source;
    vertex_descriptor target;
    const edge_property_type &property;
  };

private:
  class edge_iterator
  {
    const matrix_graph_view *__view{};
    usize __raw{};

    void
    skip()
    {
      while ( __raw < __view->edge_slots() ) {
        const usize row = __raw / __view->__size;
        const usize column = __raw % __view->__size;
        if ( (is_directed || row <= column) && __view->__present(row, column) ) break;
        ++__raw;
      }
    }

  public:
    edge_iterator(const matrix_graph_view *view, usize raw) : __view(view), __raw(raw) { skip(); }

    [[nodiscard]] edge_reference
    operator*() const
    {
      edge_descriptor id(static_cast<Index>(__raw));
      return { id, __view->source(id), __view->target(id), __view->edge_property_unchecked(id) };
    }

    edge_iterator &
    operator++()
    {
      ++__raw;
      skip();
      return *this;
    }

    friend bool
    operator==(edge_iterator a, edge_iterator b)
    {
      return a.__view == b.__view && a.__raw == b.__raw;
    }

    friend bool
    operator!=(edge_iterator a, edge_iterator b)
    {
      return !(a == b);
    }
  };

  class neighbor_iterator
  {
    const matrix_graph_view *__view{};
    usize __fixed{};
    usize __scan{};
    bool __incoming{};

    void
    skip()
    {
      while ( __scan < __view->__size ) {
        const bool present = __incoming ? __view->__present(__scan, __fixed) : __view->__present(__fixed, __scan);
        if ( present ) break;
        ++__scan;
      }
    }

  public:
    neighbor_iterator(const matrix_graph_view *view, usize fixed, usize scan, bool incoming)
        : __view(view), __fixed(fixed), __scan(scan), __incoming(incoming)
    {
      skip();
    }

    [[nodiscard]] vertex_descriptor
    operator*() const noexcept
    {
      return vertex_descriptor(static_cast<Index>(__scan));
    }

    neighbor_iterator &
    operator++()
    {
      ++__scan;
      skip();
      return *this;
    }

    friend bool
    operator==(neighbor_iterator a, neighbor_iterator b)
    {
      return a.__view == b.__view && a.__fixed == b.__fixed && a.__scan == b.__scan && a.__incoming == b.__incoming;
    }

    friend bool
    operator!=(neighbor_iterator a, neighbor_iterator b)
    {
      return !(a == b);
    }
  };

  class incident_iterator
  {
    neighbor_iterator __neighbor;
    const matrix_graph_view *__view{};
    vertex_descriptor __vertex{};
    bool __incoming{};

  public:
    incident_iterator(const matrix_graph_view *view, vertex_descriptor vertex, usize scan, bool incoming)
        : __neighbor(view, static_cast<usize>(vertex.value), scan, incoming), __view(view), __vertex(vertex), __incoming(incoming)
    {
    }

    [[nodiscard]] edge_descriptor
    operator*() const noexcept
    {
      const vertex_descriptor other = *__neighbor;
      return __incoming ? __view->find_edge(other, __vertex) : __view->find_edge(__vertex, other);
    }

    incident_iterator &
    operator++()
    {
      ++__neighbor;
      return *this;
    }

    friend bool
    operator==(const incident_iterator &a, const incident_iterator &b)
    {
      return a.__neighbor == b.__neighbor;
    }

    friend bool
    operator!=(const incident_iterator &a, const incident_iterator &b)
    {
      return !(a == b);
    }
  };

public:
  struct vertex_range {
    const matrix_graph_view *view;

    [[nodiscard]] auto
    begin() const
    {
      return vertex_iterator(Index(0));
    }

    [[nodiscard]] auto
    end() const
    {
      return vertex_iterator(static_cast<Index>(view->__size));
    }
  };

  struct edge_range {
    const matrix_graph_view *view;

    [[nodiscard]] auto
    begin() const
    {
      return edge_iterator(view, 0);
    }

    [[nodiscard]] auto
    end() const
    {
      return edge_iterator(view, view->edge_slots());
    }
  };

  struct neighbor_range {
    const matrix_graph_view *view;
    vertex_descriptor vertex;
    bool incoming;

    [[nodiscard]] auto
    begin() const
    {
      return neighbor_iterator(view, static_cast<usize>(vertex.value), 0, incoming);
    }

    [[nodiscard]] auto
    end() const
    {
      return neighbor_iterator(view, static_cast<usize>(vertex.value), view->__size, incoming);
    }
  };

  struct incident_range {
    const matrix_graph_view *view;
    vertex_descriptor vertex;
    bool incoming;

    [[nodiscard]] auto
    begin() const
    {
      return incident_iterator(view, vertex, 0, incoming);
    }

    [[nodiscard]] auto
    end() const
    {
      return incident_iterator(view, vertex, view->__size, incoming);
    }
  };

  [[nodiscard]] vertex_range
  vertices() const noexcept
  {
    return { this };
  }

  [[nodiscard]] edge_range
  edges() const noexcept
  {
    return { this };
  }

  [[nodiscard]] neighbor_range
  out_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, false };
  }

  [[nodiscard]] neighbor_range
  in_neighbors(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, true };
  }

  [[nodiscard]] neighbor_range
  neighbors(vertex_descriptor vertex) const noexcept
  {
    return out_neighbors(vertex);
  }

  [[nodiscard]] incident_range
  out_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, false };
  }

  [[nodiscard]] incident_range
  in_edges(vertex_descriptor vertex) const noexcept
  {
    return { this, vertex, true };
  }

  [[nodiscard]] usize
  edges_count() const
  {
    usize count = 0;
    for ( auto edge : edges() ) {
      (void)edge;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  out_degree(vertex_descriptor vertex) const
  {
    usize count = 0;
    for ( auto neighbor : out_neighbors(vertex) ) {
      (void)neighbor;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  in_degree(vertex_descriptor vertex) const
  {
    usize count = 0;
    for ( auto neighbor : in_neighbors(vertex) ) {
      (void)neighbor;
      ++count;
    }
    return count;
  }

  [[nodiscard]] usize
  degree(vertex_descriptor vertex) const
  {
    if constexpr ( is_directed ) return in_degree(vertex) + out_degree(vertex);
    return out_degree(vertex);
  }
};

template<typename Matrix, micron::integral I = u32, class Direction = graphs::directed_t>
[[nodiscard]] auto
as_graph_view(const Matrix &matrix)
{
  return matrix_graph_view<Matrix, I, Direction>(matrix);
}

};      // namespace micron::math
