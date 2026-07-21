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

  template<typename T>
  static void *
  __raw_new(usize cap)
  {
    if constexpr ( alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
      return ::operator new(sizeof(T) * cap, static_cast<std::align_val_t>(alignof(T)));
    else
      return ::operator new(sizeof(T) * cap);
  }

  template<typename T>
  static void
  __raw_delete(void *p) noexcept
  {
    if ( !p ) return;
    if constexpr ( alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
      ::operator delete(p, static_cast<std::align_val_t>(alignof(T)));
    else
      ::operator delete(p);
  }

  template<usize I>
  void
  __alloc_column(usize cap)
  {
    __cols[I] = __raw_new<__col_t<I>>(cap);
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
      __raw_delete<T>(__cols[I]);
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
      static_assert(micron::is_nothrow_move_constructible_v<T>,
                    "soa: growable non-trivially-copyable column types must be nothrow-move-constructible");
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

  // roll back a partial allocation
  template<usize... Is>
  static void
  __free_raw_indexed(void **cols, usize upto, micron::index_sequence<Is...>) noexcept
  {
    (((void)(Is < upto ? (__raw_delete<__col_t<Is>>(cols[Is]), cols[Is] = nullptr, void()) : void())), ...);
  }

  // free OLD column buffers after a successful relocate
  template<usize... Is>
  void
  __free_old_raw(void **cols, micron::index_sequence<Is...>) noexcept
  {
    (((void)(cols[Is] ? (__raw_delete<__col_t<Is>>(cols[Is]), void()) : void())), ...);
  }

  template<usize... Is>
  void
  __grow_to_impl(usize new_cap, void **new_cols, micron::index_sequence<Is...> seq)
  {
    usize done = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      (((new_cols[Is] = __raw_new<__col_t<Is>>(new_cap)), ++done), ...);
    } catch ( ... ) {
      __free_raw_indexed(new_cols, done, seq);      // free the columns already allocated, then rethrow
      throw;
    }
#else
    (((new_cols[Is] = __raw_new<__col_t<Is>>(new_cap)), ++done), ...);
#endif
  }

  void
  __grow_to(usize new_cap)
  {
    if ( new_cap <= __capacity ) return;
    auto seq = micron::make_index_sequence<__ncols>{};
    void *new_cols[__ncols] = { nullptr };
    __grow_to_impl(new_cap, new_cols, seq);
    __move_all(__cols, new_cols, __size, seq);
    __free_old_raw(__cols, seq);
    for ( usize i = 0; i < __ncols; ++i ) __cols[i] = new_cols[i];
    __capacity = new_cap;
  }

  template<usize I>
  void
  __destroy_one_at(usize idx) noexcept
  {
    using T = __col_t<I>;
    if constexpr ( !micron::is_trivially_destructible_v<T> ) __as<I>(__cols[I])[idx].~T();
  }

  template<usize... Is>
  void
  __destroy_row_indexed(usize idx, usize upto, micron::index_sequence<Is...>) noexcept
  {
    (((void)(Is < upto ? (__destroy_one_at<Is>(idx), void()) : void())), ...);
  }

  template<usize... Is>
  void
  __emplace_at(usize idx, micron::index_sequence<Is...> seq, Ts &&...vs)
  {
    usize done = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      (((void)(new (__as<Is>(__cols[Is]) + idx) __col_t<Is>(micron::forward<Ts>(vs))), ++done), ...);
    } catch ( ... ) {
      __destroy_row_indexed(idx, done, seq);      // unwind the already-built columns of this row
      throw;
    }
#else
    (((void)(new (__as<Is>(__cols[Is]) + idx) __col_t<Is>(micron::forward<Ts>(vs))), ++done), ...);
#endif
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

  template<usize I>
  __col_t<I> &
  front() noexcept
  {
    return __as<I>(__cols[I])[0];
  }

  template<usize I>
  const __col_t<I> &
  front() const noexcept
  {
    return __as<I>(__cols[I])[0];
  }

  template<usize I>
  __col_t<I> &
  back() noexcept
  {
    return __as<I>(__cols[I])[__size - 1];
  }

  template<usize I>
  const __col_t<I> &
  back() const noexcept
  {
    return __as<I>(__cols[I])[__size - 1];
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
