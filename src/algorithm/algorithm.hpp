#pragma once

#include "../concepts.hpp"

#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

template <typename T>
constexpr const T &
clamp(const T &v, const T &lo, const T &hi) noexcept
{
  return (v < lo) ? lo : (hi < v) ? hi : v;
}

template <typename T, typename C>
constexpr const T &
clamp(const T &v, const T &lo, const T &hi, C comp) noexcept
{
  return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template <is_iterable_container T>
  requires micron::is_floating_point_v<typename T::value_type>
constexpr f128
sum(const T &src) noexcept
{
  f128 sm = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    sm += static_cast<f128>(src[i]);
  return sm;
}

template <is_iterable_container T>
  requires micron::is_integral_v<typename T::value_type>
constexpr umax_t
sum(const T &src) noexcept
{
  umax_t sm = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    sm += static_cast<umax_t>(src[i]);
  return sm;
}

template <is_iterable_container T, typename R>
  requires(micron::is_integral_v<typename T::value_type> && micron::is_integral_v<R>)
constexpr T &
fill(T &src, const R r) noexcept
{
  for ( auto &n : src )
    n = r;
  return src;
}

template <typename T, class P>
constexpr void
fill(T *first, T *end, const P &o) noexcept
{
  if constexpr ( micron::is_class_v<T> ) {
    for ( ; first != end; ++first )
      *first = o;
  } else {
    constexpr_memset(first, o, end - first);
  }
}

template <typename T, class P>
constexpr T *
fill_n(T *first, size_t n, const P &value) noexcept
{
  for ( size_t i = 0; i < n; ++i, ++first )
    *first = value;
  return first;
}

template <is_iterable_container C, class P>
constexpr C &
fill_n(C &c, size_t n, const P &value) noexcept
{
  fill_n(c.begin(), n, value);
  return c;
}

template <typename T, class P>
constexpr bool
contains(const T *first, const T *end, const P &value) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == value )
      return true;
  return false;
}

template <is_iterable_container C, class P>
constexpr bool
contains(const C &c, const P &value) noexcept
{
  return contains(c.begin(), c.end(), value);
}

template <is_iterable_container T, typename R = typename T::value_type>
constexpr T &
clear(T &src, const R r = 0) noexcept
{
  if constexpr ( micron::is_object_v<micron::remove_cv_t<typename T::value_type>> ) {
    for ( auto &n : src )
      n = r;
  } else if constexpr ( micron::is_fundamental_v<micron::remove_cv_t<typename T::value_type>> ) {
    constexpr_memset(src.begin(), r, src.size());
  }
  return src;
}

template <typename R = f64, typename T>
  requires micron::is_object_v<T>
constexpr R
mean(const T &src) noexcept
{
  return static_cast<R>(sum(src)) / static_cast<R>(src.size());
}

template <typename R = flong, typename T>
  requires micron::is_object_v<T>
constexpr R
geomean(const T &src) noexcept
{
  R mulsm = static_cast<R>(src[0]);
  for ( size_t i = 1; i < src.size(); i++ )
    mulsm *= static_cast<R>(src[i]);
  return math::powerflong(mulsm, static_cast<R>(R(1) / R(src.size())));
}

template <typename R = flong, typename T>
  requires micron::is_object_v<T>
constexpr R
harmonicmean(const T &src) noexcept
{
  R recsum = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    recsum += (R(1) / static_cast<R>(src[i]));
  return static_cast<R>(src.size()) / recsum;
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
round(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start )
    *start = math::round(*start);
}

template <typename T>
constexpr void
round(T &t) noexcept
{
  round(t.begin(), t.end());
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
ceil(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start )
    *start = math::ceil(*start);
}

template <is_iterable_container T>
constexpr T &
ceil(T &t) noexcept
{
  ceil(t.begin(), t.end());
  return t;
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
floor(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start )
    *start = math::floor(*start);
}

template <typename T>
constexpr void
floor(T &t) noexcept
{
  floor(t.begin(), t.end());
}

template <typename T>
  requires micron::is_pointer_v<T>
constexpr void
reverse(T __restrict start, T __restrict end) noexcept
{
  while ( start < end ) {
    auto tmp = *start;
    *start = *end;
    *end = tmp;
    ++start;
    --end;
  }
}

