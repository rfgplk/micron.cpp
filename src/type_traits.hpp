//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// NOTE: type_traits has been implemented _exactly_ to current cpp23 spec
// should perform vis-a-vis to the stl
namespace micron
{
using __tt_size_t = long unsigned int;
using __tt_nullptr_t = decltype(nullptr);

using __bfloat16_t = __decltype(0.0bf16);

template <typename T> class reference_wrapper;
template <typename T, T V> struct integral_constant {
  static constexpr T value = V;
  using value_type = T;
  using type = integral_constant<T, V>;
  constexpr
  operator value_type() const noexcept
  {
    return value;
  }

  constexpr value_type
  operator()() const noexcept
  {
    return value;
  }
};
template <bool V> using __bool_constant = integral_constant<bool, V>;

using true_type = __bool_constant<true>;

using false_type = __bool_constant<false>;

template <bool V> using bool_constant = __bool_constant<V>;

template <bool, typename T = void> struct enable_if {
};

template <typename T> struct enable_if<true, T> {
  using type = T;
};

template <bool C, typename T = void> using __enable_if_t = typename enable_if<C, T>::type;

template <bool> struct __conditional {
  template <typename T, typename> using type = T;
};

template <> struct __conditional<false> {
  template <typename, typename Uxp> using type = Uxp;
};

template <bool C, typename _If, typename _Else>
using __conditional_t = typename __conditional<C>::template type<_If, _Else>;

template <typename _Type> struct __type_identity {
  using type = _Type;
};

template <typename T> using __type_identity_t = typename __type_identity<T>::type;

namespace __detail
{

template <typename T, typename...> using __first_t = T;

template <typename... B> auto __or_fn(int) -> __first_t<false_type, __enable_if_t<!bool(B::value)>...>;

template <typename... B> auto __or_fn(...) -> true_type;

template <typename... B> auto __and_fn(int) -> __first_t<true_type, __enable_if_t<bool(B::value)>...>;

template <typename... B> auto __and_fn(...) -> false_type;
}

template <typename... B> struct __or_ : decltype(__detail::__or_fn<B...>(0)) {
};

template <typename... B> struct __and_ : decltype(__detail::__and_fn<B...>(0)) {
};

template <typename P> struct __not_ : __bool_constant<!bool(P::value)> {
};

template <typename... B> inline constexpr bool __or_v = __or_<B...>::value;
template <typename... B> inline constexpr bool __and_v = __and_<B...>::value;

namespace __detail
{
template <typename, typename _B1, typename... B> struct __disjunction_impl {
  using type = _B1;
};

template <typename _B1, typename _B2, typename... B>
struct __disjunction_impl<__enable_if_t<!bool(_B1::value)>, _B1, _B2, B...> {
  using type = typename __disjunction_impl<void, _B2, B...>::type;
};

template <typename, typename _B1, typename... B> struct __conjunction_impl {
  using type = _B1;
};

template <typename _B1, typename _B2, typename... B>
struct __conjunction_impl<__enable_if_t<bool(_B1::value)>, _B1, _B2, B...> {
  using type = typename __conjunction_impl<void, _B2, B...>::type;
};
}

template <typename... B> struct conjunction : __detail::__conjunction_impl<void, B...>::type {
};

template <> struct conjunction<> : true_type {
};

template <typename... B> struct disjunction : __detail::__disjunction_impl<void, B...>::type {
};

template <> struct disjunction<> : false_type {
};

template <typename P> struct negation : __not_<P>::type {
};

template <typename... B> inline constexpr bool conjunction_v = conjunction<B...>::value;

template <typename... B> inline constexpr bool disjunction_v = disjunction<B...>::value;

template <typename P> inline constexpr bool negation_v = negation<P>::value;

template <typename> struct is_reference;
template <typename> struct is_function;
template <typename> struct is_void;
template <typename> struct remove_cv;
template <typename> struct is_const;

template <typename> struct __is_array_unknown_bounds;

template <typename T, __tt_size_t = sizeof(T)>
constexpr true_type
__is_complete_or_unbounded(__type_identity<T>)
{
  return {};
}

template <typename _TypeIdentity, typename _NestedType = typename _TypeIdentity::type>
constexpr typename __or_<is_reference<_NestedType>, is_function<_NestedType>, is_void<_NestedType>,
                         __is_array_unknown_bounds<_NestedType>>::type
__is_complete_or_unbounded(_TypeIdentity)
{
  return {};
}

template <typename T> using __remove_cv_t = typename remove_cv<T>::type;

template <typename T> struct is_void : public false_type {
};

template <> struct is_void<void> : public true_type {
};

template <> struct is_void<const void> : public true_type {
};

template <> struct is_void<volatile void> : public true_type {
};

template <> struct is_void<const volatile void> : public true_type {
};

template <typename> struct __is_integral_helper : public false_type {
};

template <> struct __is_integral_helper<bool> : public true_type {
};

template <> struct __is_integral_helper<char> : public true_type {
};

template <> struct __is_integral_helper<signed char> : public true_type {
};

template <> struct __is_integral_helper<unsigned char> : public true_type {
};

template <> struct __is_integral_helper<wchar_t> : public true_type {
};
// NOTE: even though the standard doesn't say this, make char8 integral too
template <> struct __is_integral_helper<char8_t> : public true_type {
};
template <> struct __is_integral_helper<char16_t> : public true_type {
};

template <> struct __is_integral_helper<char32_t> : public true_type {
};

template <> struct __is_integral_helper<short> : public true_type {
};

template <> struct __is_integral_helper<unsigned short> : public true_type {
};

template <> struct __is_integral_helper<int> : public true_type {
};

template <> struct __is_integral_helper<unsigned int> : public true_type {
};

template <> struct __is_integral_helper<long> : public true_type {
};

template <> struct __is_integral_helper<unsigned long> : public true_type {
};

template <> struct __is_integral_helper<long long> : public true_type {
};

template <> struct __is_integral_helper<unsigned long long> : public true_type {
};

__extension__ template <> struct __is_integral_helper<__int128> : public true_type {
};

__extension__ template <> struct __is_integral_helper<unsigned __int128> : public true_type {
};
template <typename T> struct is_integral : public __is_integral_helper<__remove_cv_t<T>>::type {
};

template <typename> struct __is_floating_point_helper : public false_type {
};

template <> struct __is_floating_point_helper<float> : public true_type {
};

template <> struct __is_floating_point_helper<double> : public true_type {
};

template <> struct __is_floating_point_helper<long double> : public true_type {
};
template <> struct __is_floating_point_helper<__float128> : public true_type {
};

template <typename T> struct is_floating_point : public __is_floating_point_helper<__remove_cv_t<T>>::type {
};

template <typename T> struct is_array : public __bool_constant<__is_array(T)> {
};
template <typename T> struct is_pointer : public __bool_constant<__is_pointer(T)> {
};
template <typename> struct is_lvalue_reference : public false_type {
};

template <typename T> struct is_lvalue_reference<T &> : public true_type {
};

template <typename> struct is_rvalue_reference : public false_type {
};

template <typename T> struct is_rvalue_reference<T &&> : public true_type {
};

template <typename T> struct is_member_object_pointer : public __bool_constant<__is_member_object_pointer(T)> {
};
template <typename T> struct is_member_function_pointer : public __bool_constant<__is_member_function_pointer(T)> {
};
template <typename T> struct is_enum : public __bool_constant<__is_enum(T)> {
};

template <typename T> struct is_union : public __bool_constant<__is_union(T)> {
};

template <typename T> struct is_class : public __bool_constant<__is_class(T)> {
};

template <typename T> struct is_function : public __bool_constant<__is_function(T)> {
};
template <typename T> struct is_null_pointer : public false_type {
};

template <> struct is_null_pointer<__tt_nullptr_t> : public true_type {
};

template <> struct is_null_pointer<const __tt_nullptr_t> : public true_type {
};

template <> struct is_null_pointer<volatile __tt_nullptr_t> : public true_type {
};

template <> struct is_null_pointer<const volatile __tt_nullptr_t> : public true_type {
};

template <typename T> struct __is_nullptr_t : public is_null_pointer<T> {
} __attribute__((__deprecated__("use '"
                                "micron::is_null_pointer"
                                "' instead")));

template <typename T> struct is_reference : public __bool_constant<__is_reference(T)> {
};
template <typename T> struct is_arithmetic : public __or_<is_integral<T>, is_floating_point<T>>::type {
};

template <typename T> struct is_fundamental : public __or_<is_arithmetic<T>, is_void<T>, is_null_pointer<T>>::type {
};

template <typename T> struct is_object : public __bool_constant<__is_object(T)> {
};
template <typename> struct is_member_pointer;

template <typename T>
struct is_scalar
    : public __or_<is_arithmetic<T>, is_enum<T>, is_pointer<T>, is_member_pointer<T>, is_null_pointer<T>>::type {
};

template <typename T> struct is_compound : public __bool_constant<!is_fundamental<T>::value> {
};

template <typename T> struct is_member_pointer : public __bool_constant<__is_member_pointer(T)> {
};
template <typename, typename> struct is_same;

template <typename T, typename... _Types> using __is_one_of = __or_<is_same<T, _Types>...>;

__extension__ template <typename T>
using __is_signed_integer
    = __is_one_of<__remove_cv_t<T>, signed char, signed short, signed int, signed long, signed long long

