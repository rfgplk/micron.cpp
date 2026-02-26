//  Distributed under the Bost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/algorithm.hpp"
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
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))     // avoid weird stuff with N = 0
class iarray
{
  alignas(alignof(T)) T stack[N];

public:
  using category_type = array_tag;
  using mutability_type = immutable_tag;
  using memory_type = stack_tag;
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

  ~iarray() { __impl_container::destroy<N>(micron::addr(stack[0])); }

  iarray() { __impl_container::zero<N>(micron::addr(stack[0])); }

  template <typename Fn>

    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  iarray(Fn &&fn)
  {
    __impl_container::set<N>(micron::addr(stack[0]), T{});
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  iarray(Fn &&fn)
  {
    __impl_container::set<N>(micron::addr(stack[0]), T{});
    micron::transform(begin(), end(), fn);
  }

  iarray(const T &o) { __impl_container::set(micron::addr(stack[0]), o); }

  iarray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw except::runtime_error("micron::iarray iarray(init_list): init_list too large.");
    size_type i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }

  template <is_container A>
    requires(!micron::is_same_v<A, iarray>)
  iarray(const A &o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::iarray iarray(const&) invalid size");
    __impl_container::copy<N>(micron::addr(o[0]), micron::addr(stack[0]));
  }

  template <is_container A>
    requires(!micron::is_same_v<A, iarray>)
  iarray(A &&o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::iarray iarray(&&) invalid size");
    __impl_container::move<N>(micron::addr(o[0]), micron::addr(stack[0]));
  }

  iarray(const iarray &o)
  {
    __impl_container::copy<N>(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }     // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]); }

  iarray(iarray &&o)
  {
    __impl_container::move<N>(micron::addr(o.stack[0]), micron::addr(stack[0]));
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

  size_type
  size() const
  {
    return N;
  }

  size_type
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
  at(const size_type i) const
  {
    if ( i >= N )
      throw except::runtime_error("micron::iarray at() out of range.");
    return stack[i];
  }

  inline const T &
  operator[](const size_type i) const
  {
    return stack[i];
  }

  inline T &
  mut(const size_type i)
  {
    return stack[i];
  }

  template <typename F, size_type M>
  iarray &
  operator=(T (&o)[M])
    requires(M <= N)
  {
    __impl_container::copy<N>(micron::addr(o.stack[0]), &o[0]);
    // micron::copy<N>(micron::addr(o.stack[0], &o[0]);
    return *this;
  }

  iarray &
  operator=(iarray &&o)
  {
    __impl_container::move<N>(micron::addr(o.stack[0]), micron::addr(stack[0]));
    return *this;
  }

  template <size_type M>
  auto
  operator+(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) += o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) += o.stack[i];
      return arr;
    }
  }

  template <size_type M>
  auto
  operator-(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) -= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) -= o.stack[i];
      return arr;
    }
  }

  template <size_type M>
  auto
  operator*(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) *= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) *= o.stack[i];
      return arr;
    }
  }

  template <size_type M>
    requires(M <= N)
  auto
  operator/(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) /= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) /= o.stack[i];
      return arr;
    }
  }

  template <size_type M>
    requires(M <= N)
  auto
  operator%(const iarray<T, M> &o)
  {
    if constexpr ( M <= N ) {
      iarray arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) %= o.stack[i];
      return arr;
    } else {
      iarray<T, M> arr(*this);
      for ( size_type i = 0; i < N; i++ )
        arr.mut(i) %= o.stack[i];
      return arr;
    }
  }

  // special functions - no idea why the stl doesn't have these
  size_type
  sum(void) const
  {
    size_type sm = 0;
    for ( size_type i = 0; i < N; i++ )
      sm += stack[i];
    return sm;
  }

  iarray
  operator*=(const T &o)
  {
    iarray arr(*this);
    for ( size_type i = 0; i < N; i++ )
      arr.mut(i) *= o;
    return arr;
  }

  iarray
  operator/=(const T &o)
  {
    iarray arr(*this);
    for ( size_type i = 0; i < N; i++ )
      arr.mut(i) /= o;
    return arr;
  }

  iarray
  operator-=(const T &o)
  {
    iarray arr(*this);
    for ( size_type i = 0; i < N; i++ )
      arr.mut(i) -= o;
    return arr;
  }

  iarray
  operator+=(const T &o)
  {
    iarray arr(*this);
    for ( size_type i = 0; i < N; i++ )
      arr.mut(i) += o;
    return arr;
  }

  bool
  all(const T &o) const
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }

  bool
  any(const T &o) const
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }
};
};     // namespace micron
