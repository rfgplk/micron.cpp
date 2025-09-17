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
#include "../algorithm/mem.hpp"
#include "../allocation/resources.hpp"
#include "../container_safety.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"
namespace micron
{
// Regular vector class, always safe, mutable, notthread safe, cannot be
// copied for performance reasons (just move it, or use a slice for that)
template <typename T, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T> && micron::is_copy_assignable_v<T>
           && micron::is_move_assignable_v<T>
class vector : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;

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
  ~vector()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }
  vector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
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
  vector(void) : __mem() {};
  vector(const size_t n) : __mem(n)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
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
  vector(size_t n, Args... args) : __mem(n)
  {
    for ( size_t i = 0; i < n; i++ )
      new (&__mem::memory[i]) T(args...);
    __mem::length = n;
  };
  vector(size_t n, const T &init_value) : __mem(n)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T(init_value);
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = init_value;
    }
    __mem::length = n;
  };
  vector(size_t n, T &&init_value) : __mem(n)
  {
    T tmp = micron::move(init_value);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( size_t i = 0; i < n; i++ )
        new (&__mem::memory[i]) T(tmp);
    } else {
      for ( size_t i = 0; i < n; i++ )
        __mem::memory[i] = init_value;
    }
    __mem::length = n;
  };
  vector(const vector &o) : __mem(o.length)
  {
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
  }
  vector(chunk<byte> &&m) : __mem(m) { m = nullptr; };
  template <typename C = T> vector(vector<C> &&o) : __mem(micron::move(o)) {}
  vector(vector &&o) : __mem(micron::move(o)) {}
  vector &
  operator=(const vector &o)
  {
    __mem::memory = nullptr;
    __mem::capacity = 0;
    realloc(o.length);
    __mem::length = o.length;
    __impl_container::copy(__mem::memory, o.memory, o.length);
    return *this;
  }
  vector &
  operator=(vector &&o)
  {
    if ( __mem::memory ) {
      // kill old memory first
      clear();
      __mem::free();
    }
    __mem::operator=(o);
    return *this;
  }

  template <typename... Args>
  vector &
  operator+=(Args &&...args)
  {
    push_back(micron::move(args)...);
    return *this;
  }
  // equivalent of .data() sortof
  chunk<byte>
  operator*() const
  {
    return __mem::data();
  }
  const_pointer
  data() const
  {
    return &__mem::memory[0];
  }
  pointer
  data()
  {
    return &__mem::memory[0];
  }
  bool
  operator!() const
  {
    return empty();
  }
  // overload this to always point to mem
  byte *
  operator&() volatile
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }
  const byte *
  operator&() const volatile
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }
  inline slice<T>
  operator[]()
  {
    return slice<T>(begin(), end());
  }
  inline const slice<T>
  operator[]() const
  {
    return slice<T>(begin(), end());
  }
  // copies vector out
  inline __attribute__((always_inline)) const slice<T>
  operator[](size_t from, size_t to) const
  {
    // meant to be safe so this is here
    if ( from >= to or from > __mem::capacity or to > __mem::capacity )
      throw except::library_error("micron::vector operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }
  inline __attribute__((always_inline)) slice<T>
  operator[](size_t from, size_t to)
  {
    // meant to be safe so this is here
    if ( from >= to or from > __mem::capacity or to > __mem::capacity )
      throw except::library_error("micron::vector operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }
  inline __attribute__((always_inline)) const T &
  operator[](size_t n) const
  {
    // meant to be safe so this is here
    if ( n > __mem::capacity )
      throw except::library_error("micron::vector operator[] out of allocated memory range.");
    return (__mem::memory)[n];
  }
  inline __attribute__((always_inline)) T &
  operator[](size_t n)
  {
    // meant to be safe so this is here
    if ( n > __mem::capacity )
      throw except::library_error("micron::vector operator[] out of allocated memory range.");
    return (__mem::memory)[n];
  }
  inline __attribute__((always_inline)) T &
  at(size_t n)
  {
    if ( n >= __mem::length )
      throw except::library_error("micron::vector at() out of bounds");
    return (__mem::memory)[n];
  }
  size_t
  at_n(iterator i) const
  {
    if ( i - begin() >= __mem::length )
      throw except::library_error("micron::ivector at_n() out of bounds");
    return static_cast<size_t>(i - begin());
  }
  T *
  itr(size_t n)
  {
    if ( n >= __mem::length )
      throw except::library_error("micron::vector at() out of bounds");
    return &(__mem::memory)[n];
  }
  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline vector &
  append(const vector<F> &o)
  {
    if ( o.empty() )
      return *this;
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());

    __impl_container::copy(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), o.length);
    // micron::memcpy(&(__mem::memory)[__mem::length],
    // &o.memory[0],
    //                o.length);
    __mem::length += o.length;
    return *this;
  }
  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline vector &
  weld(vector<F> &&o)
  {
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());
    __impl_container::copy(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    // micron::memcpy(&(__mem::memory)[__mem::length],
    // &o.memory[0],
    //                o.length);
    __mem::length += o.length;
    return *this;
  }
  template <typename C = T>
  void
  swap(vector<C> &o) noexcept
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
  }
  size_t
  max_size() const
  {
    return __mem::capacity;
  }
  size_t
  size() const
  {
    return __mem::length;
  }
  void
  set_size(const size_t n)
  {
    __mem::length = n;
  }
  bool
  empty() const
  {
    return __mem::length == 0 ? true : false;
  }

  // grow container
  inline void
  reserve(const size_t n)
  {
    if ( n < __mem::capacity )
      return;
    __mem::expand(n);
  }
  inline void
  try_reserve(const size_t n)
  {
    if ( n < __mem::capacity )
      throw except::memory_error("micron vector failed to reserve memory");
    __mem::expand(n);
  }
  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]),
                       reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }
  inline vector<T>
  clone(void)
  {
    return vector<T>(*this);
  }
  void
  fill(const T &v)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = v;
  }
  // resize to how much and fill with a value v
  void
  resize(size_t n, const T &v)
  {
    if ( !(n > __mem::length) ) {
      return;
    }
    if ( n >= __mem::capacity ) {
      reserve(n);
    }
    T *f_ptr = __mem::memory;
    for ( size_t i = __mem::length; i < n; i++ )
      new (&f_ptr[i]) T(v);

    __mem::length = n;
  }
  template <typename... Args>
    requires(micron::is_class_v<T>)
  inline void
  emplace_back(Args &&...v)
  {
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(v)...);
      return;
    } else {
      reserve(__mem::capacity + 1);
      new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(v)...);
    }
  }
  template <typename V = T>
    requires(!micron::is_class_v<T>) && (!micron::is_class_v<V>)
  inline void emplace_back(V v)
  {
    if ( __mem::length < __mem::capacity ) {
      __mem::memory[__mem::length++] = static_cast<T>(v);
      return;
    } else {
      reserve(__mem::capacity + 1);
      __mem::memory[__mem::length++] = static_cast<T>(v);
    }
  }
  inline iterator
  get(const size_t n)
  {
    if ( n > __mem::length )
      throw except::library_error("micron::vector get() out of range");
    return &(__mem::memory[n]);
  }
  inline const_iterator
  get(const size_t n) const
  {
    if ( n > __mem::length )
      throw except::library_error("micron::vector get() out of range");
    return &(__mem::memory[n]);
  }
  inline const_iterator
  cget(const size_t n) const
  {
    if ( n > __mem::length )
      throw except::library_error("micron::vector cget() out of range");
    return &(__mem::memory[n]);
  }
  inline iterator
  find(const T &o)
  {
    T *f_ptr = __mem::memory;
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }
  inline const_iterator
  find(const T &o) const
  {
    T *f_ptr = __mem::memory;
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }
  inline iterator
  begin()
  {
    return (__mem::memory);
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
  inline iterator
  last()
  {
    return (__mem::memory) + (__mem::length - 1);
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
    if ( !__mem::length ) {
      for ( size_t i = 0; i < cnt; ++i )
        push_back(val);
      return begin();
    }
    if ( __mem::length + cnt > __mem::capacity )
      reserve(__mem::capacity + 1);
    if ( n >= __mem::length )
      throw except::library_error("micron::vector insert(): out of allocated memory range.");
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
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__mem::capacity + 1);
    if ( n >= __mem::length )
      throw except::library_error("micron::vector insert(): out of allocated memory range.");
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
    if ( !__mem::length ) {
      emplace_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__mem::capacity + 1);
    if ( n >= __mem::length )
      throw except::library_error("micron::vector insert(): out of allocated memory range.");
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
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( (__mem::length) >= __mem::capacity ) {
      size_t dif = static_cast<size_t>(it - __mem::memory);
      reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }     // invalidated if
    if ( it >= end() or it < begin() )
      throw except::library_error("micron::vector insert(): out of allocated memory range.");
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
    if ( !__mem::length ) {
      for ( size_t i = 0; i < cnt; ++i )
        push_back(val);
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_t dif = it - __mem::memory;
      reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }
    if ( it >= end() or it < begin() )
      throw except::library_error("micron::vector insert(): out of allocated memory range.");
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
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_t dif = it - __mem::memory;
      reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }
    if ( it >= end() or it < begin() )
      throw except::library_error("micron::vector insert(): out of allocated memory range.");
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
    if ( size() < 100000 )
      micron::sort::heap(*this);     // NOTE: need an inplace sort since *this cannot be copied
    else
      micron::sort::quick(*this);
  }
  inline iterator
  insert_sort(T &&val)     // NOTE: we won't check if this is presort, bad things will happen if it isn't
  {
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( (__mem::length + 1) >= __mem::capacity ) {
      reserve(__mem::capacity + 1);
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

  inline vector &
  assign(const size_t cnt, const T &val)
  {
    if ( (cnt * (sizeof(T) / sizeof(byte))) >= __mem::capacity ) {
      reserve(__mem::capacity + (cnt) * sizeof(T));
    }
    clear();     // clear the vec
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
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        new (&__mem::memory[__mem::length++]) T(v);
        return;
      } else {
        reserve(__mem::capacity * sizeof(T) + 1);
        new (&__mem::memory[__mem::length++]) T(v);
      }
    } else {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        __mem::memory[__mem::length++] = v;
        return;
      } else {
        reserve(__mem::capacity * sizeof(T) + 1);
        __mem::memory[__mem::length++] = v;
      }
    }
  }
  inline void
  push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        new (&__mem::memory[__mem::length++]) T(v);
        return;
      } else {
        reserve(__mem::capacity * sizeof(T) + 1);
        new (&__mem::memory[__mem::length++]) T(v);
      }
    } else {
      if ( (__mem::length + 1 <= __mem::capacity) ) {
        __mem::memory[__mem::length++] = micron::move(v);
        return;
      } else {
        reserve(__mem::capacity * sizeof(T) + 1);
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  inline void
  pop_back()
  {
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
    if ( n >= end() or n < begin() )
      throw except::library_error("micron::vector erase(): out of allocated memory range.");
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
    if ( n >= size() )
      throw except::library_error("micron::vector erase(): out of allocated memory range.");
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
    if ( !__mem::length )
      return;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        (__mem::memory)[i].~T();
    }
    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __mem::capacity * (sizeof(T) / sizeof(byte)));
    __mem::length = 0;
  }

  inline const T &
  front() const
  {
    return (__mem::memory)[0];
  }
  inline const T &
  back() const
  {
    return (__mem::memory)[__mem::length - 1];
  }
  inline T &
  front()
  {
    return (__mem::memory)[0];
  }
  inline T &
  back()
  {
    return (__mem::memory)[__mem::length - 1];
  }
  // access at element
};
};     // namespace micron
