//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"
#include <type_traits>

#include "atomic/atomic.hpp"
#include "memory/actions.hpp"

// ptr is pointer
// pointer is unique_ptr
// shared is shared_ptr

namespace micron
{
typedef size_t count_t;

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
template <class Type> class thread_pointer
{
  thread_safe_handler<Type> *control;

public:
  thread_pointer() : control(new thread_safe_handler<Type>({ new Type(), 1 })) {};

  thread_pointer(Type *p) : control(new thread_safe_handler<Type>({ p, 1 })) {};
  thread_pointer(const thread_pointer<Type> &t) : control(t.control) { control->get()->refs++; };
  thread_pointer(thread_pointer<Type> &&t) : control(t.control) { t.control = nullptr; };

  template <class... Args>
  thread_pointer(Args &&...args) : control(new thread_safe_handler<Type>({ new Type(args...), 1 })){};
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
  ~thread_pointer()
  {
    if ( control != nullptr ) [[likely]] {
      auto *t = control->get();
      if ( !--t->refs ) [[unlikely]] {
        delete t->pnt;
        delete t;
        control->release();
      }
    }
  };
};

template <class Type> class shared
{
  shared_handler<Type> *control;

public:
  shared() : control(new shared_handler<Type>({ new Type(), 1 })) {};
  template <typename V>
    requires std::is_null_pointer_v<V>
  shared(V) : control(nullptr){};
  shared(Type *p) : control(new shared_handler<Type>({ p, 1 })) {};
  shared(const shared<Type> &t) : control(t.control)
  {
    if ( control != nullptr ) [[likely]]
      control->refs++;
  };
  shared(shared<Type> &t) : control(t.control)
  {
    if ( control != nullptr ) [[likely]]
      control->refs++;
  };
  shared(shared<Type> &&t) : control(t.control) { t.control = nullptr; };

  template <class... Args> shared(Args &&...args) : control(new shared_handler<Type>({ new Type(args...), 1 })){};
  shared &
  operator=(shared<Type> &&o)
  {
    if ( control != nullptr ) {
      control = o.control;
    }
    return *this;
  };
  shared &
  operator=(const shared<Type> &o)
  {
    if ( control != nullptr ) {
      control = o.control;
      control->refs++;
    }
    return *this;
  };
  template <typename V>
    requires std::is_null_pointer_v<V>
  shared &
  operator=(V)
  {
    if ( control != nullptr ) {
      control->refs--;
    }
    return *this;
  };

  Type *
  operator()()
  {
    if ( control != nullptr ) {
      return control->pnt;
    } else
      return nullptr;
  };
  const Type *
  operator()() const
  {
    if ( control != nullptr ) {
      return control->pnt;
    } else
      return nullptr;
  };

  Type *
  operator->()
  {
    if ( control != nullptr )
      return control->pnt;
    else
      throw nullptr;
  };
  const Type *
  operator->() const
  {
    if ( control != nullptr )
      return control->pnt;
    else
      throw nullptr;
  };

  bool
  operator!(void) const
  {
    return control == nullptr;
  };
  Type &
  operator*()
  {
    if ( control != nullptr )
      return *control->pnt;
    else
      throw nullptr;
  };
  const Type &
  operator*() const
  {
    if ( control != nullptr )
      return *(control->pnt);
    else
      throw nullptr;
  };
  size_t
  refs() const
  {
    if ( control != nullptr )
      return control->refs;
    else
      return sizeof(size_t);
  }
  ~shared()
  {
    if ( control != nullptr ) [[likely]] {
      if ( !--control->refs ) [[unlikely]] {
        delete control->pnt;
        delete control;
      }
    }
  };
};

// A unique pointer, a general replacement of unique_ptr with a few differences
// unique_ptr is meant to always create a new instance of the object on creation and this is intentional
// it's impossible to create a unique_pointer by passing a raw pointer into it without moving it
// prevents two instances of the same pointer existing simult.
template <class Type> class unique_pointer
{
  Type *internal_pointer;

public:
  unique_pointer() : internal_pointer(new Type()) {};
  template <typename V>
    requires std::is_null_pointer_v<V>
  unique_pointer(V) : internal_pointer(nullptr){};
  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args> unique_pointer(Args &&...args) : internal_pointer(new Type(args...)){};

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  unique_pointer(const unique_pointer &p) = delete;
  ~unique_pointer()
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
  };

