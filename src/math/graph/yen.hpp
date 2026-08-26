//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "paths.hpp"

namespace micron::math::graphs
{

template<micron::integral I, typename D> struct weighted_path {
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> vertices;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
  D cost{};
};

template<micron::integral I, typename D> struct yen_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<weighted_path<I, D>, micron::allocator_serial<>, false> paths;
};

template<micron::integral I, typename D> struct yen_workspace {
  micron::vector<D, micron::allocator_serial<>, false> potential;
  micron::vector<D, micron::allocator_serial<>, false> distance;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> parent;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> parent_edge;
  micron::vector<u8, micron::allocator_serial<>, false> reached;
  micron::vector<u8, micron::allocator_serial<>, false> settled;
  micron::vector<u8, micron::allocator_serial<>, false> excluded_vertex;
  micron::vector<u8, micron::allocator_serial<>, false> excluded_edge;
  micron::vector<u8, micron::allocator_serial<>, false> path_vertex;
  micron::vector<u8, micron::allocator_serial<>, false> reach_seen;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> reach_stack;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> heap_vertex;
  micron::vector<D, micron::allocator_serial<>, false> heap_distance;
  micron::vector<weighted_path<I, D>, micron::allocator_serial<>, false> candidates;

  void
  reserve(usize vertex_slots, usize edge_slots)
  {
    potential.reserve(vertex_slots);
    distance.reserve(vertex_slots);
    parent.reserve(vertex_slots);
    parent_edge.reserve(vertex_slots);
    reached.reserve(vertex_slots);
    settled.reserve(vertex_slots);
    excluded_vertex.reserve(vertex_slots);
    excluded_edge.reserve(edge_slots);
    path_vertex.reserve(vertex_slots);
    reach_seen.reserve(vertex_slots);
    reach_stack.reserve(vertex_slots);
    heap_vertex.reserve(edge_slots + 1);
    heap_distance.reserve(edge_slots + 1);
  }
};

namespace __impl
{

template<graph_model G, typename WeightMap, typename D>
algorithm_status
yen_potentials(const G &graph, WeightMap &weight_map, micron::vector<D, micron::allocator_serial<>, false> &potential)
{
  potential.resize(graph.vertex_slots(), D{});
  potential.fill(D{});
  for ( auto edge : graph.edges() )
    if ( __impl::nonfinite_weight(static_cast<D>(__impl::weight(weight_map, graph, edge.id))) ) return algorithm_status::invalid_weight;

  auto relax = [&](typename G::vertex_descriptor u, typename G::vertex_descriptor v, const D &weight) {
    D candidate{};
    if ( !__impl::add_distance(potential.data()[static_cast<usize>(u.value)], weight, candidate) ) return -1;
    D &target = potential.data()[static_cast<usize>(v.value)];
    if ( candidate < target ) {
      target = candidate;
      return 1;
    }
    return 0;
  };
  for ( usize pass = 1; pass < graph.vertices_count(); ++pass ) {
    bool changed = false;
    for ( auto edge : graph.edges() ) {
      const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge.id));
      const int forward = relax(edge.source, edge.target, weight);
      if ( forward < 0 ) return algorithm_status::overflow;
      changed |= forward != 0;
      if constexpr ( !G::is_directed ) {
        const int reverse = relax(edge.target, edge.source, weight);
        if ( reverse < 0 ) return algorithm_status::overflow;
        changed |= reverse != 0;
      }
    }
    if ( !changed ) break;
  }
  for ( auto edge : graph.edges() ) {
    const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge.id));
    const int forward = relax(edge.source, edge.target, weight);
    if ( forward < 0 ) return algorithm_status::overflow;
    if ( forward > 0 ) return algorithm_status::negative_cycle;
    if constexpr ( !G::is_directed ) {
      const int reverse = relax(edge.target, edge.source, weight);
      if ( reverse < 0 ) return algorithm_status::overflow;
      if ( reverse > 0 ) return algorithm_status::negative_cycle;
    }
  }
  return algorithm_status::ok;
}

