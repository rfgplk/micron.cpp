#include <micron/lz.hpp>
#include <micron/vector/vector.hpp>

int main()
{
  namespace lz = micron::lz;
  micron::vector<int> values({ 1, 2, 3, 4, 5 });
  auto result = values | lz::filter([](int x) { return x % 2 != 0; })
                       | lz::fmap([](int x) { return x * 10; })
                       | lz::take(2) | lz::collect<micron::vector<int>>();
  return result.size() == 2 && result[0] == 10 && result[1] == 30 ? 0 : 1;
}
