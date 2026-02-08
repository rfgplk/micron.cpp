#include <thread>
#include <vector>

int
main()
{
  std::vector<std::thread> threads;
  threads.reserve(512);

  for ( int i = 0; i < 512; ++i ) {
    threads.emplace_back([]() -> int { return 0; });
  }

  for ( auto &t : threads ) {
    if ( t.joinable() )
      t.join();
  }
}
