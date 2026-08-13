// rigor_algo_fp_filter.cpp — snowball suite for
//   src/algorithm/filter.hpp + src/algorithm/fpfilter.hpp
//
// Coverage:
//   filter (pointer + container variants)
//   prune
//   reject
//   partition
//   take / drop
//   take_while / drop_while
//   span / sbreak
//   unique / nub
//   filter_c / reject_c / partition_c / take_c / drop_c (curried)
//   take_while_c / drop_while_c
//
// Container-returning algorithms use vector::resize to shrink the output;
// the recent vector::resize patch (project-vector-resize-only-grows) makes
// these tests' .size() assertions meaningful.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/filter.hpp"
#include "../../src/algorithm/find.hpp"
#include "../../src/algorithm/fpfilter.hpp"
#include "../../src/algorithm/fpmap.hpp"
#include "../../src/strings.hpp"

#include "../support/algo_rigor.hpp"

namespace
{

// what nub is DEFINED as, in the least clever way available: keep an element the first time
// operator== has not seen it. Neither of nub's two paths is allowed to differ from this.
template<typename C>
usize
oracle_nub_size(const C &c)
{
  micron::fvector<typename C::value_type> keep;
  for ( const auto &e : c ) {
    bool seen = false;
    for ( usize j = 0; j < keep.size(); ++j )
      if ( keep[j] == e ) {
        seen = true;
        break;
      }
    if ( !seen ) keep.push_back(e);
  }
  return keep.size();
}

};      // namespace

