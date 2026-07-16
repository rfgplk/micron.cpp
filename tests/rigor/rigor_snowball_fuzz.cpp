//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../snowball/snowball_fuzz.hpp"

namespace sbf = snowball::fuzzing;

struct span {
  u32 off;
  u32 len;
};

int
main(void)
{

  sbf::check_property(
      "aligned index stays aligned",
      [](int idx) {
        if ( idx % 8 != 0 ) throw "index not 8-aligned";
      },
      { .seed = 0xA11CE, .count = 50000 }, sbf::range<int>(0, 4096).aligned(8));

  sbf::check_property(
      "filename bytes are filename-safe",
      [](micron::string s) {
        for ( usize i = 0; i < s.size(); ++i ) {
          char c = s[i];
          bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
          if ( !ok ) throw "unsafe filename byte";
        }
      },
      { .seed = 0xB0B, .count = 50000 }, sbf::string_of(sbf::alpha::filename).len(0, 64));

  sbf::check_property(
      "span fits its buffer",
      [](span sp) {
        if ( static_cast<u64>(sp.off) + sp.len > 1024 ) throw "span overruns 1024";
      },
      { .seed = 0xC0DE, .count = 50000 },
      sbf::reflect<span>().with<^^span::off>(sbf::range<u32>(0, 512)).with<^^span::len>(sbf::range<u32>(0, 512)));

  snowball::print("rigor_snowball_fuzz: all properties held");
  return 0;
}
