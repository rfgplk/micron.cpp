#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int
main()
{
  const char *path = "tools/src/main.cc";
  std::ifstream file(path, std::ios::binary | std::ios::ate);

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(size);
  if ( !file.read(buffer.data(), size) ) {
    std::cerr << "Failed to read file\n";
    return 1;
  }
  std::cout.write(buffer.data(), size);
  return 0;
}
