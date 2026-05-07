// types.cpp
// micron's fundamental type aliases (src/types.hpp).
//
// micron does not include <cstdint> or <cstddef>. All integer, float,
// pointer, and size types are defined in src/types.hpp directly from
// compiler built-in macros — portable without libc dependency.
//
// See also:
//   examples/numerics.cpp — numeric_limits<T> bounds
//   examples/concepts.cpp — concepts that pivot on these types
//   examples/io.cpp       — println dispatches per type

#include "../src/io/console.hpp"
#include "../src/types.hpp"

int
main()
{
  // ================================================================
  // 1. Fixed-width integers
  // ----------------------------------------------------------------
  // Short aliases for the verbose stdint names: i8/u8/i16/u16/i32/u32/i64/u64.
  // ================================================================
  micron::io::println("-- 1. fixed-width integers --");

  i8  a = -1;
  u8  b = 255;
  i16 c = -1000;
  u16 d = 65535;
  i32 e = -1'000'000;
  u32 f = 4'000'000'000u;
  i64 g = -9'000'000'000LL;
  u64 h = 18'000'000'000'000ULL;

  micron::io::println("i8 =", a, " u8 =", b);
  micron::io::println("i16=", c, " u16=", d);
  micron::io::println("i32=", e, " u32=", f);
  micron::io::println("i64=", g, " u64=", h);

  // ================================================================
  // 2. 128-bit integers (x86-64 only)
  // ----------------------------------------------------------------
  // i128 / u128 use __int128. There is no built-in printk for these
  // (you'd format manually); they're useful for hashes and crypto.
  // ================================================================
  micron::io::println("-- 2. 128-bit ints --");

  i128 big  = static_cast<i128>(-1) << 64;
  u128 ubig = static_cast<u128>(1)  << 100;
  micron::io::println("sizeof i128 = ", sizeof(big),  " bytes");
  micron::io::println("sizeof u128 = ", sizeof(ubig), " bytes");

  // ================================================================
  // 3. Size and pointer-sized types
  // ----------------------------------------------------------------
  // usize  — size_t  (unsigned, pointer-sized)
  // ssize_t — signed pointer-sized
  // ptr_t / addr_t — uintptr_t — store an address as an integer
  // ================================================================
  micron::io::println("-- 3. size / pointer types --");

  usize sz  = sizeof(u64);
  ptr_t raw = reinterpret_cast<ptr_t>(&sz);
  micron::io::println("sizeof(u64) = ", sz);
  micron::io::println("addr_t hex  = ", &sz);
  (void)raw;

  // ================================================================
  // 4. Fast integers
  // ----------------------------------------------------------------
  // fint8_t .. fint64_t — "fastest" unsigned type >= N bits, picked
  // from the compiler's __UINT_FAST*_TYPE__ macros. The width is
  // arch-dependent; on most 64-bit platforms fint32_t is u32.
  // ================================================================
  micron::io::println("-- 4. fast ints --");

  fint32_t fast = 42;
  micron::io::println("fint32_t = ", fast, "  sizeof=", sizeof(fast));

  // ================================================================
  // 5. Floating point
  // ----------------------------------------------------------------
  // f32 / f64 mirror float / double.
  // f128 = __float128 (GCC extension, may be 64-bit on some targets).
  // f16  available where _Float16 is supported.
  // flong = long double for the host.
  // ================================================================
  micron::io::println("-- 5. floats --");

  f32 x = 3.14f;
  f64 y = 2.718281828;
  micron::io::println("f32 = ", x);
  micron::io::println("f64 = ", y);

  // ================================================================
  // 6. max_t / umax_t
  // ----------------------------------------------------------------
  // The widest signed/unsigned integer the compiler offers
  // (intmax_t / uintmax_t). Used as accumulators in sum() / mean().
  // ================================================================
  micron::io::println("-- 6. max_t --");

  max_t  signed_max   = __INTMAX_MAX__;
  umax_t unsigned_max = __UINTMAX_MAX__;
  micron::io::println("max_t  = ", signed_max);
  micron::io::println("umax_t = ", unsigned_max);

  // ================================================================
  // 7. byte
  // ----------------------------------------------------------------
  // byte is uint8_t. Use it for raw memory buffers. println dispatches
  // it to the arithmetic overload so it prints as a number (not a
  // character). For hex, format manually or use io::bin (see io.cpp).
  // ================================================================
  micron::io::println("-- 7. byte --");

  byte buf[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  micron::io::println("buf[0] = ", buf[0]);

  // ================================================================
  // 8. Kernel / syscall types
  // ----------------------------------------------------------------
  // kernel_pid_t, kernel_uid_t, kernel_gid_t, ... mirror the Linux
  // ABI types — useful when writing raw syscall wrappers.
  // ================================================================
  micron::io::println("-- 8. kernel types --");

  kernel_pid_t pid = 1;
  micron::io::println("kernel_pid_t = ", pid, " sizeof=", sizeof(pid));

  return 0;
}
