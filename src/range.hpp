// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "concepts.hpp"
#include "type_traits.hpp"
#include "types.hpp"

namespace micron
{

// ranges
// now (mostly) STL compliant

template <typename F, typename T>
concept range_size_t = micron::convertible_to<T, F>;

template <typename T, bool = micron::is_floating_point_v<T>> struct _iter_difference {
  using type = micron::make_signed_t<T>;
};

template <typename T> struct _iter_difference<T, true> {
  using type = T;
};

template <typename T> using iter_difference_t = typename _iter_difference<T>::type;

template <typename T, bool = micron::is_floating_point_v<T>> struct _iter_size {
  using type = micron::make_unsigned_t<T>;
};

template <typename T> struct _iter_size<T, true> {
  using type = usize;
};

template <typename T> using iter_size_t = typename _iter_size<T>::type;

template <typename T> struct counting_iter {
  using value_type = T;
  using difference_type = micron::iter_difference_t<T>;
  using pointer = const T *;
  using reference = T;

  T _val;

  constexpr counting_iter() noexcept : _val{} {}

  constexpr explicit counting_iter(T v) noexcept : _val(v) {}

  constexpr T
  operator*() const noexcept
  {
    return _val;
  }

  constexpr T
  operator[](difference_type n) const noexcept
  {
    return _val + static_cast<T>(n);
  }

  constexpr counting_iter &
  operator++() noexcept
  {
    ++_val;
    return *this;
  }

  constexpr counting_iter
  operator++(int) noexcept
  {
    auto t = *this;
    ++_val;
    return t;
  }

  constexpr counting_iter &
  operator--() noexcept
  {
    --_val;
    return *this;
  }

  constexpr counting_iter
  operator--(int) noexcept
  {
    auto t = *this;
    --_val;
    return t;
  }

  constexpr counting_iter
  operator+(difference_type n) const noexcept
  {
    return counting_iter(_val + static_cast<T>(n));
  }

  constexpr counting_iter
  operator-(difference_type n) const noexcept
  {
    return counting_iter(_val - static_cast<T>(n));
  }

  constexpr counting_iter &
  operator+=(difference_type n) noexcept
  {
    _val += static_cast<T>(n);
    return *this;
  }

  constexpr counting_iter &
  operator-=(difference_type n) noexcept
  {
    _val -= static_cast<T>(n);
    return *this;
  }

  constexpr difference_type
  operator-(const counting_iter &o) const noexcept
  {
    return static_cast<difference_type>(_val) - static_cast<difference_type>(o._val);
  }

  friend constexpr counting_iter
  operator+(difference_type n, const counting_iter &it) noexcept
  {
    return counting_iter(it._val + static_cast<T>(n));
  }

  constexpr bool
  operator==(const counting_iter &o) const noexcept
  {
    return _val == o._val;
  }

  constexpr bool
  operator!=(const counting_iter &o) const noexcept
  {
    return _val != o._val;
  }

  constexpr bool
  operator<(const counting_iter &o) const noexcept
  {
    return _val < o._val;
  }

  constexpr bool
  operator>(const counting_iter &o) const noexcept
  {
    return _val > o._val;
  }

  constexpr bool
  operator<=(const counting_iter &o) const noexcept
  {
    return _val <= o._val;
  }

  constexpr bool
  operator>=(const counting_iter &o) const noexcept
  {
    return _val >= o._val;
  }
};

template <typename Iter, bool = micron::is_pointer_v<Iter>> struct _iter_traits {
  using value_type = typename Iter::value_type;
  using difference_type = typename Iter::difference_type;
  using pointer = typename Iter::pointer;
  using reference = typename Iter::reference;
};

template <typename Iter> struct _iter_traits<Iter, true> {
  using value_type = micron::remove_pointer_t<Iter>;
  using difference_type = ssize_t;
  using pointer = Iter;
  using reference = value_type &;
};

template <typename Iter> struct reverse_iter {
  using _tr = _iter_traits<Iter>;
  using value_type = typename _tr::value_type;
  using difference_type = typename _tr::difference_type;
  using pointer = typename _tr::pointer;
  using reference = typename _tr::reference;

  Iter _base;

  constexpr reverse_iter() noexcept = default;

  constexpr explicit reverse_iter(Iter it) noexcept : _base(it) {}

  constexpr Iter
  base() const noexcept
  {
    return _base;
  }

  constexpr reference
  operator*() const noexcept
  {
    Iter tmp = _base;
    return *--tmp;
  }

  constexpr reference
  operator[](difference_type n) const noexcept
  {
    return *(*this + n);
  }

  constexpr reverse_iter &
  operator++() noexcept
  {
    --_base;
    return *this;
  }

  constexpr reverse_iter
  operator++(int) noexcept
  {
    auto t = *this;
    --_base;
    return t;
  }

  constexpr reverse_iter &
  operator--() noexcept
  {
    ++_base;
    return *this;
  }

  constexpr reverse_iter
  operator--(int) noexcept
  {
    auto t = *this;
    ++_base;
    return t;
  }

  constexpr reverse_iter
  operator+(difference_type n) const noexcept
  {
    return reverse_iter(_base - n);
  }

  constexpr reverse_iter
  operator-(difference_type n) const noexcept
  {
    return reverse_iter(_base + n);
  }

  constexpr reverse_iter &
  operator+=(difference_type n) noexcept
  {
    _base -= n;
    return *this;
  }

  constexpr reverse_iter &
  operator-=(difference_type n) noexcept
  {
    _base += n;
    return *this;
  }

  constexpr difference_type
  operator-(const reverse_iter &o) const noexcept
  {
    return o._base - _base;
  }

  friend constexpr reverse_iter
  operator+(difference_type n, const reverse_iter &it) noexcept
  {
    return reverse_iter(it._base - n);
  }

  constexpr bool
  operator==(const reverse_iter &o) const noexcept
  {
    return _base == o._base;
  }

  constexpr bool
  operator!=(const reverse_iter &o) const noexcept
  {
    return _base != o._base;
  }

  constexpr bool
  operator<(const reverse_iter &o) const noexcept
  {
    return _base > o._base;
  }

  constexpr bool
  operator>(const reverse_iter &o) const noexcept
  {
    return _base < o._base;
  }

  constexpr bool
  operator<=(const reverse_iter &o) const noexcept
  {
    return _base >= o._base;
  }

  constexpr bool
  operator>=(const reverse_iter &o) const noexcept
  {
    return _base <= o._base;
  }
};

template <typename T> struct const_counting_iter : counting_iter<T> {
  using typename counting_iter<T>::value_type;
  using typename counting_iter<T>::difference_type;
  using pointer = const T *;
  using reference = const T;

  using counting_iter<T>::counting_iter;

  constexpr const_counting_iter(const const_counting_iter &) noexcept = default;

  const_counting_iter &operator=(const_counting_iter &&) = delete;
  const_counting_iter &operator=(const const_counting_iter &) = delete;
};

template <umax_t From, range_size_t<umax_t> auto To>
  requires(From < To && micron::is_arithmetic_v<umax_t>)
struct range {

