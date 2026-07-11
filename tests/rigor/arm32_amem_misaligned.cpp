// arm32_amem_misaligned.cpp
// Regression for the arm32 SIMD alignment-trap bug: the `__block_*_16_a`
// inline-asm leaves in __bits/__asm_blocks_arm32.hpp used to emit
// `vld1.8 {qN},[rM :128]` / `vst1.8 {... :128]`. The `:128` alignment
// qualifier raises SIGBUS when the runtime pointer is not 16-byte aligned
// (qemu-verified). The arm64 siblings use plain non-trapping ldr/str; arm32
// must match. With the qualifier dropped, the `a*` / `nt*` entry points must
// run on a deliberately MISALIGNED pointer (buf+1) without faulting and still
// copy/set/compare correctly. The __builtin_assume_aligned in the wrappers is
// only an optimizer hint, so a misaligned argument is UB-by-contract but must
// not hardware-trap (exactly arm64's behavior).
//
// snowball convention: exit 1 == success; judge by the banner. Reaching the
// final OK banner proves BOTH that nothing SIGBUSed and that every byte landed
// correctly (require_true aborts on mismatch).
// Build & run: duck build tests/rigor/arm32_amem_misaligned.cpp --arm
//              && duck emulate tests/rigor/arm32_amem_misaligned.cpp --arm

#include "../../src/bits/__arch.hpp"

#if defined(__micron_arch_arm32) && defined(__micron_arm_neon)

#include "../../src/simd/arch/memory_arm32.hpp"

#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

namespace ms = micron::simd;

// A buffer large enough that buf+1 leaves room for several 16-byte blocks plus
// a tail remainder, with a leading byte to force misalignment and a trailing
// canary zone to catch any over-write past the requested length.
static constexpr u8 CANARY = 0xC3;

template<u64 N> struct mbuf {
  // 16-aligned storage; payload() = data+1 is therefore guaranteed misaligned.
  alignas(16) u8 data[1 + N + 16];

  mbuf() noexcept
  {
    for ( u64 i = 0; i < sizeof(data); i++ ) data[i] = CANARY;
  }

  u8 *
  payload() noexcept
  {
    return data + 1;      // off-by-one => not 16-byte aligned
  }

  bool
  tail_intact(u64 written) const noexcept
  {
    for ( u64 i = 1 + written; i < sizeof(data); i++ )
      if ( data[i] != CANARY ) return false;
    return true;
  }
};