                  ,
                  signed __int128>;

__extension__ template <typename T>
using __is_unsigned_integer
    = __is_one_of<__remove_cv_t<T>, unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long

                  ,
                  unsigned __int128>;

template <typename T> using __is_standard_integer = __or_<__is_signed_integer<T>, __is_unsigned_integer<T>>;

template <typename...> using Void_t = void;

template <typename T> struct is_const : public __bool_constant<__is_const(T)> {
};
template <typename T> struct is_volatile : public __bool_constant<__is_volatile(T)> {
};
template <typename T>
struct

    is_trivial : public __bool_constant<__is_trivial(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_trivially_copyable : public __bool_constant<__is_trivially_copyable(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_standard_layout : public __bool_constant<__is_standard_layout(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct

    is_pod : public __bool_constant<__is_pod(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct

    [[__deprecated__]]

    is_literal_type : public __bool_constant<__is_literal_type(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_empty : public __bool_constant<__is_empty(T)> {
};

template <typename T> struct is_polymorphic : public __bool_constant<__is_polymorphic(T)> {
};

template <typename T> struct is_final : public __bool_constant<__is_final(T)> {
};

template <typename T> struct is_abstract : public __bool_constant<__is_abstract(T)> {
};

template <typename T, bool = is_arithmetic<T>::value> struct __is_signed_helper : public false_type {
};

template <typename T> struct __is_signed_helper<T, true> : public __bool_constant<T(-1) < T(0)> {
};

template <typename T> struct is_signed : public __is_signed_helper<T>::type {
};

template <typename T> struct is_unsigned : public __and_<is_arithmetic<T>, __not_<is_signed<T>>>::type {
};

template <typename T, typename Uxp = T &&> Uxp __declval(int);

template <typename T> T __declval(long);

template <typename T> auto declval() noexcept -> decltype(__declval<T>(0));

template <typename> struct remove_all_extents;

template <typename T> struct __is_array_known_bounds : public false_type {
};

template <typename T, __tt_size_t _Size> struct __is_array_known_bounds<T[_Size]> : public true_type {
};

template <typename T> struct __is_array_unknown_bounds : public false_type {
};

template <typename T> struct __is_array_unknown_bounds<T[]> : public true_type {
};
struct __do_is_destructible_impl {
  template <typename T, typename = decltype(declval<T &>().~T())> static true_type __test(int);

  template <typename> static false_type __test(...);
};

template <typename T> struct __is_destructible_impl : public __do_is_destructible_impl {
  using type = decltype(__test<T>(0));
};

template <typename T, bool = __or_<is_void<T>, __is_array_unknown_bounds<T>, is_function<T>>::value,
          bool = __or_<is_reference<T>, is_scalar<T>>::value>
struct __is_destructible_safe;

template <typename T>
struct __is_destructible_safe<T, false, false>
    : public __is_destructible_impl<typename remove_all_extents<T>::type>::type {
};

template <typename T> struct __is_destructible_safe<T, true, false> : public false_type {
};

template <typename T> struct __is_destructible_safe<T, false, true> : public true_type {
};

template <typename T> struct is_destructible : public __is_destructible_safe<T>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

struct __do_is_nt_destructible_impl {
  template <typename T> static __bool_constant<noexcept(declval<T &>().~T())> __test(int);

  template <typename> static false_type __test(...);
};

template <typename T> struct __is_nt_destructible_impl : public __do_is_nt_destructible_impl {
  using type = decltype(__test<T>(0));
};

template <typename T, bool = __or_<is_void<T>, __is_array_unknown_bounds<T>, is_function<T>>::value,
          bool = __or_<is_reference<T>, is_scalar<T>>::value>
struct __is_nt_destructible_safe;

template <typename T>
struct __is_nt_destructible_safe<T, false, false>
    : public __is_nt_destructible_impl<typename remove_all_extents<T>::type>::type {
};

template <typename T> struct __is_nt_destructible_safe<T, true, false> : public false_type {
};

template <typename T> struct __is_nt_destructible_safe<T, false, true> : public true_type {
};

template <typename T> struct is_nothrow_destructible : public __is_nt_destructible_safe<T>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T, typename... Args> using __is_constructible_impl = __bool_constant<__is_constructible(T, Args...)>;

template <typename T, typename... Args> struct is_constructible : public __is_constructible_impl<T, Args...> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_default_constructible : public __is_constructible_impl<T> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> using __add_lval_ref_t = __add_lvalue_reference(T);
template <typename T> struct is_copy_constructible : public __is_constructible_impl<T, __add_lval_ref_t<const T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> using __add_rval_ref_t = __add_rvalue_reference(T);
template <typename T> struct is_move_constructible : public __is_constructible_impl<T, __add_rval_ref_t<T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T, typename... Args>
using __is_nothrow_constructible_impl = __bool_constant<__is_nothrow_constructible(T, Args...)>;

template <typename T, typename... Args>
struct is_nothrow_constructible : public __is_nothrow_constructible_impl<T, Args...> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_nothrow_default_constructible : public __is_nothrow_constructible_impl<T> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_nothrow_copy_constructible : public __is_nothrow_constructible_impl<T, __add_lval_ref_t<const T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_nothrow_move_constructible : public __is_nothrow_constructible_impl<T, __add_rval_ref_t<T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T, typename Uxp> using __is_assignable_impl = __bool_constant<__is_assignable(T, Uxp)>;

template <typename T, typename Uxp> struct is_assignable : public __is_assignable_impl<T, Uxp> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_copy_assignable : public __is_assignable_impl<__add_lval_ref_t<T>, __add_lval_ref_t<const T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_move_assignable : public __is_assignable_impl<__add_lval_ref_t<T>, __add_rval_ref_t<T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T, typename Uxp>
using __is_nothrow_assignable_impl = __bool_constant<__is_nothrow_assignable(T, Uxp)>;

template <typename T, typename Uxp> struct is_nothrow_assignable : public __is_nothrow_assignable_impl<T, Uxp> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_nothrow_copy_assignable : public __is_nothrow_assignable_impl<__add_lval_ref_t<T>, __add_lval_ref_t<const T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_nothrow_move_assignable : public __is_nothrow_assignable_impl<__add_lval_ref_t<T>, __add_rval_ref_t<T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T, typename... Args>
using __is_trivially_constructible_impl = __bool_constant<__is_trivially_constructible(T, Args...)>;

template <typename T, typename... Args>
struct is_trivially_constructible : public __is_trivially_constructible_impl<T, Args...> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_trivially_default_constructible : public __is_trivially_constructible_impl<T> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};
struct __do_is_implicitly_default_constructible_impl {
  template <typename T> static void __helper(const T &);

  template <typename T> static true_type __test(const T &, decltype(__helper<const T &>({})) * = 0);

  static false_type __test(...);
};

template <typename T>
struct __is_implicitly_default_constructible_impl : public __do_is_implicitly_default_constructible_impl {
  using type = decltype(__test(declval<T>()));
};

template <typename T>
struct __is_implicitly_default_constructible_safe : public __is_implicitly_default_constructible_impl<T>::type {
};

template <typename T>
struct __is_implicitly_default_constructible
    : public __and_<__is_constructible_impl<T>, __is_implicitly_default_constructible_safe<T>>::type {
};

template <typename T>
struct is_trivially_copy_constructible : public __is_trivially_constructible_impl<T, __add_lval_ref_t<const T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_trivially_move_constructible : public __is_trivially_constructible_impl<T, __add_rval_ref_t<T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T, typename Uxp>
using __is_trivially_assignable_impl = __bool_constant<__is_trivially_assignable(T, Uxp)>;

template <typename T, typename Uxp> struct is_trivially_assignable : public __is_trivially_assignable_impl<T, Uxp> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_trivially_copy_assignable
    : public __is_trivially_assignable_impl<__add_lval_ref_t<T>, __add_lval_ref_t<const T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_trivially_move_assignable : public __is_trivially_assignable_impl<__add_lval_ref_t<T>, __add_rval_ref_t<T>> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
struct is_trivially_destructible
    : public __and_<__is_destructible_safe<T>, __bool_constant<__has_trivial_destructor(T)>>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct has_virtual_destructor : public __bool_constant<__has_virtual_destructor(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct alignment_of : public integral_constant<__tt_size_t, alignof(T)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct rank : public integral_constant<__tt_size_t, __array_rank(T)> {
};
template <typename, unsigned Uxint = 0> struct extent : public integral_constant<__tt_size_t, 0> {
};

template <typename T, __tt_size_t _Size> struct extent<T[_Size], 0> : public integral_constant<__tt_size_t, _Size> {
};

template <typename T, unsigned Uxint, __tt_size_t _Size>
struct extent<T[_Size], Uxint> : public extent<T, Uxint - 1>::type {
};

template <typename T> struct extent<T[], 0> : public integral_constant<__tt_size_t, 0> {
};

template <typename T, unsigned Uxint> struct extent<T[], Uxint> : public extent<T, Uxint - 1>::type {
};

template <typename T, typename Uxp> struct is_same : public __bool_constant<__is_same(T, Uxp)> {
};
template <typename _Base, typename _Derived> struct is_base_of : public __bool_constant<__is_base_of(_Base, _Derived)> {
};
template <typename F, typename U> struct is_convertible : public __bool_constant<__is_convertible(F, U)> {
};
template <typename UElementType, typename FElementType>
using __is_array_convertible = is_convertible<FElementType (*)[], UElementType (*)[]>;
template <typename T, typename... Args>
struct __is_nothrow_new_constructible_impl
    : __bool_constant<noexcept(::new (micron::declval<void *>()) T(micron::declval<Args>()...))> {
};

template <typename T, typename... Args>

inline constexpr bool __is_nothrow_new_constructible
    = __and_<is_constructible<T, Args...>, __is_nothrow_new_constructible_impl<T, Args...>>::value;

template <typename T> struct remove_const {
  using type = T;
};

template <typename T> struct remove_const<T const> {
  using type = T;
};

template <typename T> struct remove_volatile {
  using type = T;
};

template <typename T> struct remove_volatile<T volatile> {
  using type = T;
};

template <typename T> struct remove_cv {
  using type = __remove_cv(T);
};
template <typename T> struct add_const {
  using type = T const;
};

template <typename T> struct add_volatile {
  using type = T volatile;
};

template <typename T> struct add_cv {
  using type = T const volatile;
};

template <typename T> using remove_const_t = typename remove_const<T>::type;

template <typename T> using remove_volatile_t = typename remove_volatile<T>::type;

template <typename T> using remove_cv_t = typename remove_cv<T>::type;

template <typename T> using add_const_t = typename add_const<T>::type;

template <typename T> using add_volatile_t = typename add_volatile<T>::type;

template <typename T> using add_cv_t = typename add_cv<T>::type;

template <typename T> struct remove_reference {
  using type = __remove_reference(T);
};
template <typename T> struct add_lvalue_reference {
  using type = __add_lval_ref_t<T>;
};

template <typename T> struct add_rvalue_reference {
  using type = __add_rval_ref_t<T>;
};

template <typename T> using remove_reference_t = typename remove_reference<T>::type;

template <typename T> using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

template <typename T> using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template <typename Uxnqualified, bool _IsConst, bool _IsVol> struct __cv_selector;

template <typename Uxnqualified> struct __cv_selector<Uxnqualified, false, false> {
  using __type = Uxnqualified;
};

template <typename Uxnqualified> struct __cv_selector<Uxnqualified, false, true> {
  using __type = volatile Uxnqualified;
};

template <typename Uxnqualified> struct __cv_selector<Uxnqualified, true, false> {
  using __type = const Uxnqualified;
};

template <typename Uxnqualified> struct __cv_selector<Uxnqualified, true, true> {
  using __type = const volatile Uxnqualified;
};

template <typename _Qualified, typename Uxnqualified, bool _IsConst = is_const<_Qualified>::value,
          bool _IsVol = is_volatile<_Qualified>::value>
class __match_cv_qualifiers
{
  using __match = __cv_selector<Uxnqualified, _IsConst, _IsVol>;

public:
  using __type = typename __match::__type;
};

template <typename T> struct __make_unsigned { using __type = T; };

template <> struct __make_unsigned<char> { using __type = unsigned char; };

template <> struct __make_unsigned<signed char> { using __type = unsigned char; };

template <> struct __make_unsigned<short> { using __type = unsigned short; };

template <> struct __make_unsigned<int> { using __type = unsigned int; };

template <> struct __make_unsigned<long> { using __type = unsigned long; };

template <> struct __make_unsigned<long long> { using __type = unsigned long long; };

__extension__ template <> struct __make_unsigned<__int128> { using __type = unsigned __int128; };
template <typename T, bool _IsInt = is_integral<T>::value, bool _IsEnum = __is_enum(T)> class __make_unsigned_selector;

template <typename T> class __make_unsigned_selector<T, true, false>
{
  using __unsigned_type = typename __make_unsigned<__remove_cv_t<T>>::__type;

public:
  using __type = typename __match_cv_qualifiers<T, __unsigned_type>::__type;
};

class __make_unsigned_selector_base
{
protected:
  template <typename...> struct _List {
  };

  template <typename T, typename... Uxp> struct _List<T, Uxp...> : _List<Uxp...> {
    static constexpr __tt_size_t __size = sizeof(T);
  };

  template <__tt_size_t _Sz, typename T, bool = (_Sz <= T::__size)> struct __select;

  template <__tt_size_t _Sz, typename Uxint, typename... UxInts> struct __select<_Sz, _List<Uxint, UxInts...>, true> {
    using __type = Uxint;
  };

  template <__tt_size_t _Sz, typename Uxint, typename... UxInts>
  struct __select<_Sz, _List<Uxint, UxInts...>, false> : __select<_Sz, _List<UxInts...>> {
  };
};

template <typename T> class __make_unsigned_selector<T, false, true> : __make_unsigned_selector_base
{

  using UxInts = _List<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long>;

  using __unsigned_type = typename __select<sizeof(T), UxInts>::__type;

public:
  using __type = typename __match_cv_qualifiers<T, __unsigned_type>::__type;
};

template <> struct __make_unsigned<wchar_t>
{
  using __type = typename __make_unsigned_selector<wchar_t, false, true>::__type;
};
template <> struct __make_unsigned<char16_t>
{
  using __type = typename __make_unsigned_selector<char16_t, false, true>::__type;
};

template <> struct __make_unsigned<char32_t>
{
  using __type = typename __make_unsigned_selector<char32_t, false, true>::__type;
};

template <typename T> struct make_unsigned {
  using type = typename __make_unsigned_selector<T>::__type;
};

template <> struct make_unsigned<bool>;
template <> struct make_unsigned<bool const>;
template <> struct make_unsigned<bool volatile>;
template <> struct make_unsigned<bool const volatile>;

template <typename T> struct __make_signed { using __type = T; };

template <> struct __make_signed<char> { using __type = signed char; };

template <> struct __make_signed<unsigned char> { using __type = signed char; };

template <> struct __make_signed<unsigned short> { using __type = signed short; };

template <> struct __make_signed<unsigned int> { using __type = signed int; };

template <> struct __make_signed<unsigned long> { using __type = signed long; };

template <> struct __make_signed<unsigned long long> { using __type = signed long long; };

__extension__ template <> struct __make_signed<unsigned __int128> { using __type = __int128; };
template <typename T, bool _IsInt = is_integral<T>::value, bool _IsEnum = __is_enum(T)> class __make_signed_selector;

template <typename T> class __make_signed_selector<T, true, false>
{
  using __signed_type = typename __make_signed<__remove_cv_t<T>>::__type;

public:
  using __type = typename __match_cv_qualifiers<T, __signed_type>::__type;
};

template <typename T> class __make_signed_selector<T, false, true>
{
  using __unsigned_type = typename __make_unsigned_selector<T>::__type;

public:
  using __type = typename __make_signed_selector<__unsigned_type>::__type;
};

template <> struct __make_signed<wchar_t>
{
  using __type = typename __make_signed_selector<wchar_t, false, true>::__type;
};
template <> struct __make_signed<char16_t>
{
  using __type = typename __make_signed_selector<char16_t, false, true>::__type;
};

template <> struct __make_signed<char32_t>
{
  using __type = typename __make_signed_selector<char32_t, false, true>::__type;
};

template <typename T> struct make_signed {
  using type = typename __make_signed_selector<T>::__type;
};

template <> struct make_signed<bool>;
template <> struct make_signed<bool const>;
template <> struct make_signed<bool volatile>;
template <> struct make_signed<bool const volatile>;

template <typename T> using make_signed_t = typename make_signed<T>::type;

template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;

template <typename T> struct remove_extent {
  using type = __remove_extent(T);
};
template <typename T> struct remove_all_extents {
  using type = __remove_all_extents(T);
};
template <typename T> using remove_extent_t = typename remove_extent<T>::type;

template <typename T> using remove_all_extents_t = typename remove_all_extents<T>::type;

template <typename T> struct remove_pointer {
  using type = __remove_pointer(T);
};
template <typename T> struct add_pointer {
  using type = __add_pointer(T);
};
template <typename T> using remove_pointer_t = typename remove_pointer<T>::type;

template <typename T> using add_pointer_t = typename add_pointer<T>::type;

struct __attribute__((__aligned__)) __aligned_storage_max_align_t {
};

constexpr __tt_size_t
__aligned_storage_default_alignment([[__maybe_unused__]] __tt_size_t __len)
{
  return alignof(__aligned_storage_max_align_t);
}
template <__tt_size_t _Len, __tt_size_t _Align = __aligned_storage_default_alignment(_Len)>
struct

    aligned_storage {
  struct type {
    alignas(_Align) unsigned char __data[_Len];
  };
};

template <typename... _Types> struct __strictest_alignment {
  static const __tt_size_t _S_alignment = 0;
  static const __tt_size_t _S_size = 0;
};

template <typename T, typename... _Types> struct __strictest_alignment<T, _Types...> {
  static const __tt_size_t _S_alignment = alignof(T) > __strictest_alignment<_Types...>::_S_alignment
                                              ? alignof(T)
                                              : __strictest_alignment<_Types...>::_S_alignment;
  static const __tt_size_t _S_size
      = sizeof(T) > __strictest_alignment<_Types...>::_S_size ? sizeof(T) : __strictest_alignment<_Types...>::_S_size;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
template <__tt_size_t _Len, typename... _Types>
struct

    aligned_union {
private:
  static_assert(sizeof...(_Types) != 0, "At least one type is required");

  using __strictest = __strictest_alignment<_Types...>;
  static const __tt_size_t _S_len = _Len > __strictest::_S_size ? _Len : __strictest::_S_size;

public:
  static const __tt_size_t alignment_value = __strictest::_S_alignment;

  using type = typename aligned_storage<_S_len, alignment_value>::type;
};

template <__tt_size_t _Len, typename... _Types> const __tt_size_t aligned_union<_Len, _Types...>::alignment_value;
#pragma GCC diagnostic pop

template <typename T> struct decay {
  using type = __decay(T);
};
template <typename T> struct __strip_reference_wrapper {
  using __type = T;
};

template <typename T> struct __strip_reference_wrapper<reference_wrapper<T>> {
  using __type = T &;
};

template <typename T> using __decay_t = typename decay<T>::type;

template <typename T> using __decay_and_strip = __strip_reference_wrapper<__decay_t<T>>;

template <typename... C> using _Require = __enable_if_t<__and_<C...>::value>;

template <typename T> using __remove_cvref_t = typename remove_cv<typename remove_reference<T>::type>::type;

template <bool C, typename _Iftrue, typename _Iffalse> struct conditional {
  using type = _Iftrue;
};

template <typename _Iftrue, typename _Iffalse> struct conditional<false, _Iftrue, _Iffalse> {
  using type = _Iffalse;
};

template <typename... T> struct common_type;
template <typename T> struct __success_type {
  using type = T;
};

struct __failure_type {
};

struct __do_common_type_impl {
  template <typename T, typename Uxp> using __cond_t = decltype(true ? micron::declval<T>() : micron::declval<Uxp>());

  template <typename T, typename Uxp> static __success_type<__decay_t<__cond_t<T, Uxp>>> _S_test(int);
  template <typename, typename> static __failure_type _S_test_2(...);

  template <typename T, typename Uxp> static decltype(_S_test_2<T, Uxp>(0)) _S_test(...);
};

template <> struct common_type<> {
};

template <typename T0> struct common_type<T0> : public common_type<T0, T0> {
};

template <typename T1, typename T2, typename _Dp1 = __decay_t<T1>, typename _Dp2 = __decay_t<T2>>
struct __common_type_impl {

  using type = common_type<_Dp1, _Dp2>;
};

template <typename T1, typename T2> struct __common_type_impl<T1, T2, T1, T2> : private __do_common_type_impl {

  using type = decltype(_S_test<T1, T2>(0));
};

template <typename T1, typename T2> struct common_type<T1, T2> : public __common_type_impl<T1, T2>::type {
};

template <typename...> struct __common_type_pack {
};

template <typename, typename, typename = void> struct __common_type_fold;

template <typename T1, typename T2, typename... _Rp>
struct common_type<T1, T2, _Rp...> : public __common_type_fold<common_type<T1, T2>, __common_type_pack<_Rp...>> {
};

template <typename _CTp, typename... _Rp>
struct __common_type_fold<_CTp, __common_type_pack<_Rp...>, Void_t<typename _CTp::type>>
    : public common_type<typename _CTp::type, _Rp...> {
};

template <typename _CTp, typename _Rp> struct __common_type_fold<_CTp, _Rp, void> {
};

template <typename T, bool = __is_enum(T)> struct __underlying_type_impl {
  using type = __underlying_type(T);
};

template <typename T> struct __underlying_type_impl<T, false> {
};

template <typename T> struct underlying_type : public __underlying_type_impl<T> {
};

template <typename T> struct __declval_protector {
  static const bool __stop = false;
};

template <typename T>
auto
declval() noexcept -> decltype(__declval<T>(0))
{
  static_assert(__declval_protector<T>::__stop, "declval() must not be used!");
  return __declval<T>(0);
}

template <typename _Signature> struct result_of;

struct __invoke_memfun_ref {
};
struct __invoke_memfun_deref {
};
struct __invoke_memobj_ref {
};
struct __invoke_memobj_deref {
};
struct __invoke_other {
};

template <typename T, typename _Tag> struct __result_of_success : __success_type<T> {
  using __invoke_type = _Tag;
};

struct __result_of_memfun_ref_impl {
  template <typename _Fp, typename T1, typename... Args>
  static __result_of_success<decltype((micron::declval<T1>().*micron::declval<_Fp>())(micron::declval<Args>()...)),
                             __invoke_memfun_ref>
  _S_test(int);

  template <typename...> static __failure_type _S_test(...);
};

template <typename Ptr, typename _Arg, typename... Args>
struct __result_of_memfun_ref : private __result_of_memfun_ref_impl {
  using type = decltype(_S_test<Ptr, _Arg, Args...>(0));
};

struct __result_of_memfun_deref_impl {
  template <typename _Fp, typename T1, typename... Args>
  static __result_of_success<decltype(((*micron::declval<T1>()).*micron::declval<_Fp>())(micron::declval<Args>()...)),
                             __invoke_memfun_deref>
  _S_test(int);

  template <typename...> static __failure_type _S_test(...);
};

template <typename Ptr, typename _Arg, typename... Args>
struct __result_of_memfun_deref : private __result_of_memfun_deref_impl {
  using type = decltype(_S_test<Ptr, _Arg, Args...>(0));
};

struct __result_of_memobj_ref_impl {
  template <typename _Fp, typename T1>
  static __result_of_success<decltype(micron::declval<T1>().*micron::declval<_Fp>()), __invoke_memobj_ref> _S_test(int);

  template <typename, typename> static __failure_type _S_test(...);
};

template <typename Ptr, typename _Arg> struct __result_of_memobj_ref : private __result_of_memobj_ref_impl {
  using type = decltype(_S_test<Ptr, _Arg>(0));
};

struct __result_of_memobj_deref_impl {
  template <typename _Fp, typename T1>
  static __result_of_success<decltype((*micron::declval<T1>()).*micron::declval<_Fp>()), __invoke_memobj_deref>
  _S_test(int);

  template <typename, typename> static __failure_type _S_test(...);
};

template <typename Ptr, typename _Arg> struct __result_of_memobj_deref : private __result_of_memobj_deref_impl {
  using type = decltype(_S_test<Ptr, _Arg>(0));
};

template <typename Ptr, typename _Arg> struct __result_of_memobj;

template <typename _Res, typename _Class, typename _Arg> struct __result_of_memobj<_Res _Class::*, _Arg> {
  using _Argval = __remove_cvref_t<_Arg>;
  using Ptr = _Res _Class::*;
  using type = typename __conditional_t<__or_<is_same<_Argval, _Class>, is_base_of<_Class, _Argval>>::value,
                                        __result_of_memobj_ref<Ptr, _Arg>, __result_of_memobj_deref<Ptr, _Arg>>::type;
};

template <typename Ptr, typename _Arg, typename... Args> struct __result_of_memfun;

template <typename _Res, typename _Class, typename _Arg, typename... Args>
struct __result_of_memfun<_Res _Class::*, _Arg, Args...> {
  using _Argval = typename remove_reference<_Arg>::type;
  using Ptr = _Res _Class::*;
  using type = typename __conditional_t<is_base_of<_Class, _Argval>::value, __result_of_memfun_ref<Ptr, _Arg, Args...>,
                                        __result_of_memfun_deref<Ptr, _Arg, Args...>>::type;
};

template <typename T, typename Uxp = __remove_cvref_t<T>> struct __inv_unwrap {
  using type = T;
};

template <typename T, typename Uxp> struct __inv_unwrap<T, reference_wrapper<Uxp>> {
  using type = Uxp &;
};

template <bool, bool, typename Fnc, typename... _ArgTypes> struct __result_of_impl {
  using type = __failure_type;
};

template <typename Ptr, typename _Arg>
struct __result_of_impl<true, false, Ptr, _Arg>
    : public __result_of_memobj<__decay_t<Ptr>, typename __inv_unwrap<_Arg>::type> {
};

template <typename Ptr, typename _Arg, typename... Args>
struct __result_of_impl<false, true, Ptr, _Arg, Args...>
    : public __result_of_memfun<__decay_t<Ptr>, typename __inv_unwrap<_Arg>::type, Args...> {
};

struct __result_of_other_impl {
  template <typename Fn, typename... Args>
  static __result_of_success<decltype(micron::declval<Fn>()(micron::declval<Args>()...)), __invoke_other> _S_test(int);

  template <typename...> static __failure_type _S_test(...);
};

template <typename Fnc, typename... _ArgTypes>
struct __result_of_impl<false, false, Fnc, _ArgTypes...> : private __result_of_other_impl {
  using type = decltype(_S_test<Fnc, _ArgTypes...>(0));
};

template <typename Fnc, typename... _ArgTypes>
struct __invoke_result : public __result_of_impl<is_member_object_pointer<typename remove_reference<Fnc>::type>::value,
                                                 is_member_function_pointer<typename remove_reference<Fnc>::type>::value,
                                                 Fnc, _ArgTypes...>::type {
};

template <typename Fn, typename... Args> using __invoke_result_t = typename __invoke_result<Fn, Args...>::type;

template <typename Fnc, typename... _ArgTypes>
struct result_of<Fnc(_ArgTypes...)> : public __invoke_result<Fnc, _ArgTypes...> {
} __attribute__((__deprecated__("use '"
                                "micron::invoke_result"
                                "' instead")));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

template <__tt_size_t _Len, __tt_size_t _Align = __aligned_storage_default_alignment(_Len)>
using aligned_storage_t = typename aligned_storage<_Len, _Align>::type;

template <__tt_size_t _Len, typename... _Types> using aligned_union_t = typename aligned_union<_Len, _Types...>::type;
#pragma GCC diagnostic pop

template <typename T> using decay_t = typename decay<T>::type;

template <bool C, typename T = void> using enable_if_t = typename enable_if<C, T>::type;

template <bool C, typename _Iftrue, typename _Iffalse>
using conditional_t = typename conditional<C, _Iftrue, _Iffalse>::type;

template <typename... T> using common_type_t = typename common_type<T...>::type;

template <typename T> using underlying_type_t = typename underlying_type<T>::type;

template <typename T> using result_of_t = typename result_of<T>::type;

template <typename...> using void_t = void;
template <typename Df, typename _AlwaysVoid, template <typename...> class Op, typename... Args> struct __detector {
  using type = Df;
  using __is_detected = false_type;
};

template <typename Df, template <typename...> class Op, typename... Args>
struct __detector<Df, Void_t<Op<Args...>>, Op, Args...> {
  using type = Op<Args...>;
  using __is_detected = true_type;
};

template <typename Df, template <typename...> class Op, typename... Args>
using __detected_or = __detector<Df, void, Op, Args...>;

template <typename Df, template <typename...> class Op, typename... Args>
using __detected_or_t = typename __detected_or<Df, Op, Args...>::type;
template <typename T> struct __is_swappable;

template <typename T> struct __is_nothrow_swappable;

template <typename> struct __is_tuple_like_impl : false_type {
};

template <typename T> struct __is_tuple_like : public __is_tuple_like_impl<__remove_cvref_t<T>>::type {
};

template <typename T>

inline _Require<__not_<__is_tuple_like<T>>, is_move_constructible<T>, is_move_assignable<T>>
swap(T &, T &) noexcept(__and_<is_nothrow_move_constructible<T>, is_nothrow_move_assignable<T>>::value);

template <typename T, __tt_size_t _Nm>

inline __enable_if_t<__is_swappable<T>::value> swap(T (&__a)[_Nm],
                                                    T (&__b)[_Nm]) noexcept(__is_nothrow_swappable<T>::value);

namespace __swappable_details
{
using micron::swap;

struct __do_is_swappable_impl {
  template <typename T, typename = decltype(swap(micron::declval<T &>(), micron::declval<T &>()))>
  static true_type __test(int);

  template <typename> static false_type __test(...);
};

struct __do_is_nothrow_swappable_impl {
  template <typename T>
  static __bool_constant<noexcept(swap(micron::declval<T &>(), micron::declval<T &>()))> __test(int);

  template <typename> static false_type __test(...);
};

}

template <typename T> struct __is_swappable_impl : public __swappable_details::__do_is_swappable_impl {
  using type = decltype(__test<T>(0));
};

template <typename T> struct __is_nothrow_swappable_impl : public __swappable_details::__do_is_nothrow_swappable_impl {
  using type = decltype(__test<T>(0));
};

template <typename T> struct __is_swappable : public __is_swappable_impl<T>::type {
};

template <typename T> struct __is_nothrow_swappable : public __is_nothrow_swappable_impl<T>::type {
};

template <typename T> struct is_swappable : public __is_swappable_impl<T>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T> struct is_nothrow_swappable : public __is_nothrow_swappable_impl<T>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>

inline constexpr bool is_swappable_v = is_swappable<T>::value;

template <typename T>

inline constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;

namespace __swappable_with_details
{
using micron::swap;

struct __do_is_swappable_with_impl {
  template <typename T, typename Uxp, typename = decltype(swap(micron::declval<T>(), micron::declval<Uxp>())),
            typename = decltype(swap(micron::declval<Uxp>(), micron::declval<T>()))>
  static true_type __test(int);

  template <typename, typename> static false_type __test(...);
};

struct __do_is_nothrow_swappable_with_impl {
  template <typename T, typename Uxp>
  static __bool_constant<noexcept(swap(micron::declval<T>(), micron::declval<Uxp>()))
                         && noexcept(swap(micron::declval<Uxp>(), micron::declval<T>()))>
  __test(int);

  template <typename, typename> static false_type __test(...);
};

}

template <typename T, typename Uxp>
struct __is_swappable_with_impl : public __swappable_with_details::__do_is_swappable_with_impl {
  using type = decltype(__test<T, Uxp>(0));
};

template <typename T> struct __is_swappable_with_impl<T &, T &> : public __swappable_details::__do_is_swappable_impl {
  using type = decltype(__test<T &>(0));
};

template <typename T, typename Uxp>
struct __is_nothrow_swappable_with_impl : public __swappable_with_details::__do_is_nothrow_swappable_with_impl {
  using type = decltype(__test<T, Uxp>(0));
};

template <typename T>
struct __is_nothrow_swappable_with_impl<T &, T &> : public __swappable_details::__do_is_nothrow_swappable_impl {
  using type = decltype(__test<T &>(0));
};

template <typename T, typename Uxp> struct is_swappable_with : public __is_swappable_with_impl<T, Uxp>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "first template argument must be a complete class or an unbounded array");
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Uxp>{}),
                "second template argument must be a complete class or an unbounded array");
};

template <typename T, typename Uxp>
struct is_nothrow_swappable_with : public __is_nothrow_swappable_with_impl<T, Uxp>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "first template argument must be a complete class or an unbounded array");
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Uxp>{}),
                "second template argument must be a complete class or an unbounded array");
};

