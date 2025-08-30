//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/string/strings.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "../src/vector/vector.hpp"
#include <vector>
#include <string>
#include "../src/math/generic.hpp"
#include "../src/bitfield.hpp"

#include <bitset>

int
main(void)
{
  mc::sstr<10> mm1 = "multiple";
  mc::sstr<10> mm2 = "messages";
  const char* mmc = "!";
  const char* mes1 = "First message!";
  mc::ustr8 str = "Another message";
  mc::console("Hello World!");
  mc::infolog("Log message");
  mc::console("Automatic formatting: ", 5.5f, " ", 4, " ", false);
  mc::console(mes1);
  mc::console(str);
  mc::console("Output of ", mm1, " ", mm2, mmc);
  try {
    mc::cerror("This is an error");
  }
  catch (mc::except::standard_error& e)
  {
    mc::console("The error was <", e.what(), ">");
  }

  std::string a = "An STL string.";
  mc::console("Can also print any string-like object: ", a);
  mc::console("Sizes of vector stl and vector mc: ", sizeof(std::vector<int>), " ", sizeof(mc::vector<int>));
  std::vector<int> vecstl(10, 4);
  mc::vector<f128> vec(5, 66234.24f);
  mc::console(vecstl);
  mc::console(vec);
}
