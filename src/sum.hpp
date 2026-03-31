//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "concepts.hpp"
#include "tuple.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "memory/actions.hpp"

#include "numerics.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// sum types/tagged unions

namespace micron
{

template <typename T> struct tag {
  using type = T;
};

namespace __impl
{

template <usize N, usize... Rest> struct max_of_values {
  static constexpr usize value = (N > max_of_values<Rest...>::value) ? N : max_of_values<Rest...>::value;
};

template <usize N> struct max_of_values<N> {
  static constexpr usize value = N;
};

template <typename T, typename... Ts> struct type_in_pack : micron::bool_constant<(micron::same_as<T, Ts> || ...)> {
};

template <typename T, typename... Ts> inline constexpr bool type_in_pack_v = type_in_pack<T, Ts...>::value;

template <typename T, typename... Ts> struct any_type_idx;

// template <typename T, typename H, typename... Ts>
// struct any_type_idx<T, H, Ts...> : micron::integral_constant<usize, micron::same_as<T, H> ? 0 : (1 + any_type_idx<T, Ts...>::value)> {
// };

template <typename T, typename... Ts> struct any_type_idx<T, T, Ts...> : integral_constant<usize, 0> {
};

// match not found -> recurse
template <typename T, typename H, typename... Ts>
struct any_type_idx<T, H, Ts...> : integral_constant<usize, 1 + any_type_idx<T, Ts...>::value> {
};

// empty pack -> error
template <typename T> struct any_type_idx<T> {
  static_assert(!sizeof(T), "type not in any<Ts...>");
};

template <typename T, typename... Ts> inline constexpr usize any_type_idx_v = any_type_idx<T, Ts...>::value;

};     // namespace __impl

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  any<Ts...>  —  cpp sum type/tagged union
//
//  a value holds __exactly one__ alternative, tracked by a usize discriminant (npos/empty/valueless)
//
//  storage is always stack-allocated; capacity = element-wise max of sizeof / alignof over all Ts

template <typename... Ts>
  requires(sizeof...(Ts) > 0)
class any
{
  static constexpr usize npos = micron::numeric_limits<usize>::max();
  static constexpr usize __storage_size = __impl::max_of_values<sizeof(Ts)...>::value;
  static constexpr usize __storage_align = __impl::max_of_values<alignof(Ts)...>::value;

  alignas(__storage_align) unsigned char storage[__storage_size];
  usize __idx = npos;

  void (*__destroy)(void *) noexcept = nullptr;
  void (*__copy)(void *, const void *) = nullptr;
  void (*__move)(void *, void *) noexcept = nullptr;

  template <typename T>
  static void
  destroy_fn(void *p) noexcept
  {
    static_cast<T *>(p)->~T();
  }

  template <typename T>
  static void
  copy_fn(void *dst, const void *src)
  {
    new (dst) T(*static_cast<const T *>(src));
  }

  template <typename T>
  static void
  move_fn(void *dst, void *src) noexcept
  {
    new (dst) T(micron::move(*static_cast<T *>(src)));
    destroy_fn<T>(src);
  }

  void
  reset_impl(void) noexcept
  {
    if ( __idx != npos ) {
      __destroy(storage);
      __idx = npos;
      __destroy = nullptr;
      __copy = nullptr;
      __move = nullptr;
    }
  }

  // comptime index of U in Ts...
  template <typename U> static constexpr usize idx_of = __impl::any_type_idx_v<U, Ts...>;

public:
  ~any(void) { reset_impl(); }

  template <typename T>
    requires __impl::type_in_pack_v<micron::decay_t<T>, Ts...>
  explicit any(tag<T>)
  {
    emplace<micron::decay_t<T>>();
  }

  template <typename T, typename A0, typename... Args>
    requires __impl::type_in_pack_v<micron::decay_t<T>, Ts...>
  any(tag<T>, A0 &&a0, Args &&...args)
  {
    emplace<micron::decay_t<T>>(micron::forward<A0>(a0), micron::forward<Args>(args)...);
  }

  template <typename T>
    requires(__impl::type_in_pack_v<micron::decay_t<T>, Ts...> && !micron::same_as<micron::decay_t<T>, any>)
  any(T &&v)
  {
    emplace<micron::decay_t<T>>(micron::forward<T>(v));
  }

  constexpr any(void) noexcept = default;

  any(const any &o)
  {
    if ( o.__idx != npos ) {
      o.__copy(storage, o.storage);
      __idx = o.__idx;
      __destroy = o.__destroy;
      __copy = o.__copy;
      __move = o.__move;
    }
  }

  any(any &&o) noexcept
  {
    if ( o.__idx != npos ) {
      o.__move(storage, o.storage);
      __idx = o.__idx;
      __destroy = o.__destroy;
      __copy = o.__copy;
      __move = o.__move;
      o.__idx = npos;
      o.__destroy = nullptr;
      o.__copy = nullptr;
      o.__move = nullptr;
    }
  }