template <typename T, typename Uxp>

inline constexpr bool is_swappable_with_v = is_swappable_with<T, Uxp>::value;

template <typename T, typename Uxp>

inline constexpr bool is_nothrow_swappable_with_v = is_nothrow_swappable_with<T, Uxp>::value;
template <typename _Result, typename R, bool = is_void<R>::value, typename = void>
struct __is_invocable_impl : false_type {
  using __nothrow_conv = false_type;
};

template <typename _Result, typename R>
struct __is_invocable_impl<_Result, R, true, Void_t<typename _Result::type>> : true_type {
  using __nothrow_conv = true_type;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"

template <typename _Result, typename R> struct __is_invocable_impl<_Result, R, false, Void_t<typename _Result::type>> {
private:
  using _Res_t = typename _Result::type;

  static _Res_t _S_get() noexcept;

  template <typename T> static void _S_conv(__type_identity_t<T>) noexcept;

  template <typename T, bool _Nothrow = noexcept(_S_conv<T>(_S_get())), typename = decltype(_S_conv<T>(_S_get())),

            bool _Dangle = __reference_converts_from_temporary(T, _Res_t)

            >
  static __bool_constant<_Nothrow && !_Dangle> _S_test(int);

  template <typename T, bool = false> static false_type _S_test(...);

public:
  using type = decltype(_S_test<R, true>(1));

  using __nothrow_conv = decltype(_S_test<R>(1));
};
#pragma GCC diagnostic pop

template <typename Fn, typename... _ArgTypes>
struct __is_invocable : __is_invocable_impl<__invoke_result<Fn, _ArgTypes...>, void>::type {
};

template <typename Fn, typename T, typename... Args>
constexpr bool
__call_is_nt(__invoke_memfun_ref)
{
  using Uxp = typename __inv_unwrap<T>::type;
  return noexcept((micron::declval<Uxp>().*micron::declval<Fn>())(micron::declval<Args>()...));
}

template <typename Fn, typename T, typename... Args>
constexpr bool
__call_is_nt(__invoke_memfun_deref)
{
  return noexcept(((*micron::declval<T>()).*micron::declval<Fn>())(micron::declval<Args>()...));
}

template <typename Fn, typename T>
constexpr bool
__call_is_nt(__invoke_memobj_ref)
{
  using Uxp = typename __inv_unwrap<T>::type;
  return noexcept(micron::declval<Uxp>().*micron::declval<Fn>());
}

template <typename Fn, typename T>
constexpr bool
__call_is_nt(__invoke_memobj_deref)
{
  return noexcept((*micron::declval<T>()).*micron::declval<Fn>());
}

template <typename Fn, typename... Args>
constexpr bool
__call_is_nt(__invoke_other)
{
  return noexcept(micron::declval<Fn>()(micron::declval<Args>()...));
}

template <typename _Result, typename Fn, typename... Args>
struct __call_is_nothrow : __bool_constant<micron::__call_is_nt<Fn, Args...>(typename _Result::__invoke_type{})> {
};

template <typename Fn, typename... Args>
using __call_is_nothrow_ = __call_is_nothrow<__invoke_result<Fn, Args...>, Fn, Args...>;

template <typename Fn, typename... Args>
struct __is_nothrow_invocable : __and_<__is_invocable<Fn, Args...>, __call_is_nothrow_<Fn, Args...>>::type {
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
struct __nonesuchbase {
};
struct __nonesuch : private __nonesuchbase {
  ~__nonesuch() = delete;
  __nonesuch(__nonesuch const &) = delete;
  void operator=(__nonesuch const &) = delete;
};
#pragma GCC diagnostic pop

template <typename Fnc, typename... _ArgTypes> struct invoke_result : public __invoke_result<Fnc, _ArgTypes...> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Fnc>{}),
                "Fnc must be a complete class or an unbounded array");
  static_assert((micron::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
                "each argument type must be a complete class or an unbounded array");
};

