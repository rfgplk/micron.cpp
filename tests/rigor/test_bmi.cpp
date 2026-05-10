// test_bmi.cpp
// Behavioral coverage for `micron::simd::bmi::*`.

#include "../../src/simd/aliases/bmi.hpp"
#include "../snowball/snowball.hpp"

namespace mb = ::micron::simd::bmi;

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

int
main()
{
  print("=== TEST BMI ===");

  test_case("bmi: tzcnt / lzcnt");
  require_true(mb::trailing_zeros_u32(0) == 32u);
  require_true(mb::trailing_zeros_u32(1) == 0u);
  require_true(mb::trailing_zeros_u32(0x80) == 7u);
  require_true(mb::trailing_zeros_u64(0xFFFFFFFFFFFFFFFFull) == 0ull);
  require_true(mb::leading_zeros_u32(0) == 32u);
  require_true(mb::leading_zeros_u32(1) == 31u);
  require_true(mb::leading_zeros_u32(0x80000000u) == 0u);
  end_test_case();

  test_case("bmi: blsr / blsi / blsmsk");
  require_true(mb::reset_lowest_set_u32(0xCAFEull) == 0xCAFCu);     // 0xCAFE = ...1110, clear bit1 -> ...1100
  require_true(mb::isolate_lowest_set_u32(0xCAFEu) == 0x0002u);
  require_true(mb::mask_below_lowest_set_u32(0xCAFEu) == 0x0003u);     // 0..0_11
  end_test_case();

  test_case("bmi: andn");
  require_true(mb::andn_u32(0xF0u, 0xFFu) == 0x0Fu);
  require_true(mb::andn_u64(0xFFul, 0xF0ul) == 0u);     // ~0xFF=0xFFF...0, & 0xF0 = 0
  end_test_case();

#if defined(__BMI2__)
  test_case("bmi: pext / pdep");
  // pext: gather selected bits per mask
  require_true(mb::parallel_extract_u32(0xC0F0u, 0xFFFFu) == 0xC0F0u);
  require_true(mb::parallel_extract_u32(0xCAFEu, 0xFFu) == 0xFEu);
  // pdep: scatter consecutive low bits to mask positions
  require_true(mb::parallel_deposit_u32(0xFu, 0xF0u) == 0xF0u);
  // pext . pdep round-trip with mask=all-ones is identity
  for ( unsigned x : { 0u, 1u, 0xCAFEu, 0xDEADBEEFu } ) {
    require_true(mb::parallel_extract_u32(mb::parallel_deposit_u32(x, 0xFFFFFFFFu), 0xFFFFFFFFu) == x);
  }
  end_test_case();
#endif

  test_case("bmi: bzhi");
  require_true(mb::zero_high_above_u32(0xFFFFFFFFu, 4) == 0xFu);
  require_true(mb::zero_high_above_u32(0xFFFFFFFFu, 16) == 0xFFFFu);
  require_true(mb::zero_high_above_u64(~0ull, 0) == 0ull);
  end_test_case();

  test_case("bmi: popcount");
  require_true(mb::popcount_u32(0) == 0);
  require_true(mb::popcount_u32(0xFFFFFFFFu) == 32);
  require_true(mb::popcount_u64(0xCAFEBABEFEEDFACEull) == __builtin_popcountll(0xCAFEBABEFEEDFACEull));
  end_test_case();

  test_case("bmi: bextr");
  // bextr extracts `len` bits starting at `start`. for 0xCAFE (= 0b1100_1010_1111_1110):
  // bits[4..11] = 0b10101111 = 0xAF.
  require_true(mb::extract_bits_u32(0xCAFEu, 4, 8) == 0xAFu);
  require_true(mb::extract_bits_u32(0xDEADBEEFu, 0, 16) == 0xBEEFu);
  end_test_case();

  print("[TEST BMI OK]");
  return 0;
}
