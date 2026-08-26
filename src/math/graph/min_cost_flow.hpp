//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "flow.hpp"

namespace micron::math
{

template<micron::integral Capacity, typename Cost, typename Property = empty_property> struct capacity_cost_property {
  static_assert(micron::is_arithmetic_v<Cost> && micron::is_signed_v<Cost>, "min-cost flow costs must use a signed arithmetic type");

  Capacity capacity{};
  Cost cost{};
  [[no_unique_address]] Property property{};

  constexpr capacity_cost_property() = default;

  constexpr capacity_cost_property(Capacity cap, Cost value) : capacity(cap), cost(value), property() { }

  constexpr capacity_cost_property(Capacity cap, Cost value, const Property &p) : capacity(cap), cost(value), property(p) { }
};

namespace graphs
{

struct intrinsic_edge_capacity {
  template<graph_model G>
  [[nodiscard]] constexpr decltype(auto)
  operator()(const G &graph, typename G::edge_descriptor edge) const noexcept
  {
    return graph.edge_property_unchecked(edge).capacity;
  }
};

struct intrinsic_edge_cost {
  template<graph_model G>
  [[nodiscard]] constexpr decltype(auto)
  operator()(const G &graph, typename G::edge_descriptor edge) const noexcept
  {
    return graph.edge_property_unchecked(edge).cost;
  }
};

template<micron::integral I, micron::integral Capacity, typename Cost> struct min_cost_residual_arc {
  I target{};
  usize reverse{};
  Capacity capacity{};
  Cost cost{};
  edge_id<I> original{ edge_id<I>::invalid() };
  bool forward{};
  bool auxiliary{};
};

template<micron::integral I, micron::integral Capacity, typename Cost> struct min_cost_flow_workspace {
  static_assert(micron::is_arithmetic_v<Cost> && micron::is_signed_v<Cost>, "min-cost flow costs must use a signed arithmetic type");

  using arc_type = min_cost_residual_arc<I, Capacity, Cost>;
  using arc_vector = micron::vector<arc_type, micron::allocator_serial<>, false>;

  struct edge_location {
    I source{ vertex_id<I>::invalid_value() };
    usize arc{};
  };

  micron::vector<arc_vector, micron::allocator_serial<>, false> residual;
  micron::vector<edge_location, micron::allocator_serial<>, false> location;
  micron::vector<Capacity, micron::allocator_serial<>, false> original_capacity;
  micron::vector<Cost, micron::allocator_serial<>, false> original_cost;
  micron::vector<max_t, micron::allocator_serial<>, false> level;
  micron::vector<usize, micron::allocator_serial<>, false> next;
  micron::vector<I, micron::allocator_serial<>, false> queue;
  micron::vector<Cost, micron::allocator_serial<>, false> potential;
  micron::vector<Cost, micron::allocator_serial<>, false> distance;
  micron::vector<I, micron::allocator_serial<>, false> parent_vertex;
  micron::vector<usize, micron::allocator_serial<>, false> parent_arc;
  micron::vector<u8, micron::allocator_serial<>, false> reached;
  micron::vector<u8, micron::allocator_serial<>, false> settled;
  micron::vector<I, micron::allocator_serial<>, false> heap_vertex;
  micron::vector<Cost, micron::allocator_serial<>, false> heap_distance;

  void
  reserve(usize vertices, usize edges)
  {
    residual.reserve(vertices);
    location.reserve(edges);
    original_capacity.reserve(edges);
    original_cost.reserve(edges);
    level.reserve(vertices);
    next.reserve(vertices);
    queue.reserve(vertices);
    potential.reserve(vertices);
    distance.reserve(vertices);
    parent_vertex.reserve(vertices);
    parent_arc.reserve(vertices);
    reached.reserve(vertices);
    settled.reserve(vertices);
    heap_vertex.reserve(edges + vertices);
    heap_distance.reserve(edges + vertices);
  }
};

template<micron::integral I, micron::integral Capacity, typename Cost> struct min_cost_flow_result {
  algorithm_status status{ algorithm_status::ok };
  Capacity achieved_flow{};
  Cost total_cost{};
  micron::vector<Capacity, micron::allocator_serial<>, false> flow;
  micron::vector<Capacity, micron::allocator_serial<>, false> incoming;
  micron::vector<Capacity, micron::allocator_serial<>, false> outgoing;

  [[nodiscard]] bool
  feasible() const noexcept
  {
    return status == algorithm_status::ok;
  }
};

namespace __impl
{

template<typename Cost>
[[nodiscard]] bool
negate_cost(const Cost &value, Cost &result) noexcept
{
  if constexpr ( micron::is_integral_v<Cost> ) {
    return !__builtin_sub_overflow(Cost{}, value, &result);
  } else {
    result = -value;
    return !nonfinite_weight(result);
  }
}

template<typename Capacity, typename Cost>
[[nodiscard]] bool
flow_cost(const Capacity &flow, const Cost &cost, Cost &result) noexcept
{
  if constexpr ( micron::is_integral_v<Cost> ) {
    return !__builtin_mul_overflow(flow, cost, &result);
  } else {
    result = static_cast<Cost>(flow) * cost;
    return !nonfinite_weight(result);
  }
}

template<micron::integral Capacity, micron::integral Supply>
[[nodiscard]] bool
supply_capacity(Supply value, Capacity &result) noexcept
{
  if constexpr ( micron::is_signed_v<Supply> )
    if ( value < Supply{} ) return false;
  const uint128_t wide = static_cast<uint128_t>(value);
  if ( wide > static_cast<uint128_t>(micron::numeric_limits<Capacity>::max()) ) return false;
  result = static_cast<Capacity>(value);
  return true;
}

template<graph_model G, typename CapacityMap, typename CostMap, typename C, typename W>
algorithm_status
build_cost_residual(const G &graph, CapacityMap &capacity_map, CostMap &cost_map,
                    min_cost_flow_workspace<typename G::index_type, C, W> &workspace, usize extra_vertices = 0)
{
  using I = typename G::index_type;
  using arc_type = min_cost_residual_arc<I, C, W>;
  using raw_capacity_type
      = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  static_assert(micron::is_integral_v<raw_capacity_type>, "min-cost flow capacities must be integral");
  const usize vertex_slots = graph.vertex_slots();
  const usize edge_slots = graph.edge_slots();
  if ( extra_vertices > micron::numeric_limits<usize>::max() - vertex_slots ) return algorithm_status::overflow;
  const usize vertices = graph.vertex_slots() + extra_vertices;
  if ( (vertices != 0 && vertices - 1 > static_cast<usize>(micron::numeric_limits<I>::max()))
       || edge_slots > micron::numeric_limits<usize>::max() - vertices )
    return algorithm_status::overflow;
  workspace.reserve(vertices, edge_slots);
  workspace.residual.clear();
  workspace.residual.resize(vertices);
  workspace.location.resize(edge_slots);
  workspace.original_capacity.resize(edge_slots, C{});
  workspace.original_capacity.fill(C{});
  workspace.original_cost.resize(edge_slots, W{});
  workspace.original_cost.fill(W{});
  for ( auto edge : graph.edges() ) {
    const raw_capacity_type raw_capacity = __impl::weight(capacity_map, graph, edge.id);
    if ( __impl::invalid_weight(raw_capacity) ) return algorithm_status::invalid_weight;
    if ( static_cast<uint128_t>(raw_capacity) > static_cast<uint128_t>(micron::numeric_limits<C>::max()) )
      return algorithm_status::overflow;
    const C capacity = static_cast<C>(raw_capacity);
    const W cost = static_cast<W>(__impl::weight(cost_map, graph, edge.id));
    if ( __impl::nonfinite_weight(cost) ) return algorithm_status::invalid_weight;
    W reverse_cost{};
    if ( !negate_cost(cost, reverse_cost) ) return algorithm_status::overflow;
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    const usize forward = workspace.residual.data()[u].size();
    const usize reverse = workspace.residual.data()[v].size();
    workspace.residual.data()[u].push_back(arc_type{ edge.target.value, reverse, capacity, cost, edge.id, true, false });
    workspace.residual.data()[v].push_back(arc_type{ edge.source.value, forward, C{}, reverse_cost, edge.id, false, false });
    workspace.location.data()[static_cast<usize>(edge.id.value)] = { edge.source.value, forward };
    workspace.original_capacity.data()[static_cast<usize>(edge.id.value)] = capacity;
    workspace.original_cost.data()[static_cast<usize>(edge.id.value)] = cost;
  }
  return algorithm_status::ok;
}

template<typename I, typename C, typename W>
algorithm_status
add_residual_arc(min_cost_flow_workspace<I, C, W> &workspace, usize source, usize target, C capacity, W cost, bool auxiliary)
{
  using arc_type = min_cost_residual_arc<I, C, W>;
  W reverse_cost{};
  if ( !negate_cost(cost, reverse_cost) ) return algorithm_status::overflow;
  const usize forward = workspace.residual.data()[source].size();
  const usize reverse = workspace.residual.data()[target].size();
  workspace.residual.data()[source].push_back(
      arc_type{ static_cast<I>(target), reverse, capacity, cost, edge_id<I>::invalid(), false, auxiliary });
  workspace.residual.data()[target].push_back(
      arc_type{ static_cast<I>(source), forward, C{}, reverse_cost, edge_id<I>::invalid(), false, auxiliary });
  return algorithm_status::ok;
}

template<typename I, typename C, typename W>
algorithm_status
dinic_residual(min_cost_flow_workspace<I, C, W> &workspace, usize source, usize sink, C limit, bool limited, C &achieved)
{
  const usize vertices = workspace.residual.size();
  workspace.level.resize(vertices, max_t(-1));
  workspace.next.resize(vertices, usize(0));
  auto levels = [&]() {
    workspace.level.fill(max_t(-1));
    workspace.queue.clear();
    workspace.queue.push_back(static_cast<I>(source));
    workspace.level.data()[source] = 0;
    for ( usize head = 0; head < workspace.queue.size(); ++head ) {
      const usize u = static_cast<usize>(workspace.queue.data()[head]);
      for ( const auto &arc : workspace.residual.data()[u] ) {
        const usize v = static_cast<usize>(arc.target);
        if ( arc.capacity > C{} && workspace.level.data()[v] < 0 ) {
          workspace.level.data()[v] = workspace.level.data()[u] + 1;
          workspace.queue.push_back(arc.target);
        }
      }
    }
    return workspace.level.data()[sink] >= 0;
  };
  bool overflow = false;
  auto send = [&](auto &&self, usize u, C offered) -> C {
    if ( u == sink || offered == C{} ) return offered;
    auto &arcs = workspace.residual.data()[u];
    for ( usize &at = workspace.next.data()[u]; at < arcs.size(); ++at ) {
      auto &arc = arcs.data()[at];
      const usize v = static_cast<usize>(arc.target);
      if ( arc.capacity <= C{} || workspace.level.data()[v] != workspace.level.data()[u] + 1 ) continue;
      const C delivered = self(self, v, offered < arc.capacity ? offered : arc.capacity);
      if ( delivered == C{} ) continue;
      C reverse{};
      if ( !__impl::add_distance(workspace.residual.data()[v].data()[arc.reverse].capacity, delivered, reverse) ) {
        overflow = true;
        return C{};
      }
      arc.capacity -= delivered;
      workspace.residual.data()[v].data()[arc.reverse].capacity = reverse;
      return delivered;
    }
    return C{};
  };
  C source_bound{};
  for ( const auto &arc : workspace.residual.data()[source] ) {
    C sum{};
    if ( !__impl::add_distance(source_bound, arc.capacity, sum) ) return algorithm_status::overflow;
    source_bound = sum;
  }
  while ( (!limited || achieved < limit) && levels() ) {
    workspace.next.fill(usize(0));
    for ( ;; ) {
      C offered = source_bound;
      if ( limited ) offered = limit - achieved < offered ? limit - achieved : offered;
      if ( offered == C{} ) break;
      const C pushed = send(send, source, offered);
      if ( overflow ) return algorithm_status::overflow;
      if ( pushed == C{} ) break;
      C total{};
      if ( !__impl::add_distance(achieved, pushed, total) ) return algorithm_status::overflow;
      achieved = total;
    }
  }
  return algorithm_status::ok;
}

template<typename I, typename C, typename W>
algorithm_status
reduced_cost_augment(min_cost_flow_workspace<I, C, W> &workspace, usize source, usize sink, C limit, bool limited, C &achieved,
                     bool &applicable)
{
  const usize vertices = workspace.residual.size();
  applicable = false;
  workspace.potential.resize(vertices, W{});
  workspace.potential.fill(W{});
  for ( usize pass = 0; pass < vertices; ++pass ) {
    bool changed = false;
    for ( usize u = 0; u < vertices; ++u )
      for ( const auto &arc : workspace.residual.data()[u] ) {
        const usize v = static_cast<usize>(arc.target);
        if ( v >= vertices || arc.capacity <= C{} || arc.auxiliary ) continue;
        W candidate{};
        if ( !__impl::add_distance(workspace.potential.data()[u], arc.cost, candidate) ) return algorithm_status::overflow;
        if ( candidate < workspace.potential.data()[v] ) {
          workspace.potential.data()[v] = candidate;
          changed = true;
          if ( pass + 1 == vertices ) return algorithm_status::ok;
        }
      }
    if ( !changed ) break;
  }
  applicable = true;

  workspace.distance.resize(vertices, W{});
  workspace.parent_vertex.resize(vertices, vertex_id<I>::invalid_value());
  workspace.parent_arc.resize(vertices, usize(0));
  workspace.reached.resize(vertices, u8(0));
  workspace.settled.resize(vertices, u8(0));

  auto push_heap = [&](I vertex, W distance) {
    workspace.heap_vertex.push_back(vertex);
    workspace.heap_distance.push_back(distance);
    usize child = workspace.heap_vertex.size() - 1;
    while ( child ) {
      const usize parent = (child - 1) / 2;
      if ( !(workspace.heap_distance.data()[child] < workspace.heap_distance.data()[parent]) ) break;
      micron::swap(workspace.heap_vertex.data()[child], workspace.heap_vertex.data()[parent]);
      micron::swap(workspace.heap_distance.data()[child], workspace.heap_distance.data()[parent]);
      child = parent;
    }
  };
  auto pop_heap = [&]() {
    const I vertex = workspace.heap_vertex.data()[0];
    const W distance = workspace.heap_distance.data()[0];
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
      const usize child
          = right < workspace.heap_vertex.size() && workspace.heap_distance.data()[right] < workspace.heap_distance.data()[left] ? right
                                                                                                                                 : left;
      if ( !(workspace.heap_distance.data()[child] < workspace.heap_distance.data()[parent]) ) break;
      micron::swap(workspace.heap_vertex.data()[child], workspace.heap_vertex.data()[parent]);
      micron::swap(workspace.heap_distance.data()[child], workspace.heap_distance.data()[parent]);
      parent = child;
    }
    return micron::pair<I, W>{ vertex, distance };
  };

