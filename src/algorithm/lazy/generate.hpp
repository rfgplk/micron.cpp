//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../../sum.hpp"
#include "../../tuple.hpp"
#include "take.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// generators
namespace micron
{
namespace lz
{

template<typename T> class iota_view: public micron::view_interface<iota_view<T>>
{
  T __from;

public:
  using __lazy_view_tag = void;
  using value_type = T;

  static constexpr size_kind __kind = size_kind::endless;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    T __v{};

  public:
    using value_type = T;
    using reference = T;
    using difference_type = micron::iter_difference_t<T>;

    constexpr iterator() = default;

    constexpr explicit iterator(T __vv) : __v(__vv) { }

    constexpr T
    operator*() const
    {
      return __v;
    }

    constexpr iterator &
    operator++()
    {
      ++__v;
      return *this;
    }

    constexpr iterator
    operator++(int)
    {
      iterator __t = *this;
      ++__v;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const noexcept
    {
      return false;
    }

    constexpr bool
    operator!=(const sentinel &) const noexcept
    {
      return true;
    }
  };

  constexpr explicit iota_view(T __f) : __from(__f) { }

  constexpr iterator
  begin() const
  {
    return iterator{ __from };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return 0u;
  }
};

template<typename T>
[[nodiscard]] constexpr auto
counting(T __from) noexcept
{
  return iota_view<T>{ __from };
}

template<typename T, typename F> class iterate_view: public micron::view_interface<iterate_view<T, F>>
{
  T __seed;
  [[no_unique_address]] F __fn;

public:
  using __lazy_view_tag = void;
  using value_type = T;

  static constexpr size_kind __kind = size_kind::endless;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    T __v{};
    [[no_unique_address]] fn_carrier<F> __f{};

  public:
    using value_type = T;
    using reference = const T &;
    using difference_type = ssize_t;

    constexpr iterator() = default;

    constexpr iterator(T __vv, const F &__ff) : __v(__vv), __f(hold<F>(__ff)) { }

    constexpr const T &
    operator*() const
    {
      return __v;
    }

    constexpr iterator &
    operator++()
    {
      __v = call<F>(__f, __v);
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
    operator==(const sentinel &) const noexcept
    {
      return false;
    }

    constexpr bool
    operator!=(const sentinel &) const noexcept
    {
      return true;
    }
  };

  constexpr iterate_view(T __s, F __f) : __seed(__s), __fn(micron::move(__f)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ __seed, __fn };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return 0u;
  }
};

template<typename F, typename T>
[[nodiscard]] constexpr auto
iterate(F &&__f, T __seed)
{
  return iterate_view<T, micron::decay_t<F>>{ __seed, micron::forward<F>(__f) };
}

template<typename T>
[[nodiscard]] constexpr auto
repeat(T __v)
{
  return iterate([](const T &__x) { return __x; }, __v);
}

template<typename T>
[[nodiscard]] constexpr auto
replicate(usize __n, T __v)
{
  return repeat(__v) | take(__n);
}

template<typename T, usize N> class fixed_view: public micron::view_interface<fixed_view<T, N>>
{
  T __v{};

public:
  using __lazy_view_tag = void;
  using value_type = T;

  static constexpr size_kind __kind = size_kind::exact;
  static constexpr usize __static_size = N;
  static constexpr bool __is_materializing = false;

  constexpr fixed_view() = default;

  constexpr explicit fixed_view(T __vv) : __v(__vv) { }

  constexpr const T *
  begin() const noexcept
  {
    return micron::addressof(__v);
  }

  constexpr const T *
  end() const noexcept
  {
    return micron::addressof(__v) + N;
  }

  constexpr usize
  __size_exact() const noexcept
  {
    return N;
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return N;
  }
};

template<typename T>
[[nodiscard]] constexpr auto
once(T __v)
{
  return fixed_view<T, 1>{ __v };
}

template<typename T>
[[nodiscard]] constexpr auto
empty() noexcept
{
  return fixed_view<T, 0>{};
}

template<typename T, typename B, typename F> class unfold_view: public micron::view_interface<unfold_view<T, B, F>>
{
  B __seed;
  [[no_unique_address]] F __fn;

public:
  using __lazy_view_tag = void;
  using value_type = T;

  static constexpr size_kind __kind = size_kind::unknown;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    B __st{};
    T __cur{};
    bool __live = false;
    [[no_unique_address]] fn_carrier<F> __f{};

    constexpr void
    __pull()
    {
      auto __r = call<F>(__f, __st);
      if ( !__r.is_first() ) {
        __live = false;
        return;
      }
      auto __p = __r.template cast<typename micron::remove_cvref_t<decltype(__r)>::first_type>();
      __cur = __impl::__first_of(__p);
      __st = __impl::__second_of(__p);
      __live = true;
    }

  public:
    using value_type = T;
    using reference = const T &;
    using difference_type = ssize_t;

    constexpr iterator() = default;

    constexpr iterator(B __s, const F &__ff) : __st(__s), __f(hold<F>(__ff)) { __pull(); }

    constexpr const T &
    operator*() const
    {
      return __cur;
    }

    constexpr iterator &
    operator++()
    {
      __pull();
      return *this;
    }

    constexpr bool
    operator==(const sentinel &) const noexcept
    {
      return !__live;
    }

    constexpr bool
    operator!=(const sentinel &__s) const noexcept
    {
      return !(*this == __s);
    }
  };

  constexpr unfold_view(B __s, F __f) : __seed(__s), __fn(micron::move(__f)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ __seed, __fn };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return 0u;
  }
};

template<typename F, typename B>
[[nodiscard]] constexpr auto
unfold(F &&__f, B __seed)
{
  using Res = micron::remove_cvref_t<micron::invoke_result_t<const micron::decay_t<F> &, const B &>>;
  using Pair = typename Res::first_type;
  using T = micron::remove_cvref_t<decltype(__impl::__first_of(micron::declval<Pair>()))>;
  return unfold_view<T, B, micron::decay_t<F>>{ __seed, micron::forward<F>(__f) };
}

};      // namespace lz
};      // namespace micron
