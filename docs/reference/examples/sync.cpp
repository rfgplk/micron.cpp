#include <micron/mutex/mutex.hpp>
#include <micron/mutex/locks.hpp>
#include <micron/sync/semaphore.hpp>

int main()
{
  micron::mutex lock;
  int value = 0;
  {
    micron::lock_guard<micron::mutex> guard(lock);
    value = 7;
  }
  micron::basic_semaphore permits(1);
  bool acquired = permits.try_wait();
  return value == 7 && acquired && !permits.try_wait() ? 0 : 1;
}
