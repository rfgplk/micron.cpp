//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "memory/addr.hpp"
#include "memory/pointers/constant.hpp"
#include "memory/pointers/free.hpp"
// #include "memory/pointers/hazard.hpp"
#include "memory/pointers/shared.hpp"
#include "memory/pointers/thread.hpp"
#include "memory/pointers/unique.hpp"
#include "memory/pointers/void.hpp"
#include "memory/pointers/weak.hpp"

namespace micron
{
using count_t = size_t;

// range constructor
// overseer

template <typename T> using ptr = unique_pointer<T>;
template <typename T> using uptr = unique_pointer<T>;
template <typename T> using sptr = shared_pointer<T>;
template <typename T> using fptr = free_pointer<T>;
template <typename T> using wptr = weak_pointer<T>;
template <typename T> using cptr = const_pointer<T>;
template <typename T> using pointer = unique_pointer<T>;
template <typename T> using shared = shared_pointer<T>;
// using hazard = hazard_pointer;

template <is_pointer_class O, is_pointer_class P>
bool
operator==(const O &o, const P &p) noexcept
{
  return o.get() == p.get();
}
template <is_pointer_class O, is_pointer_class P>
bool
operator>(const O &o, const P &p) noexcept
{
  return o.get() > p.get();
}
template <is_pointer_class O, is_pointer_class P>
bool
operator<(const O &o, const P &p) noexcept
{
  return o.get() < p.get();
}
template <is_pointer_class O, is_pointer_class P>
bool
operator<=(const O &o, const P &p) noexcept
{
  return o.get() <= p.get();
}
template <is_pointer_class O, is_pointer_class P>
bool
operator>=(const O &o, const P &p) noexcept
{
  return o.get() >= p.get();
}

template <class T>
pointer<T>
unique()
{
  return pointer<T>();
};
template <class T, class F>
pointer<T>
unique(std::initializer_list<F> list)
{
  return pointer<T>(list);
};
template <class T, class... Args>
pointer<T>
unique(Args &&...x)
{
  return pointer<T>(x...);
};
template <class T, class Y>
pointer<T>
unique(Y &&ptr)
{
  return pointer<T>(micron::forward<Y>(ptr));
};

template <template <is_pointer_class> class P, typename T>
P<T>
as_pointer()
{
  return P<T>();
};
template <template <is_pointer_class> class P, typename T, class F>
P<T>
as_pointer(std::initializer_list<F> list)
{
  return P<T>(list);
};
template <template <is_pointer_class> class P, typename T, class... Args>
P<T>
as_pointer(Args &&...x)
{
  return P<T>(x...);
};
template <template <is_pointer_class> class P, typename T, class Y>
P<T>
as_pointer(Y &&ptr)
{
  return P<T>(ptr);
};

template <class T>
void *
voidify(const pointer<T> &pnt)
{
  using erase_type = micron::remove_cv_t<decltype(pnt())>;
  if ( pnt() != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(pnt())));
  else
    return (void *)nullptr;
};

template <class T, size_t N>
void *
voidify(const pointer<T[N]> &pnt)
{
  using erase_type = micron::remove_cv_t<decltype(pnt())>;
  if ( pnt() != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(pnt())));
  else
    return (void *)nullptr;
};
};     // namespace micron
