//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{
template <typename F> struct lambda_return;

template <typename F, typename R, typename... Args> struct lambda_return<R (F::*)(Args...) const> {
  using type = R;
};

template <typename F> using lambda_return_t = typename lambda_return<decltype(&F::operator())>::type;
};
