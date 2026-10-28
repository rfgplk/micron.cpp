//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "connectivity.hpp"

namespace micron::math::graphs
{

// Kosaraju with explicit DFS frames; depth does not consume the process stack
template<graph_model G>
[[nodiscard]] components_result<typename G::index_type>
strongly_connected_components_iterative(const G &graph)
{
  using I = typename G::index_type;
  using vertex = typename G::vertex_descriptor;
  if constexpr ( !G::is_directed ) return connected_components(graph);
  using iterator = decltype(graph.out_neighbors(vertex{}).begin());

  struct frame {
    vertex node;
    iterator next, end;
  };

  const usize slots = graph.vertex_slots();
  vector<u8> seen(slots, u8{ 0 });
  vector<vertex> finish, pending;
  vector<frame> frames;
  finish.reserve(graph.vertices_count());
  frames.reserve(graph.vertices_count());
  pending.reserve(graph.vertices_count());
  for ( const auto root : graph.vertices() ) {
    if ( seen[root.value] ) continue;
    seen[root.value] = 1;
    const auto neighbors = graph.out_neighbors(root);
    frames.push_back({ root, neighbors.begin(), neighbors.end() });
    while ( !frames.empty() ) {
      auto &top = frames.back();
      if ( top.next == top.end ) {
        finish.push_back(top.node);
        frames.pop_back();
        continue;
      }
      const auto child = *top.next;
      ++top.next;
      if ( seen[child.value] ) continue;
      seen[child.value] = 1;
      const auto children = graph.out_neighbors(child);
      frames.push_back({ child, children.begin(), children.end() });
    }
  }
  const I none = numeric_limits<I>::max();
  components_result<I> result{ algorithm_status::ok, 0, vector<I, allocator_serial<>, false>(slots, none) };
  for ( usize i = finish.size(); i; --i ) {
    const auto root = finish[i - 1];
    if ( result.component[root.value] != none ) continue;
    const I component = static_cast<I>(result.count++);
    result.component[root.value] = component;
    pending.push_back(root);
    while ( !pending.empty() ) {
      const auto node = pending.back();
      pending.pop_back();
      for ( const auto child : graph.in_neighbors(node) ) {
        if ( result.component[child.value] != none ) continue;
        result.component[child.value] = component;
        pending.push_back(child);
      }
    }
  }
  return result;
}

};      // namespace micron::math::graphs
