//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <tuple>
#include <type_traits>
#include <utility>

#include <exception>
#include <execinfo.h>     // TODO: add Windows spec.
#include <stdexcept>
#include <string>

namespace snowball
{
using string_type = std::string;

string_type __global_test_case = "";
void (*__global_on_require)() = nullptr;
void (*__global_on_check)() = nullptr;

namespace config
{
constexpr static const bool __default_print_stack = true;
constexpr static const bool __default_abort_on_require = true;
constexpr static const bool __default_else_throw_on_require = false;
};

// start out functions

__attribute__((noreturn)) void
__exit(void)
{
  __builtin_exit(6);
}

void __attribute__((noreturn))
__abort(void)
{
  if constexpr ( config::__default_abort_on_require ) {
    __exit();
  } else if constexpr ( config::__default_else_throw_on_require ) {
    throw std::runtime_error{ "snowball exception in abort()" };
  }
}

// this is here so you can swap func's out with custom ones
inline __attribute__((always_inline)) void
__print_error(const char *str)
{
  if ( __global_test_case.size() ) {
    std::fprintf(stderr, "\033[34m:: Test case error...\033[0m\n\r");
    std::fprintf(stderr, "\033[90m");
    std::fprintf(stderr, "[ %s ]", __global_test_case.c_str());
    std::fprintf(stderr, "\033[0m");
    std::fprintf(stderr, "\n\r");
  }
  std::fprintf(stderr, str);
}

inline __attribute__((always_inline)) void
__print(const char *str)
{
  std::fprintf(stdout, str);
}

template <typename T>
inline __attribute__((always_inline)) void
__print(const char *str, T t)
{
  std::fprintf(stdout, str, t);
}

// end out functions

void
__print_stack()
{
  constexpr int max_frames = 64;
  void *buffer[max_frames];

  int nptrs = backtrace(buffer, max_frames);
  char **symbols = backtrace_symbols(buffer, nptrs);
  if ( !symbols )
    return;

  __print("Start of call stack:\n\r");
  for ( int i = 0; i < nptrs; ++i ) {
    __print("#");
    __print("%d: ", i);
    __print(symbols[i]);
    __print("\n\r");
  }
  std::free(symbols);
}

void
stdout(const char *str)
{
  __print(str);
  __print("\n");
}
void
verify_debug(void)
{
#if defined(__OPTIMIZE__) || __has_feature(debug_info)
  __print("\033[34msnowball warning:\033[0m the executable *wasn't* compiled in debug mode (-g).\n\r");
#endif
}

#define enable_scope(x) if constexpr ( true )
#define disable_scope(x) if constexpr ( false )

inline __attribute__((always_inline)) void
should_print_stack(void)
{
  if constexpr ( config::__default_print_stack )
    __print_stack();
}

// helpers

template <typename T> struct function_traits;

// free functions
template <typename R, typename... Args> struct function_traits<R(Args...)> {
  using return_type = R;
  static constexpr std::size_t arity = sizeof...(Args);
  using args_tuple = std::tuple<Args...>;

  template <std::size_t N> using arg_type = typename std::tuple_element<N, args_tuple>::type;
};

// function pointers
template <typename R, typename... Args> struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {
};

// member functions
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
};

// callable objects (functors/lambdas)
template <typename F> struct function_traits : function_traits<decltype(&F::operator())> {
};

template <typename Tuple, typename F, std::size_t... I>
constexpr void
for_each_type_impl(F &&f, std::index_sequence<I...>)
{
  (f.template operator()<std::tuple_element_t<I, Tuple>>(), ...);
}

