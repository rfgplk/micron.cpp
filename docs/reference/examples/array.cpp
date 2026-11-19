#include <micron/array/array.hpp>

int main()
{
  micron::array<int, 4> values({ 2, 4, 6, 8 });
  values[1] = 5;
  return values.size() == 4 && values[1] == 5 ? 0 : 1;
}
