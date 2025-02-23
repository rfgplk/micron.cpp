#pragma once

#include "../mutex/spinlock.hpp"
#include "../types.hpp"
#include <type_traits>

namespace micron
{
class when
{

public:
  template <typename... Targs, typename D> when(Targs &&...args, D &result)
  {
    for ( ;; ) {
    }
  }
};

};
