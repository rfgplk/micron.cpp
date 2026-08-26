//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "connectivity.hpp"
#include "paths.hpp"

namespace micron::math::graphs
{

template<micron::integral I, typename Weight> struct spanning_forest_result {
  algorithm_status status{ algorithm_status::ok };
  Weight weight{};
  usize components{};
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
};

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
kruskal(const G &graph, WeightMap weight_map)
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;

  struct candidate {
    W weight;
    typename G::edge_descriptor edge;
    typename G::vertex_descriptor source;
    typename G::vertex_descriptor target;

    [[nodiscard]] bool
    operator>(const candidate &other) const
    {
      return weight > other.weight;
    }

    [[nodiscard]] bool
    operator<(const candidate &other) const
    {
      return weight < other.weight;
    }
  };

  spanning_forest_result<I, W> result;
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  micron::vector<candidate, micron::allocator_serial<>, false> candidates;
  candidates.reserve(graph.edges_count());
  for ( auto edge : graph.edges() ) {
    const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge.id));
    if ( __impl::invalid_weight(weight) && !(micron::is_signed_v<W> || micron::is_floating_point_v<W>)) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    if constexpr ( micron::is_floating_point_v<W> ) {
      if ( __impl::nonfinite_weight(weight) ) {
        result.status = algorithm_status::invalid_weight;
        return result;
      }
    }
    candidates.push_back(candidate{ weight, edge.id, edge.source, edge.target });
  }
  candidates.sort();
  union_find<I> sets(graph.vertex_slots());
  result.components = graph.vertices_count();
  result.edges.reserve(graph.vertices_count() ? graph.vertices_count() - 1 : 0);
  for ( const candidate &item : candidates ) {
    if ( !sets.unite(item.source.value, item.target.value) ) continue;
    W next{};
    if ( !__impl::add_distance(result.weight, item.weight, next) ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    result.weight = next;
    result.edges.push_back(item.edge);
    --result.components;
  }
  if ( graph.vertices_count() > 0 && result.components != 1 ) result.status = algorithm_status::disconnected;
  return result;
}

template<graph_model G>
[[nodiscard]] auto
kruskal(const G &graph)
{
  return kruskal(graph, intrinsic_edge_weight{});
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
prim(const G &graph, WeightMap weight_map)
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;

  spanning_forest_result<I, W> result;
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  for ( auto edge : graph.edges() ) {
    const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge.id));
    if ( __impl::nonfinite_weight(weight) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
  }

  const usize slots = graph.vertex_slots();
  micron::vector<u8, micron::allocator_serial<>, false> selected(slots, u8(0));
  micron::vector<u8, micron::allocator_serial<>, false> key_valid(slots, u8(0));
  micron::vector<W, micron::allocator_serial<>, false> key(slots, W{});
  micron::vector<edge_descriptor, micron::allocator_serial<>, false> key_edge(slots, edge_descriptor::invalid());
  result.edges.reserve(graph.vertices_count() ? graph.vertices_count() - 1 : 0);

  auto update_frontier = [&](vertex_descriptor u) {
    for ( auto edge : graph.out_edges(u) ) {
      const vertex_descriptor v = graph.opposite(edge, u);
      const usize slot = static_cast<usize>(v.value);
      if ( selected.data()[slot] ) continue;
      const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge));
      if ( !key_valid.data()[slot] || weight < key.data()[slot] ) {
        key_valid.data()[slot] = 1;
        key.data()[slot] = weight;
        key_edge.data()[slot] = edge;
      }
    }
  };

  for ( auto root : graph.vertices() ) {
    const usize root_slot = static_cast<usize>(root.value);
    if ( selected.data()[root_slot] ) continue;
    ++result.components;
    selected.data()[root_slot] = 1;
    update_frontier(root);

    for ( ;; ) {
      vertex_descriptor next = vertex_descriptor::invalid();
      for ( auto candidate : graph.vertices() ) {
        const usize slot = static_cast<usize>(candidate.value);
        if ( selected.data()[slot] || !key_valid.data()[slot] ) continue;
        if ( !next.valid() || key.data()[slot] < key.data()[static_cast<usize>(next.value)] ) next = candidate;
      }
      if ( !next.valid() ) break;

      const usize slot = static_cast<usize>(next.value);
      W total{};
      if ( !__impl::add_distance(result.weight, key.data()[slot], total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      result.weight = total;
      result.edges.push_back(key_edge.data()[slot]);
      selected.data()[slot] = 1;
      key_valid.data()[slot] = 0;
      update_frontier(next);
    }
  }
  if ( graph.vertices_count() > 0 && result.components != 1 ) result.status = algorithm_status::disconnected;
  return result;
}

template<graph_model G>
[[nodiscard]] auto
prim(const G &graph)
{
  return prim(graph, intrinsic_edge_weight{});
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
boruvka(const G &graph, WeightMap weight_map)
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using edge_descriptor = typename G::edge_descriptor;

  spanning_forest_result<I, W> result;
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  for ( auto edge : graph.edges() ) {
    const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge.id));
    if ( __impl::nonfinite_weight(weight) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
  }

  const usize slots = graph.vertex_slots();
  union_find<I> sets(slots);
  micron::vector<edge_descriptor, micron::allocator_serial<>, false> cheapest(slots, edge_descriptor::invalid());
  micron::vector<W, micron::allocator_serial<>, false> cheapest_weight(slots, W{});
  result.components = graph.vertices_count();
  result.edges.reserve(result.components ? result.components - 1 : 0);

  while ( result.components > 1 ) {
    cheapest.fill(edge_descriptor::invalid());
    for ( auto edge : graph.edges() ) {
      const I a = sets.find(edge.source.value);
      const I b = sets.find(edge.target.value);
      if ( a == b ) continue;
      const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge.id));
      const usize as = static_cast<usize>(a);
      const usize bs = static_cast<usize>(b);
      if ( !cheapest.data()[as].valid() || weight < cheapest_weight.data()[as] ) {
        cheapest.data()[as] = edge.id;
        cheapest_weight.data()[as] = weight;
      }
      if ( !cheapest.data()[bs].valid() || weight < cheapest_weight.data()[bs] ) {
        cheapest.data()[bs] = edge.id;
        cheapest_weight.data()[bs] = weight;
      }
    }

    usize merged = 0;
    for ( usize slot = 0; slot < cheapest.size(); ++slot ) {
      const edge_descriptor edge = cheapest.data()[slot];
      if ( !edge.valid() ) continue;
      const auto u = graph.source(edge);
      const auto v = graph.target(edge);
      if ( !sets.unite(u.value, v.value) ) continue;
      const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge));
      W total{};
      if ( !__impl::add_distance(result.weight, weight, total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      result.weight = total;
      result.edges.push_back(edge);
      --result.components;
      ++merged;
    }
    if ( merged == 0 ) break;
  }
  if ( graph.vertices_count() > 0 && result.components != 1 ) result.status = algorithm_status::disconnected;
  return result;
}

