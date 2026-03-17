//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "memory/actions.hpp"
#include "memory/allocation/kmemory.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "tags.hpp"

// for string concepts
#include "string/unitypes.hpp"

// all concepts go here
namespace micron
{
template <typename T>
concept integral = micron::is_integral_v<T>;

template <typename T, typename U>
concept __same_as = is_same_v<T, U>;

template <typename T, typename U>
concept same_as = __same_as<T, U> and __same_as<U, T>;

template <typename T, size_t N> constexpr bool size_of = (sizeof(T) == N);

template <typename F, typename T>
concept convertible_to = is_convertible_v<F, T> and requires { static_cast<T>(declval<F>()); };

template <typename T>
concept destructible = micron::is_nothrow_destructible_v<T>;

template <typename T, typename... Args>
concept constructible_from = destructible<T> and requires { T(declval<Args>()...); };

template <typename L, typename R>
concept assignable_from = requires(L &lhs, R &&rhs) {
  { lhs = micron::forward<R>(rhs) } -> micron::same_as<L>;
};
template <typename T>
concept equality_comparable = requires(const T &a, const T &b) {
  { a == b } -> micron::convertible_to<bool>;
  { a != b } -> micron::convertible_to<bool>;
};

template <typename T, typename U>
concept equality_comparable_with = micron::__same_as<micron::remove_cvref_t<T>, micron::remove_cvref_t<T>>
                                   && micron::__same_as<micron::remove_cvref_t<U>, micron::remove_cvref_t<U>>
                                   && micron::equality_comparable<T> && micron::equality_comparable<U> && requires(const T &t, const U &u) {
                                        { t == u } -> micron::convertible_to<bool>;
                                        { t != u } -> micron::convertible_to<bool>;
                                        { u == t } -> micron::convertible_to<bool>;
                                        { u != t } -> micron::convertible_to<bool>;
                                      };

template <typename T>
concept boolean_testable = micron::convertible_to<T, bool>;

template <typename T>
concept totally_ordered = micron::equality_comparable<T> && requires(const T &a, const T &b) {
  { a < b } -> micron::boolean_testable;
  { a > b } -> micron::boolean_testable;
  { a <= b } -> micron::boolean_testable;
  { a >= b } -> micron::boolean_testable;
};

template <typename T, typename U>
concept totally_ordered_with = micron::totally_ordered<T> && micron::totally_ordered<U> && micron::equality_comparable_with<T, U>
                               && requires(const T &t, const U &u) {
                                    { t < u } -> micron::boolean_testable;
                                    { t > u } -> micron::boolean_testable;
                                    { t <= u } -> micron::boolean_testable;
                                    { t >= u } -> micron::boolean_testable;
                                    { u < t } -> micron::boolean_testable;
                                    { u > t } -> micron::boolean_testable;
                                    { u <= t } -> micron::boolean_testable;
                                    { u >= t } -> micron::boolean_testable;
                                  };
template <typename T, typename U>
concept comparable_with = micron::totally_ordered<T> && micron::totally_ordered<U> && micron::equality_comparable_with<T, U>
                          && requires(const T &t, const U &u) {
                               { t < u } -> micron::boolean_testable;
                               { t > u } -> micron::boolean_testable;
                               { t <= u } -> micron::boolean_testable;
                               { t >= u } -> micron::boolean_testable;
                               { u < t } -> micron::boolean_testable;
                               { u > t } -> micron::boolean_testable;
                               { u <= t } -> micron::boolean_testable;
                               { u >= t } -> micron::boolean_testable;
                             };
template <typename T>
concept movable = micron::constructible_from<T &&, T> and micron::assignable_from<T &, T> and micron::destructible<T>;

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
concept is_movable_object = micron::movable<T> and micron::is_move_constructible_v<T> and micron::is_move_assignable_v<T>;

template <typename T>
concept is_constexpr_valid = micron::is_trivially_destructible_v<T> && requires {
  { T{} } -> micron::same_as<T>;
};

template <typename T>
concept is_scalar_literal = micron::is_literal_type_v<T> and micron::is_fundamental_v<T>;

template <typename T>
concept is_fundamental_object = micron::is_literal_type_v<T> and (micron::is_fundamental_v<T> or micron::is_scalar_v<T>);

template <typename T>
concept is_regular_object = micron::copyable<T> and micron::movable<T> and micron::is_copy_constructible_v<T>
                            and micron::is_move_constructible_v<T> and micron::is_copy_assignable_v<T> and micron::is_move_assignable_v<T>;

template <typename M>
concept is_mutex = requires(M m) {
  { m.lock() };
  { m.try_lock() } noexcept -> micron::same_as<bool>;
  { m.unlock() } noexcept -> micron::same_as<void>;
  { m.is_locked() } noexcept -> micron::same_as<bool>;
} && !micron::is_copy_constructible_v<M> && !micron::is_move_constructible_v<M>;

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

template <typename T, typename I = size_t>
concept is_iterable = requires(T t, I i) {
  { &t } -> micron::same_as<byte *>;
  { &t[i] } -> micron::same_as<typename T::value_type *>;
} || requires(T t) {
  { t.begin() } -> micron::same_as<typename T::value_type *>;
  { t.end() } -> micron::same_as<typename T::value_type *>;
};
template <typename T>
concept addressable = requires(T t) {
  { &t } -> micron::same_as<T *>;
};

template <typename Cmp, typename C>
concept is_valid_comp = requires(Cmp c, typename C::value_type a, typename C::value_type b) {
  { c(a, b) } -> micron::convertible_to<bool>;
};

template <typename T>
concept is_atomic_type
    = micron::is_integral_v<T> || micron::is_pointer_v<T> || micron::is_floating_point_v<T>
      || (micron::is_trivially_copyable_v<T> && !micron::is_integral_v<T> && !micron::is_pointer_v<T> && !micron::is_floating_point_v<T>);

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
  { t.end() } -> micron::same_as<typename T::iterator>;
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
concept has_cstr = requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
};

