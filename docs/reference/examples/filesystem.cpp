#include <micron/io/fsys.hpp>

int main()
{
  micron::io::path_t path("/proc/self/status");
  return micron::io::file_size(path) >= 0 ? 0 : 1;
}
