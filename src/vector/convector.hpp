//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../bits/__container.hpp"

#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../container_safety.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{
// Regular convector class, always safe, mutable, notthread safe, cannot be
// copied for performance reasons (just move it, or use a slice for that)
template <is_movable_object T, class Alloc = micron::allocator_serial<>> class convector : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  micron::mutex __mtx;

  inline void
  __unlocked_clear()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length )
      return;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        (__mem::memory)[i].~T();
    }
    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __mem::capacity * (sizeof(T) / sizeof(byte)));
    __mem::length = 0;
  }

  inline void
  __unlocked_reserve(const size_t n)
  {
    if ( n < __mem::capacity )
      return;
    if ( __mem::is_zero() ) {
      // NOTE: if a container has been moved out, we need to reinit. memor
      __mem::realloc(n);
      return;
    }

    __mem::expand(n);
  }

  inline void
  __unlocked_push_back(const T &v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        new (&__mem::memory[__mem::length++]) T(v);
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        new (&__mem::memory[__mem::length++]) T(v);
      }
    } else {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        __mem::memory[__mem::length++] = v;
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        __mem::memory[__mem::length++] = v;
      }
    }
  }

  inline void
  __unlocked_push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        new (&__mem::memory[__mem::length++]) T(v);
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        new (&__mem::memory[__mem::length++]) T(v);
      }
    } else {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        __mem::memory[__mem::length++] = micron::move(v);
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

public:
  using category_type = vector_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef size_t size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  // NOTE: by convetion destructors should be the first method, adjust all other classes to be in the same order
  ~convector()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }

  convector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      size_t i = 0;
      for ( T &&value : lst ) {
        new (&__mem::memory[i++]) T(micron::move(value));
      }
      __mem::length = lst.size();
    } else {
      size_t i = 0;
      for ( T value : lst ) {
        __mem::memory[i++] = value;
      }
      __mem::length = lst.size();
    }
  };

  convector(void) : __mem() {};

  convector(const size_t n) : __mem(n)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T();
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = T{};
    }
    __mem::length = n;
  };

  template <typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  convector(size_t n, Args... args) : __mem(n)
  {
    for ( size_t i = 0; i < n; i++ )
      new (&__mem::memory[i]) T(args...);
    __mem::length = n;
  };

  convector(size_t n, const T &init_value) : __mem(n)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T(init_value);
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = init_value;
    }
    __mem::length = n;
  };

  convector(size_t n, T &&init_value) : __mem(n)
  {
    T tmp = micron::move(init_value);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T(tmp);
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = init_value;
    }
    __mem::length = n;
  };

  convector(const convector &o) : __mem(o.length)
  {
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
  }

  convector(chunk<byte> &&m) : __mem(m) { m = nullptr; };

  template <typename C = T> convector(convector<C> &&o) : __mem(micron::move(o)) {}

  convector(convector &&o) : __mem(micron::move(o)) {}

  convector &
  operator=(const convector &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __mem::memory = nullptr;
    __mem::capacity = 0;
    realloc(o.length);
    __mem::length = o.length;
    __impl_container::copy(__mem::memory, o.memory, o.length);
    return *this;
  }

  convector &
  operator=(convector &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::memory ) {
      // kill old memory first
      clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  template <typename... Args>
  convector &
  operator+=(Args &&...args)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __unlocked_push_back(micron::move(args)...);
    return *this;
  }

  const_pointer
  data() const
  {
    return &__mem::memory[0];
  }

  bool
  operator!() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return empty();
  }

  // overload this to always point to mem
  const byte *
  operator&() const volatile
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return reinterpret_cast<byte *>(__mem::memory);
  }

  inline slice<T>
  operator[]()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return slice<T>(begin(), end());
  }

  inline const slice<T>
  operator[]() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return slice<T>(begin(), end());
  }

  // copies convector out
  inline __attribute__((always_inline)) const slice<T>
  operator[](size_t from, size_t to) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    // meant to be safe so this is here
    if ( from >= to or from > __mem::capacity or to > __mem::capacity )
      exc<except::library_error>("micron::convector operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_t from, size_t to)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    // meant to be safe so this is here
    if ( from >= to or from > __mem::capacity or to > __mem::capacity )
      exc<except::library_error>("micron::convector operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }

  inline __attribute__((always_inline)) const T &
  operator[](size_type n) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    // meant to be safe so this is here
    if ( n > __mem::capacity )
      exc<except::library_error>("micron::convector operator[] out of allocated memory range.");
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  operator[](size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    // meant to be safe so this is here
    if ( n > __mem::capacity )
      exc<except::library_error>("micron::convector operator[] out of allocated memory range.");
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  at(size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n >= __mem::length )
      exc<except::library_error>("micron::convector at() out of bounds");
    return (__mem::memory)[n];
  }

  size_t
  at_n(iterator i) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( i - begin() >= __mem::length )
      exc<except::library_error>("micron::iconvector at_n() out of bounds");
    return static_cast<size_t>(i - begin());
  }

  const T *
  itr(size_t n) const
  {
    if ( n >= __mem::length )
      exc<except::library_error>("micron::convector at() out of bounds");
    return &(__mem::memory)[n];
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline convector &
  append(const convector<F> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( o.empty() )
      return *this;
    if ( !__mem::has_space(o.length) )
      __unlocked_reserve(__mem::capacity + o.max_size());

    __impl_container::copy(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), o.length);
    // micron::memcpy(&(__mem::memory)[__mem::length],
    // &o.memory[0],
    //                o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline convector &
  weld(convector<F> &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::has_space(o.length) )
      __unlocked_reserve(__mem::capacity + o.max_size());
    __impl_container::copy(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    // micron::memcpy(&(__mem::memory)[__mem::length],
    // &o.memory[0],
    //                o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename C = T>
  void
  swap(convector<C> &o) noexcept
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
  }

  size_t
  max_size() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::capacity;
  }

  size_t
  size() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::length;
  }

  void
  set_size(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __mem::length = n;
  }

  bool
  empty() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::length == 0 ? true : false;
  }

  // grow container
  inline void
  reserve(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n < __mem::capacity )
      return;
    if ( __mem::is_zero() ) {
      // NOTE: if a container has been moved out, we need to reinit. memor
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline void
  try_reserve(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n < __mem::capacity )
      exc<except::memory_error>("micron convector failed to reserve memory");
    if ( __mem::is_zero() ) {
      // NOTE: if a container has been moved out, we need to reinit. memor
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline slice<byte>
  into_bytes()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]), reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }

  inline convector<T>
  clone(void)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return convector<T>(*this);
  }

  void
  fill(const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = v;
  }

  // resize to how much and fill with a value v
  void
  resize(size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !(n > __mem::length) ) {
      return;
    }
    if ( n >= __mem::capacity ) {
      __unlocked_reserve(n);
    }
    T *f_ptr = __mem::memory;
    for ( size_t i = __mem::length; i < n; i++ )
      new (&f_ptr[i]) T{};

    __mem::length = n;
  }

  void
  resize(size_t n, const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !(n > __mem::length) ) {
      return;
    }
    if ( n >= __mem::capacity ) {
      __unlocked_reserve(n);
    }
    T *f_ptr = __mem::memory;
    for ( size_t i = __mem::length; i < n; i++ )
      new (&f_ptr[i]) T(v);

    __mem::length = n;
  }

  template <typename... Args>
  inline void
  emplace_back(Args &&...v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(v)...);
      return;
    } else {
      __unlocked_reserve(__mem::capacity + 1);
      new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(v)...);
    }
  }

  inline void
  move_back(T &&t)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length < __mem::capacity ) {
      __mem::memory[__mem::length++] = micron::move(t);
      return;
    } else {
      __unlocked_reserve(__mem::capacity + 1);
      __mem::memory[__mem::length++] = micron::move(t);
    }
  }

  inline const_iterator
  get(const size_t n) const
  {
    if ( n > __mem::length )
      exc<except::library_error>("micron::convector get() out of range");
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_t n) const
  {
    if ( n > __mem::length )
      exc<except::library_error>("micron::convector cget() out of range");
    return &(__mem::memory[n]);
  }

  inline iterator
  find(const T &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    T *f_ptr = __mem::memory;
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

  inline const_iterator
  find(const T &o) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    T *f_ptr = __mem::memory;
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

  inline const_iterator
  begin() const
  {
    return (__mem::memory);
  }

  inline const_iterator
  cbegin() const
  {
    return (__mem::memory);
  }

  inline const_iterator
  end() const
  {
    return (__mem::memory) + (__mem::length);
  }

  inline const_iterator
  last() const
  {
    return (__mem::memory) + (__mem::length - 1);
  }

  inline const_iterator
  cend() const
  {
    return (__mem::memory) + (__mem::length);
  }

  inline iterator
  insert(size_t n, const T &val, size_t cnt)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      for ( size_t i = 0; i < cnt; ++i )
        __unlocked_push_back(val);
      return begin();
    }
    if ( __mem::length + cnt > __mem::capacity )
      __unlocked_reserve(__mem::capacity + 1);
    if ( n >= __mem::length )
      exc<except::library_error>("micron::convector insert(): out of allocated memory range.");
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length - 1];
    micron::memmove(its + cnt, its, ite - its);
    //*its = (val);
    for ( size_t i = 0; i < cnt; ++i )
      new (its + i) T(val);
    __mem::length += cnt;
    return its;
  }

  inline iterator
  insert(size_t n, const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      __unlocked_reserve(__mem::capacity + 1);
    if ( n >= __mem::length )
      exc<except::library_error>("micron::convector insert(): out of allocated memory range.");
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length - 1];
    micron::memmove(its + 1, its, ite - its);
    //*its = (val);
    new (its) T(val);
    __mem::length++;
    return its;
  }

  inline iterator
  insert(size_t n, T &&val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_emplace_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      __unlocked_reserve(__mem::capacity + 1);
    if ( n >= __mem::length )
      exc<except::library_error>("micron::convector insert(): out of allocated memory range.");
    T *its = itr(n);
    T *ite = end();
    micron::memmove(its + 1, its, (ite - its));
    //*its = (val);
    new (its) T(micron::move(val));
    __mem::length++;
    return its;
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return begin();
    }
    if ( (__mem::length) >= __mem::capacity ) {
      size_t dif = static_cast<size_t>(it - __mem::memory);
      __unlocked_reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }     // invalidated if
    if ( it >= end() or it < begin() )
      exc<except::library_error>("micron::convector insert(): out of allocated memory range.");
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    //*it = (val);
    __mem::length++;
    return it;
  }

  inline iterator
  insert(iterator it, const T &val, const size_t cnt)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      for ( size_t i = 0; i < cnt; ++i )
        __unlocked_push_back(val);
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_t dif = it - __mem::memory;
      __unlocked_reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }
    if ( it >= end() or it < begin() )
      exc<except::library_error>("micron::convector insert(): out of allocated memory range.");
    T *ite = end();
    micron::memmove(it + cnt, it, ite - it);
    //*it = (val);
    for ( size_t i = 0; i < cnt; ++i )
      new (it + i) T(val);
    __mem::length++;
    return it;
  }

  inline iterator
  insert(iterator it, const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_t dif = it - __mem::memory;
      __unlocked_reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }
    if ( it >= end() or it < begin() )
      exc<except::library_error>("micron::convector insert(): out of allocated memory range.");
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    //*it = (val);
    new (it) T(val);
    __mem::length++;
    return it;
  }

  inline void
  sort(void)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( size() < 100000 )
      micron::sort::heap(*this);     // NOTE: need an inplace sort since *this cannot be copied
    else
      micron::sort::quick(*this);
  }

  inline iterator
  insert_sort(T &&val)     // NOTE: we won't check if this is presort, bad things will happen if it isn't
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return begin();
    }
    if ( (__mem::length + 1) >= __mem::capacity ) {
      __unlocked_reserve(__mem::capacity + 1);
    }     // invalidated if
    T *ite = end();
    size_t i = 0;
    for ( ; i < size() - 1; i++ ) {
      if ( __mem::memory[i] >= val ) {
        i++;
        break;
      }
    }
    // if i == size() - 1 it's the largest element
    if ( i == size() - 1 )
      i++;
    T *it = &__mem::memory[i];
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    __mem::length++;
    return it;
  }

  inline convector &
  assign(const size_t cnt, const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( (cnt * (sizeof(T) / sizeof(byte))) >= __mem::capacity ) {
      __unlocked_reserve(__mem::capacity + (cnt) * sizeof(T));
    }
    __unlocked_clear();     // clear the vec
    for ( size_t i = 0; i < cnt; i++ ) {
      //(__mem::memory)[i] = val;
      new (&__mem::memory[i]) T(val);
    }
    __mem::length = cnt;
    return *this;
  }

  inline void
  push_back(const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        new (&__mem::memory[__mem::length++]) T(v);
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        new (&__mem::memory[__mem::length++]) T(v);
      }
    } else {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        __mem::memory[__mem::length++] = v;
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        __mem::memory[__mem::length++] = v;
      }
    }
  }

  inline void
  push_back(T &&v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        new (&__mem::memory[__mem::length++]) T(v);
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        new (&__mem::memory[__mem::length++]) T(v);
      }
    } else {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        __mem::memory[__mem::length++] = micron::move(v);
        return;
      } else {
        __unlocked_reserve(__mem::capacity * sizeof(T) + 1);
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  inline void
  pop_back()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      (__mem::memory)[(__mem::length - 1)].~T();
    } else {
      (__mem::memory)[(__mem::length - 1)] = 0x0;
    }
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
  }

  inline void
  erase(iterator n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n >= end() or n < begin() )
      exc<except::library_error>("micron::convector erase(): out of allocated memory range.");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      n->~T();
    } else {
    }
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      size_t _n = (n - cbegin());
      for ( size_t i = _n; i < (__mem::length - 1); i++ )
        (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);
    } else {
      __impl_container::copy(n, n + 1, __mem::length);
    }
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
  }

  inline void
  erase(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n >= size() )
      exc<except::library_error>("micron::convector erase(): out of allocated memory range.");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      ~(__mem::memory)[n]();
    } else {
    }

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      size_t _n = (n - cbegin());
      for ( size_t i = n; i < (__mem::length - 1); i++ )
        (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);
    } else {
      __impl_container::copy(__mem::memory[n], __mem::memory[n + 1], __mem::length);
    }
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
    __mem::length--;
  }

  inline void
  clear()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length )
      return;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        (__mem::memory)[i].~T();
    }
    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __mem::capacity * (sizeof(T) / sizeof(byte)));
    __mem::length = 0;
  }

  inline const T &
  front() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return (__mem::memory)[0];
  }

  inline const T &
  back() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return (__mem::memory)[__mem::length - 1];
  }

  inline T &
  front()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return (__mem::memory)[0];
  }

  inline T &
  back()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return (__mem::memory)[__mem::length - 1];
  }

  static constexpr bool
  is_pod() noexcept
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class_type() noexcept
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }

  // access at element
};
};     // namespace micron