template <typename Fn, typename... Args> using invoke_result_t = typename invoke_result<Fn, Args...>::type;

template <typename Fn, typename... _ArgTypes>
struct is_invocable

    : public __bool_constant<__is_invocable(Fn, _ArgTypes...)>

{
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Fn>{}),
                "Fn must be a complete class or an unbounded array");
  static_assert((micron::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
                "each argument type must be a complete class or an unbounded array");
};

template <typename R, typename Fn, typename... _ArgTypes>
struct is_invocable_r : __is_invocable_impl<__invoke_result<Fn, _ArgTypes...>, R>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Fn>{}),
                "Fn must be a complete class or an unbounded array");
  static_assert((micron::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
                "each argument type must be a complete class or an unbounded array");
  static_assert(micron::__is_complete_or_unbounded(__type_identity<R>{}),
                "R must be a complete class or an unbounded array");
};

template <typename Fn, typename... _ArgTypes>
struct is_nothrow_invocable

    : public __bool_constant<__is_nothrow_invocable(Fn, _ArgTypes...)>

{
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Fn>{}),
                "Fn must be a complete class or an unbounded array");
  static_assert((micron::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
                "each argument type must be a complete class or an unbounded array");
};

template <typename _Result, typename R>
using __is_nt_invocable_impl = typename __is_invocable_impl<_Result, R>::__nothrow_conv;

