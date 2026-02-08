//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/memory.hpp"
#include "pointer.hpp"
#include "type_traits.hpp"

namespace micron
{

template <typename> class function;

template <typename F, typename... Args> class function<F(Args...)>
{

  struct f_base {
    virtual ~f_base() = default;
    virtual F call(Args...) = 0;
  };
  template <typename G> struct f_cll : f_base {
    G fnc;
    f_cll(G &&g) : fnc(micron::forward<G>(g)) {}
    F
    call(Args... args) override
    {
      return fnc(micron::forward<Args>(args)...);
    };
  };
  micron::shared<f_base> callable = nullptr;

public:
  template <typename G> function(G &&g) : callable(new f_cll<G>(micron::forward<G>(g))) {}
  ~function() { delete callable; }
  F
  operator()(Args... args)
  {
    return callable->call(micron::forward<Args>(args)...);
    function &operator=(const function &o)
    {
      callable = o.callable;
      return *this;
    }
  }
};
template <typename T>
concept is_function = micron::is_integral_v<T>;

};
