#include <cstddef>
#include <vector>
#include <iostream>
int
main()
{
  constexpr std::size_t N = 100'000'000;

  std::vector<std::size_t> v;
  v.reserve(N);

  for ( std::size_t i = 0; i < N; ++i )
    v.emplace_back(i);
  
  std::cout << v[100] << std::endl << v[1000] << v[100000] << std::endl;
  return 0;
}
