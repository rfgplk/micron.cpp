//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../vector/vector.hpp"
#include "../matrix/dynmat.hpp"
#include "../sparse/csr.hpp"
#include "graph.hpp"
#include "paths.hpp"

namespace micron::math::graphs
{

template<typename Matrix, micron::integral I> struct matrix_conversion_result {
  Matrix value;
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
  micron::vector<I, micron::allocator_serial<>, false> vertex_to_dense;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> dense_to_edge;
};

template<graph_model G>
[[nodiscard]] auto
__dense_vertex_mapping(const G &graph)
{
  using I = typename G::index_type;

  struct mapping {
    micron::vector<vertex_id<I>, micron::allocator_serial<>, false> dense_to_vertex;
    micron::vector<I, micron::allocator_serial<>, false> vertex_to_dense;
  } result{ {}, micron::vector<I, micron::allocator_serial<>, false>(graph.vertex_slots(), vertex_id<I>::invalid_value()) };

  result.dense_to_vertex.reserve(graph.vertices_count());
  for ( auto vertex : graph.vertices() ) {
    const I dense = static_cast<I>(result.dense_to_vertex.size());
    result.vertex_to_dense.data()[static_cast<usize>(vertex.value)] = dense;
    result.dense_to_vertex.push_back(vertex);
  }
  return result;
}

template<arith_scalar T = u8, graph_model G>
[[nodiscard]] matrix_conversion_result<dynmat<T>, typename G::index_type>
adjacency_matrix(const G &graph, T absent = T(0), T present = T(1))
{
  using I = typename G::index_type;
  auto mapping = __dense_vertex_mapping(graph);
  const usize n = graph.vertices_count();
  matrix_conversion_result<dynmat<T>, I> result{
    dynmat<T>(n, n, absent), micron::move(mapping.dense_to_vertex), micron::move(mapping.vertex_to_dense), {}
  };
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    result.value.at(u, v) = present;
    if constexpr ( !G::is_directed ) result.value.at(v, u) = present;
  }
  return result;
}

template<arith_scalar T = u8, graph_model G>
[[nodiscard]] dynmat<T>
to_adjacency_matrix(const G &graph, T absent = T(0), T present = T(1))
{
  auto result = adjacency_matrix<T>(graph, absent, present);
  return micron::move(result.value);
}

template<arith_scalar T, graph_model G, typename WeightMap = intrinsic_edge_weight>
[[nodiscard]] matrix_conversion_result<dynmat<T>, typename G::index_type>
weighted_adjacency_matrix(const G &graph, WeightMap weight_map = {}, T absent = T(0))
{
  using I = typename G::index_type;
  auto mapping = __dense_vertex_mapping(graph);
  const usize n = graph.vertices_count();
  matrix_conversion_result<dynmat<T>, I> result{
    dynmat<T>(n, n, absent), micron::move(mapping.dense_to_vertex), micron::move(mapping.vertex_to_dense), {}
  };
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    const T weight = static_cast<T>(__impl::weight(weight_map, graph, edge.id));
    result.value.at(u, v) = weight;
    if constexpr ( !G::is_directed ) result.value.at(v, u) = weight;
  }
  return result;
}

template<arith_scalar T = usize, graph_model G>
[[nodiscard]] matrix_conversion_result<dynmat<T>, typename G::index_type>
degree_matrix(const G &graph)
{
  using I = typename G::index_type;
  auto mapping = __dense_vertex_mapping(graph);
  const usize n = graph.vertices_count();
  matrix_conversion_result<dynmat<T>, I> result{
    dynmat<T>(n, n, T(0)), micron::move(mapping.dense_to_vertex), micron::move(mapping.vertex_to_dense), {}
  };
  for ( usize i = 0; i < n; ++i ) result.value.at(i, i) = static_cast<T>(graph.degree(result.dense_to_vertex.data()[i]));
  return result;
}

