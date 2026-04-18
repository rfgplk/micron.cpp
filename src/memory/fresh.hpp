// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

// fresh
// used for initializing contiguous memory
// standard compliant for object model correctness
// currently just calls into __impl_container

#include "../bits/__container.hpp"
#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "actions.hpp"
#include "cmemory.hpp"

namespace micron
{

namespace __impl_fresh
{

template <typename C> auto __elem_deduce(C &c) -> micron::remove_reference_t<decltype(*c.begin())>;

template <typename C> using elem_t = decltype(__elem_deduce(micron::declval<C &>()));

};     // namespace __impl_fresh

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shallow_copys
// POD memcpys in place

template <usize N, typename T>
inline void
fresh_shallow_copy(T *__restrict dest, const T *__restrict src) noexcept
{
  __impl_container::shallow_copy<N, T>(dest, src);
}

template <usize N, typename T>
inline void
fresh_shallow_copy(T *__restrict dest, T *__restrict src) noexcept
{
  __impl_container::shallow_copy<N, T>(dest, src);
}

template <typename T>
inline void
fresh_shallow_copy(T *__restrict dest, const T *__restrict src, usize cnt) noexcept
{
  __impl_container::shallow_copy(dest, src, cnt);
}

template <typename T>
inline void
fresh_shallow_copy(T *__restrict dest, T *__restrict src, usize cnt) noexcept
{
  __impl_container::shallow_copy(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_shallow_copy(D &dest, const S &src) noexcept
{
  __impl_container::shallow_copy(dest.data(), src.data(), src.size());
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_shallow_copy(D &dest, S &src) noexcept
{
  __impl_container::shallow_copy(dest.data(), src.data(), src.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deep copies
// goes through placement new

template <usize N, typename T>
inline void
fresh_deep_copy(T *__restrict dest, T *__restrict src)
{
  __impl_container::deep_copy<N, T>(dest, src);
}

template <usize N, typename T>
inline void
fresh_deep_copy(T *__restrict dest, const T *__restrict src)
{
  __impl_container::deep_copy<N, T>(dest, src);
}

template <typename T>
inline void
fresh_deep_copy(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::deep_copy(dest, src, cnt);
}

template <typename T>
inline void
fresh_deep_copy(T *__restrict dest, const T *__restrict src, usize cnt)
{
  __impl_container::deep_copy(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_deep_copy(D &dest, const S &src)
{
  auto dit = dest.begin();
  auto sit = src.cbegin();
  using T = __impl_fresh::elem_t<D>;
  for ( ; dit != dest.end() && sit != src.cend(); ++dit, ++sit ) new (micron::addr(*dit)) T(*sit);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_deep_copy(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  using T = __impl_fresh::elem_t<D>;
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) new (micron::addr(*dit)) T(*sit);
}

template <is_iterable D, is_iterable S>
  requires(!is_iterable_container<D> || !is_iterable_container<S>)
inline void
fresh_deep_copy(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  using T = __impl_fresh::elem_t<D>;
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) new (micron::addr(*dit)) T(*sit);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deep_copy_assign
// must be initialized memory!

template <usize N, typename T>
inline void
fresh_deep_copy_assign(T *__restrict dest, T *__restrict src)
{
  __impl_container::deep_copy_assign<N, T>(dest, src);
}

template <usize N, typename T>
inline void
fresh_deep_copy_assign(T *__restrict dest, const T *__restrict src)
{
  __impl_container::deep_copy_assign<N, T>(dest, src);
}

template <typename T>
inline void
fresh_deep_copy_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::deep_copy_assign(dest, src, cnt);
}

template <typename T>
inline void
fresh_deep_copy_assign(T *__restrict dest, const T *__restrict src, usize cnt)
{
  __impl_container::deep_copy_assign(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_deep_copy_assign(D &dest, const S &src)
{
  auto dit = dest.begin();
  auto sit = src.cbegin();
  for ( ; dit != dest.end() && sit != src.cend(); ++dit, ++sit ) *dit = *sit;
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_deep_copy_assign(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) *dit = *sit;
}

template <is_iterable D, is_iterable S>
  requires(!is_iterable_container<D> || !is_iterable_container<S>)
inline void
fresh_deep_copy_assign(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) *dit = *sit;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shallow_move
// memcpies in place POD

template <usize N, typename T>
inline void
fresh_shallow_move(T *__restrict dest, T *__restrict src)
{
  __impl_container::shallow_move<N, T>(dest, src);
}

template <typename T>
inline void
fresh_shallow_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::shallow_move(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_shallow_move(D &dest, S &src)
{
  __impl_container::shallow_move(dest.data(), src.data(), src.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deep_moves
// placement new

template <usize N, typename T>
inline void
fresh_deep_move(T *__restrict dest, T *__restrict src)
{
  __impl_container::deep_move<N, T>(dest, src);
}

template <typename T>
inline void
fresh_deep_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::deep_move(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_deep_move(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  using T = __impl_fresh::elem_t<D>;
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) {
    new (micron::addr(*dit)) T(micron::move(*sit));
    sit->~T();
  }
}

template <is_iterable D, is_iterable S>
  requires(!is_iterable_container<D> || !is_iterable_container<S>)
inline void
fresh_deep_move(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  using T = __impl_fresh::elem_t<D>;
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) {
    new (micron::addr(*dit)) T(micron::move(*sit));
    sit->~T();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%
// deep_move_assign
// must be initialized!

template <usize N, typename T>
inline void
fresh_deep_move_assign(T *__restrict dest, T *__restrict src)
{
  __impl_container::deep_move_assign<N, T>(dest, src);
}

template <typename T>
inline void
fresh_deep_move_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::deep_move_assign(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_deep_move_assign(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) *dit = micron::move(*sit);
}

template <is_iterable D, is_iterable S>
  requires(!is_iterable_container<D> || !is_iterable_container<S>)
inline void
fresh_deep_move_assign(D &dest, S &src)
{
  auto dit = dest.begin();
  auto sit = src.begin();
  for ( ; dit != dest.end() && sit != src.end(); ++dit, ++sit ) *dit = micron::move(*sit);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_copy

template <usize N, typename T>
inline void
fresh_copy(T *__restrict dest, T *__restrict src)
{
  __impl_container::copy<N, T>(dest, src);
}

template <usize N, typename T>
inline void
fresh_copy(T *__restrict dest, const T *__restrict src)
{
  __impl_container::copy<N, T>(dest, src);
}

template <typename T>
inline void
fresh_copy(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::copy(dest, src, cnt);
}

template <typename T>
inline void
fresh_copy(T *__restrict dest, const T *__restrict src, usize cnt)
{
  __impl_container::copy(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_copy(D &dest, const S &src)
{
  __impl_container::copy(dest.data(), src.data(), src.size());
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_copy(D &dest, S &src)
{
  __impl_container::copy(dest.data(), src.data(), src.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_copy_assign

template <usize N, typename T>
inline void
fresh_copy_assign(T *__restrict dest, T *__restrict src)
{
  __impl_container::copy_assign<N, T>(dest, src);
}

template <usize N, typename T>
inline void
fresh_copy_assign(T *__restrict dest, const T *__restrict src)
{
  __impl_container::copy_assign<N, T>(dest, src);
}

template <typename T>
inline void
fresh_copy_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::copy_assign(dest, src, cnt);
}

template <typename T>
inline void
fresh_copy_assign(T *__restrict dest, const T *__restrict src, usize cnt)
{
  __impl_container::copy_assign(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_copy_assign(D &dest, const S &src)
{
  __impl_container::copy_assign(dest.data(), src.data(), src.size());
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_copy_assign(D &dest, S &src)
{
  __impl_container::copy_assign(dest.data(), src.data(), src.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_move

template <usize N, typename T>
inline void
fresh_move(T *__restrict dest, T *__restrict src)
{
  __impl_container::move<N, T>(dest, src);
}

template <typename T>
inline void
fresh_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::move(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_move(D &dest, S &src)
{
  __impl_container::move(dest.data(), src.data(), src.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_move_assign

template <usize N, typename T>
inline void
fresh_move_assign(T *__restrict dest, T *__restrict src)
{
  __impl_container::move_assign<N, T>(dest, src);
}

template <typename T>
inline void
fresh_move_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  __impl_container::move_assign(dest, src, cnt);
}

template <is_iterable_container D, is_iterable_container S>
inline void
fresh_move_assign(D &dest, S &src)
{
  __impl_container::move_assign(dest.data(), src.data(), src.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_destroy

template <usize N, typename T>
inline void
fresh_destroy(T *src)
{
  __impl_container::destroy<N, T>(src);
}

template <typename T>
inline void
fresh_destroy(T *src, usize cnt)
{
  __impl_container::destroy(src, cnt);
}

template <is_iterable_container C>
inline void
fresh_destroy(C &c)
{
  __impl_container::destroy(c.data(), c.size());
}

template <is_iterable C>
  requires(!is_iterable_container<C>)
inline void
fresh_destroy(C &c)
{
  using T = __impl_fresh::elem_t<C>;
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( auto it = c.begin(); it != c.end(); ++it ) it->~T();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_destroy_fast
// (doesnt zero out)

template <usize N, typename T>
inline void
fresh_destroy_fast(T *src)
{
  __impl_container::destroy_fast<N, T>(src);
}

template <typename T>
inline void
fresh_destroy_fast(T *src, usize cnt)
{
  __impl_container::destroy_fast(src, cnt);
}

template <is_iterable_container C>
inline void
fresh_destroy_fast(C &c)
{
  __impl_container::destroy_fast(c.data(), c.size());
}

template <is_iterable C>
  requires(!is_iterable_container<C>)
inline void
fresh_destroy_fast(C &c)
{
  using T = __impl_fresh::elem_t<C>;
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( auto it = c.begin(); it != c.end(); ++it ) it->~T();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_zero

template <usize N, typename T>
inline void
fresh_zero(T *src)
{
  __impl_container::zero<N, T>(src);
}

template <typename T>
inline void
fresh_zero(T *src, usize cnt)
{
  micron::byteset(src, 0x0, cnt * sizeof(T));
}

template <is_iterable_container C>
inline void
fresh_zero(C &c)
{
  using T = __impl_fresh::elem_t<C>;
  micron::byteset(c.data(), 0x0, c.size() * sizeof(T));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_set

template <usize N, typename T>
inline void
fresh_set(T *__restrict src, const T &val)
{
  __impl_container::set<N, T>(src, val);
}

template <typename T>
inline void
fresh_set(T *__restrict src, const T &val, usize cnt)
{
  __impl_container::set(src, val, cnt);
}

template <is_iterable_container C>
inline void
fresh_set(C &c, const __impl_fresh::elem_t<C> &val)
{
  __impl_container::set(c.data(), val, c.size());
}

template <is_iterable C>
  requires(!is_iterable_container<C>)
inline void
fresh_set(C &c, const __impl_fresh::elem_t<C> &val)
{
  for ( auto it = c.begin(); it != c.end(); ++it ) *it = val;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fresh_construct

template <usize N, typename T>
inline void
fresh_construct(T *__restrict src, const T &val)
{
  __impl_container::construct<N, T>(src, val);
}

template <typename T>
inline void
fresh_construct(T *__restrict src, const T &val, usize cnt)
{
  __impl_container::construct(src, val, cnt);
}

template <is_iterable_container C>
inline void
fresh_construct(C &c, const __impl_fresh::elem_t<C> &val)
{
  __impl_container::construct(c.data(), val, c.size());
}

template <is_iterable C>
  requires(!is_iterable_container<C>)
inline void
fresh_construct(C &c, const __impl_fresh::elem_t<C> &val)
{
  using T = __impl_fresh::elem_t<C>;
  for ( auto it = c.begin(); it != c.end(); ++it ) new (micron::addr(*it)) T(val);
}

};     // namespace micron
