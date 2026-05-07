// memory_memcpy_cmp.cpp
// micron's memcpy, memmove, and memcmp families (src/memory/cmemory/).
//
// See also:
//   examples/memory_memset.cpp — memset variants
//   examples/memory_alloc.cpp  — chunk<T>, allocator_serial
//
// memcpy variants:
//   memcpy(dest, src, count)    — basic copy, SIMD-optimized
//   cmemcpy<N>(dest, src)       — compile-time count, fully unrolled
//   bytecpy(dest, src, count)   — byte-level explicit form
//   voidcpy(dest, src, count)   — void pointer version
//
// memmove variants:
//   memmove(dest, src, count)   — handles overlapping ranges
//   bytemove(dest, src, count)  — byte-level form
//
// memcmp variants:
//   memcmp(src, dest, count)    — returns 0 if equal, offset of first diff otherwise
//   bytecmp(a, b, count)        — byte-level comparison
//   cmemcmp<N>(a, b)            — compile-time count
//
// Key difference from libc: memcmp returns the BYTE OFFSET of the first
// mismatch (not a sign indicating which is "greater"), and returns 0 on equal.

#include "../src/memory/cmemory.hpp"
#include "../src/io/console.hpp"

int
main()
{
  // ================================================================
  // MEMCPY
  // ================================================================

  // --- Basic memcpy ---
  byte src[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  byte dst[16] = {};
  micron::memcpy(dst, src, u64(16));
  micron::io::println("memcpy: dst[5]=", dst[5], " dst[15]=", dst[15]);

  // --- cmemcpy<N> — compile-time count, fully loop-unrolled ---
  // Use for small, fixed-size copies in hot paths
  byte cdst[8] = {};
  micron::cmemcpy<8>(cdst, src);
  micron::io::println("cmemcpy<8>: cdst[3]=", cdst[3]);

  // --- Typed memcpy — works on types larger than byte ---
  int isrc[4] = {100, 200, 300, 400};
  int idst[4] = {};
  micron::memcpy(idst, isrc, u64(4));   // copies 4 ints
  micron::io::println("memcpy<int>: idst[2]=", idst[2]);

  // --- bytecpy — same as memcpy but names intent as "byte copy" ---
  byte bcpy_dst[8] = {};
  micron::bytecpy(bcpy_dst, src, u64(8));
  micron::io::println("bytecpy: bcpy_dst[7]=", bcpy_dst[7]);

  // --- voidcpy — void pointer form (for type-erased buffers) ---
  byte vcpy_dst[8] = {};
  micron::voidcpy(static_cast<void *>(vcpy_dst),
                  static_cast<const void *>(src), u64(8));
  micron::io::println("voidcpy: vcpy_dst[4]=", vcpy_dst[4]);

  // --- Compile-time count: cmemcpy<N> ---
  // The N is a template arg, fully unrolled by the compiler.
  byte fc8[8] = {};
  micron::cmemcpy<8>(fc8, src);
  micron::io::println("cmemcpy<8>: fc8[7]=", fc8[7]);

  // ================================================================
  // MEMMOVE — safe for overlapping ranges
  // ================================================================

  // memmove handles dest > src (backward copy to avoid corruption)
  byte mv[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  // Shift elements 0..7 right by 4 positions (overlaps with dest 4..11)
  micron::memmove(mv + 4, mv, u64(8));
  micron::io::println("memmove(+4): mv[4]=", mv[4], " mv[8]=", mv[8]);   // 0, 4

  // bytemove — explicit byte-level form
  byte bm[16] = {10, 20, 30, 40, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  micron::bytemove(bm + 2, bm, u64(5));
  micron::io::println("bytemove(+2): bm[2]=", bm[2], " bm[4]=", bm[4]);

  // ================================================================
  // MEMCMP — comparison
  // Returns 0 if equal; byte offset of first mismatch otherwise.
  // This is a DEVIATION from libc which returns negative/zero/positive.
  // ================================================================

  byte ca[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  byte cb[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  byte cc[8] = {1, 2, 3, 99, 5, 6, 7, 8};   // differs at index 3

  // memcmp requires explicit T argument: memcmp<ComparisonType>(src, dest, cnt)
  // T is the unit of comparison (byte for byte-by-byte, int for word-by-word)
  i64 eq   = micron::memcmp<byte>(ca, cb, u64(8));
  i64 diff = micron::memcmp<byte>(ca, cc, u64(8));
  micron::io::println("memcmp(equal)=",       eq,    " (0 means equal)");
  micron::io::println("memcmp(diff at 3)=",   diff,  " (byte offset, may differ from libc)");

  // --- bytecmp — byte comparison form (byte is explicit alias) ---
  i64 bcmp_r = micron::bytecmp(ca, cb, u64(8));
  micron::io::println("bytecmp(equal)=", bcmp_r);

  // --- cmemcmp<N, T> — compile-time count, comparison type required ---
  i64 ccmp = micron::cmemcmp<8, byte>(ca, cb);
  micron::io::println("cmemcmp<8,byte>(equal)=", ccmp);

  // ================================================================
  // Interaction: copy then compare to verify
  // ================================================================
  int isrc2[4] = {1, 2, 3, 4};
  int idst2[4] = {};
  micron::memcpy(idst2, isrc2, u64(4));
  i64 verify = micron::memcmp<int>(isrc2, idst2, u64(4));
  micron::io::println("copy+compare verify=", verify == 0 ? "ok" : "MISMATCH");

  // ================================================================
  // constexpr_memset + constexpr contexts
  // ================================================================
  // constexpr_memset can be used in constexpr functions
  constexpr auto make_buf = []() {
    byte b[8] = {};
    micron::constexpr_memset(b, 0xAA, u64(8));
    return b[0];
  };
  static_assert(make_buf() == 0xAA);
  micron::io::println("constexpr_memset in constexpr context: ok");

  return 0;
}