template <typename T>
concept is_char_ptr
    = micron::is_same_v<T, char *> || micron::is_same_v<T, schar *> || micron::is_same_v<T, wide *> || micron::is_same_v<T, unicode8 *>
      || micron::is_same_v<T, unicode32 *> || micron::is_same_v<T, const char *> || micron::is_same_v<T, const schar *>
      || micron::is_same_v<T, const wide *> || micron::is_same_v<T, const unicode8 *> || micron::is_same_v<T, const unicode32 *>;

template <typename T>
concept is_char_elem
    = micron::is_same_v<T, char> || micron::is_same_v<T, schar> || micron::is_same_v<T, wide> || micron::is_same_v<T, unicode8>
      || micron::is_same_v<T, unicode32> || micron::is_same_v<T, const char> || micron::is_same_v<T, const schar>
      || micron::is_same_v<T, const wide> || micron::is_same_v<T, const unicode8> || micron::is_same_v<T, const unicode32>;

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

template <typename, typename = void> struct is_string_tt : micron::false_type {
};

template <typename T>
struct is_string_tt<
    T, micron::void_t<decltype(micron::declval<T>().c_str()), decltype(micron::declval<T>().data()), decltype(micron::declval<T>().size()),
                      decltype(micron::declval<T>().begin()), decltype(micron::declval<T>().end()), decltype(micron::declval<T>().cbegin()),
                      decltype(micron::declval<T>().cend()), typename T::pointer, typename T::iterator, typename T::const_iterator>>
    : micron::bool_constant<micron::same_as<decltype(micron::declval<T>().c_str()), const char *>
                            && micron::same_as<decltype(micron::declval<T>().data()), typename T::pointer>
                            && micron::convertible_to<decltype(micron::declval<T>().size()), size_t>
                            && micron::same_as<decltype(micron::declval<T>().begin()), typename T::iterator>
                            && micron::same_as<decltype(micron::declval<T>().end()), typename T::iterator>
                            && micron::same_as<decltype(micron::declval<T>().cbegin()), typename T::const_iterator>
                            && micron::same_as<decltype(micron::declval<T>().cend()), typename T::const_iterator>> {
};

template <typename T> inline constexpr bool is_string_v = is_string_tt<T>::value;

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

template <typename T>
concept is_tagged_map = requires {
  typename micron::remove_cvref_t<T>::category_type;
  requires micron::is_same_v<typename micron::remove_cvref_t<T>::category_type, map_tag>;
};
template <typename T>
concept is_map = requires {
  typename micron::remove_cvref_t<T>::category_type;
  requires micron::is_same_v<typename micron::remove_cvref_t<T>::category_type, map_tag>;
} || requires(micron::remove_cvref_t<T> t) {
  { t.size() } -> micron::convertible_to<usize>;
  { t.capacity() } -> micron::convertible_to<usize>;
  { t.empty() } -> micron::convertible_to<bool>;
  { t.clear() };
  { t.begin() };
  { t.end() };
};

template <typename T>
concept is_swiss_map = requires(micron::remove_cvref_t<T> t) {
  { t.capacity() } -> micron::convertible_to<usize>;
  { t.size() } -> micron::convertible_to<usize>;
  { t.begin() };
  { t.end() };
  { (*t.begin()).a };
  { (*t.begin()).b };
} && !is_tagged_map<T> && !has_cstr<T>;

template <typename F, typename Arg>
concept strict_invocable
    = (is_invocable_v<F, Arg>
       && (same_as<Arg, remove_cvref_t<Arg>> || same_as<Arg, remove_cvref_t<Arg> &> || same_as<Arg, const remove_cvref_t<Arg> &>));

};     // namespace micron
