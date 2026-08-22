//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  micron::vector<int> values;
  values.push_back(2);
  values.push_back(3);
  values.push_back(5);

  micron::vector<micron::vector<int>> rows;
  rows.push_back(values);
  rows.emplace_back();
  rows.back().push_back(8);
  rows.back().push_back(13);

  micron::io::println("rows=", rows.size(), " first-size=", rows[0].size(), " last=", rows[1].back());
  return 0;
}
