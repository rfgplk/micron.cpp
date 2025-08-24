//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"
#include <type_traits>

#include "atomic/atomic.hpp"
#include "except.hpp"
#include "memory/actions.hpp"
#include "memory/new.hpp"

// ptr is pointer
// pointer is unique_ptr
// shared is shared_ptr

namespace micron
{
using count_t = size_t;

template <typename T>
void
ptr_deleter(T *ptr)
{
  if ( ptr != nullptr )
    delete ptr;
}
template <typename T>
void
sptr_deleter(T *ptr)
{
}
template <typename T>
concept is_nullptr = std::is_null_pointer_v<T>;
template <typename T>
concept is_valid_pointer = std::is_pointer_v<T> and !std::is_null_pointer_v<T>;

template <class Type> struct __internal_pointer_alloc {
  template <typename... Args>
  inline __attribute__((always_inline)) Type *
  __impl_alloc(Args &&...args)
  {
    return __new<Type>(forward<Args>(args)...);
    // return new Type(forward<Args>(args)...);
  }

  inline __attribute__((always_inline)) void
  __impl_dealloc(Type *&pointer)
  {
    if ( pointer != nullptr ) {
      __delete(pointer);
    }
  }
  inline __attribute__((always_inline)) void
  __impl_constdealloc(const Type *const &pointer)
  {
    if ( pointer != nullptr ) {
      __const_delete(pointer);
    }
  }
};

template <class Type> struct __internal_pointer_arralloc {
  template <typename... Args>
  inline __attribute__((always_inline)) Type *
  __impl_alloc(Args &&...args)
  {
    return __new_arr<Type>(forward<Args>(args)...);
    // return new Type(forward<Args>(args)...);
  }

  inline __attribute__((always_inline)) void
  __impl_dealloc(Type *&pointer)
  {
    if ( pointer != nullptr ) {
      __delete_arr(pointer);
    }
  }
};

template <typename T> class t_ptr
{
};

template <typename T> struct shared_handler {
  T *pnt;
  count_t refs;
  // count_t weaks
};
template <typename T> using thread_safe_handler = atomic<shared_handler<T>>;
// thread safe pointer
template <class Type> class thread_pointer : private __internal_pointer_alloc<thread_safe_handler<Type>>
{
  using __alloc = __internal_pointer_alloc<thread_safe_handler<Type>>;
  thread_safe_handler<Type> *control;

public:
  ~thread_pointer()
  {
    if ( control != nullptr ) [[likely]] {
      auto *t = control->get();
      if ( !--t->refs ) [[unlikely]] {
        __delete(t->pnt);     // TODO: think about passing through stored type to __dealloc
        __alloc::__impl_dealloc(t);
        control->release();
      }
    }
  };
  thread_pointer(void)
      : control(__alloc::__impl_alloc(__new<Type>(), 1)) {};     // new shared_handler<Type>({ new Type(), 1 })) {};

  thread_pointer(Type *p)
      : control(__alloc::__impl_alloc(__new<Type>(), 1)) {}     // new thread_safe_handler<Type>({ p, 1 })) {};
  thread_pointer(const thread_pointer<Type> &t) : control(t.control) { control->get()->refs++; };
  thread_pointer(thread_pointer<Type> &&t) : control(t.control) { t.control = nullptr; };

  template <class... Args>
  thread_pointer(Args &&...args) : control(__alloc::__impl_alloc(__new<Type>(forward<Args>(args)...), 1))
  {
  }     // new thread_safe_handler<Type>({ new Type(args...), 1 })){};
  const auto
  operator=(const thread_safe_handler<Type> &o)
  {
    if ( control != nullptr ) {
      control = o.control;
      control->get()->refs++;
      control->release();
    }
  };

  const auto
  operator()() const
  {
    if ( control != nullptr ) {
      auto t = control->get()->pnt;
      control->release();
      return t;
    } else
      return nullptr;
  };

  Type *
  operator->() const
  {
    if ( control != nullptr ) {
      auto *t = *control->get()->pnt;
      control->release();
      return t;
    } else
      throw nullptr;
  };
  Type
  operator*() const
  {
    if ( control != nullptr ) {
      auto t = *control->get()->pnt;
      control->release();
      return t;
    } else
      throw nullptr;
  };
};

template <class Type> class shared_pointer : private __internal_pointer_alloc<shared_handler<Type>>
{
  using __alloc = __internal_pointer_alloc<shared_handler<Type>>;
  shared_handler<Type> *control;

public:
  ~shared_pointer()
  {
    if ( control != nullptr ) [[likely]] {
      if ( !--control->refs ) [[unlikely]] {
        __delete(control->pnt);     // TODO: think about passing through stored type to __dealloc
        __alloc::__impl_dealloc(control);
        // delete control->pnt;
        // delete control;
      }
    }
  };
  shared_pointer(void)
      : control(__alloc::__impl_alloc(__new<Type>(), 1)) {};     // new shared_handler<Type>({ new Type(), 1 })) {};
  template <is_nullptr V> shared_pointer(V) : control(nullptr){};
  shared_pointer(Type *p) : control(__impl_alloc(p, 1)) {};     // new shared_pointer_handler<Type>({ p, 1 })) {};
  shared_pointer(const shared_pointer<Type> &t) : control(t.control)
  {
    if ( control != nullptr ) [[likely]]
      control->refs++;
  };
  shared_pointer(shared_pointer<Type> &t) : control(t.control)
  {
    if ( control != nullptr ) [[likely]]
      control->refs++;
  };
  shared_pointer(shared_pointer<Type> &&t) noexcept : control(t.control) { t.control = nullptr; };

  template <class... Args>
  shared_pointer(Args &&...args)
      : control(__alloc::__impl_alloc(__new<Type>(forward<Args>(args)...),
                                      1)){};     // control(new shared_handler<Type>({ new Type(args...), 1 })){};
  shared_pointer &
  operator=(shared_pointer<Type> &&o) noexcept
  {
    if ( control != nullptr ) {
      control = o.control;
    }
    return *this;
  };
  shared_pointer &
  operator=(const shared_pointer<Type> &o) noexcept
  {
    if ( control != nullptr ) {
      control = o.control;
      control->refs++;
    }
    return *this;
  };
  template <is_nullptr V>
  shared_pointer &
  operator=(V) noexcept
  {
    if ( control != nullptr ) {
      control->refs--;
    }
    return *this;
  };

  Type *
  operator()() noexcept
  {
    if ( control != nullptr ) {
      return control->pnt;
    } else
      return nullptr;
  };
  const Type *
  operator()() const noexcept
  {
    if ( control != nullptr ) {
      return control->pnt;
    } else
      return nullptr;
  };

  Type *
  operator->() noexcept
  {
    if ( control != nullptr )
      return control->pnt;
    else
      return nullptr;
  };
  const Type *
  operator->() const noexcept
  {
    if ( control != nullptr )
      return control->pnt;
    else
      return nullptr;
  };

  bool
  operator!(void) const noexcept
  {
    return control == nullptr;
  };
  Type &
  operator*()
  {
    if ( control != nullptr )
      return *control->pnt;
    else
      throw except::memory_error("shared_pointer operator*(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( control != nullptr )
      return *(control->pnt);
    else
      throw except::memory_error("shared_pointer operator*(): internal_pointer was null");
  };
  size_t
  refs() const noexcept
  {
    if ( control != nullptr )
      return control->refs;
    else
      return sizeof(size_t);
  }
};

// A unique pointer, a general replacement of unique_ptr with a few differences
// unique_ptr is meant to always create a new instance of the object on creation and this is intentional
// it's impossible to create a unique_pointer by passing a raw pointer into it without moving it
// prevents two instances of the same pointer existing simult.
template <class Type> class unique_pointer : private __internal_pointer_alloc<Type>
{
  using __alloc = __internal_pointer_alloc<Type>;
  Type *internal_pointer;

public:
  ~unique_pointer() { __alloc::__impl_dealloc(internal_pointer); };
  unique_pointer(void) : internal_pointer(__alloc::__impl_alloc()) {};     // new Type()) {};
  template <typename V>
    requires std::is_null_pointer_v<V>
  unique_pointer(V) : internal_pointer(nullptr){};
  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args>
  unique_pointer(Args &&...args)
      : internal_pointer(__alloc::__impl__alloc(forward<Args>(args)...)){};     // new Type(args...)){};

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  unique_pointer(const unique_pointer &p) = delete;

  unique_pointer &operator=(const unique_pointer &) = delete;
  unique_pointer &
  operator=(Type *&&t) noexcept
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  unique_pointer &
  operator=(unique_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  bool
  operator==(const unique_pointer &o) noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const unique_pointer &o) noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const unique_pointer &o) noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const unique_pointer &o) noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const unique_pointer &o) noexcept
  {
    return internal_pointer >= o();
  }
  Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("unique_pointer operator*(): internal_pointer was null");
  }
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("unique_pointer operator*(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("unique_pointer operator*(): internal_pointer was null");
  };
  inline Type *
  release() const noexcept
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
};

