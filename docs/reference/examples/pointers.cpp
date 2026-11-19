#include <micron/pointer.hpp>

struct value {
  int number;
  explicit value(int n) : number(n) { }
};

int main()
{
  micron::ptr<value> owner(42);
  return owner->number == 42 ? 0 : 1;
}