template<arith_scalar T = i64, graph_model G>
[[nodiscard]] matrix_conversion_result<dynmat<T>, typename G::index_type>
laplacian_matrix(const G &graph)
{
  auto result = adjacency_matrix<T>(graph, T(0), T(1));
  const usize n = result.value.rows;
  for ( usize i = 0; i < n; ++i ) {
    T degree{};
    for ( usize j = 0; j < n; ++j ) degree += result.value.at(i, j);
    for ( usize j = 0; j < n; ++j ) result.value.at(i, j) = -result.value.at(i, j);
    result.value.at(i, i) += degree;
  }
  return result;
}

template<arith_scalar T = i64, graph_model G>
[[nodiscard]] matrix_conversion_result<dynmat<T>, typename G::index_type>
incidence_matrix(const G &graph)
{
  using I = typename G::index_type;
  auto mapping = __dense_vertex_mapping(graph);
  const usize n = graph.vertices_count();
  const usize m = graph.edges_count();
  matrix_conversion_result<dynmat<T>, I> result{
    dynmat<T>(n, m, T(0)), micron::move(mapping.dense_to_vertex), micron::move(mapping.vertex_to_dense), {}
  };
  result.dense_to_edge.reserve(m);
  usize column = 0;
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    if constexpr ( G::is_directed ) {
      result.value.at(u, column) -= T(1);
      result.value.at(v, column) += T(1);
    } else {
      result.value.at(u, column) += T(1);
      result.value.at(v, column) += T(1);
    }
    result.dense_to_edge.push_back(edge.id);
    ++column;
  }
  return result;
}

template<arith_scalar T = u8, graph_model G>
[[nodiscard]] matrix_conversion_result<sparse::csr<T, typename G::index_type>, typename G::index_type>
sparse_adjacency_matrix(const G &graph, T present = T(1))
{
  using I = typename G::index_type;
  auto mapping = __dense_vertex_mapping(graph);
  const usize n = graph.vertices_count();
  matrix_conversion_result<sparse::csr<T, I>, I> result{
    sparse::csr<T, I>(n, n), micron::move(mapping.dense_to_vertex), micron::move(mapping.vertex_to_dense), {}
  };
  micron::vector<usize, micron::allocator_serial<>, false> count(n, usize(0));
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    ++count.data()[u];
    if constexpr ( !G::is_directed )
      if ( u != v ) ++count.data()[v];
  }
  for ( usize row = 0; row < n; ++row )
    result.value.outer.data()[row + 1] = result.value.outer.data()[row] + static_cast<I>(count.data()[row]);
  result.value.inner.resize(static_cast<usize>(result.value.outer.data()[n]), I{});
  result.value.values.resize(result.value.inner.size(), T{});
  micron::vector<usize, micron::allocator_serial<>, false> cursor(n, usize(0));
  for ( usize row = 0; row < n; ++row ) cursor.data()[row] = static_cast<usize>(result.value.outer.data()[row]);
  for ( auto edge : graph.edges() ) {
    const I u = result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)];
    const I v = result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)];
    usize position = cursor.data()[static_cast<usize>(u)]++;
    result.value.inner.data()[position] = v;
    result.value.values.data()[position] = present;
    if constexpr ( !G::is_directed ) {
      if ( u != v ) {
        position = cursor.data()[static_cast<usize>(v)]++;
        result.value.inner.data()[position] = u;
        result.value.values.data()[position] = present;
      }
    }
  }
  return result;
}

