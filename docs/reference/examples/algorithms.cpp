#include <micron/algorithm/find.hpp>
#include <micron/sort/quick.hpp>
#include <micron/vector/vector.hpp>

int main()
{
  micron::vector<int> values({ 4, 1, 3 });
  micron::sort::quick(values);
  const int *found = micron::find(values.cbegin(), values.cend(), 3);
  const int *missing = micron::find(values.cbegin(), values.cend(), 9);
  return found && *found == 3 && missing == nullptr && values[0] == 1 ? 0 : 1;
}
