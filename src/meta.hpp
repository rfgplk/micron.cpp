//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"

#if defined(__micron_reflection)

#include "__special/meta"

#include "concepts.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "memory/addr.hpp"
#include "slice.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// micron::meta
//
// implementation of [meta.reflection]; std compliant semantics
//
// NOTE:
//   .. everything is consteval
//   .. whole header is guarded behind __micron_reflection (so this is includeable under cpp23)
//   .. use micron::reflect if you don't want the raw metafunctions

namespace micron
{
namespace meta
{

// vocabulary
using info = std::meta::info;
using access_context = std::meta::access_context;
using member_offset = std::meta::member_offset;
using operators = std::meta::operators;

// true when micron declared std::meta itself, false when it deferred to libstdc++
#if defined(_MICRON_META_LIBSTDCXX)
inline constexpr bool self_hosted = false;
#else
inline constexpr bool self_hosted = true;
#endif

struct identifier {
  const char *__p = nullptr;
  usize __n = 0;

  constexpr identifier() noexcept = default;

  constexpr identifier(const char *__s, usize __l) noexcept : __p(__s), __n(__l) { }

  constexpr const char *
  data() const noexcept
  {
    return __p;
  }

  constexpr usize
  size() const noexcept
  {
    return __n;
  }

  constexpr bool
  empty() const noexcept
  {
    return __n == 0;
  }

  constexpr char
  operator[](usize __i) const noexcept
  {
    return __p[__i];
  }

  constexpr const char *
  begin() const noexcept
  {
    return __p;
  }

