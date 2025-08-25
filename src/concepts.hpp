//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "allocation/chunks.hpp"
// all concepts go here
namespace micron
{

template <typename T, typename U>
concept __same_as = is_same_v<T, U>;

template <typename T, typename U>
concept same_as = __same_as<T, U> && __same_as<U, T>;

template <typename F, typename T>
concept convertible_to = is_convertible_v<F, T> && requires { static_cast<T>(declval<F>()); };

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
} && !requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
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