  using value_type = umax_t;
  using size_type = umax_t;
  using difference_type = micron::make_signed_t<umax_t>;
  using iterator = counting_iter<umax_t>;
  using const_iterator = const_counting_iter<umax_t>;
  using reverse_iterator = reverse_iter<iterator>;
  using const_reverse_iterator = reverse_iter<const_iterator>;

  ~range() = default;
  range(void) = default;
  range(const range &o) = default;
  range(range &&o) = default;
  range &operator=(const range &) = delete;
  range &operator=(range &&) = delete;

  template <typename F>
    requires micron::is_invocable_v<F>
  static void
  perform(F &f)
  {
    for ( umax_t i = From; i < To; i++ ) f();
  }

  template <class C, typename... Fargs>
  static void
  perform(C &obj, void (C::*f)(Fargs...), Fargs &&...args)
  {
    for ( umax_t i = From; i < To; i++ ) (obj.*f)(args...);
  }

  static constexpr iterator
  begin() noexcept
  {
    return iterator{ From };
  }

  static constexpr iterator
  end() noexcept
  {
    return iterator{ To };
  }

  static constexpr const_iterator
  cbegin() noexcept
  {
    return const_iterator{ From };
  }

  static constexpr const_iterator
  cend() noexcept
  {
    return const_iterator{ To };
  }

