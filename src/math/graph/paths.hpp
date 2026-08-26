//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../numerics.hpp"
#include "../../type_traits.hpp"
#include "../../vector/vector.hpp"
#include "../ieee.hpp"
#include "graph.hpp"
#include "traversal.hpp"

namespace micron::math::graphs
{

struct intrinsic_edge_weight {
  template<graph_model G>
  [[nodiscard]] constexpr decltype(auto)
  operator()(const G &graph, typename G::edge_descriptor edge) const noexcept
  {
    if constexpr ( weighted_bundle<typename G::edge_property_type> )
      return graph.edge_property_unchecked(edge).weight;
    else
      return usize(1);
  }
};

namespace __impl
{

template<typename WeightMap, typename G>
[[nodiscard]] constexpr decltype(auto)
weight(WeightMap &map, const G &graph, typename G::edge_descriptor edge)
  requires(requires { micron::invoke(map, graph, edge); } || requires { micron::invoke(map, edge); } || requires { map[edge]; })
{
  if constexpr ( requires { micron::invoke(map, graph, edge); } )
    return micron::invoke(map, graph, edge);
  else if constexpr ( requires { micron::invoke(map, edge); } )
    return micron::invoke(map, edge);
  else
    return map[edge];
}

template<typename W>
[[nodiscard]] constexpr bool
invalid_weight(const W &weight) noexcept
{
  if constexpr ( micron::is_floating_point_v<W> ) return !micron::math::ieee::is_finite(weight);
  if constexpr ( micron::is_signed_v<W> ) return weight < W(0);
  return false;
}

template<typename W>
[[nodiscard]] constexpr bool
nonfinite_weight(const W &weight) noexcept
{
  if constexpr ( micron::is_floating_point_v<W> ) return !micron::math::ieee::is_finite(weight);
  return false;
}

template<typename D, typename W>
[[nodiscard]] constexpr bool
add_distance(const D &a, const W &b, D &result) noexcept
{
  if constexpr ( micron::is_integral_v<D> && micron::is_integral_v<W> ) {
    return !__builtin_add_overflow(a, static_cast<D>(b), &result);
  } else {
    result = static_cast<D>(a + static_cast<D>(b));
    if constexpr ( micron::is_floating_point_v<D> ) return micron::math::ieee::is_finite(result);
    return true;
  }
}

template<typename D, typename W>
[[nodiscard]] constexpr bool
sub_distance(const D &a, const W &b, D &result) noexcept
{
  if constexpr ( micron::is_integral_v<D> && micron::is_integral_v<W> ) {
    return !__builtin_sub_overflow(a, static_cast<D>(b), &result);
  } else {
    result = static_cast<D>(a - static_cast<D>(b));
    if constexpr ( micron::is_floating_point_v<D> ) return micron::math::ieee::is_finite(result);
    return true;
  }
}

template<graph_model G>
[[nodiscard]] constexpr typename G::vertex_descriptor
edge_neighbor(const G &graph, typename G::edge_descriptor edge, typename G::vertex_descriptor from) noexcept
{
  if constexpr ( G::is_directed ) return graph.target(edge);
  return graph.opposite(edge, from);
}

};      // namespace __impl

template<micron::integral I, typename D> struct shortest_path_workspace {
  micron::vector<D, micron::allocator_serial<>, false> distance;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> parent;
  micron::vector<u8, micron::allocator_serial<>, false> reached;
  micron::vector<u8, micron::allocator_serial<>, false> settled;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> queue;
  micron::vector<D, micron::allocator_serial<>, false> queue_distance;

  void
  reserve(usize slots, usize queue_entries = 0)
  {
    distance.reserve(slots);
    parent.reserve(slots);
    reached.reserve(slots);
    settled.reserve(slots);
    queue.reserve(queue_entries > slots ? queue_entries : slots);
    queue_distance.reserve(queue_entries > slots ? queue_entries : slots);
  }

  void
  reset(usize slots)
  {
    distance.resize(slots, D{});
    distance.fill(D{});
    parent.resize(slots, vertex_id<I>::invalid());
    parent.fill(vertex_id<I>::invalid());
    reached.resize(slots, u8(0));
    reached.fill(u8(0));
    settled.resize(slots, u8(0));
    settled.fill(u8(0));
    queue.clear();
    queue_distance.clear();
  }
};

template<micron::integral I, typename D> struct shortest_paths_result {
  using distance_type = D;
  algorithm_status status{ algorithm_status::ok };
  vertex_id<I> source{};
  micron::vector<D, micron::allocator_serial<>, false> distance;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> parent;
  micron::vector<u8, micron::allocator_serial<>, false> reached;

  [[nodiscard]] bool
  contains(vertex_id<I> vertex) const noexcept
  {
    const usize slot = static_cast<usize>(vertex.value);
    return vertex.valid() && slot < reached.size() && reached.data()[slot] != 0;
  }

  [[nodiscard]] const D *
  try_distance(vertex_id<I> vertex) const noexcept
  {
    return contains(vertex) ? distance.data() + static_cast<usize>(vertex.value) : nullptr;
  }

  [[nodiscard]] micron::vector<vertex_id<I>, micron::allocator_serial<>, false>
  path_to(vertex_id<I> target) const
  {
    micron::vector<vertex_id<I>, micron::allocator_serial<>, false> path;
    if ( !contains(target) ) return path;
    vertex_id<I> current = target;
    for ( usize guard = 0; guard <= parent.size(); ++guard ) {
      path.push_back(current);
      if ( current == source ) break;
      const usize slot = static_cast<usize>(current.value);
      if ( slot >= parent.size() || !parent.data()[slot].valid() ) {
        path.clear();
        return path;
      }
      current = parent.data()[slot];
    }
    for ( usize a = 0, b = path.size() ? path.size() - 1 : 0; a < b; ++a, --b ) micron::swap(path.data()[a], path.data()[b]);
    return path;
  }
};

template<graph_model G, typename WeightMap>
algorithm_status
dijkstra_into(
    const G &graph, typename G::vertex_descriptor source,
    shortest_path_workspace<typename G::index_type,
                            micron::remove_cvref_t<decltype(__impl::weight(micron::declval<WeightMap &>(), micron::declval<const G &>(),
                                                                           micron::declval<typename G::edge_descriptor>()))>> &workspace,
    WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using vertex_descriptor = typename G::vertex_descriptor;
  if ( !graph.has_vertex(source) ) return algorithm_status::invalid_vertex;
  workspace.reset(graph.vertex_slots());
  const usize source_slot = static_cast<usize>(source.value);
  workspace.reached.data()[source_slot] = 1;
  workspace.distance.data()[source_slot] = D{};
  workspace.parent.data()[source_slot] = source;
  workspace.queue.push_back(source);
  workspace.queue_distance.push_back(D{});

  auto push_heap = [&](vertex_descriptor vertex) {
    workspace.queue.push_back(vertex);
    workspace.queue_distance.push_back(workspace.distance.data()[static_cast<usize>(vertex.value)]);
    usize child = workspace.queue.size() - 1;
    while ( child ) {
      const usize parent = (child - 1) / 2;
      if ( !(workspace.queue_distance.data()[child] < workspace.queue_distance.data()[parent]) ) break;
      micron::swap(workspace.queue.data()[child], workspace.queue.data()[parent]);
      micron::swap(workspace.queue_distance.data()[child], workspace.queue_distance.data()[parent]);
      child = parent;
    }
  };
  auto pop_heap = [&]() {
    const vertex_descriptor result = workspace.queue.data()[0];
    const vertex_descriptor tail = workspace.queue.data()[workspace.queue.size() - 1];
    const D tail_distance = workspace.queue_distance.data()[workspace.queue_distance.size() - 1];
    workspace.queue.pop_back();
    workspace.queue_distance.pop_back();
    if ( !workspace.queue.empty() ) {
      workspace.queue.data()[0] = tail;
      workspace.queue_distance.data()[0] = tail_distance;
      usize parent = 0;
      for ( ;; ) {
        const usize left = parent * 2 + 1;
        if ( left >= workspace.queue.size() ) break;
        const usize right = left + 1;
        usize child = left;
        if ( right < workspace.queue.size() && workspace.queue_distance.data()[right] < workspace.queue_distance.data()[left] )
          child = right;
        if ( !(workspace.queue_distance.data()[child] < workspace.queue_distance.data()[parent]) ) break;
        micron::swap(workspace.queue.data()[child], workspace.queue.data()[parent]);
        micron::swap(workspace.queue_distance.data()[child], workspace.queue_distance.data()[parent]);
        parent = child;
      }
    }
    return result;
  };

  while ( !workspace.queue.empty() ) {
    const vertex_descriptor selected = pop_heap();
    const usize selected_slot = static_cast<usize>(selected.value);
    if ( workspace.settled.data()[selected_slot] ) continue;
    workspace.settled.data()[selected_slot] = 1;

    for ( auto edge : graph.out_edges(selected) ) {
      const auto raw_weight = __impl::weight(weight_map, graph, edge);
      if ( __impl::invalid_weight(raw_weight) ) return algorithm_status::invalid_weight;
      const vertex_descriptor neighbor = __impl::edge_neighbor(graph, edge, selected);
      const usize neighbor_slot = static_cast<usize>(neighbor.value);
      D candidate{};
      if ( !__impl::add_distance(workspace.distance.data()[selected_slot], raw_weight, candidate) ) return algorithm_status::overflow;
      if ( !workspace.reached.data()[neighbor_slot] || candidate < workspace.distance.data()[neighbor_slot] ) {
        workspace.reached.data()[neighbor_slot] = 1;
        workspace.distance.data()[neighbor_slot] = candidate;
        workspace.parent.data()[neighbor_slot] = selected;
        push_heap(neighbor);
      }
    }
  }
  return algorithm_status::ok;
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
dijkstra(const G &graph, typename G::vertex_descriptor source, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  shortest_path_workspace<typename G::index_type, D> workspace;
  workspace.reserve(graph.vertex_slots(), graph.edges_count() + 1);
  const algorithm_status status = dijkstra_into(graph, source, workspace, weight_map);
  return shortest_paths_result<typename G::index_type, D>{ status, source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                           micron::move(workspace.reached) };
}

template<graph_model G>
[[nodiscard]] auto
dijkstra(const G &graph, typename G::vertex_descriptor source)
{
  return dijkstra(graph, source, intrinsic_edge_weight{});
}

template<graph_model G, micron::integral U>
[[nodiscard]] auto
dijkstra(const G &graph, U source)
{
  return dijkstra(graph, typename G::vertex_descriptor(static_cast<typename G::index_type>(source)), intrinsic_edge_weight{});
}

template<graph_model G>
[[nodiscard]] shortest_paths_result<typename G::index_type, usize>
unweighted_shortest_paths(const G &graph, typename G::vertex_descriptor source)
{
  auto traversal = bfs(graph, source);
  return { traversal.status, source, micron::move(traversal.depth), micron::move(traversal.parent), micron::move(traversal.reached) };
}

template<graph_model G, micron::integral U>
[[nodiscard]] auto
unweighted_shortest_paths(const G &graph, U source)
{
  return unweighted_shortest_paths(graph, typename G::vertex_descriptor(static_cast<typename G::index_type>(source)));
}

template<micron::integral I, typename D> struct path_result {
  algorithm_status status{ algorithm_status::unreachable };
  D distance{};
  bool has_distance{};
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> path;
};

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
shortest_path(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target, WeightMap weight_map)
{
  auto paths = dijkstra(graph, source, weight_map);
  using D = typename decltype(paths)::distance_type;
  if ( paths.status != algorithm_status::ok ) return path_result<typename G::index_type, D>{ paths.status, D{}, false, {} };
  if ( !paths.contains(target) ) return path_result<typename G::index_type, D>{ algorithm_status::unreachable, D{}, false, {} };
  return path_result<typename G::index_type, D>{ algorithm_status::ok, paths.distance.data()[static_cast<usize>(target.value)], true,
                                                 paths.path_to(target) };
}

template<graph_model G>
[[nodiscard]] auto
shortest_path(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target)
{
  return shortest_path(graph, source, target, intrinsic_edge_weight{});
}

namespace __impl
{
template<typename Heuristic, typename G>
[[nodiscard]] constexpr decltype(auto)
heuristic(Heuristic &fn, const G &graph, typename G::vertex_descriptor vertex, typename G::vertex_descriptor target)
{
  if constexpr ( requires { micron::invoke(fn, graph, vertex, target); } )
    return micron::invoke(fn, graph, vertex, target);
  else if constexpr ( requires { micron::invoke(fn, vertex, target); } )
    return micron::invoke(fn, vertex, target);
  else
    return micron::invoke(fn, vertex);
}
};      // namespace __impl

template<graph_model G, typename Heuristic, typename WeightMap = intrinsic_edge_weight>
[[nodiscard]] auto
astar(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target, Heuristic heuristic,
      WeightMap weight_map = {})
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using vertex_descriptor = typename G::vertex_descriptor;
  shortest_path_workspace<typename G::index_type, D> workspace;
  workspace.reserve(graph.vertex_slots());
  workspace.reset(graph.vertex_slots());
  if ( !graph.has_vertex(source) || !graph.has_vertex(target) )
    return path_result<typename G::index_type, D>{ algorithm_status::invalid_vertex, D{}, false, {} };
  const usize ss = static_cast<usize>(source.value);
  workspace.reached.data()[ss] = 1;
  workspace.parent.data()[ss] = source;
  algorithm_status status = algorithm_status::ok;

  for ( usize iteration = 0; iteration < graph.vertices_count(); ++iteration ) {
    vertex_descriptor selected = vertex_descriptor::invalid();
    D selected_score{};
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( !workspace.reached.data()[slot] || workspace.settled.data()[slot] ) continue;
      const auto h = __impl::heuristic(heuristic, graph, vertex, target);
      if ( __impl::invalid_weight(h) ) {
        status = algorithm_status::invalid_weight;
        break;
      }
      D score{};
      if ( !__impl::add_distance(workspace.distance.data()[slot], h, score) ) {
        status = algorithm_status::overflow;
        break;
      }
      if ( !selected.valid() || score < selected_score ) {
        selected = vertex;
        selected_score = score;
      }
    }
    if ( status != algorithm_status::ok || !selected.valid() ) break;
    if ( selected == target ) break;
    const usize us = static_cast<usize>(selected.value);
    workspace.settled.data()[us] = 1;
    for ( auto edge : graph.out_edges(selected) ) {
      const auto weight = __impl::weight(weight_map, graph, edge);
      if ( __impl::invalid_weight(weight) ) {
        status = algorithm_status::invalid_weight;
        break;
      }
      const vertex_descriptor v = __impl::edge_neighbor(graph, edge, selected);
      const usize vs = static_cast<usize>(v.value);
      D candidate{};
      if ( !__impl::add_distance(workspace.distance.data()[us], weight, candidate) ) {
        status = algorithm_status::overflow;
        break;
      }
      if ( !workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs] ) {
        workspace.reached.data()[vs] = 1;
        workspace.distance.data()[vs] = candidate;
        workspace.parent.data()[vs] = selected;
      }
    }
    if ( status != algorithm_status::ok ) break;
  }
  if ( status != algorithm_status::ok ) return path_result<typename G::index_type, D>{ status, D{}, false, {} };
  if ( !workspace.reached.data()[static_cast<usize>(target.value)] )
    return path_result<typename G::index_type, D>{ algorithm_status::unreachable, D{}, false, {} };
  shortest_paths_result<typename G::index_type, D> paths{ algorithm_status::ok, source, micron::move(workspace.distance),
                                                          micron::move(workspace.parent), micron::move(workspace.reached) };
  return path_result<typename G::index_type, D>{ algorithm_status::ok, paths.distance.data()[static_cast<usize>(target.value)], true,
                                                 paths.path_to(target) };
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
bellman_ford(const G &graph, typename G::vertex_descriptor source, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using vertex_descriptor = typename G::vertex_descriptor;
  shortest_path_workspace<typename G::index_type, D> workspace;
  workspace.reserve(graph.vertex_slots());
  workspace.reset(graph.vertex_slots());
  if ( !graph.has_vertex(source) )
    return shortest_paths_result<typename G::index_type, D>{ algorithm_status::invalid_vertex, source, micron::move(workspace.distance),
                                                             micron::move(workspace.parent), micron::move(workspace.reached) };
  const usize ss = static_cast<usize>(source.value);
  workspace.reached.data()[ss] = 1;
  workspace.parent.data()[ss] = source;

  algorithm_status status = algorithm_status::ok;
  auto relax = [&](vertex_descriptor u, vertex_descriptor v, const auto &raw_weight) -> bool {
    const usize us = static_cast<usize>(u.value);
    const usize vs = static_cast<usize>(v.value);
    if ( !workspace.reached.data()[us] ) return false;
    D candidate{};
    if ( !__impl::add_distance(workspace.distance.data()[us], raw_weight, candidate) ) {
      status = algorithm_status::overflow;
      return false;
    }
    if ( !workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs] ) {
      workspace.reached.data()[vs] = 1;
      workspace.distance.data()[vs] = candidate;
      workspace.parent.data()[vs] = u;
      return true;
    }
    return false;
  };

  for ( usize pass = 1; pass < graph.vertices_count(); ++pass ) {
    bool changed = false;
    for ( auto edge : graph.edges() ) {
      const auto raw_weight = __impl::weight(weight_map, graph, edge.id);
      if constexpr ( micron::is_floating_point_v<decltype(raw_weight)> ) {
        if ( __impl::nonfinite_weight(raw_weight) ) status = algorithm_status::invalid_weight;
      }
      changed |= relax(edge.source, edge.target, raw_weight);
      if constexpr ( !G::is_directed ) changed |= relax(edge.target, edge.source, raw_weight);
      if ( status != algorithm_status::ok ) break;
    }
    if ( status != algorithm_status::ok || !changed ) break;
  }

  if ( status == algorithm_status::ok ) {
    for ( auto edge : graph.edges() ) {
      const auto raw_weight = __impl::weight(weight_map, graph, edge.id);
      const usize us = static_cast<usize>(edge.source.value);
      const usize vs = static_cast<usize>(edge.target.value);
      D candidate{};
      if ( workspace.reached.data()[us] && __impl::add_distance(workspace.distance.data()[us], raw_weight, candidate)
           && (!workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs]) ) {
        status = algorithm_status::negative_cycle;
        break;
      }
      if constexpr ( !G::is_directed ) {
        if ( workspace.reached.data()[vs] && __impl::add_distance(workspace.distance.data()[vs], raw_weight, candidate)
             && candidate < workspace.distance.data()[us] ) {
          status = algorithm_status::negative_cycle;
          break;
        }
      }
    }
  }
  return shortest_paths_result<typename G::index_type, D>{ status, source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                           micron::move(workspace.reached) };
}

