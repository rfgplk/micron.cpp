//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/memory.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "memory/actions.hpp"
#include "sync/invoke.hpp"

#include "__special/initializer_list"

namespace micron
{
template <typename T, typename F> struct pair {
  T a;     // or first
  F b;     // or second
  pair() : a(T()), b(F()) {}
  template <typename C>
    requires((micron::is_same_v<T, C>) or (micron::is_same_v<F, C>) or ((micron::is_convertible_v<T, C>) or micron::is_convertible_v<F, C>))
  pair(std::initializer_list<C> &&lst)
  {
    size_t i = 0;
    for ( C val : lst ) {
      if ( (i++) == 0 )
        a = static_cast<T>(micron::move(val));
      if ( (i++) == 1 )
        b = static_cast<F>(micron::move(val));
      if ( (i++) > 1 )
        break;
    }
  }
  template <typename C>
    requires((micron::is_same_v<T, C>) or (micron::is_same_v<F, C>) or ((micron::is_convertible_v<T, C>) or micron::is_convertible_v<F, C>))
  pair &
  operator=(std::initializer_list<C> &&lst)
  {
    size_t i = 0;
    for ( C val : lst ) {
      if ( (i++) == 0 )
        a = static_cast<T>(micron::move(val));
      if ( (i++) == 1 )
        b = static_cast<F>(micron::move(val));
      if ( (i++) > 1 )
        break;
    }
  }
  template <typename K, typename L>
    requires(micron::is_convertible_v<T, K> and micron::is_convertible_v<F, L>)
  pair(const K &x, const L &y) : a(static_cast<T>(x)), b(static_cast<F>(y))
  {
  }
  template <typename K, typename L>
    requires((micron::is_convertible_v<T, K> and micron::is_convertible_v<F, L>)
             and (!micron::is_same_v<T, K> and !micron::is_same_v<F, L>))
  pair(K &&x, L &&y) : a(micron::move(x)), b(micron::move(y))
  {
  }
  pair(T &&x, F &&y) : a(micron::move(x)), b(micron::move(y)) {}
  pair(const T &x, const F &y) : a(x), b(y) {}
  template <typename K, typename L> pair(K &&x, L &&y) : a(micron::move(x)), b(micron::move(y)) {}
  pair(const pair &o) : a(o.a), b(o.b) {}
  template <typename K, typename L> pair(const pair<K, L> &o) : a(static_cast<K>(o.a)), b(static_cast<L>(o.b)) {}
  pair(pair &&o) : a(micron::move(o.a)), b(micron::move(o.b))
  {
    if constexpr ( micron::is_class_v<T> )
      o.a.~T();
    else
      o.a = 0x0;
    if constexpr ( micron::is_class_v<F> )
      o.b.~F();
    else
      o.b = 0x0;
  }
  template <typename K, typename L> pair(pair<K, L> &&o) : a(micron::move(o.a)), b(micron::move(o.b))
  {
    if constexpr ( micron::is_class_v<T> )
      o.a.~T();
    else
      o.a = 0x0;
    if constexpr ( micron::is_class_v<F> )
      o.b.~F();
    else
      o.b = 0x0;
  }
  pair &
  operator=(const pair &o)
  {
    a = o.a;
    b = o.b;
    return *this;
  }
  template <typename K, typename L>
  pair &
  operator=(const pair<K, L> &o)
  {
    a = o.a;
    b = o.b;
    return *this;
  }
  pair &
  operator=(pair &&o)
  {
    a = micron::move(o.a);
    b = micron::move(o.b);
    return *this;
  }
  template <typename K, typename L>
  pair &
  operator=(pair<K, L> &&o)
  {
    a = micron::move(o.a);
    b = micron::move(o.b);
    return *this;
  }
  template <typename K = T>
    requires(micron::is_convertible_v<T, K>)
  pair &
  operator=(const K &x)
  {
    a = static_cast<T>(x);
    return *this;
  }

  template <typename L = F>
    requires(micron::is_convertible_v<F, L>)
  pair &
  operator=(const L &y)
  {
    b = static_cast<F>(y);
    return *this;
  }
  pair &
  operator=(T &&x)
  {
    a = micron::move(x);
    return *this;
  }
  template <typename L = F>
    requires(!micron::same_as<T, F>)
  pair &
  operator=(L &&y)
  {
    b = micron::move(y);
    return *this;
  }
  pair
  get(void) const
  {
    return pair(*this);
  }

  ~pair() = default;
};
template <typename C>
micron::pair<C, C>
tie(std::initializer_list<C> &&lst)
{
  micron::pair<C, C> c(micron::move(lst));
  return c;
}
template <typename C, typename D>
micron::pair<C, D>
tie(const C &c, const D &d)
{
  micron::pair<C, D> p(c, d);
  return p;
}
template <typename C, typename D>
micron::pair<C, D>
tie(C &&c, D &&d)
{
  micron::pair<C, D> p(micron::move(c), micron::move(d));
  return p;
}

template <typename... Ts> class tuple;

template <size_t I, typename T> struct tuple_element;

template <typename T> struct tuple_size;

template <size_t I, typename T, bool = micron::is_empty_v<T> && !micron::is_final_v<T>> class tuple_leaf
{
  T __value;

public:
  constexpr tuple_leaf() = default;

  template <typename U>
    requires micron::is_constructible_v<T, U>
  constexpr explicit tuple_leaf(U &&v) : __value(micron::forward<U>(v))
  {
  }

  constexpr T &
  get() & noexcept
  {
    return __value;
  }
  constexpr const T &
  get() const & noexcept
  {
    return __value;
  }
  constexpr T &&
  get() && noexcept
  {
    return micron::move(__value);
  }
  constexpr const T &&
  get() const && noexcept
  {
    return micron::move(__value);
  }
};

template <size_t I, typename T> class tuple_leaf<I, T, true> : private T
{
public:
  constexpr tuple_leaf() = default;

  template <typename U>
    requires micron::is_constructible_v<T, U>
  constexpr explicit tuple_leaf(U &&v) : T(micron::forward<U>(v))
  {
  }

  constexpr T &
  get() & noexcept
  {
    return *this;
  }
  constexpr const T &
  get() const & noexcept
  {
    return *this;
  }
  constexpr T &&
  get() && noexcept
  {
    return micron::move(*this);
  }
  constexpr const T &&
  get() const && noexcept
  {
    return micron::move(*this);
  }
};

template <typename Indices, typename... Ts> class tuple_impl;

template <size_t... Indices, typename... Ts> class tuple_impl<index_sequence<Indices...>, Ts...> : public tuple_leaf<Indices, Ts>...
{
public:
  constexpr tuple_impl() = default;

  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) && (micron::is_constructible_v<Ts, Us> && ...)
  constexpr explicit tuple_impl(Us &&...args) : tuple_leaf<Indices, Ts>(micron::forward<Us>(args))...
  {
  }
};