  static constexpr reverse_iterator
  rbegin() noexcept
  {
    return reverse_iterator{ end() };
  }

  static constexpr reverse_iterator
  rend() noexcept
  {
    return reverse_iterator{ begin() };
  }

  static constexpr const_reverse_iterator
  crbegin() noexcept
  {
    return const_reverse_iterator{ cend() };
  }

  static constexpr const_reverse_iterator
  crend() noexcept
  {
    return const_reverse_iterator{ cbegin() };
  }

  static constexpr size_type
  size() noexcept
  {
    return static_cast<size_type>(To - From);
  }

  static constexpr difference_type
  ssize() noexcept
  {
    return static_cast<difference_type>(To - From);
  }

  static constexpr bool
  empty() noexcept
  {
    return false;
  }
};

template <typename T, T From, range_size_t<T> auto To>
  requires(From < To && micron::is_arithmetic_v<T>)
struct count_range {

  using value_type = T;
  using size_type = micron::iter_size_t<T>;
  using difference_type = micron::iter_difference_t<T>;
  using iterator = counting_iter<T>;
  using const_iterator = const_counting_iter<T>;
  using reverse_iterator = reverse_iter<iterator>;
  using const_reverse_iterator = reverse_iter<const_iterator>;

  ~count_range() = default;
  count_range(void) = default;
  count_range(const count_range &o) = default;
  count_range(count_range &&o) = default;
  count_range &operator=(const count_range &) = delete;
  count_range &operator=(count_range &&) = delete;

  template <typename F>
    requires micron::is_invocable_v<F, T>
  static void
  perform(F &f)
  {
    for ( T i = From; i < To; i++ ) f(i);
  }

  template <class C, typename R, typename Arg = T>
  static void
  perform(C &obj, R (C::*f)(const Arg &))
  {
    for ( T i = From; i < To; i++ ) (obj.*f)(i);
  }

  template <class C, typename R, typename Arg = T>
  static void
  perform(C &obj, R (C::*f)(Arg))
  {
    for ( T i = From; i < To; i++ ) (obj.*f)(i);
  }

  template <class C, typename R, typename Arg = T>
  static void
  perform(C &obj, R (C::*f)(Arg &&))
  {
    for ( T i = From; i < To; i++ ) (obj.*f)(i);
  }

  static constexpr iterator
  begin() noexcept
  {
    return iterator{ From };
  }

  static constexpr iterator
  end() noexcept
  {
    return iterator{ To };
  }

  static constexpr const_iterator
  cbegin() noexcept
  {
    return const_iterator{ From };
  }

  static constexpr const_iterator
  cend() noexcept
  {
    return const_iterator{ To };
  }

  static constexpr reverse_iterator
  rbegin() noexcept
  {
    return reverse_iterator{ end() };
  }

  static constexpr reverse_iterator
  rend() noexcept
  {
    return reverse_iterator{ begin() };
  }

  static constexpr const_reverse_iterator
  crbegin() noexcept
  {
    return const_reverse_iterator{ cend() };
  }

  static constexpr const_reverse_iterator
  crend() noexcept
  {
    return const_reverse_iterator{ cbegin() };
  }

  static constexpr size_type
  size() noexcept
  {
    return static_cast<size_type>(To - From);
  }

  static constexpr difference_type
  ssize() noexcept
  {
    return static_cast<difference_type>(To - From);
  }