  while ( !limited || achieved < limit ) {
    workspace.distance.fill(W{});
    workspace.parent_vertex.fill(vertex_id<I>::invalid_value());
    workspace.reached.fill(u8(0));
    workspace.settled.fill(u8(0));
    workspace.heap_vertex.clear();
    workspace.heap_distance.clear();
    workspace.reached.data()[source] = 1;
    push_heap(static_cast<I>(source), W{});
    while ( !workspace.heap_vertex.empty() ) {
      const auto selected = pop_heap();
      const usize u = static_cast<usize>(selected.a);
      if ( workspace.settled.data()[u] || selected.b != workspace.distance.data()[u] ) continue;
      workspace.settled.data()[u] = 1;
      if ( u == sink ) break;
      const auto &arcs = workspace.residual.data()[u];
      for ( usize at = 0; at < arcs.size(); ++at ) {
        const auto &arc = arcs.data()[at];
        const usize v = static_cast<usize>(arc.target);
        if ( v >= vertices || arc.capacity <= C{} || arc.auxiliary ) continue;
        W partial{}, reduced{};
        if ( !__impl::add_distance(arc.cost, workspace.potential.data()[u], partial)
             || !__impl::sub_distance(partial, workspace.potential.data()[v], reduced) )
          return algorithm_status::overflow;
        if ( reduced < W{} ) {
          applicable = false;
          return algorithm_status::ok;
        }
        W candidate{};
        if ( !__impl::add_distance(workspace.distance.data()[u], reduced, candidate) ) return algorithm_status::overflow;
        if ( !workspace.reached.data()[v] || candidate < workspace.distance.data()[v] ) {
          workspace.reached.data()[v] = 1;
          workspace.distance.data()[v] = candidate;
          workspace.parent_vertex.data()[v] = static_cast<I>(u);
          workspace.parent_arc.data()[v] = at;
          push_heap(static_cast<I>(v), candidate);
        }
      }
    }
    if ( !workspace.reached.data()[sink] ) return algorithm_status::ok;
    for ( usize vertex = 0; vertex < vertices; ++vertex ) {
      if ( !workspace.reached.data()[vertex] ) continue;
      W updated{};
      if ( !__impl::add_distance(workspace.potential.data()[vertex], workspace.distance.data()[vertex], updated) )
        return algorithm_status::overflow;
      workspace.potential.data()[vertex] = updated;
    }

    C pushed{};
    bool have = false;
    usize current = sink;
    while ( current != source ) {
      const I parent_value = workspace.parent_vertex.data()[current];
      if ( parent_value == vertex_id<I>::invalid_value() ) return algorithm_status::invalid_graph;
      const usize parent = static_cast<usize>(parent_value);
      const auto &arc = workspace.residual.data()[parent].data()[workspace.parent_arc.data()[current]];
      if ( !have || arc.capacity < pushed ) {
        pushed = arc.capacity;
        have = true;
      }
      current = parent;
    }
    if ( limited ) {
      const C remaining = limit - achieved;
      if ( remaining < pushed ) pushed = remaining;
    }
    if ( !have || pushed == C{} ) return algorithm_status::invalid_graph;
    current = sink;
    while ( current != source ) {
      const usize parent = static_cast<usize>(workspace.parent_vertex.data()[current]);
      auto &arc = workspace.residual.data()[parent].data()[workspace.parent_arc.data()[current]];
      C reverse{};
      if ( !__impl::add_distance(workspace.residual.data()[current].data()[arc.reverse].capacity, pushed, reverse) )
        return algorithm_status::overflow;
      arc.capacity -= pushed;
      workspace.residual.data()[current].data()[arc.reverse].capacity = reverse;
      current = parent;
    }
    C total{};
    if ( !__impl::add_distance(achieved, pushed, total) ) return algorithm_status::overflow;
    achieved = total;
  }
  return algorithm_status::ok;
}

template<typename I, typename C, typename W>
algorithm_status
cancel_negative_cycles(min_cost_flow_workspace<I, C, W> &workspace, usize vertices)
{
  workspace.distance.resize(vertices, W{});
  workspace.parent_vertex.resize(vertices, vertex_id<I>::invalid_value());
  workspace.parent_arc.resize(vertices, usize(0));
  for ( ;; ) {
    workspace.distance.fill(W{});
    workspace.parent_vertex.fill(vertex_id<I>::invalid_value());
    I changed = vertex_id<I>::invalid_value();
    for ( usize pass = 0; pass < vertices; ++pass ) {
      changed = vertex_id<I>::invalid_value();
      for ( usize u = 0; u < vertices; ++u ) {
        const auto &arcs = workspace.residual.data()[u];
        for ( usize at = 0; at < arcs.size(); ++at ) {
          const auto &arc = arcs.data()[at];
          const usize v = static_cast<usize>(arc.target);
          if ( v >= vertices || arc.capacity <= C{} || arc.auxiliary ) continue;
          W candidate{};
          if ( !__impl::add_distance(workspace.distance.data()[u], arc.cost, candidate) ) return algorithm_status::overflow;
          if ( candidate < workspace.distance.data()[v] ) {
            workspace.distance.data()[v] = candidate;
            workspace.parent_vertex.data()[v] = static_cast<I>(u);
            workspace.parent_arc.data()[v] = at;
            changed = static_cast<I>(v);
          }
        }
      }
      if ( changed == vertex_id<I>::invalid_value() ) break;
    }
    if ( changed == vertex_id<I>::invalid_value() ) return algorithm_status::ok;
    usize cycle = static_cast<usize>(changed);
    for ( usize i = 0; i < vertices; ++i ) {
      const I parent = workspace.parent_vertex.data()[cycle];
      if ( parent == vertex_id<I>::invalid_value() ) return algorithm_status::invalid_graph;
      cycle = static_cast<usize>(parent);
    }
    C bottleneck{};
    bool have = false;
    usize current = cycle;
    do {
      const usize parent = static_cast<usize>(workspace.parent_vertex.data()[current]);
      const auto &arc = workspace.residual.data()[parent].data()[workspace.parent_arc.data()[current]];
      if ( !have || arc.capacity < bottleneck ) {
        bottleneck = arc.capacity;
        have = true;
      }
      current = parent;
    } while ( current != cycle );
    if ( !have || bottleneck == C{} ) return algorithm_status::invalid_graph;
    current = cycle;
    do {
      const usize parent = static_cast<usize>(workspace.parent_vertex.data()[current]);
      auto &arc = workspace.residual.data()[parent].data()[workspace.parent_arc.data()[current]];
      C reverse{};
      if ( !__impl::add_distance(workspace.residual.data()[current].data()[arc.reverse].capacity, bottleneck, reverse) )
        return algorithm_status::overflow;
      arc.capacity -= bottleneck;
      workspace.residual.data()[current].data()[arc.reverse].capacity = reverse;
      current = parent;
    } while ( current != cycle );
  }
}

template<typename I, typename C, typename W>
algorithm_status
negative_cycle_status(min_cost_flow_workspace<I, C, W> &workspace, usize vertices)
{
  workspace.distance.resize(vertices, W{});
  workspace.distance.fill(W{});
  for ( usize pass = 0; pass < vertices; ++pass ) {
    bool changed = false;
    for ( usize u = 0; u < vertices; ++u )
      for ( const auto &arc : workspace.residual.data()[u] ) {
        const usize v = static_cast<usize>(arc.target);
        if ( v >= vertices || arc.capacity <= C{} || arc.auxiliary ) continue;
        W candidate{};
        if ( !__impl::add_distance(workspace.distance.data()[u], arc.cost, candidate) ) return algorithm_status::overflow;
        if ( candidate < workspace.distance.data()[v] ) {
          workspace.distance.data()[v] = candidate;
          changed = true;
          if ( pass + 1 == vertices ) return algorithm_status::negative_cycle;
        }
      }
    if ( !changed ) break;
  }
  return algorithm_status::ok;
}

template<graph_model G, typename C, typename W>
algorithm_status
finish_cost_result(const G &graph, min_cost_flow_workspace<typename G::index_type, C, W> &workspace,
                   min_cost_flow_result<typename G::index_type, C, W> &result)
{
  result.flow.resize(graph.edge_slots(), C{});
  result.flow.fill(C{});
  result.incoming.resize(graph.vertex_slots(), C{});
  result.incoming.fill(C{});
  result.outgoing.resize(graph.vertex_slots(), C{});
  result.outgoing.fill(C{});
  result.total_cost = W{};
  for ( auto edge : graph.edges() ) {
    const usize slot = static_cast<usize>(edge.id.value);
    const auto location = workspace.location.data()[slot];
    const C remaining = workspace.residual.data()[static_cast<usize>(location.source)].data()[location.arc].capacity;
    const C flow = workspace.original_capacity.data()[slot] - remaining;
    result.flow.data()[slot] = flow;
    C sum{};
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    if ( !__impl::add_distance(result.outgoing.data()[u], flow, sum) ) return algorithm_status::overflow;
    result.outgoing.data()[u] = sum;
    if ( !__impl::add_distance(result.incoming.data()[v], flow, sum) ) return algorithm_status::overflow;
    result.incoming.data()[v] = sum;
    W term{}, total{};
    if ( !flow_cost(flow, workspace.original_cost.data()[slot], term) || !__impl::add_distance(result.total_cost, term, total) )
      return algorithm_status::overflow;
    result.total_cost = total;
  }
  return algorithm_status::ok;
}

};      // namespace __impl

template<graph_model G, typename Capacity, typename CapacityMap, typename CostMap>
[[nodiscard]] auto
min_cost_flow(
    const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, Capacity requested, CapacityMap capacity_map,
    CostMap cost_map,
    min_cost_flow_workspace<typename G::index_type, Capacity,
                            micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CostMap &>(), micron::declval<const G &>(),
                                                                           micron::declval<typename G::edge_descriptor>()))>> &workspace)
  requires micron::integral<Capacity>
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(cost_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  min_cost_flow_result<I, Capacity, W> result;
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  if ( !graph.has_vertex(source) || !graph.has_vertex(sink) || source == sink ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }
  if constexpr ( micron::is_signed_v<Capacity> )
    if ( requested < Capacity{} ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
  result.status = __impl::build_cost_residual<G, CapacityMap, CostMap, Capacity, W>(graph, capacity_map, cost_map, workspace);
  if ( result.status != algorithm_status::ok ) return result;
  bool reduced_costs = false;
  result.status = __impl::reduced_cost_augment(workspace, static_cast<usize>(source.value), static_cast<usize>(sink.value), requested, true,
                                               result.achieved_flow, reduced_costs);
  if ( result.status == algorithm_status::ok && !reduced_costs )
    result.status = __impl::dinic_residual(workspace, static_cast<usize>(source.value), static_cast<usize>(sink.value), requested, true,
                                           result.achieved_flow);
  if ( result.status != algorithm_status::ok ) return result;
  if ( result.achieved_flow != requested ) {
    result.status = algorithm_status::infeasible;
    return result;
  }
  result.status = __impl::cancel_negative_cycles(workspace, graph.vertex_slots());
  if ( result.status == algorithm_status::ok ) result.status = __impl::finish_cost_result(graph, workspace, result);
  return result;
}

template<graph_model G, typename Capacity, typename CapacityMap, typename CostMap>
[[nodiscard]] auto
min_cost_flow(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, Capacity requested,
              CapacityMap capacity_map, CostMap cost_map)
  requires micron::integral<Capacity>
{
  using W = micron::remove_cvref_t<decltype(__impl::weight(cost_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  min_cost_flow_workspace<typename G::index_type, Capacity, W> workspace;
  return min_cost_flow(graph, source, sink, requested, capacity_map, cost_map, workspace);
}

template<graph_model G, typename Capacity>
[[nodiscard]] auto
min_cost_flow(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, Capacity requested)
  requires micron::integral<Capacity>
{
  return min_cost_flow(graph, source, sink, requested, intrinsic_edge_capacity{}, intrinsic_edge_cost{});
}

template<graph_model G, typename CapacityMap, typename CostMap>
[[nodiscard]] auto
min_cost_max_flow(
    const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, CapacityMap capacity_map, CostMap cost_map,
    min_cost_flow_workspace<typename G::index_type,
                            micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CapacityMap &>(), micron::declval<const G &>(),
                                                                           micron::declval<typename G::edge_descriptor>()))>,
                            micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CostMap &>(), micron::declval<const G &>(),
                                                                           micron::declval<typename G::edge_descriptor>()))>> &workspace)
{
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using W = micron::remove_cvref_t<decltype(__impl::weight(cost_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  min_cost_flow_result<typename G::index_type, C, W> result;
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  if ( !graph.has_vertex(source) || !graph.has_vertex(sink) || source == sink ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }
  result.status = __impl::build_cost_residual<G, CapacityMap, CostMap, C, W>(graph, capacity_map, cost_map, workspace);
  if ( result.status != algorithm_status::ok ) return result;
  bool reduced_costs = false;
  result.status = __impl::reduced_cost_augment(workspace, static_cast<usize>(source.value), static_cast<usize>(sink.value), C{}, false,
                                               result.achieved_flow, reduced_costs);
  if ( result.status == algorithm_status::ok && !reduced_costs )
    result.status = __impl::dinic_residual(workspace, static_cast<usize>(source.value), static_cast<usize>(sink.value), C{}, false,
                                           result.achieved_flow);
  if ( result.status == algorithm_status::ok ) result.status = __impl::cancel_negative_cycles(workspace, graph.vertex_slots());
  if ( result.status == algorithm_status::ok ) result.status = __impl::finish_cost_result(graph, workspace, result);
  return result;
}

template<graph_model G, typename CapacityMap, typename CostMap>
[[nodiscard]] auto
min_cost_max_flow(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, CapacityMap capacity_map,
                  CostMap cost_map)
{
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using W = micron::remove_cvref_t<decltype(__impl::weight(cost_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  min_cost_flow_workspace<typename G::index_type, C, W> workspace;
  return min_cost_max_flow(graph, source, sink, capacity_map, cost_map, workspace);
}

template<graph_model G>
[[nodiscard]] auto
min_cost_max_flow(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink)
{
  return min_cost_max_flow(graph, source, sink, intrinsic_edge_capacity{}, intrinsic_edge_cost{});
}

template<graph_model G, typename Supplies, typename CapacityMap, typename CostMap>
[[nodiscard]] auto
min_cost_circulation(
    const G &graph, const Supplies &supplies, CapacityMap capacity_map, CostMap cost_map,
    min_cost_flow_workspace<typename G::index_type,
                            micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CapacityMap &>(), micron::declval<const G &>(),
                                                                           micron::declval<typename G::edge_descriptor>()))>,
                            micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CostMap &>(), micron::declval<const G &>(),
                                                                           micron::declval<typename G::edge_descriptor>()))>> &workspace)
{
  using I = typename G::index_type;
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using W = micron::remove_cvref_t<decltype(__impl::weight(cost_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using S = micron::remove_cvref_t<decltype(supplies[usize(0)])>;
  static_assert(micron::integral<S>, "min_cost_circulation supplies must be integral");
  min_cost_flow_result<I, C, W> result;
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  const usize vertices = graph.vertex_slots();
  if ( supplies.size() < vertices ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  result.status = __impl::build_cost_residual<G, CapacityMap, CostMap, C, W>(graph, capacity_map, cost_map, workspace, 2);
  if ( result.status != algorithm_status::ok ) return result;
  const usize super_source = vertices;
  const usize super_sink = vertices + 1;
  C total_supply{};
  C total_demand{};
  for ( auto vertex : graph.vertices() ) {
    const usize slot = static_cast<usize>(vertex.value);
    const auto value = supplies[slot];
    if ( value > decltype(value){} ) {
      C amount{};
      if ( !__impl::supply_capacity(value, amount) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      C total{};
      if ( !__impl::add_distance(total_supply, amount, total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      total_supply = total;
      result.status = __impl::add_residual_arc(workspace, super_source, slot, amount, W{}, true);
    } else if ( value < decltype(value){} ) {
      S positive{};
      if ( __builtin_sub_overflow(S{}, value, &positive) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      C amount{};
      if ( !__impl::supply_capacity(positive, amount) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      C total{};
      if ( !__impl::add_distance(total_demand, amount, total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      total_demand = total;
      result.status = __impl::add_residual_arc(workspace, slot, super_sink, amount, W{}, true);
    }
    if ( result.status != algorithm_status::ok ) return result;
  }
  if ( total_supply != total_demand ) {
    result.status = algorithm_status::infeasible;
    return result;
  }
  C sent{};
  result.status = __impl::dinic_residual(workspace, super_source, super_sink, total_supply, true, sent);
  if ( result.status != algorithm_status::ok ) return result;
  if ( sent != total_supply ) {
    result.status = algorithm_status::infeasible;
    return result;
  }
  result.achieved_flow = sent;
  for ( usize u = 0; u < workspace.residual.size(); ++u )
    for ( auto &arc : workspace.residual.data()[u] )
      if ( arc.auxiliary ) arc.capacity = C{};
  result.status = __impl::cancel_negative_cycles(workspace, vertices);
  if ( result.status == algorithm_status::ok ) result.status = __impl::finish_cost_result(graph, workspace, result);
  return result;
}

template<graph_model G, typename Supplies, typename CapacityMap, typename CostMap>
[[nodiscard]] auto
min_cost_circulation(const G &graph, const Supplies &supplies, CapacityMap capacity_map, CostMap cost_map)
{
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  using W = micron::remove_cvref_t<decltype(__impl::weight(cost_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  min_cost_flow_workspace<typename G::index_type, C, W> workspace;
  return min_cost_circulation(graph, supplies, capacity_map, cost_map, workspace);
}

template<graph_model G, typename Supplies>
[[nodiscard]] auto
min_cost_circulation(const G &graph, const Supplies &supplies)
{
  return min_cost_circulation(graph, supplies, intrinsic_edge_capacity{}, intrinsic_edge_cost{});
}

template<graph_model G, typename C, typename W, typename CapacityMap, typename CostMap>
[[nodiscard]] bool
verify_min_cost_flow(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink,
                     const min_cost_flow_result<typename G::index_type, C, W> &result, CapacityMap capacity_map, CostMap cost_map)
{
  if ( result.status != algorithm_status::ok || result.flow.size() < graph.edge_slots() ) return false;
  micron::vector<C, micron::allocator_serial<>, false> incoming(graph.vertex_slots(), C{});
  micron::vector<C, micron::allocator_serial<>, false> outgoing(graph.vertex_slots(), C{});
  min_cost_flow_workspace<typename G::index_type, C, W> workspace;
  if ( __impl::build_cost_residual<G, CapacityMap, CostMap, C, W>(graph, capacity_map, cost_map, workspace) != algorithm_status::ok )
    return false;
  W total_cost{};
  for ( auto edge : graph.edges() ) {
    const usize slot = static_cast<usize>(edge.id.value);
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    const C flow = result.flow.data()[slot];
    if constexpr ( micron::is_signed_v<C> )
      if ( flow < C{} ) return false;
    if ( flow > capacity ) return false;
    C capacity_sum{};
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    if ( !__impl::add_distance(outgoing.data()[u], flow, capacity_sum) ) return false;
    outgoing.data()[u] = capacity_sum;
    if ( !__impl::add_distance(incoming.data()[v], flow, capacity_sum) ) return false;
    incoming.data()[v] = capacity_sum;
    W term{}, cost_sum{};
    if ( !__impl::flow_cost(flow, static_cast<W>(__impl::weight(cost_map, graph, edge.id)), term)
         || !__impl::add_distance(total_cost, term, cost_sum) )
      return false;
    total_cost = cost_sum;
    const auto location = workspace.location.data()[slot];
    auto &forward = workspace.residual.data()[static_cast<usize>(location.source)].data()[location.arc];
    auto &reverse = workspace.residual.data()[static_cast<usize>(forward.target)].data()[forward.reverse];
    forward.capacity = capacity - flow;
    reverse.capacity = flow;
  }
  if ( total_cost != result.total_cost ) return false;
  for ( auto vertex : graph.vertices() ) {
    const usize slot = static_cast<usize>(vertex.value);
    if ( vertex != source && vertex != sink && incoming.data()[slot] != outgoing.data()[slot] ) return false;
  }
  const usize ss = static_cast<usize>(source.value);
  const usize ts = static_cast<usize>(sink.value);
  if ( !(outgoing.data()[ss] >= incoming.data()[ss] && outgoing.data()[ss] - incoming.data()[ss] == result.achieved_flow
         && incoming.data()[ts] >= outgoing.data()[ts] && incoming.data()[ts] - outgoing.data()[ts] == result.achieved_flow) )
    return false;
  if ( result.incoming.size() != result.outgoing.size() ) return false;
  if ( result.incoming.size() == graph.vertex_slots() )
    for ( usize i = 0; i < graph.vertex_slots(); ++i )
      if ( result.incoming.data()[i] != incoming.data()[i] || result.outgoing.data()[i] != outgoing.data()[i] ) return false;
  return __impl::negative_cycle_status(workspace, graph.vertex_slots()) == algorithm_status::ok;
}

template<graph_model G, typename Supplies, typename C, typename W, typename CapacityMap, typename CostMap>
[[nodiscard]] bool
verify_min_cost_flow(const G &graph, const Supplies &supplies, const min_cost_flow_result<typename G::index_type, C, W> &result,
                     CapacityMap capacity_map, CostMap cost_map)
{
  using S = micron::remove_cvref_t<decltype(supplies[usize(0)])>;
  static_assert(micron::integral<S>, "min-cost circulation supplies must be integral");
  if ( result.status != algorithm_status::ok || supplies.size() < graph.vertex_slots() || result.flow.size() < graph.edge_slots() )
    return false;
  micron::vector<C, micron::allocator_serial<>, false> incoming(graph.vertex_slots(), C{});
  micron::vector<C, micron::allocator_serial<>, false> outgoing(graph.vertex_slots(), C{});
  min_cost_flow_workspace<typename G::index_type, C, W> workspace;
  if ( __impl::build_cost_residual<G, CapacityMap, CostMap, C, W>(graph, capacity_map, cost_map, workspace) != algorithm_status::ok )
    return false;
  W total_cost{};
  for ( auto edge : graph.edges() ) {
    const usize slot = static_cast<usize>(edge.id.value);
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    const C flow = result.flow.data()[slot];
    if constexpr ( micron::is_signed_v<C> )
      if ( flow < C{} ) return false;
    if ( flow > capacity ) return false;
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    C sum{};
    if ( !__impl::add_distance(outgoing.data()[u], flow, sum) ) return false;
    outgoing.data()[u] = sum;
    if ( !__impl::add_distance(incoming.data()[v], flow, sum) ) return false;
    incoming.data()[v] = sum;
    W term{}, cost_sum{};
    if ( !__impl::flow_cost(flow, static_cast<W>(__impl::weight(cost_map, graph, edge.id)), term)
         || !__impl::add_distance(total_cost, term, cost_sum) )
      return false;
    total_cost = cost_sum;
    const auto location = workspace.location.data()[slot];
    auto &forward = workspace.residual.data()[static_cast<usize>(location.source)].data()[location.arc];
    auto &reverse = workspace.residual.data()[static_cast<usize>(forward.target)].data()[forward.reverse];
    forward.capacity = capacity - flow;
    reverse.capacity = flow;
  }
  if ( total_cost != result.total_cost ) return false;
  C total_supply{};
  for ( usize slot = 0; slot < graph.vertex_slots(); ++slot ) {
    const auto vertex = typename G::vertex_descriptor(static_cast<typename G::index_type>(slot));
    if ( !graph.has_vertex(vertex) ) {
      if ( supplies[slot] != S{} ) return false;
      continue;
    }
    const S supply = supplies[slot];
    C amount{};
    if ( supply > S{} ) {
      if ( !__impl::supply_capacity(supply, amount) || outgoing[slot] < incoming[slot] || outgoing[slot] - incoming[slot] != amount )
        return false;
      C sum{};
      if ( !__impl::add_distance(total_supply, amount, sum) ) return false;
      total_supply = sum;
    } else if ( supply < S{} ) {
      S positive{};
      if ( __builtin_sub_overflow(S{}, supply, &positive) || !__impl::supply_capacity(positive, amount) || incoming[slot] < outgoing[slot]
           || incoming[slot] - outgoing[slot] != amount )
        return false;
    } else if ( incoming[slot] != outgoing[slot] ) {
      return false;
    }
  }
  if ( result.achieved_flow != total_supply ) return false;
  if ( result.incoming.size() != graph.vertex_slots() || result.outgoing.size() != graph.vertex_slots() ) return false;
  for ( usize slot = 0; slot < graph.vertex_slots(); ++slot )
    if ( result.incoming[slot] != incoming[slot] || result.outgoing[slot] != outgoing[slot] ) return false;
  return __impl::negative_cycle_status(workspace, graph.vertex_slots()) == algorithm_status::ok;
}

template<graph_model G, typename Supplies, typename C, typename W>
[[nodiscard]] bool
verify_min_cost_flow(const G &graph, const Supplies &supplies, const min_cost_flow_result<typename G::index_type, C, W> &result)
{
  return verify_min_cost_flow(graph, supplies, result, intrinsic_edge_capacity{}, intrinsic_edge_cost{});
}

template<graph_model G, typename C, typename W>
[[nodiscard]] bool
verify_min_cost_flow(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink,
                     const min_cost_flow_result<typename G::index_type, C, W> &result)
{
  return verify_min_cost_flow(graph, source, sink, result, intrinsic_edge_capacity{}, intrinsic_edge_cost{});
}

};      // namespace graphs
};      // namespace micron::math
