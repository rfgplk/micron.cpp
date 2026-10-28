//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/svector.hpp"
#include "traversal.hpp"

namespace micron::math::graphs
{

template<micron::integral I, usize Capacity> struct fixed_bfs_workspace {
  static_assert(Capacity > 0);
  using vertex_descriptor = vertex_id<I>;

  micron::svector<vertex_descriptor, Capacity> frontier;
  micron::svector<vertex_descriptor, Capacity> order;
  micron::svector<vertex_descriptor, Capacity> parent;
  micron::svector<usize, Capacity> depth;
  micron::svector<u8, Capacity> discovered;

  [[nodiscard]] bool
  reset(usize slots)
  {
    if ( slots > Capacity ) return false;
    frontier.clear();
    order.clear();
    parent.clear();
    depth.clear();
    discovered.clear();
    for ( usize i = 0; i < slots; ++i ) {
      parent.push_back(vertex_descriptor::invalid());
      depth.push_back(0);
      discovered.push_back(0);
    }
    return true;
  }
};

struct bfs_limits {
  usize depth = ~usize{ 0 };
  usize vertices = ~usize{ 0 };
  usize edges = ~usize{ 0 };
};

struct bounded_bfs_result {
  algorithm_status status{ algorithm_status::ok };
  usize edges = 0;
  // False on a radius boundary, visitor stop or exhausted limit. A false value
  // does not prove unseen reachable vertices exist; it disclaims full traversal.
  bool complete = true;
};

// Allocation-free BFS for fixed graphs and ordinary graph views/CSR alike.
// Invalid source/capacity/zero-vertex-budget preflight preserves the workspace.
// After traversal starts, failures retain inspectable partial work; callers must
// check status before publishing it. Edge limits count adjacency visits, not
// hidden work inside a representation's iterators (fixed_graph scans slots).
template<graph_model G, usize Capacity, typename Visitor = null_visitor>
[[nodiscard]] bounded_bfs_result
bounded_bfs_into(const G &graph, typename G::vertex_descriptor source, fixed_bfs_workspace<typename G::index_type, Capacity> &workspace,
                 bfs_limits limits = {}, Visitor visitor = {})
{
  if ( !graph.has_vertex(source) ) return { algorithm_status::invalid_vertex, 0, false };
  if ( graph.vertex_slots() > Capacity || !limits.vertices ) return { algorithm_status::overflow, 0, false };
  if ( !workspace.reset(graph.vertex_slots()) ) return { algorithm_status::overflow, 0, false };
  workspace.frontier.push_back(source);
  workspace.discovered.data()[static_cast<usize>(source.value)] = 1;
  workspace.parent.data()[static_cast<usize>(source.value)] = source;
  bounded_bfs_result result;
  if ( !__impl::discover(visitor, graph, source) ) return { algorithm_status::ok, 0, false };

  usize head = 0;
  while ( head < workspace.frontier.size() ) {
    const auto u = workspace.frontier.data()[head++];
    workspace.order.push_back(u);
    const usize depth = workspace.depth.data()[static_cast<usize>(u.value)];
    if ( depth == limits.depth ) {
      result.complete = false;
      __impl::finish(visitor, graph, u);
      continue;
    }
    for ( auto edge : graph.out_edges(u) ) {
      if ( result.edges == limits.edges ) return { algorithm_status::overflow, result.edges, false };
      ++result.edges;
      if ( !__impl::examine_edge(visitor, graph, edge) ) return { algorithm_status::ok, result.edges, false };
      const auto v = G::is_directed ? graph.target(edge) : graph.opposite(edge, u);
      const usize slot = static_cast<usize>(v.value);
      if ( !graph.has_vertex(v) || slot >= workspace.discovered.size() ) return { algorithm_status::invalid_graph, result.edges, false };
      if ( workspace.discovered.data()[slot] ) continue;
      if ( workspace.frontier.size() == Capacity || workspace.frontier.size() == limits.vertices )
        return { algorithm_status::overflow, result.edges, false };
      workspace.discovered.data()[slot] = 1;
      workspace.parent.data()[slot] = u;
      workspace.depth.data()[slot] = depth + 1;
      workspace.frontier.push_back(v);
      if ( !__impl::discover(visitor, graph, v) ) return { algorithm_status::ok, result.edges, false };
    }
    __impl::finish(visitor, graph, u);
  }
  return result;
}

};      // namespace micron::math::graphs
