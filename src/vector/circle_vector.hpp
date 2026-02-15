//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once
#include "algorithm/algorithm.hpp"
#include "except.hpp"
#include "types.hpp"

namespace micron
{

// renamed from circle_buffer/circular, aliases kept that point to this file
// no longer heap allocated
template <is_regular_object T, size_t N>
  requires((N & (N - 1)) == 0)
class circle_vector
{
  micron::carray<T, N> __buffer;
  size_t __head = 0;
  size_t __tail = 0;
  size_t __size = 0;
  static constexpr size_t __mask = N - 1;

  inline void
  __deep_copy(T *dest, const T *src, size_t cnt)
  {
    for ( size_t i = 0; i < cnt; ++i ) {
      dest[i] = src[i];
    }
  }

  inline void
  __shallow_copy(T *dest, const T *src, size_t cnt)
  {
    micron::bytecpy<byte>(dest, src, cnt * sizeof(T));
  }

  template <typename U>
  inline void
  __impl_copy(U *dest, const U *src, size_t cnt)
  {
    if constexpr ( micron::is_class_v<U> or !micron::is_trivially_copyable_v<U> ) {
      __deep_copy(dest, src, cnt);
    } else {
      __shallow_copy(dest, src, cnt);
    }
  }

public:
  using category_type = list_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template <bool Cnst> class circular_iterator;

  typedef circular_iterator<false> iterator;
  typedef circular_iterator<true> const_iterator;

  template <bool Cnst> class circular_iterator
  {
    friend class circle_vector;
    using buffer_ptr = typename micron::conditional<Cnst, const circle_vector *, circle_vector *>::type;
    using value_ref = typename micron::conditional<Cnst, const T &, T &>::type;
    using value_ptr = typename micron::conditional<Cnst, const T *, T *>::type;

    buffer_ptr __buf;
    size_t __index;

    circular_iterator(buffer_ptr buf, size_t index) noexcept : __buf(buf), __index(index) {}

  public:
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = value_ptr;
    using reference = value_ref;

    circular_iterator() noexcept : __buf(nullptr), __index(0) {}

    template <bool WasConst = Cnst>
      requires(Cnst && !WasConst)
    circular_iterator(const circular_iterator<false> &other) noexcept : __buf(other.__buf), __index(other.__index)
    {
    }

    reference
    operator*() const noexcept
    {
      return __buf->__buffer[(__buf->__tail + __index) & __mask];
    }

    pointer
    operator->() const noexcept
    {
      return &__buf->__buffer[(__buf->__tail + __index) & __mask];
    }

    reference
    operator[](difference_type n) const noexcept
    {
      return __buf->__buffer[(__buf->__tail + __index + n) & __mask];
    }

    circular_iterator &
    operator++() noexcept
    {
      ++__index;
      return *this;
    }

    circular_iterator
    operator++(int) noexcept
    {
      circular_iterator tmp = *this;
      ++__index;
      return tmp;
    }

    circular_iterator &
    operator--() noexcept
    {
      --__index;
      return *this;
    }

    circular_iterator
    operator--(int) noexcept
    {
      circular_iterator tmp = *this;
      --__index;
      return tmp;
    }

    circular_iterator &
    operator+=(difference_type n) noexcept
    {
      __index += n;
      return *this;
    }

    circular_iterator &
    operator-=(difference_type n) noexcept
    {
      __index -= n;
      return *this;
    }

    circular_iterator
    operator+(difference_type n) const noexcept
    {
      circular_iterator tmp = *this;
      tmp.__index += n;
      return tmp;
    }

    circular_iterator
    operator-(difference_type n) const noexcept
    {
      circular_iterator tmp = *this;
      tmp.__index -= n;
      return tmp;
    }

    friend circular_iterator
    operator+(difference_type n, const circular_iterator &it) noexcept
    {
      return it + n;
    }

    difference_type
    operator-(const circular_iterator &other) const noexcept
    {
      return static_cast<difference_type>(__index) - static_cast<difference_type>(other.__index);
    }

    bool
    operator==(const circular_iterator &other) const noexcept
    {
      return __buf == other.__buf && __index == other.__index;
    }

    bool
    operator!=(const circular_iterator &other) const noexcept
    {
      return !(*this == other);
    }

    bool
    operator<(const circular_iterator &other) const noexcept
    {
      return __index < other.__index;
    }

    bool
    operator<=(const circular_iterator &other) const noexcept
    {
      return __index <= other.__index;
    }

    bool
    operator>(const circular_iterator &other) const noexcept
    {
      return __index > other.__index;
    }

    bool
    operator>=(const circular_iterator &other) const noexcept
    {
      return __index >= other.__index;
    }

    template <bool OtherConst>
    bool
    operator==(const circular_iterator<OtherConst> &other) const noexcept
    {
      return __buf == other.__buf && __index == other.__index;
    }

    template <bool OtherConst>
    bool
    operator!=(const circular_iterator<OtherConst> &other) const noexcept
    {
      return !(*this == other);
    }
  };

