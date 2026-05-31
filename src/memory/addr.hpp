//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

// all functions related to addresses go here

namespace micron
{

template<class T>
void *
voidify(const T *ptr)
{
  using erase_type = micron::remove_cv_t<decltype(ptr)>;
  if ( ptr != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(ptr)));
  else
    return (void *)nullptr;
};

// arbitrary regular pointer -> regular pointer
template<typename T, typename P>
  requires(micron::is_pointer_v<T> && micron::is_pointer_v<micron::remove_reference_t<P>>)
constexpr T
ptr_cast(P p) noexcept
{
  using tgt = micron::remove_pointer_t<T>;
  using sp = micron::remove_pointer_t<micron::remove_reference_t<P>>;
  if constexpr ( micron::is_function_v<tgt> || micron::is_function_v<sp> )
    return reinterpret_cast<T>(p);
  else
    return reinterpret_cast<T>(reinterpret_cast<addr_t>(p));
}

// arbitrary function pointer -> function pointer
template<typename T, typename P>
  requires(micron::is_pointer_v<T> && micron::is_function_v<micron::remove_pointer_t<T>>
           && micron::is_pointer_v<micron::remove_reference_t<P>>
           && micron::is_function_v<micron::remove_pointer_t<micron::remove_reference_t<P>>>)
constexpr T
fn_cast(P p) noexcept
{
  return reinterpret_cast<T>(p);
}

// arbitrary pointer-to-member -> pointer-to-member
template<typename T, typename P>
  requires(micron::is_member_pointer_v<T> && micron::is_member_pointer_v<micron::remove_reference_t<P>>)
constexpr T
mem_ptr_cast(P p) noexcept
{
  return reinterpret_cast<T>(p);
}

template<typename T, typename P>
constexpr T
cast(P &&p) noexcept
{
  using src = micron::remove_reference_t<P>;
  if constexpr ( (micron::is_pointer_v<T> || micron::is_member_pointer_v<T>) && micron::is_null_pointer_v<src> )
    return static_cast<T>(nullptr);
  else if constexpr ( micron::is_convertible_v<src, T> )
    return static_cast<T>(p);      // value conv / cv-add / adjusted upcast
  else if constexpr ( micron::is_pointer_v<T> && micron::is_pointer_v<src> )
    return micron::ptr_cast<T>(p);      // arbitrary ptr->ptr (const-strip, unrelated)
  else if constexpr ( micron::is_member_pointer_v<T> && micron::is_member_pointer_v<src> )
    return micron::mem_ptr_cast<T>(p);      // arbitrary member ptr -> member ptr
  else
    return reinterpret_cast<T>(p);
}

// arbitrary lvalue reference reinterpret -> O&
template<typename O, typename P>
constexpr O &
ref_cast(P &p) noexcept
{
  return *reinterpret_cast<O *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

// const-result form of ref_cast
template<typename O, typename P>
constexpr const O &
cref_cast(const P &p) noexcept
{
  return micron::ref_cast<const O>(p);
}

// const add/strip for references
template<typename T>
constexpr const T &
as_const(T &r) noexcept
{
  return r;
}

template<typename T> void as_const(const T &&) = delete;

template<typename T>
constexpr T &
as_mutable(const T &r) noexcept
{
  return const_cast<T &>(r);
}

template<typename T>
usize
max_cast(T &&t)
{
  return static_cast<usize>(const_cast<micron::remove_const<T>::type>(t));
}

template<typename P>
constexpr addr_t *
real_addr(P &p) noexcept
{
  return reinterpret_cast<addr_t *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename O, typename P>
constexpr O *
real_addr_as(P &p) noexcept
{
  return reinterpret_cast<O *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename P>
constexpr P *
addressof(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename P>
constexpr P *
addr_of(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename P>
  requires(micron::is_pointer_v<P>)
constexpr P *
addr(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename P>
  requires(!micron::is_pointer_v<P>)
constexpr P *
addr(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename P>
constexpr P *
addr(P &&p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template<typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
not_null(T *p)
{
  if ( p == nullptr ) {
  }      // exc<except::memory_error>("not_null was nullptr");
  return p;
}
};      // namespace micron