  template <typename T, typename... Args>
    requires(__impl::type_in_pack_v<micron::decay_t<T>, Ts...>)
  micron::decay_t<T> &
  emplace(Args &&...args)
  {
    using U = micron::decay_t<T>;
    reset_impl();
    new (storage) U(micron::forward<Args>(args)...);
    __idx = __impl::any_type_idx_v<U, Ts...>;
    __destroy = &destroy_fn<U>;
    __copy = &copy_fn<U>;
    __move = &move_fn<U>;
    return *reinterpret_cast<U *>(storage);
  }

  void
  reset(void) noexcept
  {
    reset_impl();
  }

  bool
  has_value(void) const noexcept
  {
    return __idx != npos;
  }

  usize
  index(void) const noexcept
  {
    return __idx;
  }

  template <typename T>
  bool
  is(void) const noexcept
  {
    return __idx == idx_of<micron::decay_t<T>>;
  }

  // allow casting to anything explictly
  template <typename T>
  micron::decay_t<T> &
  cast(void)
  {
    return *reinterpret_cast<micron::decay_t<T> *>(storage);
  }

  // allow casting to anything explictly
  template <typename T>
  const micron::decay_t<T> &
  cast(void) const
  {
    return *reinterpret_cast<const micron::decay_t<T> *>(storage);
  }

  template <typename T>
    requires(__impl::type_in_pack_v<micron::decay_t<T>, Ts...> && !micron::same_as<micron::decay_t<T>, any>)
  any &
  operator=(T &&v)
  {
    emplace<micron::decay_t<T>>(micron::forward<T>(v));
    return *this;
  }

  template <typename T>
    requires __impl::type_in_pack_v<micron::decay_t<T>, Ts...>
  any &
  operator=(tag<T>)
  {
    emplace<micron::decay_t<T>>();
    return *this;
  }

  any &
  operator=(const any &o)
  {
    if ( this != &o ) {
      reset_impl();
      if ( o.__idx != npos ) {
        o.__copy(storage, o.storage);
        __idx = o.__idx;
        __destroy = o.__destroy;
        __copy = o.__copy;
        __move = o.__move;
      }
    }
    return *this;
  }

  any &
  operator=(any &&o) noexcept
  {
    if ( this != &o ) {
      reset_impl();
      if ( o.__idx != npos ) {
        o.__move(storage, o.storage);
        __idx = o.__idx;
        __destroy = o.__destroy;
        __copy = o.__copy;
        __move = o.__move;
        o.__idx = npos;
        o.__destroy = nullptr;
        o.__copy = nullptr;
        o.__move = nullptr;
      }
    }
    return *this;
  }

  template <typename T>
    requires(__impl::type_in_pack_v<T, Ts...>)
  operator T &() &
  {
    return cast<T>();
  }

  template <typename T>
    requires(__impl::type_in_pack_v<T, Ts...>)
  operator const T &() const &
  {
    return cast<T>();
  }

  template <typename T>
    requires(__impl::type_in_pack_v<T, Ts...>)
  operator T() &&
  {
    return micron::move(cast<T>());
  }

  /*template <typename T>
    requires(__impl::type_in_pack_v<micron::decay_t<T>, Ts...>)
  operator micron::decay_t<T> &() &
  {
    return cast<micron::decay_t<T>>();
  }

  template <typename T>
    requires(__impl::type_in_pack_v<micron::decay_t<T>, Ts...>)
  operator const micron::decay_t<T> &() const &
  {
    return cast<micron::decay_t<T>>();
  }

  template <typename T>
    requires(__impl::type_in_pack_v<micron::decay_t<T>, Ts...>)
  operator micron::decay_t<T>() &&
  {
    return micron::move(cast<micron::decay_t<T>>());
  }*/
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// option<T, F>  binary sum type/tunion of exactly two alternatives
//  equivalent to any<T,F> but uses a 3-valued enum discriminant
//
//  T and F must be distinct types!

template <typename T, typename F>
  requires(micron::distinct<T, F>)
class option
{
  static constexpr usize storage_size = sizeof(T) > sizeof(F) ? sizeof(T) : sizeof(F);
  static constexpr usize storage_aln = alignof(T) > alignof(F) ? alignof(T) : alignof(F);

  alignas(storage_aln) unsigned char storage[storage_size];

  enum class which : unsigned char { none = 0, first = 1, second = 2 } __active = which::none;

  void (*__destroy)(void *) noexcept = nullptr;
  void (*__copy)(void *, const void *) = nullptr;
  void (*__move)(void *, void *) noexcept = nullptr;

  template <typename U>
  static void
  destroy_fn(void *p) noexcept
  {
    static_cast<U *>(p)->~U();
  }

  template <typename U>
  static void
  copy_fn(void *dst, const void *src)
  {
    new (dst) U(*static_cast<const U *>(src));
  }

