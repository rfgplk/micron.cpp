//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/string/strings.hpp"
#include "../src/string/format.hpp"
#include "../src/attributes.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

int
main(void)
{
  mc::sstr<256> hello = "Hello, World World World!";
  mc::format::replace(hello, "World", "Cat");
  mc::console(hello);
  return 0;
}
