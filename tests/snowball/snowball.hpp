//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/std.hpp"

#include "../../src/concepts.hpp"
#include "../../src/tuple.hpp"
#include "../../src/type_traits.hpp"

#include "../../src/string/strings.hpp"

#include "../../src/io/console.hpp"
#include "../../src/io/io.hpp"
#include "../../src/io/stdout.hpp"

#include "../../src/except.hpp"
#include "../../src/exit.hpp"

namespace snowball
{
using string_type = micron::string;

inline string_type __global_test_case{};
inline void (*__global_on_require)() = nullptr;
inline void (*__global_on_check)() = nullptr;

namespace config
{
constexpr static const bool __default_print_stack = true;
constexpr static const bool __default_abort_on_require = true;
constexpr static const bool __default_else_throw_on_require = false;
};     // namespace config

// start out functions

[[noreturn]] inline void
__exit(void)
{
  micron::sys_exit(6);
}

[[noreturn]] inline void
__abort(void)
{
  if constexpr ( config::__default_abort_on_require ) {
    __exit();
  } else if constexpr ( config::__default_else_throw_on_require ) {
    throw micron::runtime{ "snowball exception in abort()" };
  }
  micron::sys_exit(6);
}

template <typename... T>
inline __attribute__((always_inline)) void
__print(const T &...args)
{
  micron::io::print(args...);
}

template <typename... T>
inline __attribute__((always_inline)) void
__print_error(const T &...args)
{
  if ( __global_test_case.size() ) {
    micron::io::print("\033[34m:: Test case error...\033[0m\n\r");
    micron::io::print("\033[90m");
    micron::io::print("[ ", __global_test_case, " ]");
    micron::io::print("\033[0m");
    micron::io::print("\n\r");
  }
  micron::io::print(args...);
}

// end out functions

inline void
__print_stack()
{
#if defined(__micron_arch_arm32)
  // arm32 thumb pins r7 as FP, but -Ofast omits the FP and frees r7 for
  // general allocation. Lowering __builtin_frame_address(0) under LTO then
  // emits an r7 reference into contexts where r7 is busy, producing
  // "r7 cannot be used in 'asm' here". Stack tracing requires a real FP, so
  // skip the walk on arm32.
  __print("Start of call stack:\n\r");
  __print("(unavailable on arm32; build without -fomit-frame-pointer for traces)\n\r");
#else
  constexpr int max_frames = 64;
  void *buffer[max_frames];
  int n = 0;

  void **fp = static_cast<void **>(__builtin_frame_address(0));
  for ( int i = 0; i < max_frames && fp; ++i ) {
    void *next_fp = fp[0];
    void *next_ret = fp[1];
    if ( !next_ret ) break;
    if ( reinterpret_cast<umax_t>(next_fp) <= reinterpret_cast<umax_t>(fp) ) break;
    buffer[n++] = next_ret;
    fp = static_cast<void **>(next_fp);
  }

  __print("Start of call stack:\n\r");
  if ( n == 0 ) {
    __print("(unavailable; compile with -fno-omit-frame-pointer for traces)\n\r");
    return;
  }
  for ( int i = 0; i < n; ++i ) {
    __print("#");
    __print(i);
    __print(": ");
    __print(buffer[i]);
    __print("\n\r");
  }
#endif
}

inline void
stdout(const char *str)
{
  __print(str);
  __print("\n");
}

inline void
verify_debug(void)
{
#if defined(__OPTIMIZE__) || (defined(__has_feature) && __has_feature(debug_info))
  __print("\033[34msnowball warning:\033[0m the executable *wasn't* compiled in debug mode (-g).\n\r");
#endif
}

#define enable_scope(x) if constexpr ( true )
#define disable_scope(x) if constexpr ( false )

inline __attribute__((always_inline)) void
should_print_stack(void)
{
  if constexpr ( config::__default_print_stack ) __print_stack();
}

// helpers

template <typename T> struct function_traits;

// free functions
template <typename R, typename... Args> struct function_traits<R(Args...)> {
  using return_type = R;
  static constexpr size_t arity = sizeof...(Args);
  using args_tuple = micron::tuple<Args...>;

  template <size_t N> using arg_type = micron::tuple_element_t<N, args_tuple>;
};

// function pointers
template <typename R, typename... Args> struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {
};

// member functions
template <typename C, typename R, typename... Args> struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {
};

template <typename C, typename R, typename... Args> struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
};

// callable objects (functors/lambdas)
template <typename F> struct function_traits : function_traits<decltype(&F::operator())> {
};

