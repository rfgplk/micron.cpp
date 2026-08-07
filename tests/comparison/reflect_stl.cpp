//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <meta>
#include <vector>

#include "../../src/meta.hpp"
#include "../../src/reflect.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#if !defined(__micron_reflection)

int
main(void)
{
  snowball::print("reflect_stl: built without -freflection, nothing to exercise");
  return 1;
}

#else

#include <string_view>
#include <type_traits>

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mm = micron::meta;
namespace mr = micron::reflect;

namespace
{

struct alpha {
  int a;
  double b;
  char c;
};

struct beta {
  alpha inner;
  u64 tag;
  const char *name;
};

struct base_one {
  int p;
};

struct base_two {
  int q;
};

struct child: base_one, base_two {
  short r;
};

enum class hue : u8 { red, green, blue };
enum class empty_enum : u32 { };

struct nothing {
};

constexpr auto mctx = mm::access_context::unchecked();
constexpr auto sctx = std::meta::access_context::unchecked();

consteval usize
m_fields(mm::info r)
{
  return mm::nonstatic_data_members_of(r, mctx).size();
}

consteval usize
s_fields(std::meta::info r)
{
  return std::meta::nonstatic_data_members_of(r, sctx).size();
}

consteval bool
names_agree(mm::info r)
{
  const auto mine = mm::identifier_of(r);
  const auto theirs = std::meta::identifier_of(r);
  if ( mine.size() != theirs.size() ) return false;
  for ( usize i = 0; i < mine.size(); ++i )
    if ( mine[i] != theirs[i] ) return false;
  return true;
}

}      // namespace

