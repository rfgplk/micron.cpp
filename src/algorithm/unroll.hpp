#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace __impl
{
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compile-time fold-expression unrollers
// used when the container exposes  static constexpr usize static_size
template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_transform_val(T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  ((data[Is] = fn(data[Is])), ...);
}

template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_transform_ptr(T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  ((data[Is] = fn(&data[Is])), ...);
}

template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_transform_cref(T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  ((data[Is] = fn(static_cast<const T &>(data[Is]))), ...);
}

template <typename T, typename O, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_transform_bin(const T *a, const T *b, O *out, Fn &fn, index_sequence<Is...>) noexcept
{
  ((out[Is] = fn(a[Is], b[Is])), ...);
}

template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr bool
__unroll_all_of(const T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  return (fn(data[Is]) && ...);
}

template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr bool
__unroll_any_of(const T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  return (fn(data[Is]) || ...);
}

template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr bool
__unroll_none_of(const T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  return (!fn(data[Is]) && ...);
}

template <typename T, typename V, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_fill(T *data, const V &val, index_sequence<Is...>) noexcept
{
  ((data[Is] = val), ...);
}

template <typename T, typename Fn, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_generate(T *data, Fn &fn, index_sequence<Is...>) noexcept
{
  ((data[Is] = fn()), ...);
}

template <typename R, typename T, usize... Is>
__attribute__((always_inline)) constexpr R
__unroll_sum(const T *data, index_sequence<Is...>) noexcept
{
  return (static_cast<R>(data[Is]) + ...);
}

template <typename T, typename Y, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_add(T *data, const Y y, index_sequence<Is...>) noexcept
{
  ((data[Is] += y), ...);
}

template <typename T, typename Y, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_subtract(T *data, const Y y, index_sequence<Is...>) noexcept
{
  ((data[Is] -= y), ...);
}

template <typename T, typename Y, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_multiply(T *data, const Y y, index_sequence<Is...>) noexcept
{
  ((data[Is] *= y), ...);
}

template <typename T, typename Y, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_divide(T *data, const Y y, index_sequence<Is...>) noexcept
{
  ((data[Is] /= y), ...);
}

template <typename T, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_negate(T *data, index_sequence<Is...>) noexcept
{
  ((data[Is] = -data[Is]), ...);
}

template <typename R, typename T, usize... Is>
__attribute__((always_inline)) constexpr R
__unroll_inner_product(const T *a, const T *b, R init, index_sequence<Is...>) noexcept
{
  return (init + ... + static_cast<R>(a[Is] * b[Is]));
}

template <typename T, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_zip_add(T *a, const T *b, index_sequence<Is...>) noexcept
{
  ((a[Is] = a[Is] + b[Is]), ...);
}

template <typename T, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_zip_subtract(T *a, const T *b, index_sequence<Is...>) noexcept
{
  ((a[Is] = a[Is] - b[Is]), ...);
}

template <typename T, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_zip_multiply(T *a, const T *b, index_sequence<Is...>) noexcept
{
  ((a[Is] = a[Is] * b[Is]), ...);
}

template <typename T, usize... Is>
__attribute__((always_inline)) constexpr void
__unroll_clamp(T *data, const T lo, const T hi, index_sequence<Is...>) noexcept
{
  ((data[Is] = (data[Is] < lo) ? lo : (hi < data[Is]) ? hi : data[Is]), ...);
}

};     // namespace __impl
};     // namespace micron
