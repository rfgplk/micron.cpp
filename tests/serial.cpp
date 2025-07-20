//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"

#include "../src/io/filesystem.hpp"
#include "../src/io/serial.hpp"
#include "../src/io/print.hpp"
#include "../src/io/paths.hpp"
#include "../src/std.h"

#include "../src/io/console.hpp"
#include "../src/control.hpp"

#include <fstream> // for std::ifstream


#include <iostream> // for std::cout
#include <vector>

int
main(void)
{
  int i = 1;
  mc::vector<int> b;
  b.resize(1000, 0);
  for(auto& c : b)
  c+=i++;
  byte* a = new byte[1024];
  mc::cmemset<1024>(a, 0x21);
  mc::fsys::file<mc::ustr8> f("/tmp/serialtestvec", mc::io::rwc, 211);
  mc::io::serialize::serialize(f, i);
  f.sync();
  return 0;
}
