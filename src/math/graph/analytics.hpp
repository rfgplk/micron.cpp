//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../vector/vector.hpp"
#include "paths.hpp"

namespace micron::math::graphs
{

template<graph_model G>
[[nodiscard]] f64
density(const G &graph) noexcept
{
  const usize n = graph.vertices_count();
  if ( n < 2 && !G::allows_loops ) return f64(0);
  usize possible{};
  if constexpr ( G::allows_loops )
    possible = G::is_directed ? n * n : n * (n + 1) / 2;
  else
    possible = G::is_directed ? n * (n - 1) : n * (n - 1) / 2;
  return possible ? static_cast<f64>(graph.edges_count()) / static_cast<f64>(possible) : f64(0);
}

template<graph_model G>
[[nodiscard]] micron::vector<usize, micron::allocator_serial<>, false>
degree_distribution(const G &graph)
{
  usize maximum = 0;
  for ( auto vertex : graph.vertices() )
    if ( graph.degree(vertex) > maximum ) maximum = graph.degree(vertex);
  micron::vector<usize, micron::allocator_serial<>, false> result(maximum + 1, usize(0));
  for ( auto vertex : graph.vertices() ) ++result.data()[graph.degree(vertex)];
  return result;
}

template<graph_model G>
[[nodiscard]] usize
common_neighbors(const G &graph, typename G::vertex_descriptor a, typename G::vertex_descriptor b)
{
  if ( !graph.has_vertex(a) || !graph.has_vertex(b) ) return 0;
  if constexpr ( bitset_neighbor_graph<G> ) {
    const auto left = graph.neighbor_words(a);
    const auto right = graph.neighbor_words(b);
    const usize words = left.words < right.words ? left.words : right.words;
    usize result = 0;
    for ( usize word = 0; word < words; ++word ) result += static_cast<usize>(__builtin_popcountll(left.data[word] & right.data[word]));
    return result;
  }
  usize result = 0;
  if constexpr ( matrix_adjacency_graph<G> ) {
    for ( auto neighbor : graph.out_neighbors(a) )
      if ( graph.matrix_has_edge(b, neighbor) || (!G::is_directed && graph.matrix_has_edge(neighbor, b)) ) ++result;
  } else {
    for ( auto neighbor : graph.out_neighbors(a) )
      if ( graph.has_edge(b, neighbor) || (!G::is_directed && graph.has_edge(neighbor, b)) ) ++result;
  }
  return result;
}

template<graph_model G>
[[nodiscard]] usize
triangles(const G &graph)
{
  usize count = 0;
  for ( auto u : graph.vertices() ) {
    for ( auto v : graph.out_neighbors(u) ) {
      if ( v.value <= u.value ) continue;
      for ( auto w : graph.out_neighbors(v) ) {
        if ( w.value <= v.value ) continue;
        if ( graph.has_edge(u, w) ) ++count;
      }
    }
  }
  return count;
}

template<graph_model G>
[[nodiscard]] f64
clustering(const G &graph, typename G::vertex_descriptor vertex)
{
  const usize degree = graph.out_degree(vertex);
  if ( degree < 2 ) return f64(0);
  usize links = 0;
  for ( auto a : graph.out_neighbors(vertex) )
    for ( auto b : graph.out_neighbors(vertex) )
      if ( a.value < b.value && (graph.has_edge(a, b) || graph.has_edge(b, a)) ) ++links;
  return (f64(2) * static_cast<f64>(links)) / static_cast<f64>(degree * (degree - 1));
}

template<graph_model G>
[[nodiscard]] micron::vector<f64, micron::allocator_serial<>, false>
clustering(const G &graph)
{
  micron::vector<f64, micron::allocator_serial<>, false> result(graph.vertex_slots(), f64(0));
  for ( auto vertex : graph.vertices() ) result.data()[static_cast<usize>(vertex.value)] = clustering(graph, vertex);
  return result;
}

template<graph_model G>
[[nodiscard]] f64
transitivity(const G &graph)
{
  usize triples = 0;
  for ( auto vertex : graph.vertices() ) {
    const usize degree = graph.out_degree(vertex);
    triples += degree * (degree - 1) / 2;
  }
  return triples ? f64(3) * static_cast<f64>(triangles(graph)) / static_cast<f64>(triples) : f64(0);
}

template<graph_model G>
[[nodiscard]] f64
reciprocity(const G &graph)
{
  if constexpr ( !G::is_directed ) return graph.edges_count() ? f64(1) : f64(0);
  if ( graph.edges_count() == 0 ) return f64(0);
  usize reciprocal = 0;
  for ( auto edge : graph.edges() )
    if ( graph.has_edge(edge.target, edge.source) ) ++reciprocal;
  return static_cast<f64>(reciprocal) / static_cast<f64>(graph.edges_count());
}

template<typename Score> struct iterative_scores_result {
  algorithm_status status{ algorithm_status::ok };
  usize iterations{};
  Score residual{};
  micron::vector<Score, micron::allocator_serial<>, false> score;
};

template<graph_model G>
[[nodiscard]] iterative_scores_result<f64>
pagerank(const G &graph, f64 damping = 0.85, f64 tolerance = 1e-10, usize max_iterations = 100)
{
  iterative_scores_result<f64> result;
  const usize n = graph.vertices_count();
  result.score = micron::vector<f64, micron::allocator_serial<>, false>(graph.vertex_slots(), f64(0));
  if ( n == 0 ) return result;
  const f64 initial = f64(1) / static_cast<f64>(n);
  for ( auto vertex : graph.vertices() ) result.score.data()[static_cast<usize>(vertex.value)] = initial;
  micron::vector<f64, micron::allocator_serial<>, false> next(graph.vertex_slots(), f64(0));
  for ( usize iteration = 0; iteration < max_iterations; ++iteration ) {
    f64 dangling = 0;
    for ( auto vertex : graph.vertices() )
      if ( graph.out_degree(vertex) == 0 ) dangling += result.score.data()[static_cast<usize>(vertex.value)];
    const f64 base = (f64(1) - damping) / static_cast<f64>(n) + damping * dangling / static_cast<f64>(n);
    next.fill(f64(0));
    for ( auto vertex : graph.vertices() ) next.data()[static_cast<usize>(vertex.value)] = base;
    for ( auto u : graph.vertices() ) {
      const usize degree = graph.out_degree(u);
      if ( degree == 0 ) continue;
      const f64 share = damping * result.score.data()[static_cast<usize>(u.value)] / static_cast<f64>(degree);
      for ( auto v : graph.out_neighbors(u) ) next.data()[static_cast<usize>(v.value)] += share;
    }
    f64 residual = 0;
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      f64 delta = next.data()[slot] - result.score.data()[slot];
      if ( delta < 0 ) delta = -delta;
      residual += delta;
      result.score.data()[slot] = next.data()[slot];
    }
    result.iterations = iteration + 1;
    result.residual = residual;
    if ( residual <= tolerance ) return result;
  }
  result.status = algorithm_status::non_convergent;
  return result;
}

