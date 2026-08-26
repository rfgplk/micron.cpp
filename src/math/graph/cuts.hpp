//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "flow.hpp"

namespace micron::math::graphs
{

template<micron::integral I, typename Capacity> struct gomory_hu_workspace {
  micron::vector<Capacity, micron::allocator_serial<>, false> base;
  micron::vector<Capacity, micron::allocator_serial<>, false> residual;
  micron::vector<max_t, micron::allocator_serial<>, false> level;
  micron::vector<usize, micron::allocator_serial<>, false> next;
  micron::vector<usize, micron::allocator_serial<>, false> queue;
  micron::vector<u8, micron::allocator_serial<>, false> side;

  void
  reserve(usize vertices)
  {
    if ( vertices == 0 || vertices <= micron::numeric_limits<usize>::max() / vertices ) {
      base.reserve(vertices * vertices);
      residual.reserve(vertices * vertices);
    }
    level.reserve(vertices);
    next.reserve(vertices);
    queue.reserve(vertices);
    side.reserve(vertices);
  }
};

template<micron::integral I, typename Capacity> struct gomory_hu_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> parent;
  micron::vector<Capacity, micron::allocator_serial<>, false> cut;
  weighted_graph<Capacity, empty_property, empty_property, empty_property, I> tree;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
  micron::vector<I, micron::allocator_serial<>, false> vertex_to_dense;
  micron::vector<micron::vector<u64, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> cut_side;

  [[nodiscard]] bool
  side_contains(usize tree_child, usize dense_vertex) const noexcept
  {
    return tree_child < cut_side.size() && dense_vertex / 64 < cut_side.data()[tree_child].size()
           && (cut_side.data()[tree_child].data()[dense_vertex / 64] & (u64(1) << (dense_vertex & 63))) != 0;
  }
};