using namespace mtest::rigor;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== ALGO/FP-FILTER RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // filter (pointer + container)
  // ════════════════════════════════════════════════════════════════════

  test_case("filter[ptr] selects matching to output");
  {
    int src[10];
    pat_sorted(src, 10);
    int out[10] = {};
    auto *last = micron::filter(src, src + 10, [](const int *x) { return ((*x) & 1) == 0; }, out);
    auto cnt = static_cast<usize>(last - out);
    require(cnt, usize(5));
    int expected[5] = { 0, 2, 4, 6, 8 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("filter[ptr,limit] respects bound");
  {
    int src[10];
    pat_sorted(src, 10);
    int out[10] = {};
    auto *last = micron::filter(src, src + 10, [](const int *x) { return *x >= 0; }, out, usize(3));
    auto cnt = static_cast<usize>(last - out);
    require(cnt, usize(3));
  }
  end_test_case();

  test_case("filter[container] returns vector of matches");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto out = micron::filter(v, [](const int *x) { return ((*x) & 1) == 0; });
    require(out.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(out[i], i * 2);
  }
  end_test_case();

  test_case("filter_inplace shrinks in place");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    micron::filter_inplace(v, [](const int *x) { return (*x) % 3 == 0; });
    // matches: 0, 3, 6, 9
    require(v.size(), usize(4));
    require(v[0], 0);
    require(v[1], 3);
    require(v[2], 6);
    require(v[3], 9);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // prune  (filter to bounded output, returns iter into output)
  // ════════════════════════════════════════════════════════════════════

  test_case("prune[ptr] returns input position, fills bounded output");
  {
    int src[10];
    pat_sorted(src, 10);
    int out[3] = {};
    // prune returns the position in INPUT where it stopped (after writing
    // `limit` matches). For pat_sorted(0..9) with predicate "x >= 0" and
    // limit=3, we stop after consuming src[0..2] and writing out[0..2].
    auto *input_pos = micron::prune(src, src + 10, [](const int *x) { return *x >= 0; }, out, usize(3));
    auto consumed = static_cast<usize>(input_pos - src);
    require(consumed, usize(3));
    require(out[0], 0);
    require(out[1], 1);
    require(out[2], 2);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // reject (fp namespace)
  // ════════════════════════════════════════════════════════════════════

  test_case("reject is complement of filter");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto out = micron::fp::reject(v, [](int x) { return (x & 1) == 0; });
    // rejected (kept the odd): 1, 3, 5, 7, 9
    require(out.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(out[i], 2 * i + 1);
  }
  end_test_case();

  test_case("reject all-false predicate yields full copy");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i + 1;
    auto out = micron::fp::reject(v, [](int) { return false; });
    require(out.size(), v.size());
    for ( usize i = 0; i < v.size(); ++i ) require(out[i], v[i]);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // partition (fp)
  // ════════════════════════════════════════════════════════════════════

  test_case("partition splits into (matching, non-matching)");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto pr = micron::fp::partition(v, [](int x) { return (x & 1) == 0; });
    auto &matching = micron::get<0>(pr);
    auto &rest = micron::get<1>(pr);
    require(matching.size(), usize(5));
    require(rest.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(matching[i], i * 2);
    for ( int i = 0; i < 5; ++i ) require(rest[i], 2 * i + 1);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // take / drop
  // ════════════════════════════════════════════════════════════════════

  test_case("take(n) returns first n");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i + 1;
    auto out = micron::fp::take(v, usize(4));
    require(out.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("take(n) where n > size returns all");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto out = micron::fp::take(v, usize(100));
    require(out.size(), usize(5));
  }
  end_test_case();

  test_case("take(0) returns empty");
  {
    micron::vector<int> v(5, 0);
    auto out = micron::fp::take(v, usize(0));
    require(out.size(), usize(0));
  }
  end_test_case();

  test_case("drop(n) drops first n");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i + 1;
    auto out = micron::fp::drop(v, usize(4));
    require(out.size(), usize(6));
    for ( int i = 0; i < 6; ++i ) require(out[i], i + 5);
  }
  end_test_case();

  test_case("drop(n >= size) returns empty");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto out = micron::fp::drop(v, usize(10));
    require(out.size(), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // take_while / drop_while
  // ════════════════════════════════════════════════════════════════════

  test_case("take_while takes until predicate fails");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 100, 5, 6, 7 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto out = micron::fp::take_while(v, [](int x) { return x < 10; });
    require(out.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("drop_while drops until predicate fails");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 100, 5, 6, 7 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto out = micron::fp::drop_while(v, [](int x) { return x < 10; });
    require(out.size(), usize(4));
    require(out[0], 100);
    require(out[1], 5);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // span / sbreak  (split on predicate)
  // ════════════════════════════════════════════════════════════════════

  test_case("span splits at first predicate failure");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 100, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto pr = micron::fp::span(v, [](int x) { return x < 10; });
    auto &head = micron::get<0>(pr);
    auto &tail = micron::get<1>(pr);
    require(head.size(), usize(3));
    require(tail.size(), usize(5));
    require(tail[0], 100);
  }
  end_test_case();

  test_case("sbreak is complement of span");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 100, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto pr = micron::fp::sbreak(v, [](int x) { return x >= 10; });
    auto &head = micron::get<0>(pr);
    auto &tail = micron::get<1>(pr);
    require(head.size(), usize(3));
    require(tail.size(), usize(5));
    require(tail[0], 100);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // unique / nub  (remove consecutive duplicates)
  // ════════════════════════════════════════════════════════════════════

  test_case("unique removes consecutive duplicates");
  {
    micron::vector<int> v(10, 0);
    int d[10] = { 1, 1, 2, 2, 2, 3, 1, 1, 4, 4 };
    for ( int i = 0; i < 10; ++i ) v[i] = d[i];
    auto out = micron::fp::unique(v);
    // expected: [1, 2, 3, 1, 4]
    require(out.size(), usize(5));
    int expected[5] = { 1, 2, 3, 1, 4 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("nub aliases unique");
  {
    micron::vector<int> v(6, 0);
    int d[6] = { 1, 1, 2, 3, 3, 4 };
    for ( int i = 0; i < 6; ++i ) v[i] = d[i];
    auto a = micron::fp::nub(v);
    auto b = micron::fp::unique(v);
    require(a.size(), b.size());
    for ( usize i = 0; i < a.size(); ++i ) require(a[i], b[i]);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Curried variants
  // ════════════════════════════════════════════════════════════════════

  test_case("filter_c partial application");
  {
    auto evens_c = micron::fp::filter_c([](const int *x) { return ((*x) & 1) == 0; });
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i;
    auto out = evens_c(v);
    require(out.size(), usize(3));
  }
  end_test_case();

  test_case("reject_c partial application");
  {
    auto not_evens_c = micron::fp::reject_c([](int x) { return (x & 1) == 0; });
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i;
    auto out = not_evens_c(v);
    require(out.size(), usize(3));
    require(out[0], 1);
    require(out[1], 3);
    require(out[2], 5);
  }
  end_test_case();

  test_case("take_c partial application");
  {
    auto take_3_c = micron::fp::take_c(usize(3));
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i + 1;
    auto out = take_3_c(v);
    require(out.size(), usize(3));
    for ( int i = 0; i < 3; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("drop_c partial application");
  {
    auto drop_2_c = micron::fp::drop_c(usize(2));
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i + 1;
    auto out = drop_2_c(v);
    require(out.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 3);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Property tests
  // ════════════════════════════════════════════════════════════════════

  property_test(
      "filter then reject of same predicate yields full input length (10k)",
      [](u32 raw_n) {
        usize n = (raw_n & 0xf) + 1;
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 109);
        int buf[16];
        pat_random_small(buf, n, rng, 0, 100);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto pred_v = [](int x) { return x < 50; };
        auto pred_p = [](const int *x) { return *x < 50; };
        auto kept = micron::filter(v, pred_p);
        auto rejected = micron::fp::reject(v, pred_v);
        require(kept.size() + rejected.size(), n);
      },
      10000);

  property_test(
      "take(n) + drop(n) concatenate to original (10k)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0xf) + 1;
        usize k = raw_k % (n + 1);
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 113);
        int buf[16];
        pat_random_small(buf, n, rng, -100, 100);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto a = micron::fp::take(v, k);
        auto b = micron::fp::drop(v, k);
        require(a.size() + b.size(), n);
        for ( usize i = 0; i < a.size(); ++i ) require(a[i], v[i]);
        for ( usize i = 0; i < b.size(); ++i ) require(b[i], v[k + i]);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // nub takes a hash set when the element type is hashable and the old quadratic rescan when it is
  // not. Both must keep the FIRST occurrence and the original order, so the two paths are each
  // other's oracle -- and a plain naive scan is the oracle for both.
  // ════════════════════════════════════════════════════════════════════

  test_case("nub: hashed and quadratic paths agree with each other and a naive oracle");
  {
    // Opaque has operator== and nothing else, so micron::hash rejects it and it takes the fallback
    struct Opaque {
      int v;
      bool
      operator==(const Opaque &o) const
      {
        return v == o.v;
      }
    };
    static_assert(micron::fp::__impl::hash_dedupable<int>, "int must take the hash path");
    static_assert(!micron::fp::__impl::hash_dedupable<Opaque>, "Opaque must take the quadratic path");

    u64 st = 0xD15EA5Eull;
    auto nx = [&st]() {
      st ^= st << 13;
      st ^= st >> 7;
      st ^= st << 17;
      return st;
    };
    for ( int trial = 0; trial < 300; ++trial ) {
      const usize n = nx() % 250;
      // a narrow alphabet forces heavy duplication, which is what nub is for
      const int alpha = static_cast<int>(1 + nx() % 40);
      micron::fvector<int> a;
      micron::fvector<Opaque> b;
      for ( usize i = 0; i < n; ++i ) {
        const int v = static_cast<int>(nx() % static_cast<u64>(alpha));
        a.push_back(v);
        b.push_back(Opaque{ v });
      }

      micron::fvector<int> oracle;
      for ( usize i = 0; i < n; ++i ) {
        bool seen = false;
        for ( usize j = 0; j < oracle.size(); ++j )
          if ( oracle[j] == a[i] ) {
            seen = true;
            break;
          }
        if ( !seen ) oracle.push_back(a[i]);
      }

      auto ra = micron::fp::nub(a);
      auto rb = micron::fp::nub(b);
      require(ra.size(), oracle.size());
      require(rb.size(), oracle.size());
      for ( usize i = 0; i < oracle.size(); ++i ) {
        require(ra[i], oracle[i]);
        require(rb[i].v, oracle[i]);
      }
    }

    // degenerate shapes
    micron::fvector<int> empty;
    require(micron::fp::nub(empty).size(), usize(0));
    micron::fvector<int> same;
    for ( int i = 0; i < 64; ++i ) same.push_back(9);
    require(micron::fp::nub(same).size(), usize(1));
    require(micron::fp::nub(same)[0], 9);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // nub is defined by operator==, so the hash set may only stand in for the == rescan where hash and
  // == agree. Floating point is where they do not: +0.0 and -0.0 compare equal and hash differently,
  // so a hashed float nub kept both -- and only on some builds, since the AVX2 zzz hash happens to
  // seat the two zeros in one slot where the SSE2 rapidhash does not. An answer that moves with
  // -march is the half of that defect most worth pinning, so these cases are worth a run under
  // --isa base as well as the default --isa native.
  // ════════════════════════════════════════════════════════════════════

  test_case("nub[float]: +/-0.0 collapse and floating point stays off the hash path");
  {
    static_assert(!micron::fp::__impl::hash_dedupable<float>, "float must not take nub's hash path");
    static_assert(!micron::fp::__impl::hash_dedupable<double>, "double must not take nub's hash path");
    static_assert(micron::fp::__impl::hash_dedupable<int>, "int keeps the hash path");
    static_assert(micron::fp::__impl::hash_dedupable<micron::string>, "strings keep the hash path");

    // -Ofast folds a -0.0f literal to +0.0f (-fno-signed-zeros), so carry the sign in through a
    // volatile and check it actually arrived before asserting anything about it
    volatile u32 nz_bits = 0x80000000u;
    const u32 nb = nz_bits;
    const float nz = __builtin_bit_cast(float, nb);
    require_true(__builtin_bit_cast(u32, nz) == 0x80000000u);
    require_true(nz == 0.0f);

    // walk the pad so the seen-set is sized across several capacities -- the defect only surfaced
    // once the two zeros' h1 landed in different buckets
    for ( usize pad = 0; pad < 96; ++pad ) {
      micron::fvector<float> v;
      for ( usize i = 0; i < pad; ++i ) v.push_back(static_cast<float>(i + 10));
      v.push_back(0.0f);
      v.push_back(nz);
      v.push_back(1.0f);
      v.push_back(0.0f);
      auto r = micron::fp::nub(v);
      require(r.size(), pad + 2);
      // first occurrence wins, so the surviving zero is the +0.0 that came first
      require_true(__builtin_bit_cast(u32, r[pad]) == 0u);
      require_true(r[pad + 1] == 1.0f);
    }

    // the same through a map's mapped_type: fpmap/fptree nub share this seen-set
    micron::heap_swiss_map<int, float> m;
    m.insert(1, 0.0f);
    m.insert(2, nz);
    m.insert(3, 1.0f);
    require(micron::fp::nub(m).size(), usize(2));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // The other half of the same contract, and the one the float fix left open. is_string_ascii is a
  // pure SHAPE test -- c_str(), data(), size(), the iterator pair -- and says nothing about size()
  // being the content length. fixed_string<N>'s is the CAPACITY (N-1) while its operator== compares
  // len() bytes, so hashing size() bytes made two fixed_strings with identical content and
  // different bytes past the terminator compare equal and hash differently. The hash now digests
  // micron::string_len(), which is len() where a type has one, so the whitelist entry is true
  // rather than merely convenient -- and every hashed container keyed on a fixed_string is fixed
  // with it, not just nub.
  // ════════════════════════════════════════════════════════════════════

  test_case("nub[fixed_string]: hashing the CAPACITY is what == never compares");
  {
    using fs8 = micron::fixed_string<8>;
    static_assert(micron::fp::__impl::hash_dedupable<fs8>, "fixed_string keeps the hash path");
    static_assert(micron::fp::__impl::hash_dedupable<micron::sstring<16>>, "sstring keeps the hash path");

    // size() is the capacity and len() is the content; they agree only on a full buffer
    require(fs8{ "abc\0def" }.size(), usize(7));
    require(fs8{ "abc\0def" }.len(), usize(3));

    // the two reachable constructions of a stale tail. the array ctor copies all N bytes verbatim;
    // buf is public by design (fixed_string has to stay structural to be an NTTP), so truncating
    // through it leaves whatever was past the cut in place.
    fs8 a{ "abc\0def" };
    fs8 b("abc", 3);      // the (ptr, n) ctor leaves the NSDMI zeroes behind it
    fs8 c{ "abcdefg" };
    c.buf[3] = 0;

    const auto ha = micron::hash<micron::hash64_t>(a);
    require_true(a == b);
    require_true(micron::hash<micron::hash64_t>(b) == ha);
    require_true(a == c);
    require_true(micron::hash<micron::hash64_t>(c) == ha);

    // cross-N equality is by CONTENT (rigor_fixed_string pins it), so the hash has to agree there
    // too or a heterogeneous lookup misses on a key it compares equal to
    micron::fixed_string<4> s4{ "abc" };      // the array ctor wants exactly N chars, NUL included
    micron::fixed_string<32> s32("abc", 3);
    require_true(s4 == a and s32 == a);
    require_true(micron::hash<micron::hash64_t>(s4) == ha);
    require_true(micron::hash<micron::hash64_t>(s32) == ha);

    // and the whole point: nub is defined by operator==, so it must answer what == says
    fs8 z1("zz", 2);
    fs8 z2{ "zzXXXXX" };
    z2.buf[2] = 0;      // content "zz", tail left dirty -- the same shape as c, different content

    micron::fvector<fs8> v;
    v.push_back(a);
    v.push_back(b);
    v.push_back(z1);
    v.push_back(c);
    v.push_back(z2);
    auto r = micron::fp::nub(v);
    require(r.size(), oracle_nub_size(v));
    require(r.size(), usize(2));
    require_true(r[0] == a and r[1] == z1);      // first occurrence wins, as everywhere else
  }
  end_test_case();

  test_case("nub[const char*]: a char pointer is compared by address, so it is not hash-dedupable");
  {
    // micron::hash on a char pointer digests the POINTEE (it runs strlen over it) while operator==
    // on the element compares the ADDRESS. That costs no false negatives, but it dereferences a
    // pointer nub was never given -- a null, non-terminated or dangling element reads arbitrary
    // memory where the quadratic rescan only ever compared two words. So char pointers keep the
    // rescan; every other pointer type has no viable hash overload and never took the fast path.
    static_assert(!micron::fp::__impl::hash_dedupable<const char *>, "char pointers dedup by address");
    static_assert(!micron::fp::__impl::hash_dedupable<char *>, "char pointers dedup by address");
    static_assert(!micron::fp::__impl::hash_dedupable<int *>, "no hash overload, so never on the fast path");

    // two distinct pointers to equal content are two distinct elements to operator==, and a null
    // element must be survivable
    const char lhs[] = "xy";
    const char rhs[] = "xy";
    micron::fvector<const char *> v;
    v.push_back(nullptr);
    v.push_back(lhs);
    v.push_back(nullptr);
    v.push_back(rhs);
    v.push_back(lhs);
    auto r = micron::fp::nub(v);
    require(r.size(), oracle_nub_size(v));
    require(r.size(), usize(3));
    require_true(r[0] == nullptr and r[1] == lhs and r[2] == rhs);
  }
  end_test_case();

  test_case("nub[float]: NaN is exactly what operator== makes of it");
  {
    // IEEE says NaN == NaN is false, so a NaN is distinct from every value including its own bit
    // pattern and nub keeps every copy. duck's default -Ofast carries -ffinite-math-only, which
    // tells gcc a NaN cannot occur and lets it compare with a predicate that reads "unordered" as
    // equal -- measured, that makes one NaN swallow the rest of the vector. nub does not get an
    // opinion either way: it must answer whatever == answers in the build it was compiled in, and
    // the naive scan below -- same TU, same flags -- is what says so.
    const double qnan = __builtin_nan("");
    micron::fvector<double> v;
    v.push_back(1.0);
    v.push_back(qnan);
    v.push_back(2.0);
    v.push_back(qnan);
    v.push_back(1.0);

    micron::fvector<double> oracle;
    for ( usize i = 0; i < v.size(); ++i ) {
      bool seen = false;
      for ( usize j = 0; j < oracle.size(); ++j )
        if ( oracle[j] == v[i] ) {
          seen = true;
          break;
        }
      if ( !seen ) oracle.push_back(v[i]);
    }

    auto r = micron::fp::nub(v);
    require(r.size(), oracle.size());
    for ( usize i = 0; i < oracle.size(); ++i ) require_true(__builtin_bit_cast(u64, r[i]) == __builtin_bit_cast(u64, oracle[i]));
  }
  end_test_case();

  test_case("nub[user]: a hash that disagrees with == does not get to decide");
  {
    // Duo is a container as far as micron::hash is concerned -- hash64 takes cbegin() and size() --
    // but its operator== deliberately ignores the second byte. hash therefore separates values that
    // == equates, which is the exact shape of the float defect, so Duo must take the == rescan.
    struct Duo {
      using value_type = char;
      using pointer = char *;
      using iterator = char *;
      using const_iterator = const char *;
      char b[2];
      pointer
      data()
      {
        return b;
      }
      iterator
      begin()
      {
        return b;
      }
      iterator
      end()
      {
        return b + 2;
      }
      const_iterator
      cbegin() const
      {
        return b;
      }
      const_iterator
      cend() const
      {
        return b + 2;
      }
      usize
      size() const
      {
        return 2;
      }
      bool
      operator==(const Duo &o) const
      {
        return b[0] == o.b[0];
      }
    };
    static_assert(requires(const Duo &d) { micron::hash<micron::hash64_t>(d); }, "Duo is hashable -- the exclusion is the rule, not a gap");
    static_assert(!micron::fp::__impl::hash_dedupable<Duo>, "a hash that disagrees with == must not take the hash path");

    micron::fvector<Duo> v;
    v.push_back(Duo{ { 'a', 'x' } });
    v.push_back(Duo{ { 'a', 'y' } });      // == 'a','x', hashes differently
    v.push_back(Duo{ { 'b', 'x' } });
    v.push_back(Duo{ { 'a', 'z' } });
    auto r = micron::fp::nub(v);
    require(r.size(), usize(2));
    require_true(r[0].b[0] == 'a' && r[0].b[1] == 'x');      // first occurrence is the representative
    require_true(r[1].b[0] == 'b');

    // and the opt-in hands the O(n) path back to a type that promises the two agree
    struct Twin: Duo {
      using hash_equality_consistent = micron::true_type;
      bool
      operator==(const Twin &o) const
      {
        return b[0] == o.b[0] && b[1] == o.b[1];
      }
    };
    static_assert(micron::fp::__impl::hash_dedupable<Twin>, "the opt-in must reach the hash path");

    micron::fvector<Twin> t;
    t.push_back(Twin{ { { 'a', 'x' } } });
    t.push_back(Twin{ { { 'a', 'y' } } });
    t.push_back(Twin{ { { 'a', 'x' } } });
    auto rt = micron::fp::nub(t);
    require(rt.size(), usize(2));
    require_true(rt[0].b[1] == 'x');
    require_true(rt[1].b[1] == 'y');
  }
  end_test_case();

  sb::print("=== ALGO/FP-FILTER RIGOR SUITE PASSED ===");
  return 1;
}