template <typename T>
constexpr void
reverse(T &arr) noexcept
{
  reverse(arr.begin(), arr.end() - 1);
}

template <typename T, typename F>
  requires micron::is_invocable_v<F, T *>
constexpr void
transform(T *start, T *end, F f) noexcept
{
  for ( ; start != end; ++start )
    *start = f(start);
}

template <typename T, typename F>
  requires micron::is_invocable_v<F, T>
constexpr void
transform(T *start, T *end, F f) noexcept
{
  for ( ; start != end; ++start )
    *start = f(*start);
}

template <is_iterable_container C, typename F>
constexpr void
transform(C &c, F f) noexcept
{
  transform(c.begin(), c.end(), f);
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr T *
find_last(T *start, T *end, const P &f) noexcept
{
  T *fnd = nullptr;
  for ( ; start != end; ++start )
    if ( *start == static_cast<T>(f) )
      fnd = start;
  return fnd;
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr const T *
find_last(const T *start, const T *end, const P &f) noexcept
{
  const T *fnd = nullptr;
  for ( ; start != end; ++start )
    if ( *start == static_cast<T>(f) )
      fnd = start;
  return fnd;
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr const T *
find(const T *start, const T *end, const P &f) noexcept
{
  for ( ; start != end; ++start )
    if ( *start == static_cast<T>(f) )
      return start;
  return nullptr;
}

template <typename T, typename F>
  requires micron::is_invocable_v<F> && requires(F f) {
    { f() } -> micron::same_as<T>;
  }
constexpr void
generate(T *first, T *end, F f) noexcept
{
  for ( ; first != end; ++first )
    *first = f();
}

template <typename T, typename F, typename... Args>
  requires micron::is_invocable_v<F, Args...>
constexpr void
generate(T *first, T *end, F f, Args &&...args) noexcept
{
  for ( ; first != end; ++first )
    *first = f(micron::forward<Args>(args)...);
}

template <is_iterable_container C, typename F>
constexpr C &
generate(C &c, F f) noexcept
{
  generate(c.begin(), c.end(), f);
  return c;
}

template <is_iterable_container C, typename F, typename... Args>
constexpr C &
generate(C &c, F f, Args &&...args) noexcept
{
  generate(c.begin(), c.end(), f, micron::forward<Args>(args)...);
  return c;
}

template <typename T>
typename T::const_iterator
max_at(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::const_iterator max_v = it;
  for ( ; it != end; ++it )
    if ( *it > *max_v )
      max_v = it;
  return max_v;
}

template <typename T>
typename T::const_iterator
min_at(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::const_iterator min_v = it;
  for ( ; it != end; ++it )
    if ( *it < *min_v )
      min_v = it;
  return min_v;
}

template <typename T>
typename T::const_iterator
max_at(const T *first, const T *end) noexcept
{
  typename T::const_iterator max_v = first;
  for ( ; first != end; ++first )
    if ( *first > *max_v )
      max_v = first;
  return max_v;
}

template <typename T>
typename T::const_iterator
min_at(const T *first, const T *end) noexcept
{
  typename T::const_iterator min_v = first;
  for ( ; first != end; ++first )
    if ( *first < *min_v )
      min_v = first;
  return min_v;
}

template <typename T>
typename T::value_type
max(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::value_type max_v = *it++;
  for ( ; it != end; ++it )
    if ( *it > max_v )
      max_v = *it;
  return max_v;
}

template <typename T>
typename T::value_type
min(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::value_type min_v = *it++;
  for ( ; it != end; ++it )
    if ( *it < min_v )
      min_v = *it;
  return min_v;
}

template <typename T>
T
max(const T *first, const T *end) noexcept
{
  T max_v = *first++;
  for ( ; first != end; ++first )
    if ( *first > max_v )
      max_v = *first;
  return max_v;
}

template <typename T>
T
min(const T *first, const T *end) noexcept
{
  T min_v = *first++;
  for ( ; first != end; ++first )
    if ( *first < min_v )
      min_v = *first;
  return min_v;
}

template <typename T, class P>
bool
all_of(const T *first, const T *end, const P &o) noexcept
{
  for ( ; first != end; ++first )
    if ( *first != o )
      return false;
  return true;
}

template <typename T, class P>
bool
any_of(const T *first, const T *end, const P &o) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == o )
      return true;
  return false;
}

template <typename T, class P>
bool
none_of(const T *first, const T *end, const P &o) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == o )
      return false;
  return true;
}

