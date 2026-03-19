//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "memory/addr.hpp"
#include "memory/ptrs.hpp"

namespace micron
{

using count_t = size_t;

// ptr aliases
// ptr<T>         : default single-object owning pointer; general unique ownership
// ptr_arr<T>     : owning pointer to a heap-allocated array of T
// atom_ptr<T>    : unique ownership with atomic load/store/exchange; use when the pointer itself is shared across threads
// atom_ptr_arr<T>: atomic owning pointer to a heap-allocated array of T
// uptr<T>        : alias for ptr<T>
// sptr<T>        : shared ownership across multiple owners; use when lifetime is non-deterministic
// fptr<T>        : non-owning borrow of a raw pointer; use when lifetime is guaranteed by the caller
// wptr<T>        : non-owning observer of a shared_pointer that does not extend lifetime; use to break cycles
// cptr<T>        : immutable owning pointer; value cannot be changed after construction
// cptr_arr<T>    : immutable owning pointer to a heap-allocated array of T
// gptr<T>        : global-scope owning pointer; lives for the entire binary execution, never frees on destruction
// gptr_arr<T>    : global-scope owning pointer to a heap-allocated array of T
// pointer<T>     : semantic alias for ptr<T>; use in APIs where "pointer" reads more naturally than "ptr"
// pointer_arr<T> : semantic alias for ptr_arr<T>
// shared<T>      : semantic alias for sptr<T>; use in APIs where "shared" reads more naturally

template <typename T> using ptr = unique_pointer<T>;
template <typename T> using ptr_arr = unique_pointer<T[]>;
template <typename T> using atom_ptr = atomic_pointer<T>;
template <typename T> using atom_ptr_arr = atomic_pointer<T[]>;
template <typename T> using uptr = unique_pointer<T>;
template <typename T> using sptr = shared_pointer<T>;
template <typename T> using fptr = free_pointer<T>;
template <typename T> using wptr = weak_pointer<T>;
template <typename T> using cptr = const_pointer<T>;
template <typename T> using cptr_arr = const_pointer<T[]>;
template <typename T> using gptr = __global_pointer<T>;
template <typename T> using gptr_arr = __global_pointer<T[]>;
template <typename T> using pointer = unique_pointer<T>;
template <typename T> using pointer_arr = unique_pointer<T[]>;
template <typename T> using shared = shared_pointer<T>;

template <is_pointer_class O, is_pointer_class P>
bool
operator==(const O &o, const P &p) noexcept
{
  return o.get() == p.get();
}

template <is_pointer_class O, is_pointer_class P>
bool
operator!=(const O &o, const P &p) noexcept
{
  return o.get() != p.get();
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

template <is_pointer_class O>
bool
operator==(const O &o, nullptr_t) noexcept
{
  return o.get() == nullptr;
}

template <is_pointer_class O>
bool
operator!=(const O &o, nullptr_t) noexcept
{
  return o.get() != nullptr;
}

template <class T>
gptr<T>
make_global()
{
  return gptr<T>();
}

template <class T, class... Args>
  requires(sizeof...(Args) > 0)
gptr<T>
make_global(Args &&...x)
{
  return gptr<T>(micron::forward<Args>(x)...);
}

template <class T, class... Args>
  requires(sizeof...(Args) > 0)
gptr_arr<T>
make_global_arr(Args &&...x)
{
  return gptr_arr<T>(micron::forward<Args>(x)...);
}

template <class T>
pointer<T>
unique()
{
  return pointer<T>();
}

template <class T, class F>
pointer<T>
unique(std::initializer_list<F> list)
{
  return pointer<T>(list);
}

template <class T, class... Args>
  requires(sizeof...(Args) > 0)
pointer<T>
unique(Args &&...x)
{
  return pointer<T>(micron::forward<Args>(x)...);
}

template <class T, class Y>
pointer<T>
unique(Y &&ptr)
{
  return pointer<T>(micron::forward<Y>(ptr));
}

template <class T>
pointer_arr<T>
unique_arr()
{
  return pointer_arr<T>();
}

template <class T, class F>
pointer_arr<T>
unique_arr(std::initializer_list<F> list)
{
  return pointer_arr<T>(list);
}

template <class T>
pointer_arr<T>
unique_arr(size_t n)
{
  return pointer_arr<T>(n);
}

template <class T, class... Args>
  requires(sizeof...(Args) > 0)
pointer_arr<T>
unique_arr(Args &&...x)
{
  return pointer_arr<T>(micron::forward<Args>(x)...);
}

template <class T, class Y>
pointer_arr<T>
unique_arr(Y &&ptr)
{
  return pointer_arr<T>(micron::forward<Y>(ptr));
}

template <class T>
atom_ptr<T>
make_atomic()
{
  return atom_ptr<T>();
}

template <class T, class... Args>
  requires(sizeof...(Args) > 0)
atom_ptr<T>
make_atomic(Args &&...x)
{
  return atom_ptr<T>(micron::forward<Args>(x)...);
}

template <class T, class... Args>
  requires(sizeof...(Args) > 0)
atom_ptr_arr<T>
make_atomic_arr(Args &&...x)
{
  return atom_ptr_arr<T>(micron::forward<Args>(x)...);
}

template <template <typename> class P, typename T>
  requires is_pointer_class<P<T>>
P<T>
as_pointer()
{
  return P<T>();
}

template <template <typename> class P, typename T, class F>
  requires is_pointer_class<P<T>>
P<T>
as_pointer(std::initializer_list<F> list)
{
  return P<T>(list);
}

template <template <typename> class P, typename T, class... Args>
  requires is_pointer_class<P<T>>
P<T>
as_pointer(Args &&...x)
{
  return P<T>(micron::forward<Args>(x)...);
}

template <template <typename> class P, typename T, class Y>
  requires is_pointer_class<P<T>>
P<T>
as_pointer(Y &&ptr)
{
  return P<T>(micron::forward<Y>(ptr));
}

template <class T>
void
swap_ptr(pointer<T> &a, pointer<T> &b) noexcept
{
  a.swap(b);
}

template <class T>
void
swap_ptr(pointer_arr<T> &a, pointer_arr<T> &b) noexcept
{
  a.swap(b);
}

template <class T>
void
swap_ptr(atom_ptr<T> &a, atom_ptr<T> &b) noexcept
{
  a.swap(b);
}

template <is_pointer_class P>
auto
to_address(const P &p) noexcept -> decltype(p.get())
{
  return p.get();
}

template <typename T>
T *
to_address(T *p) noexcept
{
  return p;
}

template <class T>
void *
voidify(const pointer<T> &pnt)
{
  using erase_type = micron::remove_cv_t<decltype(pnt())>;
  if ( pnt() != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(pnt())));
  else
    return (void *)nullptr;
}

template <class T, size_t N>
void *
voidify(const pointer<T[N]> &pnt)
{
  using erase_type = micron::remove_cv_t<decltype(pnt())>;
  if ( pnt() != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(pnt())));
  else
    return (void *)nullptr;
}

template <is_pointer_class P>
void *
voidify(const P &pnt) noexcept
{
  return pnt.get() ? const_cast<void *>(static_cast<const void *>(pnt.get())) : nullptr;
}

template <typename To, is_pointer_class P>
To *
pointer_cast(const P &p) noexcept
{
  return reinterpret_cast<To *>(p.get());
}

template <is_pointer_class P>
bool
is_null(const P &p) noexcept
{
  return p.get() == nullptr;
}

template <typename T>
bool
is_null(T *p) noexcept
{
  return p == nullptr;
}

template <is_pointer_class P>
bool
is_active(const P &p) noexcept
{
  return p.get() != nullptr;
}

};     // namespace micron