template<graph_model G>
[[nodiscard]] auto
bellman_ford(const G &graph, typename G::vertex_descriptor source)
{
  return bellman_ford(graph, source, intrinsic_edge_weight{});
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
spfa(const G &graph, typename G::vertex_descriptor source, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using vertex_descriptor = typename G::vertex_descriptor;
  shortest_path_workspace<typename G::index_type, D> workspace;
  workspace.reserve(graph.vertex_slots());
  workspace.reset(graph.vertex_slots());
  if ( !graph.has_vertex(source) )
    return shortest_paths_result<typename G::index_type, D>{ algorithm_status::invalid_vertex, source, micron::move(workspace.distance),
                                                             micron::move(workspace.parent), micron::move(workspace.reached) };

  micron::vector<u8, micron::allocator_serial<>, false> queued(graph.vertex_slots(), u8(0));
  micron::vector<usize, micron::allocator_serial<>, false> relaxations(graph.vertex_slots(), usize(0));
  workspace.queue.push_back(source);
  queued.data()[static_cast<usize>(source.value)] = 1;
  workspace.reached.data()[static_cast<usize>(source.value)] = 1;
  workspace.parent.data()[static_cast<usize>(source.value)] = source;
  usize head = 0;
  algorithm_status status = algorithm_status::ok;
  while ( head < workspace.queue.size() && status == algorithm_status::ok ) {
    const vertex_descriptor u = workspace.queue.data()[head++];
    const usize us = static_cast<usize>(u.value);
    queued.data()[us] = 0;
    for ( auto edge : graph.out_edges(u) ) {
      const auto raw_weight = __impl::weight(weight_map, graph, edge);
      if constexpr ( micron::is_floating_point_v<decltype(raw_weight)> )
        if ( __impl::nonfinite_weight(raw_weight) ) {
          status = algorithm_status::invalid_weight;
          break;
        }
      const vertex_descriptor v = __impl::edge_neighbor(graph, edge, u);
      const usize vs = static_cast<usize>(v.value);
      D candidate{};
      if ( !__impl::add_distance(workspace.distance.data()[us], raw_weight, candidate) ) {
        status = algorithm_status::overflow;
        break;
      }
      if ( !workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs] ) {
        workspace.reached.data()[vs] = 1;
        workspace.distance.data()[vs] = candidate;
        workspace.parent.data()[vs] = u;
        if ( ++relaxations.data()[vs] >= graph.vertices_count() ) {
          status = algorithm_status::negative_cycle;
          break;
        }
        if ( !queued.data()[vs] ) {
          queued.data()[vs] = 1;
          workspace.queue.push_back(v);
        }
      }
    }
  }
  return shortest_paths_result<typename G::index_type, D>{ status, source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                           micron::move(workspace.reached) };
}

