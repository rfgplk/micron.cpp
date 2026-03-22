//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/string/format.hpp"
#include "../src/string/unistring.hpp"

int
main(void)
{
  float f = 1.0;
  float f2 = 10.0;
  float f3 = 4.3456666664;
  double d = 1.0;
  double d2 = 1.43563;
  double d3 = 100.1;
  mc::console(f);
  mc::console(f2);
  mc::console(f3);
  mc::console(d);
  mc::console(d2);
  mc::console(d3);
  {
    double val = 1.0;

    union {
      f64 f;
      u64 u;
    } conv;

    conv.f = val;

    char b1[64];
    usize n1 = micron::__impl::__ryu::d2s_buffered(val, b1);
    b1[n1] = '\0';

    // print sizes + raw bits + d2d result + final string
    mc::io::print("sizeof(f64)=");
    mc::io::print(sizeof(f64));
    mc::io::print(" sizeof(u64)=");
    mc::io::print(sizeof(u64));
    mc::io::print(" bits=0x");
    // print bits as hex
    auto hexstr = micron::to_hex_stack<u64, 20>(conv.u, true);
    mc::io::print(hexstr);
    mc::io::print(" d2s=");
    mc::io::print(b1);
    mc::io::print("\n");

    // also test d2d directly
    u64 ieeeMant = conv.u & ((1ull << 52) - 1);
    u32 ieeeExp = static_cast<u32>((conv.u >> 52) & 0x7FF);
    mc::io::print("ieeeMant=");
    mc::io::print(ieeeMant);
    mc::io::print(" ieeeExp=");
    mc::io::print(ieeeExp);
    mc::io::print("\n");
  }
  return 0;
}
