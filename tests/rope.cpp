//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/memory/memory.hpp"
#include "../src/range.hpp"
#include "../src/std.hpp"
#include "../src/string/string_view.hpp"
#include "../src/string/strings.hpp"
#include "../src/string/unistring.hpp"

#include "snowball/snowball.hpp"

int
main(void)
{
  mc::rope str("Hello World!");
  mc::console(str);
  mc::rope rst;
  for ( u64 i : mc::u64_range<1, 100000>() )
    rst = mc::move(str.insert(6, i, i));
  mc::console(rst);
  //~30s
  /*
   // cut this off after 5 minutes
   // not equivalent but closest you can get to
  std::string str("Hello World!");
  std::string rst;
  for(size_t i = 0; i < 100000; ++i)
  // yes should be like this idea is youre inserting at root
      rst = str.insert(6, i, i);
  std::cout << rst << std::endl;
   * */

  return 0;
}
