//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/string/strings.hpp"
#include "../src/io/console.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"
#include "../src/string/string_view.hpp"
#include "../src/string/unistring.hpp"

#include "snowball/snowball.hpp"

using namespace micron::format;
int
main(void)
{
  mc::string d_uni = micron::int_to_string<int, schar>(500);
  sb::require(d_uni.size(), 3);
  sb::require_true(is_in(d_uni, "500"));
  sb::require_false(is_in(d_uni, ""));
  mc::string test = "Hello World!";
  sb::require_true(test == "Hello World!");
  sb::require_false(test == "Hello World");
  sb::require_true(fast_find(test, "World") == test.begin() + 6);
  test.insert((size_t)1, "Hello");
  sb::require_true(test == "HHelloello World!");
  mc::sstr<256> test_stack = "Hello World!";
  mc::string_view<mc::sstr<256>> vw(test_stack);
  test_stack.insert(6, '-', 5);
  test_stack.insert((size_t)0, '"', 1);
  mc::console("Substr: ", test.substr((test.size()) - 3, 3));
  mc::console("Substr: ", test_stack.substr((test_stack.size()) - 3, 3));
  mc::console("Ends with: ", ends_with(test, "ld!"));
  mc::console("Ends with: ", ends_with(test_stack, "ld!"));
  mc::console("Starts with: ", starts_with(test, "Hel"));
  mc::console(mc::strlen("Hello World!") == mc::strlen("Hello World!"));
  mc::console("Size check: (true) ", test.size() == test_stack.size());
  mc::console("Size check: (true) ", test.size() == mc::strlen("Hello World!"));
  mc::console("Size check: (true) ", test.size() == mc::strlen(test.c_str()));
  mc::console(test_stack);
  for ( size_t i = 0; i < 10; i++ )
    test.insert(5, "u90drgzdjiopbqa3e90gikos0rkbv90rsib90-srj90pvsri,90phi");
  mc::console("Inserted");
  mc::string first = "John";
  mc::string second = "Bananaseed";
  mc::console(first + " " + second);
  mc::sstr<256> c = "XApplXes aXnd erroOranges! errory text              ";
  c.erase(0);
  c.erase(4);
  c.erase(8);
  c.erase(11, 4);
  mc::console(c);
  mc::console(find(c, "errory") - c.begin());
  c.erase(find(c, "errory"), 6);
  c.erase(find(c, "text"), 4);
  mc::console(*(c.end()) == '\0');
  mc::console(c.size());
  mc::console(c);
  strip(c);
  mc::console(c);
  mc::console(c.size());
  casefold(c);
  mc::console(c);
  upper(c);
  mc::console(c);
}