template <typename... Ts> class tuple : public tuple_impl<index_sequence_for<Ts...>, Ts...>
{
  using base_type = tuple_impl<index_sequence_for<Ts...>, Ts...>;

public:
  constexpr tuple() = default;

  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) && (micron::is_constructible_v<Ts, Us> && ...)
  constexpr explicit(!(micron::is_convertible_v<Us, Ts> && ...)) tuple(Us &&...args) : base_type(micron::forward<Us>(args)...)
  {
  }

  constexpr tuple(const tuple &) = default;

  constexpr tuple(tuple &&) = default;

  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) && (micron::is_constructible_v<Ts, const Us &> && ...)
  constexpr explicit(!(micron::is_convertible_v<const Us &, Ts> && ...)) tuple(const tuple<Us...> &other)
      : base_type(static_cast<const tuple_impl<index_sequence_for<Us...>, Us...> &>(other))
  {
  }

  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) && (micron::is_constructible_v<Ts, Us> && ...)
  constexpr explicit(!(micron::is_convertible_v<Us, Ts> && ...)) tuple(tuple<Us...> &&other)
      : base_type(static_cast<tuple_impl<index_sequence_for<Us...>, Us...> &&>(other))
  {
  }

  constexpr tuple &operator=(const tuple &) = default;

  constexpr tuple &operator=(tuple &&) = default;

  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) && (micron::is_assignable_v<Ts &, const Us &> && ...)
  constexpr tuple &
  operator=(const tuple<Us...> &other)
  {
    assign_impl(other, index_sequence_for<Ts...>{});
    return *this;
  }

  template <typename... Us>
    requires(sizeof...(Us) == sizeof...(Ts)) && (micron::is_assignable_v<Ts &, Us> && ...)
  constexpr tuple &
  operator=(tuple<Us...> &&other)
  {
    assign_impl(micron::move(other), index_sequence_for<Ts...>{});
    return *this;
  }

  constexpr void
  swap(tuple &other) noexcept((micron::is_nothrow_swappable_v<Ts> && ...))
    requires(micron::is_swappable_v<Ts> && ...)
  {
    swap_impl(other, index_sequence_for<Ts...>{});
  }

