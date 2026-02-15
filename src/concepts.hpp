//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "allocation/linux/kmemory.hpp"
#include "memory/actions.hpp"
#include "type_traits.hpp"
#include "types.hpp"
// all concepts go here
namespace micron
{
template <typename T>
concept integral = micron::is_integral_v<T>;

template <typename T, typename U>
concept __same_as = is_same_v<T, U>;

template <typename T, typename U>
concept same_as = __same_as<T, U> and __same_as<U, T>;

template <typename F, typename T>
concept convertible_to = is_convertible_v<F, T> and requires { static_cast<T>(declval<F>()); };

template <typename T>
concept destructible = micron::is_nothrow_destructible_v<T>;

template <typename T, typename... Args>
concept constructible_from = destructible<T> and requires { T(declval<Args>()...); };

template <typename L, typename R>
concept assignable_from = requires(L lhs, R &&rhs) {
  { lhs = micron::forward<R>(rhs) } -> micron::same_as<L>;
};
template <typename T>
concept movable = micron::constructible_from<T, T> and micron::assignable_from<T &, T> and micron::destructible<T>;

template <typename T>
concept copyable = movable<T> and micron::constructible_from<T, const T &> and micron::assignable_from<T &, const T &>;

template <typename T>
concept default_initializable = micron::constructible_from<T>;

template <typename T>
concept is_semiregular = micron::copyable<T> and micron::default_initializable<T>;

template <typename T>
concept is_regular = micron::is_semiregular<T> and requires(const T &a, const T &b) {
  { a == b } -> micron::convertible_to<bool>;
  { a != b } -> micron::convertible_to<bool>;
};

template <typename T>
concept is_movable_object
    = micron::movable<T> and micron::is_move_constructible_v<T> and micron::is_move_assignable_v<T>;

template <typename T>
concept is_constexpr_valid = micron::is_trivially_destructible_v<T> && requires {
  { T{} } -> micron::same_as<T>;
};

template <typename T>
concept is_scalar_literal = micron::is_literal_type_v<T> and micron::is_fundamental_v<T>;

template <typename T>
concept is_fundamental_object = micron::is_literal_type_v<T> and (micron::is_fundamental_v<T> or micron::is_scalar_v<T>);

template <typename T>
concept is_regular_object
    = micron::copyable<T> and micron::movable<T> and micron::is_copy_constructible_v<T>
      and micron::is_move_constructible_v<T> and micron::is_copy_assignable_v<T> and micron::is_move_assignable_v<T>;

template <typename T, typename I = size_t>
concept is_iterable_container = requires(T t, I i) {
  { t.data() } -> micron::same_as<typename T::pointer>;
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
  { t[i] } -> micron::same_as<typename T::reference>;
  { t.size() } -> micron::same_as<typename T::size_type>;
};

template <typename T>
concept is_general_pointer_class = requires(T t) {
  typename T::element_type;
  { t.get() } -> micron::same_as<typename T::element_type *>;
};
template <typename T>
concept is_micron_structure = requires(T a) {
  { *a } -> micron::same_as<chunk<byte>>;
  { &a } -> micron::same_as<byte *>;
};

// short hand, binds all the important types
template <typename T>
concept stl_container = requires(T t) {
  {
    t.get_allocator()
  } -> micron::same_as<typename T::allocator_type>;     // important, micron containers DONT provide
                                                        // this
  { t.data() } -> micron::same_as<typename T::iterator>;
  { t.size() } -> micron::convertible_to<typename T::size_type>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.begin() } -> micron::same_as<typename T::const_iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::const_iterator>;
};

template <typename T>
concept is_container_or_string = requires(T t) {
  { t.data() } -> micron::same_as<typename T::pointer>;
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
};

template <typename T>
concept is_container = requires(T t) {
  { t.data() } -> micron::same_as<typename T::pointer>;
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
} and !requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
};
template <typename C>
concept is_constexpr_container = requires {
  []() consteval {
    C c{};
    auto n = c.size();
    (void)n;

    if constexpr ( requires { c[0]; } ) {
      auto x = c[0];
      (void)x;
    }

    for ( auto it = c.begin(); it != c.end(); ++it ) {
      (void)*it;
    }
  }();
};

template <typename T>
concept is_string_ascii = requires(T t) {
  typename T::value_type;
  requires micron::is_same_v<typename T::value_type, char>;
  { t.c_str() } -> micron::same_as<const char *>;
  { t.data() } -> micron::same_as<typename T::pointer>;
  { t.size() } -> micron::convertible_to<size_t>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
};

template <typename T>
concept is_string = requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
  { t.data() } -> micron::same_as<typename T::pointer>;
  { t.size() } -> micron::convertible_to<size_t>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
};

template <typename T>
concept is_string_on_stack = requires(T t) {
  typename T::stack_tag;
  { t.c_str() } -> micron::same_as<const typename T::value_type *>;
  { t.data() } -> micron::same_as<typename T::pointer>;
  { t.size() } -> micron::convertible_to<size_t>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
  { t.cbegin() } -> micron::same_as<typename T::iterator>;
  { t.cend() } -> micron::same_as<typename T::iterator>;
};
};
