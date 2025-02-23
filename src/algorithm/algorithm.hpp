#pragma once

#include "../math/generic.hpp"
#include "../memory/memory.hpp"
#include "../types.hpp"
#include <exception>
#include <type_traits>

namespace micron
{

template <class T>
  requires std::is_object_v<T> && (std::is_floating_point_v<typename T::value_type>)
inline f128 sum(const T &src)
{
  f128 sm = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    sm += static_cast<f128>(src[i]);
  return sm;
}
template <class T>
  requires std::is_object_v<T>
inline umax_t
sum(const T &src)
{
  umax_t sm = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    sm += static_cast<umax_t>(src[i]);
  return sm;
}

template <typename R = f64, class T>
  requires std::is_object_v<T>
inline R
mean(const T &src)
{
  return static_cast<R>(static_cast<R>(sum(src)) / static_cast<R>(src.size()));
}

template <typename R = flong, class T>
  requires std::is_object_v<T>
inline R
geomean(const T &src) 
{
  // will overflow, use with caution
  // TODO: expand beyond 64bit
  //[[deprecated("Likely to overflow.")]]
  R mulsm = static_cast<R>(src[0]);
  for ( size_t i = 1; i < src.size(); i++ )
    mulsm *= static_cast<R>(src[i]);
  return math::powerflong(static_cast<R>(mulsm), static_cast<R>((R)1 / (R)src.size()));
}

template <typename R = flong, class T>
  requires std::is_object_v<T>
inline R
harmonicmean(const T &src)
{
  R recsum = 0.0f;
  for ( size_t i = 0; i < src.size(); i++ )
    recsum += (1 / static_cast<R>(src[i]));
  return static_cast<R>(src.size()) / recsum;
}

template <class T>
  requires std::is_arithmetic_v<T>
void
round(T *__restrict start, T *__restrict end)
{
  for ( ; start != end; start++ )
    *start = math::round(*start);
}
template <class T>
void
round(T &t)
{
  round(t.begin(), t.end());
}

template <class T>
  requires std::is_arithmetic_v<T>
void
ceil(T *__restrict start, T *__restrict end)
{
  for ( ; start != end; start++ )
    *start = math::ceil(*start);
}
template <class T>
void
ceil(T &t)
{
  ceil(t.begin(), t.end());
}
template <class T>
  requires std::is_arithmetic_v<T>
void
floor(T *__restrict start, T *__restrict end)
{
  for ( ; start != end; start++ )
    *start = math::floor(*start);
}
template <class T>
void
floor(T &t)
{
  floor(t.begin(), t.end());
}
template <class T>
  requires std::is_pointer_v<T>
void
reverse(T __restrict start, T __restrict end)
{
  // if even set
  while ( start != end and end > start ) {
    std::remove_pointer_t<T> tmp = *start;
    *start = *end;
    *end = tmp;
    ++start;
    --end;
  }
}

template <class T>
void
reverse(T &arr)
{
  reverse(arr.begin(), arr.end() - 1);
}

template <class T, typename F>
  requires std::is_integral_v<F>
void
transform(const T *start, const T *end, F f)
{
  for ( ; start != end; start++ )
    f(start);
}

template <class T, class P>
const T *
find_last(const T *start, const T *end, const P &f)
{
  const T *fnd = nullptr;
  for ( ; start != end; start++ )
    if ( *start == f )
      fnd = start;
  return fnd;
}
template <class T, class P>
const T *
find(const T *start, const T *end, const P &f)
{
  for ( ; start != end; start++ )
    if ( *start == f )
      return start;
  return nullptr;
}

template <class T, class P>
bool
all_of(const T *first, const T *end, const P &o)
{
  for ( ; first != end; first++ )
    if ( *first != o )
      return false;
  return true;
}
template <class T, class P>
bool
any_of(const T *first, const T *end, const P &o)
{
  for ( ; first != end; first++ )
    if ( *first == o )
      return true;
  return false;
}
template <class T, class P>
void
fill(const T *first, const T *end, const P &o)
{
  if constexpr ( std::is_class<T>::value ) {
    for ( ; first != end; first++ )
      *first = o;
  } else {     // if not a class, no deep copying need, just blitz
    micron::memset(first, o, end - first);
  }
}
template <class T, typename F>
  requires std::is_integral_v<F>
void
generate(const T *first, const T *end, F f)
{
  for ( ; first != end; first++ )
    *first = f();
}
template <class T>
T::value_type
max(const T &arr)
{
  const auto *first = arr.cbegin();
  const auto *end = arr.cend();
  typename T::value_type max_v = *first++;
  for ( ; first != end; first++ )
    if ( *first > max_v )
      max_v = *first;
  return max_v;
}
template <class T>
T::value_type
min(const T &arr)
{
  const auto *first = arr.cbegin();
  const auto *end = arr.cend();
  typename T::value_type min = *first++;
  for ( ; first != end; first++ )
    if ( *first < min )
      min = *first;
  return min;
}
template <class T>
T
max(const T *first, const T *end)
{
  T max_v = *first++;
  for ( ; first != end; first++ )
    if ( *first > max_v )
      max_v = *first;
  return max_v;
}
template <class T>
T
min(const T *first, const T *end)
{
  T min = *first++;
  for ( ; first != end; first++ )
    if ( *first < min )
      min = *first;
  return min;
}
};     // namespace micron
