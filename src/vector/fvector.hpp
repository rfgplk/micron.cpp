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
#include "../allocator.hpp"
#include "../concepts.hpp"
#include "../container_safety.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../memory/new.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"

namespace micron
{

namespace __impl
{
inline constexpr usize
fgrow(usize cap)
{
  return cap == 0 ? 8 : cap * 2;
}
};      // namespace __impl

template<is_regular_object T, class Alloc = micron::allocator_serial<>, bool Sf = true>
class fvector: public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;

public:
  using category_type = vector_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  // NOTE: by convention destructors are first
  ~fvector(void)
  {
    if ( __mem::is_zero() ) return;
    clear();
  }

  fvector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      size_type i = 0;
      for ( const T &value : lst ) new (addr(__mem::memory[i++])) T(value);
      __mem::length = lst.size();
    } else {
      size_type i = 0;
      for ( T value : lst ) __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  fvector(void) : __mem() { }

  fvector(const size_type n) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
  }

  template<typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  fvector(const size_type n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::generate(begin(), end(), fn);
  }

  template<typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  fvector(const size_type n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::transform(begin(), end(), fn);
  }

  template<typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  fvector(size_type n, Args... args) : __mem(n)
  {
    for ( size_type i = 0; i < n; i++ ) new (addr(__mem::memory[i])) T(args...);
    __mem::length = n;
  }

  fvector(size_type n, const T &init_value) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), init_value, n);
    __mem::length = n;
  }

  fvector(const fvector &) = delete;

  fvector(chunk<byte> &&m) : __mem(m) { m = nullptr; }

  template<typename C = T, bool Sf2 = Sf> fvector(fvector<C, Alloc, Sf2> &&o) : __mem(micron::move(o)) { }

  fvector(fvector &&o) : __mem(micron::move(o)) { }

  fvector &operator=(const fvector &) = delete;

  fvector &
  operator=(fvector &&o)
  {
    if constexpr ( Sf == true ) {
      if ( this == micron::addressof(o) ) return *this;
    }
    if ( __mem::memory ) {
      clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  template<typename... Args>
  fvector &
  operator+=(Args &&...args)
  {
    push_back(micron::move(args)...);
    return *this;
  }

  chunk<byte>
  operator*(void) const
  {
    return __mem::data();
  }

  const_pointer
  data(void) const
  {
    return __mem::memory;
  }

  pointer
  data(void)
  {
    return __mem::memory;
  }

  bool
  operator!(void) const
  {
    return empty();
  }

  byte *
  operator&(void)
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }

  const byte *
  operator&(void) const
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }

  inline slice<T>
  operator[](void)
  {
    return slice<T>(begin(), end());
  }

  inline const slice<T>
  operator[](void) const
  {
    return slice<T>(begin(), end());
  }

  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  inline __attribute__((always_inline)) const T &
  operator[](size_type n) const
  {
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  operator[](size_type n)
  {
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  at(size_type n)
  {
    return (__mem::memory)[n];
  }

  size_type
  at_n(iterator i) const
  {
    return static_cast<size_type>(i - begin());
  }

  T *
  itr(size_type n)
  {
    return &(__mem::memory)[n];
  }

  template<typename F, bool Sf2 = Sf>
    requires(sizeof(T) == sizeof(F))
  inline fvector &
  append(const fvector<F, Alloc, Sf2> &o)
  {
    if ( o.empty() ) return *this;
    if ( !__mem::has_space(o.length) ) reserve(__mem::capacity + o.max_size());
    __impl_container::copy(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), o.length);
    __mem::length += o.length;
    return *this;
  }

  template<typename F, bool Sf2 = Sf>
    requires(sizeof(T) == sizeof(F))
  inline fvector &
  weld(fvector<F, Alloc, Sf2> &&o)
  {
    if ( o.empty() ) return *this;
    if ( !__mem::has_space(o.length) ) reserve(__mem::capacity + o.max_size());
    __impl_container::move(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    __mem::length += o.length;
    o.length = 0;
    return *this;
  }

  template<typename C = T, bool Sf2 = Sf>
  void
  swap(fvector<C, Alloc, Sf2> &o) noexcept
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  size_type
  max_size(void) const
  {
    return __mem::capacity;
  }

  size_type
  size(void) const
  {
    return __mem::length;
  }

  void
  set_size(const size_type n)
  {
    __mem::length = n;
  }

  bool
  empty(void) const
  {
    return __mem::length == 0;
  }

  inline void
  reserve(const size_type n)
  {
    if ( n < __mem::capacity ) return;
    if ( __mem::is_zero() ) {
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline void
  try_reserve(const size_type n)
  {
    if ( __mem::is_zero() ) {
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline slice<byte>
  into_bytes(void)
  {
    if ( __mem::memory == nullptr || __mem::length == 0 ) return slice<byte>(nullptr, nullptr);
    return slice<byte>(reinterpret_cast<byte *>(__mem::memory), reinterpret_cast<byte *>(__mem::memory + __mem::length));
  }

  inline fvector<T, Alloc, Sf>
  clone(void)
  {
    return fvector<T, Alloc, Sf>(micron::move(*this));
  }

  void
  fill(const T &v)
  {
    for ( size_type i = 0; i < __mem::length; i++ ) __mem::memory[i] = v;
  }

  void
  resize(size_type n, const T &v)
  {

    if ( n == __mem::length ) return;
    if ( n < __mem::length ) {
      if constexpr ( !micron::is_trivially_destructible_v<T> ) {
        for ( size_type i = n; i < __mem::length; ++i ) __mem::memory[i].~T();
      }
      __mem::length = n;
      return;
    }
    if ( n >= __mem::capacity ) reserve(n);
    for ( size_type i = __mem::length; i < n; i++ ) new (addr(__mem::memory[i])) T(v);
    __mem::length = n;
  }

  void
  resize(size_type n)
  {
    if ( n == __mem::length ) return;
    if ( n < __mem::length ) {
      if constexpr ( !micron::is_trivially_destructible_v<T> ) {
        for ( size_type i = n; i < __mem::length; ++i ) __mem::memory[i].~T();
      }
      __mem::length = n;
      return;
    }
    if ( n >= __mem::capacity ) reserve(n);
    for ( size_type i = __mem::length; i < n; i++ ) new (addr(__mem::memory[i])) T{};
    __mem::length = n;
  }

  template<typename... Args>
  void
  emplace_back(Args &&...v)
  {
    if ( __mem::length < __mem::capacity ) {
      new (addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    } else {
      reserve(__impl::fgrow(__mem::capacity));
      new (addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    }
  }

  void
  move_back(T &&t)
  {
    if ( __mem::length < __mem::capacity ) {
      new (addr(__mem::memory[__mem::length++])) T(micron::move(t));
    } else {
      reserve(__impl::fgrow(__mem::capacity));
      new (addr(__mem::memory[__mem::length++])) T(micron::move(t));
    }
  }

  inline iterator
  get(const size_type n)
  {
    return &(__mem::memory[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    return &(__mem::memory[n]);
  }

  inline iterator
  find(const T &o)
  {
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( __mem::memory[i] == o ) return &__mem::memory[i];
    return nullptr;
  }

  inline const_iterator
  find(const T &o) const
  {
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( __mem::memory[i] == o ) return &__mem::memory[i];
    return nullptr;
  }

  inline iterator
  begin(void)
  {
    return __mem::memory;
  }

  inline const_iterator
  begin(void) const
  {
    return __mem::memory;
  }

  inline const_iterator
  cbegin(void) const
  {
    return __mem::memory;
  }

  inline iterator
  end(void)
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  end(void) const
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  cend(void) const
  {
    return __mem::memory + __mem::length;
  }

  inline iterator
  last(void)
  {
    return __mem::memory + (__mem::length - 1);
  }

  inline const_iterator
  last(void) const
  {
    return __mem::memory + (__mem::length - 1);
  }

  inline iterator
  insert(size_type n, const T &val, size_type cnt)
  {
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i ) push_back(val);
      return begin();
    }
    if ( __mem::length + cnt > __mem::capacity ) reserve(__impl::fgrow(__mem::capacity + cnt));
    __impl_container::open_gap(__mem::memory, __mem::length, n, cnt);
    for ( size_type i = 0; i < cnt; ++i ) new (addr(__mem::memory[n + i])) T(val);
    __mem::length += cnt;
    return addr(__mem::memory[n]);
  }

  inline iterator
  insert(size_type n, const T &val)
  {
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity ) reserve(__impl::fgrow(__mem::capacity));
    __impl_container::open_gap(__mem::memory, __mem::length, n, 1);
    new (addr(__mem::memory[n])) T(val);
    __mem::length++;
    return addr(__mem::memory[n]);
  }

  inline iterator
  insert(size_type n, T &&val)
  {
    if ( !__mem::length ) {
      emplace_back(micron::move(val));
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity ) reserve(__impl::fgrow(__mem::capacity));
    __impl_container::open_gap(__mem::memory, __mem::length, n, 1);
    new (addr(__mem::memory[n])) T(micron::move(val));
    __mem::length++;
    return addr(__mem::memory[n]);
  }

  inline iterator
  insert(iterator it, const T &val, const size_type cnt)
  {
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i ) push_back(val);
      return begin();
    }
    const size_type p = static_cast<size_type>(it - __mem::memory);
    if ( __mem::length + cnt > __mem::capacity ) reserve(__impl::fgrow(__mem::capacity + cnt));
    __impl_container::open_gap(__mem::memory, __mem::length, p, cnt);
    for ( size_type i = 0; i < cnt; ++i ) new (addr(__mem::memory[p + i])) T(val);
    __mem::length += cnt;
    return addr(__mem::memory[p]);
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    if ( !__mem::length ) {
      emplace_back(micron::move(val));
      return begin();
    }
    const size_type p = static_cast<size_type>(it - __mem::memory);
    if ( __mem::length + 1 > __mem::capacity ) reserve(__impl::fgrow(__mem::capacity));
    __impl_container::open_gap(__mem::memory, __mem::length, p, 1);
    new (addr(__mem::memory[p])) T(micron::move(val));
    __mem::length++;
    return addr(__mem::memory[p]);
  }

  inline iterator
  insert(iterator it, const T &val)
  {
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    const size_type p = static_cast<size_type>(it - __mem::memory);
    if ( __mem::length + 1 > __mem::capacity ) reserve(__impl::fgrow(__mem::capacity));
    __impl_container::open_gap(__mem::memory, __mem::length, p, 1);
    new (addr(__mem::memory[p])) T(val);
    __mem::length++;
    return addr(__mem::memory[p]);
  }

  inline void
  sort(void)
  {
    if ( size() < 100000 )
      micron::sort::heap(*this);
    else
      micron::sort::quick(*this);
  }

  iterator
  insert_sort(T &&val)
  {
    if ( !__mem::length ) {
      push_back(micron::move(val));
      return begin();
    }
    if ( __mem::length + 1 >= __mem::capacity ) reserve(__impl::fgrow(__mem::capacity));

    size_type pos = 0;
    for ( ; pos < __mem::length; ++pos )
      if ( __mem::memory[pos] >= val ) break;

    T *it = __mem::memory + pos;
    T *ite = end();
    micron::memmove(it + 1, it, (ite - it));
    new (it) T(micron::move(val));
    __mem::length++;
    return it;
  }

  inline fvector &
  assign(const size_type cnt, const T &val)
  {
    if ( cnt >= __mem::capacity ) reserve(__impl::fgrow(__mem::capacity > cnt ? __mem::capacity : cnt));
    clear();
    for ( size_type i = 0; i < cnt; i++ ) new (addr(__mem::memory[i])) T(val);
    __mem::length = cnt;
    return *this;
  }

  __attribute__((always_inline)) void
  inline_push_back(const T &v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (addr(__mem::memory[__mem::length++])) T(v);
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        new (addr(__mem::memory[__mem::length++])) T(v);
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = v;
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        __mem::memory[__mem::length++] = v;
      }
    }
  }

  __attribute__((always_inline)) void
  inline_push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (addr(__mem::memory[__mem::length++])) T(micron::move(v));
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        new (addr(__mem::memory[__mem::length++])) T(micron::move(v));
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = micron::move(v);
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  void
  push_back(const T &v)
  {
    // WARNING: for all qualifiable containers, be sure to ALWAYS use addr() or addr() semantically equivalent calls (yields true
    // pointer/addr to object of type object), operator& may be overloaded, causing construction to point to junk memory or worse (likely
    // will cause catastrophic UB/segfaulting) for types where the two operations are equivalent (operator& yielding a byte* to internal
    // storage), this will silently succeed (in cases where operator& yields a byte*)
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (addr(__mem::memory[__mem::length++])) T(v);
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        new (addr(__mem::memory[__mem::length++])) T(v);
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = v;
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        __mem::memory[__mem::length++] = v;
      }
    }
  }

  void
  push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (addr(__mem::memory[__mem::length++])) T(micron::move(v));
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        new (addr(__mem::memory[__mem::length++])) T(micron::move(v));
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = micron::move(v);
      } else {
        reserve(__impl::fgrow(__mem::capacity));
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  inline void
  pop_back(void)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[__mem::length - 1].~T();
    else
      (__mem::memory)[__mem::length - 1] = T{};
    --__mem::length;
    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(addr(__mem::memory[__mem::length])));
  }

  inline void
  __remove(const T &val)
  {
  remove_goto:
    auto itr = find(val);
    if ( itr == nullptr ) return;
    erase(itr);
    goto remove_goto;
  }

  template<typename... Args>
    requires(micron::convertible_to<T, Args> && ...)
  inline void
  remove(const Args &...val)
  {
    (__remove(static_cast<T>(val)), ...);
  }

  inline void
  erase(iterator first, iterator last)
  {
    const size_type p = static_cast<size_type>(first - __mem::memory);
    const size_type count = static_cast<size_type>(last - first);
    __impl_container::close_gap(__mem::memory, __mem::length, p, count);
    __mem::length -= count;
  }

  inline void
  erase(size_type from, size_type to)
  {
    const size_type count = to - from;
    __impl_container::close_gap(__mem::memory, __mem::length, from, count);
    __mem::length -= count;
  }

  inline void
  erase(iterator it)
  {
    const size_type p = static_cast<size_type>(it - __mem::memory);
    __impl_container::close_gap(__mem::memory, __mem::length, p, 1);
    --__mem::length;
  }

  inline void
  erase(const size_type n)
  {
    __impl_container::close_gap(__mem::memory, __mem::length, n, 1);
    --__mem::length;
  }

  inline void
  fast_clear(void)
  {
    if constexpr ( !micron::is_class_v<T> )
      __mem::length = 0;
    else
      clear();
  }

  inline void
  clear(void)
  {
    if ( !__mem::length ) return;
    __impl_container::destroy_fast(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  inline const T &
  front(void) const
  {
    return (__mem::memory)[0];
  }

  inline const T &
  back(void) const
  {
    return (__mem::memory)[__mem::length - 1];
  }

  inline T &
  front(void)
  {
    return (__mem::memory)[0];
  }

  inline T &
  back(void)
  {
    return (__mem::memory)[__mem::length - 1];
  }

  static constexpr bool
  is_pod(void)
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
};

}      // namespace micron
