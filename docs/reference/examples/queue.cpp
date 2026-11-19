#include <micron/queue/spsc_queue.hpp>

int main()
{
  micron::spsc_queue<int, 16> values;
  int input = 7;
  int output = 0;
  return values.push(micron::move(input)) && values.pop(output) && output == 7 ? 0 : 1;
}