private:
  template <typename... Us, size_t... Is>
  constexpr void
  assign_impl(const tuple<Us...> &other, index_sequence<Is...>)
  {
    ((get<Is>(*this) = get<Is>(other)), ...);
  }

  template <typename... Us, size_t... Is>
  constexpr void
  assign_impl(tuple<Us...> &&other, index_sequence<Is...>)
  {
    ((get<Is>(*this) = get<Is>(micron::move(other))), ...);
  }

  template <size_t... Is>
  constexpr void
  swap_impl(tuple &other, index_sequence<Is...>)
  {
    using micron::swap;
    ((swap(get<Is>(*this), get<Is>(other))), ...);
  }
};

template <typename... Ts> tuple(Ts...) -> tuple<Ts...>;

template <> class tuple<>
{
public:
  constexpr tuple() = default;
  constexpr void
  swap(tuple &) noexcept
  {
  }
};

template <typename T> struct tuple_size;

template <typename T>
  requires requires { typename tuple_size<T>::type; }
struct tuple_size<const T> : micron::integral_constant<size_t, tuple_size<T>::value> {
};

template <typename T>
  requires requires { typename tuple_size<T>::type; }
struct tuple_size<volatile T> : micron::integral_constant<size_t, tuple_size<T>::value> {
};

template <typename T>
  requires requires { typename tuple_size<T>::type; }
struct tuple_size<const volatile T> : micron::integral_constant<size_t, tuple_size<T>::value> {
};

template <typename... Ts> struct tuple_size<tuple<Ts...>> : micron::integral_constant<size_t, sizeof...(Ts)> {
};

template <typename T> inline constexpr size_t tuple_size_v = tuple_size<T>::value;

template <size_t I, typename T> struct tuple_element;

template <size_t I, typename T> struct tuple_element<I, const T> {
  using type = micron::add_const_t<typename tuple_element<I, T>::type>;
};

template <size_t I, typename T> struct tuple_element<I, volatile T> {
  using type = micron::add_volatile_t<typename tuple_element<I, T>::type>;
};

template <size_t I, typename T> struct tuple_element<I, const volatile T> {
  using type = micron::add_cv_t<typename tuple_element<I, T>::type>;
};

template <size_t I, typename Head, typename... Tail> struct tuple_element<I, tuple<Head, Tail...>> : tuple_element<I - 1, tuple<Tail...>> {
};

template <typename Head, typename... Tail> struct tuple_element<0, tuple<Head, Tail...>> {
  using type = Head;
};

template <size_t I, typename T> using tuple_element_t = typename tuple_element<I, T>::type;

template <size_t I, typename... Ts>
constexpr tuple_element_t<I, tuple<Ts...>> &
get(tuple<Ts...> &t) noexcept
{
  using leaf_type = tuple_leaf<I, tuple_element_t<I, tuple<Ts...>>>;
  return static_cast<leaf_type &>(t).get();
}

template <size_t I, typename... Ts>
constexpr const tuple_element_t<I, tuple<Ts...>> &
get(const tuple<Ts...> &t) noexcept
{
  using leaf_type = tuple_leaf<I, tuple_element_t<I, tuple<Ts...>>>;
  return static_cast<const leaf_type &>(t).get();
}

template <size_t I, typename... Ts>
constexpr tuple_element_t<I, tuple<Ts...>> &&
get(tuple<Ts...> &&t) noexcept
{
  using leaf_type = tuple_leaf<I, tuple_element_t<I, tuple<Ts...>>>;
  return static_cast<leaf_type &&>(t).get();
}

template <size_t I, typename... Ts>
constexpr const tuple_element_t<I, tuple<Ts...>> &&
get(const tuple<Ts...> &&t) noexcept
{
  using leaf_type = tuple_leaf<I, tuple_element_t<I, tuple<Ts...>>>;
  return static_cast<const leaf_type &&>(t).get();
}

namespace impl
{
template <typename T, typename... Ts> struct type_index;

template <typename T, typename... Ts> struct type_index<T, T, Ts...> : micron::integral_constant<size_t, 0> {
};

template <typename T, typename U, typename... Ts>
struct type_index<T, U, Ts...> : micron::integral_constant<size_t, 1 + type_index<T, Ts...>::value> {
};

template <typename T, typename... Ts> inline constexpr size_t type_index_v = type_index<T, Ts...>::value;

template <typename T, typename... Ts> struct count_type : micron::integral_constant<size_t, (micron::is_same_v<T, Ts> + ...)> {
};

template <typename T, typename... Ts> inline constexpr size_t count_type_v = count_type<T, Ts...>::value;
}

