#include "../../src/simd/strings.hpp"
#include "../../src/io/console.hpp"
// NOTE: values must be UNIQUE within the array for "first match == i" to hold, which caps the
// element count at 200 so a u8 array stays collision-free. Needle 0 is never stored, so it is a
// guaranteed miss for every width.
template<typename T>
int
chk(const char *nm)
{
  constexpr usize N = 200;
  T a[N];
  for ( usize i = 0; i < N; ++i ) a[i] = (T)(i + 1);
  int bad = 0;
  for ( usize i = 0; i < N; ++i )
    if ( micron::simd::find_first_elem(a, N, (T)(i + 1)) != i ) ++bad;
  if ( micron::simd::find_first_elem(a, N, (T)0) != N ) ++bad;
  // short buffers exercise the scalar tail past every vector width
  for ( usize n = 0; n < 40; ++n )
    for ( usize k = 0; k < n; ++k )
      if ( micron::simd::find_first_elem(a, n, (T)(k + 1)) != k ) ++bad;
  if ( bad ) micron::io::println("  FAIL ", nm, " bad=", bad);
  return bad;
}

// find_first_ne_elem is find_first_elem with the compare mask negated, so it needs the same
// per-arch coverage. The two traps it can fall into are both about which mask bits are lanes:
// SSE2's movemask sets only the low 16, so inverting without masking finds a phantom hit at bit
// 16, and NEON's narrowed word uses all 64 (4 bits per byte) so it must NOT be masked.
template<typename T>
int
chk_ne(const char *nm)
{
  constexpr usize N = 200;
  T a[N];
  int bad = 0;

  // uniform buffer: no element differs, so every length must answer len
  for ( usize i = 0; i < N; ++i ) a[i] = (T)7;
  for ( usize n = 0; n <= N; ++n )
    if ( micron::simd::find_first_ne_elem(a, n, (T)7) != n ) ++bad;

  // one odd element walked across every position, at every length that can hold it
  for ( usize n = 1; n <= N; ++n ) {
    for ( usize pos = 0; pos < n; ++pos ) {
      a[pos] = (T)9;
      if ( micron::simd::find_first_ne_elem(a, n, (T)7) != pos ) ++bad;
      a[pos] = (T)7;
    }
  }

  // a needle that matches nothing means element 0 already differs
  for ( usize n = 1; n <= N; ++n )
    if ( micron::simd::find_first_ne_elem(a, n, (T)3) != 0 ) ++bad;

  if ( bad ) micron::io::println("  FAIL ne ", nm, " bad=", bad);
  return bad;
}

int
main()
{
  int bad = chk<u8>("u8") + chk<u16>("u16") + chk<u32>("u32") + chk<u64>("u64") + chk<i64>("i64") + chk<i16>("i16");
  bad += chk_ne<u8>("u8") + chk_ne<u16>("u16") + chk_ne<u32>("u32") + chk_ne<u64>("u64") + chk_ne<i64>("i64") + chk_ne<i16>("i16");
  micron::io::println("lane mismatches: ", bad);
  return bad == 0 ? 1 : 0;
}
