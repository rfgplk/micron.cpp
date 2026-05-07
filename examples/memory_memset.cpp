// memory_memset.cpp
// micron's rich memset family (src/memory/cmemory/memset.hpp).
//
// See also:
//   examples/memory_memcpy_cmp.cpp — memcpy / memmove / memcmp
//   examples/memory_alloc.cpp      — chunk<T>, allocator_serial
//
// Unlike libc memset (always fills bytes), micron provides:
//   memset(ptr, byte, count)    — byte fill (like libc)
//   byteset(ptr, byte, count)   — explicit "this is byte-level" alias
//   typeset<T>(ptr, T val, n)   — fill with T-sized values (not bytes!)
//   zero(ptr, count)            — fill with 0
//   cmemset<N>(ptr, byte)       — compile-time count (loop-unrolled)
//   invert(ptr, count)          — bitwise NOT in-place
//   and_mask/or_mask/xor_mask   — bitwise ops across a buffer
//   increment/decrement         — ++ / -- each element
// All dispatch to AVX2/SSE2/NEON where available.

#include "../src/memory/cmemory.hpp"
#include "../src/io/console.hpp"

int
main()
{
  // ----------------------------------------------------------------
  // memset — byte fill (identical to libc memset, but SIMD-optimized)
  // ----------------------------------------------------------------
  byte buf[32] = {};
  micron::memset(buf, 0xAB, 32);
  micron::io::println("memset 0xAB: buf[0]=", buf[0], " buf[31]=", buf[31]);

  // ----------------------------------------------------------------
  // byteset — explicit byte-level fill (same as memset, clearer intent)
  // ----------------------------------------------------------------
  byte buf2[16] = {};
  micron::byteset(buf2, 0xFF, 16);
  micron::io::println("byteset 0xFF: buf2[0]=", buf2[0]);

  // ----------------------------------------------------------------
  // typeset<T> — fill with T-sized values, NOT byte values
  // typeset<int>(ptr, 0x0102, n) writes int(0x0102) = {02, 01, 00, 00}
  // unlike memset which would write byte 0x02 four times per int.
  // ----------------------------------------------------------------
  int ibuf[8] = {};
  micron::typeset(ibuf, 42, u64(8));   // fill 8 ints with value 42
  micron::io::println("typeset<int>(42): ibuf[0]=", ibuf[0], " ibuf[7]=", ibuf[7]);

  f64 fbuf[4] = {};
  micron::typeset(fbuf, 3.14, u64(4));
  micron::io::println("typeset<f64>(3.14): fbuf[0]=", fbuf[0]);

  // ----------------------------------------------------------------
  // zero / czero — fill with zero (optimized path vs memset(ptr,0,n))
  // ----------------------------------------------------------------
  int zbuf[16] = {};
  micron::typeset(zbuf, 99, u64(16));   // first fill with non-zero
  micron::zero(zbuf, u64(16));
  micron::io::println("zero: zbuf[0]=", zbuf[0], " zbuf[15]=", zbuf[15]);

  // ----------------------------------------------------------------
  // cmemset<N> — compile-time count: loop is fully unrolled at compile time
  // Use when N is a compile-time constant for guaranteed unrolling
  // ----------------------------------------------------------------
  byte cbuf[8] = {};
  micron::cmemset<8>(cbuf, 0xCD);    // unrolled 8-element fill
  micron::io::println("cmemset<8>(0xCD): cbuf[0]=", cbuf[0]);

  // ----------------------------------------------------------------
  // memset<byte, N, F> — non-type template: both byte and count are constexpr
  // The most aggressively optimized form; everything is compile-time
  // ----------------------------------------------------------------
  byte cct_buf[16] = {};
  micron::memset<0x55, 16>(cct_buf);
  micron::io::println("memset<0x55,16>: cct_buf[0]=", cct_buf[0]);

  // ----------------------------------------------------------------
  // smemset — safe variant: validates alignment, returns pointer or nullptr
  // ----------------------------------------------------------------
  byte safe_buf[32] = {};
  byte *sr = micron::smemset(safe_buf, 0x77, 32);
  micron::io::println("smemset ok=", sr != nullptr);

  // ----------------------------------------------------------------
  // invert — bitwise NOT every byte in the buffer
  // ----------------------------------------------------------------
  byte inv[4] = {0x00, 0x0F, 0xF0, 0xFF};
  micron::invert(inv, u64(4));
  micron::io::println("invert: [0x00->", inv[0], "] [0x0F->", inv[1], "]");

  // ----------------------------------------------------------------
  // and_mask / or_mask / xor_mask
  // Apply a bitmask to every element of a buffer
  // ----------------------------------------------------------------
  byte mask_buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  micron::and_mask(mask_buf, byte(0x0F), u64(4));   // keep low nibble only
  micron::io::println("and_mask(0x0F): [0]=", mask_buf[0]);   // 0x0F

  byte or_buf[4] = {0x00, 0x00, 0x00, 0x00};
  micron::or_mask(or_buf, byte(0x80), u64(4));   // set high bit
  micron::io::println("or_mask(0x80): [0]=", or_buf[0]);   // 0x80

  byte xor_buf[4] = {0xAA, 0xAA, 0xAA, 0xAA};
  micron::xor_mask(xor_buf, byte(0xFF), u64(4));   // flip all bits
  micron::io::println("xor_mask(0xFF): [0]=", xor_buf[0]);   // 0x55

  // ----------------------------------------------------------------
  // increment / decrement — apply ++ / -- to every element
  // ----------------------------------------------------------------
  byte inc_buf[4] = {1, 2, 3, 4};
  micron::increment(inc_buf, u64(4));
  micron::io::println("increment: [0..3]=", inc_buf[0], " ", inc_buf[1], " ", inc_buf[2], " ", inc_buf[3]);

  micron::decrement(inc_buf, u64(4));
  micron::io::println("decrement back: [0]=", inc_buf[0]);

  // ----------------------------------------------------------------
  // bzero — explicit zero-fill for byte buffers (alias family)
  // ----------------------------------------------------------------
  byte bz[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  micron::bzero(bz, u64(8));
  micron::io::println("bzero: bz[0]=", bz[0], " bz[7]=", bz[7]);

  // ----------------------------------------------------------------
  // Fixed-width fast variants: memset_8b/16b/32b/64b
  // Unrolled for specific sizes, useful in hot inner loops
  // ----------------------------------------------------------------
  byte fb8[8] = {};
  micron::memset_8b(fb8, 0x42);
  micron::io::println("memset_8b(0x42): fb8[0]=", fb8[0]);

  byte fb16[16] = {};
  micron::memset_16b(fb16, 0x1A);
  micron::io::println("memset_16b(0x1A): fb16[15]=", fb16[15]);

  return 0;
}
