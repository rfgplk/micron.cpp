//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/string/string_view.hpp"
#include "../src/string/strings.hpp"
#include "../src/string/unistring.hpp"
#include "../src/io/print.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"

using namespace micron::format;
int
main(void)
{
  mc::string d_uni = micron::int_to_string<int, schar>(500);
  mc::io::println(d_uni);
  mc::string test = "Hello World!";
  test.insert((size_t)0, "Hello");
  mc::io::println(test);
  mc::sstr<256> test_stack = "Hello World!";
  mc::string_view<mc::sstr<256>> vw(test_stack);
  test_stack.insert(6, '-', 5);
  test_stack.insert((size_t)0, '"', 1);
  mc::io::println("Substr: ", test.substr((test.size()) - 3, 3));
  mc::io::println("Substr: ", test_stack.substr((test_stack.size()) - 3, 3));
  mc::io::println("Ends with: ", ends_with(test, "ld!"));
  mc::io::println("Ends with: ", ends_with(test_stack, "ld!"));
  mc::io::println("Starts with: ", starts_with(test, "Hel"));
  mc::io::println(mc::strlen("Hello World!") == mc::strlen("Hello World!"));
  mc::io::println("Size check: (true) ", test.size() == test_stack.size());
  mc::io::println("Size check: (true) ", test.size() == mc::strlen("Hello World!"));
  mc::io::println("Size check: (true) ", test.size() == mc::strlen(test.c_str()));
  mc::io::println(test_stack);
  for ( size_t i = 0; i < 10000; i++ )
    test.insert(5, "{||||-bla-||||}");
  mc::io::println("Inserted");
  mc::string first = "John";
  mc::string second = "Bananaseed";
  mc::io::println(first + " " + second);
  mc::sstr<256> c = "XApplXes aXnd erroOranges! errory text              ";
  c.erase(0);
  c.erase(4);
  c.erase(8);
  c.erase(11, 4);
  mc::io::println(c);
  mc::io::println(find(c, "errory") - c.begin());
  c.erase(find(c, "errory"), 6);
  c.erase(find(c, "text"), 4);
  mc::io::println(*(c.end()) == '\0');
  mc::io::println(c.size());
  mc::io::println(c);
  strip(c);
  mc::io::println(c);
  mc::io::println(c.size());
  casefold(c);
  mc::io::println(c);
  upper(c);
  mc::io::println(c);
}
