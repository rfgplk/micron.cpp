#define MICRON_ABC_MT 1
#include <micron/thread/thread_types/auto_thread.hpp>

int main()
{
  micron::auto_thread<> worker([](int value) { return value * 2; }, 21);
  int result = worker.result<int>();
  worker.join();
  return result == 42 ? 0 : 1;
}