template<typename I, typename D>
[[nodiscard]] bool
same_weighted_path(const weighted_path<I, D> &a, const weighted_path<I, D> &b) noexcept
{
  if ( a.edges.size() != b.edges.size() || a.vertices.size() != b.vertices.size() ) return false;
  for ( usize i = 0; i < a.edges.size(); ++i )
    if ( a.edges.data()[i] != b.edges.data()[i] ) return false;
  for ( usize i = 0; i < a.vertices.size(); ++i )
    if ( a.vertices.data()[i] != b.vertices.data()[i] ) return false;
  return true;
}

template<typename I, typename D>
[[nodiscard]] bool
weighted_path_less(const weighted_path<I, D> &a, const weighted_path<I, D> &b) noexcept
{
  if ( a.cost < b.cost ) return true;
  if ( b.cost < a.cost ) return false;
  const usize vertices = a.vertices.size() < b.vertices.size() ? a.vertices.size() : b.vertices.size();
  for ( usize i = 0; i < vertices; ++i ) {
    if ( a.vertices.data()[i] < b.vertices.data()[i] ) return true;
    if ( b.vertices.data()[i] < a.vertices.data()[i] ) return false;
  }
  if ( a.vertices.size() != b.vertices.size() ) return a.vertices.size() < b.vertices.size();
  for ( usize i = 0; i < a.edges.size(); ++i ) {
    if ( a.edges.data()[i] < b.edges.data()[i] ) return true;
    if ( b.edges.data()[i] < a.edges.data()[i] ) return false;
  }
  return false;
}

};      // namespace __impl

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
yen(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target, usize count,
    yen_workspace<typename G::index_type,
                  micron::remove_cvref_t<decltype(__impl::weight(micron::declval<WeightMap &>(), micron::declval<const G &>(),
                                                                 micron::declval<typename G::edge_descriptor>()))>> &workspace,
    WeightMap weight_map)
{
  using I = typename G::index_type;
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  yen_result<I, D> result;
  if ( count == 0 ) return result;
  if ( !graph.has_vertex(source) || !graph.has_vertex(target) ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }
  workspace.reserve(graph.vertex_slots(), graph.edge_slots());
  result.status = __impl::yen_potentials(graph, weight_map, workspace.potential);
  if ( result.status != algorithm_status::ok ) return result;
  workspace.candidates.clear();
  workspace.excluded_vertex.resize(graph.vertex_slots(), u8(0));
  workspace.excluded_edge.resize(graph.edge_slots(), u8(0));

  auto shortest_filtered = [&](vertex_descriptor start, vertex_descriptor finish, weighted_path<I, D> &path) {
    const usize slots = graph.vertex_slots();
    workspace.distance.resize(slots, D{});
    workspace.distance.fill(D{});
    workspace.parent.resize(slots, vertex_descriptor::invalid());
    workspace.parent.fill(vertex_descriptor::invalid());
    workspace.parent_edge.resize(slots, edge_descriptor::invalid());
    workspace.parent_edge.fill(edge_descriptor::invalid());
    workspace.reached.resize(slots, u8(0));
    workspace.reached.fill(u8(0));
    workspace.settled.resize(slots, u8(0));
    workspace.settled.fill(u8(0));
    workspace.heap_vertex.clear();
    workspace.heap_distance.clear();
    if ( workspace.excluded_vertex.data()[static_cast<usize>(start.value)]
         || workspace.excluded_vertex.data()[static_cast<usize>(finish.value)] )
      return algorithm_status::unreachable;
    workspace.reached.data()[static_cast<usize>(start.value)] = 1;
    workspace.parent.data()[static_cast<usize>(start.value)] = start;

    auto less = [](const D &ad, vertex_descriptor av, const D &bd, vertex_descriptor bv) { return ad < bd || (!(bd < ad) && av < bv); };
    auto push = [&](vertex_descriptor vertex, const D &distance) {
      workspace.heap_vertex.push_back(vertex);
      workspace.heap_distance.push_back(distance);
      usize child = workspace.heap_vertex.size() - 1;
      while ( child ) {
        const usize parent = (child - 1) / 2;
        if ( !less(workspace.heap_distance.data()[child], workspace.heap_vertex.data()[child], workspace.heap_distance.data()[parent],
                   workspace.heap_vertex.data()[parent]) )
          break;
        micron::swap(workspace.heap_vertex.data()[child], workspace.heap_vertex.data()[parent]);
        micron::swap(workspace.heap_distance.data()[child], workspace.heap_distance.data()[parent]);
        child = parent;
      }
    };
    push(start, D{});
    while ( !workspace.heap_vertex.empty() ) {
      const vertex_descriptor selected = workspace.heap_vertex.data()[0];
      const D selected_distance = workspace.heap_distance.data()[0];
      const usize last = workspace.heap_vertex.size() - 1;
      workspace.heap_vertex.data()[0] = workspace.heap_vertex.data()[last];
      workspace.heap_distance.data()[0] = workspace.heap_distance.data()[last];
      workspace.heap_vertex.pop_back();
      workspace.heap_distance.pop_back();
      usize parent = 0;
      while ( parent < workspace.heap_vertex.size() ) {
        const usize left = parent * 2 + 1;
        if ( left >= workspace.heap_vertex.size() ) break;
        const usize right = left + 1;
        usize child = left;
        if ( right < workspace.heap_vertex.size()
             && less(workspace.heap_distance.data()[right], workspace.heap_vertex.data()[right], workspace.heap_distance.data()[left],
                     workspace.heap_vertex.data()[left]) )
          child = right;
        if ( !less(workspace.heap_distance.data()[child], workspace.heap_vertex.data()[child], workspace.heap_distance.data()[parent],
                   workspace.heap_vertex.data()[parent]) )
          break;
        micron::swap(workspace.heap_vertex.data()[child], workspace.heap_vertex.data()[parent]);
        micron::swap(workspace.heap_distance.data()[child], workspace.heap_distance.data()[parent]);
        parent = child;
      }
      const usize us = static_cast<usize>(selected.value);
      if ( workspace.settled.data()[us] || selected_distance != workspace.distance.data()[us] ) continue;
      workspace.settled.data()[us] = 1;
      if ( selected == finish ) break;
      for ( auto edge : graph.out_edges(selected) ) {
        if ( workspace.excluded_edge.data()[static_cast<usize>(edge.value)] ) continue;
        const vertex_descriptor neighbor = __impl::edge_neighbor(graph, edge, selected);
        const usize vs = static_cast<usize>(neighbor.value);
        if ( workspace.excluded_vertex.data()[vs] || workspace.settled.data()[vs] ) continue;
        const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge));
        D partial{}, reduced{}, candidate{};
        if ( !__impl::add_distance(weight, workspace.potential.data()[us], partial)
             || !__impl::sub_distance(partial, workspace.potential.data()[vs], reduced)
             || !__impl::add_distance(workspace.distance.data()[us], reduced, candidate) )
          return algorithm_status::overflow;
        const bool equal
            = workspace.reached.data()[vs] && !(candidate < workspace.distance.data()[vs]) && !(workspace.distance.data()[vs] < candidate);
        if ( !workspace.reached.data()[vs] || candidate < workspace.distance.data()[vs]
             || (equal
                 && (selected < workspace.parent.data()[vs]
                     || (selected == workspace.parent.data()[vs] && edge < workspace.parent_edge.data()[vs]))) ) {
          workspace.reached.data()[vs] = 1;
          workspace.distance.data()[vs] = candidate;
          workspace.parent.data()[vs] = selected;
          workspace.parent_edge.data()[vs] = edge;
          push(neighbor, candidate);
        }
      }
    }
    if ( !workspace.reached.data()[static_cast<usize>(finish.value)] ) return algorithm_status::unreachable;
    path.vertices.clear();
    path.edges.clear();
    workspace.path_vertex.resize(slots, u8(0));
    workspace.path_vertex.fill(u8(0));
    workspace.reach_seen.resize(slots, u8(0));
    vertex_descriptor current = start;
    path.vertices.push_back(current);
    workspace.path_vertex.data()[static_cast<usize>(current.value)] = 1;
    bool path_overflow = false;

    auto tight = [&](vertex_descriptor u, edge_descriptor edge, vertex_descriptor v) {
      const usize us = static_cast<usize>(u.value);
      const usize vs = static_cast<usize>(v.value);
      const D weight = static_cast<D>(__impl::weight(weight_map, graph, edge));
      D partial{}, reduced{}, candidate{};
      if ( !__impl::add_distance(weight, workspace.potential.data()[us], partial)
           || !__impl::sub_distance(partial, workspace.potential.data()[vs], reduced)
           || !__impl::add_distance(workspace.distance.data()[us], reduced, candidate) ) {
        path_overflow = true;
        return false;
      }
      return workspace.reached.data()[us] && workspace.reached.data()[vs] && !(candidate < workspace.distance.data()[vs])
             && !(workspace.distance.data()[vs] < candidate);
    };

    auto reaches_finish = [&](vertex_descriptor first) {
      workspace.reach_seen.fill(u8(0));
      workspace.reach_stack.clear();
      workspace.reach_stack.push_back(first);
      workspace.reach_seen.data()[static_cast<usize>(first.value)] = 1;
      while ( !workspace.reach_stack.empty() ) {
        const vertex_descriptor u = workspace.reach_stack.data()[workspace.reach_stack.size() - 1];
        workspace.reach_stack.pop_back();
        if ( u == finish ) return true;
        for ( auto edge : graph.out_edges(u) ) {
          if ( workspace.excluded_edge.data()[static_cast<usize>(edge.value)] ) continue;
          const vertex_descriptor v = __impl::edge_neighbor(graph, edge, u);
          const usize vs = static_cast<usize>(v.value);
          if ( workspace.excluded_vertex.data()[vs] || workspace.path_vertex.data()[vs] || workspace.reach_seen.data()[vs]
               || !tight(u, edge, v) )
            continue;
          workspace.reach_seen.data()[vs] = 1;
          workspace.reach_stack.push_back(v);
        }
      }
      return false;
    };

    for ( usize guard = 0; current != finish && guard < graph.vertices_count(); ++guard ) {
      vertex_descriptor selected_vertex = vertex_descriptor::invalid();
      edge_descriptor selected_edge = edge_descriptor::invalid();
      for ( auto edge : graph.out_edges(current) ) {
        if ( workspace.excluded_edge.data()[static_cast<usize>(edge.value)] ) continue;
        const vertex_descriptor neighbor = __impl::edge_neighbor(graph, edge, current);
        const usize slot = static_cast<usize>(neighbor.value);
        if ( workspace.excluded_vertex.data()[slot] || workspace.path_vertex.data()[slot] || !tight(current, edge, neighbor) ) continue;
        if ( selected_vertex.valid() && (selected_vertex < neighbor || (selected_vertex == neighbor && selected_edge < edge)) ) continue;
        if ( reaches_finish(neighbor) ) {
          selected_vertex = neighbor;
          selected_edge = edge;
        }
      }
      if ( path_overflow ) return algorithm_status::overflow;
      if ( !selected_vertex.valid() ) return algorithm_status::invalid_graph;
      path.edges.push_back(selected_edge);
      path.vertices.push_back(selected_vertex);
      workspace.path_vertex.data()[static_cast<usize>(selected_vertex.value)] = 1;
      current = selected_vertex;
    }
    if ( current != finish ) return algorithm_status::invalid_graph;
    D partial{};
    if ( !__impl::sub_distance(workspace.distance.data()[static_cast<usize>(finish.value)],
                               workspace.potential.data()[static_cast<usize>(start.value)], partial)
         || !__impl::add_distance(partial, workspace.potential.data()[static_cast<usize>(finish.value)], path.cost) )
      return algorithm_status::overflow;
    return algorithm_status::ok;
  };

  workspace.excluded_vertex.fill(u8(0));
  workspace.excluded_edge.fill(u8(0));
  weighted_path<I, D> first;
  result.status = shortest_filtered(source, target, first);
  if ( result.status != algorithm_status::ok ) return result;
  result.paths.push_back(micron::move(first));

  while ( result.paths.size() < count ) {
    const weighted_path<I, D> &previous = result.paths.data()[result.paths.size() - 1];
    D root_cost{};
    for ( usize spur_index = 0; spur_index < previous.edges.size(); ++spur_index ) {
      workspace.excluded_vertex.fill(u8(0));
      workspace.excluded_edge.fill(u8(0));
      for ( usize i = 0; i < spur_index; ++i ) workspace.excluded_vertex.data()[static_cast<usize>(previous.vertices.data()[i].value)] = 1;
      for ( const auto &accepted : result.paths ) {
        if ( accepted.edges.size() <= spur_index ) continue;
        bool same_root = accepted.vertices.data()[spur_index] == previous.vertices.data()[spur_index];
        for ( usize i = 0; same_root && i < spur_index; ++i )
          same_root = accepted.edges.data()[i] == previous.edges.data()[i] && accepted.vertices.data()[i] == previous.vertices.data()[i];
        if ( same_root ) workspace.excluded_edge.data()[static_cast<usize>(accepted.edges.data()[spur_index].value)] = 1;
      }

      weighted_path<I, D> spur;
      const algorithm_status spur_status = shortest_filtered(previous.vertices.data()[spur_index], target, spur);
      if ( spur_status == algorithm_status::overflow ) {
        result.status = spur_status;
        return result;
      }
      if ( spur_status == algorithm_status::ok ) {
        weighted_path<I, D> candidate;
        candidate.vertices.reserve(spur_index + spur.vertices.size());
        candidate.edges.reserve(spur_index + spur.edges.size());
        for ( usize i = 0; i < spur_index; ++i ) {
          candidate.vertices.push_back(previous.vertices.data()[i]);
          candidate.edges.push_back(previous.edges.data()[i]);
        }
        for ( auto vertex : spur.vertices ) candidate.vertices.push_back(vertex);
        for ( auto edge : spur.edges ) candidate.edges.push_back(edge);
        if ( !__impl::add_distance(root_cost, spur.cost, candidate.cost) ) {
          result.status = algorithm_status::overflow;
          return result;
        }
        bool duplicate = false;
        for ( const auto &accepted : result.paths ) duplicate |= __impl::same_weighted_path(accepted, candidate);
        for ( const auto &queued : workspace.candidates ) duplicate |= __impl::same_weighted_path(queued, candidate);
        if ( !duplicate ) workspace.candidates.push_back(micron::move(candidate));
      }
      D next_cost{};
      const D edge_weight = static_cast<D>(__impl::weight(weight_map, graph, previous.edges.data()[spur_index]));
      if ( !__impl::add_distance(root_cost, edge_weight, next_cost) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      root_cost = next_cost;
    }
    if ( workspace.candidates.empty() ) break;
    usize best = 0;
    for ( usize i = 1; i < workspace.candidates.size(); ++i )
      if ( __impl::weighted_path_less(workspace.candidates.data()[i], workspace.candidates.data()[best]) ) best = i;
    result.paths.push_back(micron::move(workspace.candidates.data()[best]));
    workspace.candidates.erase(best);
  }
  result.status = algorithm_status::ok;
  return result;
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
yen(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target, usize count, WeightMap weight_map)
{
  using D = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  yen_workspace<typename G::index_type, D> workspace;
  return yen(graph, source, target, count, workspace, weight_map);
}

template<graph_model G>
[[nodiscard]] auto
yen(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target, usize count)
{
  return yen(graph, source, target, count, intrinsic_edge_weight{});
}

};      // namespace micron::math::graphs
