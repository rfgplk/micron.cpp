//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "graph.hpp"

namespace micron::math::graphs
{

template<micron::integral I> struct partition_result {
  algorithm_status status{ algorithm_status::ok };
  usize communities{};
  usize iterations{};
  micron::vector<I, micron::allocator_serial<>, false> community;
};

template<graph_model G>
[[nodiscard]] partition_result<typename G::index_type>
label_propagation(const G &graph, usize max_iterations = 100)
{
  using I = typename G::index_type;
  partition_result<I> result{ algorithm_status::ok, graph.vertices_count(), 0,
                              micron::vector<I, micron::allocator_serial<>, false>(graph.vertex_slots(), vertex_id<I>::invalid_value()) };
  for ( auto vertex : graph.vertices() ) result.community.data()[static_cast<usize>(vertex.value)] = vertex.value;
  for ( usize iteration = 0; iteration < max_iterations; ++iteration ) {
    bool changed = false;
    for ( auto vertex : graph.vertices() ) {
      I best = result.community.data()[static_cast<usize>(vertex.value)];
      usize best_count = 0;
      for ( auto candidate : graph.out_neighbors(vertex) ) {
        const I label = result.community.data()[static_cast<usize>(candidate.value)];
        usize count = 0;
        for ( auto neighbor : graph.out_neighbors(vertex) )
          if ( result.community.data()[static_cast<usize>(neighbor.value)] == label ) ++count;
        if constexpr ( G::is_directed )
          for ( auto neighbor : graph.in_neighbors(vertex) )
            if ( result.community.data()[static_cast<usize>(neighbor.value)] == label ) ++count;
        if ( count > best_count || (count == best_count && label < best) ) {
          best = label;
          best_count = count;
        }
      }
      if constexpr ( G::is_directed ) {
        for ( auto candidate : graph.in_neighbors(vertex) ) {
          const I label = result.community.data()[static_cast<usize>(candidate.value)];
          usize count = 0;
          for ( auto neighbor : graph.out_neighbors(vertex) )
            if ( result.community.data()[static_cast<usize>(neighbor.value)] == label ) ++count;
          for ( auto neighbor : graph.in_neighbors(vertex) )
            if ( result.community.data()[static_cast<usize>(neighbor.value)] == label ) ++count;
          if ( count > best_count || (count == best_count && label < best) ) {
            best = label;
            best_count = count;
          }
        }
      }
      I &current = result.community.data()[static_cast<usize>(vertex.value)];
      if ( current != best ) {
        current = best;
        changed = true;
      }
    }
    result.iterations = iteration + 1;
    if ( !changed ) break;
    if ( iteration + 1 == max_iterations ) result.status = algorithm_status::non_convergent;
  }
  // Canonicalize sparse labels to [0,k).
  micron::vector<I, micron::allocator_serial<>, false> labels;
  for ( auto vertex : graph.vertices() ) {
    const I label = result.community.data()[static_cast<usize>(vertex.value)];
    bool found = false;
    for ( I existing : labels )
      if ( existing == label ) {
        found = true;
        break;
      }
    if ( !found ) labels.push_back(label);
  }
  for ( auto vertex : graph.vertices() ) {
    I &label = result.community.data()[static_cast<usize>(vertex.value)];
    for ( usize i = 0; i < labels.size(); ++i )
      if ( labels.data()[i] == label ) {
        label = static_cast<I>(i);
        break;
      }
  }
  result.communities = labels.size();
  return result;
}

template<graph_model G, typename Partition>
[[nodiscard]] f64
modularity(const G &graph, const Partition &community, f64 resolution = 1.0)
{
  const f64 m = static_cast<f64>(graph.edges_count());
  if ( m == 0 ) return f64(0);
  f64 score = 0;
  for ( auto u : graph.vertices() ) {
    for ( auto v : graph.vertices() ) {
      if ( community[static_cast<usize>(u.value)] != community[static_cast<usize>(v.value)] ) continue;
      const f64 adjacency = graph.has_edge(u, v) ? f64(1) : f64(0);
      if constexpr ( G::is_directed )
        score += adjacency - resolution * static_cast<f64>(graph.out_degree(u) * graph.in_degree(v)) / m;
      else
        score += adjacency - resolution * static_cast<f64>(graph.degree(u) * graph.degree(v)) / (f64(2) * m);
    }
  }
  return score / (G::is_directed ? m : f64(2) * m);
}

template<graph_model G>
[[nodiscard]] auto
louvain(const G &graph, usize max_iterations = 100)
{
  using I = typename G::index_type;
  partition_result<I> result{ algorithm_status::ok, graph.vertices_count(), 0,
                              micron::vector<I, micron::allocator_serial<>, false>(graph.vertex_slots(), vertex_id<I>::invalid_value()) };
  for ( auto vertex : graph.vertices() ) result.community.data()[static_cast<usize>(vertex.value)] = vertex.value;
  if ( graph.edges_count() == 0 ) return result;
  const f64 normalization = static_cast<f64>(graph.edges_count());
  micron::vector<f64, micron::allocator_serial<>, false> out_total(graph.vertex_slots(), f64(0));
  micron::vector<f64, micron::allocator_serial<>, false> in_total(graph.vertex_slots(), f64(0));
  for ( auto vertex : graph.vertices() ) {
    const usize slot = static_cast<usize>(vertex.value);
    out_total.data()[slot] = static_cast<f64>(G::is_directed ? graph.out_degree(vertex) : graph.degree(vertex));
    in_total.data()[slot] = static_cast<f64>(G::is_directed ? graph.in_degree(vertex) : graph.degree(vertex));
  }

  for ( usize iteration = 0; iteration < max_iterations; ++iteration ) {
    bool changed = false;
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      const I original = result.community.data()[slot];
      micron::vector<I, micron::allocator_serial<>, false> candidates;
      micron::vector<f64, micron::allocator_serial<>, false> outgoing;
      micron::vector<f64, micron::allocator_serial<>, false> incoming;
      candidates.push_back(original);
      outgoing.push_back(0);
      incoming.push_back(0);
      auto consider = [&](auto neighbor, bool is_outgoing) {
        const I label = result.community.data()[static_cast<usize>(neighbor.value)];
        usize index = 0;
        while ( index < candidates.size() && candidates.data()[index] != label ) ++index;
        if ( index == candidates.size() ) {
          candidates.push_back(label);
          outgoing.push_back(0);
          incoming.push_back(0);
        }
        if ( is_outgoing )
          outgoing.data()[index] += 1;
        else
          incoming.data()[index] += 1;
      };
      for ( auto neighbor : graph.out_neighbors(vertex) ) consider(neighbor, true);
      if constexpr ( G::is_directed ) {
        for ( auto neighbor : graph.in_neighbors(vertex) ) consider(neighbor, false);
      } else {
        for ( usize i = 0; i < outgoing.size(); ++i ) incoming.data()[i] = outgoing.data()[i];
      }

      const f64 out_degree = static_cast<f64>(graph.out_degree(vertex));
      const f64 in_degree = static_cast<f64>(G::is_directed ? graph.in_degree(vertex) : graph.degree(vertex));
      out_total.data()[static_cast<usize>(original)] -= G::is_directed ? out_degree : in_degree;
      in_total.data()[static_cast<usize>(original)] -= in_degree;

      I best = original;
      f64 best_gain = -1.0;
      bool have_gain = false;
      for ( usize i = 0; i < candidates.size(); ++i ) {
        const I candidate = candidates.data()[i];
        f64 gain{};
        if constexpr ( G::is_directed ) {
          gain = (outgoing.data()[i] + incoming.data()[i]) / normalization
                 - (out_degree * in_total.data()[static_cast<usize>(candidate)]
                    + in_degree * out_total.data()[static_cast<usize>(candidate)])
                       / (normalization * normalization);
        } else {
          gain = outgoing.data()[i] / normalization
                 - in_degree * in_total.data()[static_cast<usize>(candidate)] / (f64(2) * normalization * normalization);
        }
        if ( !have_gain || gain > best_gain + 1e-12 || (gain >= best_gain - 1e-12 && candidate < best) ) {
          have_gain = true;
          best = candidate;
          best_gain = gain;
        }
      }
      result.community.data()[slot] = best;
      out_total.data()[static_cast<usize>(best)] += G::is_directed ? out_degree : in_degree;
      in_total.data()[static_cast<usize>(best)] += in_degree;
      if ( best != original ) changed = true;
    }
    result.iterations = iteration + 1;
    if ( !changed ) break;
    if ( iteration + 1 == max_iterations ) result.status = algorithm_status::non_convergent;
  }

