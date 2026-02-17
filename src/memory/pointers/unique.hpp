//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{

// A unique pointer, a general replacement of unique_ptr with a few differences
// unique_ptr is meant to always create a new instance of the object on creation and this is intentional
// it's impossible to create a unique_pointer by passing a raw pointer into it without moving it
// prevents two instances of the same pointer existing simult.
template <class Type> class unique_pointer : private __internal_pointer_alloc<Type>
{
  Type *internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using element_type = Type;

  using __alloc = __internal_pointer_alloc<Type>;

  ~unique_pointer() { __alloc::__impl_dealloc(internal_pointer); };

  unique_pointer(void) : internal_pointer(nullptr) {};
  template <typename V>
    requires micron::is_null_pointer_v<V>
  unique_pointer(V) : internal_pointer(nullptr){};

  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args>
    requires(sizeof...(Args) > 0)
  unique_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(micron::forward<Args>(args)...)){};

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };

  unique_pointer(const unique_pointer &p) = delete;

  unique_pointer &
  operator=(unique_pointer &&t)
  {
    if ( this != &t ) {
      __alloc::__impl_dealloc(internal_pointer);
      internal_pointer = t.internal_pointer;
      t.internal_pointer = nullptr;
    }
    return *this;
  };

  unique_pointer &operator=(const unique_pointer &) = delete;

  unique_pointer &
  operator=(Type *&t) noexcept
  {
    return operator=(micron::move(t));
  };

  unique_pointer &
  operator=(Type *&&t) noexcept
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };

  template <is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return internal_pointer == o.get();
  }

  template <is_pointer_class O>
  bool
  operator>(const O &o) const noexcept
  {
    return internal_pointer > o.get();
  }

  template <is_pointer_class O>
  bool
  operator<(const O &o) const noexcept
  {
    return internal_pointer < o.get();
  }

  template <is_pointer_class O>
  bool
  operator<=(const O &o) const noexcept
  {
    return internal_pointer <= o.get();
  }

  template <is_pointer_class O>
  bool
  operator>=(const O &o) const noexcept
  {
    return internal_pointer >= o.get();
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  };

  Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("unique_pointer operator->(): internal_pointer was null");
  }

  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("unique_pointer operator*(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("unique_pointer operator*(): internal_pointer was null");
  };

  inline Type *
  release() noexcept
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };

  Type *
  get() const noexcept
  {
    return internal_pointer;
  }

  bool
  active() const noexcept
  {
    return (internal_pointer != nullptr);
  }

  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = nullptr;
  };
};

template <class Type> class unique_pointer<Type[]> : private __internal_pointer_arralloc<Type>
{
  Type *internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using element_type = Type;

  using __alloc = __internal_pointer_arralloc<Type>;

  ~unique_pointer() { __alloc::__impl_dealloc(internal_pointer); };

  unique_pointer(void) : internal_pointer(nullptr) {};
  template <typename V>
    requires micron::is_null_pointer_v<V>
  unique_pointer(V) : internal_pointer(nullptr){};

  template <typename... Args>
    requires(sizeof...(Args) > 0)
  unique_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(micron::forward<Args>(args)...))
  {
  }

  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };

  unique_pointer(const unique_pointer &p) = delete;
  unique_pointer &operator=(const unique_pointer &) = delete;

  unique_pointer &
  operator=(unique_pointer &&t)
  {
    if ( this != &t ) {
      __alloc::__impl_dealloc(internal_pointer);
      internal_pointer = t.internal_pointer;
      t.internal_pointer = nullptr;
    }
    return *this;
  };

  Type &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (internal_pointer)[n];
    else
      exc<except::memory_error>("unique_pointer[] operator[](): internal_pointer was null");
  };

  const Type &
  operator[](const size_t n) const
  {
    if ( internal_pointer != nullptr )
      return (internal_pointer)[n];
    else
      exc<except::memory_error>("unique_pointer[] operator[](): internal_pointer was null");
  };

  unique_pointer &
  operator=(Type *&t) noexcept
  {
    return operator=(micron::move(t));
  };

  unique_pointer &
  operator=(Type *&&t) noexcept
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };

  const Type *
  operator()() const
  {
    return internal_pointer;
  }

  const Type *
  operator&() const
  {
    return internal_pointer;
  }

  inline Type *
  release() noexcept
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };

  Type *
  get() const noexcept
  {
    return internal_pointer;
  }

  bool
  active() const noexcept
  {
    return (internal_pointer != nullptr);
  }

  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = nullptr;
  };

  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("unique_pointer[] operator*(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("unique_pointer[] operator*(): internal_pointer was null");
  };

  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("unique_pointer[] operator->(): internal_pointer was null");
  };

  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("unique_pointer[] operator->(): internal_pointer was null");
  };

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }
};

};
