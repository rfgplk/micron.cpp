#pragma once

#include "../mutex/spinlock.hpp"
#include "../types.hpp"
#include <type_traits>

namespace micron
{

template <typename K, typename... Args>
  requires(std::is_invocable_v<K, Args...>)
class contract
{
  spin_lock lck;
  K (*fcallback)(Args...);     // main callback
  
public:
  ~contract() {}
  template <typename... Targs>
  contract(Targs&&... args) {
    
  }
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
