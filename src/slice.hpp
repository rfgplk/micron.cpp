//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "allocator.hpp"
#include "memory/allocation/resources.hpp"

#include "algorithm/memory.hpp"
#include "memory/addr.hpp"
#include "tags.hpp"
#include "types.hpp"
#include "view.hpp"

// a slice is a view of contiguous memory
// can be instantiated on its own (allocating new memory) or can be used to
// access memory of other containers (view-like)
// in case of container access, no new memory is allocated
// always moves, never copies
namespace micron
{

template <typename T> struct raw_slice {
  T *ptr = nullptr;
  size_t len = 0;

  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using iterator = T *;
  using const_iterator = const T *;

  constexpr raw_slice() = default;

  constexpr raw_slice(T *p, size_t l) : ptr(p), len(l) {}

  constexpr T *
  begin()
  {
    return ptr;
  }

  constexpr T *
  end()
  {
    return ptr + len;
  }

  constexpr const T *
  begin() const
  {
    return ptr;
  }

  constexpr const T *
  end() const
  {
    return ptr + len;
  }

  constexpr const T *
  cbegin() const
  {
    return ptr;
  }

  constexpr const T *
  cend() const
  {
    return ptr + len;
  }

  constexpr size_t
  size() const
  {
    return len;
  }

  constexpr bool
  is_empty() const
  {
    return len == 0;
  }

  constexpr bool
  valid() const
  {
    return ptr != nullptr;
  }

  constexpr T &
  operator[](size_t i)
  {
    return ptr[i];
  }

  constexpr const T &
  operator[](size_t i) const
  {
    return ptr[i];
  }

  constexpr T *
  get(size_t i)
  {
    return i < len ? ptr + i : nullptr;
  }

  constexpr const T *
  get(size_t i) const
  {
    return i < len ? ptr + i : nullptr;
  }
};

template <typename T> struct dual_raw_slice {
  raw_slice<T> first;
  raw_slice<T> second;

  constexpr bool
  valid() const
  {
    return first.valid() || second.valid();
  }
};

template <typename T> struct split_first_result {
  T *elem;
  raw_slice<T> rest;

  constexpr bool
  valid() const
  {
    return elem != nullptr;
  }
};

template <typename T> struct split_last_result {
  raw_slice<T> init;
  T *elem;

  constexpr bool
  valid() const
  {
    return elem != nullptr;
  }
};

template <typename T, size_t N> struct split_chunk_result {
  T *chunk;
  raw_slice<T> rest;

  constexpr bool
  valid() const
  {
    return chunk != nullptr;
  }
};

template <typename T, typename U> struct align_result {
  raw_slice<T> prefix;
  raw_slice<U> middle;
  raw_slice<T> suffix;
};

template <typename T> struct disjoint_pair {
  T *a = nullptr;
  T *b = nullptr;

  constexpr bool
  valid() const
  {
    return a != nullptr && b != nullptr;
  }
};

template <typename T> struct ptr_range {
  T *begin = nullptr;
  T *end = nullptr;
};

template <is_movable_object T, class Alloc = micron::allocator_serial<>> struct slice : public __immutable_memory_resource<T, Alloc> {
  using __mem = __immutable_memory_resource<T, Alloc>;
  using category_type = slice_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using safety_type = unsafe_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~slice()
  {
    if ( __mem::memory == nullptr )
      return;
    if ( __mem::length == 0 )
      return;     // viewing memory, do not free
    __mem::free();
  }

  slice(void) : __mem(Alloc::auto_size()) { __mem::length = __mem::capacity; }

  slice(T *a, T *b) : __mem(static_cast<size_t>(b - a) / sizeof(T))
  {
    micron::memcpy(micron::addr(__mem::memory[0]), a, b - a);
    __mem::length = b - a;
  }

  explicit slice(const size_t n) : __mem(n) { __mem::length = __mem::capacity; }

  slice(const size_t n, const T &r) : __mem(n)
  {
    __mem::length = n;
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = r;
  }

  template <typename Fn, typename R>
    requires(micron::is_invocable_v<Fn, const T &> or micron::is_invocable_v<Fn, T &> or micron::is_invocable_v<Fn, T &&>)
  slice(Fn &&fn, const R &r) : __mem(r.size())
  {
    __mem::length = r.size();
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = fn(r[i]);
  }

  slice(const slice &) = delete;

  slice(slice &&o) : __mem(micron::move(o)) {}

  slice &operator=(const slice &) = delete;

  slice &
  operator=(slice &&o)
  {
    if ( __mem::memory )
      __mem::free();
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }

  slice &
  set(const T n)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = n;
    return *this;
  }