  static constexpr bool
  empty() noexcept
  {
    return false;
  }
};

template <typename T, range_size_t<size_t> auto Cnt> struct range_of {

  ~range_of() = default;
  range_of(void) = default;
  range_of(const range_of &o) = default;
  range_of(range_of &&o) = default;
  range_of &operator=(const range_of &) = delete;
  range_of &operator=(range_of &&) = delete;

  template <typename C, typename F>
  static void
  perform(C &obj, F f)
  {
    typename T::iterator end = obj.begin() + static_cast<typename T::size_type>(Cnt);
    for ( typename T::iterator itr = obj.begin(); itr != end; ++itr ) f(*itr);
  }

  template <typename C> struct view {
    using iterator = typename C::iterator;
    using const_iterator = typename C::const_iterator;
    using reverse_iterator = micron::reverse_iter<iterator>;
    using const_reverse_iterator = micron::reverse_iter<const_iterator>;
    using value_type = typename C::value_type;
    using size_type = typename C::size_type;
    using difference_type = decltype(micron::declval<iterator>() - micron::declval<iterator>());
    using pointer = typename C::pointer;
    using const_pointer = const value_type *;

    C &_c;

    explicit constexpr view(C &c) noexcept : _c(c) {}

    constexpr iterator
    begin() noexcept
    {
      return _c.begin();
    }

    constexpr iterator
    end() noexcept
    {
      return _c.begin() + static_cast<difference_type>(Cnt);
    }

    constexpr const_iterator
    begin() const noexcept
    {
      return _c.cbegin();
    }

    constexpr const_iterator
    end() const noexcept
    {
      return _c.cbegin() + static_cast<difference_type>(Cnt);
    }

    constexpr const_iterator
    cbegin() const noexcept
    {
      return _c.cbegin();
    }

    constexpr const_iterator
    cend() const noexcept
    {
      return _c.cbegin() + static_cast<difference_type>(Cnt);
    }

    constexpr reverse_iterator
    rbegin() noexcept
    {
      return reverse_iterator{ end() };
    }

    constexpr reverse_iterator
    rend() noexcept
    {
      return reverse_iterator{ begin() };
    }

    constexpr const_reverse_iterator
    crbegin() const noexcept
    {
      return const_reverse_iterator{ cend() };
    }

    constexpr const_reverse_iterator
    crend() const noexcept
    {
      return const_reverse_iterator{ cbegin() };
    }

    static constexpr size_type
    size() noexcept
    {
      return static_cast<size_type>(Cnt);
    }

    static constexpr difference_type
    ssize() noexcept
    {
      return static_cast<difference_type>(Cnt);
    }

    static constexpr bool
    empty() noexcept
    {
      return Cnt == 0;
    }

    constexpr pointer
    data() noexcept
    {
      return _c.data();
    }

    constexpr const_pointer
    cdata() const noexcept
    {
      return _c.data();
    }

    constexpr auto &
    operator[](size_type i) noexcept
    {
      return _c[i];
    }

    constexpr const auto &
    operator[](size_type i) const noexcept
    {
      return _c[i];
    }
  };

  template <typename C>
  static constexpr view<C>
  bind(C &c) noexcept
  {
    return view<C>{ c };
  }

  template <typename C>
  static constexpr view<const C>
  bind(const C &c) noexcept
  {
    return view<const C>{ const_cast<C &>(c) };
  }
};

template <i32 From, range_size_t<i32> auto To> using int_range = count_range<i32, From, To>;
template <float From, range_size_t<float> auto To> using float_range = count_range<float, From, To>;
template <u32 From, range_size_t<u32> auto To> using u32_range = count_range<u32, From, To>;
template <u64 From, range_size_t<u64> auto To> using u64_range = count_range<u64, From, To>;
template <i64 From, range_size_t<i64> auto To> using i64_range = count_range<i64, From, To>;

namespace ranges
{

struct _begin_fn {
  template <typename R>
    requires requires(R &r) { r.begin(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.begin())) -> decltype(r.begin())
  {
    return r.begin();
  }

  template <typename T, usize N>
  constexpr T *
  operator()(T (&arr)[N]) const noexcept
  {
    return arr;
  }
};

inline constexpr _begin_fn begin{};

struct _end_fn {
  template <typename R>
    requires requires(R &r) { r.end(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.end())) -> decltype(r.end())
  {
    return r.end();
  }

  template <typename T, usize N>
  constexpr T *
  operator()(T (&arr)[N]) const noexcept
  {
    return arr + N;
  }
};

inline constexpr _end_fn end{};

struct _cbegin_fn {
  template <typename R>
    requires requires(const R &r) { r.cbegin(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.cbegin())) -> decltype(r.cbegin())
  {
    return r.cbegin();
  }

