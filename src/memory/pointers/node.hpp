#pragma once

#include "bits.hpp"

namespace micron {

template <class Type, > class node_pointer : private __internal_pointer_alloc<Type>
{
  using pointer_type = weak_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  
  Type *internal_pointer;
public:
  ~node_pointer() {}     // don't delete on destruction
  node_pointer(void) : internal_pointer(nullptr) {}
  template <typename... Args> node_pointer(Args &&...args) : internal_pointer(__impl_alloc(forward<Args>(args)...)) {}
  node_pointer(const node_pointer &) = delete;
  node_pointer(node_pointer &&o) noexcept : internal_pointer(o.internal_pointer) { o.internal_pointer = nullptr; }
  node_pointer &operator=(const node_pointer &o) = delete;
  node_pointer &
  operator=(node_pointer &&o) noexcept
  {
    internal_pointer = o.internal_pointer;
    o.internal_pointer = nullptr;
    return *this;
  }
  node_pointer &
  swap(node_pointer<Type> &&o) noexcept
  {
    auto *tmp = internal_pointer;
    internal_pointer = o.internal_pointer;
    o.internal_pointer = tmp;
    return *this;
  }
  void
  release()
  {
    __impl_dealloc(internal_pointer);
  }
};

};