template <typename T, typename... Ts>
  requires(impl::count_type_v<T, Ts...> == 1)
constexpr T &
get(tuple<Ts...> &t) noexcept
{
  return get<impl::type_index_v<T, Ts...>>(t);
}

template <typename T, typename... Ts>
  requires(impl::count_type_v<T, Ts...> == 1)
constexpr const T &
get(const tuple<Ts...> &t) noexcept
{
  return get<impl::type_index_v<T, Ts...>>(t);
}

template <typename T, typename... Ts>
  requires(impl::count_type_v<T, Ts...> == 1)
constexpr T &&
get(tuple<Ts...> &&t) noexcept
{
  return get<impl::type_index_v<T, Ts...>>(micron::move(t));
}

template <typename T, typename... Ts>
  requires(impl::count_type_v<T, Ts...> == 1)
constexpr const T &&
get(const tuple<Ts...> &&t) noexcept
{
  return get<impl::type_index_v<T, Ts...>>(micron::move(t));
}

namespace impl
{
template <typename T> struct unwrap_refwrapper {
  using type = T;
};

template <typename T> struct unwrap_refwrapper<micron::reference_wrapper<T>> {
  using type = T &;
};

template <typename T> using unwrap_decay_t = typename unwrap_refwrapper<micron::decay_t<T>>::type;
}

template <typename... Ts>
constexpr tuple<impl::unwrap_decay_t<Ts>...>
make_tuple(Ts &&...args)
{
  return tuple<impl::unwrap_decay_t<Ts>...>(micron::forward<Ts>(args)...);
}

template <typename... Ts>
constexpr tuple<Ts &...>
tie(Ts &...args) noexcept
{
  return tuple<Ts &...>(args...);
}

template <typename... Ts>
constexpr tuple<Ts &&...>
forward_as_tuple(Ts &&...args) noexcept
{
  return tuple<Ts &&...>(micron::forward<Ts>(args)...);
}

namespace impl
{
template <typename... Tps> struct tuple_cat_result;

template <> struct tuple_cat_result<> {
  using type = tuple<>;
};

template <typename... Ts> struct tuple_cat_result<tuple<Ts...>> {
  using type = tuple<Ts...>;
};

template <typename... Ts, typename... Us, typename... Rest>
struct tuple_cat_result<tuple<Ts...>, tuple<Us...>, Rest...> : tuple_cat_result<tuple<Ts..., Us...>, Rest...> {
};

template <typename... Tps> using tuple_cat_result_t = typename tuple_cat_result<micron::remove_cvref_t<Tps>...>::type;

template <typename Tp, size_t... Is>
constexpr auto
tuple_to_args_impl(Tp &&t, index_sequence<Is...>)
{
  return forward_as_tuple(get<Is>(micron::forward<Tp>(t))...);
}

template <typename Tp>
constexpr auto
tuple_to_args(Tp &&t)
{
  return tuple_to_args_impl(micron::forward<Tp>(t), make_index_sequence<tuple_size_v<micron::remove_cvref_t<Tp>>>{});
}

template <typename Result, typename Tp, size_t... Is>
constexpr Result
make_from_tuple_impl(Tp &&t, index_sequence<Is...>)
{
  return Result(get<Is>(micron::forward<Tp>(t))...);
}
}

template <typename... Tps>
constexpr impl::tuple_cat_result_t<Tps...>
tuple_cat(Tps &&...tuples)
{
  auto all_args = make_tuple(impl::tuple_to_args(micron::forward<Tps>(tuples))...);

  return impl::make_from_tuple_impl<impl::tuple_cat_result_t<Tps...>>(all_args, make_index_sequence<tuple_size_v<decltype(all_args)>>{});
}