template <class Type> class unique_pointer<Type[]> : private __internal_pointer_arralloc<Type[]>
{
  using __alloc = __internal_pointer_arralloc<Type[]>;
  Type *internal_pointer;

public:
  ~unique_pointer() { __alloc::__impl_dealloc(internal_pointer); };
  unique_pointer(void) = delete;
  template <typename... Args>
  unique_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...))
  {
  }     // internal_pointer(new Type[sizeof...(args)]{ args... }){};

  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  unique_pointer(const unique_pointer &p) = delete;
  unique_pointer(unique_pointer &p) = delete;
  unique_pointer &operator=(const unique_pointer &) = delete;
  unique_pointer &
  operator=(unique_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  const Type &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    else
      throw except::memory_error("unique_pointer[] operator[](): internal_pointer was null");
  };
  unique_pointer &
  operator=(Type *&&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  bool
  operator==(const unique_pointer &o) const noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const unique_pointer &o) const noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const unique_pointer &o) const noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const unique_pointer &o) const noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const unique_pointer &o) const noexcept
  {
    return internal_pointer >= o();
  }
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
  release()
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };

  inline void
  clear()
  {
    __impl_dealloc(internal_pointer);
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("unique_pointer[] operator*(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("unique_pointer[] operator*(): internal_pointer was null");
  };
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("unique_pointer[] operator*(): internal_pointer was null");
  };

  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("unique_pointer[] operator*(): internal_pointer was null");
  };
};

