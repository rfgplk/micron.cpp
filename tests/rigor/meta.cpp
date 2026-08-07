//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// rigor: micron::meta, the c++26 [meta.reflection] surface.

#include "../../src/meta.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#if !defined(__micron_reflection)

int
main(void)
{
  snowball::print("meta: built without -freflection, nothing to exercise");
  return 1;
}

#else

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mm = micron::meta;

namespace
{

struct plain {
  int a;
  double b;
  char c;
};

struct nested {
  plain p;
  u64 tag;
};

struct empty {
};

struct single {
  i32 only;
};

struct bits {
  u32 lo : 3;
  u32 hi : 5;
  u32 rest;
};

struct base_a {
  int x;
};

struct base_b {
  int y;
};

struct derived: base_a, base_b {
  int z;
};

enum class colour : u8 { red = 0, green = 1, blue = 2 };

enum flags_e : u32 { f_none = 0, f_one = 1, f_two = 2, f_four = 4 };

template<typename T> struct boxed {
  T v;
};

template<typename T>
  requires micron::is_integral_v<T>
struct only_int {
  T v;
};

constexpr auto ctx = mm::access_context::unchecked();

consteval usize
nfields(mm::info r)
{
  return mm::nonstatic_data_members_of(r, ctx).size();
}

constexpr auto plain_m = mm::define_static_array(mm::nonstatic_data_members_of(^^plain, ctx));

}      // namespace

