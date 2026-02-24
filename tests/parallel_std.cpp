#include <cstddef>
#include <execution>     // for std::execution::par
#include <stdexcept>
#include <vector>

int
main()
{
  constexpr std::size_t N = 1uLL << 24;
  std::vector<unsigned long long> vec(N);

  std::for_each(std::execution::par, vec.begin(), vec.end(), [](auto &x) { x = 1; });
  for ( auto &n : vec )
    if ( n != 1 )
      throw std::runtime_error("Wasn't equal to one");
  std::for_each(std::execution::par, vec.begin(), vec.end(), [](auto &x) { x = 5; });
  for ( auto &n : vec )
    if ( n != 5 )
      throw std::runtime_error("Wasn't equal to 5");
   std::for_each(std::execution::par, vec.begin(), vec.end(), [](auto &x) { x = 10; });
  for ( auto &n : vec )
    if ( n != 10 )
      throw std::runtime_error("Wasn't equal to 10");
 
  return 0;
}
