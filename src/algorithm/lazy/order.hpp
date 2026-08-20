//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../../sort/sort.hpp"
#include "../../vector/fvector.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reverse / sort / sort_by

namespace micron
{
namespace lz
{

namespace __impl
{

template<typename I> class __rev_cursor
{
  mutable I __i{};

public:
  using value_type = micron::iter_value_of_t<I>;
  using reference = micron::iter_ref_t<I>;
  using difference_type = micron::iter_diff_t<I>;

  constexpr __rev_cursor() = default;

  constexpr explicit __rev_cursor(I __ii) : __i(__ii) { }

  constexpr reference
  operator*() const
  {

    if constexpr ( micron::random_access_iterator<I> )
      return __i[-1];
    else {
      I __t = __i;
      --__t;
      return *__t;
    }
  }

  constexpr __rev_cursor &
  operator++()
  {
    --__i;
    return *this;
  }

  constexpr __rev_cursor
  operator++(int)
  {
    __rev_cursor __t = *this;
    --__i;
    return __t;
  }

  constexpr __rev_cursor &
  operator--()
  {
    ++__i;
    return *this;
  }

  constexpr __rev_cursor &
  operator+=(difference_type __n)
    requires micron::random_access_iterator<I>
  {
    __i -= __n;
    return *this;
  }

  constexpr __rev_cursor &
  operator-=(difference_type __n)
    requires micron::random_access_iterator<I>
  {
    __i += __n;
    return *this;
  }

  constexpr __rev_cursor
  operator+(difference_type __n) const
    requires micron::random_access_iterator<I>
  {
    return __rev_cursor{ __i - __n };
  }

  constexpr __rev_cursor
  operator-(difference_type __n) const
    requires micron::random_access_iterator<I>
  {
    return __rev_cursor{ __i + __n };
  }

  constexpr difference_type
  operator-(const __rev_cursor &__o) const
    requires micron::random_access_iterator<I>
  {
    return __o.__i - __i;
  }

  constexpr reference
  operator[](difference_type __n) const
    requires micron::random_access_iterator<I>
  {
    return __i[-__n - 1];
  }

  constexpr bool
  operator<(const __rev_cursor &__o) const
    requires micron::random_access_iterator<I>
  {
    return __o.__i < __i;
  }

  constexpr bool
  operator>(const __rev_cursor &__o) const
    requires micron::random_access_iterator<I>
  {
    return __i < __o.__i;
  }

  constexpr bool
  operator<=(const __rev_cursor &__o) const
    requires micron::random_access_iterator<I>
  {
    return !(__o.__i < __i);
  }

  constexpr bool
  operator>=(const __rev_cursor &__o) const
    requires micron::random_access_iterator<I>
  {
    return !(__i < __o.__i);
  }

  constexpr bool
  operator==(const __rev_cursor &__o) const
  {
    return __i == __o.__i;
  }

  constexpr bool
  operator!=(const __rev_cursor &__o) const
  {
    return !(*this == __o);
  }
};

};      // namespace __impl

template<typename T> class buffer_view: public micron::view_interface<buffer_view<T>>
{
  micron::fvector<T> __buf;

public:
  using __lazy_view_tag = void;
  using value_type = T;

  static constexpr size_kind __kind = size_kind::exact;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = true;

  constexpr explicit buffer_view(micron::fvector<T> &&__b) : __buf(micron::move(__b)) { }

  buffer_view(const buffer_view &) = delete;
  buffer_view &operator=(const buffer_view &) = delete;
  constexpr buffer_view(buffer_view &&) = default;

  constexpr const T *
  begin() const noexcept
  {
    return __buf.begin();
  }

  constexpr const T *
  end() const noexcept
  {
    return __buf.end();
  }

  constexpr usize
  __size_exact() const noexcept
  {
    return __buf.size();
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return __buf.size();
  }
};

namespace __impl
{

template<typename V>
constexpr auto
__drain(const V &__v)
{
  micron::fvector<micron::ranges::range_value_t<V>> __b;
  const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
  if ( __hint ) __b.reserve(__hint);
  __each(__v, [&](auto &&__x) { __b.push_back(static_cast<micron::ranges::range_value_t<V>>(__x)); });
  return __b;
}

};      // namespace __impl

template<typename V> class reverse_view: public micron::view_interface<reverse_view<V>>
{
  V __base;

  using __ui = micron::ranges::iterator_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = kind_of<V>;
  static constexpr usize __static_size = static_size_of<V>;
  static constexpr bool __is_materializing = false;

  using iterator = __impl::__rev_cursor<__ui>;

  constexpr explicit reverse_view(V __v) : __base(micron::move(__v)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::end(__base) };
  }

  constexpr iterator
  end() const
  {
    return iterator{ micron::ranges::begin(__base) };
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    return static_cast<usize>(micron::ranges::size(__base));
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

struct __reverse_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::reverse: this pipeline never terminates, and reverse starts at the END");

    if constexpr ( reversible<B> ) {
      return reverse_view<B>{ micron::move(__v) };
    } else {

      using T = micron::ranges::range_value_t<B>;
      auto __b = __impl::__drain(__v);
      const usize __n = __b.size();
      for ( usize __k = 0; __k + __k + 1 < __n; ++__k ) micron::swap(__b[__k], __b[__n - 1 - __k]);
      return buffer_view<T>{ micron::move(__b) };
    }
  }
};

[[nodiscard]] constexpr auto
reverse() noexcept
{
  return __reverse_fn{};
}

[[nodiscard]] constexpr auto
reverse_c() noexcept
{
  return __reverse_fn{};
}

struct __sort_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using T = micron::ranges::range_value_t<B>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::sort: this pipeline never terminates, and sort needs all of it");

    auto __b = __impl::__drain(__v);
    micron::sort::sort(__b);
    return buffer_view<T>{ micron::move(__b) };
  }
};

template<typename Cmp> struct __sort_by_fn {
  [[no_unique_address]] Cmp __cmp;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using T = micron::ranges::range_value_t<B>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::sort_by: this pipeline never terminates, and sort needs all of it");

    auto __b = __impl::__drain(__v);
    micron::sort::sort(__b, __cmp);
    return buffer_view<T>{ micron::move(__b) };
  }
};

[[nodiscard]] constexpr auto
sort() noexcept
{
  return __sort_fn{};
}

template<typename Cmp>
[[nodiscard]] constexpr auto
sort_by(Cmp &&__c)
{
  return __sort_by_fn<micron::decay_t<Cmp>>{ micron::forward<Cmp>(__c) };
}

[[nodiscard]] constexpr auto
sort_c() noexcept
{
  return __sort_fn{};
}

template<typename Cmp>
[[nodiscard]] constexpr auto
sort_by_c(Cmp &&__c)
{
  return sort_by(micron::forward<Cmp>(__c));
}

};      // namespace lz
};      // namespace micron
