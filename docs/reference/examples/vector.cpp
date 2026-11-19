#include <micron/vector/vector.hpp>

int main()
{
  micron::vector<int> values;
  values.reserve(4);
  values.push_back(3);
  values.push_back(7);
  values.insert(1, 5);
  values.erase(usize(0));
  return values.size() == 2 && values[0] == 5 && values[1] == 7 && values.find(99) == nullptr ? 0 : 1;
}