template <typename R, typename Fn, typename... _ArgTypes>
struct is_nothrow_invocable_r
    : __and_<__is_nt_invocable_impl<__invoke_result<Fn, _ArgTypes...>, R>, __call_is_nothrow_<Fn, _ArgTypes...>>::type {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<Fn>{}),
                "Fn must be a complete class or an unbounded array");
  static_assert((micron::__is_complete_or_unbounded(__type_identity<_ArgTypes>{}) && ...),
                "each argument type must be a complete class or an unbounded array");
  static_assert(micron::__is_complete_or_unbounded(__type_identity<R>{}),
                "R must be a complete class or an unbounded array");
};

// value'd'
template <typename T> inline constexpr bool is_void_v = is_void<T>::value;
template <typename T> inline constexpr bool is_null_pointer_v = is_null_pointer<T>::value;
template <typename T> inline constexpr bool is_integral_v = is_integral<T>::value;
template <typename T> inline constexpr bool is_floating_point_v = is_floating_point<T>::value;

template <typename T> inline constexpr bool is_array_v = __is_array(T);
template <typename T> inline constexpr bool is_pointer_v = __is_pointer(T);
template <typename T> inline constexpr bool is_lvalue_reference_v = false;
template <typename T> inline constexpr bool is_lvalue_reference_v<T &> = true;
template <typename T> inline constexpr bool is_rvalue_reference_v = false;
template <typename T> inline constexpr bool is_rvalue_reference_v<T &&> = true;

