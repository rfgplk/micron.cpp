//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../closures.hpp"
#include "../concepts.hpp"
#include "../sum.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "bits.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// functional io glue

namespace micron
{
namespace io
{

// the io unit type: what void-returning callables map to inside option<_, error_t>
using unit_t = micron::tuple<>;

template<typename R> using __unit_if_void_t = micron::conditional_t<micron::is_void_v<R>, unit_t, micron::decay_t<R>>;

// fixed streaming window for read_with/write_with
inline constexpr usize fp_window = 16384;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fn shapes

// per-chunk consumer over a read-only window
template<typename Fn>
concept chunk_fn = requires(Fn fn, const byte *p, usize n) { fn(p, n); };

// streaming producer
template<typename Fn>
concept producer_fn = requires(Fn fn, byte *dst, usize cap) {
  { fn(dst, cap) } -> same_as<usize>;
};

// raw per-line consumer
template<typename Fn>
concept line_fn = requires(Fn fn, const char *s, usize n) { fn(s, n); };

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// callable detection
namespace __fn_impl
{
struct __call_probe {
  void operator()();      // participates in name lookup only
};

template<typename T> struct __call_mixin: T, __call_probe {
};

// resolves iff T contributes NO operator() of any kind
template<typename T>
concept __no_call_op = requires { &__call_mixin<T>::operator(); };
};      // namespace __fn_impl

// anything that is semantically a callable
// (monomorphic and generic lambdas, functors with overloaded/templated operator(), function (pointer)s, member-fn pointers)
// NOTE: a final functor whose only operator() is templated/overloaded and that is not nullary-invocable is undetectable and still goes in
// the TC tier
template<typename T>
concept fn_like
    = (micron::is_class_v<micron::remove_cvref_t<T>> && !micron::is_final_v<micron::remove_cvref_t<T>>
       && !__fn_impl::__no_call_op<micron::remove_cvref_t<T>>)
      || requires { &micron::remove_cvref_t<T>::operator(); }      // final monomorphic functors
      || micron::is_function_v<micron::remove_reference_t<T>>
      || (micron::is_pointer_v<micron::remove_cvref_t<T>> && micron::is_function_v<micron::remove_pointer_t<micron::remove_cvref_t<T>>>)
      || micron::is_member_function_pointer_v<micron::remove_cvref_t<T>>
      || micron::is_invocable_v<micron::remove_cvref_t<T>>;      // final nullary functors

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// signature deduction
// (monomorphic callables and plain functions only)

template<typename Fn>
concept fn_deducible
    = requires { &micron::remove_cvref_t<Fn>::operator(); } || micron::is_function_v<micron::remove_reference_t<Fn>>
      || (micron::is_pointer_v<micron::remove_cvref_t<Fn>> && micron::is_function_v<micron::remove_pointer_t<micron::remove_cvref_t<Fn>>>);

template<fn_deducible Fn> inline constexpr usize fn_arity_v = ::function_traits<micron::remove_cvref_t<Fn>>::arity;

template<fn_deducible Fn> using fn_ret_t = typename ::function_traits<micron::remove_cvref_t<Fn>>::return_type;

template<fn_deducible Fn> using fn_arg0_exact_t = typename ::function_traits<micron::remove_cvref_t<Fn>>::template arg_type<0>;

template<fn_deducible Fn> using fn_arg0_t = micron::remove_cvref_t<fn_arg0_exact_t<Fn>>;

// monadics
inline micron::option<max_t, io::error_t>
to_option(max_t n)
{
  if ( n < 0 ) [[unlikely]]
    return micron::option<max_t, io::error_t>{ io::error_t(static_cast<i32>(n)) };
  return micron::option<max_t, io::error_t>{ n };
}

};      // namespace io
};      // namespace micron