template <typename Tuple, typename F, size_t... I>
constexpr void
for_each_type_impl(F &&f, micron::index_sequence<I...>)
{
  (f.template operator()<micron::tuple_element_t<I, Tuple>>(), ...);
}

template <typename Tuple, typename F>
constexpr void
for_each_type(F &&f)
{
  for_each_type_impl<Tuple>(micron::forward<F>(f), micron::make_index_sequence<micron::tuple_size_v<Tuple>>{});
}

template <typename F, typename... T> constexpr bool all_invocable_t = (micron::is_invocable_v<F, T> && ...);

template <typename E, typename Fn, typename... Ts>
bool
__check_eq(const E &e, Fn &&fn, Ts... args)
{
  return fn(micron::forward<Ts>(args)...) == e;
}

// variadic check: returns true only if all pack members satisfy __check_eq
template <typename E, typename Fn, typename... Ts>
constexpr bool
__check(const E &expected, Fn &&fn, Ts &&...__args)
{
  // this has to be like this so it properly binds to any type of function passed in
  return (...
          && micron::apply([&](auto &&...args) { return __check_eq(expected, fn, micron::forward<decltype(args)>(args)...); },
                           micron::forward<Ts>(__args)));
}

template <typename E, typename Fn, typename... Ts>
constexpr bool
check_false(const E &expected, Fn &&fn, Ts &&...__args)
{
  return (...
          && micron::apply([&](auto &&...args) { return !__check_eq(expected, fn, micron::forward<decltype(args)>(args)...); },
                           micron::forward<Ts>(__args)));
}

template <typename E, typename Fn, typename... Ts>
constexpr void
call_fn(const E &expected, Fn &&fn, Ts &&...__args)
{
  (...
   && micron::apply([&](auto &&...args) { return !__check(expected, fn, micron::forward<decltype(args)>(args)...); },
                    micron::forward<Ts>(__args)));
}

// global setting helpers

inline void
require_callback(void (*fn)())
{
  if ( fn != nullptr ) __global_on_require = fn;
}

inline void
check_callback(void (*fn)())
{
  if ( fn != nullptr ) __global_on_check = fn;
}

inline void
__require_clbck(void)
{
  if ( __global_on_require != nullptr ) __global_on_require();
}

inline void
__check_clbck(void)
{
  if ( __global_on_check != nullptr ) __global_on_check();
}

template <typename T>
  requires(micron::is_object_v<T>)
string_type
test_case(const T &str)
{
  __global_test_case = str;
  return __global_test_case;
}

inline string_type
test_case(const char *str)
{
  __global_test_case = str;
  return __global_test_case;
}

inline void
end_test_case(void)
{
  __global_test_case.clear();
}

[[noreturn]] inline void
early_end(void)
{
  __abort();
}

template <typename... T>
void
print(const T &...p)
{
  __print("\033[34msnowball msg:\033[0m ");
  __print(p...);
  __print("\n");
}

inline void
print(const char *p)
{
  __print("\033[34msnowball msg:\033[0m ");
  __print(p);
  __print("\n\r");
}

template <typename T>
[[noreturn]] void
error(const T &p)
{
  __print_error("\033[34msnowball error():\033[0m ");
  __print_error(p);
  __print_error("\n\r");
  __require_clbck();
  __abort();
}

[[noreturn]] inline void
error(const char *ptr)
{
  __print_error("\033[34msnowball error():\033[0m ");
  __print_error(ptr);
  __print_error("\r\n");
  __require_clbck();
  __abort();
}