  slice &
  operator=(const byte n)
  {
    micron::memset(micron::addr(__mem::memory[0]), n, __mem::length);
    return *this;
  }

  chunk<byte>
  operator*()
  {
    return __mem::data();
  }

  byte *
  operator&() volatile
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  T &
  operator[](const R n)
  {
    return __mem::memory[n];
  }

  const T &
  operator[](const size_t n) const
  {
    return __mem::memory[n];
  }

  slice
  operator[](const size_t n, const size_t m) const
  {
    return slice<T, Alloc>(micron::addr(__mem::memory[n]), micron::addr(__mem::memory[m]));
  }

  slice
  operator[](void) const
  {
    return slice<T, Alloc>(micron::addr(__mem::memory[0]), micron::addr(__mem::memory[__mem::length]));
  }

  iterator
  data()
  {
    return micron::addr(__mem::memory[0]);
  }

  const_iterator
  data() const
  {
    return micron::addr(__mem::memory[0]);
  }

  iterator
  addr()
  {
    return micron::addr(__mem::memory[0]);
  }

  const_iterator
  addr() const
  {
    return micron::addr(__mem::memory[0]);
  }

  pointer
  as_ptr()
  {
    return __mem::memory;
  }

  const_pointer
  as_ptr() const
  {
    return __mem::memory;
  }

  ptr_range<T>
  as_ptr_range()
  {
    return { __mem::memory, __mem::memory + __mem::length };
  }

  ptr_range<const T>
  as_ptr_range() const
  {
    return { __mem::memory, __mem::memory + __mem::length };
  }

  ptr_range<T>
  as_mut_ptr_range()
  {
    return { __mem::memory, __mem::memory + __mem::length };
  }

  iterator
  begin()
  {
    return micron::addr(__mem::memory[0]);
  }

  iterator
  end()
  {
    return micron::addr(__mem::memory[__mem::length - 1]);
  }

  const_iterator
  cbegin() const
  {
    return micron::addr(__mem::memory[0]);
  }

  const_iterator
  cend() const
  {
    return micron::addr(__mem::memory[__mem::length - 1]);
  }

  raw_slice<T>
  iter()
  {
    return { __mem::memory, __mem::length };
  }

  raw_slice<const T>
  iter() const
  {
    return { __mem::memory, __mem::length };
  }

  raw_slice<T>
  iter_mut()
  {
    return { __mem::memory, __mem::length };
  }

  size_t
  size() const
  {
    return __mem::length;
  }

  size_t
  max_size() const
  {
    return __mem::capacity;
  }

  size_t
  len() const
  {
    return __mem::length;
  }

  bool
  is_empty() const
  {
    return __mem::length == 0;
  }

  void
  mark(size_t new_len)
  {
    if ( new_len > __mem::capacity ) [[unlikely]]
      return;
    __mem::length = new_len;
  }

  slice
  to_vec() const
  {
    return slice(__mem::memory, __mem::memory + __mem::length);
  }

  void
  reset()
  {
    // zero out the whole thing
    micron::zero(__mem::memory, __mem::capacity);
    __mem::length = __mem::capacity;
  }

