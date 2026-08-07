//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// rigor: micron::reflect.

#include "../../src/reflect.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#if !defined(__micron_reflection)

int
main(void)
{
  snowball::print("reflect: built without -freflection, nothing to exercise");
  return 1;
}

#else

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mr = micron::reflect;

namespace
{

struct packet {
  u32 id;
  i64 stamp;
  f64 weight;
  u8 kind;
};

struct point {
  i32 x;
  i32 y;
};

struct wrapper {
  point origin;
  u64 flags;
};

struct one_field {
  u16 solo;
};

struct base_l {
  i32 inherited;
};

struct base_r {
  u16 tagged;
};

struct with_bases: base_l, base_r {
  i32 own;
};

enum class state : u16 { idle = 0, arming = 1, live = 2, halted = 3 };

struct field_desc {
  const char *name;
  usize offset;
  usize size;
};

constexpr field_desc packet_truth[] = {
  { "id", __builtin_offsetof(packet, id), sizeof(u32) },
  { "stamp", __builtin_offsetof(packet, stamp), sizeof(i64) },
  { "weight", __builtin_offsetof(packet, weight), sizeof(f64) },
  { "kind", __builtin_offsetof(packet, kind), sizeof(u8) },
};
constexpr usize packet_truth_n = 4;

constexpr field_desc point_truth[] = {
  { "x", __builtin_offsetof(point, x), sizeof(i32) },
  { "y", __builtin_offsetof(point, y), sizeof(i32) },
};
constexpr usize point_truth_n = 2;

const char *const state_truth[] = { "idle", "arming", "live", "halted" };
constexpr usize state_truth_n = 4;

bool
packet_equal_byhand(const packet &a, const packet &b) noexcept
{
  return a.id == b.id and a.stamp == b.stamp and a.weight == b.weight and a.kind == b.kind;
}

u64
packet_hash_byhand(const packet &p) noexcept
{
  u64 h = 0xcbf29ce484222325ull;
  const auto word = [&h](u64 w) {
    for ( usize i = 0; i < sizeof(u64); ++i ) {
      h ^= (w >> (i * 8)) & 0xffull;
      h *= 0x100000001b3ull;
    }
  };
  word(static_cast<u64>(p.id));
  word(static_cast<u64>(p.stamp));
  word(p.weight == 0.0 ? 0ull : __builtin_bit_cast(u64, p.weight));
  word(static_cast<u64>(p.kind));
  return h;
}

packet
gen_packet(prng &rng) noexcept
{
  packet p{};
  p.id = static_cast<u32>(rng.next());
  p.stamp = static_cast<i64>(rng.next());

  p.weight = static_cast<f64>(static_cast<i32>(rng.next()));
  p.kind = static_cast<u8>(rng.next());
  return p;
}

template<micron::fixed_string Name, class T, class M> struct named_ptr {
  M T::*ptr;
  static constexpr auto key = Name;
};

template<class T, usize... I>
consteval auto
derive_pack(micron::index_sequence<I...>)
{
  return micron::tuple{ named_ptr<mr::field_name<I, T>(), T, mr::field_type<I, T>>{ mr::member_ptr<mr::members<T>.ptr[I]>() }... };
}

bool
same_name(micron::meta::identifier got, const char *want) noexcept
{
  usize n = 0;
  while ( want[n] ) ++n;
  if ( got.size() != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( got[i] != want[i] ) return false;
  return true;
}

}      // namespace

int
main(void)
{
  print("=== REFLECT RIGOR ===");

  test_case("field_count matches the hand-written table");
  {
    static_assert(mr::field_count<packet> == packet_truth_n);
    static_assert(mr::field_count<point> == point_truth_n);
    static_assert(mr::field_count<wrapper> == 2);
    static_assert(mr::field_count<one_field> == 1);
    require_true(mr::field_count<packet> == packet_truth_n);
    require_true(mr::field_count<one_field> == 1);
  }
  end_test_case();

  test_case("field_name / field_offset / field_type vs the table");
  {
    static_assert(mr::field_offset<0, packet>() == packet_truth[0].offset);
    static_assert(mr::field_offset<1, packet>() == packet_truth[1].offset);
    static_assert(mr::field_offset<2, packet>() == packet_truth[2].offset);
    static_assert(mr::field_offset<3, packet>() == packet_truth[3].offset);
    static_assert(micron::is_same_v<mr::field_type<0, packet>, u32>);
    static_assert(micron::is_same_v<mr::field_type<1, packet>, i64>);
    static_assert(micron::is_same_v<mr::field_type<2, packet>, f64>);
    static_assert(micron::is_same_v<mr::field_type<3, packet>, u8>);

    constexpr auto n0 = mr::field_name<0, packet>();
    constexpr auto n2 = mr::field_name<2, packet>();
    require_true(n0.size() == 2 and n0.buf[0] == 'i' and n0.buf[1] == 'd');
    require_true(n2.size() == 6 and n2.buf[0] == 'w');
    require_true(mr::field_offset<3, packet>() == packet_truth[3].offset);
  }
  end_test_case();

  test_case("for_each_field visits every field, in order, aliasing the real storage");
  {
    packet p{ 1, 2, 3.0, 4 };
    usize seen = 0;
    bool ok = true;
    mr::for_each_field_indexed(p, [&](usize i, auto name, auto &ref) {
      if ( i >= packet_truth_n ) {
        ok = false;
        return;
      }

      if ( name.size() != micron::strlen(packet_truth[i].name) ) ok = false;
      for ( usize k = 0; k < name.size() and ok; ++k )
        if ( name.buf[k] != packet_truth[i].name[k] ) ok = false;

      if ( sizeof(ref) != packet_truth[i].size ) ok = false;
      const auto off
          = reinterpret_cast<const unsigned char *>(micron::addressof(ref)) - reinterpret_cast<const unsigned char *>(micron::addressof(p));
      if ( static_cast<usize>(off) != packet_truth[i].offset ) ok = false;
      ++seen;
    });
    require_true(seen == packet_truth_n);
    require_true(ok);
  }
  end_test_case();

  test_case("writing through a visited reference mutates the object");
  {
    point pt{ 0, 0 };
    mr::for_each_field(pt, [](auto, auto &ref) { ref = 7; });
    require_true(pt.x == 7 and pt.y == 7);
  }
  end_test_case();

  test_case("get_field reads the same bytes as the member does");
  {
    prng rng(0xC0FFEE0DDF00Dull);
    for ( usize t = 0; t < 4096; ++t ) {
      const packet p = gen_packet(rng);
      require_true(mr::get_field<0>(p) == p.id);
      require_true(mr::get_field<1>(p) == p.stamp);
      require_true(mr::get_field<2>(p) == p.weight);
      require_true(mr::get_field<3>(p) == p.kind);
    }
  }
  end_test_case();

  test_case("equal() agrees with the by-hand comparison");
  {
    prng rng(0x5EEDF00D5EEDF00Dull);
    for ( usize t = 0; t < 8192; ++t ) {
      packet a = gen_packet(rng);
      packet b = (rng.next() & 1) ? a : gen_packet(rng);
      require_true(mr::equal(a, b) == packet_equal_byhand(a, b));
      require_true(mr::equal(a, a));

      packet c = a;
      c.kind = static_cast<u8>(c.kind + 1);
      require_true(!mr::equal(a, c));
    }
  }
  end_test_case();

  test_case("hash() agrees with the by-hand fnv-1a, and respects equality");
  {
    prng rng(0xDEADBEEFCAFEBABEull);
    for ( usize t = 0; t < 8192; ++t ) {
      const packet a = gen_packet(rng);
      require_true(mr::hash(a) == packet_hash_byhand(a));
      const packet b = a;
      require_true(mr::hash(a) == mr::hash(b));
    }
  }
  end_test_case();

  test_case("hash() honours the equal -> same-hash contract for float zeros");
  {

    struct fz {
      f64 d;
      f32 f;
    };

    const fz pos{ 0.0, 0.0f };
    const fz neg{ -0.0, -0.0f };
    require_true(mr::equal(pos, neg));
    require_true(mr::hash(pos) == mr::hash(neg));

    const fz other{ 1.0, 0.0f };
    require_true(!mr::equal(pos, other));
    require_true(mr::hash(pos) != mr::hash(other));
  }
  end_test_case();

  test_case("hash() and equal() are usable in a constant expression");
  {

    constexpr point ca{ 3, 4 };
    constexpr point cb{ 3, 4 };
    constexpr point cc{ 3, 5 };
    static_assert(mr::equal(ca, cb));
    static_assert(!mr::equal(ca, cc));
    static_assert(mr::hash(ca) == mr::hash(cb));
    static_assert(mr::hash(ca) != mr::hash(cc));
    require_true(mr::hash(ca) == mr::hash(cb));
  }
  end_test_case();

  test_case("hash() ignores padding, which memcmp would not");
  {

    packet a{};
    packet b{};
    auto *ra = reinterpret_cast<unsigned char *>(&a);
    auto *rb = reinterpret_cast<unsigned char *>(&b);
    for ( usize i = 0; i < sizeof(packet); ++i ) {
      ra[i] = 0x00;
      rb[i] = 0xFF;
    }
    a.id = b.id = 5;
    a.stamp = b.stamp = -9;
    a.weight = b.weight = 1.5;
    a.kind = b.kind = 3;
    require_true(mr::equal(a, b));
    require_true(mr::hash(a) == mr::hash(b));
  }
  end_test_case();

  test_case("to_tuple / tie_fields round-trip");
  {
    prng rng(0xABCDEF0123456789ull);
    for ( usize t = 0; t < 2048; ++t ) {
      const packet p = gen_packet(rng);
      auto tup = mr::to_tuple(p);
      static_assert(micron::tuple_size_v<decltype(tup)> == packet_truth_n);
      require_true(micron::get<0>(tup) == p.id);
      require_true(micron::get<1>(tup) == p.stamp);
      require_true(micron::get<2>(tup) == p.weight);
      require_true(micron::get<3>(tup) == p.kind);
    }

    point pt{ 1, 2 };
    auto tied = mr::tie_fields(pt);
    micron::get<0>(tied) = 99;
    require_true(pt.x == 99);
  }
  end_test_case();

  test_case("nested aggregates recurse instead of comparing padding");
  {
    wrapper a{ { 1, 2 }, 0xF00Dull };
    wrapper b{ { 1, 2 }, 0xF00Dull };
    wrapper c{ { 1, 3 }, 0xF00Dull };
    require_true(mr::equal(a, b));
    require_true(!mr::equal(a, c));
    require_true(mr::hash(a) == mr::hash(b));
    require_true(mr::hash(a) != mr::hash(c));
  }
  end_test_case();

  test_case("equal/hash cover inherited fields, which members<T> alone does not");
  {

    static_assert(mr::field_count<with_bases> == 1);
    static_assert(mr::base_count<with_bases> == 2);

    with_bases a{};
    a.inherited = 1;
    a.tagged = 2;
    a.own = 3;
    with_bases b = a;
    require_true(mr::equal(a, b));
    require_true(mr::hash(a) == mr::hash(b));

    with_bases c = a;
    c.inherited = 99;
    require_true(!mr::equal(a, c));
    require_true(mr::hash(a) != mr::hash(c));

    with_bases d = a;
    d.tagged = 99;
    require_true(!mr::equal(a, d));
    require_true(mr::hash(a) != mr::hash(d));

    with_bases e = a;
    e.own = 99;
    require_true(!mr::equal(a, e));
    require_true(mr::hash(a) != mr::hash(e));
  }
  end_test_case();

  test_case("private members are reflected, deliberately");
  {

    class boxed
    {
      i32 hidden;

    public:
      i32 shown;

      constexpr boxed(i32 h, i32 s) : hidden(h), shown(s) { }
    };

    static_assert(mr::field_count<boxed> == 2);
    boxed x{ 1, 2 }, y{ 9, 2 };
    require_true(!mr::equal(x, y));
  }
  end_test_case();

  test_case("index_of / has_field resolve names to ordinals");
  {
    static_assert(mr::index_of<packet>(micron::meta::identifier{ "id", 2 }) == 0);
    static_assert(mr::index_of<packet>(micron::meta::identifier{ "weight", 6 }) == 2);
    static_assert(mr::index_of<packet>(micron::meta::identifier{ "kind", 4 }) == 3);
    static_assert(!mr::has_field<packet>(micron::meta::identifier{ "nope", 4 }));
    static_assert(mr::has_field<packet>(micron::meta::identifier{ "stamp", 5 }));
    constexpr usize i = mr::index_of<packet>(micron::meta::identifier{ "stamp", 5 });
    require_true(i == 1);
  }
  end_test_case();

  test_case("enum_name / enum_from_name round-trip over every enumerator");
  {
    static_assert(mr::enum_count<state> == state_truth_n);
    const state vals[] = { state::idle, state::arming, state::live, state::halted };
    for ( usize i = 0; i < state_truth_n; ++i ) {
      require_true(same_name(mr::enum_name(vals[i]), state_truth[i]));
      state back{};
      require_true(mr::enum_from_name<state>(mr::enum_name(vals[i]), back));
      require_true(back == vals[i]);
    }

    require_true(mr::enum_name(static_cast<state>(77)).empty());
    state unused{};
    require_true(!mr::enum_from_name<state>(micron::meta::identifier{ "bogus", 5 }, unused));
  }
  end_test_case();

  test_case("enum_values yields the declared values in declaration order");
  {
    constexpr auto vs = mr::enum_values<state>();
    static_assert(vs[0] == state::idle);
    static_assert(vs[3] == state::halted);
    require_true(vs[1] == state::arming and vs[2] == state::live);
  }
  end_test_case();

  test_case("type_name and member_ptr");
  {
    constexpr auto tn = mr::type_name<packet>();
    require_true(tn.size() == 6 and tn.buf[0] == 'p' and tn.buf[5] == 't');

    static_assert(mr::type_name<const packet &>().size() == 6);

    constexpr auto mp = mr::member_ptr<mr::members<point>.ptr[1]>();
    static_assert(micron::is_same_v<decltype(mp), i32 point::*const>);
    point pt{ 4, 5 };
    require_true(pt.*mp == 5);
    pt.*mp = 6;
    require_true(pt.y == 6);
  }
  end_test_case();

  test_case("name_of survives as a non-type template parameter");
  {

    constexpr auto nm = mr::name_of<mr::members<packet>.ptr[2]>();
    static_assert(micron::is_same_v<decltype(nm), const micron::fixed_string<7>>);
    static_assert(nm.size() == 6);
    require_true(nm.buf[0] == 'w' and nm.buf[5] == 't' and nm.buf[6] == '\0');
  }
  end_test_case();

  test_case("a reflected field pack is the same TYPE as the hand-written one");
  {

    constexpr auto by_hand = micron::tuple{ named_ptr<micron::fixed_string{ "x" }, point, i32>{ &point::x },
                                            named_ptr<micron::fixed_string{ "y" }, point, i32>{ &point::y } };
    constexpr auto derived = derive_pack<point>(micron::make_index_sequence<mr::field_count<point>>{});
    static_assert(micron::is_same_v<decltype(by_hand), decltype(derived)>);

    point pt{ 11, 22 };
    require_true(pt.*(micron::get<0>(derived).ptr) == 11);
    require_true(pt.*(micron::get<1>(derived).ptr) == 22);
    require_true(micron::get<1>(derived).key.buf[0] == 'y');
  }
  end_test_case();

  print("=== REFLECT RIGOR PASSED ===");
  return 1;
}

#endif