template<graph_model G>
[[nodiscard]] auto
spfa(const G &graph, typename G::vertex_descriptor source)
{
  return spfa(graph, source, intrinsic_edge_weight{});
}

template<micron::integral I> struct topological_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> order;
};

template<graph_model G>
[[nodiscard]] topological_result<typename G::index_type>
topological_sort(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  topological_result<typename G::index_type> result;
  if constexpr ( !G::is_directed ) {
    if ( graph.edges_count() != 0 ) {
      result.status = algorithm_status::not_a_dag;
      return result;
    }
  }
  micron::vector<usize, micron::allocator_serial<>, false> indegree(graph.vertex_slots(), usize(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());
  result.order.reserve(graph.vertices_count());
  for ( auto vertex : graph.vertices() ) {
    indegree.data()[static_cast<usize>(vertex.value)] = graph.in_degree(vertex);
    if ( indegree.data()[static_cast<usize>(vertex.value)] == 0 ) queue.push_back(vertex);
  }
  usize head = 0;
  while ( head < queue.size() ) {
    const vertex_descriptor u = queue.data()[head++];
    result.order.push_back(u);
    for ( auto v : graph.out_neighbors(u) ) {
      usize &degree = indegree.data()[static_cast<usize>(v.value)];
      if ( degree > 0 && --degree == 0 ) queue.push_back(v);
    }
  }
  if ( result.order.size() != graph.vertices_count() ) result.status = algorithm_status::not_a_dag;
  return result;
}

template<graph_model G>
[[nodiscard]] auto
topological_generations(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  micron::vector<micron::vector<vertex_descriptor, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> result;
  auto sorted = topological_sort(graph);
  if ( sorted.status != algorithm_status::ok ) return result;
  micron::vector<usize, micron::allocator_serial<>, false> level(graph.vertex_slots(), usize(0));
  usize max_level = 0;
  for ( auto u : sorted.order ) {
    const usize next = level.data()[static_cast<usize>(u.value)] + 1;
    for ( auto v : graph.out_neighbors(u) ) {
      usize &target_level = level.data()[static_cast<usize>(v.value)];
      if ( next > target_level ) target_level = next;
      if ( target_level > max_level ) max_level = target_level;
    }
  }
  result.resize(max_level + 1);
  for ( auto vertex : sorted.order ) result.data()[level.data()[static_cast<usize>(vertex.value)]].push_back(vertex);
  return result;
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
dag_shortest_paths(const G &graph, typename G::vertex_descriptor source, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  shortest_path_workspace<typename G::index_type, D> workspace;
  workspace.reserve(graph.vertex_slots());
  workspace.reset(graph.vertex_slots());
  auto sorted = topological_sort(graph);
  if ( sorted.status != algorithm_status::ok || !graph.has_vertex(source) )
    return shortest_paths_result<typename G::index_type, D>{ sorted.status == algorithm_status::ok ? algorithm_status::invalid_vertex
                                                                                                   : sorted.status,
                                                             source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                             micron::move(workspace.reached) };
  const usize ss = static_cast<usize>(source.value);
  workspace.reached.data()[ss] = 1;
  workspace.parent.data()[ss] = source;
  algorithm_status status = algorithm_status::ok;
  for ( auto u : sorted.order ) {
    const usize us = static_cast<usize>(u.value);
    if ( !workspace.reached.data()[us] ) continue;
    for ( auto edge : graph.out_edges(u) ) {
      const auto raw_weight = __impl::weight(weight_map, graph, edge);
      if ( __impl::nonfinite_weight(raw_weight) ) {
        status = algorithm_status::invalid_weight;
        break;
      }
      const auto v = graph.target(edge);
      const usize vs = static_cast<usize>(v.value);
      D candidate{};
      if ( !__impl::add_distance(workspace.distance.data()[us], raw_weight, candidate) ) {
        status = algorithm_status::overflow;
        break;
      }
      if ( !workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs] ) {
        workspace.reached.data()[vs] = 1;
        workspace.distance.data()[vs] = candidate;
        workspace.parent.data()[vs] = u;
      }
    }
    if ( status != algorithm_status::ok ) break;
  }
  return shortest_paths_result<typename G::index_type, D>{ status, source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                           micron::move(workspace.reached) };
}

template<graph_model G>
[[nodiscard]] auto
dag_shortest_paths(const G &graph, typename G::vertex_descriptor source)
{
  return dag_shortest_paths(graph, source, intrinsic_edge_weight{});
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
dag_longest_paths(const G &graph, typename G::vertex_descriptor source, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  shortest_path_workspace<typename G::index_type, D> workspace;
  workspace.reserve(graph.vertex_slots());
  workspace.reset(graph.vertex_slots());
  auto sorted = topological_sort(graph);
  if ( sorted.status != algorithm_status::ok || !graph.has_vertex(source) )
    return shortest_paths_result<typename G::index_type, D>{ sorted.status == algorithm_status::ok ? algorithm_status::invalid_vertex
                                                                                                   : sorted.status,
                                                             source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                             micron::move(workspace.reached) };
  const usize ss = static_cast<usize>(source.value);
  workspace.reached.data()[ss] = 1;
  workspace.parent.data()[ss] = source;
  algorithm_status status = algorithm_status::ok;
  for ( auto u : sorted.order ) {
    const usize us = static_cast<usize>(u.value);
    if ( !workspace.reached.data()[us] ) continue;
    for ( auto edge : graph.out_edges(u) ) {
      const auto raw_weight = __impl::weight(weight_map, graph, edge);
      if ( __impl::nonfinite_weight(raw_weight) ) {
        status = algorithm_status::invalid_weight;
        break;
      }
      const auto v = graph.target(edge);
      const usize vs = static_cast<usize>(v.value);
      D candidate{};
      if ( !__impl::add_distance(workspace.distance.data()[us], raw_weight, candidate) ) {
        status = algorithm_status::overflow;
        break;
      }
      if ( !workspace.reached.data()[vs] || workspace.distance.data()[vs] < candidate ) {
        workspace.reached.data()[vs] = 1;
        workspace.distance.data()[vs] = candidate;
        workspace.parent.data()[vs] = u;
      }
    }
    if ( status != algorithm_status::ok ) break;
  }
  return shortest_paths_result<typename G::index_type, D>{ status, source, micron::move(workspace.distance), micron::move(workspace.parent),
                                                           micron::move(workspace.reached) };
}

template<graph_model G>
[[nodiscard]] auto
dag_longest_paths(const G &graph, typename G::vertex_descriptor source)
{
  return dag_longest_paths(graph, source, intrinsic_edge_weight{});
}

template<micron::integral I, typename D> struct all_pairs_shortest_paths_result {
  algorithm_status status{ algorithm_status::ok };
  usize slots{};
  micron::vector<D, micron::allocator_serial<>, false> distance;
  micron::vector<u8, micron::allocator_serial<>, false> reached;

  [[nodiscard]] bool
  contains(vertex_id<I> u, vertex_id<I> v) const noexcept
  {
    return reached.data()[static_cast<usize>(u.value) * slots + static_cast<usize>(v.value)] != 0;
  }

  [[nodiscard]] const D *
  try_distance(vertex_id<I> u, vertex_id<I> v) const noexcept
  {
    return contains(u, v) ? distance.data() + static_cast<usize>(u.value) * slots + static_cast<usize>(v.value) : nullptr;
  }
};

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
floyd_warshall(const G &graph, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  const usize n = graph.vertex_slots();
  all_pairs_shortest_paths_result<typename G::index_type, D> result{ algorithm_status::ok, n,
                                                                     micron::vector<D, micron::allocator_serial<>, false>(n * n, D{}),
                                                                     micron::vector<u8, micron::allocator_serial<>, false>(n * n, u8(0)) };
  for ( auto vertex : graph.vertices() ) {
    const usize i = static_cast<usize>(vertex.value);
    result.reached.data()[i * n + i] = 1;
  }
  for ( auto edge : graph.edges() ) {
    const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge.id));
    if ( __impl::nonfinite_weight(weight) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    if ( !result.reached.data()[u * n + v] || weight < result.distance.data()[u * n + v] ) {
      result.reached.data()[u * n + v] = 1;
      result.distance.data()[u * n + v] = weight;
    }
    if constexpr ( !G::is_directed ) {
      result.reached.data()[v * n + u] = 1;
      result.distance.data()[v * n + u] = weight;
    }
  }
  for ( auto k_id : graph.vertices() ) {
    const usize k = static_cast<usize>(k_id.value);
    for ( auto i_id : graph.vertices() ) {
      const usize i = static_cast<usize>(i_id.value);
      if ( !result.reached.data()[i * n + k] ) continue;
      for ( auto j_id : graph.vertices() ) {
        const usize j = static_cast<usize>(j_id.value);
        if ( !result.reached.data()[k * n + j] ) continue;
        D candidate{};
        if ( !__impl::add_distance(result.distance.data()[i * n + k], result.distance.data()[k * n + j], candidate) ) {
          result.status = algorithm_status::overflow;
          return result;
        }
        if ( !result.reached.data()[i * n + j] || candidate < result.distance.data()[i * n + j] ) {
          result.reached.data()[i * n + j] = 1;
          result.distance.data()[i * n + j] = candidate;
        }
      }
    }
  }
  if constexpr ( micron::is_signed_v<D> || micron::is_floating_point_v<D> ) {
    for ( auto vertex : graph.vertices() ) {
      const usize i = static_cast<usize>(vertex.value);
      if ( result.distance.data()[i * n + i] < D(0) ) result.status = algorithm_status::negative_cycle;
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
floyd_warshall(const G &graph)
{
  return floyd_warshall(graph, intrinsic_edge_weight{});
}

template<micron::integral I, typename D> struct johnson_workspace {
  micron::vector<D, micron::allocator_serial<>, false> potential;
  shortest_path_workspace<I, D> shortest;

  void
  reserve(usize vertex_slots, usize edge_entries)
  {
    potential.reserve(vertex_slots);
    shortest.reserve(vertex_slots, edge_entries);
  }
};

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
johnson(const G &graph, WeightMap weight_map,
        johnson_workspace<typename G::index_type,
                          micron::remove_cvref_t<decltype(__impl::weight(micron::declval<WeightMap &>(), micron::declval<const G &>(),
                                                                         micron::declval<typename G::edge_descriptor>()))>> &storage)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using I = typename G::index_type;
  using vertex_descriptor = typename G::vertex_descriptor;
  const usize slots = graph.vertex_slots();
  all_pairs_shortest_paths_result<I, D> result{ algorithm_status::ok, slots,
                                                micron::vector<D, micron::allocator_serial<>, false>(slots * slots, D{}),
                                                micron::vector<u8, micron::allocator_serial<>, false>(slots * slots, u8(0)) };
  storage.reserve(slots, graph.edges_count() + 1);
  storage.potential.resize(slots, D{});
  storage.potential.fill(D{});
  auto &potential = storage.potential;

  for ( auto edge : graph.edges() )
    if ( __impl::nonfinite_weight(static_cast<D>(__impl::weight(weight_map, graph, edge.id))) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }

  auto relax_potential = [&](vertex_descriptor u, vertex_descriptor v, const D &weight) -> bool {
    D candidate{};
    if ( !__impl::add_distance(potential.data()[static_cast<usize>(u.value)], weight, candidate) ) {
      result.status = algorithm_status::overflow;
      return false;
    }
    D &target = potential.data()[static_cast<usize>(v.value)];
    if ( candidate < target ) {
      target = candidate;
      return true;
    }
    return false;
  };

  for ( usize pass = 1; pass < graph.vertices_count(); ++pass ) {
    bool changed = false;
    for ( auto edge : graph.edges() ) {
      const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge.id));
      changed |= relax_potential(edge.source, edge.target, weight);
      if constexpr ( !G::is_directed ) changed |= relax_potential(edge.target, edge.source, weight);
      if ( result.status != algorithm_status::ok ) return result;
    }
    if ( !changed ) break;
  }

  for ( auto edge : graph.edges() ) {
    const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge.id));
    auto can_relax = [&](vertex_descriptor u, vertex_descriptor v) {
      D candidate{};
      if ( !__impl::add_distance(potential.data()[static_cast<usize>(u.value)], weight, candidate) ) {
        result.status = algorithm_status::overflow;
        return false;
      }
      return candidate < potential.data()[static_cast<usize>(v.value)];
    };
    if ( can_relax(edge.source, edge.target) || (!G::is_directed && can_relax(edge.target, edge.source)) ) {
      if ( result.status == algorithm_status::ok ) result.status = algorithm_status::negative_cycle;
      return result;
    }
  }

  auto &workspace = storage.shortest;
  for ( auto source : graph.vertices() ) {
    workspace.reset(slots);
    const usize source_slot = static_cast<usize>(source.value);
    workspace.reached.data()[source_slot] = 1;
    workspace.parent.data()[source_slot] = source;
    workspace.queue.push_back(source);
    workspace.queue_distance.push_back(D{});

    auto push_heap = [&](vertex_descriptor vertex, const D &distance) {
      workspace.queue.push_back(vertex);
      workspace.queue_distance.push_back(distance);
      usize child = workspace.queue.size() - 1;
      while ( child ) {
        const usize parent = (child - 1) / 2;
        if ( !(workspace.queue_distance.data()[child] < workspace.queue_distance.data()[parent]) ) break;
        micron::swap(workspace.queue.data()[child], workspace.queue.data()[parent]);
        micron::swap(workspace.queue_distance.data()[child], workspace.queue_distance.data()[parent]);
        child = parent;
      }
    };
    auto pop_heap = [&]() {
      const vertex_descriptor selected = workspace.queue.data()[0];
      const D selected_distance = workspace.queue_distance.data()[0];
      const usize last = workspace.queue.size() - 1;
      workspace.queue.data()[0] = workspace.queue.data()[last];
      workspace.queue_distance.data()[0] = workspace.queue_distance.data()[last];
      workspace.queue.pop_back();
      workspace.queue_distance.pop_back();
      usize parent = 0;
      while ( parent < workspace.queue.size() ) {
        const usize left = parent * 2 + 1;
        if ( left >= workspace.queue.size() ) break;
        const usize right = left + 1;
        const usize child = right < workspace.queue.size() && workspace.queue_distance.data()[right] < workspace.queue_distance.data()[left]
                                ? right
                                : left;
        if ( !(workspace.queue_distance.data()[child] < workspace.queue_distance.data()[parent]) ) break;
        micron::swap(workspace.queue.data()[child], workspace.queue.data()[parent]);
        micron::swap(workspace.queue_distance.data()[child], workspace.queue_distance.data()[parent]);
        parent = child;
      }
      return micron::pair<vertex_descriptor, D>{ selected, selected_distance };
    };

    while ( !workspace.queue.empty() ) {
      const auto selected_pair = pop_heap();
      const vertex_descriptor u = selected_pair.a;
      const usize us = static_cast<usize>(u.value);
      if ( workspace.settled.data()[us] || selected_pair.b != workspace.distance.data()[us] ) continue;
      workspace.settled.data()[us] = 1;
      for ( auto edge : graph.out_edges(u) ) {
        const vertex_descriptor v = __impl::edge_neighbor(graph, edge, u);
        const usize vs = static_cast<usize>(v.value);
        const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge));
        D reduced{}, partial{};
        if ( !__impl::add_distance(weight, potential.data()[us], partial)
             || !__impl::sub_distance(partial, potential.data()[vs], reduced) ) {
          result.status = algorithm_status::overflow;
          return result;
        }
        D candidate{};
        if ( !__impl::add_distance(workspace.distance.data()[us], reduced, candidate) ) {
          result.status = algorithm_status::overflow;
          return result;
        }
        if ( !workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs] ) {
          workspace.reached.data()[vs] = 1;
          workspace.distance.data()[vs] = candidate;
          workspace.parent.data()[vs] = u;
          push_heap(v, candidate);
        }
      }
    }

    for ( auto target : graph.vertices() ) {
      const usize target_slot = static_cast<usize>(target.value);
      if ( !workspace.reached.data()[target_slot] ) continue;
      D partial{}, original{};
      if ( !__impl::sub_distance(workspace.distance.data()[target_slot], potential.data()[source_slot], partial)
           || !__impl::add_distance(partial, potential.data()[target_slot], original) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      result.reached.data()[source_slot * slots + target_slot] = 1;
      result.distance.data()[source_slot * slots + target_slot] = original;
    }
  }
  return result;
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
johnson(const G &graph, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  johnson_workspace<typename G::index_type, D> workspace;
  return johnson(graph, weight_map, workspace);
}

template<graph_model G>
[[nodiscard]] auto
johnson(const G &graph)
{
  return johnson(graph, intrinsic_edge_weight{});
}

};      // namespace micron::math::graphs