  unique_pointer &operator=(const unique_pointer &) = delete;
  unique_pointer &
  operator=(Type *&&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  unique_pointer &
  operator=(unique_pointer &&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  bool
  operator==(const unique_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const unique_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const unique_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const unique_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const unique_pointer &o)
  {
    return internal_pointer >= o();
  }
  Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw nullptr;
  }
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
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
    if ( internal_pointer )
      delete internal_pointer;
    internal_pointer = nullptr;
  };
};
/*
template <class Type, size_t N> class unique_pointer<Type[N]>
{
  Type *internal_pointer;

public:
  template <typename L = size_t> unique_pointer(L n) : internal_pointer(new Type[N])
  {
    for ( size_t i = 0; i < N; i++ )
      internal_pointer[i] = n;
  };

  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  //unique_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  //unique_pointer(void *raw_ptr) : internal_pointer(static_cast<Type *>(raw_ptr)) {};
  template <class... Args> unique_pointer(Args &&...args) : internal_pointer(new Type[sizeof...(args)]{ args... }){};

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  unique_pointer(const unique_pointer &p) = delete;
  unique_pointer(unique_pointer &p) = delete;
  virtual ~unique_pointer()
  {
    if ( internal_pointer != nullptr )
      delete[] internal_pointer;
  };

  unique_pointer &
  operator=(unique_pointer &t)
  {
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  const Type&
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    else
      throw nullptr;
  };
  unique_pointer &
  operator=(Type *&&t)
  {
    if ( internal_pointer != nullptr )
      delete[] internal_pointer;
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  unique_pointer &
  operator=(unique_pointer<Type> &&t)
  {
    if ( internal_pointer != nullptr )
      delete[] internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  bool
  operator==(const unique_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const unique_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const unique_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const unique_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const unique_pointer &o)
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
    if(internal_pointer)
      delete[] internal_pointer;
  };
  Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  Type &
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
};*/

template <class Type> class unique_pointer<Type[]>
{
  Type *internal_pointer;

public:
  unique_pointer() = delete;
  template <typename... Args> unique_pointer(Args... args) : internal_pointer(new Type[sizeof...(args)]{ args... }){};

  unique_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args> unique_pointer(Args &&...args) : internal_pointer(new Type[sizeof...(args)]{ args... }){};

  unique_pointer(unique_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  unique_pointer(const unique_pointer &p) = delete;
  unique_pointer(unique_pointer &p) = delete;
  ~unique_pointer()
  {
    if ( internal_pointer != nullptr )
      delete[] internal_pointer;
  };
  unique_pointer &operator=(const unique_pointer &) = delete;
  unique_pointer &
  operator=(unique_pointer &&t)
  {
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
      throw nullptr;
  };
  unique_pointer &
  operator=(Type *&&t)
  {
    if ( internal_pointer != nullptr )
      delete[] internal_pointer;
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  unique_pointer &
  operator=(unique_pointer<Type> &&t)
  {
    if ( internal_pointer != nullptr )
      delete[] internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  bool
  operator==(const unique_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const unique_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const unique_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const unique_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const unique_pointer &o)
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
    if ( internal_pointer )
      delete[] internal_pointer;
    internal_pointer = nullptr;
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw nullptr;
  };
};

// A constant pointer, cannot be reassigned nor written to
template <class Type> class const_pointer
{
  const Type *const internal_pointer;

public:
  ~const_pointer()
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
  };
  const_pointer() : internal_pointer(new Type()) {};
  template <typename V>
    requires std::is_null_pointer_v<V>
  const_pointer(V) = delete;
  const_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args> const_pointer(Args &&...args) : internal_pointer(new Type(args...)){};

  // important, cannot be moved
  const_pointer(const_pointer &&p) = delete;
  const_pointer(const const_pointer &p) = delete;

  const_pointer &operator=(const const_pointer &) = delete;
  const_pointer &operator=(const_pointer &&t) = delete;
  bool
  operator==(const const_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const const_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const const_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const const_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const const_pointer &o)
  {
    return internal_pointer >= o();
  }
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw nullptr;
  }
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  inline Type *release() = delete;

  void clear() = delete;
};

