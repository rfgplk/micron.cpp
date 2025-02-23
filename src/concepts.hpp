#pragma once

#include "allocation/chunks.hpp"
#include <concepts>
#include <type_traits>
// all concepts go here
namespace micron
{

template <typename T>
concept is_micron_structure = requires(T a) {
  { *a } -> std::same_as<chunk<byte>>;
  { &a } -> std::same_as<byte *>;
};

// short hand, binds all the important types
template <typename T>
concept stl_container = requires(T t) {
  {
    t.get_allocator()
  } -> std::same_as<typename T::allocator_type>;     // important, micron containers DONT provide
                                                     // this
  { t.data() } -> std::same_as<typename T::iterator>;
  { t.size() } -> std::convertible_to<typename T::size_type>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.begin() } -> std::same_as<typename T::const_iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::const_iterator>;
};

template <typename T>
concept is_container_or_string = requires(T t) {
  { t.data() } -> std::same_as<typename T::pointer>;
  { t.cbegin() } -> std::same_as<typename T::const_iterator>;
  { t.cend() } -> std::same_as<typename T::const_iterator>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
};
template <typename T>
concept is_container = requires(T t) {
  { t.data() } -> std::same_as<typename T::pointer>;
  { t.cbegin() } -> std::same_as<typename T::const_iterator>;
  { t.cend() } -> std::same_as<typename T::const_iterator>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
} && !requires(T t) {
  { t.c_str() } -> std::same_as<const char *>;
};

template <typename T>
concept is_string_ascii = requires(T t) {
  typename T::value_type;
  requires std::is_same_v<typename T::value_type, char>;
  { t.c_str() } -> std::same_as<const char *>;
  { t.data() } -> std::same_as<typename T::pointer>;
  { t.size() } -> std::convertible_to<size_t>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
  { t.cbegin() } -> std::same_as<typename T::const_iterator>;
  { t.cend() } -> std::same_as<typename T::const_iterator>;
};

template <typename T>
concept is_string = requires(T t) {
  { t.c_str() } -> std::same_as<const char *>;
  { t.data() } -> std::same_as<typename T::pointer>;
  { t.size() } -> std::convertible_to<size_t>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
  { t.cbegin() } -> std::same_as<typename T::const_iterator>;
  { t.cend() } -> std::same_as<typename T::const_iterator>;
};

template <typename T>
concept is_string_on_stack = requires(T t) {
  typename T::stack_tag;
  { t.c_str() } -> std::same_as<const typename T::value_type *>;
  { t.data() } -> std::same_as<typename T::pointer>;
  { t.size() } -> std::convertible_to<size_t>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
  { t.cbegin() } -> std::same_as<typename T::iterator>;
  { t.cend() } -> std::same_as<typename T::iterator>;
};

};
