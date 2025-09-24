//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/string/strings.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/string/format.hpp"

namespace bench = mc;

int
main(void)
{
  bench::string str = "Hello World!";
  if constexpr ( false ) {
    for ( size_t i = 0; i < 100000; ++i )
      str.insert(i, "abcdefghi");
    mc::console(str.size());
  }
  if constexpr ( true ) {
    for ( size_t i = 0; i < 100000; ++i )
      mc::console(mc::format::find(str, "World"));
  }
}
