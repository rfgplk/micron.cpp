//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../types.hpp"

#include "../allocator.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/new.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"

namespace micron
{

// standard struct-of-arrays container
// given a tuple of column types Ts..., allocate one heap array per column

template<typename... Ts>
  requires(sizeof...(Ts) > 0)
class soa
{
  static constexpr usize __ncols = sizeof...(Ts);

  void *__cols[__ncols] = { nullptr };
  usize __size = 0;
  usize __capacity = 0;

  template<usize I> using __col_t = typename micron::tuple_element_t<I, micron::tuple<Ts...>>;

  template<usize I>
  __col_t<I> *
  __as(void *p) noexcept
  {
    return static_cast<__col_t<I> *>(p);
  }

  template<usize I>
  const __col_t<I> *
  __as(const void *p) const noexcept
  {
    return static_cast<const __col_t<I> *>(p);
  }

  template<usize I>
  void
  __alloc_column(usize cap)
  {
    using T = __col_t<I>;
    __cols[I] = ::operator new(sizeof(T) * cap);
  }

  template<usize I>
  void
  __free_column() noexcept
  {
    using T = __col_t<I>;
    if ( __cols[I] ) {
      if constexpr ( !micron::is_trivially_destructible_v<T> ) {
        T *p = __as<I>(__cols[I]);
        for ( usize i = 0; i < __size; ++i ) p[i].~T();
      }
      ::operator delete(__cols[I]);
      __cols[I] = nullptr;
    }
  }

  template<usize I>
  void
  __move_column(void *src, void *dst, usize n)
  {
    using T = __col_t<I>;
    T *s = static_cast<T *>(src);
    T *d = static_cast<T *>(dst);
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::bytecpy(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), n * sizeof(T));
    } else {
      for ( usize i = 0; i < n; ++i ) {
        new (d + i) T(micron::move(s[i]));
        s[i].~T();
      }
    }
  }

  template<usize... Is>
  void
  __alloc_all(usize cap, micron::index_sequence<Is...>)
  {
    (__alloc_column<Is>(cap), ...);
  }

  template<usize... Is>
  void
  __free_all(micron::index_sequence<Is...>) noexcept
  {
    (__free_column<Is>(), ...);
  }

  template<usize... Is>
  void
  __move_all(void **src_cols, void **dst_cols, usize n, micron::index_sequence<Is...>)
  {
    (__move_column<Is>(src_cols[Is], dst_cols[Is], n), ...);
  }

  void
  __grow_to(usize new_cap)
  {
    if ( new_cap <= __capacity ) return;
    void *new_cols[__ncols] = { nullptr };
    __grow_to_impl(new_cap, new_cols, micron::make_index_sequence<__ncols>{});
    __move_all(__cols, new_cols, __size, micron::make_index_sequence<__ncols>{});
    for ( usize i = 0; i < __ncols; ++i ) {
      if ( __cols[i] ) ::operator delete(__cols[i]);
      __cols[i] = new_cols[i];
    }
    __capacity = new_cap;
  }

  template<usize... Is>
  void
  __grow_to_impl(usize new_cap, void **new_cols, micron::index_sequence<Is...>)
  {
    (((new_cols[Is] = ::operator new(sizeof(__col_t<Is>) * new_cap))), ...);
  }

  template<usize... Is>
  void
  __emplace_at(usize idx, micron::index_sequence<Is...>, Ts &&...vs)
  {
    (new (__as<Is>(__cols[Is]) + idx) __col_t<Is>(micron::forward<Ts>(vs)), ...);
  }

  template<usize I>
  void
  __destroy_column_elements() noexcept
  {
    using T = __col_t<I>;
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      T *p = __as<I>(__cols[I]);
      for ( usize i = 0; i < __size; ++i ) p[i].~T();
    }
  }

  template<usize... Is>
  void
  __destroy_all_elements(micron::index_sequence<Is...>) noexcept
  {
    (__destroy_column_elements<Is>(), ...);
  }

  template<usize I>
  void
  __pop_back_column() noexcept
  {
    using T = __col_t<I>;
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      __as<I>(__cols[I])[__size - 1].~T();
    }
  }

  template<usize... Is>
  void
  __pop_back_all(micron::index_sequence<Is...>) noexcept
  {
    (__pop_back_column<Is>(), ...);
  }

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;

  static constexpr usize column_count = __ncols;

  ~soa() { __free_all(micron::make_index_sequence<__ncols>{}); }

  soa() = default;

  explicit soa(usize cap)
  {
    if ( cap > 0 ) {
      __alloc_all(cap, micron::make_index_sequence<__ncols>{});
      __capacity = cap;
    }
  }

  soa(const soa &) = delete;
  soa &operator=(const soa &) = delete;

  soa(soa &&o) noexcept : __size(o.__size), __capacity(o.__capacity)
  {
    for ( usize i = 0; i < __ncols; ++i ) {
      __cols[i] = o.__cols[i];
      o.__cols[i] = nullptr;
    }
    o.__size = 0;
    o.__capacity = 0;
  }

  soa &
  operator=(soa &&o) noexcept
  {
    if ( this == &o ) return *this;
    __free_all(micron::make_index_sequence<__ncols>{});
    for ( usize i = 0; i < __ncols; ++i ) {
      __cols[i] = o.__cols[i];
      o.__cols[i] = nullptr;
    }
    __size = o.__size;
    __capacity = o.__capacity;
    o.__size = 0;
    o.__capacity = 0;
    return *this;
  }

  usize
  size() const noexcept
  {
    return __size;
  }

  usize
  capacity() const noexcept
  {
    return __capacity;
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  void
  reserve(usize cap)
  {
    __grow_to(cap);
  }

  void
  emplace_back(Ts... vs)
  {
    if ( __size == __capacity ) __grow_to(__capacity == 0 ? 16 : __capacity * 2);
    __emplace_at(__size, micron::make_index_sequence<__ncols>{}, micron::forward<Ts>(vs)...);
    ++__size;
  }

  // typed access to column I
  template<usize I>
  __col_t<I> *
  column() noexcept
  {
    return __as<I>(__cols[I]);
  }

  template<usize I>
  const __col_t<I> *
  column() const noexcept
  {
    return __as<I>(__cols[I]);
  }

  template<usize I>
  __col_t<I> &
  at(usize idx)
  {
    if ( idx >= __size ) [[unlikely]]
      exc<except::library_error>("soa::at: out of range");
    return __as<I>(__cols[I])[idx];
  }

  template<usize I>
  const __col_t<I> &
  at(usize idx) const
  {
    if ( idx >= __size ) [[unlikely]]
      exc<except::library_error>("soa::at: out of range");
    return __as<I>(__cols[I])[idx];
  }

  void
  clear() noexcept
  {
    __destroy_all_elements(micron::make_index_sequence<__ncols>{});
    __size = 0;
  }

  usize
  max_size() const noexcept
  {
    return __capacity;
  }

  void
  swap(soa &o) noexcept
  {
    for ( usize i = 0; i < __ncols; ++i ) {
      void *t = __cols[i];
      __cols[i] = o.__cols[i];
      o.__cols[i] = t;
    }
    usize s = __size;
    __size = o.__size;
    o.__size = s;
    usize c = __capacity;
    __capacity = o.__capacity;
    o.__capacity = c;
  }

  void
  pop_back()
  {
    if ( __size == 0 ) [[unlikely]]
      exc<except::library_error>("soa::pop_back: empty");
    __pop_back_all(micron::make_index_sequence<__ncols>{});
    --__size;
  }
};

};      // namespace micron