template<arith_scalar T, graph_model G, typename WeightMap = intrinsic_edge_weight>
[[nodiscard]] matrix_conversion_result<sparse::csr<T, typename G::index_type>, typename G::index_type>
sparse_weighted_adjacency_matrix(const G &graph, WeightMap weight_map = {})
{
  auto result = sparse_adjacency_matrix<T>(graph, T{});
  micron::vector<usize, micron::allocator_serial<>, false> cursor(result.value.rows, usize(0));
  for ( usize row = 0; row < result.value.rows; ++row ) cursor.data()[row] = static_cast<usize>(result.value.outer.data()[row]);
  for ( auto edge : graph.edges() ) {
    const auto u = result.vertex_to_dense.data()[static_cast<usize>(edge.source.value)];
    const auto v = result.vertex_to_dense.data()[static_cast<usize>(edge.target.value)];
    const T weight = static_cast<T>(__impl::weight(weight_map, graph, edge.id));
    result.value.values.data()[cursor.data()[static_cast<usize>(u)]++] = weight;
    if constexpr ( !G::is_directed )
      if ( u != v ) result.value.values.data()[cursor.data()[static_cast<usize>(v)]++] = weight;
  }
  return result;
}

template<arith_scalar T = u8, graph_model G>
[[nodiscard]] matrix_conversion_result<sparse::csc<T, typename G::index_type>, typename G::index_type>
sparse_adjacency_matrix_csc(const G &graph, T present = T(1))
{
  auto row_major = sparse_adjacency_matrix<T>(graph, present);
  return { sparse::to_csc(row_major.value), micron::move(row_major.dense_to_vertex), micron::move(row_major.vertex_to_dense),
           micron::move(row_major.dense_to_edge) };
}

template<typename G = graph<>, typename Matrix>
[[nodiscard]] G
from_adjacency_matrix(const Matrix &matrix)
{
  G result;
  const usize n = static_cast<usize>(matrix.rows < matrix.cols ? matrix.rows : matrix.cols);
  (void)result.add_vertices(n);
  for ( usize i = 0; i < n; ++i ) {
    const usize begin = G::is_directed ? 0 : i;
    for ( usize j = begin; j < n; ++j ) {
      const auto &entry = matrix.at(i, j);
      using entry_type = micron::remove_cvref_t<decltype(entry)>;
      if ( entry == entry_type(0) ) continue;
      if constexpr ( weighted_bundle<typename G::edge_property_type> )
        (void)result.add_edge(static_cast<typename G::index_type>(i), static_cast<typename G::index_type>(j), entry);
      else
        (void)result.add_edge(static_cast<typename G::index_type>(i), static_cast<typename G::index_type>(j));
    }
  }
  return result;
}

template<typename G = graph<>, arith_scalar T, micron::integral I>
[[nodiscard]] G
from_sparse_adjacency_matrix(const sparse::csr<T, I> &matrix)
{
  G result;
  const usize n = matrix.rows < matrix.cols ? matrix.rows : matrix.cols;
  (void)result.add_vertices(n);
  if ( matrix.outer.size() < matrix.rows + 1 || matrix.inner.size() != matrix.values.size() ) return result;
  for ( usize row = 0; row < n; ++row ) {
    usize begin = static_cast<usize>(matrix.outer.data()[row]);
    usize end = static_cast<usize>(matrix.outer.data()[row + 1]);
    if ( begin > end || end > matrix.inner.size() ) return G{};
    for ( usize position = begin; position < end; ++position ) {
      const usize column = static_cast<usize>(matrix.inner.data()[position]);
      if ( column >= n || matrix.values.data()[position] == T{} ) continue;
      if constexpr ( !G::is_directed )
        if ( column < row ) continue;
      if constexpr ( weighted_bundle<typename G::edge_property_type> )
        (void)result.add_edge(static_cast<typename G::index_type>(row), static_cast<typename G::index_type>(column),
                              matrix.values.data()[position]);
      else
        (void)result.add_edge(static_cast<typename G::index_type>(row), static_cast<typename G::index_type>(column));
    }
  }
  return result;
}

template<typename G = graph<>, arith_scalar T, micron::integral I>
[[nodiscard]] G
from_sparse_adjacency_matrix(const sparse::csc<T, I> &matrix)
{
  return from_sparse_adjacency_matrix<G>(sparse::to_csr(matrix));
}

};      // namespace micron::math::graphs