template<graph_model G, typename CapacityMap>
[[nodiscard]] auto
gomory_hu(const G &graph, CapacityMap capacity_map,
          gomory_hu_workspace<typename G::index_type,
                              micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CapacityMap &>(), micron::declval<const G &>(),
                                                                             micron::declval<typename G::edge_descriptor>()))>> &workspace)
{
  using I = typename G::index_type;
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  gomory_hu_result<I, C> result;
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  auto mapping = __dense_vertex_mapping(graph);
  result.dense_to_vertex = micron::move(mapping.dense_to_vertex);
  result.vertex_to_dense = micron::move(mapping.vertex_to_dense);
  const usize n = result.dense_to_vertex.size();
  if ( n != 0 && n > micron::numeric_limits<usize>::max() / n ) {
    result.status = algorithm_status::overflow;
    return result;
  }
  workspace.reserve(n);
  workspace.base.resize(n * n, C{});
  workspace.base.fill(C{});
  for ( auto edge : graph.edges() ) {
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    if ( __impl::invalid_weight(capacity) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    const usize u = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    if ( u == v ) continue;
    C sum{};
    if ( !__impl::add_distance(workspace.base.data()[u * n + v], capacity, sum) ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    workspace.base.data()[u * n + v] = sum;
    if ( !__impl::add_distance(workspace.base.data()[v * n + u], capacity, sum) ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    workspace.base.data()[v * n + u] = sum;
  }

  result.parent.resize(n, vertex_id<I>::invalid());
  result.cut.resize(n, C{});
  for ( usize vertex = 1; vertex < n; ++vertex ) result.parent.data()[vertex] = vertex_id<I>(I(0));
  workspace.level.resize(n, max_t(-1));
  workspace.next.resize(n, usize(0));
  workspace.side.resize(n, u8(0));

  auto max_flow = [&](usize source, usize sink, C &value) {
    workspace.residual = workspace.base;
    value = C{};
    auto build_levels = [&]() {
      workspace.level.fill(max_t(-1));
      workspace.queue.clear();
      workspace.queue.push_back(source);
      workspace.level.data()[source] = 0;
      usize head = 0;
      while ( head < workspace.queue.size() ) {
        const usize u = workspace.queue.data()[head++];
        for ( usize v = 0; v < n; ++v )
          if ( workspace.residual.data()[u * n + v] > C{} && workspace.level.data()[v] < 0 ) {
            workspace.level.data()[v] = workspace.level.data()[u] + 1;
            workspace.queue.push_back(v);
          }
      }
      return workspace.level.data()[sink] >= 0;
    };
    bool overflow = false;
    auto send = [&](auto &&self, usize u, C offered) -> C {
      if ( u == sink || offered == C{} ) return offered;
      for ( usize &v = workspace.next.data()[u]; v < n; ++v ) {
        C &capacity = workspace.residual.data()[u * n + v];
        if ( capacity <= C{} || workspace.level.data()[v] != workspace.level.data()[u] + 1 ) continue;
        const C pushed = self(self, v, offered < capacity ? offered : capacity);
        if ( pushed == C{} ) continue;
        C reverse{};
        if ( !__impl::add_distance(workspace.residual.data()[v * n + u], pushed, reverse) ) {
          overflow = true;
          return C{};
        }
        capacity -= pushed;
        workspace.residual.data()[v * n + u] = reverse;
        return pushed;
      }
      return C{};
    };
    C bound{};
    for ( usize v = 0; v < n; ++v ) {
      C sum{};
      if ( !__impl::add_distance(bound, workspace.base.data()[source * n + v], sum) ) return algorithm_status::overflow;
      bound = sum;
    }
    while ( build_levels() ) {
      workspace.next.fill(usize(0));
      for ( ;; ) {
        const C pushed = send(send, source, bound);
        if ( overflow ) return algorithm_status::overflow;
        if ( pushed == C{} ) break;
        C total{};
        if ( !__impl::add_distance(value, pushed, total) ) return algorithm_status::overflow;
        value = total;
      }
    }
    workspace.side.fill(u8(0));
    workspace.queue.clear();
    workspace.queue.push_back(source);
    workspace.side.data()[source] = 1;
    for ( usize head = 0; head < workspace.queue.size(); ++head ) {
      const usize u = workspace.queue.data()[head];
      for ( usize v = 0; v < n; ++v )
        if ( workspace.residual.data()[u * n + v] > C{} && !workspace.side.data()[v] ) {
          workspace.side.data()[v] = 1;
          workspace.queue.push_back(v);
        }
    }
    return algorithm_status::ok;
  };

  for ( usize source = 1; source < n; ++source ) {
    const usize sink = static_cast<usize>(result.parent.data()[source].value);
    C value{};
    result.status = max_flow(source, sink, value);
    if ( result.status != algorithm_status::ok ) return result;
    for ( usize vertex = source + 1; vertex < n; ++vertex )
      if ( result.parent.data()[vertex].value == static_cast<I>(sink) && workspace.side.data()[vertex] )
        result.parent.data()[vertex] = vertex_id<I>(static_cast<I>(source));
    if ( sink != 0 ) {
      const usize sink_parent = static_cast<usize>(result.parent.data()[sink].value);
      if ( workspace.side.data()[sink_parent] ) {
        result.parent.data()[source] = result.parent.data()[sink];
        result.parent.data()[sink] = vertex_id<I>(static_cast<I>(source));
        result.cut.data()[source] = result.cut.data()[sink];
        result.cut.data()[sink] = value;
        continue;
      }
    }
    result.cut.data()[source] = value;
  }

  (void)result.tree.add_vertices(n);
  for ( usize child = 1; child < n; ++child )
    (void)result.tree.add_edge(static_cast<I>(child), result.parent.data()[child].value, result.cut.data()[child]);

  const usize words = (n + 63) / 64;
  result.cut_side.resize(n);
  for ( usize child = 1; child < n; ++child ) {
    auto &bits = result.cut_side.data()[child];
    bits.resize(words, u64(0));
    workspace.side.fill(u8(0));
    workspace.queue.clear();
    workspace.queue.push_back(child);
    workspace.side.data()[child] = 1;
    for ( usize head = 0; head < workspace.queue.size(); ++head ) {
      const usize u = workspace.queue.data()[head];
      bits.data()[u / 64] |= u64(1) << (u & 63);
      if ( u != 0 ) {
        const usize p = static_cast<usize>(result.parent.data()[u].value);
        if ( !((u == child && p == static_cast<usize>(result.parent.data()[child].value))
               || (p == child && u == static_cast<usize>(result.parent.data()[child].value)))
             && !workspace.side.data()[p] ) {
          workspace.side.data()[p] = 1;
          workspace.queue.push_back(p);
        }
      }
      for ( usize v = 1; v < n; ++v ) {
        const usize p = static_cast<usize>(result.parent.data()[v].value);
        if ( p != u || (v == child && u == static_cast<usize>(result.parent.data()[child].value)) || workspace.side.data()[v] ) continue;
        workspace.side.data()[v] = 1;
        workspace.queue.push_back(v);
      }
    }
  }
  return result;
}

template<graph_model G, typename CapacityMap>
[[nodiscard]] auto
gomory_hu(const G &graph, CapacityMap capacity_map)
{
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  gomory_hu_workspace<typename G::index_type, C> workspace;
  return gomory_hu(graph, capacity_map, workspace);
}

template<graph_model G>
[[nodiscard]] auto
gomory_hu(const G &graph)
{
  return gomory_hu(graph, intrinsic_edge_weight{});
}

};      // namespace micron::math::graphs
