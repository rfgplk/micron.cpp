#include <iostream>

#include "../src/vector/vector.hpp"
#include "../src/vector/fvector.hpp"
#include "../src/vector/svector.hpp"

int
main()
{
  micron::vector<size_t> buf = { 1, 2, 3, 4 };
  for(auto x : buf)
    std::cout << x << std::endl;
  buf.assign(10, 9999);
  for ( size_t i = 0; i < buf.size(); i++ )
    std::cout << buf[i] << std::endl;
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
  std::cout << "Size of byte: " << sizeof(byte) << std::endl;
  std::cout << "Size of char: " << sizeof(char) << std::endl;
  std::cout << "Size of float: " << sizeof(float) << std::endl;
  std::cout << "Size of size_t: " << sizeof(size_t) << std::endl;
  std::cout << "Size of byte_data(): " << byte_data.size() << std::endl;
  std::cout << "Capacity of byte_data(): " << byte_data.max_size() << std::endl;
  std::cout << "Size of char_data(): " << char_data.size() << std::endl;
  std::cout << "Capacity of char_data(): " << char_data.max_size() << std::endl;

  std::cout << "Size of int_data(): " << int_data.size() << std::endl;
  std::cout << "Capacity of int_data(): " << int_data.max_size() << std::endl;

  std::cout << "Size of float_data(): " << float_data.size() << std::endl;
  std::cout << "Capacity of float_data(): " << float_data.max_size() << std::endl;

  std::cout << "Size of max_data(): " << max_data.size() << std::endl;
  std::cout << "Capacity of max_data(): " << max_data.max_size() << std::endl;

  for ( size_t i = 0; i < 50; i++ ) {
    std::cout << "i: " << (int)byte_data[i] << std::endl;
    std::cout << "i: " << (int)char_data[i] << std::endl;
    std::cout << "i: " << int_data[i] << std::endl;
    std::cout << "i: " << float_data[i] << std::endl;
    std::cout << "i: " << max_data[i] << std::endl;
  }

  for ( size_t i = 9950; i < 10000; i++ ) {
    std::cout << "i: " << (int)byte_data[i] << std::endl;
    std::cout << "i: " << (int)char_data[i] << std::endl;
    std::cout << "i: " << int_data[i] << std::endl;
    std::cout << "i: " << float_data[i] << std::endl;
    std::cout << "i: " << max_data[i] << std::endl;
  }
  std::cout << "Slicing" << std::endl;
  micron::slice<size_t> slc = max_data[4, 20];
  micron::vector<int> nint_data;
  nint_data = micron::move(int_data);
  std::cout << "size of int_data" << int_data.size() << std::endl;
  std::cout << "size of nint_data" << nint_data.size() << std::endl;
  return 0;
}