template <typename T> inline constexpr bool is_member_object_pointer_v = __is_member_object_pointer(T);

template <typename T> inline constexpr bool is_member_function_pointer_v = __is_member_function_pointer(T);

template <typename T> inline constexpr bool is_enum_v = __is_enum(T);
template <typename T> inline constexpr bool is_union_v = __is_union(T);
template <typename T> inline constexpr bool is_class_v = __is_class(T);

template <typename T> inline constexpr bool is_reference_v = __is_reference(T);
template <typename T> inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
template <typename T> inline constexpr bool is_fundamental_v = is_fundamental<T>::value;

template <typename T> inline constexpr bool is_object_v = __is_object(T);

template <typename T> inline constexpr bool is_scalar_v = is_scalar<T>::value;
template <typename T> inline constexpr bool is_compound_v = !is_fundamental_v<T>;

template <typename T> inline constexpr bool is_member_pointer_v = __is_member_pointer(T);

template <typename T> inline constexpr bool is_const_v = __is_const(T);
template <typename T> inline constexpr bool is_function_v = __is_function(T);
template <typename T> inline constexpr bool is_volatile_v = __is_volatile(T);

template <typename T>

inline constexpr bool is_trivial_v = __is_trivial(T);
template <typename T> inline constexpr bool is_trivially_copyable_v = __is_trivially_copyable(T);
template <typename T> inline constexpr bool is_standard_layout_v = __is_standard_layout(T);
template <typename T>