  template <typename R>
    requires(!requires(const R &r) { r.cbegin(); }) && requires(const R &r) { r.begin(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.begin())) -> decltype(r.begin())
  {
    return r.begin();
  }

  template <typename T, usize N>
  constexpr const T *
  operator()(const T (&arr)[N]) const noexcept
  {
    return arr;
  }
};

inline constexpr _cbegin_fn cbegin{};

struct _cend_fn {
  template <typename R>
    requires requires(const R &r) { r.cend(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.cend())) -> decltype(r.cend())
  {
    return r.cend();
  }

  template <typename R>
    requires(!requires(const R &r) { r.cend(); }) && requires(const R &r) { r.end(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.end())) -> decltype(r.end())
  {
    return r.end();
  }

  template <typename T, usize N>
  constexpr const T *
  operator()(const T (&arr)[N]) const noexcept
  {
    return arr + N;
  }
};

inline constexpr _cend_fn cend{};

struct _rbegin_fn {
  template <typename R>
    requires requires(R &r) { r.rbegin(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.rbegin())) -> decltype(r.rbegin())
  {
    return r.rbegin();
  }

  template <typename R>
    requires(!requires(R &r) { r.rbegin(); }) && requires(R &r) { r.end(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.end()))
  {
    return micron::reverse_iter<decltype(r.end())>{ r.end() };
  }

  template <typename T, usize N>
  constexpr micron::reverse_iter<T *>
  operator()(T (&arr)[N]) const noexcept
  {
    return micron::reverse_iter<T *>{ arr + N };
  }
};

inline constexpr _rbegin_fn rbegin{};

struct _rend_fn {
  template <typename R>
    requires requires(R &r) { r.rend(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.rend())) -> decltype(r.rend())
  {
    return r.rend();
  }

  template <typename R>
    requires(!requires(R &r) { r.rend(); }) && requires(R &r) { r.begin(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.begin()))
  {
    return micron::reverse_iter<decltype(r.begin())>{ r.begin() };
  }

  template <typename T, usize N>
  constexpr micron::reverse_iter<T *>
  operator()(T (&arr)[N]) const noexcept
  {
    return micron::reverse_iter<T *>{ arr };
  }
};

inline constexpr _rend_fn rend{};

struct _crbegin_fn {
  template <typename R>
    requires requires(const R &r) { r.crbegin(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.crbegin())) -> decltype(r.crbegin())
  {
    return r.crbegin();
  }

  template <typename R>
    requires(!requires(const R &r) { r.crbegin(); }) && requires(const R &r) { r.cend(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.cend()))
  {
    return micron::reverse_iter<decltype(r.cend())>{ r.cend() };
  }

  template <typename T, usize N>
  constexpr micron::reverse_iter<const T *>
  operator()(const T (&arr)[N]) const noexcept
  {
    return micron::reverse_iter<const T *>{ arr + N };
  }
};

inline constexpr _crbegin_fn crbegin{};

struct _crend_fn {
  template <typename R>
    requires requires(const R &r) { r.crend(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.crend())) -> decltype(r.crend())
  {
    return r.crend();
  }

  template <typename R>
    requires(!requires(const R &r) { r.crend(); }) && requires(const R &r) { r.cbegin(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.cbegin()))
  {
    return micron::reverse_iter<decltype(r.cbegin())>{ r.cbegin() };
  }

  template <typename T, usize N>
  constexpr micron::reverse_iter<const T *>
  operator()(const T (&arr)[N]) const noexcept
  {
    return micron::reverse_iter<const T *>{ arr };
  }
};

inline constexpr _crend_fn crend{};

struct _size_fn {
  template <typename R>
    requires requires(const R &r) { r.size(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.size())) -> decltype(r.size())
  {
    return r.size();
  }

  template <typename T, usize N>
  constexpr usize
  operator()(const T (&)[N]) const noexcept
  {
    return N;
  }
};

inline constexpr _size_fn size{};

struct _ssize_fn {
  template <typename R>
    requires requires(const R &r) { r.ssize(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.ssize())) -> decltype(r.ssize())
  {
    return r.ssize();
  }

