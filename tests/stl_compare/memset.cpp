#include <array>
#include <cstring>
#include <iostream>

int
main()
{
  std::array<unsigned char, 64> buf{};
  volatile int x = 0;

  for ( std::size_t i = 0; i < 1'000'000'000; ++i ) {
    std::memset(buf.data(), 6, buf.size());
    ++x;
  }

  for ( auto b : buf )
    std::cout << static_cast<int>(b) << ' ';
}
