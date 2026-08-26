//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "matrix.hpp"
#include "paths.hpp"

namespace micron::math::graphs
{

template<micron::integral I, typename Capacity> struct max_flow_result {
  algorithm_status status{ algorithm_status::ok };
  Capacity value{};
  micron::vector<Capacity, micron::allocator_serial<>, false> flow;
  micron::vector<u8, micron::allocator_serial<>, false> source_side;
};

template<graph_model G, typename CapacityMap>
[[nodiscard]] auto
dinic(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, CapacityMap capacity_map)
{
  using I = typename G::index_type;
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;

  struct residual_arc {
    I target{};
    usize reverse{};
    C capacity{};
  };

  struct edge_location {
    I source{ vertex_id<I>::invalid_value() };
    usize arc{};
  };

  max_flow_result<I, C> result{ algorithm_status::ok, C{}, micron::vector<C, micron::allocator_serial<>, false>(graph.edge_slots(), C{}),
                                micron::vector<u8, micron::allocator_serial<>, false>(graph.vertex_slots(), u8(0)) };
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  if ( !graph.has_vertex(source) || !graph.has_vertex(sink) || source == sink ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }

  using arc_vector = micron::vector<residual_arc, micron::allocator_serial<>, false>;
  micron::vector<arc_vector, micron::allocator_serial<>, false> residual(graph.vertex_slots());
  micron::vector<edge_location, micron::allocator_serial<>, false> location(graph.edge_slots());
  for ( auto edge : graph.edges() ) {
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    if ( __impl::invalid_weight(capacity) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    const usize forward = residual.data()[u].size();
    const usize reverse = residual.data()[v].size();
    residual.data()[u].push_back(residual_arc{ edge.target.value, reverse, capacity });
    residual.data()[v].push_back(residual_arc{ edge.source.value, forward, C{} });
    location.data()[static_cast<usize>(edge.id.value)] = edge_location{ edge.source.value, forward };
  }

  micron::vector<max_t, micron::allocator_serial<>, false> level(graph.vertex_slots(), max_t(-1));
  micron::vector<usize, micron::allocator_serial<>, false> next(graph.vertex_slots(), usize(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());

  auto build_levels = [&]() {
    level.fill(max_t(-1));
    queue.clear();
    queue.push_back(source);
    level.data()[static_cast<usize>(source.value)] = 0;
    usize head = 0;
    while ( head < queue.size() ) {
      const auto u = queue.data()[head++];
      for ( const residual_arc &arc : residual.data()[static_cast<usize>(u.value)] ) {
        const usize v = static_cast<usize>(arc.target);
        if ( arc.capacity > C{} && level.data()[v] < 0 ) {
          level.data()[v] = level.data()[static_cast<usize>(u.value)] + 1;
          queue.push_back(typename G::vertex_descriptor(arc.target));
        }
      }
    }
    return level.data()[static_cast<usize>(sink.value)] >= 0;
  };

  auto send = [&](auto &&self, I raw_u, C pushed) -> C {
    if ( raw_u == sink.value || pushed == C{} ) return pushed;
    const usize u = static_cast<usize>(raw_u);
    arc_vector &arcs = residual.data()[u];
    for ( usize &at = next.data()[u]; at < arcs.size(); ++at ) {
      residual_arc &arc = arcs.data()[at];
      const usize v = static_cast<usize>(arc.target);
      if ( arc.capacity <= C{} || level.data()[v] != level.data()[u] + 1 ) continue;
      const C offered = pushed < arc.capacity ? pushed : arc.capacity;
      const C delivered = self(self, arc.target, offered);
      if ( delivered == C{} ) continue;
      arc.capacity -= delivered;
      residual.data()[v].data()[arc.reverse].capacity += delivered;
      return delivered;
    }
    return C{};
  };

  // The source's outgoing residual sum is a finite, input-derived upper bound;
  // it replaces an infinity sentinel and remains sound under -Ofast.
  C source_bound{};
  for ( const residual_arc &arc : residual.data()[static_cast<usize>(source.value)] ) {
    C next_bound{};
    if ( !__impl::add_distance(source_bound, arc.capacity, next_bound) ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    source_bound = next_bound;
  }

  while ( build_levels() ) {
    next.fill(usize(0));
    for ( ;; ) {
      const C pushed = send(send, source.value, source_bound);
      if ( pushed == C{} ) break;
      C total{};
      if ( !__impl::add_distance(result.value, pushed, total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      result.value = total;
    }
  }

  for ( auto edge : graph.edges() ) {
    const edge_location place = location.data()[static_cast<usize>(edge.id.value)];
    const C original = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    const C remaining = residual.data()[static_cast<usize>(place.source)].data()[place.arc].capacity;
    result.flow.data()[static_cast<usize>(edge.id.value)] = original - remaining;
  }

  // Final residual reachability is the min-cut certificate.
  queue.clear();
  queue.push_back(source);
  result.source_side.data()[static_cast<usize>(source.value)] = 1;
  usize head = 0;
  while ( head < queue.size() ) {
    const auto u = queue.data()[head++];
    for ( const residual_arc &arc : residual.data()[static_cast<usize>(u.value)] ) {
      const usize v = static_cast<usize>(arc.target);
      if ( arc.capacity > C{} && !result.source_side.data()[v] ) {
        result.source_side.data()[v] = 1;
        queue.push_back(typename G::vertex_descriptor(arc.target));
      }
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
dinic(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink)
{
  return dinic(graph, source, sink, intrinsic_edge_weight{});
}

template<graph_model G, typename CapacityMap>
[[nodiscard]] auto
edmonds_karp(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, CapacityMap capacity_map)
{
  using I = typename G::index_type;
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;

  struct residual_arc {
    I target{};
    usize reverse{};
    C capacity{};
  };

  struct edge_location {
    I source{ vertex_id<I>::invalid_value() };
    usize arc{};
  };

  max_flow_result<I, C> result{ algorithm_status::ok, C{}, micron::vector<C, micron::allocator_serial<>, false>(graph.edge_slots(), C{}),
                                micron::vector<u8, micron::allocator_serial<>, false>(graph.vertex_slots(), u8(0)) };
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  if ( !graph.has_vertex(source) || !graph.has_vertex(sink) || source == sink ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }

  using arc_vector = micron::vector<residual_arc, micron::allocator_serial<>, false>;
  micron::vector<arc_vector, micron::allocator_serial<>, false> residual(graph.vertex_slots());
  micron::vector<edge_location, micron::allocator_serial<>, false> location(graph.edge_slots());
  for ( auto edge : graph.edges() ) {
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    if ( __impl::invalid_weight(capacity) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    const usize forward = residual.data()[u].size();
    const usize reverse = residual.data()[v].size();
    residual.data()[u].push_back(residual_arc{ edge.target.value, reverse, capacity });
    residual.data()[v].push_back(residual_arc{ edge.source.value, forward, C{} });
    location.data()[static_cast<usize>(edge.id.value)] = edge_location{ edge.source.value, forward };
  }

  const usize slots = graph.vertex_slots();
  micron::vector<I, micron::allocator_serial<>, false> parent(slots, vertex_id<I>::invalid_value());
  micron::vector<usize, micron::allocator_serial<>, false> parent_arc(slots, usize(0));
  micron::vector<u8, micron::allocator_serial<>, false> reached(slots, u8(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());

  for ( ;; ) {
    parent.fill(vertex_id<I>::invalid_value());
    reached.fill(u8(0));
    queue.clear();
    queue.push_back(source);
    reached.data()[static_cast<usize>(source.value)] = 1;
    usize head = 0;
    while ( head < queue.size() && !reached.data()[static_cast<usize>(sink.value)] ) {
      const auto u = queue.data()[head++];
      const auto &arcs = residual.data()[static_cast<usize>(u.value)];
      for ( usize arc_index = 0; arc_index < arcs.size(); ++arc_index ) {
        const residual_arc &arc = arcs.data()[arc_index];
        const usize v = static_cast<usize>(arc.target);
        if ( arc.capacity <= C{} || reached.data()[v] ) continue;
        reached.data()[v] = 1;
        parent.data()[v] = u.value;
        parent_arc.data()[v] = arc_index;
        queue.push_back(typename G::vertex_descriptor(arc.target));
        if ( arc.target == sink.value ) break;
      }
    }
    if ( !reached.data()[static_cast<usize>(sink.value)] ) {
      result.source_side = micron::move(reached);
      break;
    }

    C bottleneck{};
    bool have_bottleneck = false;
    I current = sink.value;
    while ( current != source.value ) {
      const I previous = parent.data()[static_cast<usize>(current)];
      const residual_arc &arc = residual.data()[static_cast<usize>(previous)].data()[parent_arc.data()[static_cast<usize>(current)]];
      if ( !have_bottleneck || arc.capacity < bottleneck ) {
        bottleneck = arc.capacity;
        have_bottleneck = true;
      }
      current = previous;
    }

    current = sink.value;
    while ( current != source.value ) {
      const I previous = parent.data()[static_cast<usize>(current)];
      residual_arc &arc = residual.data()[static_cast<usize>(previous)].data()[parent_arc.data()[static_cast<usize>(current)]];
      arc.capacity -= bottleneck;
      residual.data()[static_cast<usize>(current)].data()[arc.reverse].capacity += bottleneck;
      current = previous;
    }
    C total{};
    if ( !__impl::add_distance(result.value, bottleneck, total) ) {
      result.status = algorithm_status::overflow;
      return result;
    }
    result.value = total;
  }

  for ( auto edge : graph.edges() ) {
    const edge_location place = location.data()[static_cast<usize>(edge.id.value)];
    const C original = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    const C remaining = residual.data()[static_cast<usize>(place.source)].data()[place.arc].capacity;
    result.flow.data()[static_cast<usize>(edge.id.value)] = original - remaining;
  }
  return result;
}

template<graph_model G, typename CapacityMap>
[[nodiscard]] auto
push_relabel(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, CapacityMap capacity_map)
{
  using I = typename G::index_type;
  using C = micron::remove_cvref_t<decltype(__impl::weight(capacity_map, graph, micron::declval<typename G::edge_descriptor>()))>;

  struct residual_arc {
    I target{};
    usize reverse{};
    C capacity{};
  };

  struct edge_location {
    I source{ vertex_id<I>::invalid_value() };
    usize arc{};
  };

  max_flow_result<I, C> result{ algorithm_status::ok, C{}, micron::vector<C, micron::allocator_serial<>, false>(graph.edge_slots(), C{}),
                                micron::vector<u8, micron::allocator_serial<>, false>(graph.vertex_slots(), u8(0)) };
  if constexpr ( !G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  if ( !graph.has_vertex(source) || !graph.has_vertex(sink) || source == sink ) {
    result.status = algorithm_status::invalid_vertex;
    return result;
  }

  using arc_vector = micron::vector<residual_arc, micron::allocator_serial<>, false>;
  micron::vector<arc_vector, micron::allocator_serial<>, false> residual(graph.vertex_slots());
  micron::vector<edge_location, micron::allocator_serial<>, false> location(graph.edge_slots());
  for ( auto edge : graph.edges() ) {
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    if ( __impl::invalid_weight(capacity) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    const usize u = static_cast<usize>(edge.source.value);
    const usize v = static_cast<usize>(edge.target.value);
    const usize forward = residual.data()[u].size();
    const usize reverse = residual.data()[v].size();
    residual.data()[u].push_back(residual_arc{ edge.target.value, reverse, capacity });
    residual.data()[v].push_back(residual_arc{ edge.source.value, forward, C{} });
    location.data()[static_cast<usize>(edge.id.value)] = edge_location{ edge.source.value, forward };
  }

  const usize slots = graph.vertex_slots();
  micron::vector<usize, micron::allocator_serial<>, false> height(slots, usize(0));
  micron::vector<usize, micron::allocator_serial<>, false> current(slots, usize(0));
  micron::vector<C, micron::allocator_serial<>, false> excess(slots, C{});
  micron::vector<u8, micron::allocator_serial<>, false> queued(slots, u8(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> active;
  active.reserve(graph.edges_count() + graph.vertices_count());
  height.data()[static_cast<usize>(source.value)] = graph.vertices_count();

  auto enqueue = [&](I raw_vertex) {
    if ( raw_vertex == source.value || raw_vertex == sink.value ) return;
    const usize slot = static_cast<usize>(raw_vertex);
    if ( excess.data()[slot] <= C{} || queued.data()[slot] ) return;
    queued.data()[slot] = 1;
    active.push_back(typename G::vertex_descriptor(raw_vertex));
  };

  arc_vector &source_arcs = residual.data()[static_cast<usize>(source.value)];
  for ( usize arc_index = 0; arc_index < source_arcs.size(); ++arc_index ) {
    residual_arc &arc = source_arcs.data()[arc_index];
    if ( arc.capacity <= C{} ) continue;
    const C amount = arc.capacity;
    arc.capacity = C{};
    residual.data()[static_cast<usize>(arc.target)].data()[arc.reverse].capacity += amount;
    if ( arc.target != source.value ) {
      C total{};
      if ( !__impl::add_distance(excess.data()[static_cast<usize>(arc.target)], amount, total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      excess.data()[static_cast<usize>(arc.target)] = total;
      enqueue(arc.target);
    }
  }

  usize head = 0;
  while ( head < active.size() ) {
    const auto vertex = active.data()[head++];
    const usize u = static_cast<usize>(vertex.value);
    queued.data()[u] = 0;
    arc_vector &arcs = residual.data()[u];

    while ( excess.data()[u] > C{} ) {
      if ( current.data()[u] == arcs.size() ) {
        bool found = false;
        usize minimum_height = 0;
        for ( const residual_arc &arc : arcs ) {
          if ( arc.capacity <= C{} ) continue;
          const usize candidate = height.data()[static_cast<usize>(arc.target)];
          if ( !found || candidate < minimum_height ) {
            found = true;
            minimum_height = candidate;
          }
        }
        if ( !found ) {
          result.status = algorithm_status::invalid_graph;
          return result;
        }
        height.data()[u] = minimum_height + 1;
        current.data()[u] = 0;
        continue;
      }

      residual_arc &arc = arcs.data()[current.data()[u]];
      const usize v = static_cast<usize>(arc.target);
      if ( arc.capacity <= C{} || height.data()[u] != height.data()[v] + 1 ) {
        ++current.data()[u];
        continue;
      }

      const C amount = excess.data()[u] < arc.capacity ? excess.data()[u] : arc.capacity;
      const C previous_excess = excess.data()[v];
      arc.capacity -= amount;
      residual.data()[v].data()[arc.reverse].capacity += amount;
      excess.data()[u] -= amount;
      if ( arc.target != source.value ) {
        C total{};
        if ( !__impl::add_distance(excess.data()[v], amount, total) ) {
          result.status = algorithm_status::overflow;
          return result;
        }
        excess.data()[v] = total;
        if ( previous_excess == C{} ) enqueue(arc.target);
      }
    }
  }

  result.value = excess.data()[static_cast<usize>(sink.value)];
  for ( auto edge : graph.edges() ) {
    const edge_location place = location.data()[static_cast<usize>(edge.id.value)];
    const C original = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    const C remaining = residual.data()[static_cast<usize>(place.source)].data()[place.arc].capacity;
    result.flow.data()[static_cast<usize>(edge.id.value)] = original - remaining;
  }

  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());
  queue.push_back(source);
  result.source_side.data()[static_cast<usize>(source.value)] = 1;
  head = 0;
  while ( head < queue.size() ) {
    const auto u = queue.data()[head++];
    for ( const residual_arc &arc : residual.data()[static_cast<usize>(u.value)] ) {
      const usize v = static_cast<usize>(arc.target);
      if ( arc.capacity > C{} && !result.source_side.data()[v] ) {
        result.source_side.data()[v] = 1;
        queue.push_back(typename G::vertex_descriptor(arc.target));
      }
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
edmonds_karp(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink)
{
  return edmonds_karp(graph, source, sink, intrinsic_edge_weight{});
}

template<graph_model G>
[[nodiscard]] auto
push_relabel(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink)
{
  return push_relabel(graph, source, sink, intrinsic_edge_weight{});
}

template<graph_model G, typename CapacityMap = intrinsic_edge_weight>
[[nodiscard]] auto
boykov_kolmogorov(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink, CapacityMap capacity_map = {})
{
  return dinic(graph, source, sink, capacity_map);
}

template<graph_model G, typename CapacityMap>
[[nodiscard]] bool
verify_flow(
    const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor sink,
    const max_flow_result<typename G::index_type,
                          micron::remove_cvref_t<decltype(__impl::weight(micron::declval<CapacityMap &>(), micron::declval<const G &>(),
                                                                         micron::declval<typename G::edge_descriptor>()))>> &flow,
    CapacityMap capacity_map)
{
  using C = typename decltype(flow.flow)::value_type;
  if ( flow.status != algorithm_status::ok ) return false;
  micron::vector<C, micron::allocator_serial<>, false> incoming(graph.vertex_slots(), C{});
  micron::vector<C, micron::allocator_serial<>, false> outgoing(graph.vertex_slots(), C{});
  for ( auto edge : graph.edges() ) {
    const C value = flow.flow.data()[static_cast<usize>(edge.id.value)];
    const C capacity = static_cast<C>(__impl::weight(capacity_map, graph, edge.id));
    if ( value < C{} || capacity < value ) return false;
    outgoing.data()[static_cast<usize>(edge.source.value)] += value;
    incoming.data()[static_cast<usize>(edge.target.value)] += value;
  }
  for ( auto vertex : graph.vertices() ) {
    if ( vertex == source || vertex == sink ) continue;
    const usize slot = static_cast<usize>(vertex.value);
    if ( incoming.data()[slot] != outgoing.data()[slot] ) return false;
  }
  const usize source_slot = static_cast<usize>(source.value);
  const usize sink_slot = static_cast<usize>(sink.value);
  return outgoing.data()[source_slot] >= incoming.data()[source_slot]
         && outgoing.data()[source_slot] - incoming.data()[source_slot] == flow.value
         && incoming.data()[sink_slot] >= outgoing.data()[sink_slot]
         && incoming.data()[sink_slot] - outgoing.data()[sink_slot] == flow.value;
}

template<micron::integral I, typename Weight> struct global_cut_result {
  algorithm_status status{ algorithm_status::ok };
  Weight value{};
  micron::vector<u8, micron::allocator_serial<>, false> side;
};

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
stoer_wagner(const G &graph, WeightMap weight_map)
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  global_cut_result<I, W> result{ algorithm_status::ok, W{},
                                  micron::vector<u8, micron::allocator_serial<>, false>(graph.vertex_slots(), u8(0)) };
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  const usize n = graph.vertices_count();
  if ( n < 2 ) return result;
  auto mapping = __dense_vertex_mapping(graph);
  micron::vector<W, micron::allocator_serial<>, false> weights(n * n, W{});
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge.id));
    if ( __impl::invalid_weight(weight) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    weights.data()[u * n + v] += weight;
    weights.data()[v * n + u] += weight;
  }
  micron::vector<usize, micron::allocator_serial<>, false> vertices(n);
  micron::vector<micron::vector<usize, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> groups(n);
  for ( usize i = 0; i < n; ++i ) {
    vertices.data()[i] = i;
    groups.data()[i].push_back(i);
  }
  bool have_cut = false;
  usize active = n;
  while ( active > 1 ) {
    micron::vector<W, micron::allocator_serial<>, false> connectivity(n, W{});
    micron::vector<u8, micron::allocator_serial<>, false> added(n, u8(0));
    usize previous = vertices.data()[0];
    usize selected = previous;
    for ( usize phase = 0; phase < active; ++phase ) {
      selected = vertices.data()[0];
      bool found = false;
      for ( usize j = 0; j < active; ++j ) {
        const usize candidate = vertices.data()[j];
        if ( added.data()[candidate] ) continue;
        if ( !found || connectivity.data()[selected] < connectivity.data()[candidate] ) {
          selected = candidate;
          found = true;
        }
      }
      if ( phase + 1 == active ) {
        const W cut = connectivity.data()[selected];
        if ( !have_cut || cut < result.value ) {
          have_cut = true;
          result.value = cut;
          result.side.fill(u8(0));
          for ( usize dense : groups.data()[selected] )
            result.side.data()[static_cast<usize>(mapping.dense_to_vertex.data()[dense].value)] = 1;
        }
        for ( usize j = 0; j < n; ++j ) {
          weights.data()[previous * n + j] += weights.data()[selected * n + j];
          weights.data()[j * n + previous] = weights.data()[previous * n + j];
        }
        for ( usize dense : groups.data()[selected] ) groups.data()[previous].push_back(dense);
        for ( usize j = 0; j < active; ++j )
          if ( vertices.data()[j] == selected ) {
            vertices.data()[j] = vertices.data()[active - 1];
            break;
          }
        --active;
        break;
      }
      added.data()[selected] = 1;
      previous = selected;
      for ( usize j = 0; j < active; ++j ) {
        const usize candidate = vertices.data()[j];
        if ( !added.data()[candidate] ) connectivity.data()[candidate] += weights.data()[selected * n + candidate];
      }
    }
  }
  return result;
}

template<graph_model G>
[[nodiscard]] auto
stoer_wagner(const G &graph)
{
  return stoer_wagner(graph, intrinsic_edge_weight{});
}

template<graph_model G>
[[nodiscard]] usize
local_edge_connectivity(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target)
{
  if ( !graph.has_vertex(source) || !graph.has_vertex(target) || source == target ) return 0;
  auto mapping = __dense_vertex_mapping(graph);
  using network_type = weighted_digraph<usize, empty_property, empty_property, empty_property, typename G::index_type, parallel_t>;
  network_type network;
  (void)network.add_vertices(graph.vertices_count());
  for ( auto edge : graph.edges() ) {
    const auto u = mapping.vertex_to_dense.data()[static_cast<usize>(edge.source.value)];
    const auto v = mapping.vertex_to_dense.data()[static_cast<usize>(edge.target.value)];
    if ( u == v ) continue;
    (void)network.add_edge(u, v, usize(1));
    if constexpr ( !G::is_directed ) (void)network.add_edge(v, u, usize(1));
  }
  const auto dense_source = mapping.vertex_to_dense.data()[static_cast<usize>(source.value)];
  const auto dense_target = mapping.vertex_to_dense.data()[static_cast<usize>(target.value)];
  auto flow
      = dinic(network, typename network_type::vertex_descriptor(dense_source), typename network_type::vertex_descriptor(dense_target));
  return flow.status == algorithm_status::ok ? flow.value : 0;
}

template<graph_model G>
[[nodiscard]] usize
edge_connectivity(const G &graph)
{
  if ( graph.vertices_count() < 2 ) return 0;
  auto vertices = graph.vertices();
  const auto first = *vertices.begin();
  usize best = graph.edges_count();
  if constexpr ( G::is_directed ) {
    for ( auto source : graph.vertices() )
      for ( auto target : graph.vertices() )
        if ( source != target ) {
          const usize value = local_edge_connectivity(graph, source, target);
          if ( value < best ) best = value;
        }
  } else {
    for ( auto target : graph.vertices() )
      if ( target != first ) {
        const usize value = local_edge_connectivity(graph, first, target);
        if ( value < best ) best = value;
      }
  }
  return best;
}

template<graph_model G>
[[nodiscard]] usize
local_node_connectivity(const G &graph, typename G::vertex_descriptor source, typename G::vertex_descriptor target)
{
  if ( !graph.has_vertex(source) || !graph.has_vertex(target) || source == target ) return 0;
  auto mapping = __dense_vertex_mapping(graph);
  using network_type = weighted_digraph<usize, empty_property, empty_property, empty_property, usize, parallel_t>;
  network_type network;
  const usize count = graph.vertices_count();
  (void)network.add_vertices(count * 2);
  const usize dense_source = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(source.value)]);
  const usize dense_target = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(target.value)]);
  for ( usize vertex = 0; vertex < count; ++vertex )
    (void)network.add_edge(vertex * 2, vertex * 2 + 1, vertex == dense_source || vertex == dense_target ? count : usize(1));
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    if ( u == v ) continue;
    (void)network.add_edge(u * 2 + 1, v * 2, usize(1));
    if constexpr ( !G::is_directed ) (void)network.add_edge(v * 2 + 1, u * 2, usize(1));
  }
  auto flow = dinic(network, typename network_type::vertex_descriptor(dense_source * 2 + 1),
                    typename network_type::vertex_descriptor(dense_target * 2));
  return flow.status == algorithm_status::ok ? flow.value : 0;
}

template<graph_model G>
[[nodiscard]] usize
node_connectivity(const G &graph)
{
  if ( graph.vertices_count() < 2 ) return 0;
  auto vertices = graph.vertices();
  const auto first = *vertices.begin();
  usize best = graph.vertices_count() - 1;
  if constexpr ( G::is_directed ) {
    for ( auto source : graph.vertices() )
      for ( auto target : graph.vertices() )
        if ( source != target ) {
          const usize value = local_node_connectivity(graph, source, target);
          if ( value < best ) best = value;
        }
  } else {
    for ( auto target : graph.vertices() )
      if ( target != first ) {
        const usize value = local_node_connectivity(graph, first, target);
        if ( value < best ) best = value;
      }
  }
  return best;
}

};      // namespace micron::math::graphs