namespace impl
{
template <typename... Ts, typename... Us, size_t... Is>
constexpr bool
tuple_eq_impl(const tuple<Ts...> &lhs, const tuple<Us...> &rhs, index_sequence<Is...>)
{
  return ((get<Is>(lhs) == get<Is>(rhs)) && ...);
}

template <typename... Ts, typename... Us, size_t... Is>
constexpr bool
tuple_ne_impl(const tuple<Ts...> &lhs, const tuple<Us...> &rhs, index_sequence<Is...>)
{
  return ((get<Is>(lhs) != get<Is>(rhs)) || ...);
}

template <typename... Ts, typename... Us, size_t... Is>
constexpr bool
tuple_lt_impl(const tuple<Ts...> &lhs, const tuple<Us...> &rhs, index_sequence<Is...>)
{
  bool result = false;
  bool done = false;

  (void)((done
          || ((get<Is>(lhs) < get<Is>(rhs))   ? (result = true, done = true)
              : (get<Is>(rhs) < get<Is>(lhs)) ? (done = true)
                                              : false))
         || ...);

  return result;
}

template <typename... Ts, typename... Us, size_t... Is>
constexpr bool
tuple_le_impl(const tuple<Ts...> &lhs, const tuple<Us...> &rhs, index_sequence<Is...>)
{
  return !tuple_lt_impl(rhs, lhs, index_sequence<Is...>{});
}

template <typename... Ts, typename... Us, size_t... Is>
constexpr bool
tuple_gt_impl(const tuple<Ts...> &lhs, const tuple<Us...> &rhs, index_sequence<Is...>)
{
  return tuple_lt_impl(rhs, lhs, index_sequence<Is...>{});
}

template <typename... Ts, typename... Us, size_t... Is>
constexpr bool
tuple_ge_impl(const tuple<Ts...> &lhs, const tuple<Us...> &rhs, index_sequence<Is...>)
{
  return !tuple_lt_impl(lhs, rhs, index_sequence<Is...>{});
}
}

template <typename... Ts, typename... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
constexpr bool
operator==(const tuple<Ts...> &lhs, const tuple<Us...> &rhs)
{
  return impl::tuple_eq_impl(lhs, rhs, index_sequence_for<Ts...>{});
}

template <typename... Ts, typename... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
constexpr bool
operator!=(const tuple<Ts...> &lhs, const tuple<Us...> &rhs)
{
  return impl::tuple_ne_impl(lhs, rhs, index_sequence_for<Ts...>{});
}

template <typename... Ts, typename... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
constexpr bool
operator<(const tuple<Ts...> &lhs, const tuple<Us...> &rhs)
{
  return impl::tuple_lt_impl(lhs, rhs, index_sequence_for<Ts...>{});
}

template <typename... Ts, typename... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
constexpr bool
operator<=(const tuple<Ts...> &lhs, const tuple<Us...> &rhs)
{
  return impl::tuple_le_impl(lhs, rhs, index_sequence_for<Ts...>{});
}

template <typename... Ts, typename... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
constexpr bool
operator>(const tuple<Ts...> &lhs, const tuple<Us...> &rhs)
{
  return impl::tuple_gt_impl(lhs, rhs, index_sequence_for<Ts...>{});
}

template <typename... Ts, typename... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
constexpr bool
operator>=(const tuple<Ts...> &lhs, const tuple<Us...> &rhs)
{
  return impl::tuple_ge_impl(lhs, rhs, index_sequence_for<Ts...>{});
}

template <typename... Ts>
  requires(micron::is_swappable_v<Ts> && ...)
constexpr void
swap(tuple<Ts...> &lhs, tuple<Ts...> &rhs) noexcept(noexcept(lhs.swap(rhs)))
{
  lhs.swap(rhs);
}

namespace impl
{
template <typename T>
concept has_tuple_element = requires {
  typename tuple_size<T>::type;
  requires micron::is_integral_v<decltype(tuple_size_v<T>)>;
};

template <typename T, size_t I>
concept has_tuple_element_at = requires {
  typename tuple_element_t<I, T>;
  { get<I>(micron::declval<T>()) } -> micron::convertible_to<const tuple_element_t<I, T> &>;
};
}

template <typename T>
concept tuple_like = impl::has_tuple_element<micron::remove_cvref_t<T>>;

template <typename T>
concept pair_like = tuple_like<T> && tuple_size_v<micron::remove_cvref_t<T>> == 2;

namespace impl
{
template <typename F, typename Tp, size_t... Is>
constexpr decltype(auto)
apply_impl(F &&f, Tp &&t, index_sequence<Is...>)
{
  return micron::invoke(micron::forward<F>(f), get<Is>(micron::forward<Tp>(t))...);
}
}

template <typename F, typename Tp>
constexpr decltype(auto)
apply(F &&f, Tp &&t)
{
  return impl::apply_impl(micron::forward<F>(f), micron::forward<Tp>(t), make_index_sequence<tuple_size_v<micron::remove_cvref_t<Tp>>>{});
}

template <typename T, typename Tp>
constexpr T
make_from_tuple(Tp &&t)
{
  return impl::make_from_tuple_impl<T>(micron::forward<Tp>(t), make_index_sequence<tuple_size_v<micron::remove_cvref_t<Tp>>>{});
}
};
