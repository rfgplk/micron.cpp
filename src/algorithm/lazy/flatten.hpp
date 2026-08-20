//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"
#include "transform.hpp"

#include "../../memory/new.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// flatten / flat_map / concat / merge

namespace micron
{
namespace lz
{

namespace __impl
{

template<typename T> class __slot
{
  alignas(T) unsigned char __b[sizeof(T)];
  bool __on = false;

public:
  constexpr __slot() noexcept = default;

  __slot(const __slot &__o)
    requires micron::is_copy_constructible_v<T>
  {
    if ( __o.__on ) {
      ::new (static_cast<void *>(__b)) T(*__o.get());
      __on = true;
    }
  }

  __slot(__slot &&__o) noexcept
  {
    if ( __o.__on ) {
      ::new (static_cast<void *>(__b)) T(micron::move(*__o.get()));
      __on = true;
      __o.clear();
    }
  }

  __slot &operator=(const __slot &) = delete;
  __slot &operator=(__slot &&) = delete;

  ~__slot() { clear(); }

  void
  clear() noexcept
  {
    if ( __on ) {
      get()->~T();
      __on = false;
    }
  }

  template<typename... A>
  T *
  put(A &&...__a)
  {
    clear();
    ::new (static_cast<void *>(__b)) T(micron::forward<A>(__a)...);
    __on = true;
    return get();
  }

  T *
  get() noexcept
  {
    return reinterpret_cast<T *>(__b);
  }

  const T *
  get() const noexcept
  {
    return reinterpret_cast<const T *>(__b);
  }

  bool
  live() const noexcept
  {
    return __on;
  }
};

};      // namespace __impl

template<typename V> class flatten_view: public micron::view_interface<flatten_view<V>>
{
  using __oi = micron::ranges::iterator_t<V>;
  using __os = micron::ranges::sentinel_t<V>;
  using __oref = micron::ranges::range_reference_t<V>;

public:
  using Inner = micron::remove_cvref_t<__oref>;

private:
  static constexpr bool __by_value = !micron::is_lvalue_reference_v<__oref>;
  using __inner_lv = micron::conditional_t<__by_value, Inner &, __oref>;

  V __base;
  mutable __impl::__slot<Inner> __hold;

  using __ii = decltype(micron::ranges::begin(micron::declval<__inner_lv>()));
  using __is = decltype(micron::ranges::end(micron::declval<__inner_lv>()));

public:
  using __lazy_view_tag = void;
  using value_type = micron::remove_cvref_t<decltype(*micron::declval<__ii &>())>;

  static constexpr size_kind __kind = (kind_of<V> == size_kind::endless) ? size_kind::endless : size_kind::unknown;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __oi __io{};
    __os __eo{};
    const flatten_view *__p = nullptr;
    mutable __ii __ib{};
    mutable __is __ie{};
    bool __done = true;

    constexpr void
    __open()
    {
      while ( !(__io == __eo) ) {
        if constexpr ( __by_value ) {
          Inner *__s = __p->__hold.put(*__io);
          __ib = micron::ranges::begin(*__s);
          __ie = micron::ranges::end(*__s);
        } else {
          __inner_lv __r = *__io;
          __ib = micron::ranges::begin(__r);
          __ie = micron::ranges::end(__r);
        }
        if ( !(__ib == __ie) ) {
          __done = false;
          return;
        }
        ++__io;
      }
      __done = true;
    }

  public:
    using value_type = flatten_view::value_type;
    using reference = decltype(*micron::declval<__ii &>());
    using difference_type = micron::iter_diff_t<__ii>;

    constexpr iterator() = default;

    constexpr iterator(__oi __o0, __os __o1, const flatten_view *__pp) : __io(__o0), __eo(__o1), __p(__pp) { __open(); }

    constexpr reference
    operator*() const
    {
      return *__ib;
    }

    constexpr iterator &
    operator++()
    {
      ++__ib;
      if ( __ib == __ie ) {
        ++__io;
        __open();
      }
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
      return __done;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr explicit flatten_view(V __v) : __base(micron::move(__v)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), this };
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

struct __flatten_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return flatten_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v) };
  }
};

[[nodiscard]] constexpr auto
flatten() noexcept
{
  return __flatten_fn{};
}

template<typename F> struct __flat_map_fn {
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return flatten()(fmap(__f)(micron::forward<R>(__r)));
  }
};

template<typename F>
[[nodiscard]] constexpr auto
flat_map(F &&__f)
{
  return __flat_map_fn<micron::decay_t<F>>{ micron::forward<F>(__f) };
}

template<typename V, typename W> class concat_view: public micron::view_interface<concat_view<V, W>>
{
  V __a;
  W __b;

  using __ai = micron::ranges::iterator_t<V>;
  using __as = micron::ranges::sentinel_t<V>;
  using __bi = micron::ranges::iterator_t<W>;
  using __bs = micron::ranges::sentinel_t<W>;

  using __aref = micron::ranges::range_reference_t<V>;
  using __bref = micron::ranges::range_reference_t<W>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  using __ref = micron::conditional_t<micron::is_same_v<__aref, __bref>, __aref, value_type>;

  static constexpr size_kind __kind
      = (kind_of<V> == size_kind::endless || kind_of<W> == size_kind::endless)
            ? size_kind::endless
            : ((kind_of<V> == size_kind::exact && kind_of<W> == size_kind::exact) ? size_kind::exact : size_kind::unknown);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ai __ia{};
    __as __ea{};
    mutable __bi __ib{};
    __bs __eb{};
    bool __second = false;

  public:
    using value_type = concat_view::value_type;
    using reference = concat_view::__ref;
    using difference_type = micron::iter_diff_t<__ai>;

    constexpr iterator() = default;

    constexpr iterator(__ai __a0, __as __a1, __bi __b0, __bs __b1) : __ia(__a0), __ea(__a1), __ib(__b0), __eb(__b1)
    {
      __second = (__ia == __ea);
    }

    constexpr reference
    operator*() const
    {
      return __second ? static_cast<reference>(*__ib) : static_cast<reference>(*__ia);
    }

    constexpr iterator &
    operator++()
    {
      if ( !__second ) {
        ++__ia;
        if ( __ia == __ea ) __second = true;
      } else {
        ++__ib;
      }
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
      return __second && __ib == __eb;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr concat_view(V __v, W __w) : __a(micron::move(__v)), __b(micron::move(__w)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__a), micron::ranges::end(__a), micron::ranges::begin(__b), micron::ranges::end(__b) };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    return static_cast<usize>(micron::ranges::size(__a)) + static_cast<usize>(micron::ranges::size(__b));
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__a)) + static_cast<usize>(micron::ranges::reserve_hint(__b));
  }
};

template<typename W> struct __concat_fn {
  W __w;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return concat_view<micron::remove_cvref_t<decltype(__v)>, W>{ micron::move(__v), __w };
  }
};

template<typename Other>
[[nodiscard]] constexpr auto
concat(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __concat_fn<decltype(__w)>{ micron::move(__w) };
}

template<typename Other>
[[nodiscard]] constexpr auto
merge(Other &&__o)
{
  return concat(micron::forward<Other>(__o));
}

};      // namespace lz
};      // namespace micron
