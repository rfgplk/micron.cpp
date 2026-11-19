#include <micron/allocator.hpp>
#include <micron/memory/memory.hpp>

int main()
{
  micron::chunk<byte> memory = micron::allocator_serial<>::create(32);
  micron::memset(memory.ptr, 42, 32);
  bool ok = memory.ptr[0] == 42 && memory.ptr[31] == 42;
  micron::allocator_serial<>::destroy(memory);
  return ok ? 0 : 1;
}
