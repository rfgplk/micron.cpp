//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/match.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/vector/fvector.hpp"

void
fn_ints(int x)
{
  mc::console(x, " was an int!");
}

void
fn_chars(char x)
{
  mc::console(x, " was a char!");
}

void
fn_strings(const mc::string &x)
{
  mc::console(x, " was a string!");
}

void
fn_vecs(const mc::fvector<char> &x)
{
  mc::console(x, " was a vector!");
}

int
main()
{
  enable_scope()
  {
    mc::option<int, bool> opt(4);
    mc::console(opt.cast<int>());
    mc::any<int, bool, mc::string> anys{false};
    mc::console(anys.cast<bool>());
    bool __t = anys;
    mc::console(__t);
    anys = mc::string{"hello world"};
    mc::console(anys.cast<mc::string>());
    mc::string __s = anys;
    mc::console(__s);
    int test = 55;
    bool test_b = 1;
    mc::string test_str = "string";
    mc::fvector<char> test_vec(20, [](void) -> char { return 1; });
    mc::match<fn_ints, fn_chars, fn_strings, fn_vecs>(test, test_b, test_str, test_vec);
  };
}
