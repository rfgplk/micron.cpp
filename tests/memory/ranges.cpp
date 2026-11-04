#include <micron/memory/ranges.hpp>

constexpr bool
constant_ranges()
{
  u8 bytes[8]{};
  char other[8]{};
  return micron::byte_ranges_overlap(bytes, 4, bytes + 3, 2) && micron::byte_ranges_overlap(bytes + 3, 2, bytes, 4)
         && !micron::byte_ranges_overlap(bytes, 4, bytes + 4, 4) && !micron::byte_ranges_overlap(bytes, 0, bytes, 8)
         && !micron::byte_ranges_overlap(bytes, 8, other, 8) && !micron::byte_ranges_overlap(static_cast<const u8 *>(nullptr), 8, bytes, 8);
}

static_assert(constant_ranges());

int
main()
{
  u8 bytes[64]{};
  char other[64]{};
  for ( usize a = 0; a <= 64; ++a )
    for ( usize b = 0; b <= 64; ++b )
      for ( usize n = 0; n <= 64 - a; ++n )
        for ( usize m = 0; m <= 64 - b; ++m ) {
          const bool expected = n != 0 && m != 0 && a < b + m && b < a + n;
          if ( micron::byte_ranges_overlap(bytes + a, n, bytes + b, m) != expected
               || micron::byte_ranges_overlap(static_cast<const void *>(bytes + a), n, bytes + b, m) != expected
               || micron::byte_ranges_overlap(bytes + a, n, other + b, m) )
            return 0;
        }
  return constant_ranges() ? 1 : 0;
}