template<typename Score> struct hits_result {
  algorithm_status status{ algorithm_status::ok };
  usize iterations{};
  Score residual{};
  micron::vector<Score, micron::allocator_serial<>, false> hubs;
  micron::vector<Score, micron::allocator_serial<>, false> authorities;
};

template<graph_model G>
[[nodiscard]] hits_result<f64>
hits(const G &graph, f64 tolerance = 1e-10, usize max_iterations = 100)
{
  hits_result<f64> result;
  const usize n = graph.vertices_count();
  result.hubs = micron::vector<f64, micron::allocator_serial<>, false>(graph.vertex_slots(), f64(0));
  result.authorities = micron::vector<f64, micron::allocator_serial<>, false>(graph.vertex_slots(), f64(0));
  if ( n == 0 ) return result;
  for ( auto vertex : graph.vertices() ) {
    result.hubs.data()[static_cast<usize>(vertex.value)] = f64(1);
    result.authorities.data()[static_cast<usize>(vertex.value)] = f64(1);
  }
  micron::vector<f64, micron::allocator_serial<>, false> next_hub(graph.vertex_slots(), f64(0));
  micron::vector<f64, micron::allocator_serial<>, false> next_auth(graph.vertex_slots(), f64(0));
  for ( usize iteration = 0; iteration < max_iterations; ++iteration ) {
    next_hub.fill(f64(0));
    next_auth.fill(f64(0));
    for ( auto edge : graph.edges() ) {
      next_auth.data()[static_cast<usize>(edge.target.value)] += result.hubs.data()[static_cast<usize>(edge.source.value)];
      next_hub.data()[static_cast<usize>(edge.source.value)] += result.authorities.data()[static_cast<usize>(edge.target.value)];
      if constexpr ( !G::is_directed ) {
        next_auth.data()[static_cast<usize>(edge.source.value)] += result.hubs.data()[static_cast<usize>(edge.target.value)];
        next_hub.data()[static_cast<usize>(edge.target.value)] += result.authorities.data()[static_cast<usize>(edge.source.value)];
      }
    }
    f64 hub_norm = 0;
    f64 auth_norm = 0;
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      hub_norm += next_hub.data()[slot] * next_hub.data()[slot];
      auth_norm += next_auth.data()[slot] * next_auth.data()[slot];
    }
    hub_norm = __builtin_sqrt(hub_norm);
    auth_norm = __builtin_sqrt(auth_norm);
    if ( hub_norm == 0 ) hub_norm = 1;
    if ( auth_norm == 0 ) auth_norm = 1;
    f64 residual = 0;
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      next_hub.data()[slot] /= hub_norm;
      next_auth.data()[slot] /= auth_norm;
      f64 dh = next_hub.data()[slot] - result.hubs.data()[slot];
      f64 da = next_auth.data()[slot] - result.authorities.data()[slot];
      if ( dh < 0 ) dh = -dh;
      if ( da < 0 ) da = -da;
      residual += dh + da;
      result.hubs.data()[slot] = next_hub.data()[slot];
      result.authorities.data()[slot] = next_auth.data()[slot];
    }
    result.iterations = iteration + 1;
    result.residual = residual;
    if ( residual <= tolerance ) return result;
  }
  result.status = algorithm_status::non_convergent;
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<f64, micron::allocator_serial<>, false>
degree_centrality(const G &graph)
{
  micron::vector<f64, micron::allocator_serial<>, false> result(graph.vertex_slots(), f64(0));
  const usize n = graph.vertices_count();
  if ( n < 2 ) return result;
  for ( auto vertex : graph.vertices() )
    result.data()[static_cast<usize>(vertex.value)] = static_cast<f64>(graph.degree(vertex)) / static_cast<f64>(n - 1);
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<f64, micron::allocator_serial<>, false>
closeness_centrality(const G &graph)
{
  micron::vector<f64, micron::allocator_serial<>, false> result(graph.vertex_slots(), f64(0));
  for ( auto source : graph.vertices() ) {
    auto paths = unweighted_shortest_paths(graph, source);
    usize reached = 0;
    usize sum = 0;
    for ( auto target : graph.vertices() ) {
      if ( target == source || !paths.contains(target) ) continue;
      ++reached;
      sum += paths.distance.data()[static_cast<usize>(target.value)];
    }
    if ( sum ) result.data()[static_cast<usize>(source.value)] = static_cast<f64>(reached) / static_cast<f64>(sum);
  }
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<f64, micron::allocator_serial<>, false>
harmonic_centrality(const G &graph)
{
  micron::vector<f64, micron::allocator_serial<>, false> result(graph.vertex_slots(), f64(0));
  for ( auto source : graph.vertices() ) {
    auto paths = unweighted_shortest_paths(graph, source);
    f64 sum = 0;
    for ( auto target : graph.vertices() ) {
      if ( target == source || !paths.contains(target) ) continue;
      const usize distance = paths.distance.data()[static_cast<usize>(target.value)];
      if ( distance ) sum += f64(1) / static_cast<f64>(distance);
    }
    result.data()[static_cast<usize>(source.value)] = sum;
  }
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<f64, micron::allocator_serial<>, false>
betweenness_centrality(const G &graph, bool normalized = true)
{
  using vertex_descriptor = typename G::vertex_descriptor;
  const usize slots = graph.vertex_slots();
  micron::vector<f64, micron::allocator_serial<>, false> centrality(slots, f64(0));
  using predecessor_vector = micron::vector<vertex_descriptor, micron::allocator_serial<>, false>;
  for ( auto source : graph.vertices() ) {
    micron::vector<predecessor_vector, micron::allocator_serial<>, false> predecessors(slots);
    micron::vector<f64, micron::allocator_serial<>, false> sigma(slots, f64(0));
    micron::vector<max_t, micron::allocator_serial<>, false> distance(slots, max_t(-1));
    micron::vector<vertex_descriptor, micron::allocator_serial<>, false> queue;
    micron::vector<vertex_descriptor, micron::allocator_serial<>, false> stack;
    sigma.data()[static_cast<usize>(source.value)] = f64(1);
    distance.data()[static_cast<usize>(source.value)] = 0;
    queue.push_back(source);
    usize head = 0;
    while ( head < queue.size() ) {
      const vertex_descriptor u = queue.data()[head++];
      stack.push_back(u);
      for ( auto v : graph.out_neighbors(u) ) {
        const usize vs = static_cast<usize>(v.value);
        if ( distance.data()[vs] < 0 ) {
          distance.data()[vs] = distance.data()[static_cast<usize>(u.value)] + 1;
          queue.push_back(v);
        }
        if ( distance.data()[vs] == distance.data()[static_cast<usize>(u.value)] + 1 ) {
          sigma.data()[vs] += sigma.data()[static_cast<usize>(u.value)];
          predecessors.data()[vs].push_back(u);
        }
      }
    }
    micron::vector<f64, micron::allocator_serial<>, false> dependency(slots, f64(0));
    while ( !stack.empty() ) {
      const vertex_descriptor w = stack.data()[stack.size() - 1];
      stack.pop_back();
      const usize ws = static_cast<usize>(w.value);
      for ( auto v : predecessors.data()[ws] ) {
        const usize vs = static_cast<usize>(v.value);
        if ( sigma.data()[ws] != 0 ) dependency.data()[vs] += sigma.data()[vs] / sigma.data()[ws] * (f64(1) + dependency.data()[ws]);
      }
      if ( w != source ) centrality.data()[ws] += dependency.data()[ws];
    }
  }
  if constexpr ( !G::is_directed )
    for ( auto vertex : graph.vertices() ) centrality.data()[static_cast<usize>(vertex.value)] *= f64(0.5);
  if ( normalized && graph.vertices_count() > 2 ) {
    const f64 scale = G::is_directed ? f64(1) / static_cast<f64>((graph.vertices_count() - 1) * (graph.vertices_count() - 2))
                                     : f64(2) / static_cast<f64>((graph.vertices_count() - 1) * (graph.vertices_count() - 2));
    for ( auto vertex : graph.vertices() ) centrality.data()[static_cast<usize>(vertex.value)] *= scale;
  }
  return centrality;
}

template<micron::integral I> struct distance_extrema_result {
  algorithm_status status{ algorithm_status::ok };
  usize radius{};
  usize diameter{};
  micron::vector<usize, micron::allocator_serial<>, false> eccentricity;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> centers;
};

template<graph_model G>
[[nodiscard]] distance_extrema_result<typename G::index_type>
distance_extrema(const G &graph)
{
  distance_extrema_result<typename G::index_type> result;
  result.eccentricity = micron::vector<usize, micron::allocator_serial<>, false>(graph.vertex_slots(), usize(0));
  bool first = true;
  for ( auto source : graph.vertices() ) {
    auto paths = unweighted_shortest_paths(graph, source);
    usize eccentricity = 0;
    for ( auto target : graph.vertices() ) {
      if ( !paths.contains(target) ) {
        result.status = algorithm_status::disconnected;
        return result;
      }
      const usize distance = paths.distance.data()[static_cast<usize>(target.value)];
      if ( distance > eccentricity ) eccentricity = distance;
    }
    result.eccentricity.data()[static_cast<usize>(source.value)] = eccentricity;
    if ( first || eccentricity < result.radius ) result.radius = eccentricity;
    if ( first || eccentricity > result.diameter ) result.diameter = eccentricity;
    first = false;
  }
  for ( auto vertex : graph.vertices() )
    if ( result.eccentricity.data()[static_cast<usize>(vertex.value)] == result.radius ) result.centers.push_back(vertex);
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<usize, micron::allocator_serial<>, false>
core_numbers(const G &graph)
{
  micron::vector<usize, micron::allocator_serial<>, false> core(graph.vertex_slots(), usize(0));
  micron::vector<usize, micron::allocator_serial<>, false> degree(graph.vertex_slots(), usize(0));
  micron::vector<u8, micron::allocator_serial<>, false> active(graph.vertex_slots(), u8(0));
  for ( auto vertex : graph.vertices() ) {
    const usize slot = static_cast<usize>(vertex.value);
    active.data()[slot] = 1;
    degree.data()[slot] = graph.out_degree(vertex);
  }
  usize current_core = 0;
  for ( usize removed = 0; removed < graph.vertices_count(); ++removed ) {
    using vertex_descriptor = typename G::vertex_descriptor;
    vertex_descriptor selected = vertex_descriptor::invalid();
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( !active.data()[slot] ) continue;
      if ( !selected.valid() || degree.data()[slot] < degree.data()[static_cast<usize>(selected.value)] ) selected = vertex;
    }
    if ( !selected.valid() ) break;
    const usize ss = static_cast<usize>(selected.value);
    if ( degree.data()[ss] > current_core ) current_core = degree.data()[ss];
    core.data()[ss] = current_core;
    active.data()[ss] = 0;
    for ( auto neighbor : graph.out_neighbors(selected) ) {
      usize &d = degree.data()[static_cast<usize>(neighbor.value)];
      if ( active.data()[static_cast<usize>(neighbor.value)] && d > current_core ) --d;
    }
  }
  return core;
}

template<graph_model G>
[[nodiscard]] f64
jaccard_coefficient(const G &graph, typename G::vertex_descriptor a, typename G::vertex_descriptor b)
{
  const usize intersection = common_neighbors(graph, a, b);
  const usize total = graph.out_degree(a) + graph.out_degree(b) - intersection;
  return total ? static_cast<f64>(intersection) / static_cast<f64>(total) : f64(0);
}

template<graph_model G>
[[nodiscard]] f64
global_efficiency(const G &graph)
{
  const usize n = graph.vertices_count();
  if ( n < 2 ) return f64(0);
  f64 sum = 0;
  for ( auto source : graph.vertices() ) {
    auto paths = unweighted_shortest_paths(graph, source);
    for ( auto target : graph.vertices() ) {
      if ( target == source || !paths.contains(target) ) continue;
      const usize distance = paths.distance.data()[static_cast<usize>(target.value)];
      if ( distance ) sum += f64(1) / static_cast<f64>(distance);
    }
  }
  return sum / static_cast<f64>(n * (n - 1));
}

template<graph_model G>
[[nodiscard]] f64
degree_assortativity(const G &graph)
{
  f64 sum_x = 0;
  f64 sum_y = 0;
  f64 sum_xx = 0;
  f64 sum_yy = 0;
  f64 sum_xy = 0;
  usize samples = 0;
  auto account = [&](f64 x, f64 y) {
    sum_x += x;
    sum_y += y;
    sum_xx += x * x;
    sum_yy += y * y;
    sum_xy += x * y;
    ++samples;
  };
  for ( auto edge : graph.edges() ) {
    const f64 source_degree = static_cast<f64>(G::is_directed ? graph.out_degree(edge.source) : graph.degree(edge.source));
    const f64 target_degree = static_cast<f64>(G::is_directed ? graph.in_degree(edge.target) : graph.degree(edge.target));
    account(source_degree, target_degree);
    if constexpr ( !G::is_directed ) account(target_degree, source_degree);
  }
  if ( samples == 0 ) return f64(0);
  const f64 count = static_cast<f64>(samples);
  const f64 covariance = count * sum_xy - sum_x * sum_y;
  const f64 variance_x = count * sum_xx - sum_x * sum_x;
  const f64 variance_y = count * sum_yy - sum_y * sum_y;
  const f64 denominator = __builtin_sqrt(variance_x * variance_y);
  return denominator == f64(0) ? f64(0) : covariance / denominator;
}

template<graph_model G>
[[nodiscard]] f64
average_clustering(const G &graph)
{
  if ( graph.vertices_count() == 0 ) return f64(0);
  f64 total = 0;
  for ( auto vertex : graph.vertices() ) total += clustering(graph, vertex);
  return total / static_cast<f64>(graph.vertices_count());
}

template<graph_model G>
[[nodiscard]] iterative_scores_result<f64>
eigenvector_centrality(const G &graph, f64 tolerance = 1e-10, usize max_iterations = 100)
{
  iterative_scores_result<f64> result;
  result.score = micron::vector<f64, micron::allocator_serial<>, false>(graph.vertex_slots(), f64(0));
  if ( graph.vertices_count() == 0 ) return result;
  const f64 initial = f64(1) / __builtin_sqrt(static_cast<f64>(graph.vertices_count()));
  for ( auto vertex : graph.vertices() ) result.score.data()[static_cast<usize>(vertex.value)] = initial;
  micron::vector<f64, micron::allocator_serial<>, false> next(graph.vertex_slots(), f64(0));
  for ( usize iteration = 0; iteration < max_iterations; ++iteration ) {
    next.fill(f64(0));
    for ( auto edge : graph.edges() ) {
      next.data()[static_cast<usize>(edge.target.value)] += result.score.data()[static_cast<usize>(edge.source.value)];
      if constexpr ( !G::is_directed )
        next.data()[static_cast<usize>(edge.source.value)] += result.score.data()[static_cast<usize>(edge.target.value)];
    }
    f64 norm = 0;
    for ( auto vertex : graph.vertices() ) {
      const f64 value = next.data()[static_cast<usize>(vertex.value)];
      norm += value * value;
    }
    norm = __builtin_sqrt(norm);
    if ( norm == f64(0) ) return result;
    f64 residual = 0;
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      next.data()[slot] /= norm;
      f64 difference = next.data()[slot] - result.score.data()[slot];
      if ( difference < 0 ) difference = -difference;
      residual += difference;
      result.score.data()[slot] = next.data()[slot];
    }
    result.iterations = iteration + 1;
    result.residual = residual;
    if ( residual <= tolerance ) return result;
  }
  result.status = algorithm_status::non_convergent;
  return result;
}

template<graph_model G>
[[nodiscard]] iterative_scores_result<f64>
katz_centrality(const G &graph, f64 alpha = 0.1, f64 beta = 1.0, f64 tolerance = 1e-10, usize max_iterations = 100)
{
  iterative_scores_result<f64> result;
  result.score = micron::vector<f64, micron::allocator_serial<>, false>(graph.vertex_slots(), f64(0));
  if ( __impl::nonfinite_weight(alpha) || __impl::nonfinite_weight(beta) || alpha < f64(0) ) {
    result.status = algorithm_status::invalid_weight;
    return result;
  }
  for ( auto vertex : graph.vertices() ) result.score.data()[static_cast<usize>(vertex.value)] = beta;
  micron::vector<f64, micron::allocator_serial<>, false> next(graph.vertex_slots(), f64(0));
  for ( usize iteration = 0; iteration < max_iterations; ++iteration ) {
    next.fill(f64(0));
    for ( auto vertex : graph.vertices() ) next.data()[static_cast<usize>(vertex.value)] = beta;
    for ( auto edge : graph.edges() ) {
      next.data()[static_cast<usize>(edge.target.value)] += alpha * result.score.data()[static_cast<usize>(edge.source.value)];
      if constexpr ( !G::is_directed )
        next.data()[static_cast<usize>(edge.source.value)] += alpha * result.score.data()[static_cast<usize>(edge.target.value)];
    }
    f64 residual = 0;
    for ( auto vertex : graph.vertices() ) {
      const usize slot = static_cast<usize>(vertex.value);
      if ( __impl::nonfinite_weight(next.data()[slot]) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      f64 difference = next.data()[slot] - result.score.data()[slot];
      if ( difference < 0 ) difference = -difference;
      residual += difference;
      result.score.data()[slot] = next.data()[slot];
    }
    result.iterations = iteration + 1;
    result.residual = residual;
    if ( residual <= tolerance ) return result;
  }
  result.status = algorithm_status::non_convergent;
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
k_core_vertices(const G &graph, usize k)
{
  auto core = core_numbers(graph);
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> result;
  for ( auto vertex : graph.vertices() )
    if ( core.data()[static_cast<usize>(vertex.value)] >= k ) result.push_back(vertex);
  return result;
}

template<graph_model G>
[[nodiscard]] micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false>
k_shell_vertices(const G &graph, usize k)
{
  auto core = core_numbers(graph);
  micron::vector<typename G::vertex_descriptor, micron::allocator_serial<>, false> result;
  for ( auto vertex : graph.vertices() )
    if ( core.data()[static_cast<usize>(vertex.value)] == k ) result.push_back(vertex);
  return result;
}

template<graph_model G>
[[nodiscard]] f64
rich_club_coefficient(const G &graph, usize degree_threshold)
{
  micron::vector<u8, micron::allocator_serial<>, false> selected(graph.vertex_slots(), u8(0));
  usize vertices = 0;
  for ( auto vertex : graph.vertices() )
    if ( graph.degree(vertex) > degree_threshold ) {
      selected.data()[static_cast<usize>(vertex.value)] = 1;
      ++vertices;
    }
  if ( vertices < 2 ) return f64(0);
  usize edges = 0;
  for ( auto edge : graph.edges() )
    if ( selected.data()[static_cast<usize>(edge.source.value)] && selected.data()[static_cast<usize>(edge.target.value)] ) ++edges;
  const usize possible = G::is_directed ? vertices * (vertices - 1) : vertices * (vertices - 1) / 2;
  return possible ? static_cast<f64>(edges) / static_cast<f64>(possible) : f64(0);
}

template<graph_model G>
[[nodiscard]] f64
adamic_adar_index(const G &graph, typename G::vertex_descriptor a, typename G::vertex_descriptor b)
{
  if ( !graph.has_vertex(a) || !graph.has_vertex(b) ) return f64(0);
  f64 result = 0;
  for ( auto neighbor : graph.out_neighbors(a) ) {
    if ( !graph.has_edge(b, neighbor) && !graph.has_edge(neighbor, b) ) continue;
    const usize degree = graph.degree(neighbor);
    if ( degree > 1 ) result += f64(1) / __builtin_log(static_cast<f64>(degree));
  }
  return result;
}

template<graph_model G>
[[nodiscard]] usize
preferential_attachment_score(const G &graph, typename G::vertex_descriptor a, typename G::vertex_descriptor b)
{
  return graph.has_vertex(a) && graph.has_vertex(b) ? graph.degree(a) * graph.degree(b) : 0;
}

template<graph_model G>
[[nodiscard]] f64
resource_allocation_index(const G &graph, typename G::vertex_descriptor a, typename G::vertex_descriptor b)
{
  if ( !graph.has_vertex(a) || !graph.has_vertex(b) ) return f64(0);
  f64 result = 0;
  for ( auto neighbor : graph.out_neighbors(a) ) {
    if ( !graph.has_edge(b, neighbor) && !graph.has_edge(neighbor, b) ) continue;
    const usize degree = graph.degree(neighbor);
    if ( degree ) result += f64(1) / static_cast<f64>(degree);
  }
  return result;
}

};      // namespace micron::math::graphs
