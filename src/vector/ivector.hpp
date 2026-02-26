//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../container_safety.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "vector.hpp"

namespace micron
{
// (Immutable) vector class. ivector, contiguous in memory, O(1) access,
// iterators never invalidated, always safe, immutable, always thread safe as
// fast as raw arrays
template <is_movable_object T, class Alloc = micron::allocator_serial<>>
class ivector : private Alloc, public __immutable_memory_resource<T, Alloc>
{
  using __mem = __immutable_memory_resource<T, Alloc>;

  // grow container, private - only int. can call
  inline void
  reserve(size_t n)
  {
    __mem::accept_new_memory(this->grow(reinterpret_cast<byte *>(__mem::memory), __mem::capacity * sizeof(T), sizeof(T) * n));
  }

  // shallow copy routine
  inline void
  shallow_copy(T *dest, T *src, size_t cnt)
  {
    micron::memcpy256(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
                      cnt * (sizeof(T) / sizeof(byte)));     // always is page aligned, 256 is
                                                             // fine, just realign back to bytes
  };

  // deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
  // cause segfaulting if underlying doesn't account for double deletes)
  inline void
  deep_copy(T *dest, T *src, size_t cnt)
  {
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  };

  inline void
  __impl_copy(T *dest, T *src, size_t cnt)
  {
    if constexpr ( micron::is_class<T>::value ) {
      deep_copy(dest, src, cnt);
    } else {
      shallow_copy(dest, src, cnt);
    }
  }

  T *
  __itr(size_t n)
  {
    if ( n >= __mem::length )
      exc<except::library_error>("micron::ivector itr() out of bounds");
    return &(__mem::memory)[n];
  }

public:
  using category_type = vector_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;

  typedef T *iterator;
  typedef const T *const_iterator;

  // disallow empty creation, for simplicity
  ivector() = delete;

  // initialize empty with n reserve
  template <typename S = size_t>
    requires micron::is_arithmetic_v<S>
  ivector(S n) : __mem(n)
  {
    __mem::length = 0;
  };

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  ivector(Fn &&fn) : __mem()
  {
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  ivector(Fn &&fn)
  {
    micron::transform(begin(), end(), fn);
  }

  // two main functions when it comes to copying over data
  ivector(const ivector &o) : __mem(o.capacity())
  {
    micron::memcpy256(&__mem::memory[0], o.__itr(0),
                      o.capacity);     // always is page aligned, 256 is fine.
    __mem::length = o.size();
  }

  ivector &
  operator=(const ivector &o)
  {
    if ( o.capacity >= __mem::capacity )
      reserve(o.capacity);
    micron::memcpy256(&__mem::memory[0], o.__itr(0),
                      o.capacity);     // always is page aligned, 256 is fine.
    __mem::length = o.size();
  };

  // data must be page aligned and cleanly / 256, otherwise will not function
  // correctly. if using an avx512 cpu, replace with 512 (but not needed,
  // will be faster like this regardless)

  // identical to regular vector
  ivector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> ) {
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

  ivector(const vector<T> &o) : __mem(o.size())
  {
    micron::memcpy(&__mem::memory[0], o.data(), o.size());
    __mem::length = o.size();
  }

  ivector(size_t n) : __mem(n)
  {
    if constexpr ( micron::is_class_v<T> ) {
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
  ivector(size_t n, Args... args) : __mem(n)
  {
    for ( size_t i = 0; i < n; i++ )
      new (&__mem::memory[i]) T(args...);
    __mem::length = n;
  };

  ivector(size_t n, const T &init_value) : __mem(n)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T(init_value);
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = init_value;
    }
    __mem::length = n;
  };

  ivector(size_t n, T &&init_value) : __mem(n)
  {
    T tmp = micron::move(init_value);
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T(tmp);
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = init_value;
    }
    __mem::length = n;
  };

  ivector(chunk<byte> &&m) : __mem(m) { m = nullptr; };

  template <typename C = T> ivector(ivector<C> &&o) : __mem(o.data()) { o.~__mem(); }

  ivector &
  operator=(ivector &&o)
  {
    if ( __mem::memory ) {
      // kill old memory first
      clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  ~ivector()
  {
    if ( __mem::memory == nullptr )
      return;

    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        (__mem::memory)[i].~T();
    }
    this->destroy(to_chunk(__mem::memory, __mem::capacity));
  }

  // equivalent of .data() sortof
  chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(__mem::memory), __mem::capacity };
  }

  // always direct
  template <typename R>
    requires(micron::is_integral_v<R>)
  inline const T &
  operator[](R n) const
  {
    return (__mem::memory)[n];
  }

  T
  at(size_t n) const
  {
    if ( n >= __mem::length )
      exc<except::library_error>("micron::ivector at() out of bounds");
    return (__mem::memory)[n];
  }

  size_t
  at_n(iterator i) const
  {
    if ( i - begin() >= __mem::length )
      exc<except::library_error>("micron::ivector at_n() out of bounds");
    return static_cast<size_t>(i - begin());
  }

