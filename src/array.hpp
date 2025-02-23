#pragma once

#include <stdexcept>
#include <type_traits>

#include "algorithm/mem.hpp"
#include "math/sqrt.hpp"
#include "math/trig.hpp"
#include "memory/memory.hpp"
#include "tags.hpp"
#include "types.hpp"

namespace micron
{
// array
// farray
// carray
// iarray

// general purpose fundamental array class, only allows fundamental types
// (int, char, etc) stack allocated, notthreadsafe, mutable. default to 64
template <class T, size_t N = 64>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> && (N > 0)
           && std::is_fundamental_v<T>     // avoid weird stuff with N = 0
class farray
{
  T stack[N];

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
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

  farray() { micron::czero<N>(&stack[0]); }
  farray(const T &o) { micron::cmemset<N>(&stack[0], o); }
  farray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw std::runtime_error("micron::farray init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }
  farray(const farray &o) { micron::copy<N>(&o.stack[0], &stack[0]); }
  farray(farray &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::cmemset<N>(&stack[0], 0x0);
  }

  size_t
  size() const
  {
    return N;
  }
  inline T &
  at(const size_t i)
  {
    if ( i >= N )
      throw std::runtime_error("micron::farray at() out of range.");
    return stack[i];
  }
  inline const T &
  at(const size_t i) const
  {
    if ( i >= N )
      throw std::runtime_error("micron::farray at() out of range.");
    return stack[i];
  }
  inline T &
  operator[](const size_t i)
  {
    return stack[i];
  }
  inline const T &
  operator[](const size_t i) const
  {
    return stack[i];
  }
  template <typename F, size_t M>
  farray &
  operator=(T (&o)[M])
    requires std::is_array_v<F> && (M <= N)
  {
    micron::copy<N>(&o.stack[0], &o[0]);
    return *this;
  }
  template <typename F>
  farray &
  operator=(const F &o)
    requires std::is_fundamental_v<F>
  {
    micron::cmemset<N>(&stack[0], o);
    return *this;
  }
  farray &
  operator=(const farray &o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    return *this;
  }
  farray &
  operator=(farray &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::cmemset<N>(&stack[0], 0x0);
  }

  template <size_t M>
    requires(M <= N)
  farray &
  operator+(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator-(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator*(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator/(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator%(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] %= o.stack[o];
    return *this;
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

  farray &
  operator*=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
  }
  void
  mul(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= n;
  }
  void
  div(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= n;
  }
  void
  sub(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= n;
  }
  void
  add(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += n;
  }
  size_t
  mul(void) const
  {
    size_t mul_ = stack[0];
    for ( size_t i = 1; i < N; i++ )
      mul_ *= stack[i];
    return mul_;
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
  void
  sqrt(void) const
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] = micron::sqrt(static_cast<float>(stack[i]));
  }
  ~farray() {}
};

// general purpose array class, stack allocated, notthreadsafe, mutable.
// default to 64
template <class T, size_t N = 64>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
           && (N > 0)     // avoid weird stuff with N = 0
class array
{
  T stack[N];
  inline void
  __impl_zero(T *src)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(T());
    } else {
      micron::cmemset<N>(src, 0x0);
    }
  }
  void
  __impl_set(T *__restrict src, const T &val)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = val;
    } else {
      micron::cmemset<N>(src, val);
    }
  }
  void
  __impl_copy(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = dest[i];
    } else {
      micron::copy<N>(src, dest);
    }
  }
  void
  __impl_move(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(dest[i]);
    } else {
      micron::copy<N>(src, dest);
      micron::czero<N>(src);
    }
  }

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
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

  array() { __impl_zero(&stack[0]); }
  array(const T &o) { __impl_set(&stack[0], o); }
  array(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw std::runtime_error("micron::array init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }
  array(const array &o) { __impl_copy(&o.stack[0], &stack[0]); }     // micron::copy<N>(&o.stack[0], &stack[0]); }
  array(array &&o)
  {
    __impl_move(&o.stack[0], &stack[0]);
    // micron::copy<N>(&o.stack[0], &stack[0]);
    // micron::cmemset<N>(&stack[0], 0x0);
  }
  size_t
  size() const
  {
    return N;
  }
  T &
  at(const size_t i)
  {
    if ( i >= N )
      throw std::runtime_error("micron::array at() out of range.");
    return stack[i];
  }
  const T &
  at(const size_t i) const
  {
    if ( i >= N )
      throw std::runtime_error("micron::array at() out of range.");
    return stack[i];
  }
  inline T &
  operator[](const size_t i)
  {
    return stack[i];
  }
  inline const T &
  operator[](const size_t i) const
  {
    return stack[i];
  }
  template <typename F, size_t M>
  array &
  operator=(T (&o)[M])
    requires std::is_array_v<F> && (M <= N)
  {
    __impl_copy(&o.stack[0], &o[0]);
    // micron::copy<N>(&o.stack[0], &o[0]);
    return *this;
  }
  template <typename F>
  array &
  operator=(const F &o)
    requires std::is_fundamental_v<F>
  {
    micron::cmemset<N>(&stack[0], o);
    return *this;
  }
  array &
  operator=(const array &o)
  {
    __impl_copy(&o.stack[0], &stack[0]);
    return *this;
  }
  array &
  operator=(array &&o)
  {
    __impl_move(&o.stack[0], &stack[0]);
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  array &
  operator+(const array<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  array &
  operator-(const array<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  array &
  operator*(const array<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  array &
  operator/(const array<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  array &
  operator%(const array<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] %= o.stack[o];
    return *this;
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

  array &
  operator*=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
  }
  array &
  operator/=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o;
    return *this;
  }
  array &
  operator-=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o;
    return *this;
  }
  array &
  operator+=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o;
    return *this;
  }
  void
  mul(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= n;
  }
  void
  div(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= n;
  }
  void
  sub(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= n;
  }
  void
  add(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += n;
  }
  size_t
  mul(void) const
  {
    size_t mul_ = stack[0];
    for ( size_t i = 1; i < N; i++ )
      mul_ *= stack[i];
    return mul_;
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
  void
  sqrt(void) const
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] = micron::sqrt(static_cast<float>(stack[i]));
  }
  ~array()
  {
    // explicit
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i].~T();
    } else {
      micron::czero<N>(&stack[0]);
    }
  }
};
// carray = cache-array. mutable & notthread safe.
template <class T, size_t N = 64>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> && (N > 0) && ((N * sizeof(T)) % 16 == 0)
           && (N % 4 == 0)     // alignment just.
