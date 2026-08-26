//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../vector/vector.hpp"
#include "graph.hpp"

namespace micron::math::graphs
{

struct null_visitor {
};

template<micron::integral I> struct traversal_workspace {
  using vertex_descriptor = vertex_id<I>;

  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> frontier;
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> from;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> incoming;
  micron::vector<u8, micron::allocator_serial<>, false> phase;
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> order;
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> parent;
  micron::vector<usize, micron::allocator_serial<>, false> depth;
  micron::vector<u8, micron::allocator_serial<>, false> discovered;

  void
  reserve(usize slots, usize stack_entries = 0)
  {
    const usize entries = stack_entries > slots ? stack_entries : slots;
    frontier.reserve(entries);
    if ( stack_entries ) {
      from.reserve(entries);
      incoming.reserve(entries);
      phase.reserve(entries);
    }
    order.reserve(slots);
    parent.reserve(slots);
    depth.reserve(slots);
    discovered.reserve(slots);
  }

  void
  reset(usize slots)
  {
    frontier.clear();
    from.clear();
    incoming.clear();
    phase.clear();
    order.clear();
    parent.resize(slots, vertex_descriptor::invalid());
    parent.fill(vertex_descriptor::invalid());
    depth.resize(slots, usize(0));
    depth.fill(usize(0));
    discovered.resize(slots, u8(0));
    discovered.fill(u8(0));
  }
};

template<micron::integral I> struct traversal_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> order;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> parent;
  micron::vector<usize, micron::allocator_serial<>, false> depth;
  micron::vector<u8, micron::allocator_serial<>, false> reached;

  [[nodiscard]] bool
  contains(vertex_id<I> vertex) const noexcept
  {
    const usize slot = static_cast<usize>(vertex.value);
    return vertex.valid() && slot < reached.size() && reached.data()[slot] != 0;
  }
};

namespace __impl
{

template<typename Visitor, typename G, typename V>
[[nodiscard]] inline bool
discover(Visitor &visitor, const G &graph, V vertex)
{
  if constexpr ( requires {
                   { visitor.discover_vertex(graph, vertex) } -> micron::convertible_to<bool>;
                 } )
    return static_cast<bool>(visitor.discover_vertex(graph, vertex));
  else if constexpr ( requires { visitor.discover_vertex(graph, vertex); } )
    visitor.discover_vertex(graph, vertex);
  return true;
}

template<typename Visitor, typename G, typename E>
[[nodiscard]] inline bool
examine_edge(Visitor &visitor, const G &graph, E edge)
{
  if constexpr ( requires {
                   { visitor.examine_edge(graph, edge) } -> micron::convertible_to<bool>;
                 } )
    return static_cast<bool>(visitor.examine_edge(graph, edge));
  else if constexpr ( requires { visitor.examine_edge(graph, edge); } )
    visitor.examine_edge(graph, edge);
  return true;
}

template<typename Visitor, typename G, typename V>
inline void
finish(Visitor &visitor, const G &graph, V vertex)
{
  if constexpr ( requires { visitor.finish_vertex(graph, vertex); } ) visitor.finish_vertex(graph, vertex);
}

};      // namespace __impl

template<graph_model G, typename Visitor = null_visitor>
algorithm_status
bfs_into(const G &graph, typename G::vertex_descriptor source, traversal_workspace<typename G::index_type> &workspace, Visitor visitor = {})
{
  using vertex_descriptor = typename G::vertex_descriptor;
  if ( !graph.has_vertex(source) ) return algorithm_status::invalid_vertex;

  workspace.reset(graph.vertex_slots());
  workspace.frontier.push_back(source);
  workspace.discovered.data()[static_cast<usize>(source.value)] = 1;
  workspace.parent.data()[static_cast<usize>(source.value)] = source;
  if ( !__impl::discover(visitor, graph, source) ) return algorithm_status::ok;

  usize head = 0;
  while ( head < workspace.frontier.size() ) {
    const vertex_descriptor u = workspace.frontier.data()[head++];
    workspace.order.push_back(u);
    for ( auto edge : graph.out_edges(u) ) {
      if ( !__impl::examine_edge(visitor, graph, edge) ) return algorithm_status::ok;
    }
    for ( auto v : graph.out_neighbors(u) ) {
      const usize slot = static_cast<usize>(v.value);
      if ( workspace.discovered.data()[slot] ) continue;
      workspace.discovered.data()[slot] = 1;
      workspace.parent.data()[slot] = u;
      workspace.depth.data()[slot] = workspace.depth.data()[static_cast<usize>(u.value)] + 1;
      workspace.frontier.push_back(v);
      if ( !__impl::discover(visitor, graph, v) ) return algorithm_status::ok;
    }
    __impl::finish(visitor, graph, u);
  }
  return algorithm_status::ok;
}

template<graph_model G, typename Visitor = null_visitor>
[[nodiscard]] traversal_result<typename G::index_type>
bfs(const G &graph, typename G::vertex_descriptor source, Visitor visitor = {})
{
  traversal_workspace<typename G::index_type> workspace;
  workspace.reserve(graph.vertex_slots());
  const algorithm_status status = bfs_into(graph, source, workspace, micron::move(visitor));
  return { status, micron::move(workspace.order), micron::move(workspace.parent), micron::move(workspace.depth),
           micron::move(workspace.discovered) };
}

