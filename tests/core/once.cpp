//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/string/strings.hpp"
#include "../../src/mutex/once.hpp"

void do_this_once(mc::string& s)
{
  mc::console(s);
  return;
}


void new_scope(void){

  mc::string p = "Works across scopes, this will never be stdouted :(";
  mc::do_once<do_this_once> test( p );
}

void do_this_again(mc::string& s)
{
  mc::console("Will be stdouted again, because the function is different");
  mc::console(s);
  return;
}

void do_this_third_time(mc::string& s)
{
  mc::console("NOTE: a new do_this_once guard will be generated per function provided, even if the signatures are identical!");
  mc::console(s);
  return;
}
int
main(void)
{
  mc::string msg = "\"Fox\" will be stdouted once only!";
  mc::console(msg);
  mc::string test = "Fox";
  mc::do_once<do_this_once> d1( test );
  mc::do_once<do_this_once> d2( test );
  mc::do_once<do_this_once> d3( test );
  mc::do_once<do_this_once> d4( test );
  mc::do_once<do_this_once> d5( test );
  mc::do_once<do_this_again> ad1( test );
  mc::do_once<do_this_again> ad2( test );
  mc::do_once<do_this_once> d6( test );
  mc::do_once<do_this_once> d7( test );
  mc::do_once<do_this_once> d8( test );
  mc::do_once<do_this_once> d9( test );
  mc::do_once<do_this_third_time> t1( test );
  mc::do_once<do_this_third_time> t2( test );
  mc::do_once<do_this_third_time> t3( test );
  new_scope();
  return 0;
}