  micron::vector<I, micron::allocator_serial<>, false> labels;
  for ( auto vertex : graph.vertices() ) {
    const I label = result.community.data()[static_cast<usize>(vertex.value)];
    bool found = false;
    for ( I existing : labels )
      if ( existing == label ) {
        found = true;
        break;
      }
    if ( !found ) labels.push_back(label);
  }
  for ( auto vertex : graph.vertices() ) {
    I &label = result.community.data()[static_cast<usize>(vertex.value)];
    for ( usize i = 0; i < labels.size(); ++i )
      if ( labels.data()[i] == label ) {
        label = static_cast<I>(i);
        break;
      }
  }
  result.communities = labels.size();
  return result;
}

template<graph_model G>
[[nodiscard]] auto
leiden(const G &graph, usize max_iterations = 100)
{
  using I = typename G::index_type;
  auto result = louvain(graph, max_iterations);
  micron::vector<I, micron::allocator_serial<>, false> refined(graph.vertex_slots(), vertex_id<I>::invalid_value());
  micron::vector<u8, micron::allocator_serial<>, false> seen(graph.vertex_slots(), u8(0));
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> queue;
  queue.reserve(graph.vertices_count());
  I next_label = 0;
  for ( auto root : graph.vertices() ) {
    const usize root_slot = static_cast<usize>(root.value);
    if ( seen.data()[root_slot] ) continue;
    const I old_label = result.community.data()[root_slot];
    queue.clear();
    queue.push_back(root);
    seen.data()[root_slot] = 1;
    refined.data()[root_slot] = next_label;
    usize head = 0;
    while ( head < queue.size() ) {
      const auto vertex = queue.data()[head++];
      auto inspect = [&](auto neighbor) {
        const usize slot = static_cast<usize>(neighbor.value);
        if ( seen.data()[slot] || result.community.data()[slot] != old_label ) return;
        seen.data()[slot] = 1;
        refined.data()[slot] = next_label;
        queue.push_back(neighbor);
      };
      for ( auto neighbor : graph.out_neighbors(vertex) ) inspect(neighbor);
      if constexpr ( G::is_directed )
        for ( auto neighbor : graph.in_neighbors(vertex) ) inspect(neighbor);
    }
    ++next_label;
  }
  result.community = micron::move(refined);
  result.communities = static_cast<usize>(next_label);
  return result;
}