template<graph_model G, micron::integral U, typename Visitor = null_visitor>
[[nodiscard]] traversal_result<typename G::index_type>
bfs(const G &graph, U source, Visitor visitor = {})
{
  return bfs(graph, typename G::vertex_descriptor(static_cast<typename G::index_type>(source)), micron::move(visitor));
}

template<graph_model G, typename Visitor = null_visitor>
algorithm_status
dfs_into(const G &graph, typename G::vertex_descriptor source, traversal_workspace<typename G::index_type> &workspace, Visitor visitor = {})
{
  using vertex_descriptor = typename G::vertex_descriptor;
  if ( !graph.has_vertex(source) ) return algorithm_status::invalid_vertex;
  workspace.reset(graph.vertex_slots());

  workspace.frontier.push_back(source);
  workspace.from.push_back(source);
  workspace.incoming.push_back(edge_id<typename G::index_type>::invalid());
  workspace.phase.push_back(0);

  while ( !workspace.frontier.empty() ) {
    const usize top = workspace.frontier.size() - 1;
    const vertex_descriptor u = workspace.frontier.data()[top];
    const vertex_descriptor from = workspace.from.data()[top];
    const auto incoming = workspace.incoming.data()[top];
    const u8 phase = workspace.phase.data()[top];
    workspace.frontier.pop_back();
    workspace.from.pop_back();
    workspace.incoming.pop_back();
    workspace.phase.pop_back();

    if ( phase ) {
      __impl::finish(visitor, graph, u);
      continue;
    }

    if ( incoming.valid() && !__impl::examine_edge(visitor, graph, incoming) ) return algorithm_status::ok;
    const usize slot = static_cast<usize>(u.value);
    if ( workspace.discovered.data()[slot] ) continue;
    workspace.discovered.data()[slot] = 1;
    workspace.parent.data()[slot] = from;
    if ( u != source ) workspace.depth.data()[slot] = workspace.depth.data()[static_cast<usize>(from.value)] + 1;
    if ( !__impl::discover(visitor, graph, u) ) return algorithm_status::ok;
    workspace.order.push_back(u);

    workspace.frontier.push_back(u);
    workspace.from.push_back(from);
    workspace.incoming.push_back(edge_id<typename G::index_type>::invalid());
    workspace.phase.push_back(1);
    for ( auto edge : graph.out_edges(u) ) {
      const vertex_descriptor v = G::is_directed ? graph.target(edge) : graph.opposite(edge, u);
      workspace.frontier.push_back(v);
      workspace.from.push_back(u);
      workspace.incoming.push_back(edge);
      workspace.phase.push_back(0);
    }
  }
  return algorithm_status::ok;
}

template<graph_model G, typename Visitor = null_visitor>
[[nodiscard]] traversal_result<typename G::index_type>
dfs(const G &graph, typename G::vertex_descriptor source, Visitor visitor = {})
{
  traversal_workspace<typename G::index_type> workspace;
  workspace.reserve(graph.vertex_slots(), graph.edges_count() * (G::is_directed ? 1 : 2) + 1);
  const algorithm_status status = dfs_into(graph, source, workspace, micron::move(visitor));
  return { status, micron::move(workspace.order), micron::move(workspace.parent), micron::move(workspace.depth),
           micron::move(workspace.discovered) };
}

template<graph_model G, micron::integral U, typename Visitor = null_visitor>
[[nodiscard]] traversal_result<typename G::index_type>
dfs(const G &graph, U source, Visitor visitor = {})
{
  return dfs(graph, typename G::vertex_descriptor(static_cast<typename G::index_type>(source)), micron::move(visitor));
}

template<graph_model G>
[[nodiscard]] bool
reachable(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target)
{
  if ( !graph.has_vertex(source) || !graph.has_vertex(target) ) return false;
  if ( source == target ) return true;
  auto result = bfs(graph, source);
  return result.contains(target);
}

template<graph_model G, micron::integral U, micron::integral V>
[[nodiscard]] bool
reachable(const G &graph, U source, V target)
{
  using I = typename G::index_type;
  return reachable(graph, typename G::vertex_descriptor(static_cast<I>(source)), typename G::vertex_descriptor(static_cast<I>(target)));
}

template<graph_model G>
[[nodiscard]] auto
diffusion(const G &graph, typename G::vertex_descriptor source, usize rounds)
{
  auto result = bfs(graph, source);
  usize keep = 0;
  while ( keep < result.order.size() && result.depth.data()[static_cast<usize>(result.order.data()[keep].value)] <= rounds ) ++keep;
  result.order.resize(keep);
  return result;
}

template<graph_model G, typename Rng>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
random_walk(const G &graph, typename G::vertex_descriptor source, usize steps, Rng &&rng)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> walk;
  if ( !graph.has_vertex(source) ) return walk;
  walk.reserve(steps + 1);
  walk.push_back(source);
  vertex_descriptor current = source;
  for ( usize step = 0; step < steps; ++step ) {
    const usize degree = graph.out_degree(current);
    if ( degree == 0 ) break;
    const usize selected = static_cast<usize>(micron::invoke(rng)) % degree;
    usize at = 0;
    vertex_descriptor next = current;
    for ( auto neighbor : graph.out_neighbors(current) ) {
      if ( at++ == selected ) {
        next = neighbor;
        break;
      }
    }
    walk.push_back(next);
    current = next;
  }
  return walk;
}

};      // namespace micron::math::graphs