// A constant pointer, cannot be reassigned nor written to
template <class Type> class const_pointer : private __internal_pointer_alloc<Type>
{
  using __alloc = __internal_pointer_alloc<Type>;
  const Type *const internal_pointer;

public:
  ~const_pointer() { __alloc::__impl_constdealloc(internal_pointer); };
  const_pointer() : internal_pointer(__alloc::__impl_alloc()) {}     // internal_pointer(new Type()) {};
  template <is_nullptr V> const_pointer(V) = delete;
  const_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args>
  const_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...))
  {
  }     // internal_pointer(new Type(args...)){};

  // important, cannot be moved
  const_pointer(const_pointer &&p) = delete;
  const_pointer(const const_pointer &p) = delete;

  const_pointer &operator=(const const_pointer &) = delete;
  const_pointer &operator=(const_pointer &&t) = delete;
  const Type *
  operator()() const noexcept
  {
    return internal_pointer;
  }
  bool
  operator==(const const_pointer &o) const noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const const_pointer &o) const noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const const_pointer &o) const noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const const_pointer &o) const noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const const_pointer &o) const noexcept
  {
    return internal_pointer >= o();
  }
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("const_pointer operator->(): internal_pointer was null");
  }
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("const_pointer operator*(): internal_pointer was null");
  };
  inline Type *release() = delete;
  void clear() = delete;
};