inline constexpr bool is_pod_v = __is_pod(T);
template <typename T>

[[__deprecated__]]

inline constexpr bool is_literal_type_v
    = __is_literal_type(T);
template <typename T> inline constexpr bool is_empty_v = __is_empty(T);
template <typename T> inline constexpr bool is_polymorphic_v = __is_polymorphic(T);
template <typename T> inline constexpr bool is_abstract_v = __is_abstract(T);
template <typename T> inline constexpr bool is_final_v = __is_final(T);

template <typename T> inline constexpr bool is_signed_v = is_signed<T>::value;
template <typename T> inline constexpr bool is_unsigned_v = is_unsigned<T>::value;

template <typename T, typename... Args> inline constexpr bool is_constructible_v = __is_constructible(T, Args...);
template <typename T> inline constexpr bool is_default_constructible_v = __is_constructible(T);
template <typename T> inline constexpr bool is_copy_constructible_v = __is_constructible(T, __add_lval_ref_t<const T>);
template <typename T> inline constexpr bool is_move_constructible_v = __is_constructible(T, __add_rval_ref_t<T>);

template <typename T, typename Uxp> inline constexpr bool is_assignable_v = __is_assignable(T, Uxp);
template <typename T>
inline constexpr bool is_copy_assignable_v = __is_assignable(__add_lval_ref_t<T>, __add_lval_ref_t<const T>);
template <typename T>
inline constexpr bool is_move_assignable_v = __is_assignable(__add_lval_ref_t<T>, __add_rval_ref_t<T>);

