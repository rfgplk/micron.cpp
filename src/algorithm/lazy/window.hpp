//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "order.hpp"
#include "source.hpp"

#include "../../sum.hpp"
#include "../fperrors.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// chunk / chunk_into / sliding / group / group_by / transpose
namespace micron
{
namespace lz
{

template<typename V> class chunk_view: public micron::view_interface<chunk_view<V>>
{
  static_assert(flat_exact<V>, "micron::lz::chunk_view: base must be a flat exact range; use lz::chunk(n), which drains first");

  using __T = micron::ranges::range_value_t<V>;

  V __base;
  usize __n;

public:
  using __lazy_view_tag = void;
  using value_type = ptr_view<__T>;

  static constexpr size_kind __kind = size_kind::exact;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = materializes<V>;

  struct sentinel {
  };

  class iterator
  {
    const __T *__p = nullptr;
    const __T *__e = nullptr;
    usize __n = 1;

  public:
    using value_type = ptr_view<__T>;
    using reference = ptr_view<__T>;
    using difference_type = ssize_t;

    constexpr iterator() = default;

    constexpr iterator(const __T *__b, const __T *__ee, usize __nn) : __p(__b), __e(__ee), __n(__nn ? __nn : 1u) { }

    constexpr ptr_view<__T>
    operator*() const
    {
      const usize __left = static_cast<usize>(__e - __p);
      return ptr_view<__T>{ __p, __p + (__n < __left ? __n : __left) };
    }

    constexpr iterator &
    operator++()
    {
      const usize __left = static_cast<usize>(__e - __p);
      __p += (__n < __left ? __n : __left);
      return *this;
    }

    constexpr iterator
    operator++(int)
    {
      iterator __t = *this;
      ++*this;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const
    {
      return __p == __e;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr chunk_view(V __v, usize __nn) : __base(micron::move(__v)), __n(__nn ? __nn : 1u) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __n };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  __size_exact() const
  {
    const usize __s = static_cast<usize>(micron::ranges::size(__base));
    return (__s + __n - 1u) / __n;
  }

  constexpr usize
  reserve_hint() const
  {
    return __size_exact();
  }
};

namespace __impl
{

template<typename V>
constexpr auto
__flatten_source(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  if constexpr ( flat_exact<B> )
    return B{ micron::move(__v) };
  else
    return buffer_view<micron::ranges::range_value_t<B>>{ __drain(__v) };
}

};      // namespace __impl

struct __chunk_fn {
  usize __n;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::chunk: this pipeline never terminates");
    auto __f = __impl::__flatten_source(micron::move(__v));
    return chunk_view<micron::remove_cvref_t<decltype(__f)>>{ micron::move(__f), __n };
  }
};

[[nodiscard]] constexpr auto
chunk(usize __n) noexcept
{
  return __chunk_fn{ __n };
}

template<typename Out> struct __chunk_into_fn {
  usize __n;

  template<typename R>
  constexpr Out
  operator()(R &&__r) const
  {
    using Inner = typename Out::value_type;
    Out __out;
    if ( __n == 0 ) return __out;

    auto __v = __as_view(micron::forward<R>(__r));
    Inner __cur;
    usize __k = 0;
    __impl::__each(__v, [&](auto &&__x) {
      __cur.push_back(static_cast<typename Inner::value_type>(__x));
      if ( ++__k == __n ) {
        __out.push_back(micron::move(__cur));
        __cur = Inner{};
        __k = 0;
      }
    });
    if ( __k ) __out.push_back(micron::move(__cur));
    return __out;
  }
};

template<typename Out>
[[nodiscard]] constexpr auto
chunk_into(usize __n) noexcept
{
  return __chunk_into_fn<Out>{ __n };
}

template<typename V> class sliding_view: public micron::view_interface<sliding_view<V>>
{
  static_assert(flat_exact<V>, "micron::lz::sliding_view: base must be a flat exact range; use lz::sliding(n)");

  using __T = micron::ranges::range_value_t<V>;

  V __base;
  usize __n;

public:
  using __lazy_view_tag = void;
  using value_type = ptr_view<__T>;

  static constexpr size_kind __kind = size_kind::exact;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = materializes<V>;

  struct sentinel {
  };

  class iterator
  {
    const __T *__p = nullptr;
    const __T *__e = nullptr;
    usize __n = 1;

  public:
    using value_type = ptr_view<__T>;
    using reference = ptr_view<__T>;
    using difference_type = ssize_t;

    constexpr iterator() = default;

    constexpr iterator(const __T *__b, const __T *__ee, usize __nn) : __p(__b), __e(__ee), __n(__nn ? __nn : 1u) { }

    constexpr ptr_view<__T>
    operator*() const
    {
      return ptr_view<__T>{ __p, __p + __n };
    }

    constexpr iterator &
    operator++()
    {
      ++__p;
      return *this;
    }

    constexpr iterator
    operator++(int)
    {
      iterator __t = *this;
      ++__p;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const
    {
      return static_cast<usize>(__e - __p) < __n;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr sliding_view(V __v, usize __nn) : __base(micron::move(__v)), __n(__nn ? __nn : 1u) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __n };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  __size_exact() const
  {
    const usize __s = static_cast<usize>(micron::ranges::size(__base));
    return __s < __n ? 0u : __s - __n + 1u;
  }

  constexpr usize
  reserve_hint() const
  {
    return __size_exact();
  }
};

struct __sliding_fn {
  usize __n;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::sliding: this pipeline never terminates");
    auto __f = __impl::__flatten_source(micron::move(__v));
    return sliding_view<micron::remove_cvref_t<decltype(__f)>>{ micron::move(__f), __n };
  }
};

[[nodiscard]] constexpr auto
sliding(usize __n) noexcept
{
  return __sliding_fn{ __n };
}

template<typename V, typename Eq> class group_view: public micron::view_interface<group_view<V, Eq>>
{
  static_assert(flat_exact<V>, "micron::lz::group_view: base must be a flat exact range; use lz::group_by(eq)");

  using __T = micron::ranges::range_value_t<V>;

  V __base;
  [[no_unique_address]] Eq __eq;

public:
  using __lazy_view_tag = void;
  using value_type = ptr_view<__T>;

  static constexpr size_kind __kind = size_kind::bounded;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = materializes<V>;

  struct sentinel {
  };

  class iterator
  {
    const __T *__p = nullptr;
    const __T *__q = nullptr;
    const __T *__e = nullptr;
    [[no_unique_address]] fn_carrier<Eq> __c{};

    constexpr void
    __extend()
    {
      __q = __p;
      if ( __q == __e ) return;
      ++__q;
      while ( __q != __e && call<Eq>(__c, *(__q - 1), *__q) ) ++__q;
    }

  public:
    using value_type = ptr_view<__T>;
    using reference = ptr_view<__T>;
    using difference_type = ssize_t;

    constexpr iterator() = default;

    constexpr iterator(const __T *__b, const __T *__ee, const Eq &__q0) : __p(__b), __e(__ee), __c(hold<Eq>(__q0)) { __extend(); }

    constexpr ptr_view<__T>
    operator*() const
    {
      return ptr_view<__T>{ __p, __q };
    }

    constexpr iterator &
    operator++()
    {
      __p = __q;
      __extend();
      return *this;
    }

    constexpr iterator
    operator++(int)
    {
      iterator __t = *this;
      ++*this;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const
    {
      return __p == __e;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr group_view(V __v, Eq __q) : __base(micron::move(__v)), __eq(micron::move(__q)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __eq };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::size(__base));
  }
};

namespace __impl
{

struct __group_eq {
  template<typename A, typename B>
  constexpr bool
  operator()(const A &__a, const B &__b) const
  {
    return __a == __b;
  }
};

};      // namespace __impl

template<typename Eq> struct __group_by_fn {
  [[no_unique_address]] Eq __q;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::group_by: this pipeline never terminates");
    auto __f = __impl::__flatten_source(micron::move(__v));
    return group_view<micron::remove_cvref_t<decltype(__f)>, Eq>{ micron::move(__f), __q };
  }
};

template<typename Eq>
[[nodiscard]] constexpr auto
group_by(Eq &&__q)
{
  return __group_by_fn<micron::decay_t<Eq>>{ micron::forward<Eq>(__q) };
}

[[nodiscard]] constexpr auto
group() noexcept
{
  return __group_by_fn<__impl::__group_eq>{ {} };
}

template<typename O, bool Trunc, typename V>
constexpr auto
__transpose_into(V &&__v)
{
  using Row = typename O::value_type;
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::transpose: this pipeline never terminates");

  micron::fvector<micron::ranges::range_value_t<B>> __rows = __impl::__drain(__v);
  const usize __nr = __rows.size();

  O __out;
  if ( __nr == 0 ) {
    if constexpr ( Trunc )
      return __out;
    else
      return micron::option<O, fp::bad_zip_error>{ micron::move(__out) };
  }

  usize __lo = static_cast<usize>(micron::ranges::size(__rows[0]));
  usize __hi = __lo;
  for ( usize __r = 1; __r < __nr; ++__r ) {
    const usize __w = static_cast<usize>(micron::ranges::size(__rows[__r]));
    if ( __w < __lo ) __lo = __w;
    if ( __w > __hi ) __hi = __w;
  }
  if constexpr ( !Trunc ) {
    if ( __lo != __hi ) return micron::option<O, fp::bad_zip_error>{ fp::bad_zip_error{} };
  }

  for ( usize __c = 0; __c < __lo; ++__c ) {
    Row __col;
    if constexpr ( requires(Row &__x, usize __k) { __x.reserve(__k); } ) __col.reserve(__nr);
    for ( usize __r = 0; __r < __nr; ++__r ) __col.push_back(static_cast<typename Row::value_type>(__rows[__r][__c]));
    __out.push_back(micron::move(__col));
  }

  if constexpr ( Trunc )
    return __out;
  else
    return micron::option<O, fp::bad_zip_error>{ micron::move(__out) };
}

template<typename O> struct __transpose_fn {
  template<typename R>
  constexpr micron::option<O, fp::bad_zip_error>
  operator()(R &&__r) const
  {
    return __transpose_into<O, false>(__as_view(micron::forward<R>(__r)));
  }
};

template<typename O> struct __transpose_trunc_fn {
  template<typename R>
  constexpr O
  operator()(R &&__r) const
  {
    return __transpose_into<O, true>(__as_view(micron::forward<R>(__r)));
  }
};

template<typename O>
[[nodiscard]] constexpr auto
transpose() noexcept
{
  return __transpose_fn<O>{};
}

template<typename O>
[[nodiscard]] constexpr auto
transpose_trunc() noexcept
{
  return __transpose_trunc_fn<O>{};
}

};      // namespace lz
};      // namespace micron