  template <typename U>
  static void
  move_fn(void *dst, void *src) noexcept
  {
    new (dst) U(micron::move(*static_cast<U *>(src)));
    destroy_fn<U>(src);
  }

  void
  reset_impl(void) noexcept
  {
    if ( __active != which::none ) {
      __destroy(storage);
      __active = which::none;
      __destroy = nullptr;
      __copy = nullptr;
      __move = nullptr;
    }
  }

  template <typename U>
  static constexpr which
  which_of(void) noexcept
  {
    if constexpr ( micron::same_as<micron::decay_t<U>, T> )
      return which::first;
    else
      return which::second;
  }

public:
  ~option() { reset_impl(); }

  explicit option(tag<T>) { emplace<T>(); }

  explicit option(tag<F>) { emplace<F>(); }

  // tag + args
  template <typename... Args> option(tag<T>, Args &&...args) { emplace<T>(micron::forward<Args>(args)...); }

  template <typename... Args> option(tag<F>, Args &&...args) { emplace<F>(micron::forward<Args>(args)...); }

  // NOTE: use tag<> on ambiguous conversions
  option(const T &v) { emplace<T>(v); }

  option(T &&v) { emplace<T>(micron::move(v)); }

  option(const F &v) { emplace<F>(v); }

  option(F &&v) { emplace<F>(micron::move(v)); }

  constexpr option() noexcept = default;

  option(const option &o)
  {
    if ( o.__active != which::none ) {
      o.__copy(storage, o.storage);
      __active = o.__active;
      __destroy = o.__destroy;
      __copy = o.__copy;
      __move = o.__move;
    }
  }

  option(option &&o) noexcept
  {
    if ( o.__active != which::none ) {
      o.__move(storage, o.storage);
      __active = o.__active;
      __destroy = o.__destroy;
      __copy = o.__copy;
      __move = o.__move;
      o.__active = which::none;
      o.__destroy = nullptr;
      o.__copy = nullptr;
      o.__move = nullptr;
    }
  }

  template <typename U, typename... Args>
    requires(micron::same_as<micron::decay_t<U>, T> or micron::same_as<micron::decay_t<U>, F>)
  micron::decay_t<U> &
  emplace(Args &&...args)
  {
    using D = micron::decay_t<U>;
    reset_impl();
    new (storage) D(micron::forward<Args>(args)...);
    __active = which_of<D>();
    __destroy = &destroy_fn<D>;
    __copy = &copy_fn<D>;
    __move = &move_fn<D>;
    return *reinterpret_cast<D *>(storage);
  }

  void
  reset(void) noexcept
  {
    reset_impl();
  }

  bool
  has_value() const noexcept
  {
    return __active != which::none;
  }

  bool
  is_first() const noexcept
  {
    return __active == which::first;
  }     // active == T

  bool
  is_second() const noexcept
  {
    return __active == which::second;
  }     // active == F

  // allow any
  template <typename U>
  bool
  is() const noexcept
  {
    return __active == which_of<micron::decay_t<U>>();
  }

  template <typename U>
    requires(micron::same_as<micron::decay_t<U>, T> or micron::same_as<micron::decay_t<U>, F>)
  micron::decay_t<U> &
  cast()
  {
    return *reinterpret_cast<micron::decay_t<U> *>(storage);
  }

  template <typename U>
    requires(micron::same_as<micron::decay_t<U>, T> or micron::same_as<micron::decay_t<U>, F>)
  const micron::decay_t<U> &
  cast() const
  {
    return *reinterpret_cast<const micron::decay_t<U> *>(storage);
  }

  option &
  operator=(tag<T>)
  {
    emplace<T>();
    return *this;
  }

  option &
  operator=(tag<F>)
  {
    emplace<F>();
    return *this;
  }

  option &
  operator=(const T &v)
  {
    emplace<T>(v);
    return *this;
  }

  option &
  operator=(T &&v)
  {
    emplace<T>(micron::move(v));
    return *this;
  }

  option &
  operator=(const F &v)
  {
    emplace<F>(v);
    return *this;
  }

  option &
  operator=(F &&v)
  {
    emplace<F>(micron::move(v));
    return *this;
  }

  option &
  operator=(const option &o)
  {
    if ( this != &o ) {
      reset_impl();
      if ( o.__active != which::none ) {
        o.__copy(storage, o.storage);
        __active = o.__active;
        __destroy = o.__destroy;
        __copy = o.__copy;
        __move = o.__move;
      }
    }
    return *this;
  }

  option &
  operator=(option &&o) noexcept
  {
    if ( this != &o ) {
      reset_impl();
      if ( o.__active != which::none ) {
        o.__move(storage, o.storage);
        __active = o.__active;
        __destroy = o.__destroy;
        __copy = o.__copy;
        __move = o.__move;
        o.__active = which::none;
        o.__destroy = nullptr;
        o.__copy = nullptr;
        o.__move = nullptr;
      }
    }
    return *this;
  }
};
};     // namespace micron
