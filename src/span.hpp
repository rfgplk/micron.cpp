//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/memory.hpp"
#include "memory/addr.hpp"
#include "tags.hpp"
#include "types.hpp"
#include "view.hpp"

#include "slice.hpp"

// a span is a stack-allocated, fixed-capacity view of contiguous memory
// always owns its memory (on the stack), never heap allocates
// always moves, never copies (where avoidable)
namespace micron
{

template <is_regular_object T, usize N = 64> class span
{
  T stack[N];
  usize length = 0;

public:
  template <is_regular_object U, usize M> friend class span;

  using category_type = slice_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  using safety_type = safe_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef usize size_type;

  ~span()
  {
    __impl_container::destroy(micron::addr(stack[0]), length);
    length = 0;
  }

  span(void) : length(0) {}

  explicit span(const size_type cnt)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): cnt exceeds stack capacity N");
    __impl_container::set(micron::addr(stack[0]), T{}, cnt);
    length = cnt;
  }

  span(const size_type cnt, const T &val)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): cnt exceeds stack capacity N");
    __impl_container::set(micron::addr(stack[0]), val, cnt);
    length = cnt;
  }

  span(T *a, T *b)
  {
    const size_type n = static_cast<size_type>(b - a);
    if ( n > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): range exceeds stack capacity N");
    micron::memcpy(micron::addr(stack[0]), a, n * sizeof(T));
    length = n;
  }

  span(const raw_slice<T> &s)
  {
    if ( s.len > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): raw_slice exceeds stack capacity N");
    micron::memcpy(micron::addr(stack[0]), s.ptr, s.len * sizeof(T));
    length = s.len;
  }

  span(const std::initializer_list<T> &lst)
  {
    if ( lst.size() > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): initializer_list exceeds stack capacity N");
    size_type i = 0;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( const T &val : lst )
        stack[i++] = val;
    } else {
      for ( auto &&val : lst )
        stack[i++] = micron::move(val);
    }
    length = static_cast<size_type>(lst.size());
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn>)
  span(const size_type cnt, Fn &&fn)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): cnt exceeds stack capacity N");
    __impl_container::set(micron::addr(stack[0]), T{}, cnt);
    length = cnt;
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  span(const size_type cnt, Fn &&fn)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::span(): cnt exceeds stack capacity N");
    __impl_container::set(micron::addr(stack[0]), T{}, cnt);
    length = cnt;
    micron::transform(begin(), end(), fn);
  }

  template <usize M> span(const span<T, M> &o)
  {
    constexpr size_type copy_n = (N < M) ? N : M;
    __impl_container::copy(stack, o.stack, copy_n);
    length = (o.length < N) ? o.length : N;
  }

  span(const span &o)
  {
    __impl_container::copy(stack, o.stack, N);
    length = o.length;
  }

  span(span &&o)
  {
    __impl_container::move<N, T>(micron::real_addr_as<int>(stack[0]), micron::real_addr_as<T>(o.stack[0]));
    length = o.length;
    o.length = 0;
  }

  template <usize M> span(span<T, M> &&o)
  {
    constexpr size_type copy_n = (N >= M) ? M : N;
    micron::copy<copy_n>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
    micron::zero<copy_n>(micron::real_addr_as<T>(o.stack[0]));
    length = (o.length < N) ? o.length : N;
    o.length = 0;
  }

  span &operator=(const span &) = default;

  span &
  operator=(span &&o)
  {
    micron::copy<N>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
    micron::zero<N>(micron::real_addr_as<T>(o.stack[0]));
    length = o.length;
    o.length = 0;
    return *this;
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  T &
  operator[](const R n)
  {
    return stack[n];
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  const T &
  operator[](const R n) const
  {
    return stack[n];
  }

  span
  operator[](const size_type from, const size_type to) const
  {
    if ( from >= to || from >= length || to > length ) [[unlikely]]
      exc<except::library_error>("micron::span operator[] out of range");
    return span(const_cast<T *>(stack + from), const_cast<T *>(stack + to));
  }

  span
  operator[](void) const
  {
    return span(const_cast<T *>(stack), const_cast<T *>(stack + length));
  }

  span &
  operator=(const byte n)
  {
    micron::memset(micron::addr(stack[0]), n, length * sizeof(T));
    return *this;
  }

  byte *
  operator&() volatile
  {
    return reinterpret_cast<byte *>(stack);
  }

  const byte *
  operator&() const
  {
    return reinterpret_cast<const byte *>(stack);
  }

  bool
  operator!() const
  {
    return empty();
  }

  pointer
  get(size_type i)
  {
    if ( i >= length ) [[unlikely]]
      return nullptr;
    return stack + i;
  }

  const_pointer
  get(size_type i) const
  {
    if ( i >= length ) [[unlikely]]
      return nullptr;
    return stack + i;
  }

  pointer
  get_mut(size_type i)
  {
    return get(i);
  }

  pointer
  get_unchecked(size_type i)
  {
    return stack + i;
  }

  const_pointer
  get_unchecked(size_type i) const
  {
    return stack + i;
  }

  pointer
  get_unchecked_mut(size_type i)
  {
    return stack + i;
  }

  disjoint_pair<T>
  get_disjoint_mut(size_type i, size_type j)
  {
    if ( i == j || i >= length || j >= length ) [[unlikely]]
      return { nullptr, nullptr };
    return { stack + i, stack + j };
  }

  disjoint_pair<T>
  get_disjoint_unchecked_mut(size_type i, size_type j)
  {
    return { stack + i, stack + j };
  }

  iterator
  begin()
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  iterator
  end()
  {
    return micron::real_addr_as<T>(stack[length]);
  }

  const_iterator
  cbegin() const
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  const_iterator
  cend() const
  {
    return micron::real_addr_as<T>(stack[length]);
  }

  iterator
  front()
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  const_iterator
  front() const
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  iterator
  back()
  {
    return micron::real_addr_as<T>(stack[length - 1]);
  }

  const_iterator
  back() const
  {
    return micron::real_addr_as<T>(stack[length - 1]);
  }

  pointer
  data()
  {
    return &stack[0];
  }

  const_pointer
  data() const
  {
    return &stack[0];
  }

  pointer
  addr()
  {
    return &stack[0];
  }

  const_pointer
  addr() const
  {
    return &stack[0];
  }

  pointer
  as_ptr()
  {
    return stack;
  }

  const_pointer
  as_ptr() const
  {
    return stack;
  }

  ptr_range<T>
  as_ptr_range()
  {
    return { stack, stack + length };
  }

  ptr_range<const T>
  as_ptr_range() const
  {
    return { stack, stack + length };
  }

  ptr_range<T>
  as_mut_ptr_range()
  {
    return { stack, stack + length };
  }

  raw_slice<T>
  iter()
  {
    return { stack, length };
  }

  raw_slice<const T>
  iter() const
  {
    return { stack, length };
  }

  raw_slice<T>
  iter_mut()
  {
    return { stack, length };
  }

  size_type
  size() const
  {
    return length;
  }

  size_type
  max_size() const
  {
    return N;
  }

  size_type
  len() const
  {
    return length;
  }

  bool
  empty() const noexcept
  {
    return length == 0;
  }

  bool
  is_empty() const
  {
    return length == 0;
  }

  bool
  full() const
  {
    return length == N;
  }

  bool
  overflowed() const
  {
    return (length + 1) > N;
  }

  void
  mark(size_type new_len)
  {
    if ( new_len > N ) [[unlikely]]
      return;
    length = new_len;
  }

  void
  __set_size(size_type s)
  {
    length = s;
  }

  span &
  push_back(const T &val)
  {
    if ( length + 1 > N ) [[unlikely]]
      exc<except::runtime_error>("micron::span push_back() exceeds stack capacity N");
    stack[length++] = val;
    return *this;
  }

  span &
  move_back(T &&val)
  {
    if ( length + 1 > N ) [[unlikely]]
      exc<except::runtime_error>("micron::span move_back() exceeds stack capacity N");
    stack[length++] = micron::move(val);
    return *this;
  }

  template <typename... Args>
  span &
  emplace_back(Args &&...args)
  {
    if ( length + 1 > N ) [[unlikely]]
      exc<except::runtime_error>("micron::span emplace_back() exceeds stack capacity N");
    stack[length++] = T(micron::forward<Args>(args)...);
    return *this;
  }

  span &
  pop_back()
  {
    if ( length == 0 ) [[unlikely]]
      return *this;
    stack[--length].~T();
    return *this;
  }

  span &
  insert(size_type ind, const T &val)
  {
    if ( length + 1 > N ) [[unlikely]]
      exc<except::runtime_error>("micron::span insert() exceeds stack capacity N");
    micron::bytemove(&stack[ind + 1], &stack[ind], (length - ind) * sizeof(T));
    stack[ind] = val;
    length++;
    return *this;
  }

  span &
  insert(iterator itr, const T &val)
  {
    if ( length + 1 > N ) [[unlikely]]
      exc<except::runtime_error>("micron::span insert() exceeds stack capacity N");
    micron::bytemove(itr + 1, itr, (end() - itr) * sizeof(T));
    *itr = val;
    length++;
    return *this;
  }

  span &
  erase(size_type n)
  {
    if ( n >= length ) [[unlikely]]
      exc<except::runtime_error>("micron::span erase() out of range");
    stack[n].~T();
    micron::memmove(micron::addr(stack[n]), micron::addr(stack[n + 1]), (length - n - 1) * sizeof(T));
    length--;
    return *this;
  }

  void
  clear()
  {
    __impl_container::destroy(micron::real_addr_as<T>(stack), length);
    length = 0;
  }

  void
  fast_clear()
  {
    if constexpr ( !micron::is_class_v<T> )
      length = 0;
    else
      clear();
  }

  span &
  set(const T &val)
  {
    for ( size_type i = 0; i < length; i++ )
      stack[i] = val;
    return *this;
  }

  span &
  fill(const T &val)
  {
    for ( size_type i = 0; i < length; i++ )
      stack[i] = val;
    return *this;
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn>
  span &
  fill_with(Fn gen)
  {
    for ( size_type i = 0; i < length; i++ )
      stack[i] = gen();
    return *this;
  }

  void
  reset()
  {
    micron::zero(stack, N);
    length = N;
  }

  pointer
  first()
  {
    return length == 0 ? nullptr : stack;
  }

  const_pointer
  first() const
  {
    return length == 0 ? nullptr : stack;
  }

  pointer
  first_mut()
  {
    return first();
  }

  pointer
  last()
  {
    return length == 0 ? nullptr : stack + length - 1;
  }

  const_pointer
  last() const
  {
    return length == 0 ? nullptr : stack + length - 1;
  }

  pointer
  last_mut()
  {
    return last();
  }

  template <size_type Chunk>
  pointer
  first_chunk()
  {
    if ( length < Chunk ) [[unlikely]]
      return nullptr;
    return stack;
  }

  template <size_type Chunk>
  const_pointer
  first_chunk() const
  {
    if ( length < Chunk ) [[unlikely]]
      return nullptr;
    return stack;
  }

  template <size_type Chunk>
  pointer
  first_chunk_mut()
  {
    return first_chunk<Chunk>();
  }

  template <size_type Chunk>
  pointer
  last_chunk()
  {
    if ( length < Chunk ) [[unlikely]]
      return nullptr;
    return stack + length - Chunk;
  }

  template <size_type Chunk>
  const_pointer
  last_chunk() const
  {
    if ( length < Chunk ) [[unlikely]]
      return nullptr;
    return stack + length - Chunk;
  }

  template <size_type Chunk>
  pointer
  last_chunk_mut()
  {
    return last_chunk<Chunk>();
  }
  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  split(Fn pred, Cb callback) const
  {
    size_type count = 0, start = 0;
    for ( size_type i = 0; i < length; i++ ) {
      if ( pred(stack[i]) ) {
        callback(raw_slice<T>(stack + start, i - start));
        start = i + 1;
        ++count;
      }
    }
    callback(raw_slice<T>(stack + start, length - start));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  split_mut(Fn pred, Cb callback)
  {
    return split(pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  split_inclusive(Fn pred, Cb callback) const
  {
    size_type count = 0, start = 0;
    for ( size_type i = 0; i < length; i++ ) {
      if ( pred(stack[i]) ) {
        callback(raw_slice<T>(stack + start, i - start + 1));
        start = i + 1;
        ++count;
      }
    }
    if ( start < length )
      callback(raw_slice<T>(stack + start, length - start));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  split_inclusive_mut(Fn pred, Cb callback)
  {
    return split_inclusive(pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  splitn(size_type n, Fn pred, Cb callback) const
  {
    if ( n == 0 )
      return 0;
    size_type count = 0, start = 0;
    for ( size_type i = 0; i < length && count + 1 < n; i++ ) {
      if ( pred(stack[i]) ) {
        callback(raw_slice<T>(stack + start, i - start));
        start = i + 1;
        ++count;
      }
    }
    callback(raw_slice<T>(stack + start, length - start));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  splitn_mut(size_type n, Fn pred, Cb callback)
  {
    return splitn(n, pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  rsplit(Fn pred, Cb callback) const
  {
    size_type count = 0, end_idx = length;
    for ( size_type i = length; i-- > 0; ) {
      if ( pred(stack[i]) ) {
        callback(raw_slice<T>(stack + i + 1, end_idx - i - 1));
        end_idx = i;
        ++count;
      }
    }
    callback(raw_slice<T>(stack, end_idx));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  rsplit_mut(Fn pred, Cb callback)
  {
    return rsplit(pred, callback);
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  rsplitn(size_type n, Fn pred, Cb callback) const
  {
    if ( n == 0 )
      return 0;
    size_type count = 0, end_idx = length;
    for ( size_type i = length; i-- > 0 && count + 1 < n; ) {
      if ( pred(stack[i]) ) {
        callback(raw_slice<T>(stack + i + 1, end_idx - i - 1));
        end_idx = i;
        ++count;
      }
    }
    callback(raw_slice<T>(stack, end_idx));
    return count + 1;
  }

  template <typename Fn, typename Cb>
    requires micron::is_invocable_v<Fn, const T &>
  size_type
  rsplitn_mut(size_type n, Fn pred, Cb callback)
  {
    return rsplitn(n, pred, callback);
  }

  dual_raw_slice<T>
  split_once(const T &delim) const
  {
    for ( size_type i = 0; i < length; i++ ) {
      if ( stack[i] == delim )
        return { raw_slice<T>(stack, i), raw_slice<T>(stack + i + 1, length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, const T &>
  dual_raw_slice<T>
  split_once(Fn pred) const
  {
    for ( size_type i = 0; i < length; i++ ) {
      if ( pred(stack[i]) )
        return { raw_slice<T>(stack, i), raw_slice<T>(stack + i + 1, length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  dual_raw_slice<T>
  rsplit_once(const T &delim) const
  {
    for ( size_type i = length; i-- > 0; ) {
      if ( stack[i] == delim )
        return { raw_slice<T>(stack, i), raw_slice<T>(stack + i + 1, length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, const T &>
  dual_raw_slice<T>
  rsplit_once(Fn pred) const
  {
    for ( size_type i = length; i-- > 0; ) {
      if ( pred(stack[i]) )
        return { raw_slice<T>(stack, i), raw_slice<T>(stack + i + 1, length - i - 1) };
    }
    return { raw_slice<T>(), raw_slice<T>() };
  }

  raw_slice<T>
  split_off_mut(size_type mid)
  {
    if ( mid > length ) [[unlikely]]
      return {};
    raw_slice<T> view{ stack + mid, length - mid };
    length = mid;
    return view;
  }

  T
  split_off_first()
  {
    if ( length == 0 ) [[unlikely]]
      return T{};
    T val = micron::move(stack[0]);
    micron::memmove(micron::addr(stack[0]), micron::addr(stack[1]), (length - 1) * sizeof(T));
    length--;
    return val;
  }

  raw_slice<T>
  split_off_first_mut()
  {
    if ( length == 0 ) [[unlikely]]
      return {};
    raw_slice<T> view{ stack, 1 };
    micron::memmove(micron::addr(stack[0]), micron::addr(stack[1]), (length - 1) * sizeof(T));
    length--;
    return view;
  }

  T
  split_off_last()
  {
    if ( length == 0 ) [[unlikely]]
      return T{};
    return micron::move(stack[--length]);
  }

  raw_slice<T>
  split_off_last_mut()
  {
    if ( length == 0 ) [[unlikely]]
      return {};
    length--;
    return { stack + length, 1 };
  }

  bool
  contains(const T &value) const
  {
    for ( size_type i = 0; i < length; i++ )
      if ( stack[i] == value )
        return true;
    return false;
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, const T &>
  bool
  contains(Fn pred) const
  {
    for ( size_type i = 0; i < length; i++ )
      if ( pred(stack[i]) )
        return true;
    return false;
  }

  size_type
  element_offset(const_pointer p) const
  {
    if ( p < stack || p >= stack + length ) [[unlikely]]
      return static_cast<size_type>(-1);
    return static_cast<size_type>(p - stack);
  }

  bool
  starts_with(const raw_slice<T> &needle) const
  {
    if ( needle.len > length )
      return false;
    for ( size_type i = 0; i < needle.len; i++ )
      if ( stack[i] != needle.ptr[i] )
        return false;
    return true;
  }

  bool
  starts_with(const_pointer needle, size_type n) const
  {
    return starts_with(raw_slice<T>(const_cast<pointer>(needle), n));
  }

  bool
  ends_with(const raw_slice<T> &needle) const
  {
    if ( needle.len > length )
      return false;
    const size_type offset = length - needle.len;
    for ( size_type i = 0; i < needle.len; i++ )
      if ( stack[offset + i] != needle.ptr[i] )
        return false;
    return true;
  }

  bool
  ends_with(const_pointer needle, size_type n) const
  {
    return ends_with(raw_slice<T>(const_cast<pointer>(needle), n));
  }

  raw_slice<T>
  strip_prefix(const raw_slice<T> &prefix) const
  {
    if ( !starts_with(prefix) )
      return {};
    return { stack + prefix.len, length - prefix.len };
  }

  raw_slice<T>
  strip_prefix(const_pointer prefix, size_type n) const
  {
    return strip_prefix(raw_slice<T>(const_cast<pointer>(prefix), n));
  }

  raw_slice<T>
  strip_suffix(const raw_slice<T> &suffix) const
  {
    if ( !ends_with(suffix) )
      return {};
    return { stack, length - suffix.len };
  }

  raw_slice<T>
  strip_suffix(const_pointer suffix, size_type n) const
  {
    return strip_suffix(raw_slice<T>(const_cast<pointer>(suffix), n));
  }

  raw_slice<T>
  strip_circumfix(const raw_slice<T> &prefix, const raw_slice<T> &suffix) const
  {
    if ( prefix.len + suffix.len > length )
      return {};
    if ( !starts_with(prefix) || !ends_with(suffix) )
      return {};
    return { stack + prefix.len, length - prefix.len - suffix.len };
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

  span &
  copy_from_slice(const raw_slice<T> &src)
  {
    const size_type n = (src.len < length) ? src.len : length;
    micron::memcpy(micron::addr(stack[0]), src.ptr, n * sizeof(T));
    return *this;
  }

  span &
  copy_from_slice(const_pointer src, size_type n)
  {
    return copy_from_slice(raw_slice<T>(const_cast<pointer>(src), n));
  }

  span &
  copy_within(size_type src_start, size_type count, size_type dst)
  {
    if ( src_start + count > length || dst + count > length ) [[unlikely]]
      return *this;
    micron::memmove(micron::addr(stack[dst]), micron::addr(stack[src_start]), count * sizeof(T));
    return *this;
  }

  span &
  write_copy_of_slice(const raw_slice<T> &src)
  {
    return copy_from_slice(src);
  }

  span &
  write_clone_of_slice(const raw_slice<T> &src)
  {
    const size_type n = (src.len < length) ? src.len : length;
    for ( size_type i = 0; i < n; i++ )
      stack[i] = src.ptr[i];
    return *this;
  }

  span &
  write_filled(const T &val)
  {
    return fill(val);
  }

  template <typename Fn>
    requires micron::is_invocable_v<Fn, size_type>
  span &
  write_with(Fn gen)
  {
    for ( size_type i = 0; i < length; i++ )
      stack[i] = gen(i);
    return *this;
  }

  template <typename InputIt>
  span &
  write_iter(InputIt first_it, InputIt last_it)
  {
    size_type i = 0;
    for ( ; first_it != last_it && i < length; ++first_it, ++i )
      stack[i] = *first_it;
    return *this;
  }

  span &
  swap(size_type i, size_type j)
  {
    if ( i >= length || j >= length ) [[unlikely]]
      return *this;
    T tmp = micron::move(stack[i]);
    stack[i] = micron::move(stack[j]);
    stack[j] = micron::move(tmp);
    return *this;
  }

  span &
  swap_unchecked(size_type i, size_type j)
  {
    T tmp = micron::move(stack[i]);
    stack[i] = micron::move(stack[j]);
    stack[j] = micron::move(tmp);
    return *this;
  }

  span &
  swap_with_span(span &other)
  {
    const size_type n = (length < other.length) ? length : other.length;
    for ( size_type i = 0; i < n; i++ ) {
      T tmp = micron::move(stack[i]);
      stack[i] = micron::move(other.stack[i]);
      other.stack[i] = micron::move(tmp);
    }
    return *this;
  }

  span &
  reverse()
  {
    if ( length < 2 )
      return *this;
    pointer lo = stack;
    pointer hi = stack + length - 1;
    while ( lo < hi ) {
      T tmp = micron::move(*lo);
      *lo = micron::move(*hi);
      *hi = micron::move(tmp);
      ++lo;
      --hi;
    }
    return *this;
  }

  span &
  rotate_left(size_type n)
  {
    if ( length == 0 )
      return *this;
    n %= length;
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
    rev(stack, stack + n - 1);
    rev(stack + n, stack + length - 1);
    rev(stack, stack + length - 1);
    return *this;
  }

  span &
  rotate_right(size_type n)
  {
    if ( length == 0 )
      return *this;
    n %= length;
    if ( n == 0 )
      return *this;
    rotate_left(length - n);
    return *this;
  }

  raw_slice<const byte>
  as_bytes() const
  {
    return { reinterpret_cast<const byte *>(stack), length * sizeof(T) };
  }

  raw_slice<byte>
  as_bytes_mut()
  {
    return { reinterpret_cast<byte *>(stack), length * sizeof(T) };
  }

  template <typename U>
  raw_slice<U>
  as_flattened()
  {
    return { reinterpret_cast<U *>(stack), length * sizeof(T) / sizeof(U) };
  }

  template <typename U>
  raw_slice<const U>
  as_flattened() const
  {
    return { reinterpret_cast<const U *>(stack), length * sizeof(T) / sizeof(U) };
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
    const byte *raw = reinterpret_cast<const byte *>(stack);
    const size_type total = length * sizeof(T);
    const size_type align = alignof(U);

    size_type prefix_bytes = 0;
    auto raw_addr = reinterpret_cast<uintptr_t>(raw);
    if ( raw_addr % align != 0 )
      prefix_bytes = align - (raw_addr % align);
    if ( prefix_bytes > total )
      prefix_bytes = total;

    size_type remaining = total - prefix_bytes;
    size_type middle_count = remaining / sizeof(U);
    size_type middle_bytes = middle_count * sizeof(U);
    size_type suffix_bytes = remaining - middle_bytes;
    size_type prefix_elems = prefix_bytes / sizeof(T);
    size_type suffix_elems = suffix_bytes / sizeof(T);

    return { raw_slice<T>(stack, prefix_elems), raw_slice<U>(reinterpret_cast<U *>(const_cast<byte *>(raw) + prefix_bytes), middle_count),
             raw_slice<T>(stack + length - suffix_elems, suffix_elems) };
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
    for ( size_type i = 0; i < length; i++ )
      if ( static_cast<unsigned char>(stack[i]) >= 128 )
        return false;
    return true;
  }

  raw_slice<const byte>
  as_ascii() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    if ( !is_ascii() )
      return {};
    return { reinterpret_cast<const byte *>(stack), length };
  }

  raw_slice<const byte>
  as_ascii_unchecked() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    return { reinterpret_cast<const byte *>(stack), length };
  }

  span &
  to_ascii_uppercase()
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    for ( size_type i = 0; i < length; i++ ) {
      auto c = static_cast<unsigned char>(stack[i]);
      if ( c >= 'a' && c <= 'z' )
        stack[i] = static_cast<T>(c - 32);
    }
    return *this;
  }

  span &
  to_ascii_lowercase()
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    for ( size_type i = 0; i < length; i++ ) {
      auto c = static_cast<unsigned char>(stack[i]);
      if ( c >= 'A' && c <= 'Z' )
        stack[i] = static_cast<T>(c + 32);
    }
    return *this;
  }

  raw_slice<T>
  trim_ascii_start() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    size_type i = 0;
    while ( i < length ) {
      auto c = static_cast<unsigned char>(stack[i]);
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' )
        break;
      ++i;
    }
    return { stack + i, length - i };
  }

  raw_slice<T>
  trim_ascii_end() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    size_type j = length;
    while ( j > 0 ) {
      auto c = static_cast<unsigned char>(stack[j - 1]);
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' )
        break;
      --j;
    }
    return { stack, j };
  }

  raw_slice<T>
  trim_ascii() const
    requires(micron::is_same_v<T, byte> || micron::is_same_v<T, char>)
  {
    const auto leading = trim_ascii_start();
    size_type j = leading.len;
    while ( j > 0 ) {
      auto c = static_cast<unsigned char>(leading.ptr[j - 1]);
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f' )
        break;
      --j;
    }
    return { leading.ptr, j };
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class_type()
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial()
  {
    return micron::is_trivial_v<T>;
  }
};

}     // namespace micron
