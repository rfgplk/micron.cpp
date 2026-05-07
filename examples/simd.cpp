// simd.cpp
// Tour of micron's portable SIMD layer (src/simd/).
//
// micron exposes vector types as concept-constrained C++ classes —
// not free intrinsic functions. The pattern is:
//
//   v128<T, Lane>   — 128-bit register, lane size set by `Lane`
//   v256<T, Lane>   — 256-bit (x86 only)
//   v512<T, Lane>   — 512-bit (x86 only)
//
// Concrete aliases (in `micron::` and `micron::simd::`):
//   v8 / v16 / v32 / v64        — 128-bit integer with 8/16/32/64-bit lanes
//   vfloat / vdouble            — 128-bit float / double
//   w8..w64 / wfloat / wdouble  — 256-bit (x86)
//   z8..z64 / zfloat / zdouble  — 512-bit (x86, AVX-512)
//
// Eigen-style names also exist: packet4f, packet2d, packet8i, packet16i, etc.
//
// Each class has:
//   - broadcast ctor    v128 v(3.14f)
//   - per-lane ctor     v128 v(1.0f, 2.0f, 3.0f, 4.0f)
//   - operator+= -= *= ^ | & on the whole register
//   - operator[](i)     scalar extract by lane
//   - get(arr)          store all lanes to a contiguous buffer
//   - load / uload      aligned / unaligned load from memory
//
// Architecture is selected at compile time. The same example builds
// on x86_64 (SSE2/AVX2) or aarch64 (NEON) — only the underlying
// intrinsic mapping changes.
//
// STL deltas:
//   - <experimental/simd> ships nothing this rich on stable releases.
//   - micron's containers (carray<T,N>, vector<T>) already use these
//     internally. You rarely write SIMD by hand; this layer is for
//     the cases when you need to.

#include "../src/io/console.hpp"
#include "../src/simd/simd.hpp"

int
main()
{
  // ================================================================
  // 1. vfloat — 4-lane single-precision
  // ----------------------------------------------------------------
  // Construct from per-lane values, broadcast a scalar, or zero-init.
  // ================================================================
  micron::io::println("-- 1. vfloat --");

  micron::vfloat a(1.0f, 2.0f, 3.0f, 4.0f);     // four lanes
  micron::vfloat b(0.5f);                         // broadcast
  a += b;

  // Extract each lane via operator[]
  micron::io::println("a + 0.5 = [", a[0], ", ", a[1], ", ", a[2], ", ", a[3], "]");

  // Store back into a contiguous array
  alignas(16) float out[4];
  a.get(out);
  micron::io::println("stored  = [", out[0], ", ", out[1], ", ", out[2], ", ", out[3], "]");

  // ================================================================
  // 2. vdouble — 2-lane double-precision (not on arm32)
  // ================================================================
#if !defined(__micron_arch_arm32)
  micron::io::println("-- 2. vdouble --");

  micron::vdouble d(1.5, 2.5);
  d *= micron::vdouble(2.0);
  micron::io::println("d * 2.0 = [", d[0], ", ", d[1], "]");
#endif

  // ================================================================
  // 3. v32 — 4-lane 32-bit integers (the i32 packet)
  // ----------------------------------------------------------------
  // packet4i is the Eigen-flavoured alias for v32.
  // ================================================================
  micron::io::println("-- 3. v32 (packet4i) --");

  micron::packet4i ints(10, 20, 30, 40);
  ints += micron::packet4i(1);     // broadcast +1 across all lanes
  micron::io::println("ints+1 = [", ints[0], ", ", ints[1], ", ", ints[2], ", ", ints[3], "]");

  // bitwise XOR on the whole register
  ints ^= micron::packet4i(0xFF);
  micron::io::println("ints^0xFF = [", ints[0], ", ", ints[1], ", ", ints[2], ", ", ints[3], "]");

  // ================================================================
  // 4. Loading from a memory buffer (uload — unaligned)
  // ----------------------------------------------------------------
  // load() requires 16-byte alignment and silently no-ops if violated.
  // uload() handles arbitrary alignment.
  // ================================================================
  micron::io::println("-- 4. load / uload --");

  alignas(16) float src[4] = {7.0f, 8.0f, 9.0f, 10.0f};
  micron::vfloat v;
  v.uload(src);
  micron::io::println("uloaded = [", v[0], ", ", v[1], ", ", v[2], ", ", v[3], "]");

  // ================================================================
  // 5. 256-bit (AVX2) — w*-prefixed types
  // ----------------------------------------------------------------
  // wfloat = 8 lanes of float. Only available on x86; the headers
  // gate on __micron_arch_x86_any.
  // ================================================================
#if defined(__micron_arch_x86_any)
  micron::io::println("-- 5. wfloat (256-bit) --");

  micron::wfloat w(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
  w += micron::wfloat(10.0f);
  micron::io::println("w[0]=", w[0], " w[7]=", w[7]);
#endif

  // ================================================================
  // 6. Where to look next
  // ----------------------------------------------------------------
  //   src/simd/math.hpp     — sqrt, rcp, rsqrt over vectors
  //   src/simd/fma.hpp      — fused multiply-add
  //   src/simd/bitwise.hpp  — and/or/xor/andnot helpers
  //   src/simd/dispatch.hpp — compile-time arch selection
  //   src/math/simd/        — vectorised exp/log/trig
  //
  // For everyday code, prefer micron::carray<T,N> / micron::vector<T>
  // which already SIMD-dispatch their arithmetic operators
  // internally — the intrinsics layer here is the escape hatch.
  // ================================================================
  micron::io::println("-- done --");
  return 0;
}