  template <typename R>
    requires(!requires(const R &r) { r.ssize(); }) && requires(const R &r) { r.size(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.size()))
  {
    using U = decltype(r.size());
    using S = micron::make_signed_t<U>;
    return static_cast<S>(r.size());
  }

  template <typename T, usize N>
  constexpr micron::make_signed_t<usize>
  operator()(const T (&)[N]) const noexcept
  {
    return static_cast<micron::make_signed_t<usize>>(N);
  }
};

inline constexpr _ssize_fn ssize{};

struct _empty_fn {
  template <typename R>
    requires requires(const R &r) { r.empty(); }
  constexpr bool
  operator()(const R &r) const noexcept(noexcept(r.empty()))
  {
    return r.empty();
  }

  template <typename R>
    requires(!requires(const R &r) { r.empty(); }) && requires(const R &r) { r.size(); }
  constexpr bool
  operator()(const R &r) const noexcept(noexcept(r.size()))
  {
    return r.size() == 0;
  }

  template <typename T, usize N>
  constexpr bool
  operator()(const T (&)[N]) const noexcept
  {
    return N == 0;
  }
};

inline constexpr _empty_fn empty{};

struct _data_fn {
  template <typename R>
    requires requires(R &r) { r.data(); }
  constexpr auto
  operator()(R &r) const noexcept(noexcept(r.data())) -> decltype(r.data())
  {
    return r.data();
  }

  template <typename T, usize N>
  constexpr T *
  operator()(T (&arr)[N]) const noexcept
  {
    return arr;
  }
};

inline constexpr _data_fn data{};

struct _cdata_fn {
  template <typename R>
    requires requires(const R &r) { r.cdata(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.cdata())) -> decltype(r.cdata())
  {
    return r.cdata();
  }

  template <typename R>
    requires(!requires(const R &r) { r.cdata(); }) && requires(const R &r) { r.data(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.data())) -> decltype(r.data())
  {
    return r.data();
  }

  template <typename T, usize N>
  constexpr const T *
  operator()(const T (&arr)[N]) const noexcept
  {
    return arr;
  }
};

inline constexpr _cdata_fn cdata{};

struct _reserve_hint_fn {
  template <typename R>
    requires requires(const R &r) { r.reserve_hint(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.reserve_hint())) -> decltype(r.reserve_hint())
  {
    return r.reserve_hint();
  }

  template <typename R>
    requires(!requires(const R &r) { r.reserve_hint(); }) && requires(const R &r) { r.size(); }
  constexpr auto
  operator()(const R &r) const noexcept(noexcept(r.size())) -> decltype(r.size())
  {
    return r.size();
  }

  template <typename T, usize N>
  constexpr usize
  operator()(const T (&)[N]) const noexcept
  {
    return N;
  }
};

inline constexpr _reserve_hint_fn reserve_hint{};

template <typename R> using iterator_t = decltype(micron::ranges::begin(micron::declval<R &>()));

template <typename R> using sentinel_t = decltype(micron::ranges::end(micron::declval<R &>()));

template <typename R> using const_iterator_t = decltype(micron::ranges::cbegin(micron::declval<const R &>()));

template <typename R> using const_sentinel_t = decltype(micron::ranges::cend(micron::declval<const R &>()));

template <typename R>
using range_difference_t = decltype(micron::ranges::begin(micron::declval<R &>()) - micron::ranges::begin(micron::declval<R &>()));

template <typename R> using range_size_type = decltype(micron::ranges::size(micron::declval<const R &>()));

template <typename R> using range_value_t = micron::remove_cvref_t<decltype(*micron::ranges::begin(micron::declval<R &>()))>;

template <typename R> using range_reference_t = decltype(*micron::ranges::begin(micron::declval<R &>()));

template <typename R> using range_const_reference_t = decltype(*micron::ranges::cbegin(micron::declval<const R &>()));

template <typename R> using range_rvalue_reference_t = decltype(micron::move(*micron::ranges::begin(micron::declval<R &>())));

template <typename R> using range_common_reference_t = micron::common_type_t<range_reference_t<R>, range_rvalue_reference_t<R>>;

};     // namespace ranges
};     // namespace micron