// main unit testing functions
template <typename... FArgs, typename... Args>
void
require_distinct(bool (*fn)(FArgs...), Args &&...args)
{
  if ( fn(micron::forward<Args>(args)...) == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename... Args>
void
require(bool v)
{
  if ( v == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename... Args>
void
require(bool (*fn)(Args...), Args &&...args)
{
  if ( fn(micron::forward<Args>(args)...) == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

// main unit testing functions
template <typename... FArgs, typename... Args>
void
require_print(bool (*fn)(FArgs...), Args &&...args)
{
  bool _t = fn(micron::forward<Args>(args)...);
  print(_t);
  if ( _t == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename... Args>
void
require_print(bool (&fn)(Args...), Args &&...args)
{
  bool _t = fn(micron::forward<Args>(args)...);
  print(_t);
  if ( _t == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

inline void
require_distinct(const bool a, const bool b)
{
  if ( a == b ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was wrong.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

inline void
require(const bool a, const bool b)
{
  if ( a != b ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was wrong.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

inline void
require_false(const bool expected_output)
{
  if ( expected_output != false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

inline void
require_true(const bool expected_output)
{
  if ( !expected_output ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename A, typename B>
void
require(const A &_a, const B &_b)
{
  if ( _a != _b ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename A, typename B>
void
require_greater(const A &_a, const B &_b)
{
  if ( _a <= _b ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename A, typename B>
void
require_smaller(const A &_a, const B &_b)
{
  if ( _a >= _b ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename A, typename B, typename Fn, typename... Args>
void
require_cmp(const A &_a, const B &_b, Fn &&f, Args &&...args)
{
  if ( f(_a, _b) == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

// spec. for void functions
template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
require(Fn &&fn, const Dt_Ex &expected_output)
{
  if ( (*fn)() != expected_output ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires(((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
require(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !__check(expected_output, micron::forward<Fn>(fn), micron::make_tuple(inputs...)) ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
}

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex, typename Fn_g>
void
require(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output, Fn_g &&getting_method)
{
  (object.*fn)(input);
  if ( (object.*getting_method)() != expected_output ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex>
void
require(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output)
{
  if ( (object.*fn)(input) != expected_output ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

// spec. for void functions
template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
require_false(Fn &&fn, const Dt_Ex &expected_output)
{
  if ( (*fn)() == expected_output ) {
    __print_error("\033[34msnowball require_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires(((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
require_false(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !check_false(expected_output, micron::forward<Fn>(fn), micron::make_tuple(inputs...)) ) {
    __print_error("\033[34msnowball require_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
}

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex, typename Fn_g>
void
require_false(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output, Fn_g &&getting_method)
{
  (object.*fn)(input);
  if ( (object.*getting_method)() == expected_output ) {
    __print_error("\033[34msnowball require_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex>
void
require_false(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output)
{
  if ( (object.*fn)(input) == expected_output ) {
    __print_error("\033[34msnowball require_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

// throw variants
template <typename Fn>
void
require_throw(Fn &&fn)
{
  try {
    fn();
    __print_error("\033[34msnowball require_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  } catch ( ... ) {
    return;
  }
};

template <typename Fn>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
require_throw(Fn &&fn)
{
  try {
    (*fn)();
    __print_error("\033[34msnowball require_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  } catch ( ... ) {
    return;
  }
};

template <typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
require_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
    __print_error("\033[34msnowball require_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  } catch ( ... ) {
    return;
  }
};

template <typename E, typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
require_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
    should_print_stack();
    __print_error("\033[34msnowball require_throw() failure:\033[0m nothing was thrown");
    __require_clbck();
    __abort();
  } catch ( const E &ex ) {
    __print("\033[34msnowball require_throw(): ");
    __print(ex.what());
    __print("\n\r");
    return;
  } catch ( ... ) {
    __print_error("\033[34msnowball require_throw() failure:\033[0m unexpected exception was thrown\n\r");
    __require_clbck();
    __abort();
  }
}

template <typename Fn>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
require_nothrow(Fn &&fn)
{
  try {
    (*fn)();
  } catch ( ... ) {
    __print_error("\033[34msnowball require_nothrow() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template <typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
require_nothrow(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
  } catch ( ... ) {
    __print_error("\033[34msnowball require_nothrow() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

// end requires
// start checks
// the only difference between a require and a check is that checks don't abort

inline void
check(const bool expected_output)
{
  if ( expected_output == false ) {
    __print_error("\033[34msnowball check() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

// spec. for void functions
template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
check(Fn &&fn, const Dt_Ex &expected_output)
{
  if ( (*fn)() != expected_output ) {
    __print_error("\033[34msnowball check() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires(((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
check(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !__check(expected_output, micron::forward<Fn>(fn), micron::make_tuple(inputs...)) ) {
    __print_error("\033[34msnowball check() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __check_clbck();
  }
}

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex, typename Fn_g>
void
check(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output, Fn_g &&getting_method)
{
  (object.*fn)(input);
  if ( (object.*getting_method)() != expected_output ) {
    __print_error("\033[34msnowball check() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex>
void
check(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output)
{
  if ( (object.*fn)(input) != expected_output ) {
    __print_error("\033[34msnowball check() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

template <typename Object, typename Fn, typename Dt_In>
void
check_nothrow(Object &object, Fn &&fn, const Dt_In &input)
{
  try {
    (object.*fn)(input);
  } catch ( ... ) {
    __print_error("\033[34msnowball check_nothrow() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

template <typename Object, typename Fn>
void
check_nothrow(Object &object, Fn &&fn)
{
  try {
    (object.*fn)();
  } catch ( ... ) {
    __print_error("\033[34msnowball check_nothrow() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

// spec. for void functions
template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
check_false(Fn &&fn, const Dt_Ex &expected_output)
{
  if ( (*fn)() == expected_output ) {
    __print_error("\033[34msnowball check_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

template <typename Fn, typename Dt_Ex, typename... Dt_In>
  requires(((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
check_false(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !check_false(expected_output, micron::forward<Fn>(fn), micron::make_tuple(inputs...)) ) {
    __print_error("\033[34msnowball check_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __check_clbck();
  }
}

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex, typename Fn_g>
void
check_false(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output, Fn_g &&getting_method)
{
  (object.*fn)(input);
  if ( (object.*getting_method)() == expected_output ) {
    __print_error("\033[34msnowball check_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

template <typename Object, typename Fn, typename Dt_In, typename Dt_Ex>
void
check_false(Object &object, Fn &&fn, const Dt_In &input, const Dt_Ex &expected_output)
{
  if ( (object.*fn)(input) == expected_output ) {
    __print_error("\033[34msnowball check_false() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __check_clbck();
  }
};

// throw variants

template <typename Fn>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
check_throw(Fn &&fn)
{
  try {
    (*fn)();
    __print_error("\033[34msnowball check_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __check_clbck();
  } catch ( ... ) {
    return;
  }
};

template <typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
check_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
    __print_error("\033[34msnowball check_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __check_clbck();
  } catch ( ... ) {
    return;
  }
};

template <typename E, typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
check_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
    should_print_stack();
    __print_error("\033[34msnowball check_throw() failure:\033[0m nothing was thrown");
    __check_clbck();
  } catch ( const E &ex ) {
    __print("\033[34msnowball check_throw(): ");
    __print(ex.what());
    __print("\n\r");
    return;
  } catch ( ... ) {
    __print_error("\033[34msnowball check_throw() failure:\033[0m unexpected exception was thrown\n\r");
    __check_clbck();
  }
};

template <typename Fn>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn>)
void
check_nothrow(Fn &&fn)
{
  try {
    (*fn)();
  } catch ( ... ) {
    __print_error("\033[34msnowball check_nothrow() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __check_clbck();
    return;
  }
};

template <typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
check_nothrow(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
  } catch ( ... ) {
    __print_error("\033[34msnowball check_nothrow() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __check_clbck();
    return;
  }
};

template <typename E, typename Fn, typename... Args>
  requires((micron::is_function_v<micron::remove_pointer_t<Fn>> or micron::is_function_v<Fn>) && micron::is_invocable_v<Fn, Args...>)
void
check_nothrow(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(micron::forward<Args>(args)...);
  } catch ( const E &ex ) {
    __print("\033[34msnowball check_throw(): ");
    __print(ex.what());
    __print("\n\r");
    return;
  } catch ( ... ) {
    __print_error("\033[34msnowball check_throw() failure:\033[0m unexpected exception was thrown\n\r");
    __check_clbck();
  }
};

namespace __impl
{
inline u64
__xorshift64(u64 &s)
{
  s ^= s << 13;
  s ^= s >> 7;
  s ^= s << 17;
  return s;
}

[[gnu::always_inline]] inline u64
__cycle_counter() noexcept
{
#if defined(__micron_arch_amd64)
  u32 lo = 0, hi = 0;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (u64(hi) << 32) | u64(lo);
#elif defined(__micron_arch_arm64)
  u64 v;
  asm volatile("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#elif defined(__micron_arch_arm32)
  u32 lo, hi;
  asm volatile("mrrc p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
  return (u64(hi) << 32) | u64(lo);
#else
  return 0;
#endif
}
};     // namespace __impl

template <typename Fn>
void
fuzz(Fn &&fn, size_t cnt)
{
  using traits = function_traits<decltype(&fn)>;
  if constexpr ( traits::arity == 1 ) {
    typename traits::template arg_type<0> var{};

    static u64 __seed = []() {
      u64 s = __impl::__cycle_counter();
      return s ? s : 0xdeadbeefULL;
    }();

    for ( size_t i = 0; i < cnt; ++i ) {
      u64 r = __impl::__xorshift64(__seed) & ((1ULL << 25) - 1);
      var = static_cast<typename traits::template arg_type<0>>(r);
      fn(var);
    }
  }
}
};     // namespace snowball

namespace sb = snowball;