  pointer
  first()
  {
    if ( __mem::capacity == 0 ) [[unlikely]]
      return nullptr;
    return __mem::memory;
  }

  const_pointer
  first() const
  {
    if ( __mem::capacity == 0 ) [[unlikely]]
      return nullptr;
    return __mem::memory;
  }

  pointer
  first_mut()
  {
    return first();
  }

  pointer
  last()
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return nullptr;
    return __mem::memory + __mem::length - 1;
  }

  const_pointer
  last() const
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return nullptr;
    return __mem::memory + __mem::length - 1;
  }

  pointer
  last_mut()
  {
    return last();
  }

  pointer
  get(size_t i)
  {
    if ( i >= __mem::length ) [[unlikely]]
      return nullptr;
    return __mem::memory + i;
  }

  const_pointer
  get(size_t i) const
  {
    if ( i >= __mem::length ) [[unlikely]]
      return nullptr;
    return __mem::memory + i;
  }

  pointer
  get_mut(size_t i)
  {
    return get(i);
  }

  pointer
  get_unchecked(size_t i)
  {
    return __mem::memory + i;
  }

  const_pointer
  get_unchecked(size_t i) const
  {
    return __mem::memory + i;
  }

  pointer
  get_unchecked_mut(size_t i)
  {
    return __mem::memory + i;
  }

  disjoint_pair<T>
  get_disjoint_mut(size_t i, size_t j)
  {
    if ( i == j || i >= __mem::length || j >= __mem::length ) [[unlikely]]
      return { nullptr, nullptr };
    return { __mem::memory + i, __mem::memory + j };
  }

  disjoint_pair<T>
  get_disjoint_unchecked_mut(size_t i, size_t j)
  {
    return { __mem::memory + i, __mem::memory + j };
  }

  size_t
  element_offset(const_pointer p) const
  {
    if ( p < __mem::memory || p >= __mem::memory + __mem::length ) [[unlikely]]
      return static_cast<size_t>(-1);
    return static_cast<size_t>(p - __mem::memory);
  }

  struct index_range {
    size_t start;
    size_t end;
  };

  template <size_t N>
  pointer
  first_chunk()
  {
    if ( __mem::length < N ) [[unlikely]]
      return nullptr;
    return __mem::memory;
  }

  template <size_t N>
  const_pointer
  first_chunk() const
  {
    if ( __mem::length < N ) [[unlikely]]
      return nullptr;
    return __mem::memory;
  }

  template <size_t N>
  pointer
  first_chunk_mut()
  {
    return first_chunk<N>();
  }

  template <size_t N>
  pointer
  last_chunk()
  {
    if ( __mem::length < N ) [[unlikely]]
      return nullptr;
    return __mem::memory + __mem::length - N;
  }

  template <size_t N>
  const_pointer
  last_chunk() const
  {
    if ( __mem::length < N ) [[unlikely]]
      return nullptr;
    return __mem::memory + __mem::length - N;
  }

  template <size_t N>
  pointer
  last_chunk_mut()
  {
    return last_chunk<N>();
  }

  template <size_t N>
  split_chunk_result<T, N>
  split_first_chunk() const
  {
    if ( __mem::length < N ) [[unlikely]]
      return { nullptr, raw_slice<T>() };
    return { __mem::memory, raw_slice<T>(__mem::memory + N, __mem::length - N) };
  }

  template <size_t N>
  split_chunk_result<T, N>
  split_first_chunk_mut()
  {
    return split_first_chunk<N>();
  }

  template <size_t N>
  split_chunk_result<T, N>
  split_last_chunk() const
  {
    if ( __mem::length < N ) [[unlikely]]
      return { nullptr, raw_slice<T>() };
    return { __mem::memory + __mem::length - N, raw_slice<T>(__mem::memory, __mem::length - N) };
  }

  template <size_t N>
  split_chunk_result<T, N>
  split_last_chunk_mut()
  {
    return split_last_chunk<N>();
  }

  dual_raw_slice<T>
  split_at(size_t mid) const
  {
    if ( mid > __mem::length ) [[unlikely]]
      mid = __mem::length;
    return { raw_slice<T>(__mem::memory, mid), raw_slice<T>(__mem::memory + mid, __mem::length - mid) };
  }

  dual_raw_slice<T>
  split_at_mut(size_t mid)
  {
    return split_at(mid);
  }

  dual_raw_slice<T>
  split_at_checked(size_t mid) const
  {
    if ( mid > __mem::length ) [[unlikely]]
      return { raw_slice<T>(), raw_slice<T>() };
    return split_at(mid);
  }

  dual_raw_slice<T>
  split_at_mut_checked(size_t mid)
  {
    return split_at_checked(mid);
  }

  dual_raw_slice<T>
  split_at_unchecked(size_t mid) const
  {
    return { raw_slice<T>(__mem::memory, mid), raw_slice<T>(__mem::memory + mid, __mem::length - mid) };
  }

  dual_raw_slice<T>
  split_at_mut_unchecked(size_t mid)
  {
    return split_at_unchecked(mid);
  }

  split_first_result<T>
  split_first() const
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return { nullptr, raw_slice<T>() };
    return { __mem::memory, raw_slice<T>(__mem::memory + 1, __mem::length - 1) };
  }

  split_first_result<T>
  split_first_mut()
  {
    return split_first();
  }

  split_last_result<T>
  split_last() const
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return { raw_slice<T>(), nullptr };
    return { raw_slice<T>(__mem::memory, __mem::length - 1), __mem::memory + __mem::length - 1 };
  }

  split_last_result<T>
  split_last_mut()
  {
    return split_last();
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  split(Fn pred, Cb callback) const
  {
    size_t count = 0, start = 0;
    for ( size_t i = 0; i < __mem::length; i++ ) {
      if ( pred(__mem::memory[i]) ) {
        callback(raw_slice<T>(__mem::memory + start, i - start));
        start = i + 1;
        ++count;
      }
    }
    callback(raw_slice<T>(__mem::memory + start, __mem::length - start));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  split_mut(Fn pred, Cb callback)
  {
    return split(pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  split_inclusive(Fn pred, Cb callback) const
  {
    size_t count = 0, start = 0;
    for ( size_t i = 0; i < __mem::length; i++ ) {
      if ( pred(__mem::memory[i]) ) {
        callback(raw_slice<T>(__mem::memory + start, i - start + 1));
        start = i + 1;
        ++count;
      }
    }
    if ( start < __mem::length )
      callback(raw_slice<T>(__mem::memory + start, __mem::length - start));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  split_inclusive_mut(Fn pred, Cb callback)
  {
    return split_inclusive(pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  splitn(size_t n, Fn pred, Cb callback) const
  {
    if ( n == 0 )
      return 0;
    size_t count = 0, start = 0;
    for ( size_t i = 0; i < __mem::length && count + 1 < n; i++ ) {
      if ( pred(__mem::memory[i]) ) {
        callback(raw_slice<T>(__mem::memory + start, i - start));
        start = i + 1;
        ++count;
      }
    }
    callback(raw_slice<T>(__mem::memory + start, __mem::length - start));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  splitn_mut(size_t n, Fn pred, Cb callback)
  {
    return splitn(n, pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  rsplit(Fn pred, Cb callback) const
  {
    size_t count = 0, end_idx = __mem::length;
    for ( size_t i = __mem::length; i-- > 0; ) {
      if ( pred(__mem::memory[i]) ) {
        callback(raw_slice<T>(__mem::memory + i + 1, end_idx - i - 1));
        end_idx = i;
        ++count;
      }
    }
    callback(raw_slice<T>(__mem::memory, end_idx));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  rsplit_mut(Fn pred, Cb callback)
  {
    return rsplit(pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  rsplitn(size_t n, Fn pred, Cb callback) const
  {
    if ( n == 0 )
      return 0;
    size_t count = 0, end_idx = __mem::length;
    for ( size_t i = __mem::length; i-- > 0 && count + 1 < n; ) {
      if ( pred(__mem::memory[i]) ) {
        callback(raw_slice<T>(__mem::memory + i + 1, end_idx - i - 1));
        end_idx = i;
        ++count;
      }
    }
    callback(raw_slice<T>(__mem::memory, end_idx));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_t
  rsplitn_mut(size_t n, Fn pred, Cb callback)
  {
    return rsplitn(n, pred, callback);
  }

  dual_raw_slice<T>
  split_once(const T &delim) const
  {
    for ( size_t i = 0; i < __mem::length; i++ ) {
      if ( __mem::memory[i] == delim )
        return { raw_slice<T>(__mem::memory, i), raw_slice<T>(__mem::memory + i + 1, __mem::length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, const T &>
  dual_raw_slice<T>
  split_once(Fn pred) const
  {
    for ( size_t i = 0; i < __mem::length; i++ ) {
      if ( pred(__mem::memory[i]) )
        return { raw_slice<T>(__mem::memory, i), raw_slice<T>(__mem::memory + i + 1, __mem::length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  dual_raw_slice<T>
  rsplit_once(const T &delim) const
  {
    for ( size_t i = __mem::length; i-- > 0; ) {
      if ( __mem::memory[i] == delim )
        return { raw_slice<T>(__mem::memory, i), raw_slice<T>(__mem::memory + i + 1, __mem::length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, const T &>
  dual_raw_slice<T>
  rsplit_once(Fn pred) const
  {
    for ( size_t i = __mem::length; i-- > 0; ) {
      if ( pred(__mem::memory[i]) )
        return { raw_slice<T>(__mem::memory, i), raw_slice<T>(__mem::memory + i + 1, __mem::length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  slice
  split_off(size_t mid)
  {
    if ( mid >= __mem::length ) [[unlikely]]
      return slice(size_t(0));
    slice tail(__mem::memory + mid, __mem::memory + __mem::length);
    __mem::length = mid;
    return tail;
  }

  T
  split_off_first()
  {
    T val = micron::move(__mem::memory[0]);
    for ( size_t i = 0; i + 1 < __mem::length; i++ )
      __mem::memory[i] = micron::move(__mem::memory[i + 1]);
    if ( __mem::length > 0 )
      __mem::length--;
    return val;
  }

  raw_slice<T>
  split_off_first_mut()
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return {};
    raw_slice<T> view{ __mem::memory, 1 };
    for ( size_t i = 0; i + 1 < __mem::length; i++ )
      __mem::memory[i] = micron::move(__mem::memory[i + 1]);
    __mem::length--;
    return view;
  }

  T
  split_off_last()
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return T{};
    T val = micron::move(__mem::memory[__mem::length - 1]);
    __mem::length--;
    return val;
  }

  raw_slice<T>
  split_off_last_mut()
  {
    if ( __mem::length == 0 ) [[unlikely]]
      return {};
    __mem::length--;
    return { __mem::memory + __mem::length, 1 };
  }

  raw_slice<T>
  split_off_mut(size_t mid)
  {
    if ( mid > __mem::length ) [[unlikely]]
      return {};
    raw_slice<T> view{ __mem::memory + mid, __mem::length - mid };
    __mem::length = mid;
    return view;
  }

  bool
  contains(const T &value) const
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( __mem::memory[i] == value )
        return true;
    return false;
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, const T &>
  bool
  contains(Fn pred) const
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( pred(__mem::memory[i]) )
        return true;
    return false;
  }

  bool
  starts_with(const raw_slice<T> &needle) const
  {
    if ( needle.len > __mem::length )
      return false;
    for ( size_t i = 0; i < needle.len; i++ )
      if ( __mem::memory[i] != needle.ptr[i] )
        return false;
    return true;
  }

  bool
  starts_with(const_pointer needle, size_t n) const
  {
    return starts_with(raw_slice<T>(const_cast<pointer>(needle), n));
  }

  bool
  ends_with(const raw_slice<T> &needle) const
  {
    if ( needle.len > __mem::length )
      return false;
    size_t offset = __mem::length - needle.len;
    for ( size_t i = 0; i < needle.len; i++ )
      if ( __mem::memory[offset + i] != needle.ptr[i] )
        return false;
    return true;
  }

  bool
  ends_with(const_pointer needle, size_t n) const
  {
    return ends_with(raw_slice<T>(const_cast<pointer>(needle), n));
  }

  raw_slice<T>
  strip_prefix(const raw_slice<T> &prefix) const
  {
    if ( !starts_with(prefix) )
      return {};
    return { __mem::memory + prefix.len, __mem::length - prefix.len };
  }

  raw_slice<T>
  strip_prefix(const_pointer prefix, size_t n) const
  {
    return strip_prefix(raw_slice<T>(const_cast<pointer>(prefix), n));
  }

  raw_slice<T>
  strip_suffix(const raw_slice<T> &suffix) const
  {
    if ( !ends_with(suffix) )
      return {};
    return { __mem::memory, __mem::length - suffix.len };
  }

  raw_slice<T>
  strip_suffix(const_pointer suffix, size_t n) const
  {
    return strip_suffix(raw_slice<T>(const_cast<pointer>(suffix), n));
  }

  raw_slice<T>
  strip_circumfix(const raw_slice<T> &prefix, const raw_slice<T> &suffix) const
  {
    if ( prefix.len + suffix.len > __mem::length )
      return {};
    if ( !starts_with(prefix) || !ends_with(suffix) )
      return {};
    return { __mem::memory + prefix.len, __mem::length - prefix.len - suffix.len };
  }

  raw_slice<T>
  trim_prefix(const raw_slice<T> &prefix) const
  {
    return strip_prefix(prefix);
  }

  raw_slice<T>
  trim_suffix(const raw_slice<T> &suffix) const
  {
    return strip_suffix(suffix);
  }

  slice &
  fill(const T &value)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = value;
    return *this;
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn>
  slice &
  fill_with(Fn gen)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = gen();
    return *this;
  }

  slice &
  copy_from_slice(const raw_slice<T> &src)
  {
    const size_t n = src.len < __mem::length ? src.len : __mem::length;
    micron::memcpy(micron::addr(__mem::memory[0]), src.ptr, n * sizeof(T));
    return *this;
  }

  slice &
  copy_from_slice(const_pointer src, size_t n)
  {
    return copy_from_slice(raw_slice<T>(const_cast<pointer>(src), n));
  }

  slice &
  copy_within(size_t src_start, size_t count, size_t dst)
  {
    if ( src_start + count > __mem::length || dst + count > __mem::length ) [[unlikely]]
      return *this;
    micron::memmove(micron::addr(__mem::memory[dst]), micron::addr(__mem::memory[src_start]), count * sizeof(T));
    return *this;
  }

  slice &
  swap(size_t i, size_t j)
  {
    if ( i >= __mem::length || j >= __mem::length ) [[unlikely]]
      return *this;
    T tmp = micron::move(__mem::memory[i]);
    __mem::memory[i] = micron::move(__mem::memory[j]);
    __mem::memory[j] = micron::move(tmp);
    return *this;
  }

  slice &
  swap_unchecked(size_t i, size_t j)
  {
    T tmp = micron::move(__mem::memory[i]);
    __mem::memory[i] = micron::move(__mem::memory[j]);
    __mem::memory[j] = micron::move(tmp);
    return *this;
  }

  slice &
  swap_with_slice(slice &other)
  {
    const size_t n = __mem::length < other.__mem::length ? __mem::length : other.__mem::length;
    for ( size_t i = 0; i < n; i++ ) {
      T tmp = micron::move(__mem::memory[i]);
      __mem::memory[i] = micron::move(other.__mem::memory[i]);
      other.__mem::memory[i] = micron::move(tmp);
    }
    return *this;
  }

  slice &
  reverse()
  {
    if ( __mem::length < 2 )
      return *this;
    pointer lo = __mem::memory;
    pointer hi = __mem::memory + __mem::length - 1;
    while ( lo < hi ) {
      T tmp = micron::move(*lo);
      *lo = micron::move(*hi);
      *hi = micron::move(tmp);
      ++lo;
      --hi;
    }
    return *this;
  }

  slice &
  rotate_left(size_t n)
  {
    if ( __mem::length == 0 )
      return *this;
    n %= __mem::length;
    if ( n == 0 )
      return *this;
    auto rev = [](pointer a, pointer b) {
      while ( a < b ) {
        T tmp = micron::move(*a);
        *a = micron::move(*b);
        *b = micron::move(tmp);
        ++a;
        --b;
      }
    };
    rev(__mem::memory, __mem::memory + n - 1);
    rev(__mem::memory + n, __mem::memory + __mem::length - 1);
    rev(__mem::memory, __mem::memory + __mem::length - 1);
    return *this;
  }

  slice &
  rotate_right(size_t n)
  {
    if ( __mem::length == 0 )
      return *this;
    n %= __mem::length;
    if ( n == 0 )
      return *this;
    rotate_left(__mem::length - n);
    return *this;
  }

  slice
  repeat(size_t n) const
  {
    if ( n == 0 )
      return slice(size_t(0));
    slice out(__mem::length * n);
    for ( size_t rep = 0; rep < n; rep++ )
      micron::memcpy(micron::addr(out.__mem::memory[rep * __mem::length]), __mem::memory, __mem::length * sizeof(T));
    return out;
  }

  slice
  concat(const raw_slice<T> &other) const
  {
    slice out(__mem::length + other.len);
    micron::memcpy(micron::addr(out.__mem::memory[0]), __mem::memory, __mem::length * sizeof(T));
    micron::memcpy(micron::addr(out.__mem::memory[__mem::length]), other.ptr, other.len * sizeof(T));
    return out;
  }

  slice
  join(const raw_slice<T> &other, const T &sep) const
  {
    size_t total = __mem::length + 1 + other.len;
    slice out(total);
    micron::memcpy(micron::addr(out.__mem::memory[0]), __mem::memory, __mem::length * sizeof(T));
    out.__mem::memory[__mem::length] = sep;
    micron::memcpy(micron::addr(out.__mem::memory[__mem::length + 1]), other.ptr, other.len * sizeof(T));
    return out;
  }

  slice
  connect(const raw_slice<T> &other, const T &sep) const
  {
    return join(other, sep);
  }

  slice &
  write_copy_of_slice(const raw_slice<T> &src)
  {
    return copy_from_slice(src);
  }

  slice &
  write_clone_of_slice(const raw_slice<T> &src)
  {
    const size_t n = src.len < __mem::length ? src.len : __mem::length;
    for ( size_t i = 0; i < n; i++ )
      __mem::memory[i] = src.ptr[i];
    return *this;
  }

  slice &
  write_filled(const T &value)
  {
    return fill(value);
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, size_t>
  slice &
  write_with(Fn gen)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = gen(i);
    return *this;
  }

  template <typename InputIt>
  slice &
  write_iter(InputIt first, InputIt last)
  {
    size_t i = 0;
    for ( ; first != last && i < __mem::length; ++first, ++i )
      __mem::memory[i] = *first;
    return *this;
  }

  raw_slice<const byte>
  as_bytes() const
  {
    return { reinterpret_cast<const byte *>(__mem::memory), __mem::length * sizeof(T) };
  }

  raw_slice<byte>
  as_bytes_mut()
  {
    return { reinterpret_cast<byte *>(__mem::memory), __mem::length * sizeof(T) };
  }

  template <typename U>
  raw_slice<U>
  as_flattened()
  {
    return { reinterpret_cast<U *>(__mem::memory), __mem::length * sizeof(T) / sizeof(U) };
  }

  template <typename U>
  raw_slice<const U>
  as_flattened() const
  {
    return { reinterpret_cast<const U *>(__mem::memory), __mem::length * sizeof(T) / sizeof(U) };
  }

  template <typename U>
  raw_slice<U>
  as_flattened_mut()
  {
    return as_flattened<U>();
  }

  template <typename U>
  align_result<T, U>
  align_to() const
  {
    const byte *raw = reinterpret_cast<const byte *>(__mem::memory);
    const size_t total = __mem::length * sizeof(T);
    const size_t align = alignof(U);

    size_t prefix_bytes = 0;
    auto raw_addr = reinterpret_cast<uintptr_t>(raw);
    if ( raw_addr % align != 0 )
      prefix_bytes = align - (raw_addr % align);
    if ( prefix_bytes > total )
      prefix_bytes = total;

    size_t remaining = total - prefix_bytes;
    size_t middle_count = remaining / sizeof(U);
    size_t middle_bytes = middle_count * sizeof(U);
    size_t suffix_bytes = remaining - middle_bytes;

    size_t prefix_elems = prefix_bytes / sizeof(T);
    size_t suffix_elems = suffix_bytes / sizeof(T);

    return { raw_slice<T>(__mem::memory, prefix_elems),
             raw_slice<U>(reinterpret_cast<U *>(const_cast<byte *>(raw) + prefix_bytes), middle_count),
             raw_slice<T>(__mem::memory + __mem::length - suffix_elems, suffix_elems) };
  }

  template <typename U>
  align_result<T, U>
  align_to_mut()
  {
    return align_to<U>();
  }

  bool
  is_ascii() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      if ( static_cast<unsigned char>(__mem::memory[i]) >= 128 )
        return false;
    return true;
  }

  raw_slice<const byte>
  as_ascii() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    if ( !is_ascii() )
      return {};
    return { reinterpret_cast<const byte *>(__mem::memory), __mem::length };
  }

  raw_slice<const byte>
  as_ascii_unchecked() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    return { reinterpret_cast<const byte *>(__mem::memory), __mem::length };
  }

  slice &
  to_ascii_uppercase()
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    for ( size_t i = 0; i < __mem::length; i++ ) {
      auto c = static_cast<unsigned char>(__mem::memory[i]);
      if ( c >= 'a' && c <= 'z' )
        __mem::memory[i] = static_cast<T>(c - 32);
    }
    return *this;
  }

  slice &
  to_ascii_lowercase()
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    for ( size_t i = 0; i < __mem::length; i++ ) {
      auto c = static_cast<unsigned char>(__mem::memory[i]);
      if ( c >= 'A' && c <= 'Z' )
        __mem::memory[i] = static_cast<T>(c + 32);
    }
    return *this;
  }

  raw_slice<T>
  trim_ascii_start() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    size_t i = 0;
    while ( i < __mem::length ) {
      auto c = static_cast<unsigned char>(__mem::memory[i]);
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' )
        break;
      ++i;
    }
    return { __mem::memory + i, __mem::length - i };
  }

  raw_slice<T>
  trim_ascii_end() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    size_t j = __mem::length;
    while ( j > 0 ) {
      auto c = static_cast<unsigned char>(__mem::memory[j - 1]);
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' )
        break;
      --j;
    }
    return { __mem::memory, j };
  }

  raw_slice<T>
  trim_ascii() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    auto leading = trim_ascii_start();
    size_t j = leading.len;
    while ( j > 0 ) {
      auto c = static_cast<unsigned char>(leading.ptr[j - 1]);
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' )
        break;
      --j;
    }
    return { leading.ptr, j };
  }
};
};     // namespace micron