template <class Type> class weak_pointer
{
  Type *internal_pointer;

public:
  ~weak_pointer()
  {
    // to make sure
    if ( internal_pointer ) {
      delete internal_pointer;
    }
  };
  weak_pointer() : internal_pointer(nullptr) {}     //: internal_pointer(new Type()) {};
  weak_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <typename V>
    requires std::is_null_pointer_v<V>
  weak_pointer(V) : internal_pointer(nullptr){};
  weak_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  // very important, if we're passing in a func pointer it's almost certainly an argument to the container and not meant
  // to go in here
  template <typename Ft>
    requires(!(std::is_invocable_v<Ft>))
  weak_pointer(Ft *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)){};
  weak_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args> weak_pointer(Args &&...args) : internal_pointer(new Type(args...)){};

  weak_pointer(weak_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  weak_pointer(const weak_pointer &p) { internal_pointer = p.internal_pointer; }

  weak_pointer &
  operator=(weak_pointer &&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  weak_pointer &
  operator=(Type *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator==(const weak_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const weak_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const weak_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const weak_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const weak_pointer &o)
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
      throw nullptr;
  };
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw nullptr;
  };


  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  template <typename Ft>
  weak_pointer &
  operator=(Ft *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  const Type *
  operator()() const
  {
    return release();
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
    if ( internal_pointer )
      delete internal_pointer;
    internal_pointer = nullptr;
  };
  template <typename P = unique_pointer<Type>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <class Type> class weak_pointer<Type[]>
{
  Type *internal_pointer;

public:
  ~weak_pointer()
  {
    if ( internal_pointer )
      delete[] internal_pointer;
  };
  weak_pointer() : internal_pointer(nullptr) {}
  weak_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  weak_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  template <typename V>
    requires std::is_null_pointer_v<V>
  weak_pointer(V) : internal_pointer(nullptr){};
  weak_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args>
  weak_pointer(Args &&...args) : internal_pointer(new Type[sizeof...(args)]{ micron::forward<Args>(args)... }){};

  weak_pointer(weak_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  weak_pointer(const weak_pointer &p) { internal_pointer = p.internal_pointer; }

  const auto &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    throw nullptr;
  };

  weak_pointer &
  operator=(Type *&&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  weak_pointer &
  operator=(weak_pointer &&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  weak_pointer &
  operator=(Type *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  template <typename Ft>
  weak_pointer &
  operator=(Ft *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator==(const weak_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const weak_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const weak_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const weak_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const weak_pointer &o)
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
      throw nullptr;
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
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
    if ( internal_pointer )
      delete[] internal_pointer;
    internal_pointer = nullptr;
  };
  template <typename P = unique_pointer<Type[]>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <class Type> class free_pointer
{
  Type *internal_pointer;

public:
  ~free_pointer() {};
  free_pointer() : internal_pointer(nullptr) {}     //: internal_pointer(new Type()) {};
  free_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <typename V>
    requires std::is_null_pointer_v<V>
  free_pointer(V) : internal_pointer(nullptr){};
  free_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  template <typename Ft> free_pointer(Ft *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)){};
  free_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args> free_pointer(Args &&...args) : internal_pointer(new Type(args...)){};

  free_pointer(free_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  free_pointer(const free_pointer &p) { internal_pointer = p.internal_pointer; }

  free_pointer &
  operator=(free_pointer &&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  free_pointer &
  operator=(Type *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator==(const free_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const free_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const free_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const free_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const free_pointer &o)
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
      throw nullptr;
  };

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
  };
  template <typename Ft>
  free_pointer &
  operator=(Ft *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  const Type *
  operator()() const
  {
    return release();
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
    if ( internal_pointer )
      delete internal_pointer;
    internal_pointer = nullptr;
  };
  template <typename P = unique_pointer<Type>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <class Type> class free_pointer<Type[]>
{
  Type *internal_pointer;

public:
  ~free_pointer() {};
  free_pointer() : internal_pointer(nullptr) {}
  free_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  free_pointer(Type *raw_ptr) : internal_pointer(raw_ptr) {};
  template <typename V>
    requires std::is_null_pointer_v<V>
  free_pointer(V) : internal_pointer(nullptr){};
  free_pointer(void *raw_ptr) : internal_pointer(reinterpret_cast<Type *>(raw_ptr)) {};
  template <class... Args> free_pointer(Args &&...args) : internal_pointer(new Type[sizeof...(args)]{ args... }){};

  free_pointer(free_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  free_pointer(const free_pointer &p) { internal_pointer = p.internal_pointer; }

  const auto &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    throw nullptr;
  };

  free_pointer &
  operator=(Type *&&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  free_pointer &
  operator=(free_pointer &&t)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  free_pointer &
  operator=(Type *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  template <typename Ft>
  free_pointer &
  operator=(Ft *raw_ptr)
  {
    if ( internal_pointer != nullptr )
      delete internal_pointer;
    internal_pointer = raw_ptr;
    return *this;
  };
  bool
  operator==(const free_pointer &o)
  {
    return internal_pointer == o();
  }
  bool
  operator>(const free_pointer &o)
  {
    return internal_pointer > o();
  }
  bool
  operator<(const free_pointer &o)
  {
    return internal_pointer < o();
  }
  bool
  operator<=(const free_pointer &o)
  {
    return internal_pointer <= o();
  }
  bool
  operator>=(const free_pointer &o)
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
      throw nullptr;
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw nullptr;
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
    if ( internal_pointer )
      delete[] internal_pointer;
    internal_pointer = nullptr;
  };
  template <typename P = unique_pointer<Type[]>>
  inline P
  convert(void)
  {
    return P(micron::move(internal_pointer));
  }
};

template <typename T> using ptr = weak_pointer<T>;
template <typename T> using pointer = unique_pointer<T>;

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
