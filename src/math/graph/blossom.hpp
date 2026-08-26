//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "matching.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// blossom alg for constructing maximum matchings

namespace micron::math::graphs
{

enum class matching_objective : u8 { maximum_weight = 0, maximum_cardinality_then_weight };

struct unit_edge_weight {
  template<graph_model G>
  [[nodiscard]] constexpr i32
  operator()(const G &, typename G::edge_descriptor) const noexcept
  {
    return 1;
  }
};

template<micron::integral I, typename Weight> struct weighted_matching_result {
  algorithm_status status{ algorithm_status::ok };
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> mate;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> mate_edge;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
  usize cardinality{};
  Weight total_weight{};
  usize vertices{};

  [[nodiscard]] bool
  perfect() const noexcept
  {
    return cardinality * 2 == vertices;
  }
};

template<micron::integral I, typename Weight> struct weighted_matching_workspace {
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
  micron::vector<I, micron::allocator_serial<>, false> vertex_to_dense;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> chosen;
  micron::vector<Weight, micron::allocator_serial<>, false> chosen_weight;

  void
  reserve(usize vertex_slots, usize vertices)
  {
    dense_to_vertex.reserve(vertices);
    vertex_to_dense.reserve(vertex_slots);
    chosen.reserve(vertices * vertices);
    chosen_weight.reserve(vertices * vertices);
  }
};

namespace __impl
{

template<typename Weight> struct blossom_score {
  using number_type = micron::conditional_t<micron::is_integral_v<Weight>, int128_t, long double>;
  int128_t cardinal{};
  number_type weight{};

  friend constexpr bool operator==(const blossom_score &, const blossom_score &) noexcept = default;

  friend constexpr bool
  operator<(const blossom_score &a, const blossom_score &b) noexcept
  {
    return a.cardinal < b.cardinal || (a.cardinal == b.cardinal && a.weight < b.weight);
  }

  friend constexpr bool
  operator>(const blossom_score &a, const blossom_score &b) noexcept
  {
    return b < a;
  }

  friend constexpr bool
  operator<=(const blossom_score &a, const blossom_score &b) noexcept
  {
    return !(b < a);
  }

  friend constexpr blossom_score
  operator+(const blossom_score &a, const blossom_score &b) noexcept
  {
    return { a.cardinal + b.cardinal, a.weight + b.weight };
  }

  friend constexpr blossom_score
  operator-(const blossom_score &a, const blossom_score &b) noexcept
  {
    return { a.cardinal - b.cardinal, a.weight - b.weight };
  }

  friend constexpr blossom_score
  operator*(const blossom_score &a, int value) noexcept
  {
    return { a.cardinal * value, a.weight * static_cast<number_type>(value) };
  }

  friend constexpr blossom_score
  operator/(const blossom_score &a, int value) noexcept
  {
    return { a.cardinal / value, a.weight / static_cast<number_type>(value) };
  }

  blossom_score &
  operator+=(const blossom_score &other) noexcept
  {
    return *this = *this + other;
  }

  blossom_score &
  operator-=(const blossom_score &other) noexcept
  {
    return *this = *this - other;
  }
};

template<typename Weight> class weighted_blossom_engine
{
  using score = blossom_score<Weight>;

  struct blossom_edge {
    int source{};
    int target{};
    score weight{};
  };

  usize __n{};
  usize __nodes{};
  usize __stride{};
  micron::vector<blossom_edge, micron::allocator_serial<>, false> __edges;
  micron::vector<score, micron::allocator_serial<>, false> __label;
  micron::vector<int, micron::allocator_serial<>, false> __match;
  micron::vector<int, micron::allocator_serial<>, false> __slack;
  micron::vector<int, micron::allocator_serial<>, false> __base;
  micron::vector<int, micron::allocator_serial<>, false> __parent;
  micron::vector<int, micron::allocator_serial<>, false> __state;
  micron::vector<usize, micron::allocator_serial<>, false> __visited;
  micron::vector<micron::vector<int, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> __flower;
  micron::vector<int, micron::allocator_serial<>, false> __flower_from;
  micron::vector<int, micron::allocator_serial<>, false> __queue;
  usize __queue_head{};
  usize __visit_token{};

  [[nodiscard]] blossom_edge &
  g(usize u, usize v) noexcept
  {
    return __edges.data()[u * __stride + v];
  }

  [[nodiscard]] const blossom_edge &
  g(usize u, usize v) const noexcept
  {
    return __edges.data()[u * __stride + v];
  }

  [[nodiscard]] int &
  from(usize u, usize v) noexcept
  {
    return __flower_from.data()[u * __stride + v];
  }

  [[nodiscard]] int
  from(usize u, usize v) const noexcept
  {
    return __flower_from.data()[u * __stride + v];
  }

  [[nodiscard]] static constexpr score
  zero() noexcept
  {
    return {};
  }

  [[nodiscard]] bool
  edge_present(usize u, usize v) const noexcept
  {
    return zero() < g(u, v).weight;
  }

  [[nodiscard]] score
  delta(const blossom_edge &edge) const noexcept
  {
    return __label.data()[edge.source] + __label.data()[edge.target] - edge.weight * 2;
  }

  void
  update_slack(int u, int x)
  {
    if ( __slack.data()[x] == 0
         || delta(g(static_cast<usize>(u), static_cast<usize>(x)))
                < delta(g(static_cast<usize>(__slack.data()[x]), static_cast<usize>(x))) )
      __slack.data()[x] = u;
  }

  void
  set_slack(int x)
  {
    __slack.data()[x] = 0;
    for ( usize u = 1; u <= __n; ++u )
      if ( edge_present(u, static_cast<usize>(x)) && __base.data()[u] != x && __state.data()[__base.data()[u]] == 0 )
        update_slack(static_cast<int>(u), x);
  }

  void
  queue_push(int x)
  {
    if ( x <= static_cast<int>(__n) ) {
      __queue.push_back(x);
      return;
    }
    for ( int vertex : __flower.data()[x] ) queue_push(vertex);
  }

  void
  set_base(int x, int base)
  {
    __base.data()[x] = base;
    if ( x > static_cast<int>(__n) )
      for ( int vertex : __flower.data()[x] ) set_base(vertex, base);
  }

  int
  flower_position(int blossom, int root)
  {
    auto &cycle = __flower.data()[blossom];
    usize position = 0;
    while ( position < cycle.size() && cycle.data()[position] != root ) ++position;
    if ( position == cycle.size() ) return 0;
    if ( position & 1u ) {
      for ( usize left = 1, right = cycle.size() - 1; left < right; ++left, --right ) micron::swap(cycle.data()[left], cycle.data()[right]);
      return static_cast<int>(cycle.size() - position);
    }
    return static_cast<int>(position);
  }

  void
  rotate_flower(int blossom, usize position)
  {
    auto &cycle = __flower.data()[blossom];
    if ( position == 0 || position >= cycle.size() ) return;
    micron::vector<int, micron::allocator_serial<>, false> rotated;
    rotated.reserve(cycle.size());
    for ( usize i = position; i < cycle.size(); ++i ) rotated.push_back(cycle.data()[i]);
    for ( usize i = 0; i < position; ++i ) rotated.push_back(cycle.data()[i]);
    cycle = micron::move(rotated);
  }

  void
  set_match(int u, int v)
  {
    const auto edge = g(static_cast<usize>(u), static_cast<usize>(v));
    __match.data()[u] = edge_present(static_cast<usize>(u), static_cast<usize>(v)) ? edge.target : 0;
    if ( u <= static_cast<int>(__n) ) return;
    const int root = from(static_cast<usize>(u), static_cast<usize>(edge.source));
    const int position = flower_position(u, root);
    for ( int i = 0; i < position; ++i )
      set_match(__flower.data()[u].data()[static_cast<usize>(i)], __flower.data()[u].data()[static_cast<usize>(i ^ 1)]);
    set_match(root, v);
    rotate_flower(u, static_cast<usize>(position));
  }

  void
  augment(int u, int v)
  {
    for ( ;; ) {
      const int next = __base.data()[__match.data()[u]];
      set_match(u, v);
      if ( next == 0 ) return;
      set_match(next, __base.data()[__parent.data()[next]]);
      u = __base.data()[__parent.data()[next]];
      v = next;
    }
  }

  int
  lca(int u, int v)
  {
    ++__visit_token;
    while ( u != 0 || v != 0 ) {
      if ( u != 0 ) {
        if ( __visited.data()[u] == __visit_token ) return u;
        __visited.data()[u] = __visit_token;
        u = __base.data()[__match.data()[u]];
        if ( u != 0 ) u = __base.data()[__parent.data()[u]];
      }
      micron::swap(u, v);
    }
    return 0;
  }

  void
  add_blossom(int u, int ancestor, int v)
  {
    int blossom = static_cast<int>(__n + 1);
    while ( blossom <= static_cast<int>(__nodes) && __base.data()[blossom] != 0 ) ++blossom;
    if ( blossom > static_cast<int>(__nodes) ) __nodes = static_cast<usize>(blossom);
    __label.data()[blossom] = zero();
    __state.data()[blossom] = 0;
    __match.data()[blossom] = __match.data()[ancestor];
    __flower.data()[blossom].clear();
    __flower.data()[blossom].push_back(ancestor);
    for ( int x = u, y = 0; x != ancestor; x = __base.data()[__parent.data()[y]] ) {
      __flower.data()[blossom].push_back(x);
      y = __base.data()[__match.data()[x]];
      __flower.data()[blossom].push_back(y);
      queue_push(y);
    }
    auto &cycle = __flower.data()[blossom];
    for ( usize left = 1, right = cycle.size() - 1; left < right; ++left, --right ) micron::swap(cycle.data()[left], cycle.data()[right]);
    for ( int x = v, y = 0; x != ancestor; x = __base.data()[__parent.data()[y]] ) {
      cycle.push_back(x);
      y = __base.data()[__match.data()[x]];
      cycle.push_back(y);
      queue_push(y);
    }
    set_base(blossom, blossom);
    for ( usize x = 1; x < __stride; ++x ) {
      g(static_cast<usize>(blossom), x) = { blossom, static_cast<int>(x), zero() };
      g(x, static_cast<usize>(blossom)) = { static_cast<int>(x), blossom, zero() };
      from(static_cast<usize>(blossom), x) = 0;
    }
    for ( int part : cycle ) {
      for ( usize x = 1; x <= __nodes; ++x )
        if ( !edge_present(static_cast<usize>(blossom), x)
             || delta(g(static_cast<usize>(part), x)) < delta(g(static_cast<usize>(blossom), x)) ) {
          g(static_cast<usize>(blossom), x) = g(static_cast<usize>(part), x);
          g(x, static_cast<usize>(blossom)) = g(x, static_cast<usize>(part));
        }
      for ( usize x = 1; x <= __n; ++x )
        if ( from(static_cast<usize>(part), x) != 0 ) from(static_cast<usize>(blossom), x) = part;
    }
    set_slack(blossom);
  }

  void
  expand_blossom(int blossom)
  {
    for ( int part : __flower.data()[blossom] ) set_base(part, part);
    const auto parent_edge = g(static_cast<usize>(blossom), static_cast<usize>(__parent.data()[blossom]));
    const int root = from(static_cast<usize>(blossom), static_cast<usize>(parent_edge.source));
    const int position = flower_position(blossom, root);
    for ( int i = 0; i < position; i += 2 ) {
      const int outer = __flower.data()[blossom].data()[static_cast<usize>(i)];
      const int inner = __flower.data()[blossom].data()[static_cast<usize>(i + 1)];
      __parent.data()[outer] = g(static_cast<usize>(inner), static_cast<usize>(outer)).source;
      __state.data()[outer] = 1;
      __state.data()[inner] = 0;
      __slack.data()[outer] = 0;
      set_slack(inner);
      queue_push(inner);
    }
    __state.data()[root] = 1;
    __parent.data()[root] = __parent.data()[blossom];
    for ( usize i = static_cast<usize>(position + 1); i < __flower.data()[blossom].size(); ++i ) {
      const int part = __flower.data()[blossom].data()[i];
      __state.data()[part] = -1;
      set_slack(part);
    }
    __base.data()[blossom] = 0;
  }

  bool
  found_edge(const blossom_edge &edge)
  {
    const int u = __base.data()[edge.source];
    const int v = __base.data()[edge.target];
    if ( __state.data()[v] == -1 ) {
      __parent.data()[v] = edge.source;
      __state.data()[v] = 1;
      const int next = __base.data()[__match.data()[v]];
      __slack.data()[v] = 0;
      __slack.data()[next] = 0;
      __state.data()[next] = 0;
      queue_push(next);
    } else if ( __state.data()[v] == 0 ) {
      const int ancestor = lca(u, v);
      if ( ancestor == 0 ) {
        augment(u, v);
        augment(v, u);
        return true;
      }
      add_blossom(u, ancestor, v);
    }
    return false;
  }

  bool
  matching_step()
  {
    __state.fill(-1);
    __slack.fill(0);
    __queue.clear();
    __queue_head = 0;
    for ( usize x = 1; x <= __nodes; ++x )
      if ( __base.data()[x] == static_cast<int>(x) && __match.data()[x] == 0 ) {
        __parent.data()[x] = 0;
        __state.data()[x] = 0;
        queue_push(static_cast<int>(x));
      }
    if ( __queue.empty() ) return false;
    for ( ;; ) {
      while ( __queue_head < __queue.size() ) {
        const int u = __queue.data()[__queue_head++];
        if ( __state.data()[__base.data()[u]] == 1 ) continue;
        for ( usize v = 1; v <= __n; ++v ) {
          if ( !edge_present(static_cast<usize>(u), v) || __base.data()[u] == __base.data()[v] ) continue;
          if ( delta(g(static_cast<usize>(u), v)) == zero() ) {
            if ( found_edge(g(static_cast<usize>(u), v)) ) return true;
          } else {
            update_slack(u, __base.data()[v]);
          }
        }
      }

      score adjustment{};
      bool have_adjustment = false;
      auto consider = [&](const score &candidate) {
        if ( !have_adjustment || candidate < adjustment ) {
          adjustment = candidate;
          have_adjustment = true;
        }
      };
      for ( usize blossom = __n + 1; blossom <= __nodes; ++blossom )
        if ( __base.data()[blossom] == static_cast<int>(blossom) && __state.data()[blossom] == 1 ) consider(__label.data()[blossom] / 2);
      for ( usize x = 1; x <= __nodes; ++x )
        if ( __base.data()[x] == static_cast<int>(x) && __slack.data()[x] != 0 ) {
          const score candidate = delta(g(static_cast<usize>(__slack.data()[x]), x));
          if ( __state.data()[x] == -1 )
            consider(candidate);
          else if ( __state.data()[x] == 0 )
            consider(candidate / 2);
        }
      if ( !have_adjustment ) return false;
      for ( usize u = 1; u <= __n; ++u ) {
        if ( __state.data()[__base.data()[u]] == 0 ) {
          if ( __label.data()[u] <= adjustment ) return false;
          __label.data()[u] -= adjustment;
        } else if ( __state.data()[__base.data()[u]] == 1 ) {
          __label.data()[u] += adjustment;
        }
      }
      for ( usize blossom = __n + 1; blossom <= __nodes; ++blossom )
        if ( __base.data()[blossom] == static_cast<int>(blossom) ) {
          if ( __state.data()[blossom] == 0 )
            __label.data()[blossom] += adjustment * 2;
          else if ( __state.data()[blossom] == 1 )
            __label.data()[blossom] -= adjustment * 2;
        }
      __queue.clear();
      __queue_head = 0;
      for ( usize x = 1; x <= __nodes; ++x )
        if ( __base.data()[x] == static_cast<int>(x) && __slack.data()[x] != 0 && __base.data()[__slack.data()[x]] != static_cast<int>(x)
             && delta(g(static_cast<usize>(__slack.data()[x]), x)) == zero() )
          if ( found_edge(g(static_cast<usize>(__slack.data()[x]), x)) ) return true;
      for ( usize blossom = __n + 1; blossom <= __nodes; ++blossom )
        if ( __base.data()[blossom] == static_cast<int>(blossom) && __state.data()[blossom] == 1 && __label.data()[blossom] == zero() )
          expand_blossom(static_cast<int>(blossom));
    }
  }

public:
  explicit weighted_blossom_engine(usize vertices)
      : __n(vertices), __nodes(vertices), __stride(vertices * 2 + 1), __edges(__stride * __stride, blossom_edge{}),
        __label(__stride, zero()), __match(__stride, 0), __slack(__stride, 0), __base(__stride, 0), __parent(__stride, 0),
        __state(__stride, -1), __visited(__stride, usize(0)), __flower(__stride), __flower_from(__stride * __stride, 0)
  {
    for ( usize u = 0; u < __stride; ++u )
      for ( usize v = 0; v < __stride; ++v ) g(u, v) = { static_cast<int>(u), static_cast<int>(v), zero() };
    for ( usize vertex = 1; vertex <= __n; ++vertex ) {
      __base.data()[vertex] = static_cast<int>(vertex);
      from(vertex, vertex) = static_cast<int>(vertex);
    }
  }

  void
  add_edge(usize u, usize v, const score &value)
  {
    if ( g(u, v).weight < value ) {
      g(u, v) = { static_cast<int>(u), static_cast<int>(v), value };
      g(v, u) = { static_cast<int>(v), static_cast<int>(u), value };
    }
  }

  [[nodiscard]] const micron::vector<int, micron::allocator_serial<>, false> &
  solve()
  {
    score maximum{};
    for ( usize u = 1; u <= __n; ++u )
      for ( usize v = 1; v <= __n; ++v )
        if ( maximum < g(u, v).weight ) maximum = g(u, v).weight;
    for ( usize u = 1; u <= __n; ++u ) __label.data()[u] = maximum;
    while ( matching_step() ) {
    }
    return __match;
  }
};

};      // namespace __impl

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
maximum_weighted_matching(
    const G &graph, WeightMap weight_map,
    weighted_matching_workspace<typename G::index_type,
                                micron::remove_cvref_t<decltype(__impl::weight(micron::declval<WeightMap &>(), micron::declval<const G &>(),
                                                                               micron::declval<typename G::edge_descriptor>()))>>
        &workspace,
    matching_objective objective = matching_objective::maximum_weight)
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  weighted_matching_result<I, W> result{
    algorithm_status::ok,
    micron::vector<vertex_id<I>, micron::allocator_serial<>, false>(graph.vertex_slots(), vertex_id<I>::invalid()),
    micron::vector<edge_id<I>, micron::allocator_serial<>, false>(graph.vertex_slots(), edge_id<I>::invalid()),
    {},
    0,
    W{},
    graph.vertices_count()
  };
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    return result;
  }
  workspace.reserve(graph.vertex_slots(), graph.vertices_count());
  workspace.dense_to_vertex.clear();
  workspace.vertex_to_dense.resize(graph.vertex_slots(), vertex_id<I>::invalid_value());
  workspace.vertex_to_dense.fill(vertex_id<I>::invalid_value());
  for ( auto vertex : graph.vertices() ) {
    workspace.vertex_to_dense.data()[static_cast<usize>(vertex.value)] = static_cast<I>(workspace.dense_to_vertex.size());
    workspace.dense_to_vertex.push_back(vertex);
  }
  const usize n = workspace.dense_to_vertex.size();
  __impl::weighted_blossom_engine<W> engine(n);
  using score = __impl::blossom_score<W>;
  workspace.chosen.resize(n * n, edge_id<I>::invalid());
  workspace.chosen.fill(edge_id<I>::invalid());
  workspace.chosen_weight.resize(n * n, W{});
  workspace.chosen_weight.fill(W{});
  auto &chosen = workspace.chosen;
  auto &chosen_weight = workspace.chosen_weight;
  for ( auto edge : graph.edges() ) {
    const W weight = static_cast<W>(__impl::weight(weight_map, graph, edge.id));
    if ( __impl::nonfinite_weight(weight) ) {
      result.status = algorithm_status::invalid_weight;
      return result;
    }
    if ( edge.source == edge.target ) continue;
    const usize u = static_cast<usize>(workspace.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(workspace.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    const usize position = u * n + v;
    if ( !chosen.data()[position].valid() || chosen_weight.data()[position] < weight
         || (!(weight < chosen_weight.data()[position]) && !(chosen_weight.data()[position] < weight)
             && edge.id < chosen.data()[position]) ) {
      chosen.data()[position] = chosen.data()[v * n + u] = edge.id;
      chosen_weight.data()[position] = chosen_weight.data()[v * n + u] = weight;
    }
  }
  typename score::number_type cardinality_bonus{};
  if ( objective == matching_objective::maximum_cardinality_then_weight ) {
    typename score::number_type largest_magnitude{};
    for ( usize u = 0; u < n; ++u )
      for ( usize v = u + 1; v < n; ++v ) {
        if ( !chosen.data()[u * n + v].valid() ) continue;
        typename score::number_type value = static_cast<typename score::number_type>(chosen_weight.data()[u * n + v]);
        if ( value < typename score::number_type{} ) value = -value;
        if ( largest_magnitude < value ) largest_magnitude = value;
      }
    cardinality_bonus = largest_magnitude * static_cast<typename score::number_type>(n + 1) + static_cast<typename score::number_type>(1);
  }
  for ( usize u = 0; u < n; ++u )
    for ( usize v = u + 1; v < n; ++v ) {
      if ( !chosen.data()[u * n + v].valid() ) continue;
      const W weight = chosen_weight.data()[u * n + v];
      score value{};
      if constexpr ( micron::is_integral_v<W> )
        value.weight = (static_cast<int128_t>(weight) + cardinality_bonus) * 2;
      else
        value.weight = (static_cast<long double>(weight) + cardinality_bonus) * 2.0L;
      if ( objective == matching_objective::maximum_weight && !(score{} < value) ) continue;
      engine.add_edge(u + 1, v + 1, value);
    }
  const auto &match = engine.solve();
  result.edges.reserve(n / 2);
  for ( usize u = 1; u <= n; ++u ) {
    const int raw_v = match.data()[u];
    if ( raw_v <= 0 || raw_v > static_cast<int>(n) ) continue;
    const usize v = static_cast<usize>(raw_v - 1);
    const auto original_u = workspace.dense_to_vertex.data()[u - 1];
    const auto original_v = workspace.dense_to_vertex.data()[v];
    const edge_id<I> edge = chosen.data()[(u - 1) * n + v];
    result.mate.data()[static_cast<usize>(original_u.value)] = original_v;
    result.mate_edge.data()[static_cast<usize>(original_u.value)] = edge;
    if ( u - 1 < v ) {
      result.edges.push_back(edge);
      ++result.cardinality;
      W total{};
      if ( !__impl::add_distance(result.total_weight, chosen_weight.data()[(u - 1) * n + v], total) ) {
        result.status = algorithm_status::overflow;
        return result;
      }
      result.total_weight = total;
    }
  }
  return result;
}

template<graph_model G, typename WeightMap>
[[nodiscard]] auto
maximum_weighted_matching(const G &graph, WeightMap weight_map, matching_objective objective = matching_objective::maximum_weight)
{
  using I = typename G::index_type;
  using W = micron::remove_cvref_t<decltype(__impl::weight(weight_map, graph, micron::declval<typename G::edge_descriptor>()))>;
  weighted_matching_workspace<I, W> workspace;
  return maximum_weighted_matching(graph, weight_map, workspace, objective);
}

template<graph_model G>
[[nodiscard]] auto
maximum_weighted_matching(
    const G &graph,
    weighted_matching_workspace<typename G::index_type, micron::remove_cvref_t<decltype(__impl::weight(
                                                            micron::declval<intrinsic_edge_weight &>(), micron::declval<const G &>(),
                                                            micron::declval<typename G::edge_descriptor>()))>> &workspace,
    matching_objective objective = matching_objective::maximum_weight)
{
  return maximum_weighted_matching(graph, intrinsic_edge_weight{}, workspace, objective);
}

template<graph_model G>
[[nodiscard]] auto
maximum_weighted_matching(const G &graph, matching_objective objective = matching_objective::maximum_weight)
{
  return maximum_weighted_matching(graph, intrinsic_edge_weight{}, objective);
}

template<graph_model G>
[[nodiscard]] auto
maximum_cardinality_matching(const G &graph)
{
  return maximum_weighted_matching(graph, unit_edge_weight{}, matching_objective::maximum_cardinality_then_weight);
}

template<graph_model G>
[[nodiscard]] auto
maximum_cardinality_matching(const G &graph, weighted_matching_workspace<typename G::index_type, i32> &workspace)
{
  return maximum_weighted_matching(graph, unit_edge_weight{}, workspace, matching_objective::maximum_cardinality_then_weight);
}

};      // namespace micron::math::graphs