int
main()
{
  print("=== ARM32 a*/nt* SIMD MISALIGNED-POINTER REGRESSION ===");

  // sanity: the pointer we hand the a*/nt* routines really is misaligned.
  {
    mbuf<128> probe;
    require_true((reinterpret_cast<u64>(probe.payload()) & 15u) != 0);
  }

  // -------------------------------------------------------------------------
  // amemcpy128 on a misaligned src+dest (was SIGBUS via __block_copy_16_a).
  test_case("amemcpy128 misaligned src+dest");
  {
    for ( u64 sz = 0; sz <= 100; sz++ ) {
      mbuf<128> src, dst;
      for ( u64 i = 0; i < sz; i++ ) src.payload()[i] = static_cast<u8>((i * 7u + 3u) & 0xFF);
      u8 *r = ms::amemcpy128(dst.payload(), src.payload(), sz);
      require_true(r == dst.payload());
      for ( u64 i = 0; i < sz; i++ ) require_true(dst.payload()[i] == src.payload()[i]);
      require_true(dst.tail_intact(sz));
    }
  }
  end_test_case();

  // -------------------------------------------------------------------------
  // amemmove128 (non-overlapping forward) on misaligned ptrs.
  test_case("amemmove128 misaligned, non-overlapping");
  {
    for ( u64 sz = 0; sz <= 100; sz++ ) {
      mbuf<128> src, dst;
      for ( u64 i = 0; i < sz; i++ ) src.payload()[i] = static_cast<u8>((i * 5u + 11u) & 0xFF);
      u8 *r = ms::amemmove128(dst.payload(), src.payload(), sz);
      require_true(r == dst.payload());
      for ( u64 i = 0; i < sz; i++ ) require_true(dst.payload()[i] == src.payload()[i]);
      require_true(dst.tail_intact(sz));
    }
  }
  end_test_case();

  // -------------------------------------------------------------------------
  // amemset128 on a misaligned dest (was SIGBUS via __block_set_16_a).
  test_case("amemset128 misaligned dest");
  {
    for ( u64 sz = 0; sz <= 100; sz++ ) {
      mbuf<128> dst;
      const u8 val = static_cast<u8>(0xA5 ^ (sz & 0xFF));
      u8 *r = ms::amemset128(dst.payload(), val, sz);
      require_true(r == dst.payload());
      for ( u64 i = 0; i < sz; i++ ) require_true(dst.payload()[i] == val);
      require_true(dst.tail_intact(sz));
    }
  }
  end_test_case();

  // -------------------------------------------------------------------------
  // ntmemset128 on a misaligned dest (used the :128 store too; now consistent
  // with ntmemcpy128's plain store, fence retained).
  test_case("ntmemset128 misaligned dest");
  {
    for ( u64 sz = 0; sz <= 100; sz++ ) {
      mbuf<128> dst;
      const u8 val = static_cast<u8>(0x3C ^ (sz & 0xFF));
      u8 *r = ms::ntmemset128(dst.payload(), val, sz);
      require_true(r == dst.payload());
      for ( u64 i = 0; i < sz; i++ ) require_true(dst.payload()[i] == val);
      require_true(dst.tail_intact(sz));
    }
  }
  end_test_case();

  // -------------------------------------------------------------------------
  // ntmemcpy128 on a misaligned dest (src intentionally misaligned too).
  test_case("ntmemcpy128 misaligned dest");
  {
    for ( u64 sz = 0; sz <= 100; sz++ ) {
      mbuf<128> src, dst;
      for ( u64 i = 0; i < sz; i++ ) src.payload()[i] = static_cast<u8>((i * 13u + 1u) & 0xFF);
      u8 *r = ms::ntmemcpy128(dst.payload(), src.payload(), sz);
      require_true(r == dst.payload());
      for ( u64 i = 0; i < sz; i++ ) require_true(dst.payload()[i] == src.payload()[i]);
      require_true(dst.tail_intact(sz));
    }
  }
  end_test_case();

  // -------------------------------------------------------------------------
  // amemcmp128 on misaligned ptrs (routes through __block_cmpeq_16's loads).
  test_case("amemcmp128 misaligned, equal + first-diff");
  {
    for ( u64 sz = 1; sz <= 100; sz++ ) {
      mbuf<128> a, b;
      for ( u64 i = 0; i < sz; i++ ) {
        const u8 v = static_cast<u8>((i * 3u + 2u) & 0xFF);
        a.payload()[i] = v;
        b.payload()[i] = v;
      }
      require_true(ms::amemcmp128(a.payload(), b.payload(), sz) == 0);

      // flip one byte and confirm the sign of the result matches the bytes.
      const u64 k = sz / 2;
      b.payload()[k] = static_cast<u8>(b.payload()[k] + 1u);
      const i64 got = ms::amemcmp128(a.payload(), b.payload(), sz);
      const i64 exp = static_cast<i64>(static_cast<unsigned>(a.payload()[k])) - static_cast<i64>(static_cast<unsigned>(b.payload()[k]));
      require_true(got == exp);
    }
  }
  end_test_case();

  print("[ARM32 a*/nt* MISALIGNED REGRESSION OK]");
  return 1;      // snowball pass sentinel (0 reads as FAIL to duck)
}

#else

#include "../snowball/snowball.hpp"

int
main()
{
  ::sb::print("=== ARM32 a*/nt* SIMD MISALIGNED-POINTER REGRESSION ===");
  ::sb::print("[skipped: not an armv7-a + NEON build]");
  return 1;
}

#endif
