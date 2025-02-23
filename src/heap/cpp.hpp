#pragma once

#include "../types.hpp"

namespace micron
{
struct default_heap {
  template <typename T>
  static __attribute__((always_inline)) inline T *
  allocate(size_t size)
  {
    return ::operator new(size);
  }
  template <typename T>
  static __attribute__((always_inline)) inline void
  deallocate(size_t, T *data)
  {
    ::operator delete(data);
  }
};

};
