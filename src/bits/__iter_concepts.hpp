//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// iterator categories and concepts

namespace micron
{
template<typename I> struct __iter_diff {
  using type = ssize_t;
};

template<typename I>
  requires requires { typename I::difference_type; }
struct __iter_diff<I> {
  using type = typename I::difference_type;
};

template<typename I>
  requires micron::is_pointer_v<I>
struct __iter_diff<I> {
  using type = ssize_t;
};

template<typename I> using iter_diff_t = typename __iter_diff<I>::type;

template<typename I> using iter_ref_t = decltype(*micron::declval<I &>());

template<typename I> using iter_value_of_t = micron::remove_cvref_t<iter_ref_t<I>>;

template<typename I>
concept indirectly_readable = requires(I &__i) {
  { *__i } -> micron::distinct<void>;
};

template<typename I>
concept weakly_incrementable = requires(I &__i) {
  { ++__i } -> micron::same_as<I &>;
};

template<typename S, typename I>
concept sentinel_for = requires(I &__i, const S &__s) {
  { __i == __s } -> micron::convertible_to<bool>;
};

template<typename I>
concept input_iterator = indirectly_readable<I> && weakly_incrementable<I>;

template<typename I>
concept forward_iterator = input_iterator<I> && micron::is_copy_constructible_v<I> && sentinel_for<I, I>;

template<typename I>
concept bidirectional_iterator = forward_iterator<I> && requires(I &__i) {
  { --__i } -> micron::same_as<I &>;
};

template<typename I>
concept random_access_iterator = bidirectional_iterator<I> && requires(I &__i, const I &__c, iter_diff_t<I> __n) {
  { __i += __n } -> micron::same_as<I &>;
  { __c + __n } -> micron::same_as<I>;
  { __c - __c } -> micron::convertible_to<iter_diff_t<I>>;
  { __c[__n] };
  { __c < __c } -> micron::convertible_to<bool>;
};

template<typename I>
concept contiguous_iterator = micron::is_pointer_v<I>;

};      // namespace micron