int
main(void)
{
  print("=== META RIGOR ===");

  test_case("arity + declaration order");
  {
    static_assert(nfields(^^plain) == 3);
    static_assert(nfields(^^nested) == 2);
    static_assert(nfields(^^empty) == 0);
    static_assert(nfields(^^single) == 1);
    static_assert(nfields(^^derived) == 1);

    static_assert(mm::identifier_of(plain_m.ptr[0]) == mm::identifier{ "a", 1 });
    static_assert(mm::identifier_of(plain_m.ptr[1]) == mm::identifier{ "b", 1 });
    static_assert(mm::identifier_of(plain_m.ptr[2]) == mm::identifier{ "c", 1 });

    constexpr usize n = plain_m.len;
    require_true(n == 3);
  }
  end_test_case();

  test_case("size_of / alignment_of vs sizeof / alignof");
  {
    static_assert(mm::size_of(^^plain) == sizeof(plain));
    static_assert(mm::size_of(^^nested) == sizeof(nested));
    static_assert(mm::size_of(^^u64) == sizeof(u64));
    static_assert(mm::alignment_of(^^plain) == alignof(plain));
    static_assert(mm::alignment_of(^^nested) == alignof(nested));
    static_assert(mm::size_of(^^derived) == sizeof(derived));
    require_true(mm::size_of(^^plain) == sizeof(plain));
  }
  end_test_case();

  test_case("offset_of vs __builtin_offsetof");
  {
    static_assert(mm::offset_of(^^plain::a).bytes == (ptrdiff_t)__builtin_offsetof(plain, a));
    static_assert(mm::offset_of(^^plain::b).bytes == (ptrdiff_t)__builtin_offsetof(plain, b));
    static_assert(mm::offset_of(^^plain::c).bytes == (ptrdiff_t)__builtin_offsetof(plain, c));
    static_assert(mm::offset_of(^^nested::tag).bytes == (ptrdiff_t)__builtin_offsetof(nested, tag));
    static_assert(mm::offset_of(^^plain::a).bits == 0);
    require_true(mm::offset_of(^^plain::b).bytes == (ptrdiff_t)__builtin_offsetof(plain, b));
  }
  end_test_case();

  test_case("bit fields carry a bit offset and a bit size");
  {
    static_assert(mm::is_bit_field(^^bits::lo));
    static_assert(mm::is_bit_field(^^bits::hi));
    static_assert(!mm::is_bit_field(^^bits::rest));
    static_assert(mm::bit_size_of(^^bits::lo) == 3);
    static_assert(mm::bit_size_of(^^bits::hi) == 5);

    static_assert(mm::offset_of(^^bits::lo).total_bits() == 0);
    static_assert(mm::offset_of(^^bits::hi).total_bits() == 3);
    require_true(mm::bit_size_of(^^bits::hi) == 5);
  }
  end_test_case();

  test_case("type_of vs decltype, and the trait mirrors");
  {
    static_assert(mm::is_same_type(mm::type_of(^^plain::a), ^^int));
    static_assert(mm::is_same_type(mm::type_of(^^plain::b), ^^double));
    static_assert(mm::is_same_type(mm::type_of(^^nested::p), ^^plain));

    static_assert(mm::is_aggregate_type(^^plain) == micron::is_aggregate_v<plain>);
    static_assert(mm::is_class_type(^^plain) == micron::is_class_v<plain>);
    static_assert(mm::is_enum_type(^^colour) == micron::is_enum_v<colour>);
    static_assert(mm::is_empty_type(^^empty) == micron::is_empty_v<empty>);
    static_assert(mm::is_integral_type(^^int) == micron::is_integral_v<int>);
    static_assert(mm::is_floating_point_type(^^double) == micron::is_floating_point_v<double>);
    static_assert(mm::is_pointer_type(^^void *) == micron::is_pointer_v<void *>);
    static_assert(mm::is_scoped_enum_type(^^colour));
    static_assert(!mm::is_scoped_enum_type(^^flags_e));
    static_assert(mm::is_trivially_copyable_type(^^plain) == micron::is_trivially_copyable_v<plain>);
    static_assert(mm::is_base_of_type(^^base_a, ^^derived) == micron::is_base_of_v<base_a, derived>);
    require_true(mm::is_aggregate_type(^^plain));
  }
  end_test_case();

  test_case("transformations mirror micron::type_traits");
  {
    static_assert(mm::is_same_type(mm::remove_cv(^^const int), ^^int));
    static_assert(mm::is_same_type(mm::remove_reference(^^int &), ^^int));
    static_assert(mm::is_same_type(mm::remove_cvref(^^const int &), ^^int));
    static_assert(mm::is_same_type(mm::add_pointer(^^int), ^^int *));
    static_assert(mm::is_same_type(mm::remove_pointer(^^int *), ^^int));
    static_assert(mm::is_same_type(mm::underlying_type(^^colour), ^^u8));
    static_assert(mm::is_same_type(mm::decay(^^int const &), ^^int));
    require_true(mm::is_same_type(mm::underlying_type(^^colour), ^^u8));
  }
  end_test_case();

  test_case("enumerators_of: count, order, values");
  {
    static_assert(mm::enumerators_of(^^colour).size() == 3);
    static_assert(mm::enumerators_of(^^flags_e).size() == 4);
    constexpr auto en = mm::define_static_array(mm::enumerators_of(^^colour));
    static_assert(mm::identifier_of(en.ptr[0]) == mm::identifier{ "red", 3 });
    static_assert(mm::identifier_of(en.ptr[2]) == mm::identifier{ "blue", 4 });
    static_assert([:en.ptr[1]:] == colour::green);
    constexpr usize n = en.len;
    require_true(n == 3);
  }
  end_test_case();

  test_case("bases_of sees both bases, in declaration order");
  {
    constexpr auto bs = mm::define_static_array(mm::bases_of(^^derived, ctx));
    static_assert(bs.len == 2);
    static_assert(mm::is_same_type(mm::type_of(bs.ptr[0]), ^^base_a));
    static_assert(mm::is_same_type(mm::type_of(bs.ptr[1]), ^^base_b));

    constexpr auto none = mm::define_static_array(mm::bases_of(^^plain, ctx));
    static_assert(none.len == 0);
    constexpr usize n = bs.len;
    constexpr usize z = none.len;
    require_true(n == 2 and z == 0);
  }
  end_test_case();

  test_case("splicing reads and writes the right field");
  {
    plain p{ 7, 2.5, 'q' };
    require_true(p.[:plain_m.ptr[0]:] == 7);
    require_true(p.[:plain_m.ptr[2]:] == 'q');
    p.[:plain_m.ptr[0]:] = 11;
    require_true(p.a == 11);

    require_true(micron::addressof(p.[:plain_m.ptr[1]:]) == micron::addressof(p.b));
  }
  end_test_case();

  test_case("template-for walks every member exactly once");
  {
    usize seen = 0;
    usize bytes = 0;
    template for ( constexpr auto m : plain_m )
    {
      ++seen;
      bytes += mm::size_of(m);
    }
    require_true(seen == 3);
    require_true(bytes == sizeof(int) + sizeof(double) + sizeof(char));
  }
  end_test_case();

  test_case("offsets are strictly increasing and inside the object");
  {
    prng rng(0x9E3779B97F4A7C15ull);
    for ( usize t = 0; t < 256; ++t ) {

      ptrdiff_t last = -1;
      template for ( constexpr auto m : plain_m )
      {
        const ptrdiff_t off = mm::offset_of(m).bytes;
        require_true(off > last);
        require_true((usize)off + mm::size_of(m) <= sizeof(plain));
        last = off;
      }
      (void)rng.next();
    }
  }
  end_test_case();

  test_case("extract round-trips a reflected constant");
  {
    static_assert(mm::extract<int>(mm::reflect_constant(42)) == 42);
    static_assert(mm::extract<colour>(mm::reflect_constant(colour::blue)) == colour::blue);
    require_true(mm::extract<int>(mm::reflect_constant(42)) == 42);
  }
  end_test_case();

  test_case("define_static_array promotes into real static storage");
  {

    static constexpr const char *name = mm::define_static_string(mm::identifier_of(^^plain));
    require_true(name[0] == 'p' and name[4] == 'n' and name[5] == '\0');

    static constexpr auto digits = mm::define_static_array(micron::raw_slice<const int>{ (const int[]){ 3, 1, 4, 1, 5 }, 5 });
    require_true(digits.len == 5);
    require_true(digits.ptr[0] == 3 and digits.ptr[4] == 5);
  }
  end_test_case();

  test_case("substitute / can_substitute accept a braced reflection list");
  {
    static_assert(mm::can_substitute(^^boxed, {
                                                  ^^int }));
    static_assert(mm::is_same_type(mm::substitute(^^boxed,
                                                  {
                                                      ^^int }),
                                   ^^boxed<int>));
    static_assert(mm::can_substitute(^^only_int, {
                                                     ^^int }));
    static_assert(!mm::can_substitute(^^only_int, {
                                                      ^^double }));
    static_assert(mm::template_arguments_of(^^boxed<int>).size() == 1);
    static_assert(mm::has_template_arguments(^^boxed<int>));
    require_true(mm::can_substitute(^^boxed, {
                                                 ^^u64 }));
  }
  end_test_case();

  test_case("define_static_object promotes a value into static storage");
  {

    static constexpr const plain *p = mm::define_static_object(plain{ 5, 0.25, 'k' });
    static_assert(p->a == 5 and p->c == 'k');
    require_true(p->a == 5 and p->b == 0.25 and p->c == 'k');
  }
  end_test_case();

  test_case("parent_of / has_identifier / rank / extent");
  {
    static_assert(mm::identifier_of(mm::parent_of(^^plain::a)) == mm::identifier{ "plain", 5 });
    static_assert(mm::has_identifier(^^plain));
    static_assert(!mm::has_identifier(^^int *));
    static_assert(mm::rank(^^int[3][4]) == 2);
    static_assert(mm::extent(^^int[3][4], 0) == 3);
    static_assert(mm::extent(^^int[3][4], 1) == 4);
    static_assert(mm::is_nonstatic_data_member(^^plain::a));
    static_assert(!mm::is_nonstatic_data_member(^^plain));
    require_true(mm::rank(^^int[3][4]) == 2);
  }
  end_test_case();

  test_case("which std::meta backend is live");
  {
    if constexpr ( mm::self_hosted )
      print("  backend: micron's own std::meta (src/__special/meta)");
    else
      print("  backend: libstdc++ <meta> (a libstdc++ header got there first)");
    require_true(true);
  }
  end_test_case();

  print("=== META RIGOR PASSED ===");
  return 1;
}

#endif
