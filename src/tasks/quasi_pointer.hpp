//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __rm_eventual: __ret is an eventual<T>*
// __rm_lvalue  : __ret is a T*
// __rm_discard : no destination
// __rm_unbound : no binding
//

namespace micron
{
template<class T> class task;

namespace coro
{

enum __ret_mode_t : u8 {
  __rm_unbound = 0,
  __rm_eventual = 1,
  __rm_lvalue = 2,
  __rm_discard = 3,
};

struct discard_t {
};

inline constexpr discard_t discard{};

template<class> struct __task_value;

template<class T> struct __task_value<micron::task<T>> {
  using type = T;
};
template<class Tk> using __task_value_t = typename __task_value<micron::remove_cvref_t<Tk>>::type;

};      // namespace coro
};      // namespace micron
