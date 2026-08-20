//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__lazy_bits.hpp"

namespace micron
{
namespace lz
{

template<typename T> class ptr_view: public micron::view_interface<ptr_view<T>>
{
  const T *__f = nullptr;
  const T *__l = nullptr;

public:
  using __lazy_view_tag = void;
  using __borrowed_tag = void;
  using value_type = T;

  static constexpr size_kind __kind = size_kind::exact;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  constexpr ptr_view() = default;

  constexpr ptr_view(const T *__b, const T *__e) noexcept : __f(__b), __l(__e) { }

  constexpr const T *
  begin() const noexcept
  {
    return __f;
  }

  constexpr const T *
  end() const noexcept
  {
    return __l;
  }

  constexpr usize
  __size_exact() const noexcept
  {
    return static_cast<usize>(__l - __f);
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return __size_exact();
  }
};

template<typename C> class ref_view: public micron::view_interface<ref_view<C>>
{
  C *__c = nullptr;

public:
  using __lazy_view_tag = void;
  using __borrowed_tag = void;
  using value_type = micron::ranges::range_value_t<C>;

  static constexpr size_kind __kind = cheaply_sized<C> ? size_kind::exact : size_kind::unknown;
  static constexpr usize __static_size
      = micron::unrollable<micron::remove_cvref_t<C>> ? micron::remove_cvref_t<C>::static_size : no_static_size;
  static constexpr bool __is_materializing = false;

  constexpr ref_view() = default;

  constexpr explicit ref_view(C &__cc) noexcept : __c(micron::addressof(__cc)) { }

  constexpr auto
  begin() const
  {
    return micron::ranges::begin(*__c);
  }

  constexpr auto
  end() const
  {
    return micron::ranges::end(*__c);
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    return static_cast<usize>(micron::ranges::size(*__c));
  }

  constexpr usize
  reserve_hint() const
  {
    if constexpr ( __kind == size_kind::exact )
      return static_cast<usize>(micron::ranges::size(*__c));
    else
      return 0u;
  }
};

template<typename C> class owning_view: public micron::view_interface<owning_view<C>>
{
  C __c;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<C>;

  static constexpr size_kind __kind = cheaply_sized<C> ? size_kind::exact : size_kind::unknown;
  static constexpr usize __static_size = micron::unrollable<C> ? C::static_size : no_static_size;
  static constexpr bool __is_materializing = false;

  constexpr explicit owning_view(C &&__cc) noexcept(micron::is_nothrow_move_constructible_v<C>) : __c(micron::move(__cc)) { }

  owning_view(const owning_view &) = delete;
  owning_view &operator=(const owning_view &) = delete;
  constexpr owning_view(owning_view &&) = default;

  constexpr auto
  begin() const
  {
    return micron::ranges::cbegin(__c);
  }

  constexpr auto
  end() const
  {
    return micron::ranges::cend(__c);
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    return static_cast<usize>(micron::ranges::size(__c));
  }

  constexpr usize
  reserve_hint() const
  {
    if constexpr ( __kind == size_kind::exact )
      return static_cast<usize>(micron::ranges::size(__c));
    else
      return 0u;
  }
};

template<typename R>
constexpr decltype(auto)
__as_view(R &&__r)
{
  using B = micron::remove_cvref_t<R>;
  if constexpr ( micron::ranges::view<B> ) {

    if constexpr ( micron::is_lvalue_reference_v<R> )
      return (__r);
    else
      return B{ micron::move(__r) };
  } else if constexpr ( micron::is_lvalue_reference_v<R> ) {

    if constexpr ( micron::is_contiguous_container<B> && !micron::unrollable<B> ) {
      using T = micron::ranges::range_value_t<B>;
      return ptr_view<T>{ micron::ranges::cbegin(__r), micron::ranges::cend(__r) };
    } else {
      return ref_view<micron::remove_reference_t<R>>{ __r };
    }
  } else {
    return owning_view<B>{ micron::move(__r) };
  }
}

template<typename R> using as_view_t = decltype(__as_view(micron::declval<R>()));

template<typename R>
[[nodiscard]] constexpr auto
of(R &&__r)
{
  return __as_view(micron::forward<R>(__r));
}

template<typename C>
[[nodiscard]] constexpr auto
owned(C &&__c)
  requires micron::is_rvalue_reference_v<C &&>
{
  return owning_view<micron::remove_cvref_t<C>>{ micron::move(__c) };
}

};      // namespace lz
};      // namespace micron
