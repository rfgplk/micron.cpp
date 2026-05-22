// Focused diagnostic: does abc::alloc / abc::realloc ever return a block whose
// [p, p+n) range overlaps an already-live allocation? Single-threaded, so the
// only writer of user memory is this probe. We check each freshly returned
// block against every live block at the instant it is handed out.

#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/math/rng/engines.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../snowball/snowball.hpp"

using namespace snowball;
using rng_t = micron::math::rng::xoshiro256ss;

// Total ops. Deterministic regression default runs well past the historical
// failure point (op 121944, the __size_of_alloc head/tail bug). Crank up with
// -D OVERLAP_OPS=<n> for a longer soak.
#ifndef OVERLAP_OPS
#define OVERLAP_OPS 2000000ull
#endif

namespace
{
constexpr usize N = 4096;
byte *ptr[N];
usize sz[N];

constexpr usize SZ_PRECISE = abc::__class_precise;
constexpr usize SZ_SMALL = abc::__class_small;
constexpr usize SZ_MEDIUM = abc::__class_medium;
constexpr usize SZ_LARGE = abc::__class_large;
constexpr usize SZ_HUGE = abc::__class_huge;

usize
longtail(rng_t &r)
{
  const u32 t = static_cast<u32>(r.next() % 1000u);
  usize lo, hi;
  if ( t < 700u ) {
    lo = 1;
    hi = SZ_PRECISE;
  } else if ( t < 880u ) {
    lo = SZ_PRECISE + 1;
    hi = SZ_SMALL;
  } else if ( t < 960u ) {
    lo = SZ_SMALL + 1;
    hi = SZ_MEDIUM;
  } else if ( t < 990u ) {
    lo = SZ_MEDIUM + 1;
    hi = SZ_LARGE;
  } else {
    lo = SZ_LARGE + 1;
    hi = SZ_HUGE;
  }
  return lo + static_cast<usize>(r.next() % (hi - lo + 1));
}

// returns the first live slot whose range overlaps [p,p+n), or N if none
usize
find_overlap(usize self, const byte *p, usize n)
{
  for ( usize t = 0; t < N; ++t ) {
    if ( t == self || !ptr[t] ) continue;
    const byte *q = ptr[t];
    const usize m = sz[t];
    const bool disjoint = (q + m <= p) || (p + n <= q);
    if ( !disjoint ) return t;
  }
  return N;
}

void
dump(const char *what, usize s, const byte *p, usize n, usize t)
{
  sb::print(what);
  sb::print("  new  slot=", s, " ptr=", reinterpret_cast<const void *>(p), " sz=", n, " qsize=", abc::query_size(const_cast<byte *>(p)),
            " present=", abc::is_present(const_cast<byte *>(p)));
  sb::print("  live slot=", t, " ptr=", reinterpret_cast<const void *>(ptr[t]), " sz=", sz[t], " qsize=", abc::query_size(ptr[t]),
            " present=", abc::is_present(ptr[t]));
  const byte *q = ptr[t];
  const usize m = sz[t];
  const byte *lo = (p < q) ? p : q;
  const byte *hi = (p + n > q + m) ? (p + n) : (q + m);
  sb::print("  span=", static_cast<usize>(hi - lo), " (new is ", (p < q ? "below" : "at/above"), " live)");
}
}      // namespace

int
main(void)
{
  sb::print("=== ABC OVERLAP PROBE ===");
  rng_t r = rng_t::from_seed(0x5040A11ull);
  for ( usize i = 0; i < N; ++i ) {
    ptr[i] = nullptr;
    sz[i] = 0;
  }

  u64 allocs = 0, frees = 0, reallocs = 0;
  for ( u64 it = 0; it < OVERLAP_OPS; ++it ) {
    const u32 op = static_cast<u32>(r.next() % 100u);
    const usize s = static_cast<usize>(r.next() % N);

    if ( op < 45u ) {
      if ( ptr[s] == nullptr ) {
        const usize n = longtail(r);
        byte *p = abc::alloc(n);
        if ( !p ) continue;
        const usize t = find_overlap(s, p, n);
        if ( t != N ) {
          dump("OVERLAP on alloc:", s, p, n, t);
          sb::print("  at op=", static_cast<usize>(it), " allocs=", static_cast<usize>(allocs));
          require_true(false);
        }
        ptr[s] = p;
        sz[s] = n;
        ++allocs;
      }
    } else if ( op < 80u ) {
      if ( ptr[s] ) {
        abc::dealloc(ptr[s]);
        ptr[s] = nullptr;
        sz[s] = 0;
        ++frees;
      }
    } else {
      if ( ptr[s] ) {
        const usize n = longtail(r);
        byte *q = static_cast<byte *>(abc::realloc(ptr[s], n));
        if ( !q ) continue;
        ptr[s] = nullptr;
        sz[s] = 0;      // remove old mapping before overlap test
        const usize t = find_overlap(s, q, n);
        if ( t != N ) {
          dump("OVERLAP on realloc:", s, q, n, t);
          sb::print("  at op=", static_cast<usize>(it), " reallocs=", static_cast<usize>(reallocs));
          require_true(false);
        }
        ptr[s] = q;
        sz[s] = n;
        ++reallocs;
      }
    }

    if ( (it % 1000000ull) == 0u )
      sb::print("  op=", static_cast<usize>(it), " allocs=", static_cast<usize>(allocs), " frees=", static_cast<usize>(frees),
                " reallocs=", static_cast<usize>(reallocs));
  }
  require_true(true);      // reached the end with no overlap handed out
  sb::print("=== NO OVERLAP DETECTED (ops=", static_cast<usize>(OVERLAP_OPS), " reallocs=", static_cast<usize>(reallocs), ") ===");
  return 1;
}
