//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include <initializer_list>     // nigh impossible to implement without invoking the darkest of sorceries :c

#include "../algorithm/algorithm.hpp"
#include "../algorithm/mem.hpp"
#include "../allocation/resources.hpp"
#include "../allocator.hpp"
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

template <typename T, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T> && micron::is_copy_assignable_v<T>
           && micron::is_move_assignable_v<T>
class fvector : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  // shallow copy routine
  inline void
  shallow_copy(T *dest, T *src, size_t cnt)
  {
    micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
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
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      deep_copy(dest, src, cnt);
    } else {
      shallow_copy(dest, src, cnt);
    }
  }
  inline void
  shallow_move(T *dest, T *src, size_t cnt)
  {
    micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src), cnt);
    micron::memset(reinterpret_cast<byte *>(src), 0x0, cnt);
  };
  inline void
  deep_move(T *dest, T *src, size_t cnt)
  {
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = micron::move(src[i]);
  };
  inline void
  __impl_move(T *dest, T *src, size_t cnt)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      deep_move(dest, src, cnt);
    } else {
      shallow_move(dest, src, cnt);
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
  ~fvector()
  {
    if ( __mem::is_zero() )
      return;
    clear();
    // this->destroy(to_chunk(__mem::memory, __mem::capacity));
  }
  fvector(const std::initializer_list<T> &lst) : __mem(lst.size())
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
  fvector(void) : __mem() {};
  fvector(const size_t n) : __mem(n)
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
  fvector(size_t n, Args... args) : __mem(n)
  {
    for ( size_t i = 0; i < n; i++ )
      new (&__mem::memory[i]) T(args...);
    __mem::length = n;
  };
  fvector(size_t n, const T &init_value) : __mem(n)
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
  fvector(size_t n, T &&init_value) : __mem(n)
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
  fvector(const fvector &) = delete;
  fvector(chunk<byte> &&m) : __mem(m) { m = nullptr; };
  template <typename C = T> fvector(fvector<C> &&o) : __mem(micron::move(o)) {}
  fvector(fvector &&o) : __mem(micron::move(o)) {}
  fvector &operator=(const fvector &) = delete;     // same reasoning as above
  fvector &
  operator=(fvector &&o)
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
  fvector &
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
    return slice<T>(begin(), last());
  }
  inline const slice<T>
  operator[]() const
  {
    return slice<T>(begin(), last());
  }
  // copies fvector out
  inline __attribute__((always_inline)) const slice<T>
  operator[](size_t from, size_t to) const
  {
    return slice<T>(get(from), get(to));
  }
  inline __attribute__((always_inline)) slice<T>
  operator[](size_t from, size_t to)
  {
    return slice<T>(get(from), get(to));
  }
  inline __attribute__((always_inline)) const T &
  operator[](size_t n) const
  {
    return (__mem::memory)[n];
  }
  inline __attribute__((always_inline)) T &
  operator[](size_t n)
  {
    return (__mem::memory)[n];
  }
  inline __attribute__((always_inline)) T &
  at(size_t n)
  {
    return (__mem::memory)[n];
  }
  size_t
  at_n(iterator i) const
  {
    return static_cast<size_t>(i - begin());
  }
  T *
  itr(size_t n)
  {
    return &(__mem::memory)[n];
  }
  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline fvector &
  append(const fvector<F> &o)
  {
    if ( o.empty() )
      return *this;
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());

    __impl_copy(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), o.length);
    // micron::memcpy(&(__mem::memory)[__mem::length],
    // &o.memory[0],
    //                o.length);
    __mem::length += o.length;
    return *this;
  }
  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline fvector &
  weld(fvector<F> &&o)
  {
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());
    __impl_copy(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    // micron::memcpy(&(__mem::memory)[__mem::length],
    // &o.memory[0],
    //                o.length);
    __mem::length += o.length;
    return *this;
  }
  template <typename C = T>
  void
  swap(fvector<C> &o) noexcept
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
    __mem::expand(n);
  }
  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]),
                       reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }
  inline fvector<T>
  clone(void)
  {
    return fvector<T>(*this);
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
      new (&__mem::memory[__mem::length++]) T(micron::move(micron::forward<Args>(v)...));
      return;
    } else {
      reserve(__mem::capacity + 1);
      new (&__mem::memory[__mem::length++]) T(micron::move(micron::forward<Args>(v)...));
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
    return &(__mem::memory[n]);
  }
  inline const_iterator
  get(const size_t n) const
  {
    return &(__mem::memory[n]);
  }
  inline const_iterator
  cget(const size_t n) const
  {
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
  insert(size_t n, const T &val)
  {
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__mem::capacity + 1);
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
    if ( (__mem::length + sizeof(T)) >= __mem::capacity ) {
      size_t dif = static_cast<size_t>(it - __mem::memory);
      reserve(__mem::capacity + 1);
      it = __mem::memory + dif;
    }     // invalidated if
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    //*it = (val);
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

  inline fvector &
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
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      n->~T();
    } else {
    }
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      size_t _n = (n - cbegin());
      for ( size_t i = _n; i < (__mem::length - 1); i++ )
        (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);
    } else {
      __impl_copy(n, n + 1, __mem::length);
    }
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
  }
  inline void
  erase(const size_t n)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      ~(__mem::memory)[n]();
    } else {
    }

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      size_t _n = (n - cbegin());
      for ( size_t i = n; i < (__mem::length - 1); i++ )
        (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);
    } else {
      __impl_copy(__mem::memory[n], __mem::memory[n + 1], __mem::length);
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