  ~circle_vector() noexcept = default;
  circle_vector() noexcept = default;

  circle_vector(const circle_vector &other) noexcept : __head(other.__head), __tail(other.__tail), __size(other.__size)
  {
    __impl_copy(__buffer.data(), other.__buffer.data(), N);
  }

  circle_vector(circle_vector &&other) noexcept : __head(other.__head), __tail(other.__tail), __size(other.__size)
  {
    __impl_copy(__buffer.data(), other.__buffer.data(), N);
    other.clear();
  }

  circle_vector &
  operator=(const circle_vector &other) noexcept
  {
    if ( this != &other ) {
      __head = other.__head;
      __tail = other.__tail;
      __size = other.__size;
      __impl_copy(__buffer.data(), other.__buffer.data(), N);
    }
    return *this;
  }

  circle_vector &
  operator=(circle_vector &&other) noexcept
  {
    if ( this != &other ) {
      __head = other.__head;
      __tail = other.__tail;
      __size = other.__size;
      __impl_copy(__buffer.data(), other.__buffer.data(), N);
      other.clear();
    }
    return *this;
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  bool
  full() const noexcept
  {
    return __size == N;
  }

  size_type
  size() const noexcept
  {
    return __size;
  }

  size_type
  max_size() const noexcept
  {
    return N;
  }

  size_type
  capacity() const noexcept
  {
    return N;
  }

  void
  push(const T &val) noexcept
  {
    __buffer[__head] = val;
    __head = (__head + 1) & __mask;
    if ( __size < N ) {
      ++__size;
    } else {
      __tail = (__tail + 1) & __mask;     // overwrite oldest if full
    }
  }

  void
  push_back(const T &val) noexcept
  {
    push(val);
  }

  void
  move_back(T &&val) noexcept
  {
    __buffer[__head] = micron::move(val);
    __head = (__head + 1) & __mask;
    if ( __size < N ) {
      ++__size;
    } else {
      __tail = (__tail + 1) & __mask;
    }
  }

  template <typename... Args>
  void
  emplace_back(Args &&...args) noexcept
  {
    __buffer[__head] = T(micron::forward<Args>(args)...);
    __head = (__head + 1) & __mask;
    if ( __size < N ) {
      ++__size;
    } else {
      __tail = (__tail + 1) & __mask;
    }
  }

  T &&
  pop() noexcept
  {
    T val = micron::move(__buffer[__tail]);
    __tail = (__tail + 1) & __mask;
    if ( __size > 0 )
      --__size;
    return val;
  }

  void
  pop_front() noexcept
  {
    if ( !empty() ) {
      __tail = (__tail + 1) & __mask;
      --__size;
    }
  }

  void
  clear() noexcept
  {
    __head = 0;
    __tail = 0;
    __size = 0;
  }

  reference
  front() noexcept
  {
    return __buffer[__tail];
  }

  const_reference
  front() const noexcept
  {
    return __buffer[__tail];
  }

  reference
  back() noexcept
  {
    return __buffer[(__head - 1) & __mask];
  }

  const_reference
  back() const noexcept
  {
    return __buffer[(__head - 1) & __mask];
  }

  reference
  operator[](size_type idx) noexcept
  {
    return __buffer[(__tail + idx) & __mask];
  }

  const_reference
  operator[](size_type idx) const noexcept
  {
    return __buffer[(__tail + idx) & __mask];
  }

  reference
  at(size_type idx)
  {
    if ( idx >= __size )
      exc<except::library_error>("circle_vector::at");
    return __buffer[(__tail + idx) & __mask];
  }

  const_reference
  at(size_type idx) const
  {
    if ( idx >= __size )
      exc<except::library_error>("circle_vector::at");
    return __buffer[(__tail + idx) & __mask];
  }

  iterator
  begin() noexcept
  {
    return iterator(this, 0);
  }

  iterator
  end() noexcept
  {
    return iterator(this, __size);
  }

  const_iterator
  begin() const noexcept
  {
    return const_iterator(this, 0);
  }

  const_iterator
  end() const noexcept
  {
    return const_iterator(this, __size);
  }

  const_iterator
  cbegin() const noexcept
  {
    return const_iterator(this, 0);
  }

  const_iterator
  cend() const noexcept
  {
    return const_iterator(this, __size);
  }

  pointer
  data() noexcept
  {
    return __buffer.data();
  }

  const_pointer
  data() const noexcept
  {
    return __buffer.data();
  }
};

}     // namespace micron