int
main(void)
{
  print("=== REFLECT vs STL ===");

  test_case("the libstdc++ branch is the one that is live");
  {

    static_assert(!mm::self_hosted);
    std::vector<int> v;
    v.push_back(3);
    v.push_back(4);
    require_true(v.size() == 2 and v[1] == 4);
    require_true(!mm::self_hosted);
  }
  end_test_case();

  test_case("micron::meta::info IS std::meta::info");
  {
    static_assert(micron::is_same_v<mm::info, std::meta::info>);
    static_assert(std::is_same_v<mm::info, std::meta::info>);
    require_true(true);
  }
  end_test_case();

  test_case("member counts agree on every subject");
  {
    static_assert(m_fields(^^alpha) == s_fields(^^alpha));
    static_assert(m_fields(^^beta) == s_fields(^^beta));
    static_assert(m_fields(^^child) == s_fields(^^child));
    static_assert(m_fields(^^nothing) == s_fields(^^nothing));
    static_assert(m_fields(^^alpha) == 3 and m_fields(^^nothing) == 0);
    require_true(m_fields(^^beta) == s_fields(^^beta));
  }
  end_test_case();

  test_case("identifier_of agrees, character for character");
  {
    static_assert(names_agree(^^alpha));
    static_assert(names_agree(^^alpha::a));
    static_assert(names_agree(^^beta::inner));
    static_assert(names_agree(^^beta::name));
    static_assert(names_agree(^^hue));
    static_assert(names_agree(^^child::r));
    require_true(names_agree(^^beta::tag));
  }
  end_test_case();

  test_case("layout queries agree, and agree with the builtins");
  {
    static_assert(mm::size_of(^^alpha) == std::meta::size_of(^^alpha));
    static_assert(mm::size_of(^^alpha) == sizeof(alpha));
    static_assert(mm::alignment_of(^^beta) == std::meta::alignment_of(^^beta));
    static_assert(mm::alignment_of(^^beta) == alignof(beta));
    static_assert(mm::offset_of(^^alpha::b).bytes == std::meta::offset_of(^^alpha::b).bytes);
    static_assert(mm::offset_of(^^alpha::b).bytes == (ptrdiff_t)__builtin_offsetof(alpha, b));
    static_assert(mm::bit_size_of(^^alpha::a) == std::meta::bit_size_of(^^alpha::a));
    require_true(mm::size_of(^^beta) == std::meta::size_of(^^beta));
  }
  end_test_case();

  test_case("the type-trait mirrors agree with std::meta AND with <type_traits>");
  {
    static_assert(mm::is_aggregate_type(^^alpha) == std::meta::is_aggregate_type(^^alpha));
    static_assert(mm::is_aggregate_type(^^alpha) == std::is_aggregate_v<alpha>);
    static_assert(mm::is_class_type(^^beta) == std::is_class_v<beta>);
    static_assert(mm::is_enum_type(^^hue) == std::is_enum_v<hue>);
    static_assert(mm::is_scoped_enum_type(^^hue) == std::is_scoped_enum_v<hue>);
    static_assert(mm::is_empty_type(^^nothing) == std::is_empty_v<nothing>);
    static_assert(mm::is_trivially_copyable_type(^^alpha) == std::is_trivially_copyable_v<alpha>);
    static_assert(mm::is_standard_layout_type(^^alpha) == std::is_standard_layout_v<alpha>);
    static_assert(mm::is_polymorphic_type(^^alpha) == std::is_polymorphic_v<alpha>);
    static_assert(mm::is_base_of_type(^^base_one, ^^child) == std::is_base_of_v<base_one, child>);
    static_assert(mm::has_unique_object_representations(^^u64) == std::has_unique_object_representations_v<u64>);

    static_assert(mm::is_aggregate_type(^^alpha) == micron::is_aggregate_v<alpha>);
    static_assert(mm::is_base_of_type(^^base_one, ^^child) == micron::is_base_of_v<base_one, child>);
    require_true(mm::is_trivially_copyable_type(^^alpha) == std::is_trivially_copyable_v<alpha>);
  }
  end_test_case();

  test_case("transformations agree with std::meta AND with <type_traits>");
  {
    static_assert(mm::is_same_type(mm::remove_cv(^^const int), ^^std::remove_cv_t<const int>));
    static_assert(mm::is_same_type(mm::remove_cvref(^^const int &), ^^std::remove_cvref_t<const int &>));
    static_assert(mm::is_same_type(mm::add_pointer(^^int), ^^std::add_pointer_t<int>));
    static_assert(mm::is_same_type(mm::decay(^^int const &), ^^std::decay_t<int const &>));
    static_assert(mm::is_same_type(mm::underlying_type(^^hue), ^^std::underlying_type_t<hue>));
    static_assert(mm::is_same_type(mm::remove_cvref(^^const int &), std::meta::remove_cvref(^^const int &)));
    require_true(mm::is_same_type(mm::underlying_type(^^hue), ^^u8));
  }
  end_test_case();

  test_case("enumerators agree in count, order and value");
  {
    static_assert(mm::enumerators_of(^^hue).size() == std::meta::enumerators_of(^^hue).size());
    static_assert(mm::enumerators_of(^^empty_enum).size() == std::meta::enumerators_of(^^empty_enum).size());
    constexpr auto mine = mm::define_static_array(mm::enumerators_of(^^hue));
    constexpr auto theirs = std::define_static_array(std::meta::enumerators_of(^^hue));
    static_assert(mine.len == theirs.size());
    static_assert(mine.ptr[0] == theirs[0] and mine.ptr[1] == theirs[1] and mine.ptr[2] == theirs[2]);
    constexpr usize n = mine.len;
    require_true(n == 3);
  }
  end_test_case();

  test_case("member lists agree element for element");
  {
    constexpr auto mine = mm::define_static_array(mm::nonstatic_data_members_of(^^beta, mctx));
    constexpr auto theirs = std::define_static_array(std::meta::nonstatic_data_members_of(^^beta, sctx));
    static_assert(mine.len == theirs.size());
    static_assert(mine.ptr[0] == theirs[0]);
    static_assert(mine.ptr[1] == theirs[1]);
    static_assert(mine.ptr[2] == theirs[2]);
    constexpr usize n = mine.len;
    require_true(n == 3);
  }
  end_test_case();

  test_case("bases agree, including the empty case");
  {
    constexpr auto mine = mm::define_static_array(mm::bases_of(^^child, mctx));
    constexpr auto theirs = std::define_static_array(std::meta::bases_of(^^child, sctx));
    static_assert(mine.len == theirs.size() and mine.len == 2);
    static_assert(mine.ptr[0] == theirs[0] and mine.ptr[1] == theirs[1]);

    constexpr auto none_m = mm::define_static_array(mm::bases_of(^^alpha, mctx));
    constexpr auto none_s = std::define_static_array(std::meta::bases_of(^^alpha, sctx));
    static_assert(none_m.len == none_s.size() and none_m.len == 0);
    constexpr usize n = mine.len;
    require_true(n == 2);
  }
  end_test_case();

  test_case("define_static_string matches std::define_static_string");
  {
    static constexpr const char *mine = mm::define_static_string(mm::identifier_of(^^alpha));
    static constexpr const char *theirs = std::define_static_string(std::meta::identifier_of(^^alpha));
    require_true(std::string_view{ mine } == std::string_view{ theirs });
    require_true(std::string_view{ mine } == "alpha");
  }
  end_test_case();

  test_case("the porcelain still works on the libstdc++ branch");
  {
    static_assert(mr::field_count<alpha> == 3);
    static_assert(mr::field_offset<1, alpha>() == __builtin_offsetof(alpha, b));
    static_assert(micron::is_same_v<mr::field_type<1, alpha>, double>);
    alpha x{ 1, 2.0, 'z' }, y{ 1, 2.0, 'z' }, z{ 9, 2.0, 'z' };
    require_true(mr::equal(x, y) and !mr::equal(x, z));
    require_true(mr::hash(x) == mr::hash(y));
    require_true(std::string_view{ mr::enum_name(hue::green).data(), mr::enum_name(hue::green).size() } == "green");
    usize seen = 0;
    mr::for_each_field(x, [&](auto, auto &) { ++seen; });
    require_true(seen == 3);
  }
  end_test_case();

  print("=== REFLECT vs STL PASSED ===");
  return 1;
}

#endif
