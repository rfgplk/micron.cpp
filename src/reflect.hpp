//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"

#if defined(__micron_reflection)

#include "meta.hpp"

#include "concepts.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "array/constexprarray.hpp"
#include "string/fixed_string.hpp"
#include "tuple.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// micron::reflect
//
// NOTE:
//   .. an info is consteval-only, so it can never be a function *argument* at runtime
//   .. field order is declaration order, matching the aggregate initializer
//   .. a type that already publishes the tuple protocol keeps
//      using it, and only a true aggregate falls through to reflection

namespace micron
{
namespace reflect
{

// concepts
template<typename T>
concept reflectable = micron::is_class_v<micron::remove_cvref_t<T>> || micron::is_union_v<micron::remove_cvref_t<T>>;

template<typename T>
concept reflectable_enum = micron::is_enum_v<micron::remove_cvref_t<T>>;

template<typename T>
concept reflectable_aggregate = reflectable<T> && micron::is_aggregate_v<micron::remove_cvref_t<T>>;

template<typename T>
inline constexpr auto members = micron::meta::define_static_array(
    micron::meta::nonstatic_data_members_of(^^micron::remove_cvref_t<T>, micron::meta::access_context::unchecked()));

template<typename T> inline constexpr usize field_count = members<T>.len;

template<typename T>
inline constexpr auto bases
    = micron::meta::define_static_array(micron::meta::bases_of(^^micron::remove_cvref_t<T>, micron::meta::access_context::unchecked()));

template<typename T> inline constexpr usize base_count = bases<T>.len;

// %%%%%%%%%%%%%%%%%%%%%%%
// names

template<micron::meta::info M>
consteval auto
name_of(void)
{
  constexpr auto __s = micron::meta::identifier_of(M);
  return micron::fixed_string<__s.size() + 1>{ __s.data(), __s.size() };
}

template<micron::meta::info M>
consteval micron::meta::identifier
name_view(void)
{
  return micron::meta::identifier_of(M);
}

template<typename T>
consteval auto
type_name(void)
{
  constexpr auto __s = micron::meta::identifier_of(micron::meta::dealias(^^micron::remove_cvref_t<T>));
  return micron::fixed_string<__s.size() + 1>{ __s.data(), __s.size() };
}

template<typename T>
consteval micron::meta::identifier
type_name_view(void)
{
  return micron::meta::identifier_of(micron::meta::dealias(^^micron::remove_cvref_t<T>));
}

template<typename T>
consteval micron::meta::identifier
type_display(void)
{
  return micron::meta::display_string_of(micron::meta::dealias(^^micron::remove_cvref_t<T>));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// member pointers
//
// NOTE: the paper spells this &[: ^^T :]::[: M :] yet gcc 16 does not parse a splice after ::
// (expected unqualified-id), using &[: M :] instead

template<micron::meta::info M>
consteval auto
member_ptr(void)
{
  return &[:M:];
}

template<micron::meta::info M> using member_type = [:micron::meta::type_of(M):];

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// field iteration

template<typename T, typename Fn>
constexpr void
for_each_field(Fn &&__fn)
{
  template for ( constexpr auto __m : members<T> ) __fn.template operator()<__m>();
}

template<typename T, typename Fn>
constexpr void
for_each_field(T &&__obj, Fn &&__fn)
{
  template for ( constexpr auto __m : members<micron::remove_cvref_t<T>> ) __fn(name_of<__m>(), __obj.[:__m:]);
}

template<typename T, typename Fn>
constexpr void
for_each_field_indexed(T &&__obj, Fn &&__fn)
{
  usize __i = 0;
  template for ( constexpr auto __m : members<micron::remove_cvref_t<T>> )
  {
    __fn(__i, name_of<__m>(), __obj.[:__m:]);
    ++__i;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// field access by ordinal

// must be decltype
template<usize I, typename T>
constexpr decltype(auto)
get_field(T &&__obj) noexcept
{
  return (__obj.[:members<micron::remove_cvref_t<T>>.ptr[I]:]);
}

template<usize I, typename T> using field_type = [:micron::meta::type_of(members<micron::remove_cvref_t<T>>.ptr[I]):];

template<usize I, typename T>
consteval auto
field_name(void)
{
  return name_of<members<micron::remove_cvref_t<T>>.ptr[I]>();
}

template<usize I, typename T>
consteval usize
field_offset(void)
{
  return static_cast<usize>(micron::meta::offset_of(members<micron::remove_cvref_t<T>>.ptr[I]).bytes);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// name -> ordinal

template<typename T>
consteval usize
index_of(micron::meta::identifier __name)
{
  usize __i = 0;
  usize __found = static_cast<usize>(-1);
  template for ( constexpr auto __m : members<T> )
  {
    if ( micron::meta::identifier_of(__m) == __name ) __found = __i;
    ++__i;
  }
  return __found;
}

template<typename T>
consteval bool
has_field(micron::meta::identifier __name)
{
  return index_of<T>(__name) != static_cast<usize>(-1);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// enums

template<typename E>
  requires reflectable_enum<E>
inline constexpr auto enumerators = micron::meta::define_static_array(micron::meta::enumerators_of(^^micron::remove_cvref_t<E>));

template<typename E>
  requires reflectable_enum<E>
inline constexpr usize enum_count = enumerators<E>.len;

template<typename E>
  requires reflectable_enum<E>
consteval auto
enum_values(void)
{
  micron::constexpr_array<micron::remove_cvref_t<E>, enum_count<E>> __out{};
  usize __i = 0;
  template for ( constexpr auto __e : enumerators<E> ) __out[__i++] = [:__e:];
  return __out;
}

template<typename E>
  requires reflectable_enum<E>
constexpr micron::meta::identifier
enum_name(E __v) noexcept
{
  template for ( constexpr auto __e : enumerators<E> )
  {
    if ( __v == [:__e:] ) return micron::meta::identifier_of(__e);
  }
  return micron::meta::identifier{};
}

template<typename E>
  requires reflectable_enum<E>
constexpr bool
enum_from_name(micron::meta::identifier __name, E &__out) noexcept
{
  template for ( constexpr auto __e : enumerators<E> )
  {
    if ( micron::meta::identifier_of(__e) == __name ) {
      __out = [:__e:];
      return true;
    }
  }
  return false;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// aggregate <-> tuple

namespace __impl
{

template<typename T, usize... I>
constexpr auto
__to_tuple(const T &__obj, micron::index_sequence<I...>)
{
  return micron::tuple<field_type<I, T>...>{ get_field<I>(__obj)... };
}

template<typename T, usize... I>
constexpr auto
__to_ref_tuple(T &__obj, micron::index_sequence<I...>)
{
  return micron::tuple<field_type<I, T> &...>{ get_field<I>(__obj)... };
}

};      // namespace __impl

template<typename T>
  requires reflectable<T>
constexpr auto
to_tuple(const T &__obj)
{
  return __impl::__to_tuple(__obj, micron::make_index_sequence<field_count<T>>{});
}

template<typename T>
  requires reflectable<T>
constexpr auto
tie_fields(T &__obj)
{
  return __impl::__to_ref_tuple(__obj, micron::make_index_sequence<field_count<T>>{});
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// structural operations

template<typename T>
  requires reflectable<T>
constexpr bool
equal(const T &__a, const T &__b) noexcept
{
  bool __eq = true;
  template for ( constexpr auto __b_ : bases<T> )
  {
    if ( __eq ) {
      using B = [:micron::meta::type_of(__b_):];
      if constexpr ( micron::equality_comparable<B> )
        __eq = (static_cast<const B &>(__a) == static_cast<const B &>(__b));
      else
        __eq = equal(static_cast<const B &>(__a), static_cast<const B &>(__b));
    }
  }
  template for ( constexpr auto __m : members<T> )
  {
    if ( __eq ) {
      using M = micron::remove_cvref_t<decltype(__a.[:__m:])>;
      if constexpr ( reflectable<M> && !micron::equality_comparable<M> )
        __eq = equal(__a.[:__m:], __b.[:__m:]);
      else
        __eq = (__a.[:__m:] == __b.[:__m:]);
    }
  }
  return __eq;
}

namespace __impl
{

inline constexpr u64 __fnv_basis = 0xcbf29ce484222325ull;
inline constexpr u64 __fnv_prime = 0x100000001b3ull;

constexpr void
__mix_bytes(u64 &__h, const unsigned char *__p, usize __n) noexcept
{
  for ( usize i = 0; i < __n; ++i ) {
    __h ^= static_cast<u64>(__p[i]);
    __h *= __fnv_prime;
  }
}

constexpr void
__mix_word(u64 &__h, u64 __w) noexcept
{
  for ( usize i = 0; i < sizeof(u64); ++i ) {
    __h ^= (__w >> (i * 8)) & 0xffull;
    __h *= __fnv_prime;
  }
}

template<typename M>
constexpr void
__mix_scalar(u64 &__h, const M &__v) noexcept
{
  if constexpr ( micron::is_floating_point_v<M> ) {
    if ( __v == static_cast<M>(0) ) {
      __mix_word(__h, 0);
      return;
    }
    if constexpr ( sizeof(M) == 4 )
      __mix_word(__h, static_cast<u64>(__builtin_bit_cast(u32, __v)));
    else if constexpr ( sizeof(M) == 8 )
      __mix_word(__h, __builtin_bit_cast(u64, __v));
    else
      __mix_bytes(__h, reinterpret_cast<const unsigned char *>(micron::addressof(__v)), sizeof(M));
  } else if constexpr ( micron::is_integral_v<M> or micron::is_enum_v<M> )
    __mix_word(__h, static_cast<u64>(__v));
  else
    __mix_bytes(__h, reinterpret_cast<const unsigned char *>(micron::addressof(__v)), sizeof(M));
}

};      // namespace __impl

template<typename T>
  requires reflectable<T>
constexpr u64
hash(const T &__obj) noexcept
{
  u64 __h = __impl::__fnv_basis;
  template for ( constexpr auto __b_ : bases<T> )
  {
    using B = [:micron::meta::type_of(__b_):];
    __h ^= hash(static_cast<const B &>(__obj));
    __h *= __impl::__fnv_prime;
  }
  template for ( constexpr auto __m : members<T> )
  {
    using M = micron::remove_cvref_t<decltype(__obj.[:__m:])>;
    if constexpr ( reflectable<M> ) {
      __h ^= hash(__obj.[:__m:]);
      __h *= __impl::__fnv_prime;
    } else
      __impl::__mix_scalar(__h, __obj.[:__m:]);
  }
  return __h;
}

};      // namespace reflect
};      // namespace micron

#endif      // __micron_reflection
