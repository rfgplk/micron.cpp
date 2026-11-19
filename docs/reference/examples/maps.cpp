#include <micron/map.hpp>

int main()
{
  micron::robin_map<int, int> values;
  values.insert(2, 20);
  int *found = values.find(2);
  return found && *found == 20 && values.find(3) == nullptr ? 0 : 1;
}