template <typename Tuple, typename F>
constexpr void
for_each_type(F &&f)
{
  for_each_type_impl<Tuple>(std::forward<F>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename F, typename... T> constexpr bool all_invocable_t = (std::is_invocable_v<F, T> && ...);

template <typename E, typename Fn, typename... Ts>
bool
__check_eq(const E &e, Fn &&fn, Ts... args)
{
  return fn(std::forward<Ts>(args)...) == e;
}
// variadic check: returns true only if all pack members satisfy __check_eq
template <typename E, typename Fn, typename... Ts>
constexpr bool
__check(const E &expected, Fn &&fn, Ts &&...__args)
{
  // this has to be like this so it properly binds to any type of function passed in
  return (...
          && std::apply([&](auto &&...args) { return __check_eq(expected, fn, std::forward<decltype(args)>(args)...); },
                        std::forward<Ts>(__args)));
}

template <typename E, typename Fn, typename... Ts>
constexpr bool
check_false(const E &expected, Fn &&fn, Ts &&...__args)
{
  return (...
          && std::apply([&](auto &&...args) { return !__check_eq(expected, fn, std::forward<decltype(args)>(args)...); },
                        std::forward<Ts>(__args)));
}
template <typename E, typename Fn, typename... Ts>
constexpr void
call_fn(const E &expected, Fn &&fn, Ts &&...__args)
{
  (...
   && std::apply([&](auto &&...args) { return !__check(expected, fn, std::forward<decltype(args)>(args)...); },
                 std::forward<Ts>(__args)));
}

// global setting helpers

void
require_callback(void (*fn)())
{
  if ( fn != nullptr )
    __global_on_require = fn;
}
void
check_callback(void (*fn)())
{
  if ( fn != nullptr )
    __global_on_check = fn;
}

void
__require_clbck(void)
{
  if ( __global_on_require != nullptr )
    __global_on_require();
}

void
__check_clbck(void)
{
  if ( __global_on_check != nullptr )
    __global_on_check();
}

template <typename T>
  requires(std::is_object_v<T>)
string_type
test_case(const T &str)
{
  __global_test_case = str;
  return __global_test_case;
}

string_type
test_case(const char *str)
{
  __global_test_case = str;
  return __global_test_case;
}

void
end_test_case(void)
{
  __global_test_case.clear();
}

void __attribute__((noreturn))
early_end(void)
{
  __abort();
}

template <typename... T>
void
print(const T &...p)
{
  std::cout << "\033[34msnowball msg:\033[0m ";
  ((std::cout << p), ...) << std::endl;
}

void
print(const char *p)
{
  __print("\033[34msnowball msg:\033[0m ");
  __print(p);
  __print("\n\r");
}

template <typename T>
void __attribute__((noreturn))
error(const T &p)
{
  __print_error("\033[34msnowball error():\033[0m ");
  std::cout << p << std::endl;
  __require_clbck();
  __abort();
}

void __attribute__((noreturn))
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
  if ( fn(std::forward<Args>(args)...) == false ) {
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
  if ( fn(std::forward<Args>(args)...) == false ) {
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
  bool _t = fn(std::forward<Args>(args)...);
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
  bool _t = fn(std::forward<Args>(args)...);
  print(_t);
  if ( _t == false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was false.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

void
require(const bool a, const bool b)
{
  if ( a != b ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was wrong.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};
void
require_false(const bool expected_output)
{
  if ( expected_output != false ) {
    __print_error("\033[34msnowball require() failure:\033[0m expected output was true.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};
void
require_true(const bool expected_output)
{
  if ( expected_output != true ) {
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires(((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
require(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !__check(expected_output, std::forward<Fn>(fn), std::make_tuple(inputs...)) ) {
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires(((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
require_false(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !check_false(expected_output, std::forward<Fn>(fn), std::make_tuple(inputs...)) ) {
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
require_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
    __print_error("\033[34msnowball require_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  } catch ( ... ) {
    return;
  }
};
template <typename E, typename Fn, typename... Args>
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
require_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
require_nothrow(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
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

void
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires(((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
check(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !__check(expected_output, std::forward<Fn>(fn), std::make_tuple(inputs...)) ) {
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires(((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && all_invocable_t<Fn, Dt_In...>))
void
check_false(Fn &&fn, const Dt_Ex &expected_output, const Dt_In &...inputs)
{
  if ( !check_false(expected_output, std::forward<Fn>(fn), std::make_tuple(inputs...)) ) {
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
check_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
    __print_error("\033[34msnowball check_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __check_clbck();
  } catch ( ... ) {
    return;
  }
};
template <typename E, typename Fn, typename... Args>
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
check_throw(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
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
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn>)
void
check_nothrow(Fn &&fn)
{
  try {
    (*fn)();
  } catch ( ... ) {
    ("\033[34msnowball check_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __check_clbck();
    return;
  }
};

template <typename Fn, typename... Args>
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
check_nothrow(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
  } catch ( ... ) {
    __print_error("\033[34msnowball check_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __check_clbck();
    return;
  }
};
template <typename E, typename Fn, typename... Args>
  requires((std::is_function_v<std::remove_pointer_t<Fn>> or std::is_function_v<Fn>) && std::is_invocable_v<Fn, Args...>)
void
check_nothrow(Fn &&fn, Args &&...args)
{
  try {
    (*fn)(std::forward<Args>(args)...);
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

// start fuzzer
// TODO: expand this later, this is simply a rudimentary brute force fuzzer
template <typename Fn>
void
fuzz(Fn &&fn, size_t cnt)
{
  // also black magic
  using traits = function_traits<decltype(&fn)>;
  // for_each_type<typename traits::args_tuple>([]<typename T>() { size_t sz = sizeof(T); });
  if ( traits::arity == 1 ) {
    typename traits::arg_type<0> var{};
    size_t sz = sizeof(var);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, (2 << (4 * 8)) - 1);

    for ( size_t i = 0; i < cnt; ++i ) {
      var = static_cast<typename traits::arg_type<0>>(dist(gen));
      fn(var);
    }
  }
}
};

namespace sb = snowball;
