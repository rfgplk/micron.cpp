//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <iostream>

#include "../src/list.hpp"
#include "../src/string/strings.hpp"

#include "../src/io/print.hpp"

int
main()
{
  {
    // micron::list<micron::string> lst;
  }
  {
    micron::list<int> lst(50);
  }
  {
    micron::list<std::string> lst(10, "Test");
    size_t i = 0;
    auto *start = lst.ibegin();
    start = lst.next(start, 5);
    while ( start->next != nullptr ) {
      std::cout << i++ << ": ";
      std::cout << (start->data) << std::endl;
      start = start->next;
    }
    std::cout << "Size is: " << lst.size() << std::endl;
  }
  {
    micron::list<micron::sstring<40>> lst(20, "Test");
    for ( size_t i = 0; i < 20; i++ )
      lst.push_front("ABCD");
    auto *start = lst.ibegin();
    while ( start->next != nullptr ) {
      std::cout << (start->data) << std::endl;
      start = start->next;
    }
  }
  {
    micron::list<micron::sstring<40>> lst(20, "Apple");
    micron::list<micron::sstring<40>> lst2(40, "Pizza");
    lst.merge(lst2);
    std::cout << lst.size() << std::endl;
    std::cout << lst2.size() << std::endl;
  }
  return 0;
}