template <typename T, class P>
umax_t
count(const T *first, const T *end, const P &o) noexcept
{
  umax_t c = 0;
  for ( ; first != end; ++first )
    if ( *first == o )
      ++c;
  return c;
}

template <typename T, class F>
  requires micron::is_invocable_v<F, T *>
umax_t
count_if(const T *first, const T *end, F f) noexcept
{
  umax_t c = 0;
  for ( ; first != end; ++first )
    if ( f(first) )
      ++c;
  return c;
}

template <typename T, class F>
  requires micron::is_invocable_v<F, T>
umax_t
count_if(const T *first, const T *end, F f) noexcept
{
  umax_t c = 0;
  for ( ; first != end; ++first )
    if ( f(*first) )
      ++c;
  return c;
}

template <typename T>
bool
equal(const T *first1, const T *end1, const T *first2) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2 )
    if ( !(*first1 == *first2) )
      return false;
  return true;
}

template <typename T, class P>
const T *
search(const T *first, const T *end, const P *pfirst, const P *pend) noexcept
{
  for ( ; first != end; ++first ) {
    const T *it1 = first;
    const P *it2 = pfirst;
    while ( it1 != end && it2 != pend && *it1 == *it2 ) {
      ++it1;
      ++it2;
    }
    if ( it2 == pend )
      return first;
  }
  return nullptr;
}

template <typename T, class P>
const T *
search_n(const T *first, const T *end, size_t n, const P &value) noexcept
{
  for ( ; first != end; ++first ) {
    size_t i = 0;
    while ( first + i != end && i < n && *(first + i) == value )
      ++i;
    if ( i == n )
      return first;
  }
  return nullptr;
}

template <typename T, class P>
bool
starts_with(const T *first, const T *end, const P *pfirst, const P *pend) noexcept
{
  while ( pfirst != pend ) {
    if ( first == end || *first != *pfirst )
      return false;
    ++first;
    ++pfirst;
  }
  return true;
}

template <typename T, class P>
bool
ends_with(const T *first, const T *end, const P *pfirst, const P *pend) noexcept
{
  size_t n = pend - pfirst;
  size_t len = end - first;
  if ( n > len )
    return false;
  return equal(first + (len - n), end, pfirst);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::iterator
find(C &c, P &v) noexcept
{
  return find(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::iterator
find_last(C &c, P &v) noexcept
{
  return find_last(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::const_iterator
find(const C &c, const P &v) noexcept
{
  return find(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::const_iterator
find_last(const C &c, const P &v) noexcept
{
  return find_last(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
umax_t
count(const C &c, const P &v) noexcept
{
  return count(c.begin(), c.end(), v);
}

template <is_iterable_container C, class F>
umax_t
count_if(const C &c, F f) noexcept
{
  return count_if(c.begin(), c.end(), f);
}

template <is_iterable_container C1, is_iterable_container C2>
bool
equal(const C1 &a, const C2 &b) noexcept
{
  return equal(a.begin(), a.end(), b.begin());
}

template <is_iterable_container C, is_iterable_container P>
const typename C::value_type *
search(const C &c, const P &p) noexcept
{
  return search(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, class V>
const typename C::value_type *
search_n(const C &c, size_t n, const V &v) noexcept
{
  return search_n(c.begin(), c.end(), n, v);
}

template <is_iterable_container C, class P>
bool
starts_with(const C &c, const P &p) noexcept
{
  return starts_with(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, class P>
bool
ends_with(const C &c, const P &p) noexcept
{
  return ends_with(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, class P>
bool
all_of(const C &c, const P &v) noexcept
{
  return all_of(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
bool
any_of(const C &c, const P &v) noexcept
{
  return any_of(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
bool
none_of(const C &c, const P &v) noexcept
{
  return none_of(c.begin(), c.end(), v);
}
}     // namespace micron
