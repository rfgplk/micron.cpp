// memory_alloc.cpp
// micron's low-level memory allocation: allocator_serial and chunk<T>.
//
// See also:
//   examples/memory_memcpy_cmp.cpp — micron::memcpy / memcmp / memmove
//   examples/memory_memset.cpp     — micron::memset variants
//   examples/pointers.cpp          — smart pointer wrappers on top
//
// You rarely call allocator_serial directly — vectors and maps use it
// internally. This example shows the layer beneath them.

#include "../src/allocator.hpp"
#include "../src/memory/memory.hpp"
#include "../src/io/console.hpp"

int
main()
{
  // --- chunk<T> ---
  // A non-owning fat pointer: ptr + len. Used as the currency of all
  // allocation in micron. Does NOT free on destruction.
  micron::chunk<byte> mem = micron::allocator_serial<>::create(256);
  micron::io::println("allocated len=", mem.len, " ptr=", reinterpret_cast<usize>(mem.ptr));

  // --- memset via micron::memset (cmemory) ---
  // Prefer micron::memset over the libc version — uses AVX2/NEON when available.
  micron::memset(mem.ptr, 0xAB, mem.len);
  micron::io::println("first byte after memset: 0x", mem.ptr[0]);

  // --- grow ---
  // Reallocates and copies existing data to a larger block.
  // Growth factor is determined by the policy (default: tripling).
  micron::chunk<byte> bigger = micron::allocator_serial<>::grow(mem, 1024);
  micron::io::println("grown len=", bigger.len);
  // NOTE: 'mem' is now dangling — grow() frees the old block.

  // --- memcpy via micron::memcpy ---
  byte src[16];
  micron::memset(src, 0x42, 16);
  micron::memcpy(bigger.ptr, src, 16);
  micron::io::println("first byte after memcpy: 0x", bigger.ptr[0]);

  // --- memmove ---
  // Safe for overlapping ranges.
  micron::memmove(bigger.ptr + 8, bigger.ptr, 8);

  // --- destroy ---
  micron::allocator_serial<>::destroy(bigger);
  micron::io::println("memory freed");

  // --- auto_size ---
  // Returns the allocator's granularity constant — allocations are
  // rounded up to this multiple (typically a page size or cache line).
  micron::io::println("allocator granularity=", micron::allocator_serial<>::auto_size());

  // --- get_grow ---
  // Returns the growth factor used by grow().
  micron::io::println("growth factor=", micron::allocator_serial<>::get_grow());

  return 0;
}
