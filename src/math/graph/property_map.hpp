//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../array/array.hpp"
#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../span.hpp"
#include "../../type_traits.hpp"
#include "../../vector/vector.hpp"
#include "descriptors.hpp"

namespace micron::math::graphs
{

template<typename Map, typename Key>
concept readable_property_map = requires(const Map map, Key key) { map[key]; };

template<typename Map, typename Key>
concept writable_property_map = readable_property_map<Map, Key> && requires(Map map, Key key) { map[key] = map[key]; };

template<typename Container, micron::integral I = u32> struct container_property_map {
  using container_type = Container;
  using value_type = typename micron::remove_cv_t<Container>::value_type;
  using index_type = I;

  Container *container{};

  constexpr explicit container_property_map(Container &c) noexcept : container(micron::addressof(c)) { }

  [[nodiscard]] constexpr decltype(auto)
  operator[](vertex_id<I> id) noexcept
  {
    return (*container)[static_cast<usize>(id.value)];
  }

  [[nodiscard]] constexpr decltype(auto)
  operator[](edge_id<I> id) noexcept
  {
    return (*container)[static_cast<usize>(id.value)];
  }

  [[nodiscard]] constexpr decltype(auto)
  operator[](vertex_id<I> id) const noexcept
  {
    return (*container)[static_cast<usize>(id.value)];
  }

  [[nodiscard]] constexpr decltype(auto)
  operator[](edge_id<I> id) const noexcept
  {
    return (*container)[static_cast<usize>(id.value)];
  }
};

template<micron::integral I = u32, typename Container>
[[nodiscard]] constexpr auto
property_map(Container &container) noexcept
{
  return container_property_map<Container, I>(container);
}

template<typename T, micron::integral I = u32> struct pointer_property_map {
  using value_type = T;
  using index_type = I;

  T *data{};
  usize size{};

  constexpr pointer_property_map() noexcept = default;

  constexpr pointer_property_map(T *p, usize n) noexcept : data(p), size(n) { }

  [[nodiscard]] constexpr T &
  operator[](vertex_id<I> id) const noexcept
  {
    return data[static_cast<usize>(id.value)];
  }

  [[nodiscard]] constexpr T &
  operator[](edge_id<I> id) const noexcept
  {
    return data[static_cast<usize>(id.value)];
  }

  [[nodiscard]] constexpr T *
  try_get(vertex_id<I> id) const noexcept
  {
    return id.valid() && static_cast<usize>(id.value) < size ? data + static_cast<usize>(id.value) : nullptr;
  }

  [[nodiscard]] constexpr T *
  try_get(edge_id<I> id) const noexcept
  {
    return id.valid() && static_cast<usize>(id.value) < size ? data + static_cast<usize>(id.value) : nullptr;
  }
};

template<micron::integral I = u32, typename T>
[[nodiscard]] constexpr auto
property_map(T *data, usize size) noexcept
{
  return pointer_property_map<T, I>(data, size);
}

template<typename Fn> struct projection_property_map {
  [[no_unique_address]] Fn projection;

  template<typename Key>
  [[nodiscard]] constexpr decltype(auto)
  operator[](Key key)
  {
    return micron::invoke(projection, key);
  }

  template<typename Key>
  [[nodiscard]] constexpr decltype(auto)
  operator[](Key key) const
  {
    return micron::invoke(projection, key);
  }
};

template<typename Fn>
[[nodiscard]] constexpr auto
project_property(Fn &&fn)
{
  return projection_property_map<micron::remove_cvref_t<Fn>>{ micron::forward<Fn>(fn) };
}

template<typename Matrix, micron::integral I = u32> struct matrix_row_property_map {
  Matrix *matrix{};
  usize row{};

  [[nodiscard]] constexpr decltype(auto)
  operator[](vertex_id<I> id) const noexcept
  {
    return matrix->at(row, static_cast<usize>(id.value));
  }
};

template<typename Matrix, micron::integral I = u32> struct matrix_column_property_map {
  Matrix *matrix{};
  usize column{};

  [[nodiscard]] constexpr decltype(auto)
  operator[](vertex_id<I> id) const noexcept
  {
    return matrix->at(static_cast<usize>(id.value), column);
  }
};

template<micron::integral I = u32, typename Matrix>
[[nodiscard]] constexpr auto
row_property_map(Matrix &matrix, usize row) noexcept
{
  return matrix_row_property_map<Matrix, I>{ micron::addressof(matrix), row };
}

template<micron::integral I = u32, typename Matrix>
[[nodiscard]] constexpr auto
column_property_map(Matrix &matrix, usize column) noexcept
{
  return matrix_column_property_map<Matrix, I>{ micron::addressof(matrix), column };
}

};      // namespace micron::math::graphs