template <typename T> inline constexpr bool is_destructible_v = is_destructible<T>::value;

template <typename T, typename... Args>
inline constexpr bool is_trivially_constructible_v = __is_trivially_constructible(T, Args...);
template <typename T> inline constexpr bool is_trivially_default_constructible_v = __is_trivially_constructible(T);
template <typename T>
inline constexpr bool is_trivially_copy_constructible_v = __is_trivially_constructible(T, __add_lval_ref_t<const T>);
template <typename T>
inline constexpr bool is_trivially_move_constructible_v = __is_trivially_constructible(T, __add_rval_ref_t<T>);

template <typename T, typename Uxp> inline constexpr bool is_trivially_assignable_v = __is_trivially_assignable(T, Uxp);
template <typename T>
inline constexpr bool is_trivially_copy_assignable_v
    = __is_trivially_assignable(__add_lval_ref_t<T>, __add_lval_ref_t<const T>);
template <typename T>
inline constexpr bool is_trivially_move_assignable_v
    = __is_trivially_assignable(__add_lval_ref_t<T>, __add_rval_ref_t<T>);
template <typename T> inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;

template <typename T, typename... Args>
inline constexpr bool is_nothrow_constructible_v = __is_nothrow_constructible(T, Args...);
template <typename T> inline constexpr bool is_nothrow_default_constructible_v = __is_nothrow_constructible(T);
template <typename T>
inline constexpr bool is_nothrow_copy_constructible_v = __is_nothrow_constructible(T, __add_lval_ref_t<const T>);
template <typename T>
inline constexpr bool is_nothrow_move_constructible_v = __is_nothrow_constructible(T, __add_rval_ref_t<T>);

template <typename T, typename Uxp> inline constexpr bool is_nothrow_assignable_v = __is_nothrow_assignable(T, Uxp);
template <typename T>
inline constexpr bool is_nothrow_copy_assignable_v
    = __is_nothrow_assignable(__add_lval_ref_t<T>, __add_lval_ref_t<const T>);
template <typename T>
inline constexpr bool is_nothrow_move_assignable_v = __is_nothrow_assignable(__add_lval_ref_t<T>, __add_rval_ref_t<T>);

template <typename T> inline constexpr bool is_nothrow_destructible_v = is_nothrow_destructible<T>::value;

template <typename T> inline constexpr bool has_virtual_destructor_v = __has_virtual_destructor(T);

template <typename T> inline constexpr __tt_size_t alignment_of_v = alignment_of<T>::value;

template <typename T> inline constexpr __tt_size_t rank_v = __array_rank(T);
template <typename T, unsigned _Idx = 0> inline constexpr __tt_size_t extent_v = 0;
template <typename T, __tt_size_t _Size> inline constexpr __tt_size_t extent_v<T[_Size], 0> = _Size;
template <typename T, unsigned _Idx, __tt_size_t _Size>
inline constexpr __tt_size_t extent_v<T[_Size], _Idx> = extent_v<T, _Idx - 1>;
template <typename T> inline constexpr __tt_size_t extent_v<T[], 0> = 0;
template <typename T, unsigned _Idx> inline constexpr __tt_size_t extent_v<T[], _Idx> = extent_v<T, _Idx - 1>;

template <typename T, typename Uxp> inline constexpr bool is_same_v = __is_same(T, Uxp);

template <typename _Base, typename _Derived> inline constexpr bool is_base_of_v = __is_base_of(_Base, _Derived);

template <typename F, typename U> inline constexpr bool is_convertible_v = __is_convertible(F, U);

template <typename Fn, typename... Args> inline constexpr bool is_invocable_v = is_invocable<Fn, Args...>::value;
template <typename Fn, typename... Args>
inline constexpr bool is_nothrow_invocable_v = is_nothrow_invocable<Fn, Args...>::value;
template <typename R, typename Fn, typename... Args>
inline constexpr bool is_invocable_r_v = is_invocable_r<R, Fn, Args...>::value;
template <typename R, typename Fn, typename... Args>
inline constexpr bool is_nothrow_invocable_r_v = is_nothrow_invocable_r<R, Fn, Args...>::value;

template <typename T>
struct has_unique_object_representations
    : bool_constant<__has_unique_object_representations(remove_cv_t<remove_all_extents_t<T>>)> {
  static_assert(micron::__is_complete_or_unbounded(__type_identity<T>{}),
                "template argument must be a complete class or an unbounded array");
};

template <typename T>
inline constexpr bool has_unique_object_representations_v = has_unique_object_representations<T>::value;

template <typename T> struct is_aggregate : bool_constant<__is_aggregate(remove_cv_t<T>)> {
};

template <typename T> inline constexpr bool is_aggregate_v = __is_aggregate(remove_cv_t<T>);

// new
template <__tt_size_t... Is> struct index_sequence {
  using type = index_sequence;
  static constexpr __tt_size_t
  size() noexcept
  {
    return sizeof...(Is);
  }
};

// helper to concatenate sequences
template <typename Seq1, typename Seq2> struct concat_index_sequence;

template <__tt_size_t... I1, __tt_size_t... I2>
struct concat_index_sequence<index_sequence<I1...>, index_sequence<I2...>> {
  using type = index_sequence<I1..., (sizeof...(I1) + I2)...>;
};

// make_index_sequence_impl recursively builds sequence 0..N-1
template <__tt_size_t N> struct make_index_sequence_impl {
private:
  using half = typename make_index_sequence_impl<N / 2>::type;
  using rem = typename make_index_sequence_impl<N - N / 2>::type;

public:
  using type = typename concat_index_sequence<half, rem>::type;
};

// base case: N = 0
template <> struct make_index_sequence_impl<0> {
  using type = index_sequence<>;
};

// base case: N = 1
template <> struct make_index_sequence_impl<1> {
  using type = index_sequence<0>;
};

// public alias
template <__tt_size_t N> using make_index_sequence = typename make_index_sequence_impl<N>::type;

// index_sequence_for - creates index sequence with sizeof...(Ts) elements
template <typename... Ts> using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

};