  // return const iterator, immutable
  const_iterator
  itr(size_t n) const
  {
    if ( n >= __mem::length )
      exc<except::library_error>("micron::ivector itr() out of bounds");
    return &(__mem::memory)[n];
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline ivector<T>
  append(const ivector<F> &o) const
  {
    micron::ivector<T> buf(__mem::capacity + o.capacity);
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);
    __impl_copy(&buf.memory[__mem::length], &o.memory[0], o.capacity);
    buf.length = __mem::length + o.length;
    return buf;
  }

  template <typename C = T>
  void
  swap(ivector<C> &&o)
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  size_t
  capacity() const
  {
    return __mem::capacity;
  }

  size_t
  size() const
  {
    return __mem::length;
  }

  // no resize
  // + instead of +=
  ivector
  operator+(const ivector &o)
  {
    return append(o);     // equivalent
  }

  template <typename... Args>
  inline ivector
  emplace_back(Args &&...v)
  {
    micron::ivector<T> buf(__mem::capacity + sizeof...(Args) * sizeof(T));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);

    new (&buf.memory[buf.size()]) T(micron::move(micron::forward<Args>(v)...));
    return buf;
  }

  inline const_iterator
  get(const size_t n)
  {
    if ( n > __mem::length )
      exc<except::library_error>("micron::ivector get() out of range");
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_t n)
  {
    if ( n > __mem::length )
      exc<except::library_error>("micron::ivector cget() out of range");
    return &(__mem::memory[n]);
  }

  inline const_iterator
  find(const T &o)
  {
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

  inline iterator
  begin()
  {
    return (__mem::memory);
  }

  inline const_iterator
  cbegin() const
  {
    return (__mem::memory);
  }

  inline iterator
  end()
  {
    return (__mem::memory) + (__mem::length);
  }

  inline const_iterator
  end() const
  {
    return (__mem::memory) + (__mem::length);
  }

  inline const_iterator
  cend() const
  {
    return (__mem::memory) + (__mem::length);
  }

  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]), reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }

  inline ivector<T>
  insert(size_t n, const T &val)
  {
    micron::ivector<T> buf(__mem::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);
    auto i = __mem::length;
    buf.length = i + 1;
    T *its = &(buf.memory)[n];
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(its + 1, its, ite - its);
    //*its = (val);
    new (its) T(val);
    return buf;
  }

  inline ivector<T>
  insert(size_t n, T &&val)
  {
    micron::ivector<T> buf(__mem::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);
    auto i = __mem::length;
    buf.length = i + 1;
    T *its = &(buf.memory)[n];
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(its + 1, its, ite - its);
    //*its = (val);
    new (its) T(val);
    return buf;
  }

  inline ivector<T>
  insert(const_iterator it, T &&val)
  {
    micron::ivector<T> buf(__mem::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);
    auto i = __mem::length;
    buf.length = i + 1;
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    return buf;
  }

  inline ivector<T>
  insert(const_iterator it, const T &val)
  {
    micron::ivector<T> buf(__mem::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);
    auto i = __mem::length;
    buf.length = i + 1;
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(it + 1, it, ite - it);
    new (it) T(val);
    return buf;
  }

  inline ivector<T>
  assign(const size_t cnt, const T &val)
  {
    micron::ivector<T> buf(__mem::capacity + (sizeof(T) * cnt));
    auto *i = buf.begin();
    for ( size_t j = 0; j < cnt; j++ ) {
      new ((T *)(i + j)) T(val);
    }
    buf.length = __mem::length + cnt;
    return buf;
  }

  inline ivector<T>
  push_back(const T &v)
  {
    micron::ivector<T> buf(__mem::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);

    new (&buf.__itr(__mem::length)) T(v);

    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector<T>
  push_back(T &&v)
  {
    micron::ivector<T> buf(__mem::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[0], &__mem::memory[0], __mem::capacity);

    new (buf.__itr(__mem::length)) T(micron::move(v));

    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector<T>
  push_front(const T &v)
  {
    micron::ivector<T> buf(__mem::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[1], &__mem::memory[0], __mem::capacity);

    new (&buf.__itr(0)) T(v);

    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector<T>
  push_front(T &&v)
  {
    micron::ivector<T> buf(__mem::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[1], &__mem::memory[0], __mem::capacity);

    new (buf.__itr(0)) T(micron::move(v));

    buf.length = __mem::length + 1;
    return buf;
  }

  inline void
  erase(const_iterator n)
  {
    if constexpr ( micron::is_class<T>::value ) {
      *n->~T();
    } else {
    }
    for ( size_t i = n; i < (__mem::length - 1); i++ )
      (*n)[i] = micron::move((__mem::memory)[i + 1]);

    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
  }

  inline void
  erase(const size_t n)
  {
    if constexpr ( micron::is_class<T>::value ) {
      ~(__mem::memory)[n]();
    } else {
    }
    for ( size_t i = n; i < (__mem::length - 1); i++ )
      (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
    __mem::length--;
  }

  inline ivector<T>
  clear()
  {
    return ivector<T>(micron::move(__mem::capacity));
  }

  inline T
  front()
  {
    return (__mem::memory)[0];
  }

  inline T
  back()
  {
    return (__mem::memory)[__mem::length - 1];
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }

  // access at element
};

template <typename T>
auto
to_persist(micron::vector<T> &vec)
{
  return ivector<T>();
}

};     // namespace micron