  constexpr const char *
  end() const noexcept
  {
    return __p + __n;
  }
};

constexpr bool
operator==(const identifier &__a, const identifier &__b) noexcept
{
  if ( __a.size() != __b.size() ) return false;
  for ( usize i = 0; i < __a.size(); ++i )
    if ( __a[i] != __b[i] ) return false;
  return true;
}

// NOTE: this has to stay a template. info is consteval; any non template member must itself be conseval (dtors CANT be)
template<typename T> class consteval_list
{
  T *__mem = nullptr;
  usize __len = 0;

public:
  using value_type = T;
  using size_type = usize;
  using const_iterator = const T *;

  constexpr consteval_list() noexcept = default;

  template<typename R> constexpr consteval_list(const R &__r) : __len(__r.size())
  {
    __mem = __len ? new T[__len] : nullptr;
    usize __i = 0;
    for ( auto __e : __r ) __mem[__i++] = __e;
  }

  constexpr consteval_list(const consteval_list &__o) : __len(__o.__len)
  {
    __mem = __len ? new T[__len] : nullptr;
    for ( usize i = 0; i < __len; ++i ) __mem[i] = __o.__mem[i];
  }

  constexpr consteval_list &
  operator=(const consteval_list &__o)
  {
    if ( this == &__o ) return *this;
    if ( __mem ) delete[] __mem;
    __len = __o.__len;
    __mem = __len ? new T[__len] : nullptr;
    for ( usize i = 0; i < __len; ++i ) __mem[i] = __o.__mem[i];
    return *this;
  }

  constexpr ~consteval_list()
  {
    if ( __mem ) delete[] __mem;
  }

  constexpr usize
  size() const noexcept
  {
    return __len;
  }

  constexpr bool
  empty() const noexcept
  {
    return __len == 0;
  }

  constexpr const T *
  begin() const noexcept
  {
    return __mem;
  }

  constexpr const T *
  end() const noexcept
  {
    return __mem + __len;
  }

  constexpr T
  operator[](usize __i) const noexcept
  {
    return __mem[__i];
  }
};

using info_list = consteval_list<info>;

template<typename R>
concept reflection_range = requires(R &&__r) {
  __r.begin();
  __r.end();
  requires micron::is_same_v<micron::remove_cvref_t<decltype(*__r.begin())>, info>;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// predicates over any reflection

consteval bool
has_identifier(info __r)
{
  return std::meta::has_identifier(__r);
}

consteval bool
is_public(info __r)
{
  return std::meta::is_public(__r);
}

consteval bool
is_protected(info __r)
{
  return std::meta::is_protected(__r);
}

consteval bool
is_private(info __r)
{
  return std::meta::is_private(__r);
}

consteval bool
is_virtual(info __r)
{
  return std::meta::is_virtual(__r);
}

consteval bool
is_pure_virtual(info __r)
{
  return std::meta::is_pure_virtual(__r);
}

consteval bool
is_override(info __r)
{
  return std::meta::is_override(__r);
}

consteval bool
is_final(info __r)
{
  return std::meta::is_final(__r);
}

consteval bool
is_deleted(info __r)
{
  return std::meta::is_deleted(__r);
}

consteval bool
is_defaulted(info __r)
{
  return std::meta::is_defaulted(__r);
}

consteval bool
is_user_provided(info __r)
{
  return std::meta::is_user_provided(__r);
}

consteval bool
is_user_declared(info __r)
{
  return std::meta::is_user_declared(__r);
}

consteval bool
is_explicit(info __r)
{
  return std::meta::is_explicit(__r);
}

consteval bool
is_noexcept(info __r)
{
  return std::meta::is_noexcept(__r);
}

consteval bool
is_bit_field(info __r)
{
  return std::meta::is_bit_field(__r);
}

consteval bool
is_enumerator(info __r)
{
  return std::meta::is_enumerator(__r);
}

consteval bool
is_annotation(info __r)
{
  return std::meta::is_annotation(__r);
}

consteval bool
is_const(info __r)
{
  return std::meta::is_const(__r);
}

consteval bool
is_volatile(info __r)
{
  return std::meta::is_volatile(__r);
}

consteval bool
is_mutable_member(info __r)
{
  return std::meta::is_mutable_member(__r);
}

consteval bool
is_lvalue_reference_qualified(info __r)
{
  return std::meta::is_lvalue_reference_qualified(__r);
}

consteval bool
is_rvalue_reference_qualified(info __r)
{
  return std::meta::is_rvalue_reference_qualified(__r);
}

consteval bool
has_static_storage_duration(info __r)
{
  return std::meta::has_static_storage_duration(__r);
}

consteval bool
has_thread_storage_duration(info __r)
{
  return std::meta::has_thread_storage_duration(__r);
}

consteval bool
has_automatic_storage_duration(info __r)
{
  return std::meta::has_automatic_storage_duration(__r);
}

consteval bool
has_internal_linkage(info __r)
{
  return std::meta::has_internal_linkage(__r);
}

consteval bool
has_module_linkage(info __r)
{
  return std::meta::has_module_linkage(__r);
}

consteval bool
has_external_linkage(info __r)
{
  return std::meta::has_external_linkage(__r);
}

consteval bool
has_c_language_linkage(info __r)
{
  return std::meta::has_c_language_linkage(__r);
}

consteval bool
has_linkage(info __r)
{
  return std::meta::has_linkage(__r);
}

consteval bool
is_complete_type(info __r)
{
  return std::meta::is_complete_type(__r);
}

consteval bool
is_enumerable_type(info __r)
{
  return std::meta::is_enumerable_type(__r);
}

consteval bool
is_variable(info __r)
{
  return std::meta::is_variable(__r);
}

consteval bool
is_type(info __r)
{
  return std::meta::is_type(__r);
}

consteval bool
is_namespace(info __r)
{
  return std::meta::is_namespace(__r);
}

consteval bool
is_type_alias(info __r)
{
  return std::meta::is_type_alias(__r);
}

consteval bool
is_namespace_alias(info __r)
{
  return std::meta::is_namespace_alias(__r);
}

consteval bool
is_function(info __r)
{
  return std::meta::is_function(__r);
}

consteval bool
is_conversion_function(info __r)
{
  return std::meta::is_conversion_function(__r);
}

consteval bool
is_operator_function(info __r)
{
  return std::meta::is_operator_function(__r);
}

consteval bool
is_literal_operator(info __r)
{
  return std::meta::is_literal_operator(__r);
}

consteval bool
is_special_member_function(info __r)
{
  return std::meta::is_special_member_function(__r);
}

consteval bool
is_constructor(info __r)
{
  return std::meta::is_constructor(__r);
}

consteval bool
is_default_constructor(info __r)
{
  return std::meta::is_default_constructor(__r);
}

consteval bool
is_copy_constructor(info __r)
{
  return std::meta::is_copy_constructor(__r);
}

consteval bool
is_move_constructor(info __r)
{
  return std::meta::is_move_constructor(__r);
}

consteval bool
is_assignment(info __r)
{
  return std::meta::is_assignment(__r);
}

consteval bool
is_copy_assignment(info __r)
{
  return std::meta::is_copy_assignment(__r);
}

consteval bool
is_move_assignment(info __r)
{
  return std::meta::is_move_assignment(__r);
}

consteval bool
is_destructor(info __r)
{
  return std::meta::is_destructor(__r);
}

consteval bool
is_function_parameter(info __r)
{
  return std::meta::is_function_parameter(__r);
}

consteval bool
is_explicit_object_parameter(info __r)
{
  return std::meta::is_explicit_object_parameter(__r);
}

consteval bool
has_default_argument(info __r)
{
  return std::meta::has_default_argument(__r);
}

consteval bool
is_vararg_function(info __r)
{
  return std::meta::is_vararg_function(__r);
}

consteval bool
is_template(info __r)
{
  return std::meta::is_template(__r);
}

consteval bool
is_function_template(info __r)
{
  return std::meta::is_function_template(__r);
}

consteval bool
is_variable_template(info __r)
{
  return std::meta::is_variable_template(__r);
}

consteval bool
is_class_template(info __r)
{
  return std::meta::is_class_template(__r);
}

consteval bool
is_alias_template(info __r)
{
  return std::meta::is_alias_template(__r);
}

consteval bool
is_conversion_function_template(info __r)
{
  return std::meta::is_conversion_function_template(__r);
}

consteval bool
is_operator_function_template(info __r)
{
  return std::meta::is_operator_function_template(__r);
}

consteval bool
is_literal_operator_template(info __r)
{
  return std::meta::is_literal_operator_template(__r);
}

consteval bool
is_constructor_template(info __r)
{
  return std::meta::is_constructor_template(__r);
}

consteval bool
is_concept(info __r)
{
  return std::meta::is_concept(__r);
}

consteval bool
is_value(info __r)
{
  return std::meta::is_value(__r);
}

consteval bool
is_object(info __r)
{
  return std::meta::is_object(__r);
}

consteval bool
is_structured_binding(info __r)
{
  return std::meta::is_structured_binding(__r);
}

consteval bool
is_class_member(info __r)
{
  return std::meta::is_class_member(__r);
}

consteval bool
is_namespace_member(info __r)
{
  return std::meta::is_namespace_member(__r);
}

consteval bool
is_nonstatic_data_member(info __r)
{
  return std::meta::is_nonstatic_data_member(__r);
}

consteval bool
is_static_member(info __r)
{
  return std::meta::is_static_member(__r);
}

consteval bool
is_base(info __r)
{
  return std::meta::is_base(__r);
}

consteval bool
has_default_member_initializer(info __r)
{
  return std::meta::has_default_member_initializer(__r);
}

consteval bool
has_parent(info __r)
{
  return std::meta::has_parent(__r);
}

consteval bool
has_template_arguments(info __r)
{
  return std::meta::has_template_arguments(__r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// predicates over a type reflection

consteval bool
is_void_type(info __r)
{
  return std::meta::is_void_type(__r);
}

consteval bool
is_null_pointer_type(info __r)
{
  return std::meta::is_null_pointer_type(__r);
}

consteval bool
is_integral_type(info __r)
{
  return std::meta::is_integral_type(__r);
}

consteval bool
is_floating_point_type(info __r)
{
  return std::meta::is_floating_point_type(__r);
}

consteval bool
is_array_type(info __r)
{
  return std::meta::is_array_type(__r);
}

consteval bool
is_pointer_type(info __r)
{
  return std::meta::is_pointer_type(__r);
}

consteval bool
is_lvalue_reference_type(info __r)
{
  return std::meta::is_lvalue_reference_type(__r);
}

consteval bool
is_rvalue_reference_type(info __r)
{
  return std::meta::is_rvalue_reference_type(__r);
}

consteval bool
is_member_object_pointer_type(info __r)
{
  return std::meta::is_member_object_pointer_type(__r);
}

consteval bool
is_member_function_pointer_type(info __r)
{
  return std::meta::is_member_function_pointer_type(__r);
}

consteval bool
is_enum_type(info __r)
{
  return std::meta::is_enum_type(__r);
}

consteval bool
is_union_type(info __r)
{
  return std::meta::is_union_type(__r);
}

consteval bool
is_class_type(info __r)
{
  return std::meta::is_class_type(__r);
}

consteval bool
is_function_type(info __r)
{
  return std::meta::is_function_type(__r);
}

consteval bool
is_reflection_type(info __r)
{
  return std::meta::is_reflection_type(__r);
}

consteval bool
is_reference_type(info __r)
{
  return std::meta::is_reference_type(__r);
}

consteval bool
is_arithmetic_type(info __r)
{
  return std::meta::is_arithmetic_type(__r);
}

consteval bool
is_fundamental_type(info __r)
{
  return std::meta::is_fundamental_type(__r);
}

consteval bool
is_object_type(info __r)
{
  return std::meta::is_object_type(__r);
}

consteval bool
is_scalar_type(info __r)
{
  return std::meta::is_scalar_type(__r);
}

consteval bool
is_compound_type(info __r)
{
  return std::meta::is_compound_type(__r);
}

consteval bool
is_member_pointer_type(info __r)
{
  return std::meta::is_member_pointer_type(__r);
}

consteval bool
is_const_type(info __r)
{
  return std::meta::is_const_type(__r);
}

consteval bool
is_volatile_type(info __r)
{
  return std::meta::is_volatile_type(__r);
}

consteval bool
is_trivially_copyable_type(info __r)
{
  return std::meta::is_trivially_copyable_type(__r);
}

consteval bool
is_standard_layout_type(info __r)
{
  return std::meta::is_standard_layout_type(__r);
}

consteval bool
is_empty_type(info __r)
{
  return std::meta::is_empty_type(__r);
}

consteval bool
is_polymorphic_type(info __r)
{
  return std::meta::is_polymorphic_type(__r);
}

consteval bool
is_abstract_type(info __r)
{
  return std::meta::is_abstract_type(__r);
}

consteval bool
is_final_type(info __r)
{
  return std::meta::is_final_type(__r);
}

consteval bool
is_aggregate_type(info __r)
{
  return std::meta::is_aggregate_type(__r);
}

consteval bool
is_structural_type(info __r)
{
  return std::meta::is_structural_type(__r);
}

consteval bool
is_signed_type(info __r)
{
  return std::meta::is_signed_type(__r);
}

consteval bool
is_unsigned_type(info __r)
{
  return std::meta::is_unsigned_type(__r);
}

consteval bool
is_bounded_array_type(info __r)
{
  return std::meta::is_bounded_array_type(__r);
}

consteval bool
is_unbounded_array_type(info __r)
{
  return std::meta::is_unbounded_array_type(__r);
}

consteval bool
is_scoped_enum_type(info __r)
{
  return std::meta::is_scoped_enum_type(__r);
}

consteval bool
is_default_constructible_type(info __r)
{
  return std::meta::is_default_constructible_type(__r);
}

consteval bool
is_copy_constructible_type(info __r)
{
  return std::meta::is_copy_constructible_type(__r);
}

consteval bool
is_move_constructible_type(info __r)
{
  return std::meta::is_move_constructible_type(__r);
}

consteval bool
is_copy_assignable_type(info __r)
{
  return std::meta::is_copy_assignable_type(__r);
}

consteval bool
is_move_assignable_type(info __r)
{
  return std::meta::is_move_assignable_type(__r);
}

consteval bool
is_swappable_type(info __r)
{
  return std::meta::is_swappable_type(__r);
}

consteval bool
is_destructible_type(info __r)
{
  return std::meta::is_destructible_type(__r);
}

consteval bool
is_trivially_default_constructible_type(info __r)
{
  return std::meta::is_trivially_default_constructible_type(__r);
}

consteval bool
is_trivially_copy_constructible_type(info __r)
{
  return std::meta::is_trivially_copy_constructible_type(__r);
}

consteval bool
is_trivially_move_constructible_type(info __r)
{
  return std::meta::is_trivially_move_constructible_type(__r);
}

consteval bool
is_trivially_copy_assignable_type(info __r)
{
  return std::meta::is_trivially_copy_assignable_type(__r);
}

consteval bool
is_trivially_move_assignable_type(info __r)
{
  return std::meta::is_trivially_move_assignable_type(__r);
}

consteval bool
is_trivially_destructible_type(info __r)
{
  return std::meta::is_trivially_destructible_type(__r);
}

consteval bool
is_nothrow_default_constructible_type(info __r)
{
  return std::meta::is_nothrow_default_constructible_type(__r);
}

consteval bool
is_nothrow_copy_constructible_type(info __r)
{
  return std::meta::is_nothrow_copy_constructible_type(__r);
}

consteval bool
is_nothrow_move_constructible_type(info __r)
{
  return std::meta::is_nothrow_move_constructible_type(__r);
}

consteval bool
is_nothrow_copy_assignable_type(info __r)
{
  return std::meta::is_nothrow_copy_assignable_type(__r);
}

consteval bool
is_nothrow_move_assignable_type(info __r)
{
  return std::meta::is_nothrow_move_assignable_type(__r);
}

consteval bool
is_nothrow_swappable_type(info __r)
{
  return std::meta::is_nothrow_swappable_type(__r);
}

consteval bool
is_nothrow_destructible_type(info __r)
{
  return std::meta::is_nothrow_destructible_type(__r);
}

consteval bool
is_implicit_lifetime_type(info __r)
{
  return std::meta::is_implicit_lifetime_type(__r);
}

consteval bool
has_virtual_destructor(info __r)
{
  return std::meta::has_virtual_destructor(__r);
}

consteval bool
has_unique_object_representations(info __r)
{
  return std::meta::has_unique_object_representations(__r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// binary type relations

consteval bool
is_assignable_type(info __a, info __b)
{
  return std::meta::is_assignable_type(__a, __b);
}

consteval bool
is_swappable_with_type(info __a, info __b)
{
  return std::meta::is_swappable_with_type(__a, __b);
}

consteval bool
is_trivially_assignable_type(info __a, info __b)
{
  return std::meta::is_trivially_assignable_type(__a, __b);
}

consteval bool
is_nothrow_assignable_type(info __a, info __b)
{
  return std::meta::is_nothrow_assignable_type(__a, __b);
}

consteval bool
is_nothrow_swappable_with_type(info __a, info __b)
{
  return std::meta::is_nothrow_swappable_with_type(__a, __b);
}

consteval bool
reference_constructs_from_temporary(info __a, info __b)
{
  return std::meta::reference_constructs_from_temporary(__a, __b);
}

consteval bool
reference_converts_from_temporary(info __a, info __b)
{
  return std::meta::reference_converts_from_temporary(__a, __b);
}

consteval bool
is_same_type(info __a, info __b)
{
  return std::meta::is_same_type(__a, __b);
}

consteval bool
is_base_of_type(info __a, info __b)
{
  return std::meta::is_base_of_type(__a, __b);
}

consteval bool
is_virtual_base_of_type(info __a, info __b)
{
  return std::meta::is_virtual_base_of_type(__a, __b);
}

consteval bool
is_convertible_type(info __a, info __b)
{
  return std::meta::is_convertible_type(__a, __b);
}

consteval bool
is_nothrow_convertible_type(info __a, info __b)
{
  return std::meta::is_nothrow_convertible_type(__a, __b);
}

consteval bool
is_layout_compatible_type(info __a, info __b)
{
  return std::meta::is_layout_compatible_type(__a, __b);
}

consteval bool
is_pointer_interconvertible_base_of_type(info __a, info __b)
{
  return std::meta::is_pointer_interconvertible_base_of_type(__a, __b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// access queries

consteval bool
is_accessible(info __r, access_context __c)
{
  return std::meta::is_accessible(__r, __c);
}

consteval bool
has_inaccessible_nonstatic_data_members(info __r, access_context __c)
{
  return std::meta::has_inaccessible_nonstatic_data_members(__r, __c);
}

consteval bool
has_inaccessible_bases(info __r, access_context __c)
{
  return std::meta::has_inaccessible_bases(__r, __c);
}

consteval bool
has_inaccessible_subobjects(info __r, access_context __c)
{
  return std::meta::has_inaccessible_subobjects(__r, __c);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reflection -> reflection

consteval info
type_of(info __r)
{
  return std::meta::type_of(__r);
}

consteval info
object_of(info __r)
{
  return std::meta::object_of(__r);
}

consteval info
constant_of(info __r)
{
  return std::meta::constant_of(__r);
}

consteval info
parent_of(info __r)
{
  return std::meta::parent_of(__r);
}

consteval info
dealias(info __r)
{
  return std::meta::dealias(__r);
}

consteval info
template_of(info __r)
{
  return std::meta::template_of(__r);
}

consteval info
variable_of(info __r)
{
  return std::meta::variable_of(__r);
}

consteval info
return_type_of(info __r)
{
  return std::meta::return_type_of(__r);
}

consteval info
remove_const(info __r)
{
  return std::meta::remove_const(__r);
}

consteval info
remove_volatile(info __r)
{
  return std::meta::remove_volatile(__r);
}

consteval info
remove_cv(info __r)
{
  return std::meta::remove_cv(__r);
}

consteval info
add_const(info __r)
{
  return std::meta::add_const(__r);
}

consteval info
add_volatile(info __r)
{
  return std::meta::add_volatile(__r);
}

consteval info
add_cv(info __r)
{
  return std::meta::add_cv(__r);
}

consteval info
remove_reference(info __r)
{
  return std::meta::remove_reference(__r);
}

consteval info
add_lvalue_reference(info __r)
{
  return std::meta::add_lvalue_reference(__r);
}

consteval info
add_rvalue_reference(info __r)
{
  return std::meta::add_rvalue_reference(__r);
}

consteval info
make_signed(info __r)
{
  return std::meta::make_signed(__r);
}

consteval info
make_unsigned(info __r)
{
  return std::meta::make_unsigned(__r);
}

consteval info
remove_extent(info __r)
{
  return std::meta::remove_extent(__r);
}

consteval info
remove_all_extents(info __r)
{
  return std::meta::remove_all_extents(__r);
}

consteval info
remove_pointer(info __r)
{
  return std::meta::remove_pointer(__r);
}

consteval info
add_pointer(info __r)
{
  return std::meta::add_pointer(__r);
}

consteval info
remove_cvref(info __r)
{
  return std::meta::remove_cvref(__r);
}

consteval info
decay(info __r)
{
  return std::meta::decay(__r);
}

consteval info
underlying_type(info __r)
{
  return std::meta::underlying_type(__r);
}

consteval info
unwrap_reference(info __r)
{
  return std::meta::unwrap_reference(__r);
}

consteval info
unwrap_ref_decay(info __r)
{
  return std::meta::unwrap_ref_decay(__r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scope identification

consteval info
current_function(void)
{
  return std::meta::current_function();
}

consteval info
current_class(void)
{
  return std::meta::current_class();
}

consteval info
current_namespace(void)
{
  return std::meta::current_namespace();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// layout

consteval usize
size_of(info __r)
{
  return std::meta::size_of(__r);
}

consteval usize
alignment_of(info __r)
{
  return std::meta::alignment_of(__r);
}

consteval usize
bit_size_of(info __r)
{
  return std::meta::bit_size_of(__r);
}

consteval usize
rank(info __r)
{
  return std::meta::rank(__r);
}

consteval usize
tuple_size(info __r)
{
  return std::meta::tuple_size(__r);
}

consteval usize
variant_size(info __r)
{
  return std::meta::variant_size(__r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// names

consteval identifier
identifier_of(info __r)
{
  const auto __s = std::meta::identifier_of(__r);
  return identifier{ __s.data(), __s.size() };
}

consteval identifier
display_string_of(info __r)
{
  const auto __s = std::meta::display_string_of(__r);
  return identifier{ __s.data(), __s.size() };
}

consteval identifier
symbol_of(operators __o)
{
  const auto __s = std::meta::symbol_of(__o);
  return identifier{ __s.data(), __s.size() };
}

consteval operators
operator_of(info __r)
{
  return std::meta::operator_of(__r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// layout

consteval member_offset
offset_of(info __r)
{
  return std::meta::offset_of(__r);
}

consteval usize
extent(info __r, unsigned __i = 0)
{
  return std::meta::extent(__r, __i);
}

consteval info
tuple_element(usize __i, info __r)
{
  return std::meta::tuple_element(__i, __r);
}

consteval info
variant_alternative(usize __i, info __r)
{
  return std::meta::variant_alternative(__i, __r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// member queries

consteval info_list
members_of(info __r, access_context __c)
{
  return info_list{ std::meta::members_of(__r, __c) };
}

consteval info_list
bases_of(info __r, access_context __c)
{
  return info_list{ std::meta::bases_of(__r, __c) };
}

consteval info_list
static_data_members_of(info __r, access_context __c)
{
  return info_list{ std::meta::static_data_members_of(__r, __c) };
}

consteval info_list
nonstatic_data_members_of(info __r, access_context __c)
{
  return info_list{ std::meta::nonstatic_data_members_of(__r, __c) };
}

consteval info_list
subobjects_of(info __r, access_context __c)
{
  return info_list{ std::meta::subobjects_of(__r, __c) };
}

consteval info_list
enumerators_of(info __r)
{
  return info_list{ std::meta::enumerators_of(__r) };
}

consteval info_list
template_arguments_of(info __r)
{
  return info_list{ std::meta::template_arguments_of(__r) };
}

consteval info_list
parameters_of(info __r)
{
  return info_list{ std::meta::parameters_of(__r) };
}

consteval info_list
annotations_of(info __r)
{
  return info_list{ std::meta::annotations_of(__r) };
}

consteval info_list
annotations_of_with_type(info __r, info __t)
{
  return info_list{ std::meta::annotations_of_with_type(__r, __t) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// extraction, substitution, result reflection

template<typename T>
consteval T
extract(info __r)
{
  return std::meta::extract<T>(__r);
}

template<reflection_range R = std::initializer_list<info>>
consteval bool
can_substitute(info __r, R &&__args)
{
  return std::meta::can_substitute(__r, static_cast<R &&>(__args));
}

template<reflection_range R = std::initializer_list<info>>
consteval info
substitute(info __r, R &&__args)
{
  return std::meta::substitute(__r, static_cast<R &&>(__args));
}

template<typename T>
consteval info
reflect_constant(T __v)
{
  return std::meta::reflect_constant(__v);
}

template<typename T>
consteval info
reflect_object(T &__v)
{
  return std::meta::reflect_object(__v);
}

template<typename T>
consteval info
reflect_function(T &__v)
{
  return std::meta::reflect_function(__v);
}

template<typename R>
consteval info
reflect_constant_string(R &&__r)
{
  return std::meta::reflect_constant_string(static_cast<R &&>(__r));
}

template<typename R>
consteval info
reflect_constant_array(R &&__r)
{
  return std::meta::reflect_constant_array(static_cast<R &&>(__r));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// [meta.define.static]
template<typename R>
consteval auto
define_static_array(R &&__r)
{
  using T = micron::remove_cvref_t<decltype(*__r.begin())>;
  // an empty range makes reflect_constant_array link against std::array, which the selfhosted branch
  // deliberately does not declare
  if ( __r.begin() == __r.end() ) return micron::raw_slice<const T>{};
  const auto __a = std::meta::reflect_constant_array(static_cast<R &&>(__r));
  const auto __t = std::meta::type_of(__a);
  if ( std::meta::is_array_type(__t) ) return micron::raw_slice<const T>{ std::meta::extract<const T *>(__a), std::meta::extent(__t, 0u) };
  return micron::raw_slice<const T>{};
}

template<typename R>
consteval auto
define_static_string(R &&__r)
{
  using T = micron::remove_cvref_t<decltype(*__r.begin())>;
  return std::meta::extract<const T *>(std::meta::reflect_constant_string(static_cast<R &&>(__r)));
}

template<typename T>
consteval const micron::remove_cvref_t<T> *
define_static_object(T &&__v)
{
  using U = micron::remove_cvref_t<T>;
  return __builtin_addressof(std::meta::extract<const U &>(std::meta::reflect_constant(static_cast<T &&>(__v))));
}

};      // namespace meta
};      // namespace micron

#endif      // __micron_reflection
