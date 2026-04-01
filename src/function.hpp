//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/memory.hpp"
#include "concepts.hpp"
#include "pointer.hpp"
#include "sum.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "memory/new.hpp"

namespace micron
{

namespace except
{
struct bad_function_call {
  const char *
  what() const noexcept
  {
    return "micron::function: call on empty wrapper";
  }
};
};     // namespace except

inline constexpr usize __function_smallobj_size = 48;
inline constexpr usize __function_smallobj_align = alignof(void *);

template <typename R, typename... Args> struct __fn_vtable {
  R (*call)(const void *self, Args... args);
  void (*destroy)(void *self) noexcept;
  void (*copy)(void *dst, const void *src);           // nullptr if move-only
  void (*move_to)(void *dst, void *src) noexcept;     // always present
  // metadata
  const void *type_tag;     // unique per G, used by target<G>()
  bool is_noexcept;
  bool on_heap;
};

template <typename G>
inline const void *
__fn_type_tag() noexcept
{
  static const int __sentinel = 0;
  return &__sentinel;
}

// custom vtable for concrete callable type G
template <typename G, typename R, typename... Args>
const __fn_vtable<R, Args...> *
__make_vtable() noexcept
{
  static const __fn_vtable<R, Args...> vt
      = { // cast self back to G and invoke
          [](const void *self, Args... args) -> R { return (*static_cast<const G *>(self))(micron::forward<Args>(args)...); },
          // destroy
          [](void *self) noexcept { static_cast<G *>(self)->~G(); },
          // nullptr when G is not copy-constructible
          micron::is_copy_constructible_v<G> ? +[](void *dst, const void *src) { new (dst) G(*static_cast<const G *>(src)); }
                                             : static_cast<void (*)(void *, const void *)>(nullptr),
          // move_to
          [](void *dst, void *src) noexcept {
            new (dst) G(micron::move(*static_cast<G *>(src)));
            static_cast<G *>(src)->~G();
          },
          // type_tag
          __fn_type_tag<micron::decay_t<G>>(), noexcept(micron::declval<G>()(micron::declval<Args>()...)), false
        };
  return &vt;
}

template <typename> class function;

template <typename R, typename... Args> class function<R(Args...)>
{
  alignas(__function_smallobj_align) unsigned char __buf[__function_smallobj_size];
  void *__heap = nullptr;                            // non-null iff on heap
  const __fn_vtable<R, Args...> *__vt = nullptr;     // null iff empty

  void *
  __self() noexcept
  {
    return __heap ? __heap : static_cast<void *>(__buf);
  }

  const void *
  __self() const noexcept
  {
    return __heap ? __heap : static_cast<const void *>(__buf);
  }

  bool
  __empty() const noexcept
  {
    return __vt == nullptr;
  }

  void
  __destroy() noexcept
  {
    if ( __vt ) {
      __vt->destroy(__self());
      if ( __heap ) {
        ::operator delete(__heap);
        __heap = nullptr;
      }
      __vt = nullptr;
    }
  }

  template <typename G, typename... CArgs>
  void
  __emplace(CArgs &&...cargs)
  {
    using D = micron::decay_t<G>;
    __destroy();

    if constexpr ( sizeof(D) <= __function_smallobj_size && alignof(D) <= __function_smallobj_align ) {
      new (__buf) D(micron::forward<CArgs>(cargs)...);
      __heap = nullptr;
    } else {
      __heap = ::operator new(sizeof(D), static_cast<std::align_val_t>(alignof(D)));
      new (__heap) D(micron::forward<CArgs>(cargs)...);
    }
    __vt = __make_vtable<D, R, Args...>();
  }

public:
  using result_type = R;

  ~function() { __destroy(); }

  function() noexcept = default;

  function(nullptr_t) noexcept {}

  template <typename G>
    requires(!micron::same_as<micron::decay_t<G>, function> && micron::invocable<micron::decay_t<G>, Args...>
             && micron::is_convertible_v<micron::invoke_result_t<micron::decay_t<G>, Args...>, R>)
  function(G &&g)
  {
    __emplace<micron::decay_t<G>>(micron::forward<G>(g));
  }

  function(const function &o)
  {
    if ( !o.__empty() ) {
      if ( !o.__vt->copy )
        exc<except::bad_function_call>();     // stored type is move-only
      if ( o.__heap ) {
        __heap = ::operator new(__function_smallobj_size * 4);     // conservative
        o.__vt->copy(__heap, o.__heap);
      } else {
        o.__vt->copy(__buf, o.__buf);
        __heap = nullptr;
      }
      __vt = o.__vt;
    }
  }

  function(function &&o) noexcept
  {
    if ( !o.__empty() ) {
      if ( o.__heap ) {
        __heap = o.__heap;
        o.__heap = nullptr;
      } else {
        o.__vt->move_to(__buf, o.__buf);
        __heap = nullptr;
      }
      __vt = o.__vt;
      o.__vt = nullptr;
    }
  }

  function &
  operator=(const function &o)
  {
    if ( this != &o ) {
      function tmp(o);
      *this = micron::move(tmp);
    }
    return *this;
  }

  function &
  operator=(function &&o) noexcept
  {
    if ( this != &o ) {
      __destroy();
      if ( !o.__empty() ) {
        if ( o.__heap ) {
          __heap = o.__heap;
          o.__heap = nullptr;
        } else {
          o.__vt->move_to(__buf, o.__buf);
        }
        __vt = o.__vt;
        o.__vt = nullptr;
      }
    }
    return *this;
  }

  function &
  operator=(nullptr_t) noexcept
  {
    __destroy();
    return *this;
  }

  template <typename G>
    requires(!micron::same_as<micron::decay_t<G>, function> && micron::invocable<micron::decay_t<G>, Args...>
             && micron::is_convertible_v<micron::invoke_result_t<micron::decay_t<G>, Args...>, R>)
  function &
  operator=(G &&g)
  {
    function tmp(micron::forward<G>(g));
    *this = micron::move(tmp);
    return *this;
  }

  R
  operator()(Args... args)
  {
    if ( __empty() )
      exc<except::bad_function_call>();
    return __vt->call(__self(), micron::forward<Args>(args)...);
  }

  R
  operator()(Args... args) const
  {
    if ( __empty() )
      exc<except::bad_function_call>();
    return __vt->call(__self(), micron::forward<Args>(args)...);
  }

  explicit
  operator bool() const noexcept
  {
    return !__empty();
  }

  bool
  operator!() const noexcept
  {
    return __empty();
  }

  void
  swap(function &o) noexcept
  {
    function tmp(micron::move(*this));
    *this = micron::move(o);
    o = micron::move(tmp);
  }

  template <typename G>
  G *
  target() noexcept
  {
    if ( __empty() || __vt->type_tag != __fn_type_tag<micron::decay_t<G>>() )
      return nullptr;
    return static_cast<G *>(__self());
  }

  template <typename G>
  const G *
  target() const noexcept
  {
    if ( __empty() || __vt->type_tag != __fn_type_tag<micron::decay_t<G>>() )
      return nullptr;
    return static_cast<const G *>(__self());
  }

  bool
  is_noexcept() const noexcept
  {
    return __vt && __vt->is_noexcept;
  }
};

template <typename R, typename... Args>
void
swap(function<R(Args...)> &a, function<R(Args...)> &b) noexcept
{
  a.swap(b);
}

template <typename R, typename... Args>
bool
operator==(const function<R(Args...)> &f, nullptr_t) noexcept
{
  return !f;
}

template <typename R, typename... Args>
bool
operator==(nullptr_t, const function<R(Args...)> &f) noexcept
{
  return !f;
}

template <typename R, typename... Args>
bool
operator!=(const function<R(Args...)> &f, nullptr_t) noexcept
{
  return !!f;
}

template <typename R, typename... Args>
bool
operator!=(nullptr_t, const function<R(Args...)> &f) noexcept
{
  return !!f;
}

// eg Foo struct foo member
// auto f = micron::bind_method(&Foo::bar, &foo);
// f(21);

// Non-const member function.
template <typename R, typename C, typename... Args>
auto
bind_method(R (C::*mfp)(Args...), C *obj)
{
  return [mfp, obj](Args... args) -> R { return (obj->*mfp)(micron::forward<Args>(args)...); };
}

template <typename R, typename C, typename... Args>
auto
bind_method(R (C::*mfp)(Args...) const, const C *obj)
{
  return [mfp, obj](Args... args) -> R { return (obj->*mfp)(micron::forward<Args>(args)...); };
}

template <typename F, typename... Args>
concept is_callable = micron::invocable<F, Args...>;

template <typename F, typename R, typename... Args>
concept is_typed_function = micron::invocable<F, Args...> && micron::is_convertible_v<micron::invoke_result_t<F, Args...>, R>;

template <typename F>
concept is_nullary = micron::invocable<F>;

template <typename F, typename A, typename B>
concept is_arrow = is_typed_function<F, B, A>;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// combinators
//
// fmap  : ('a -> 'b) -> F 'a -> F 'b
// bind  : F 'a -> ('a -> F 'b) -> F 'b       (>>=)
// pure  : 'a -> F 'a                          (return / unit)
// ap    : F ('a -> 'b) -> F 'a -> F 'b        (<*>)

// fmap id : ('a -> 'b) -> 'a -> 'b
template <typename F, typename A>
  requires micron::invocable<F, A>
auto
fmap(F &&f, A &&a) -> micron::invoke_result_t<F, A>
{
  return micron::forward<F>(f)(micron::forward<A>(a));
}

// bind id : 'a -> ('a -> 'b) -> 'b
template <typename A, typename F>
  requires micron::invocable<F, A>
auto
bind(A &&a, F &&f) -> micron::invoke_result_t<F, A>
{
  return micron::forward<F>(f)(micron::forward<A>(a));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  option<T, F> --> OCaml `('a, 'b) either`  /  `'a option` (when F = unit)

// fmap for option<T, E>
// returns option<U, E> where U = invoke_result_t<Fn, T>
template <typename Fn, typename T, typename E>
  requires micron::invocable<Fn, T>
auto
fmap(Fn &&fn, const micron::option<T, E> &opt) -> micron::option<micron::invoke_result_t<Fn, T>, E>
{
  using U = micron::invoke_result_t<Fn, T>;
  if ( opt.is_first() )
    return micron::option<U, E>{ micron::forward<Fn>(fn)(opt.template cast<T>()) };
  else
    return micron::option<U, E>{ opt.template cast<E>() };
}

template <typename Fn, typename T, typename E>
  requires micron::invocable<Fn, T>
auto
fmap(Fn &&fn, micron::option<T, E> &&opt) -> micron::option<micron::invoke_result_t<Fn, T>, E>
{
  using U = micron::invoke_result_t<Fn, T>;
  if ( opt.is_first() )
    return micron::option<U, E>{ micron::forward<Fn>(fn)(micron::move(opt.template cast<T>())) };
  else
    return micron::option<U, E>{ micron::move(opt.template cast<E>()) };
}

template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, T> && micron::is_option<micron::invoke_result_t<Fn, T>>
auto
bind(const micron::option<T, E> &opt, Fn &&fn) -> micron::invoke_result_t<Fn, T>
{
  if ( opt.is_first() )
    return micron::forward<Fn>(fn)(opt.template cast<T>());
  else {
    using Ret = micron::invoke_result_t<Fn, T>;
    // Propagate the error branch — Ret must hold E.
    return Ret{ opt.template cast<E>() };
  }
}

template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, T> && micron::is_option<micron::invoke_result_t<Fn, T>>
auto
bind(micron::option<T, E> &&opt, Fn &&fn) -> micron::invoke_result_t<Fn, T>
{
  if ( opt.is_first() )
    return micron::forward<Fn>(fn)(micron::move(opt.template cast<T>()));
  else {
    using Ret = micron::invoke_result_t<Fn, T>;
    return Ret{ micron::move(opt.template cast<E>()) };
  }
}

template <typename E, typename T>
micron::option<micron::decay_t<T>, E>
pure(T &&v)
{
  return micron::option<micron::decay_t<T>, E>{ micron::forward<T>(v) };
}

template <typename Fn, typename T, typename E>
  requires micron::invocable<Fn, T>
auto
ap(const micron::option<Fn, E> &of, const micron::option<T, E> &ov) -> micron::option<micron::invoke_result_t<Fn, T>, E>
{
  using U = micron::invoke_result_t<Fn, T>;
  if ( of.is_first() && ov.is_first() )
    return micron::option<U, E>{ of.template cast<Fn>()(ov.template cast<T>()) };
  else if ( !of.is_first() )
    return micron::option<U, E>{ of.template cast<E>() };
  else
    return micron::option<U, E>{ ov.template cast<E>() };
}

// map2 : ('a -> 'b -> 'c) -> 'a option -> 'b option -> 'c option
// both branches must be first (ok) for fn to be applied; first error wins
template <typename Fn, typename T, typename U, typename E>
  requires micron::invocable<Fn, T, U>
auto
map2(Fn &&fn, const micron::option<T, E> &oa, const micron::option<U, E> &ob) -> micron::option<micron::invoke_result_t<Fn, T, U>, E>
{
  using V = micron::invoke_result_t<Fn, T, U>;
  if ( oa.is_first() && ob.is_first() )
    return micron::option<V, E>{ micron::forward<Fn>(fn)(oa.template cast<T>(), ob.template cast<U>()) };
  else if ( !oa.is_first() )
    return micron::option<V, E>{ oa.template cast<E>() };
  else
    return micron::option<V, E>{ ob.template cast<E>() };
}

template <typename Fn, typename T, typename U, typename E>
  requires micron::invocable<Fn, T, U>
auto
map2(Fn &&fn, micron::option<T, E> &&oa, micron::option<U, E> &&ob) -> micron::option<micron::invoke_result_t<Fn, T, U>, E>
{
  using V = micron::invoke_result_t<Fn, T, U>;
  if ( oa.is_first() && ob.is_first() )
    return micron::option<V, E>{ micron::forward<Fn>(fn)(micron::move(oa.template cast<T>()), micron::move(ob.template cast<U>())) };
  else if ( !oa.is_first() )
    return micron::option<V, E>{ micron::move(oa.template cast<E>()) };
  else
    return micron::option<V, E>{ micron::move(ob.template cast<E>()) };
}

// join/flatten : option<option<T,E>,E> -> option<T,E>
// collapses one layer of nesting; equivalent to bind(x, id)
template <typename T, typename E>
micron::option<T, E>
join(const micron::option<micron::option<T, E>, E> &outer)
{
  if ( outer.is_first() )
    return outer.template cast<micron::option<T, E>>();
  else
    return micron::option<T, E>{ outer.template cast<E>() };
}

template <typename T, typename E>
micron::option<T, E>
join(micron::option<micron::option<T, E>, E> &&outer)
{
  if ( outer.is_first() )
    return micron::move(outer.template cast<micron::option<T, E>>());
  else
    return micron::option<T, E>{ micron::move(outer.template cast<E>()) };
}

// value: 'a option -> default:'a -> 'a
// extract the first branch or return the supplied default
template <typename T, typename E>
T
value(const micron::option<T, E> &opt, T &&def)
{
  if ( opt.is_first() )
    return opt.template cast<T>();
  return micron::forward<T>(def);
}

template <typename T, typename E>
T
value(micron::option<T, E> &&opt, T &&def)
{
  if ( opt.is_first() )
    return micron::move(opt.template cast<T>());
  return micron::forward<T>(def);
}

// and_then: option<T,E> -> (T -> option<U,E>) -> option<U,E>
// alias for bind when the continuation is named more idiomatically
template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, T> && micron::is_option<micron::invoke_result_t<Fn, T>>
auto
and_then(const micron::option<T, E> &opt, Fn &&fn) -> micron::invoke_result_t<Fn, T>
{
  if ( opt.is_first() )
    return micron::forward<Fn>(fn)(opt.template cast<T>());
  else {
    using Ret = micron::invoke_result_t<Fn, T>;
    return Ret{ opt.template cast<E>() };
  }
}

template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, T> && micron::is_option<micron::invoke_result_t<Fn, T>>
auto
and_then(micron::option<T, E> &&opt, Fn &&fn) -> micron::invoke_result_t<Fn, T>
{
  if ( opt.is_first() )
    return micron::forward<Fn>(fn)(micron::move(opt.template cast<T>()));
  else {
    using Ret = micron::invoke_result_t<Fn, T>;
    return Ret{ micron::move(opt.template cast<E>()) };
  }
}

// or_else: option<T,E> -> (E -> option<T,F>) -> option<T,F>
// recover from the error branch; success passes through unchanged
template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, E> && micron::is_option<micron::invoke_result_t<Fn, E>>
auto
or_else(const micron::option<T, E> &opt, Fn &&fn) -> micron::invoke_result_t<Fn, E>
{
  using Ret = micron::invoke_result_t<Fn, E>;
  if ( opt.is_second() )
    return micron::forward<Fn>(fn)(opt.template cast<E>());
  else
    return Ret{ opt.template cast<T>() };
}

template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, E> && micron::is_option<micron::invoke_result_t<Fn, E>>
auto
or_else(micron::option<T, E> &&opt, Fn &&fn) -> micron::invoke_result_t<Fn, E>
{
  using Ret = micron::invoke_result_t<Fn, E>;
  if ( opt.is_second() )
    return micron::forward<Fn>(fn)(micron::move(opt.template cast<E>()));
  else
    return Ret{ micron::move(opt.template cast<T>()) };
}

// map_error: option<T,E> -> (E -> F) -> option<T,F>
// transform only the error branch, leaving the success branch unchanged
template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, E>
auto
map_error(const micron::option<T, E> &opt, Fn &&fn) -> micron::option<T, micron::invoke_result_t<Fn, E>>
{
  using F2 = micron::invoke_result_t<Fn, E>;
  if ( opt.is_first() )
    return micron::option<T, F2>{ opt.template cast<T>() };
  else
    return micron::option<T, F2>{ micron::forward<Fn>(fn)(opt.template cast<E>()) };
}

template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, E>
auto
map_error(micron::option<T, E> &&opt, Fn &&fn) -> micron::option<T, micron::invoke_result_t<Fn, E>>
{
  using F2 = micron::invoke_result_t<Fn, E>;
  if ( opt.is_first() )
    return micron::option<T, F2>{ micron::move(opt.template cast<T>()) };
  else
    return micron::option<T, F2>{ micron::forward<Fn>(fn)(micron::move(opt.template cast<E>())) };
}

// bind_error: option<T,E> -> (E -> option<T,F>) -> option<T,F>
// monadic bind over the error branch; lets you chain fallback strategies
template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, E> && micron::is_option<micron::invoke_result_t<Fn, E>>
auto
bind_error(const micron::option<T, E> &opt, Fn &&fn) -> micron::invoke_result_t<Fn, E>
{
  using Ret = micron::invoke_result_t<Fn, E>;
  if ( opt.is_second() )
    return micron::forward<Fn>(fn)(opt.template cast<E>());
  else
    return Ret{ opt.template cast<T>() };
}

template <typename T, typename E, typename Fn>
  requires micron::invocable<Fn, E> && micron::is_option<micron::invoke_result_t<Fn, E>>
auto
bind_error(micron::option<T, E> &&opt, Fn &&fn) -> micron::invoke_result_t<Fn, E>
{
  using Ret = micron::invoke_result_t<Fn, E>;
  if ( opt.is_second() )
    return micron::forward<Fn>(fn)(micron::move(opt.template cast<E>()));
  else
    return Ret{ micron::move(opt.template cast<T>()) };
}

// option<T,E> -> (T -> U) -> (E -> F) -> option<U,F>
// maps both branches simultaneously
template <typename T, typename E, typename FnT, typename FnE>
  requires fn_domain<FnT, T> && fn_domain<FnE, E>
auto
bimap(const micron::option<T, E> &opt, FnT &&ft, FnE &&fe)
    -> micron::option<micron::invoke_result_t<FnT, T>, micron::invoke_result_t<FnE, E>>
{
  using U = micron::invoke_result_t<FnT, T>;
  using F = micron::invoke_result_t<FnE, E>;
  if ( opt.is_first() )
    return micron::option<U, F>{ micron::forward<FnT>(ft)(opt.template cast<T>()) };
  else
    return micron::option<U, F>{ micron::forward<FnE>(fe)(opt.template cast<E>()) };
}

template <typename T, typename E, typename FnT, typename FnE>
  requires fn_domain<FnT, T> && fn_domain<FnE, E>
auto
bimap(micron::option<T, E> &&opt, FnT &&ft, FnE &&fe) -> micron::option<micron::invoke_result_t<FnT, T>, micron::invoke_result_t<FnE, E>>
{
  using U = micron::invoke_result_t<FnT, T>;
  using F = micron::invoke_result_t<FnE, E>;
  if ( opt.is_first() )
    return micron::option<U, F>{ micron::forward<FnT>(ft)(micron::move(opt.template cast<T>())) };
  else
    return micron::option<U, F>{ micron::forward<FnE>(fe)(micron::move(opt.template cast<E>())) };
}

// micron::function overload
template <typename T, typename E, typename U, typename F>
micron::option<U, F>
bimap(const micron::option<T, E> &opt, micron::function<U(T)> ft, micron::function<F(E)> fe)
{
  if ( opt.is_first() )
    return micron::option<U, F>{ ft(opt.template cast<T>()) };
  else
    return micron::option<U, F>{ fe(opt.template cast<E>()) };
}

// fold/match: option<T,E> -> (T -> R) -> (E -> R) -> R
// eliminates the option by applying the appropriate function
template <typename T, typename E, typename FnT, typename FnE>
  requires fn_domain<FnT, T> && fn_domain<FnE, E> && micron::is_same_v<micron::invoke_result_t<FnT, T>, micron::invoke_result_t<FnE, E>>
auto
fold(const micron::option<T, E> &opt, FnT &&ft, FnE &&fe) -> micron::invoke_result_t<FnT, T>
{
  if ( opt.is_first() )
    return micron::forward<FnT>(ft)(opt.template cast<T>());
  else
    return micron::forward<FnE>(fe)(opt.template cast<E>());
}

template <typename T, typename E, typename FnT, typename FnE>
  requires fn_domain<FnT, T> && fn_domain<FnE, E> && micron::is_same_v<micron::invoke_result_t<FnT, T>, micron::invoke_result_t<FnE, E>>
auto
fold(micron::option<T, E> &&opt, FnT &&ft, FnE &&fe) -> micron::invoke_result_t<FnT, T>
{
  if ( opt.is_first() )
    return micron::forward<FnT>(ft)(micron::move(opt.template cast<T>()));
  else
    return micron::forward<FnE>(fe)(micron::move(opt.template cast<E>()));
}

// micron::function overload
template <typename T, typename E, typename R>
R
fold(const micron::option<T, E> &opt, micron::function<R(T)> ft, micron::function<R(E)> fe)
{
  if ( opt.is_first() )
    return ft(opt.template cast<T>());
  else
    return fe(opt.template cast<E>());
}

// option<T,E> -> (T -> void) -> option<T,E>
// executes side-effect on first branch without modifying the option
template <typename T, typename E, typename Fn>
  requires fn_domain<Fn, T>
const micron::option<T, E> &
tap(const micron::option<T, E> &opt, Fn &&fn)
{
  if ( opt.is_first() )
    micron::forward<Fn>(fn)(opt.template cast<T>());
  return opt;
}

template <typename T, typename E, typename Fn>
  requires fn_domain<Fn, const T *> && (!fn_domain<Fn, T>)
const micron::option<T, E> &
tap(const micron::option<T, E> &opt, Fn &&fn)
{
  if ( opt.is_first() )
    micron::forward<Fn>(fn)(&opt.template cast<T>());
  return opt;
}

// tap_error: side-effect on error branch
template <typename T, typename E, typename Fn>
  requires fn_domain<Fn, E>
const micron::option<T, E> &
tap_error(const micron::option<T, E> &opt, Fn &&fn)
{
  if ( opt.is_second() )
    micron::forward<Fn>(fn)(opt.template cast<E>());
  return opt;
}

// (option<T,E>, option<U,E>) -> option<tuple<T,U>, E>
template <typename T, typename U, typename E>
micron::option<micron::tuple<T, U>, E>
sequence_pair(const micron::option<T, E> &oa, const micron::option<U, E> &ob)
{
  if ( !oa.is_first() )
    return micron::option<micron::tuple<T, U>, E>{ oa.template cast<E>() };
  if ( !ob.is_first() )
    return micron::option<micron::tuple<T, U>, E>{ ob.template cast<E>() };
  return micron::option<micron::tuple<T, U>, E>{ micron::make_tuple(oa.template cast<T>(), ob.template cast<U>()) };
}

template <typename T, typename U, typename E>
micron::option<micron::tuple<T, U>, E>
sequence_pair(micron::option<T, E> &&oa, micron::option<U, E> &&ob)
{
  if ( !oa.is_first() )
    return micron::option<micron::tuple<T, U>, E>{ micron::move(oa.template cast<E>()) };
  if ( !ob.is_first() )
    return micron::option<micron::tuple<T, U>, E>{ micron::move(ob.template cast<E>()) };
  return micron::option<micron::tuple<T, U>, E>{ micron::make_tuple(micron::move(oa.template cast<T>()),
                                                                    micron::move(ob.template cast<U>())) };
}

// map3: ('a -> 'b -> 'c -> 'd) -> opt 'a -> opt 'b -> opt 'c -> opt 'd
template <typename Fn, typename T, typename U, typename V, typename E>
  requires micron::is_invocable_v<Fn, T, U, V>
auto
map3(Fn &&fn, const micron::option<T, E> &oa, const micron::option<U, E> &ob, const micron::option<V, E> &oc)
    -> micron::option<micron::invoke_result_t<Fn, T, U, V>, E>
{
  using W = micron::invoke_result_t<Fn, T, U, V>;
  if ( !oa.is_first() )
    return micron::option<W, E>{ oa.template cast<E>() };
  if ( !ob.is_first() )
    return micron::option<W, E>{ ob.template cast<E>() };
  if ( !oc.is_first() )
    return micron::option<W, E>{ oc.template cast<E>() };
  return micron::option<W, E>{ micron::forward<Fn>(fn)(oa.template cast<T>(), ob.template cast<U>(), oc.template cast<V>()) };
}

// bool -> T -> option<T,E>
template <typename T, typename E>
micron::option<T, E>
when_first(bool cond, T &&v)
{
  if ( cond )
    return micron::option<T, E>{ micron::forward<T>(v) };
  return micron::option<T, E>{ E{} };
}

// bool -> T -> option<T,E>
// constructs first branch if condition is false
template <typename T, typename E>
micron::option<T, E>
unless_first(bool cond, T &&v)
{
  if ( !cond )
    return micron::option<T, E>{ micron::forward<T>(v) };
  return micron::option<T, E>{ E{} };
}

template <typename T, typename E, typename Fn>
  requires micron::is_invocable_v<Fn> && micron::is_same_v<micron::invoke_result_t<Fn>, T>
micron::option<T, E>
try_call(Fn &&fn) noexcept
{
  return micron::option<T, E>{ micron::forward<Fn>(fn)() };
}

// bool -> option<unit_t, E>  (where unit_t = micron::tuple<>)
template <typename E>
micron::option<micron::tuple<>, E>
from_bool(bool cond)
{
  if ( cond )
    return micron::option<micron::tuple<>, E>{ micron::tuple<>{} };
  return micron::option<micron::tuple<>, E>{ E{} };
}

// option<T,E> -> (T -> bool) -> option<T,E>
// keeps the first branch only if the predicate holds; otherwise converts to error
template <typename T, typename E, typename Fn>
  requires fn_predicate<Fn, T>
micron::option<T, E>
ensure(const micron::option<T, E> &opt, Fn &&fn, E err = E{})
{
  if ( !opt.is_first() )
    return opt;
  if ( micron::forward<Fn>(fn)(opt.template cast<T>()) )
    return opt;
  return micron::option<T, E>{ micron::move(err) };
}

template <typename T, typename E, typename Fn>
  requires fn_predicate<Fn, T>
micron::option<T, E>
ensure(micron::option<T, E> &&opt, Fn &&fn, E err = E{})
{
  if ( !opt.is_first() )
    return micron::move(opt);
  if ( micron::forward<Fn>(fn)(opt.template cast<T>()) )
    return micron::move(opt);
  return micron::option<T, E>{ micron::move(err) };
}

template <typename T, typename E>
micron::option<T, E>
ensure(const micron::option<T, E> &opt, micron::function<bool(T)> fn, E err = E{})
{
  if ( !opt.is_first() )
    return opt;
  if ( fn(opt.template cast<T>()) )
    return opt;
  return micron::option<T, E>{ micron::move(err) };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// monadics
//
// fmap f g   = f o g  (fn composition)
// bind g f   = ,\ x -> f (g x) x
// pure v     = ,\ _ -> v (const fn)

// fmap: (B -> C) -> function<B(A)> -> function<C(A)>
// (f o g)(x) = f(g(x))
template <typename F, typename R, typename A>
  requires micron::invocable<F, R>
auto
fmap(F &&f, micron::function<R(A)> g) -> micron::function<micron::invoke_result_t<F, R>(A)>
{
  using C = micron::invoke_result_t<F, R>;
  return micron::function<C(A)>{ [f = micron::forward<F>(f), g = micron::move(g)](A a) mutable -> C {
    return f(g(micron::forward<A>(a)));
  } };
}

// bind: function<B(A)> -> (B -> function<C(A)>) -> function<C(A)>
// (m >>= f) x = f (m x) x
template <typename R, typename A, typename F>
  requires micron::invocable<F, R> && micron::invocable<micron::invoke_result_t<F, R>, A>
auto
bind(micron::function<R(A)> g, F &&f) -> micron::function<micron::invoke_result_t<micron::invoke_result_t<F, R>, A>(A)>
{
  using C = micron::invoke_result_t<micron::invoke_result_t<F, R>, A>;
  return micron::function<C(A)>{ [g = micron::move(g), f = micron::forward<F>(f)](A a) mutable -> C {
    return f(g(a))(micron::forward<A>(a));
  } };
}

// pure: B -> function<B(A)>    (const fn, ignores its argument)
template <typename A, typename B>
micron::function<micron::decay_t<B>(A)>
pure(B &&v)
{
  return micron::function<micron::decay_t<B>(A)>{ [v = micron::forward<B>(v)](A) mutable -> micron::decay_t<B> { return v; } };
}

// ap: function<(B->C)(A)> -> function<B(A)> -> function<C(A)>
// S-combinator: (f <*> g) x = f x (g x)
template <typename R, typename A, typename B>
  requires micron::invocable<R, B>
auto
ap(micron::function<R(A)> ff, micron::function<B(A)> fg) -> micron::function<micron::invoke_result_t<R, B>(A)>
{
  using C = micron::invoke_result_t<R, B>;
  return micron::function<C(A)>{ [ff = micron::move(ff), fg = micron::move(fg)](A a) mutable -> C { return ff(a)(fg(a)); } };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ocaml style pipe |>
// ie let result = 3 |> string_of_int;;
//
//  x |> f |> g |> h in place of h(g(f(x)))

template <typename A, typename F>
  requires micron::invocable<F, A>
auto
operator|(A &&a, F &&f) -> micron::invoke_result_t<F, A>
{
  return micron::forward<F>(f)(micron::forward<A>(a));
}

//  (f << g)(x)  =  f(g(x))
//  (f >> g)(x)  =  g(f(x))

// right-to-left composition: (f << g)(x) = f(g(x))
template <typename F, typename G>
  requires micron::invocable<G>     // at least nullary; real args checked below
           || true                  // always enable, constraint on call site
auto
compose_rtl(F &&f, G &&g)
{
  return [f = micron::forward<F>(f),
          g = micron::forward<G>(g)](auto &&...args) mutable -> micron::invoke_result_t<F, micron::invoke_result_t<G, decltype(args)...>> {
    return f(g(micron::forward<decltype(args)>(args)...));
  };
}

template <typename F, typename G>
  requires micron::invocable<G>     // at least nullary; real args checked below
           || true                  // always enable, constraint on call site
auto
operator<<(F &&f, G &&g)
{
  return compose_rtl(micron::forward<F &&>(f), micron::forward<G &&>(g));
}

// left-to-right composition: (f >> g)(x) = g(f(x))
template <typename F, typename G>
auto
compose_ltr(F &&f, G &&g)
{
  return [f = micron::forward<F>(f),
          g = micron::forward<G>(g)](auto &&...args) mutable -> micron::invoke_result_t<G, micron::invoke_result_t<F, decltype(args)...>> {
    return g(f(micron::forward<decltype(args)>(args)...));
  };
}

template <typename F, typename G>
auto
operator>>(F &&f, G &&g)
{
  return compose_ltr(micron::forward<F &&>(f), micron::forward<G &&>(g));
}

namespace __impl
{
// recursive wrapper that captures already-bound arguments
template <typename F, typename... Bnd> struct __curried {
  F __fn;
  micron::tuple<Bnd...> __bound;

  template <typename Arg>
  auto
  operator()(Arg &&arg) &&
  {
    auto next_bound = micron::tuple_cat(micron::move(__bound), micron::make_tuple(micron::forward<Arg>(arg)));

    // if F is invocable with all accumulated args, call it
    if constexpr ( micron::invocable<F, Bnd..., Arg> ) {
      return micron::apply(micron::move(__fn), micron::move(next_bound));
    } else {
      using Next = __curried<F, Bnd..., micron::decay_t<Arg>>;
      return Next{ micron::move(__fn), micron::move(next_bound) };
    }
  }

  template <typename Arg>
  auto
  operator()(Arg &&arg) const &
  {
    auto next_bound = micron::tuple_cat(__bound, micron::make_tuple(micron::forward<Arg>(arg)));

    if constexpr ( micron::invocable<F, Bnd..., Arg> ) {
      return micron::apply(__fn, micron::move(next_bound));
    } else {
      using Next = __curried<F, Bnd..., micron::decay_t<Arg>>;
      return Next{ __fn, micron::move(next_bound) };
    }
  }
};

// stores a zero-argument callable and evaluates it at most once
template <typename F>
  requires micron::invocable<F>
struct __thunk {
  using value_type = micron::invoke_result_t<F>;

  mutable F __fn;
  mutable bool __evaluated = false;
  alignas(alignof(value_type)) mutable unsigned char __storage[sizeof(value_type)];

  explicit __thunk(F &&f) : __fn(micron::forward<F>(f)) {}

  __thunk(const __thunk &o) : __fn(o.__fn), __evaluated(false) {}

  __thunk(__thunk &&o) noexcept : __fn(micron::move(o.__fn)), __evaluated(false) {}

  ~__thunk()
  {
    if ( __evaluated )
      reinterpret_cast<value_type *>(__storage)->~value_type();
  }

  const value_type &
  force() const
  {
    if ( !__evaluated ) {
      new (__storage) value_type(__fn());
      __evaluated = true;
    }
    return *reinterpret_cast<const value_type *>(__storage);
  }

  const value_type &
  operator()() const
  {
    return force();
  }
};
};     // namespace __impl

template <typename F>
auto
curry(F &&f)
{
  return __impl::__curried<micron::decay_t<F>>{ micron::forward<F>(f), {} };
}

// partial : binds one or more leading arguments to an n-ary function
// partial(f, a, b)(c, d)  ==  f(a, b, c, d)
template <typename F, typename... BArgs>
auto
partial(F &&f, BArgs &&...bound)
{
  return [f = micron::forward<F>(f), tup = micron::make_tuple(micron::forward<BArgs>(bound)...)](
             auto &&...rest) mutable -> micron::invoke_result_t<F, BArgs..., decltype(rest)...> {
    return micron::apply(
        [&f, &rest...](auto &&...b) mutable -> micron::invoke_result_t<F, BArgs..., decltype(rest)...> {
          return f(micron::forward<decltype(b)>(b)..., micron::forward<decltype(rest)>(rest)...);
        },
        tup);
  };
}

// lazy : (() -> 'a) -> 'a Lazy.t
// wraps a zero-argument callable; the body is executed only on the first call to force() / operator()(), and the result is cached
// thereafter
template <typename F>
  requires micron::invocable<F>
auto
lazy(F &&f)
{
  return __impl::__thunk<micron::decay_t<F>>{ micron::forward<F>(f) };
}

// force: 'a Lazy.t -> 'a
// mirrors OCaml `Lazy.force`
template <typename F>
  requires micron::invocable<F>
const typename __impl::__thunk<F>::value_type &
force(const __impl::__thunk<F> &t)
{
  return t.force();
}

//  flip swaps the first two arguments of a binary function
//  ocaml `Fun.flip : ('a -> 'b -> 'c) -> 'b -> 'a -> 'c`

template <typename F>
auto
flip(F &&f)
{
  return [f = micron::forward<F>(f)](auto &&b, auto &&a) mutable -> micron::invoke_result_t<F, decltype(a), decltype(b)> {
    return f(micron::forward<decltype(a)>(a), micron::forward<decltype(b)>(b));
  };
}

//  ocaml `Fun.const : 'a -> 'b -> 'a`

template <typename A>
auto
const_fn(A &&a)
{
  return [a = micron::forward<A>(a)](auto &&...) mutable -> A & { return a; };
}

inline constexpr auto identity = [](auto &&x) -> decltype(auto) { return micron::forward<decltype(x)>(x); };

};     // namespace micron
