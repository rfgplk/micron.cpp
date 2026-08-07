//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/reflect.hpp"
#include "../../src/meta.hpp"

#if !defined(__micron_reflection)

int
main(void)
{
  return 1;
}

#else

#include "../../src/std.hpp"

namespace
{

struct flat {
  int a;
  double b;
  char c;
};

struct nested {
  flat inner;
  u64 tag;
};

struct with_bits {
  u32 lo : 3;
  u32 hi : 5;
};

struct base_l {
  int p;
};

struct base_r {
  int q;
};

struct derived: base_l, base_r {
  short s;
};

enum class mode : u16 { off, on, latched };

}      // namespace

static_assert(micron::reflect::field_count<flat> == 3);
static_assert(micron::reflect::field_count<nested> == 2);
static_assert(micron::reflect::base_count<derived> == 2);
static_assert(micron::reflect::enum_count<mode> == 3);
static_assert(micron::meta::size_of(^^flat) == sizeof(flat));
static_assert(micron::meta::is_bit_field(^^with_bits::lo));
static_assert(micron::reflect::field_name<0, flat>().size() == 1);
static_assert(micron::is_same_v<micron::reflect::field_type<1, flat>, double>);
static_assert(micron::reflect::index_of<flat>(micron::meta::identifier{ "c", 1 }) == 2);

template<micron::fixed_string N> struct keyed {
  static constexpr auto key = N;
};

using k0 = keyed<micron::reflect::name_of<micron::reflect::members<flat>.ptr[0]>()>;
static_assert(k0::key.size() == 1);

int
main(void)
{
  flat f{ 1, 2.0, 'x' };
  nested n{ f, 7 };
  derived d{};
  d.s = 3;

  usize acc = 0;
  micron::reflect::for_each_field(f, [&acc](auto nm, auto &v) { acc += nm.size() + sizeof(v); });
  micron::reflect::for_each_field_indexed(n, [&acc](usize i, auto, auto &) { acc += i; });

  acc += static_cast<usize>(micron::reflect::equal(f, f));
  acc += static_cast<usize>(micron::reflect::hash(n) & 0xff);
  acc += micron::reflect::enum_name(mode::latched).size();
  acc += micron::get<0>(micron::reflect::to_tuple(f)) ? 1u : 0u;

  constexpr auto mp = micron::reflect::member_ptr<micron::reflect::members<flat>.ptr[2]>();
  acc += static_cast<usize>(f.*mp);

  micron::reflect::tie_fields(f);
  acc += static_cast<usize>(d.s);

  return static_cast<int>(acc & 0x7f) | 1;
}

#endif
