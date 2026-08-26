//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../numerics.hpp"
#include "../../vector/vector.hpp"
#include "graph.hpp"
#include "traversal.hpp"

namespace micron::math::graphs
{

template<micron::integral I> struct components_result {
  algorithm_status status{ algorithm_status::ok };
  usize count{};
  micron::vector<I, micron::allocator_serial<>, false> component;

  [[nodiscard]] I
  operator[](vertex_id<I> vertex) const noexcept
  {
    return component.data()[static_cast<usize>(vertex.value)];
  }
};

template<graph_model G>
[[nodiscard]] components_result<typename G::index_type>
connected_components(const G &graph)
{
  using I = typename G::index_type;
  using vertex_descriptor = typename G::vertex_descriptor;
  const I none = micron::numeric_limits<I>::max();
  components_result<I> result{ algorithm_status::ok, 0, micron::vector<I, micron::allocator_serial<>, false>(graph.vertex_slots(), none) };
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());

  for ( auto root : graph.vertices() ) {
    if ( result.component.data()[static_cast<usize>(root.value)] != none ) continue;
    const I component_id = static_cast<I>(result.count++);
    queue.clear();
    queue.push_back(root);
    result.component.data()[static_cast<usize>(root.value)] = component_id;
    usize head = 0;
    while ( head < queue.size() ) {
      const vertex_descriptor u = queue.data()[head++];
      for ( auto v : graph.out_neighbors(u) ) {
        const usize slot = static_cast<usize>(v.value);
        if ( result.component.data()[slot] != none ) continue;
        result.component.data()[slot] = component_id;
        queue.push_back(v);
      }
      if constexpr ( G::is_directed ) {
        for ( auto v : graph.in_neighbors(u) ) {
          const usize slot = static_cast<usize>(v.value);
          if ( result.component.data()[slot] != none ) continue;
          result.component.data()[slot] = component_id;
          queue.push_back(v);
        }
      }
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
weakly_connected_components(const G &graph)
{
  return connected_components(graph);
}

template<graph_model G>
[[nodiscard]] components_result<typename G::index_type>
strongly_connected_components(const G &graph)
{
  using I = typename G::index_type;
  using vertex_descriptor = typename G::vertex_descriptor;
  if constexpr ( !G::is_directed ) return connected_components(graph);

  const usize slots = graph.vertex_slots();
  micron::vector<u8, micron::allocator_serial<>, false> seen(slots, u8(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> finish;
  finish.reserve(graph.vertices_count());

  auto visit_out = [&](auto &&self, vertex_descriptor u) -> void {
    seen.data()[static_cast<usize>(u.value)] = 1;
    for ( auto v : graph.out_neighbors(u) )
      if ( !seen.data()[static_cast<usize>(v.value)] ) self(self, v);
    finish.push_back(u);
  };
  for ( auto vertex : graph.vertices() )
    if ( !seen.data()[static_cast<usize>(vertex.value)] ) visit_out(visit_out, vertex);

  const I none = micron::numeric_limits<I>::max();
  components_result<I> result{ algorithm_status::ok, 0, micron::vector<I, micron::allocator_serial<>, false>(slots, none) };
  auto visit_in = [&](auto &&self, vertex_descriptor u, I component_id) -> void {
    result.component.data()[static_cast<usize>(u.value)] = component_id;
    for ( auto v : graph.in_neighbors(u) )
      if ( result.component.data()[static_cast<usize>(v.value)] == none ) self(self, v, component_id);
  };
  for ( usize i = finish.size(); i > 0; --i ) {
    const vertex_descriptor root = finish.data()[i - 1];
    if ( result.component.data()[static_cast<usize>(root.value)] != none ) continue;
    const I component_id = static_cast<I>(result.count++);
    visit_in(visit_in, root, component_id);
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
kosaraju_scc(const G &graph)
{
  return strongly_connected_components(graph);
}

template<graph_model G>
[[nodiscard]] auto
tarjan_scc(const G &graph)
{
  using I = typename G::index_type;
  using vertex_descriptor = typename G::vertex_descriptor;
  if constexpr ( !G::is_directed ) return connected_components(graph);

  const usize slots = graph.vertex_slots();
  const I none = micron::numeric_limits<I>::max();
  components_result<I> result{ algorithm_status::ok, 0, micron::vector<I, micron::allocator_serial<>, false>(slots, none) };
  micron::vector<usize, micron::allocator_serial<>, false> discovery(slots, usize(0));
  micron::vector<usize, micron::allocator_serial<>, false> low(slots, usize(0));
  micron::vector<u8, micron::allocator_serial<>, false> on_stack(slots, u8(0));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> stack;
  stack.reserve(graph.vertices_count());
  usize time = 0;

  auto visit = [&](auto &&self, vertex_descriptor u) -> void {
    const usize us = static_cast<usize>(u.value);
    discovery.data()[us] = low.data()[us] = ++time;
    stack.push_back(u);
    on_stack.data()[us] = 1;

    for ( auto v : graph.out_neighbors(u) ) {
      const usize vs = static_cast<usize>(v.value);
      if ( discovery.data()[vs] == 0 ) {
        self(self, v);
        if ( low.data()[vs] < low.data()[us] ) low.data()[us] = low.data()[vs];
      } else if ( on_stack.data()[vs] && discovery.data()[vs] < low.data()[us] ) {
        low.data()[us] = discovery.data()[vs];
      }
    }

    if ( low.data()[us] != discovery.data()[us] ) return;
    const I component_id = static_cast<I>(result.count++);
    for ( ;; ) {
      const vertex_descriptor member = stack.back();
      stack.pop_back();
      const usize slot = static_cast<usize>(member.value);
      on_stack.data()[slot] = 0;
      result.component.data()[slot] = component_id;
      if ( member == u ) break;
    }
  };

  for ( auto vertex : graph.vertices() )
    if ( discovery.data()[static_cast<usize>(vertex.value)] == 0 ) visit(visit, vertex);
  return result;
}

template<graph_model G>
[[nodiscard]] bool
is_connected(const G &graph)
{
  if ( graph.vertices_count() < 2 ) return true;
  return connected_components(graph).count == 1;
}

template<graph_model G>
[[nodiscard]] bool
is_strongly_connected(const G &graph)
{
  if ( graph.vertices_count() < 2 ) return true;
  return strongly_connected_components(graph).count == 1;
}

template<graph_model G> struct condensation_result {
  using index_type = typename G::index_type;
  digraph<empty_property, empty_property, empty_property, index_type> value;
  components_result<index_type> components;
};

template<graph_model G>
[[nodiscard]] condensation_result<G>
condensation(const G &graph)
{
  using I = typename G::index_type;
  condensation_result<G> result;
  result.components = strongly_connected_components(graph);
  (void)result.value.add_vertices(result.components.count);
  for ( auto edge : graph.edges() ) {
    const I a = result.components.component.data()[static_cast<usize>(edge.source.value)];
    const I b = result.components.component.data()[static_cast<usize>(edge.target.value)];
    if ( a != b ) (void)result.value.add_edge(a, b);
  }
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::index_type, micron::allocator_serial<>, false>
attracting_components(const G &graph)
{
  using I = typename G::index_type;
  auto scc = strongly_connected_components(graph);
  micron::vector<u8, micron::allocator_serial<>, false> attracting(scc.count, u8(1));
  for ( auto edge : graph.edges() ) {
    const I a = scc.component.data()[static_cast<usize>(edge.source.value)];
    const I b = scc.component.data()[static_cast<usize>(edge.target.value)];
    if ( a != b ) attracting.data()[static_cast<usize>(a)] = 0;
  }
  micron::vector<I, micron::allocator_serial<>, false> result;
  for ( usize i = 0; i < attracting.size(); ++i )
    if ( attracting.data()[i] ) result.push_back(static_cast<I>(i));
  return result;
}

template<micron::integral I> struct bipartite_result {
  algorithm_status status{ algorithm_status::ok };
  bool bipartite{ true };
  micron::vector<i8, micron::allocator_serial<>, false> color;
  vertex_id<I> conflict_a{};
  vertex_id<I> conflict_b{};
};

template<graph_model G>
[[nodiscard]] bipartite_result<typename G::index_type>
bipartite_test(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  bipartite_result<typename G::index_type> result;
  result.color = micron::vector<i8, micron::allocator_serial<>, false>(graph.vertex_slots(), i8(-1));
  micron::vector<vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());
  for ( auto root : graph.vertices() ) {
    if ( result.color.data()[static_cast<usize>(root.value)] != -1 ) continue;
    result.color.data()[static_cast<usize>(root.value)] = 0;
    queue.clear();
    queue.push_back(root);
    usize head = 0;
    while ( head < queue.size() ) {
      const vertex_descriptor u = queue.data()[head++];
      const i8 next_color = static_cast<i8>(1 - result.color.data()[static_cast<usize>(u.value)]);
      auto inspect = [&](vertex_descriptor v) {
        i8 &color = result.color.data()[static_cast<usize>(v.value)];
        if ( color == -1 ) {
          color = next_color;
          queue.push_back(v);
        } else if ( color != next_color ) {
          result.bipartite = false;
          result.conflict_a = u;
          result.conflict_b = v;
        }
      };
      for ( auto v : graph.out_neighbors(u) ) inspect(v);
      if constexpr ( G::is_directed )
        for ( auto v : graph.in_neighbors(u) ) inspect(v);
      if ( !result.bipartite ) return result;
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] bool
is_bipartite(const G &graph)
{
  return bipartite_test(graph).bipartite;
}

template<micron::integral I = u32> class union_find
{
  micron::vector<I, micron::allocator_serial<>, false> __parent;
  micron::vector<u8, micron::allocator_serial<>, false> __rank;
  usize __sets{};

public:
  union_find() = default;

  explicit union_find(usize size) : __parent(size), __rank(size, u8(0)), __sets(size)
  {
    for ( usize i = 0; i < size; ++i ) __parent.data()[i] = static_cast<I>(i);
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __parent.size();
  }

  [[nodiscard]] usize
  sets_count() const noexcept
  {
    return __sets;
  }

  I
  find(I item) noexcept
  {
    I root = item;
    while ( __parent.data()[static_cast<usize>(root)] != root ) root = __parent.data()[static_cast<usize>(root)];
    while ( __parent.data()[static_cast<usize>(item)] != item ) {
      const I next = __parent.data()[static_cast<usize>(item)];
      __parent.data()[static_cast<usize>(item)] = root;
      item = next;
    }
    return root;
  }

  bool
  unite(I a, I b) noexcept
  {
    a = find(a);
    b = find(b);
    if ( a == b ) return false;
    u8 &rank_a = __rank.data()[static_cast<usize>(a)];
    u8 &rank_b = __rank.data()[static_cast<usize>(b)];
    if ( rank_a < rank_b )
      __parent.data()[static_cast<usize>(a)] = b;
    else {
      __parent.data()[static_cast<usize>(b)] = a;
      if ( rank_a == rank_b ) ++rank_a;
    }
    --__sets;
    return true;
  }

  [[nodiscard]] bool
  connected(I a, I b) noexcept
  {
    return find(a) == find(b);
  }
};

template<micron::integral I> struct cut_structure_result {
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> articulation_points;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> bridges;
};

template<graph_model G>
[[nodiscard]] cut_structure_result<typename G::index_type>
cut_structure(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  const usize slots = graph.vertex_slots();
  cut_structure_result<typename G::index_type> result;
  micron::vector<usize, micron::allocator_serial<>, false> discovery(slots, usize(0));
  micron::vector<usize, micron::allocator_serial<>, false> low(slots, usize(0));
  micron::vector<u8, micron::allocator_serial<>, false> articulation(slots, u8(0));
  usize time = 0;

  auto visit = [&](auto &&self, vertex_descriptor u, edge_descriptor parent_edge) -> void {
    const usize us = static_cast<usize>(u.value);
    discovery.data()[us] = low.data()[us] = ++time;
    usize children = 0;
    for ( auto edge : graph.out_edges(u) ) {
      if ( edge == parent_edge ) continue;
      const vertex_descriptor v = G::is_directed ? graph.target(edge) : graph.opposite(edge, u);
      const usize vs = static_cast<usize>(v.value);
      if ( discovery.data()[vs] == 0 ) {
        ++children;
        self(self, v, edge);
        if ( low.data()[vs] < low.data()[us] ) low.data()[us] = low.data()[vs];
        if ( parent_edge.valid() && low.data()[vs] >= discovery.data()[us] ) articulation.data()[us] = 1;
        if ( low.data()[vs] > discovery.data()[us] ) result.bridges.push_back(edge);
      } else if ( discovery.data()[vs] < low.data()[us] ) {
        low.data()[us] = discovery.data()[vs];
      }
    }
    if ( !parent_edge.valid() && children > 1 ) articulation.data()[us] = 1;
  };

  for ( auto root : graph.vertices() )
    if ( discovery.data()[static_cast<usize>(root.value)] == 0 ) visit(visit, root, edge_descriptor::invalid());
  for ( auto vertex : graph.vertices() )
    if ( articulation.data()[static_cast<usize>(vertex.value)] ) result.articulation_points.push_back(vertex);
  return result;
}

template<graph_model G>
[[nodiscard]] auto
articulation_points(const G &graph)
{
  return cut_structure(graph).articulation_points;
}

template<graph_model G>
[[nodiscard]] auto
bridges(const G &graph)
{
  return cut_structure(graph).bridges;
}

template<micron::integral I> struct biconnected_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<micron::vector<edge_id<I>, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> blocks;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> articulation;
};

template<graph_model G>
[[nodiscard]] biconnected_result<typename G::index_type>
biconnected_components(const G &graph)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  using edge_descriptor = typename G::edge_descriptor;
  biconnected_result<typename G::index_type> result;
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  micron::vector<usize, micron::allocator_serial<>, false> discovery(graph.vertex_slots(), usize(0));
  micron::vector<usize, micron::allocator_serial<>, false> low(graph.vertex_slots(), usize(0));
  micron::vector<edge_descriptor, micron::allocator_serial<>, false> stack;
  micron::vector<u8, micron::allocator_serial<>, false> loop_seen(graph.edge_slots(), u8(0));
  usize time = 0;
  auto visit = [&](auto &&self, vertex_descriptor vertex, edge_descriptor parent) -> void {
    const usize slot = static_cast<usize>(vertex.value);
    discovery.data()[slot] = low.data()[slot] = ++time;
    for ( auto edge : graph.out_edges(vertex) ) {
      if ( graph.source(edge) == graph.target(edge) ) {
        const usize es = static_cast<usize>(edge.value);
        if ( !loop_seen.data()[es] ) {
          loop_seen.data()[es] = 1;
          micron::vector<edge_descriptor, micron::allocator_serial<>, false> block;
          block.push_back(edge);
          result.blocks.push_back(micron::move(block));
        }
        continue;
      }
      if ( edge == parent ) continue;
      const vertex_descriptor neighbor = graph.opposite(edge, vertex);
      const usize ns = static_cast<usize>(neighbor.value);
      if ( discovery.data()[ns] == 0 ) {
        stack.push_back(edge);
        self(self, neighbor, edge);
        if ( low.data()[ns] < low.data()[slot] ) low.data()[slot] = low.data()[ns];
        if ( low.data()[ns] >= discovery.data()[slot] ) {
          micron::vector<edge_descriptor, micron::allocator_serial<>, false> block;
          for ( ;; ) {
            const edge_descriptor member = stack.data()[stack.size() - 1];
            stack.pop_back();
            block.push_back(member);
            if ( member == edge ) break;
          }
          result.blocks.push_back(micron::move(block));
        }
      } else if ( discovery.data()[ns] < discovery.data()[slot] ) {
        stack.push_back(edge);
        if ( discovery.data()[ns] < low.data()[slot] ) low.data()[slot] = discovery.data()[ns];
      }
    }
  };
  for ( auto root : graph.vertices() )
    if ( discovery.data()[static_cast<usize>(root.value)] == 0 ) visit(visit, root, edge_descriptor::invalid());
  result.articulation = articulation_points(graph);
  return result;
}

};      // namespace micron::math::graphs
