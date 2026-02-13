//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/spinlock.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// TODO: implement

namespace micron
{

template <typename K, typename... Args>
  requires(micron::is_invocable_v<K, Args...>)
class contract
{
  spin_lock lck;
  K (*fcallback)(Args...);     // main callback

public:
  ~contract() {}
  template <typename... Targs> contract(Targs &&...args) {}
  contract(const contract &o) = delete;
  contract(contract &&) = delete;
  contract &operator=(const contract &) = delete;
  contract &operator=(contract &&) = delete;
  // requirement that needs to be true in order to fire ensures
  void
  require()
  {
  }
  // result of the contract
  void
  ensure()
  {
  }
};

};
