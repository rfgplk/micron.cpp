//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{

template <class Type> class free_pointer : private __internal_pointer_alloc<Type>
{
  Type *internal_pointer;

public:
  using pointer_type = weak_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;

  using __alloc = __internal_pointer_alloc<Type>;

  ~free_pointer() {};
  free_pointer(void) : internal_pointer(nullptr) {}     //: internal_pointer(new Type()) {};
  free_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <is_nullptr V> free_pointer(V) : internal_pointer(nullptr){};
  free_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  free_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args>
  free_pointer(Args &&...args)
      : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...)){};     // internal_pointer(new Type(args...)){};

  free_pointer(free_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  free_pointer(const free_pointer &p) { internal_pointer = p.internal_pointer; }

  free_pointer &
  operator=(const free_pointer &t)
  {
    internal_pointer = t.internal_pointer;
    return *this;
  };
  free_pointer &
  operator=(free_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  free_pointer &
  operator=(Type *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  };

  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator->(): internal_pointer was null");
  };
  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator->(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator*(): internal_pointer was null");
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator*(): internal_pointer was null");
  };
  template <typename Ft>
  free_pointer &
  operator=(Ft *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  Type *
  operator()()
  {
    return release();
  }
  inline Type *
  release()
  {
    Type *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };
  Type *
  get() const noexcept
  {
    return internal_pointer;
  }
  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
  };
  template <is_pointer_class P>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }
};

template <class Type> class free_pointer<Type[]> : private __internal_pointer_arralloc<Type[]>
{
  using pointer_type = weak_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using __alloc = __internal_pointer_arralloc<Type[]>;
  Type *internal_pointer;

public:
  ~free_pointer() {};
  free_pointer() : internal_pointer(nullptr) {}
  free_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  free_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  template <is_nullptr V> free_pointer(V) : internal_pointer(nullptr){};
  free_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args>
  free_pointer(Args &&...args)
      : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...)){};     // internal_pointer(new Type[sizeof...(args)]{ args... }){};

  free_pointer(free_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  free_pointer(const free_pointer &p) { internal_pointer = p.internal_pointer; }

  free_pointer &
  operator=(const free_pointer &t)
  {
    internal_pointer = t.internal_pointer;
    return *this;
  };
  auto &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    throw nullptr;
  };
  const auto &
  operator[](const size_t n) const
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    exc<except::memory_error>("free_pointer[] operator[](): internal_pointer was null");
  };
  free_pointer &
  operator=(Type *&&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  free_pointer &
  operator=(free_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  free_pointer &
  operator=(Type *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  template <typename Ft>
  free_pointer &
  operator=(Ft *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  Type *
  get() const noexcept
  {
    return internal_pointer;
  }
  bool
  operator==(const free_pointer &o) noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const free_pointer &o) noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const free_pointer &o) noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const free_pointer &o) noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const free_pointer &o) noexcept
  {
    return internal_pointer >= o();
  }
  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  };
  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator->(): internal_pointer was null");
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator->(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("free_pointer operator->(): internal_pointer was null");
  };
  const Type *
  operator()() const
  {
    return internal_pointer;
  }
  inline Type *
  release() const
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };

  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
  };
  template <is_pointer_class P>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

};
