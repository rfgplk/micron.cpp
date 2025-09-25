//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/vector/convector.hpp"
#include "../src/vector/fvector.hpp"
#include "../src/vector/svector.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  sb::verify_debug();
  enable_scope()
  {
    sb::test_case("micron::vector(), testing insertions and erasures");
    micron::fvector<int> vec;
    for ( int i = 0; i < 10; i++ )
      vec.push_back(i);
    vec.erase(vec.begin());
    vec.erase(vec.begin() + 3);
    vec.erase(vec.begin() + 7);
    vec.erase(vec.begin() + 9);
    micron::console(vec);
  };
  mc::convector<float> conf;
  conf.resize(50);
  conf.fill(5.5f);
  mc::console(conf[40]);
  micron::console("Redo");
  micron::vector<u16> cv = { 1, 2, 3, 4, 5, 6 };
  micron::vector<u16> bb = cv;
  bb.push_back(100);
  cv.push_back(2);
  cv.push_back(5);
  cv.push_back(5);
  micron::console(cv);
  micron::console(bb);

  micron::vector<size_t> buf = { 1, 2, 3, 4 };
  for ( auto x : buf )
    micron::console(x);
  buf.assign(10, 9999);
  for ( size_t i = 0; i < buf.size(); i++ )
    micron::console(buf[i]);
  micron::svector<int> stack_vec;
  micron::vector<byte> byte_data;
  micron::vector<char> char_data;
  micron::vector<int> int_data;
  micron::vector<float> float_data;
  micron::vector<size_t> max_data;
  for ( size_t i = 0; i < 10000; i++ ) {
    byte_data.push_back(static_cast<byte>(i));
    char_data.push_back(static_cast<char>(i));
    int_data.push_back(static_cast<int>(i));
    float_data.push_back(static_cast<float>(i));
    max_data.push_back(static_cast<size_t>(i));
  }
  micron::console("Size of byte: ", sizeof(byte));
  micron::console("Size of char: ", sizeof(char));
  micron::console("Size of float: ", sizeof(float));
  micron::console("Size of size_t: ", sizeof(size_t));
  micron::console("Size of byte_data(): ", byte_data.size());
  micron::console("Capacity of byte_data(): ", byte_data.max_size());
  micron::console("Size of char_data(): ", char_data.size());
  micron::console("Capacity of char_data(): ", char_data.max_size());

  micron::console("Size of int_data(): ", int_data.size());
  micron::console("Capacity of int_data(): ", int_data.max_size());

  micron::console("Size of float_data(): ", float_data.size());
  micron::console("Capacity of float_data(): ", float_data.max_size());

  micron::console("Size of max_data(): ", max_data.size());
  micron::console("Capacity of max_data(): ", max_data.max_size());

  for ( size_t i = 0; i < 50; i++ ) {
    micron::console("i: ", (int)byte_data[i]);
    micron::console("i: ", (int)char_data[i]);
    micron::console("i: ", int_data[i]);
    micron::console("i: ", float_data[i]);
    micron::console("i: ", max_data[i]);
  }

  for ( size_t i = 9950; i < 10000; i++ ) {
    micron::console("i: ", (int)byte_data[i]);
    micron::console("i: ", (int)char_data[i]);
    micron::console("i: ", int_data[i]);
    micron::console("i: ", float_data[i]);
    micron::console("i: ", max_data[i]);
  }
  micron::console("Slicing");
  micron::slice<size_t> slc = max_data[4, 20];
  micron::vector<int> nint_data;
  micron::console("moving");
  nint_data = micron::move(int_data);
  micron::console("moving");
  micron::console("size of int_data", int_data.size());
  micron::console("size of nint_data", nint_data.size());
  return 0;
}
