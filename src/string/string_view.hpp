//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../compare.hpp"
#include "../except.hpp"
#include "../type_traits.hpp"

#include "fixed_string.hpp"
#include "unitypes.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// string_view<S>
//
// a non-owning (begin, end) pair over an existing S
namespace micron
{

template<is_string S> class string_view
{
  using T = typename S::const_pointer;
  T __start;
  T __end;

  template<typename U>
  static constexpr T
  __as(U p) noexcept
  {
    if constexpr ( micron::is_same_v<U, T> )
      return p;
    else
      return reinterpret_cast<T>(p);
  }

public:
  using value_type = micron::remove_cvref_t<decltype(*micron::declval<T>())>;
  using const_pointer = T;
  using const_iterator = T;
  using size_type = usize;

  constexpr ~string_view() = default;

  constexpr string_view() = delete;

  constexpr string_view(T s) noexcept : __start(s), __end(s + micron::strlen(s)) { }

  constexpr string_view(T s, T e) noexcept : __start(s), __end(e) { }

  template<is_string F>
    requires(!micron::is_same_v<F, S>)
  constexpr string_view(typename F::iterator a, typename F::iterator b) noexcept : __start(__as(a)), __end(__as(b))
  {
    static_assert(sizeof(micron::remove_pointer_t<typename F::iterator>) == sizeof(micron::remove_pointer_t<typename S::iterator>),
                  "micron::string_view cross-type ctor requires equal element width");
  }

  constexpr string_view(const char *ptr, usize count) noexcept : __start(ptr), __end(ptr + count) { }

  template<is_string F> constexpr string_view(const F &f) noexcept : __start(__as(f.cbegin())), __end(__as(f.cend())) { }

  template<is_string F> constexpr string_view(const F &f, const usize n) : __start(__as(f.cbegin())), __end(__start)
  {
    if ( n > f.size() ) exc<except::library_error>("micron::string_view set() out of memory range");
    __end = __as(f.cbegin() + n);
  }

  constexpr string_view(const string_view &o) noexcept : __start(o.__start), __end(o.__end) { }

  constexpr string_view(string_view &&) = delete;

  constexpr string_view &
  operator=(const string_view &o) noexcept
  {
    __start = o.__start;
    __end = o.__end;
    return *this;
  }

  constexpr string_view &
  operator=(const S &o) noexcept
  {
    if ( o.empty() ) return *this;
    __start = o.cbegin();
    __end = o.cend();
    return *this;
  }

  constexpr string_view &
  set(const S &o, const usize n = 0)
  {
    if ( o.empty() ) return *this;
    if ( n >= o.size() ) exc<except::library_error>("micron::string_view set() out of memory range");
    __start = o.cbegin() + n;
    __end = o.cend();
    return *this;
  }

  constexpr string_view &
  advance(const usize n)
  {
    if ( n >= static_cast<usize>(__end - __start) ) exc<except::library_error>("micron::string_view advance() out of memory range");
    __start += n;
    return *this;
  }

  constexpr string_view &
  __advance(const usize n) noexcept
  {
    __start += n;
    return *this;
  }

  constexpr string_view &
  __move(const usize n) noexcept
  {
    __start += n;
    __end += n;
    return *this;
  }

  constexpr string_view &
  __push(const usize n) noexcept
  {
    __end += n;
    return *this;
  }

  constexpr const auto &
  operator[](const usize n) const noexcept
  {
    return __start[n];
  }

  constexpr const auto &
  at(const usize n) const
  {
    if ( n >= static_cast<usize>(__end - __start) ) exc<except::library_error>("micron::string_view at() out of memory range");
    return __start[n];
  }

  constexpr T
  ptr(const usize n) const noexcept
  {
    return __start + n;
  }

  constexpr const T
  begin() const noexcept
  {
    return __start;
  }

  constexpr const T
  end() const noexcept
  {
    return __end;
  }

  constexpr const T
  cbegin() const noexcept
  {
    return __start;
  }

  constexpr const T
  cend() const noexcept
  {
    return __end;
  }

  constexpr const auto &
  front() const noexcept
  {
    return *__start;
  }

  constexpr const auto &
  last() const
  {
    if ( __end == __start ) exc<except::library_error>("micron::string_view last() on empty view");
    return *(__end - 1);
  }

  constexpr usize
  size() const noexcept
  {
    return static_cast<usize>(__end - __start);
  }

  constexpr bool
  empty() const noexcept
  {
    return __end == __start;
  }

  constexpr const T
  data() const noexcept
  {
    return __start;
  }

  constexpr string_view
  substr(const usize a, const usize b) const
  {
    if ( a > b || b > static_cast<usize>(__end - __start) ) exc<except::library_error>("micron::string_view substr() out of memory range");
    return string_view(__start + a, __start + b);
  }

  constexpr string_view
  substr(const usize a) const
  {
    if ( a >= static_cast<usize>(__end - __start) ) exc<except::library_error>("micron::string_view substr() out of memory range");
    return string_view(__start + a, __end);
  }

  constexpr int
  compare(const value_type *p, usize n) const noexcept
  {
    return __fs_lexcmp(__start, size(), p, n);
  }

  constexpr std::strong_ordering
  operator<=>(const string_view &o) const noexcept
  {
    return __fs_ord(__fs_lexcmp(__start, size(), o.__start, o.size()));
  }

  constexpr bool
  operator==(const string_view &o) const noexcept
  {
    return size() == o.size() && __fs_lexcmp(__start, size(), o.__start, o.size()) == 0;
  }

  constexpr std::strong_ordering
  operator<=>(const value_type *s) const noexcept
    requires(micron::is_same_v<value_type, char>)
  {
    return __fs_ord(compare(s, micron::strlen(s)));
  }

  constexpr bool
  operator==(const value_type *s) const noexcept
    requires(micron::is_same_v<value_type, char>)
  {
    return compare(s, micron::strlen(s)) == 0;
  }

  template<usize N>
  constexpr std::strong_ordering
  operator<=>(const fixed_string<N> &o) const noexcept
    requires(micron::is_same_v<value_type, char>)
  {
    return __fs_ord(compare(o.buf, o.len()));
  }

  template<usize N>
  constexpr bool
  operator==(const fixed_string<N> &o) const noexcept
    requires(micron::is_same_v<value_type, char>)
  {
    return compare(o.buf, o.len()) == 0;
  }

  template<is_string F>
  constexpr std::strong_ordering
  operator<=>(const F &f) const
    requires(micron::is_same_v<value_type, char>)
  {
    return __fs_ord(compare(f.c_str(), f.size()));
  }

  template<is_string F>
  constexpr bool
  operator==(const F &f) const
    requires(micron::is_same_v<value_type, char>)
  {
    return compare(f.c_str(), f.size()) == 0;
  }
};

template<is_string S> using cstring_view = string_view<S>;

};      // namespace micron
