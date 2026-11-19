#include <micron/atomic/atomic.hpp>

int main()
{
  micron::atomic_token<int> value(2);
  value.fetch_add(3, micron::memory_order_relaxed);
  return value.get(micron::memory_order_relaxed) == 5 ? 0 : 1;
}