template<graph_model G>
[[nodiscard]] auto
greedy_modularity_communities(const G &graph, usize max_iterations = 100)
{
  using I = typename G::index_type;
  partition_result<I> result{ algorithm_status::ok, graph.vertices_count(), 0,
                              micron::vector<I, micron::allocator_serial<>, false>(graph.vertex_slots(), vertex_id<I>::invalid_value()) };
  for ( auto vertex : graph.vertices() ) result.community.data()[static_cast<usize>(vertex.value)] = vertex.value;
  if ( graph.edges_count() == 0 ) return result;
  const f64 normalization = static_cast<f64>(graph.edges_count());

  for ( usize iteration = 0; iteration < max_iterations && result.communities > 1; ++iteration ) {
    micron::vector<I, micron::allocator_serial<>, false> labels;
    for ( auto vertex : graph.vertices() ) {
      const I label = result.community.data()[static_cast<usize>(vertex.value)];
      bool found = false;
      for ( I existing : labels )
        if ( existing == label ) {
          found = true;
          break;
        }
      if ( !found ) labels.push_back(label);
    }

    const usize count = labels.size();
    micron::vector<usize, micron::allocator_serial<>, false> label_index(graph.vertex_slots(), usize(-1));
    for ( usize i = 0; i < count; ++i ) label_index.data()[static_cast<usize>(labels.data()[i])] = i;
    micron::vector<f64, micron::allocator_serial<>, false> outgoing(count, f64(0));
    micron::vector<f64, micron::allocator_serial<>, false> incoming(count, f64(0));
    micron::vector<f64, micron::allocator_serial<>, false> between(count * count, f64(0));
    for ( auto vertex : graph.vertices() ) {
      const usize community = label_index.data()[static_cast<usize>(result.community.data()[static_cast<usize>(vertex.value)])];
      outgoing.data()[community] += static_cast<f64>(G::is_directed ? graph.out_degree(vertex) : graph.degree(vertex));
      incoming.data()[community] += static_cast<f64>(G::is_directed ? graph.in_degree(vertex) : graph.degree(vertex));
    }
    for ( auto edge : graph.edges() ) {
      const usize a = label_index.data()[static_cast<usize>(result.community.data()[static_cast<usize>(edge.source.value)])];
      const usize b = label_index.data()[static_cast<usize>(result.community.data()[static_cast<usize>(edge.target.value)])];
      if ( a == b ) continue;
      between.data()[a * count + b] += 1;
      if constexpr ( !G::is_directed ) between.data()[b * count + a] += 1;
    }

    bool improved = false;
    I best_a{};
    I best_b{};
    f64 best_delta = 0;
    for ( usize a = 0; a < count; ++a ) {
      for ( usize b = a + 1; b < count; ++b ) {
        f64 delta{};
        if constexpr ( G::is_directed ) {
          delta = (between.data()[a * count + b] + between.data()[b * count + a]) / normalization
                  - (outgoing.data()[a] * incoming.data()[b] + outgoing.data()[b] * incoming.data()[a]) / (normalization * normalization);
        } else {
          delta = between.data()[a * count + b] / normalization
                  - outgoing.data()[a] * outgoing.data()[b] / (f64(2) * normalization * normalization);
        }
        if ( delta > best_delta + 1e-12 ) {
          improved = true;
          best_delta = delta;
          best_a = labels.data()[a];
          best_b = labels.data()[b];
        }
      }
    }
    if ( !improved ) break;
    for ( auto vertex : graph.vertices() ) {
      I &label = result.community.data()[static_cast<usize>(vertex.value)];
      if ( label == best_b ) label = best_a;
    }
    --result.communities;
    result.iterations = iteration + 1;
    if ( iteration + 1 == max_iterations && result.communities > 1 ) result.status = algorithm_status::non_convergent;
  }
  micron::vector<I, micron::allocator_serial<>, false> labels;
  for ( auto vertex : graph.vertices() ) {
    const I label = result.community.data()[static_cast<usize>(vertex.value)];
    bool found = false;
    for ( I existing : labels )
      if ( existing == label ) {
        found = true;
        break;
      }
    if ( !found ) labels.push_back(label);
  }
  for ( auto vertex : graph.vertices() ) {
    I &label = result.community.data()[static_cast<usize>(vertex.value)];
    for ( usize i = 0; i < labels.size(); ++i )
      if ( labels.data()[i] == label ) {
        label = static_cast<I>(i);
        break;
      }
  }
  result.communities = labels.size();
  return result;
}

template<graph_model G, typename Partition>
[[nodiscard]] f64
partition_coverage(const G &graph, const Partition &community)
{
  if ( graph.edges_count() == 0 ) return f64(0);
  usize internal = 0;
  for ( auto edge : graph.edges() )
    if ( community[static_cast<usize>(edge.source.value)] == community[static_cast<usize>(edge.target.value)] ) ++internal;
  return static_cast<f64>(internal) / static_cast<f64>(graph.edges_count());
}

};      // namespace micron::math::graphs