template <class Type> class weak_pointer : private __internal_pointer_alloc<Type>
{
  using __alloc = __internal_pointer_alloc<Type>;
  Type *internal_pointer;

public:
  ~weak_pointer() { __alloc::__impl_dealloc(internal_pointer); };
  weak_pointer() : internal_pointer(nullptr) {}     //: internal_pointer(new Type()) {};
  weak_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <is_nullptr V> weak_pointer(V) : internal_pointer(nullptr){};
  weak_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  weak_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args>
  weak_pointer(Args &&...args)
      : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...)){};     // internal_pointer(new Type(args...)){};

  weak_pointer(weak_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  weak_pointer(const weak_pointer &p) { internal_pointer = p.internal_pointer; }

  weak_pointer &
  operator=(weak_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  weak_pointer &
  operator=(Type *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator==(const weak_pointer &o) noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const weak_pointer &o) noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const weak_pointer &o) noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const weak_pointer &o) noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const weak_pointer &o) noexcept
  {
    return internal_pointer >= o();
  }
  bool
  operator==(const weak_pointer &o) const noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const weak_pointer &o) const noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const weak_pointer &o) const noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const weak_pointer &o) const noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const weak_pointer &o) const noexcept
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
      throw except::memory_error("weak_pointer operator->(): internal_pointer was null");
  };
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("weak_pointer operator->(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("weak_pointer operator*(): internal_pointer was null");
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("weak_pointer operator*(): internal_pointer was null");
  };
  template <typename Ft>
  weak_pointer &
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
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };

  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
  };
  template <typename P = unique_pointer<Type>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <class Type> class weak_pointer<Type[]> : private __internal_pointer_arralloc<Type[]>
{
  using __alloc = __internal_pointer_arralloc<Type>;
  Type *internal_pointer;

public:
  ~weak_pointer() { __alloc::__impl_dealloc(internal_pointer); };
  weak_pointer(void) : internal_pointer(nullptr) {}
  weak_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  weak_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  template <is_nullptr V> weak_pointer(V) : internal_pointer(nullptr){};
  weak_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args>
  weak_pointer(Args &&...args)
      : internal_pointer(__alloc::__impl_alloc(forward<Args>(
            args)...)){};     // internal_pointer(new Type[sizeof...(args)]{ micron::forward<Args>(args)... }){};

  weak_pointer(weak_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  weak_pointer(const weak_pointer &p) { internal_pointer = p.internal_pointer; }

  auto &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    throw except::memory_error("weak_pointer operator[](): internal_pointer was null");
  };

  const auto &
  operator[](const size_t n) const
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    throw except::memory_error("weak_pointer operator[](): internal_pointer was null");
  };
  weak_pointer &
  operator=(Type *&&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  weak_pointer &
  operator=(weak_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  weak_pointer &
  operator=(Type *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  template <typename Ft>
  weak_pointer &
  operator=(Ft *raw_ptr)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator==(const weak_pointer &o) noexcept
  {
    return internal_pointer == o();
  }
  bool
  operator>(const weak_pointer &o) noexcept
  {
    return internal_pointer > o();
  }
  bool
  operator<(const weak_pointer &o) noexcept
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const weak_pointer &o) noexcept
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const weak_pointer &o) noexcept
  {
    return internal_pointer >= o();
  }
  bool
  operator!(void) const
  {
    return internal_pointer == nullptr;
  };
  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("weak_pointer operator->(): internal_pointer was null");
  };

  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("weak_pointer operator->(): internal_pointer was null");
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("weak_pointer operator->(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  const Type *
  operator()() const
  {
    return internal_pointer;
  }
  Type *
  operator()()
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
  template <typename P = unique_pointer<Type[]>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <class Type> class free_pointer : private __internal_pointer_alloc<Type>
{
  using __alloc = __internal_pointer_alloc<Type>;
  Type *internal_pointer;

public:
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

  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("free_pointer operator->(): internal_pointer was null");
  };
  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("free_pointer operator->(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("free_pointer operator*(): internal_pointer was null");
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("free_pointer operator*(): internal_pointer was null");
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

  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
  };
  template <typename P = unique_pointer<Type>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <class Type> class free_pointer<Type[]> : private __internal_pointer_arralloc<Type[]>
{
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
      : internal_pointer(__alloc::__impl_alloc(
            forward<Args>(args)...)){};     // internal_pointer(new Type[sizeof...(args)]{ args... }){};

  free_pointer(free_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  free_pointer(const free_pointer &p) { internal_pointer = p.internal_pointer; }

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
    throw except::memory_error("free_pointer[] operator[](): internal_pointer was null");
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
      throw except::memory_error("free_pointer operator->(): internal_pointer was null");
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("free_pointer operator->(): internal_pointer was null");
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("free_pointer operator->(): internal_pointer was null");
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
  template <typename P = unique_pointer<Type[]>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

// TODO: move above code to pointers, and separate out

template <typename T> using ptr = weak_pointer<T>;
template <typename T> using fptr = free_pointer<T>;
template <typename T> using cptr = const_pointer<T>;
template <typename T> using pointer = unique_pointer<T>;
template <typename T> using shared = shared_pointer<T>;

template <class T>
pointer<T>
unique()
{
  return pointer<T>();
};
template <class T, class F>
pointer<T>
unique(std::initializer_list<F> list)
{
  return pointer<T>(list);
};
template <class T, class... Args>
pointer<T>
unique(Args &&...x)
{
  return pointer<T>(x...);
};
template <class T, class Y>
pointer<T>
unique(Y &&ptr)
{
  return pointer<T>(ptr);
};

template <class T>
void *
voidify(const T *ptr)
{
  using erase_type = std::remove_cv_t<decltype(ptr)>;
  if ( ptr != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(ptr)));
  else
    return (void *)nullptr;
};

template <class T>
void *
voidify(const pointer<T> &pnt)
{
  using erase_type = std::remove_cv_t<decltype(pnt())>;
  if ( pnt() != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(pnt())));
  else
    return (void *)nullptr;
};

template <class T, size_t N>
void *
voidify(const pointer<T[N]> &pnt)
{
  using erase_type = std::remove_cv_t<decltype(pnt())>;
  if ( pnt() != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(pnt())));
  else
    return (void *)nullptr;
};
};     // namespace micron
