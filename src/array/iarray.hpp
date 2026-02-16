//  Distributed under the Bost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../tags.hpp"
#include "../types.hpp"

namespace micron
{
// general purpose immutable iarray class, stack allocated, threadsafe, immutable.
// default to 64
template <is_movable_object T, size_t N = 64>
  requires(N > 0)     // avoid weird stuff with N = 0
class iarray
{
  alignas(alignof(T)) T stack[N];
  inline void
  __impl_zero(T *src)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(T());
    } else {
      micron::cmemset<N>(src, 0x0);
    }
  }
  void
  __impl_set(T *__restrict src, const T &val)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = val;
    } else {
      micron::cmemset<N>(src, val);
    }
  }
  void
  __impl_copy(const T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < N; i++ )
        dest[i] = src[i];
    } else {
      micron::copy<N>(src, dest);
    }
  }
  void
  __impl_move(T *__restrict src, const T *__restrict dest)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < N; i++ )
        dest[i] = micron::move(src[i]);
    } else {
      micron::copy<N>(src, dest);
      micron::czero<N>(src);
    }
  }
  void
  __impl_copy(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < N; i++ )
        dest[i] = src[i];
    } else {
      micron::copy<N>(src, dest);
    }
  }
  void
  __impl_move(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < N; i++ )
        dest[i] = micron::move(src[i]);
    } else {
      micron::copy<N>(src, dest);
      micron::czero<N>(src);
    }
  }

public:
  using category_type = array_tag;
  using mutability_type = immutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~iarray()
  {
    // explicit
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i].~T();
    } else {
      micron::czero<N>(micron::addr(stack[0]));
    }
  }
  iarray() { __impl_zero(micron::addr(stack[0])); }
  iarray(const T &o) { __impl_set(micron::addr(stack[0]), o); }
  iarray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw except::runtime_error("micron::iarray iarray(init_list): init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }
  template <is_container A>
    requires(!micron::is_same_v<A, iarray>)
  iarray(const A &o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::iarray iarray(const&) invalid size");
    __impl_copy(micron::addr(o[0]), micron::addr(stack[0]));
  }
  template <is_container A>
    requires(!micron::is_same_v<A, iarray>)
  iarray(A &&o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::iarray iarray(&&) invalid size");
    __impl_move(micron::addr(o[0]), micron::addr(stack[0]));
  }
  iarray(const iarray &o)
  {
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }     // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]); }
  iarray(iarray &&o)
  {
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
    // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]);
    // micron::cmemset<N>(micron::addr(stack[0], 0x0);
  }
  const_iterator
  begin() const noexcept
  {
    return micron::addr(stack[0]);
  }
  const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }
  const_iterator
  end() const noexcept
  {
    return micron::addr(stack[N]);
  }
  const_iterator
  cend() const noexcept
  {
    return micron::addr(stack[N]);
  }
  size_t
  size() const
  {
    return N;
  }
  size_t
  max_size() const
  {
    return N;
  }
  const T *
  data() const
  {
    return stack;
  }
  const T &
  at(const size_t i) const
  {
    if ( i >= N )
      throw except::runtime_error("micron::iarray at() out of range.");
    return stack[i];
  }
  inline const T &
  operator[](const size_t i) const
  {
    return stack[i];
  }
  inline T &
  mut(const size_t i)
  {
    return stack[i];
  }
  template <typename F, size_t M>
  iarray &
  operator=(T (&o)[M])
    requires(M <= N)
  {
    __impl_copy(micron::addr(o.stack[0]), &o[0]);
    // micron::copy<N>(micron::addr(o.stack[0], &o[0]);
    return *this;
  }
  iarray &
  operator=(iarray &&o)
  {
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
    return *this;
  }

  template <size_t M>
  auto
  operator+(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) += o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) += o.stack[i];
      return arr;
    }
  }
  template <size_t M>
  auto
  operator-(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) -= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) -= o.stack[i];
      return arr;
    }
  }
  template <size_t M>
  auto
  operator*(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) *= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) *= o.stack[i];
      return arr;
    }
  }
  template <size_t M>
    requires(M <= N)
  auto
  operator/(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) /= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) /= o.stack[i];
      return arr;
    }
  }
  template <size_t M>
    requires(M <= N)
  auto
  operator%(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) %= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_t i = 0; i < N; i++ )
        arr.mut(i) %= o.stack[i];
      return arr;
    }
  }

  // special functions - no idea why the stl doesn't have these
  size_t
  sum(void) const
  {
    size_t sm = 0;
    for ( size_t i = 0; i < N; i++ )
      sm += stack[i];
    return sm;
  }

  iarray
  operator*=(const T &o)
  {
    iarray arr(*this);
    for ( size_t i = 0; i < N; i++ )
      arr.mut(i) *= o;
    return arr;
  }
  iarray
  operator/=(const T &o)
  {
    iarray arr(*this);
    for ( size_t i = 0; i < N; i++ )
      arr.mut(i) /= o;
    return arr;
  }
  iarray
  operator-=(const T &o)
  {
    iarray arr(*this);
    for ( size_t i = 0; i < N; i++ )
      arr.mut(i) -= o;
    return arr;
  }
  iarray
  operator+=(const T &o)
  {
    iarray arr(*this);
    for ( size_t i = 0; i < N; i++ )
      arr.mut(i) += o;
    return arr;
  }
  bool
  all(const T &o) const
  {
    for ( size_t i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }
  bool
  any(const T &o) const
  {
    for ( size_t i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }
};
};     // namespace micron