template<graph_model G>
[[nodiscard]] auto
boruvka(const G &graph)
{
  return boruvka(graph, intrinsic_edge_weight{});
}

template<graph_model G, typename Rng>
[[nodiscard]] spanning_forest_result<typename G::index_type, usize>
random_spanning_tree(const G &graph, Rng &&rng)
{
  auto random_weight = [&rng](const auto &, auto) { return static_cast<usize>(micron::invoke(rng)); };
  auto result = kruskal(graph, random_weight);
  return { result.status, result.weight, result.components, micron::move(result.edges) };
}

template<micron::integral I> struct arborescence_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
};

template<graph_model G, typename WeightMap>
[[nodiscard]] arborescence_result<typename G::index_type>
directed_arborescence(const G &graph, typename G::vertex_descriptor root, WeightMap weight_map)
{
  using edge_descriptor = typename G::edge_descriptor;
  arborescence_result<typename G::index_type> result;
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  if ( !graph.has_vertex(root) ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }
  result.edges.reserve(graph.vertices_count() ? graph.vertices_count() - 1 : 0);
  for ( auto vertex : graph.vertices() ) {
    if ( vertex == root ) continue;
    edge_descriptor best = edge_descriptor::invalid();
    for ( auto edge : graph.in_edges(vertex) ) {
      if ( !best.valid() || __impl::weight(weight_map, graph, edge) < __impl::weight(weight_map, graph, best) ) best = edge;
    }
    if ( !best.valid() ) {
      result.status = algorithm_status::unreachable;
      result.edges.clear();
      return result;
    }
    result.edges.push_back(best);
  }
  // An incoming-edge certificate can still contain a directed cycle not
  // reachable from root.  Validate reachability in the selected subgraph.
  micron::vector<u8, micron::allocator_serial<>, false> reached(graph.vertex_slots(), u8(0));
  reached.data()[static_cast<usize>(root.value)] = 1;
  for ( usize pass = 0; pass < graph.vertices_count(); ++pass ) {
    bool changed = false;
    for ( auto edge : result.edges ) {
      const auto u = graph.source(edge);
      const auto v = graph.target(edge);
      if ( reached.data()[static_cast<usize>(u.value)] && !reached.data()[static_cast<usize>(v.value)] ) {
        reached.data()[static_cast<usize>(v.value)] = 1;
        changed = true;
      }
    }
    if ( !changed ) break;
  }
  for ( auto vertex : graph.vertices() )
    if ( !reached.data()[static_cast<usize>(vertex.value)] ) {
      result.status = algorithm_status::invalid_graph;
      result.edges.clear();
      break;
    }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
directed_arborescence(const G &graph, typename G::vertex_descriptor root)
{
  return directed_arborescence(graph, root, intrinsic_edge_weight{});
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
smallest_last_ordering(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> elimination;
  micron::vector<u8, micron::allocator_serial<>, false> active(graph.vertex_slots(), u8(0));
  micron::vector<usize, micron::allocator_serial<>, false> degree(graph.vertex_slots(), usize(0));
  elimination.reserve(graph.vertices_count());
  for ( auto vertex : graph.vertices() ) {
    active.data()[static_cast<usize>(vertex.value)] = 1;
    degree.data()[static_cast<usize>(vertex.value)] = graph.degree(vertex);
  }
  while ( elimination.size() < graph.vertices_count() ) {
    vertex_descriptor selected = vertex_descriptor::invalid();
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( !active.data()[slot] ) continue;
      if ( !selected.valid() || degree.data()[slot] < degree.data()[static_cast<usize>(selected.value)] ) selected = vertex;
    }
    if ( !selected.valid() ) break;
    active.data()[static_cast<usize>(selected.value)] = 0;
    elimination.push_back(selected);
    for ( auto neighbor : graph.out_neighbors(selected) ) {
      usize &value = degree.data()[static_cast<usize>(neighbor.value)];
      if ( active.data()[static_cast<usize>(neighbor.value)] && value ) --value;
    }
    if constexpr ( G::is_directed ) {
      for ( auto neighbor : graph.in_neighbors(selected) ) {
        usize &value = degree.data()[static_cast<usize>(neighbor.value)];
        if ( active.data()[static_cast<usize>(neighbor.value)] && value ) --value;
      }
    }
  }
  for ( usize a = 0, b = elimination.size() ? elimination.size() - 1 : 0; a < b; ++a, --b )
    micron::swap(elimination.data()[a], elimination.data()[b]);
  return elimination;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
cuthill_mckee_ordering(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;

  struct by_degree {
    vertex_descriptor vertex;
    usize degree;

    [[nodiscard]] bool
    operator>(const by_degree &other) const
    {
      return degree > other.degree;
    }

    [[nodiscard]] bool
    operator<(const by_degree &other) const
    {
      return degree < other.degree;
    }
  };

  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> result;
  micron::vector<u8, micron::allocator_serial<>, false> seen(graph.vertex_slots(), u8(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> queue;
  result.reserve(graph.vertices_count());
  queue.reserve(graph.vertices_count());
  while ( result.size() < graph.vertices_count() ) {
    vertex_descriptor root = vertex_descriptor::invalid();
    for ( auto vertex : graph.vertices() ) {
      if ( seen.data()[static_cast<usize>(vertex.value)] ) continue;
      if ( !root.valid() || graph.degree(vertex) < graph.degree(root) ) root = vertex;
    }
    if ( !root.valid() ) break;
    queue.clear();
    queue.push_back(root);
    seen.data()[static_cast<usize>(root.value)] = 1;
    usize head = 0;
    while ( head < queue.size() ) {
      const vertex_descriptor u = queue.data()[head++];
      result.push_back(u);
      micron::vector<by_degree, micron::allocator_serial<>, false> next;
      for ( auto v : graph.out_neighbors(u) )
        if ( !seen.data()[static_cast<usize>(v.value)] ) next.push_back({ v, graph.degree(v) });
      if constexpr ( G::is_directed )
        for ( auto v : graph.in_neighbors(u) )
          if ( !seen.data()[static_cast<usize>(v.value)] ) next.push_back({ v, graph.degree(v) });
      next.sort();
      for ( auto item : next ) {
        const usize slot = static_cast<usize>(item.vertex.value);
        if ( seen.data()[slot] ) continue;
        seen.data()[slot] = 1;
        queue.push_back(item.vertex);
      }
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
reverse_cuthill_mckee_ordering(const G &graph)
{
  auto result = cuthill_mckee_ordering(graph);
  for ( usize a = 0, b = result.size() ? result.size() - 1 : 0; a < b; ++a, --b ) micron::swap(result.data()[a], result.data()[b]);
  return result;
}

template<graph_model G>
[[nodiscard]] auto
king_ordering(const G &graph)
{
  return reverse_cuthill_mckee_ordering(graph);
}

template<graph_model G>
[[nodiscard]] auto
sloan_ordering(const G &graph)
{
  return reverse_cuthill_mckee_ordering(graph);
}

template<graph_model G>
[[nodiscard]] auto
minimum_degree_ordering(const G &graph)
{
  return smallest_last_ordering(graph);
}

template<micron::integral I>
[[nodiscard]] vertex_id<I>
lowest_common_ancestor(const micron::vector<vertex_id<I>, micron::allocator_serial<>, false> &parent, vertex_id<I> a, vertex_id<I> b)
{
  micron::vector<u8, micron::allocator_serial<>, false> ancestor(parent.size(), u8(0));
  for ( usize guard = 0; a.valid() && static_cast<usize>(a.value) < parent.size() && guard <= parent.size(); ++guard ) {
    ancestor.data()[static_cast<usize>(a.value)] = 1;
    const vertex_id<I> next = parent.data()[static_cast<usize>(a.value)];
    if ( next == a ) break;
    a = next;
  }
  for ( usize guard = 0; b.valid() && static_cast<usize>(b.value) < parent.size() && guard <= parent.size(); ++guard ) {
    if ( ancestor.data()[static_cast<usize>(b.value)] ) return b;
    const vertex_id<I> next = parent.data()[static_cast<usize>(b.value)];
    if ( next == b ) break;
    b = next;
  }
  return vertex_id<I>::invalid();
}

};      // namespace micron::math::graphs
