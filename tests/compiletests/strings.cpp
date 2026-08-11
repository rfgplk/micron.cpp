//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: micron::string surface compiles on every arch/opt. Not run.

#include "../../src/string/string_view.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector.hpp"

int
main()
{
  micron::string s("hello");
  s += " world";
  micron::string t(5, 'z');
  int acc = static_cast<int>(s.size() + t.size());

  micron::vector<micron::string> vs;
  vs.push_back(s);
  vs.emplace_back("compiletest");
  acc += static_cast<int>(vs.size());

  // text -> float. odr-use every tier, because the array/container overloads are templates and
  // would otherwise never instantiate -- so a bad `requires` clause or a missing if-constexpr arm
  // on a 32-bit arch would sail through the whole matrix unnoticed.
  micron::string num("1.5e3");
  f64 d = 0.0;
  f32 f = 0.0f;
  acc += static_cast<int>(micron::try_parse_double(num.cdata(), num.size(), d));
  acc += static_cast<int>(micron::try_parse_float(num.cdata(), num.size(), f));
  acc += static_cast<int>(micron::try_string_to_double(num, d));
  acc += static_cast<int>(micron::format::parse_double("1e5").is_first());
  acc += static_cast<int>(micron::format::parse_float(num).is_first());
  acc += static_cast<int>(micron::format::to_double(num));
  acc += static_cast<int>(micron::format::to_float("2.5", 3u));

  // fixed_string and string_view. odr-use both halves of every member, because these are
  // templates and an arch where (say) micron::memcmp's `if !consteval` split or strnlen picked a
  // different overload would otherwise never instantiate them and sail through the whole matrix.
  // the folded half is proven in tests/rigor/{fixed_string,string_view}_constexpr.cpp; what is
  // exercised here is that the RUNTIME instantiation compiles on every arch and ISA level
  constexpr micron::fixed_string fs{ "compile matrix" };
  static_assert(fs.size() == 14 && fs.len() == 14);
  acc += static_cast<int>(fs.len() + fs.size() + fs.capacity() + fs.max_size());
  acc += static_cast<int>(fs.find('m') + fs.rfind('m') + fs.find("matrix"));
  acc += static_cast<int>(fs.find_first_of("xyz") + fs.find_last_of("xyz"));
  acc += static_cast<int>(fs.find_first_not_of("com") + fs.find_last_not_of("xir"));
  acc += static_cast<int>(fs.find_first_of_n("xyz", 3) + fs.find_last_not_of_n("xir", 3));
  acc += static_cast<int>(fs.count('i') + fs.count("ma"));
  acc += static_cast<int>(fs.starts_with("comp")) + static_cast<int>(fs.ends_with("trix"));
  acc += static_cast<int>(fs.contains("pile")) + static_cast<int>(fs.empty());
  acc += static_cast<int>(fs.to_lower()[0] + fs.to_upper()[0] + fs.trim()[0] + fs.reverse()[0]);
  acc += static_cast<int>(fs.substr<0, 7>().len() + (fs + fs).len());
  acc += static_cast<int>(fs.at(0) + fs.front() + fs.back() + fs.c_str()[0] + *fs.cbegin());
  acc += static_cast<int>(fs.compare("x") + fs.compare("x", 1) + fs.compare(t));
  // the armada: <=> plus == with the other four relations synthesised off them
  acc += static_cast<int>(fs == "compile matrix") + static_cast<int>(fs != t);
  acc += static_cast<int>(fs < "z") + static_cast<int>(fs > "a");
  acc += static_cast<int>(fs <= "z") + static_cast<int>(fs >= "a");
  acc += static_cast<int>("a" < fs) + static_cast<int>(fs == micron::fixed_string<32>{ "compile matrix", 14 });

  micron::string_view<micron::string> view(s);
  acc += static_cast<int>(view.size() + view.empty());
  acc += static_cast<int>(view.front() + view.at(0) + *view.begin() + *view.cbegin() + view.data()[0]);
  acc += static_cast<int>(view.substr(0, 2).size() + view.substr(1).size());
  acc += static_cast<int>(view == "hello world") + static_cast<int>(view < "z") + static_cast<int>(view >= "a");
  acc += static_cast<int>(view.compare("x", 1));
  view.advance(1);
  // cstring_view is now an alias of the same class, not a second copy of the design
  micron::cstring_view<micron::sstr<32>> cview("literal", 7u);
  acc += static_cast<int>(cview.size() + (cview == "literal"));

  // conversions/fixed.hpp -- the %f / %e writers and their sizing helpers. odr-used here so a
  // bad `if constexpr` arm or a 32-bit-only overflow in the u64 kernel cannot sail through the
  // arch x opt x freestanding matrix unbuilt.
  {
    char fbuf[1400];
    namespace ry = micron::__impl::__ryu;
    acc += static_cast<int>(ry::d2f_buffered(3.5, fbuf, sizeof(fbuf), 6));
    acc += static_cast<int>(ry::d2f_trim_buffered(3.5, fbuf, sizeof(fbuf), 6));
    acc += static_cast<int>(ry::d2e_buffered(3.5, fbuf, sizeof(fbuf), 6));
    acc += static_cast<int>(ry::d2f_size(3.5, 6));
    acc += static_cast<int>(ry::d2e_size(3.5, 6));
    acc += static_cast<int>(ry::__fx::__fixed_u64(1ull << 52, -52, 6).ok);
    acc += static_cast<int>(ry::d2f_buffered(1e300, fbuf, sizeof(fbuf), 30));      // forces tier 2
    acc += static_cast<int>(ry::d2g_buffered(1.5, fbuf, sizeof(fbuf), 6, false, false));
    acc += static_cast<int>(ry::d2a_buffered(1.5, fbuf, sizeof(fbuf), 3, true, false));
  }

  // conversions/chars.hpp -- to_chars/from_chars at every width. the integer templates are
  // instantiated per type here so a bad make_unsigned or an overflow check that only holds on
  // 64-bit cannot pass the matrix unbuilt.
  {
    char cbuf[128];
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<i8>(-1), 2));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<u8>(1), 36));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<i16>(-1)));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<u16>(1)));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<i32>(-1), 16, true));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<u32>(1)));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<i64>(-1)));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<u64>(1)));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), true));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), static_cast<const void *>(cbuf)));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), 1.5, micron::float_format::shortest));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), 1.5f, micron::float_format::shortest));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), 1.5, micron::float_format::general, 6));
    acc += static_cast<int>(micron::to_chars(cbuf, sizeof(cbuf), 1.5, micron::float_format::hex, -1));

    i8 a1{};
    u8 a2{};
    i16 a3{};
    u16 a4{};
    i32 a5{};
    u32 a6{};
    i64 a7{};
    u64 a8{};
    bool a9{};
    f32 a10{};
    f64 a11{};
    acc += static_cast<int>(micron::from_chars(a1, "1", 1) + micron::from_chars(a2, "1", 1, 16));
    acc += static_cast<int>(micron::from_chars(a3, "1", 1) + micron::from_chars(a4, "1", 1));
    acc += static_cast<int>(micron::from_chars(a5, "1", 1) + micron::from_chars(a6, "1", 1));
    acc += static_cast<int>(micron::from_chars(a7, "1", 1) + micron::from_chars(a8, "1", 1));
    acc += static_cast<int>(micron::from_chars(a9, "1", 1) + micron::from_chars(a10, "1", 1));
    acc += static_cast<int>(micron::from_chars(a11, "1", 1));

    const u8 raw[2] = { 0xAB, 0x0F };
    acc += static_cast<int>(micron::bytes_to_hex(cbuf, sizeof(cbuf), raw, 2));
  }

  // 128-bit and the wide/small types. u128 is a builtin on the 64-bit arches and micron's own
  // struct on i386/armv7-a, and is_integral_v<u128> is true on amd64 ONLY -- so these entry
  // points have to build on every cell to prove the type-keyed overloads really are type-keyed.
  {
    char wbuf[200];
    const u128 big = (static_cast<u128>(1) << 100) + static_cast<u128>(7);
    const i128 nbig = -(static_cast<i128>(1) << 100);
    acc += static_cast<int>(micron::to_chars(wbuf, sizeof(wbuf), big, 10u));
    acc += static_cast<int>(micron::to_chars(wbuf, sizeof(wbuf), big, 2u));
    acc += static_cast<int>(micron::to_chars(wbuf, sizeof(wbuf), nbig, 16u, true));
    u128 ub{};
    i128 ib{};
    acc += static_cast<int>(micron::from_chars(ub, "1", 1) + micron::from_chars(ib, "-1", 2));
    acc += static_cast<int>(micron::int_to_string<u128>(big).size());
    acc += static_cast<int>(micron::to_string<u128>(big).size());
    acc += static_cast<int>(micron::to_string<bool>(true).size());
    acc += static_cast<int>(micron::to_string<f64>(1.5).size());
    acc += static_cast<int>(micron::to_string<f32>(1.5f).size());

    bool bv = false;
    acc += static_cast<int>(micron::try_parse_bool<char>("true", 4, bv));
    acc += static_cast<int>(micron::bool_to_string<char>(bv).size());

#if defined(__micron_has_wide_float)
    acc += static_cast<int>(micron::__impl::__ryu::x2a_buffered(static_cast<long double>(1.5L), wbuf, sizeof(wbuf)));
#endif
#if defined(__micron_f128_distinct)
    acc += static_cast<int>(micron::__impl::__ryu::x2a_buffered(static_cast<f128>(1.5), wbuf, sizeof(wbuf)));
#endif

    // the whitespace-tolerant / full-width cursor parsers
    const char *cp = "  12 0x7fffffffffffffff";
    const char *ce = cp + micron::strlen(cp);
    acc += static_cast<int>(micron::parse_uint_ws(cp, ce));
    acc += static_cast<int>(micron::parse_int_ws(cp, ce));
    acc += static_cast<int>(micron::parse_hex_u64(cp, ce));
  }

  return acc & 0x7f;
}