class alignas(32) carray
{
  T stack[N];
  inline void
  __impl_zero(T *src)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(T());
    } else {
      micron::cmemset<N>(src, 0x0);
    }
  }
  void
  __impl_set(T *__restrict src, const T &val)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = val;
    } else {
      micron::cmemset<N>(src, val);
    }
  }
  void
  __impl_copy(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = dest[i];
    } else {
      micron::copy<N>(src, dest);
    }
  }
  void
  __impl_move(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(dest[i]);
    } else {
      micron::copy<N>(src, dest);
      micron::czero<N>(src);
    }
  }

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
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

  carray() { __impl_zero(&stack[0]); }
  carray(const T &o)
  {
    __impl_set(&stack[0], o);
    // if constexpr (std::is_class<T>::value) {
    //   for (size_t i = 0; i < N; i++)
    //     stack[i] = o;
    // } else {
    //   if constexpr (N * (sizeof(T) / sizeof(byte)) % (256 / 8) == 0)
    //     micron::memset128(&stack[0], o, N);
    //   else if constexpr (N * (sizeof(T) / sizeof(byte)) % (128 / 8) ==
    //                      0) // verbose for clarity, is it 128-bit aligned
    //     micron::memset128(&stack[0], o, N);
    //   else if constexpr (true) {
    //     micron::cmemset<N>(&stack[0], o);
    //   }
    // }
  }
  carray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw std::runtime_error("micron::array init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }
  carray(const carray &o) { micron::copy<N>(&o.stack[0], &stack[0]); }
  carray(carray &&o)
  {
    __impl_copy(&o.stack[0], &stack[0]);
    __impl_zero(&o.stack[0]);
    // micron::copy<N>(&o.stack[0], &stack[0]);
    // micron::memset(&stack[0], 0x0, N);
  }

  size_t
  size() const
  {
    return N;
  }
  inline T &
  at(const size_t i)
  {
    if ( i >= N )
      throw std::runtime_error("micron::array at() out of range.");
    return stack[i];
  }
  inline const T &
  at(const size_t i) const
  {
    if ( i >= N )
      throw std::runtime_error("micron::array at() out of range.");
    return stack[i];
  }
  inline T &
  operator[](const size_t i)
  {
    return stack[i];
  }
  inline const T &
  operator[](const size_t i) const
  {
    return stack[i];
  }
  template <typename F, size_t M>
  carray &
  operator=(T (&o)[M])
    requires std::is_array_v<F> && (M <= N)
  {
    __impl_copy(&o[0], &stack[0]);
    return *this;
  }
  template <typename F>
  carray &
  operator=(const F &o)
    requires std::is_fundamental_v<F>
  {
    micron::memset(&stack[0], o, N);
    return *this;
  }
  carray &
  operator=(const carray &o)
  {
    __impl_copy(&o.stack[0], &stack[0]);
    return *this;
  }
  carray &
  operator=(carray &&o)
  {
    __impl_move(&o.stack[0], &stack[0]);
  }

  template <size_t M>
    requires(M <= N)
  carray &
  operator+(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  carray &
  operator-(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o.stack[o];
    return *this;
  }
  carray &
  operator*=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
  }

  carray &
  operator/=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o;
    return *this;
  }
  carray &
  operator-=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o;
    return *this;
  }
  carray &
  operator+=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o;
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  carray &
  operator*(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  carray &
  operator/(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  carray &
  operator%(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] %= o.stack[o];
    return *this;
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
  void
  mul(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= n;
  }
  void
  div(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= n;
  }
  void
  sub(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= n;
  }
  void
  add(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += n;
  }
  size_t
  mul(void) const
  {
    size_t mul_ = stack[0];
    for ( size_t i = 1; i < N; i++ )
      mul_ *= stack[i];
    return mul_;
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
  void
  sqrt(void) const
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] = micron::sqrt(static_cast<float>(stack[i]));
  }
  ~carray()
  {
    // explicit
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i].~T();
    } else {
      micron::czero<N>(&stack[0]);
    }
  }
};
};     // namespace micron
