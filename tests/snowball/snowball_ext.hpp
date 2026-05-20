//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Companion to snowball.hpp. Adds:
//   - property_test(name, fn, count): randomized invocation driver
//   - expect_no_throw / expect_throw: lambda-friendly throw checks
//   - repeat_until_throw: exception-injection loop
// snowball::fuzz at snowball.hpp:949 is single-arg only; property_test
// supports multi-arg functions via fold over function_traits.

#include "snowball.hpp"

namespace snowball
{

namespace __impl
{

inline u64
__property_seed(void) noexcept
{
  static u64 s = []() {
    u64 v = __cycle_counter();
    return v ? v : 0xdeadbeefULL;
  }();
  return s;
}

template<typename T>
T
__gen_one(u64 &state) noexcept
{
  u64 r = __xorshift64(state);
  if constexpr ( micron::is_integral_v<T> ) {
    return static_cast<T>(r);
  } else if constexpr ( micron::is_floating_point_v<T> ) {
    // map to [-1, 1)
    return static_cast<T>(static_cast<i64>(r)) / static_cast<T>(static_cast<i64>(1) << 62);
  } else {
    // fallback default-construct
    return T{};
  }
}

};      // namespace __impl

// Run a callable `fn` `count` times with random arguments (per-arg
// xorshift64). Each iteration that escapes via exception is reported and the
// driver stops.
template<typename Fn>
void
property_test(const char *name, Fn &&fn, size_t count)
{
  using traits = function_traits<decltype(&Fn::operator())>;
  using args_tuple = typename traits::args_tuple;

  test_case(name);
  u64 state = __impl::__property_seed();

  for ( size_t i = 0; i < count; ++i ) {
    try {
      auto args = []<size_t... Is>(u64 &st, micron::index_sequence<Is...>) {
        return micron::make_tuple(__impl::__gen_one<micron::remove_cvref_t<typename traits::template arg_type<Is>>>(st)...);
      }(state, micron::make_index_sequence<traits::arity>{});
      micron::apply(fn, args);
    } catch ( ... ) {
      __print_error("\033[34msnowball property_test() failure:\033[0m exception at iteration ");
      __print_error(i);
      __print_error(" seed=");
      __print_error(state);
      __print_error("\n\r");
      should_print_stack();
      __require_clbck();
      __abort();
    }
  }
  end_test_case();
};

// Lambda-friendly variants (snowball::require_nothrow requires a function
// pointer or function reference, not a lambda).
template<typename Fn>
void
expect_no_throw(Fn &&fn)
{
  try {
    fn();
  } catch ( ... ) {
    __print_error("\033[34msnowball expect_no_throw() failure:\033[0m something was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

template<typename Fn>
void
expect_throw(Fn &&fn)
{
  try {
    fn();
    __print_error("\033[34msnowball expect_throw() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  } catch ( ... ) {
    return;
  }
};

template<typename E, typename Fn>
void
expect_throw_type(Fn &&fn)
{
  try {
    fn();
    __print_error("\033[34msnowball expect_throw_type() failure:\033[0m nothing was thrown.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  } catch ( const E & ) {
    return;
  } catch ( ... ) {
    __print_error("\033[34msnowball expect_throw_type() failure:\033[0m wrong exception type.\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

// Wrap a tracking allocator scope: run fn, then assert allocator balance.
template<typename Allocator, typename Fn>
void
expect_leak_free(Fn &&fn)
{
  Allocator::reset();
  fn();
  if ( Allocator::outstanding() != 0 ) {
    __print_error("\033[34msnowball expect_leak_free() failure:\033[0m outstanding=");
    __print_error(static_cast<i64>(Allocator::outstanding()));
    __print_error("\n\r");
    should_print_stack();
    __require_clbck();
    __abort();
  }
};

};      // namespace snowball
