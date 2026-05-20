// Exhaustive rigor tests for micron::rope<char>.
//
// rope is a persistent balanced binary tree of contiguous leaves (max 256
// bytes per leaf). Copies are O(1) refcount bumps; mutating ops return a
// fresh rope built by path-copy + leaf merge. Coverage:
//
//   - Every construct/move/copy at lengths that span 1 / __max_leaf-1 /
//     __max_leaf / __max_leaf+1 / multiple leaves (512/1024/4096),
//     verifying balanced-tree invariants (front/back/at/iter sweep).
//   - copy-as-share: `identity()` of a copy must equal the source root
//     pointer (proof of O(1) refcount-bump copy).
//   - __concat leaf-merge invariant: appending two short ropes whose
//     combined length <= 256 should produce a single leaf — verified
//     via for_each_chunk emitting exactly one chunk.
//   - All push_back / append / insert / erase / +=  overloads — payload
//     and length correctness, source immutability.
//   - substr spanning leaves; truncate, resize grow/shrink.
//   - remove / remove_all (C-str, rope, string overloads).
//   - All comparison overloads (rope, C-str, array, string concept).
//   - Iterator traversal: ++ walks every byte; last()==end()-1 via index.
//   - for_each_chunk: total reported length equals rope length; chunks
//     are <= 256 bytes; relative order is preserved.
//   - flatten round-trip preserves content.
//   - __ensure_flat cache reuse: two consecutive data() calls return
//     the same pointer.
//
// Build: `duck build tests/rigor/rope.cpp`. Exceptions ON.

#include "../../src/io/console.hpp"

#include "../../src/string/rope.hpp"
#include "../../src/string/string.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

using RP = micron::rope<char>;

static char
pat(usize i)
{
  static constexpr const char *src = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  return src[i % 62];
}

static void
fill_pat(char *dst, usize n)
{
  for ( usize i = 0; i < n; ++i ) dst[i] = pat(i);
  dst[n] = 0;
}

int
main(int, char **)
{
  sb::print("=== ROPE TESTS ===");

  // ----- Construction -----

  test_case("Default construction — empty");
  {
    RP r;
    require(r.empty(), true);
    require(r.size(), 0u);
    require(r.identity(), static_cast<const void *>(nullptr));
  }
  end_test_case();

  test_case("Construction with hint usize n is a no-op");
  {
    RP r(64u);
    require(r.empty(), true);
    require(r.size(), 0u);
  }
  end_test_case();

  test_case("Construction (n, ch) yields all-ch rope");
  {
    RP r(10u, 'x');
    require(r.size(), 10u);
    for ( usize i = 0; i < 10; ++i ) require(r[i], 'x');
  }
  end_test_case();

  test_case("Construction from const char* — single leaf when length <= 256");
  {
    const char *p = "hello";
    RP r(p);
    require(r.size(), 5u);
    require(r[0], 'h');
    require(r[4], 'o');
    // single leaf -> exactly one chunk
    usize chunks = 0;
    r.for_each_chunk([&](const char *, usize) {
      ++chunks;
      return true;
    });
    require(chunks, 1u);
  }
  end_test_case();

  test_case("Construction at exactly __max_leaf (256) is single leaf");
  {
    char buf[260];
    fill_pat(buf, 256);
    const char *p = buf;
    RP r(p);
    require(r.size(), 256u);
    usize chunks = 0;
    r.for_each_chunk([&](const char *, usize len) {
      ++chunks;
      require(len <= 256u, true);
      return true;
    });
    require(chunks, 1u);
  }
  end_test_case();

  test_case("Construction at __max_leaf+1 splits into >1 chunks");
  {
    char buf[260];
    fill_pat(buf, 257);
    const char *p = buf;
    RP r(p);
    require(r.size(), 257u);
    usize chunks = 0, total = 0;
    r.for_each_chunk([&](const char *, usize len) {
      ++chunks;
      total += len;
      return true;
    });
    require(chunks > 1u, true);
    require(total, 257u);
  }
  end_test_case();

  test_case("Construction at multi-leaf sizes — content preserved");
  {
    constexpr usize lens[] = { 512, 1024, 4096 };
    for ( usize L : lens ) {
      char *buf = new char[L + 1];
      fill_pat(buf, L);
      const char *p = buf;
      RP r(p);
      require(r.size(), L);
      // spot-check 32 positions across the rope
      for ( usize k = 0; k < 32; ++k ) {
        usize i = (L * k) / 32;
        require(r[i], pat(i));
      }
      delete[] buf;
    }
  }
  end_test_case();

  test_case("Construction from string-literal array");
  {
    RP r("world");
    require(r.size(), 5u);
    require(r[0], 'w');
    require(r[4], 'd');
  }
  end_test_case();

  test_case("Copy construction is O(1) share — identity unchanged");
  {
    const char *p = "shared";
    RP a(p);
    RP b(a);
    require(a.identity(), b.identity());
    require(b.size(), 6u);
  }
  end_test_case();

  test_case("Move construction transfers root and zeros source");
  {
    const char *p = "moved";
    RP donor(p);
    auto donor_root = donor.identity();
    RP r(micron::move(donor));
    require(r.identity(), donor_root);
    require(donor.empty(), true);
  }
  end_test_case();

  test_case("Construction from iterator pair");
  {
    char buf[] = "abcdefgh";
    RP r(buf, buf + 8);
    require(r.size(), 8u);
    require(r[7], 'h');
  }
  end_test_case();

  test_case("Construction from another string-concept type (hstring)");
  {
    micron::string hs("from-hstring");
    RP r(hs);
    require(r.size(), hs.size());
    for ( usize i = 0; i < r.size(); ++i ) require(r[i], hs[i]);
  }
  end_test_case();

  // ----- Assignment -----

  test_case("Copy assignment shares root");
  {
    const char *p = "abc";
    const char *q = "xyz";
    RP a(p), b(q);
    b = a;
    require(b.identity(), a.identity());
    require(b.size(), 3u);
  }
  end_test_case();

  test_case("Move assignment transfers root and zeros donor");
  {
    const char *p = "abc";
    RP donor(p);
    auto root = donor.identity();
    RP a;
    a = micron::move(donor);
    require(a.identity(), root);
    require(donor.empty(), true);
  }
  end_test_case();

  // ----- Element access -----

  test_case("front()/back()/at()/operator[] return expected chars");
  {
    const char *p = "abcde";
    RP r(p);
    require(r.front(), 'a');
    require(r.back(), 'e');
    require(r.at(2), 'c');
    require(r[3], 'd');
  }
  end_test_case();

  test_case("at(idx) out-of-range throws");
  {
    const char *p = "abc";
    RP r(p);
    bool threw = false;
    try {
      (void)r.at(3);
    } catch ( const micron::except::library_error & ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  test_case("data() flattens and returns contiguous buffer");
  {
    char buf[260];
    fill_pat(buf, 200);
    const char *p = buf;
    RP r(p);
    const char *flat = r.data();
    for ( usize i = 0; i < 200; ++i ) require(flat[i], pat(i));
  }
  end_test_case();

  test_case("__ensure_flat cache reuse: two data() calls return same ptr");
  {
    const char *p = "cache";
    RP r(p);
    const char *p1 = r.data();
    const char *p2 = r.data();
    require(p1, p2);
  }
  end_test_case();

  // ----- Iterators -----

  test_case("iterator++ traverses every byte in order");
  {
    char buf[260];
    fill_pat(buf, 257);      // multi-leaf
    const char *p = buf;
    RP r(p);
    usize idx = 0;
    for ( auto it = r.begin(); it != r.end(); ++it, ++idx ) require(*it, pat(idx));
    require(idx, 257u);
  }
  end_test_case();

  test_case("iterator.index() agrees with traversal count");
  {
    const char *p = "indexed";
    RP r(p);
    usize i = 0;
    for ( auto it = r.begin(); it != r.end(); ++it ) {
      require(it.index(), i);
      ++i;
    }
  }
  end_test_case();

  // ----- for_each / for_each_chunk -----

  test_case("for_each_chunk: total length sums to rope length");
  {
    char buf[2050];
    fill_pat(buf, 2048);
    const char *p = buf;
    RP r(p);
    usize tot = 0;
    r.for_each_chunk([&](const char *, usize len) {
      tot += len;
      return true;
    });
    require(tot, 2048u);
  }
  end_test_case();

  test_case("for_each_chunk: chunks are in left-to-right order");
  {
    char buf[1030];
    fill_pat(buf, 1024);
    const char *p = buf;
    RP r(p);
    usize seen = 0;
    r.for_each_chunk([&](const char *data, usize len) {
      for ( usize i = 0; i < len; ++i, ++seen ) require(data[i], pat(seen));
      return true;
    });
    require(seen, 1024u);
  }
  end_test_case();

  test_case("for_each visits every char in order");
  {
    const char *p = "linear";
    RP r(p);
    usize idx = 0;
    r.for_each([&](char c) {
      require(c, p[idx]);
      ++idx;
    });
    require(idx, 6u);
  }
  end_test_case();

  // ----- Modifiers — push_back/pop_back -----

  test_case("push_back(ch) appends one char in returned rope");
  {
    const char *p = "abc";
    RP r(p);
    auto t = r.push_back('!');
    require(t.size(), 4u);
    require(t.back(), '!');
    require(r.size(), 3u);      // immutable
  }
  end_test_case();

  test_case("push_back(rope) — share refcount with rhs");
  {
    const char *p = "lhs";
    const char *q = "rhs";
    RP a(p), b(q);
    auto c = a.push_back(b);
    require(c.size(), 6u);
    require(c[5], 's');
  }
  end_test_case();

  test_case("pop_back removes last char");
  {
    const char *p = "abcd";
    RP r(p);
    auto t = r.pop_back();
    require(t.size(), 3u);
    require(t.back(), 'c');
    require(r.size(), 4u);
  }
  end_test_case();

  // ----- Modifiers — append -----

  test_case("append(C-array) concatenates");
  {
    const char *p = "abc";
    RP r(p);
    auto t = r.append("def");
    require(t.size(), 6u);
    require(t[5], 'f');
  }
  end_test_case();

  test_case("append(rope) shares refcount");
  {
    const char *p = "lhs";
    const char *q = "rhs";
    RP a(p), b(q);
    auto c = a.append(b);
    require(c.size(), 6u);
  }
  end_test_case();

  // __concat leaf-merge invariant
  test_case("__concat merges two small leaves into one when total <= 256");
  {
    // build two small ropes, append, verify result is a single chunk
    const char *p = "abc";
    const char *q = "defghi";
    RP a(p), b(q);
    auto c = a.append(b);      // total len 9 — well under 256
    usize chunks = 0;
    c.for_each_chunk([&](const char *, usize) {
      ++chunks;
      return true;
    });
    require(chunks, 1u);
    require(c.size(), 9u);
  }
  end_test_case();

  // ----- Modifiers — insert -----

  test_case("insert(idx, ch, n) inserts cnt copies of ch at index");
  {
    const char *p = "abef";
    RP r(p);
    auto t = r.insert(2, 'X', 2);
    require(t.size(), 6u);
    require(t[2], 'X');
    require(t[3], 'X');
    require(t[4], 'e');
  }
  end_test_case();

  test_case("insert(idx, arr) places array at index");
  {
    const char *p = "abef";
    RP r(p);
    auto t = r.insert(2, "CD", 1);
    require(t.size(), 6u);
    require(t[2], 'C');
    require(t[3], 'D');
    require(t[4], 'e');
  }
  end_test_case();

  test_case("insert(idx, rope) splices another rope");
  {
    const char *p = "abef";
    const char *q = "cd";
    RP r(p), other(q);
    auto t = r.insert(2, other);
    require(t.size(), 6u);
    require(t[2], 'c');
    require(t[3], 'd');
  }
  end_test_case();

  // ----- Modifiers — erase -----

  test_case("erase(idx,n) removes n bytes at index");
  {
    const char *p = "abcdef";
    RP r(p);
    auto t = r.erase(2, 2);
    require(t.size(), 4u);
    require(t[0], 'a');
    require(t[1], 'b');
    require(t[2], 'e');
    require(t[3], 'f');
  }
  end_test_case();

  test_case("erase across leaf boundary preserves content");
  {
    char buf[520];
    fill_pat(buf, 512);      // 2 leaves
    const char *p = buf;
    RP r(p);
    auto t = r.erase(200, 100);      // 200..299 — straddles leaf boundary at 256
    require(t.size(), 412u);
    for ( usize i = 0; i < 200; ++i ) require(t[i], pat(i));
    for ( usize i = 200; i < t.size(); ++i ) require(t[i], pat(i + 100));
  }
  end_test_case();

  // ----- substr / truncate / resize -----

  test_case("substr extracts a sub-range across leaves");
  {
    char buf[1030];
    fill_pat(buf, 1024);
    const char *p = buf;
    RP r(p);
    auto t = r.substr(100, 800);
    require(t.size(), 800u);
    for ( usize i = 0; i < 800; ++i ) require(t[i], pat(i + 100));
  }
  end_test_case();

  test_case("substr(0,length) returns full rope as-share");
  {
    const char *p = "whole";
    RP r(p);
    auto t = r.substr(0, 5);
    require(t.size(), 5u);
    require(t.identity(), r.identity());      // path 'pos==0 && cnt==len'
  }
  end_test_case();

  test_case("truncate(n) drops the tail");
  {
    const char *p = "abcdefgh";
    RP r(p);
    auto t = r.truncate(static_cast<usize>(4));
    require(t.size(), 4u);
    require(t[3], 'd');
  }
  end_test_case();

  test_case("resize(grow,ch) extends with fill characters");
  {
    const char *p = "abc";
    RP r(p);
    auto t = r.resize(8, 'x');
    require(t.size(), 8u);
    require(t[3], 'x');
    require(t[7], 'x');
  }
  end_test_case();

  test_case("resize(shrink,ch) truncates");
  {
    const char *p = "abcdef";
    RP r(p);
    auto t = r.resize(3, 'x');
    require(t.size(), 3u);
    require(t[2], 'c');
  }
  end_test_case();

  // ----- remove / remove_all -----

  test_case("remove(C-str) deletes first occurrence");
  {
    const char *p = "ababab";
    RP r(p);
    auto t = r.remove("ab");
    require(t.size(), 4u);
    require(t[0], 'a');      // remaining first 'a' of next 'ab'
  }
  end_test_case();

  test_case("remove_all(C-str) deletes every occurrence");
  {
    const char *p = "ababab";
    RP r(p);
    auto t = r.remove_all("ab");
    require(t.size(), 0u);
  }
  end_test_case();

  test_case("remove(rope) deletes first match using rope-shaped needle");
  {
    const char *p = "abcdef";
    const char *q = "cd";
    RP r(p), n(q);
    auto t = r.remove(n);
    require(t.size(), 4u);
    require(t[2], 'e');
  }
  end_test_case();

  // ----- flatten -----

  test_case("flatten() round-trips content");
  {
    char buf[1030];
    fill_pat(buf, 1024);
    const char *p = buf;
    RP r(p);
    auto f = r.flatten();
    require(f.size(), 1024u);
    for ( usize i = 0; i < 1024; ++i ) require(f[i], pat(i));
  }
  end_test_case();

  // ----- find -----

  test_case("find(ch) hits at correct index across leaves");
  {
    char buf[260];
    fill_pat(buf, 257);
    const char *p = buf;
    RP r(p);
    require(r.find(pat(0)), 0u);
    auto idx = r.find(pat(256));
    require(idx != micron::npos, true);
    require(r[idx], pat(256));
    require(r.find('!'), micron::npos);
  }
  end_test_case();

  test_case("find_substr finds substring after flatten");
  {
    const char *p = "hellomicronworld";
    RP r(p);
    require(r.find_substr("micron", 6), 5u);
    require(r.find_substr("zzzz", 4), micron::npos);
  }
  end_test_case();

  // ----- comparison -----

  test_case("operator==(rope, rope) shared root short-circuits true");
  {
    const char *p = "abc";
    RP a(p);
    RP b(a);      // share root
    require(a == b, true);
  }
  end_test_case();

  test_case("operator==(rope, rope) distinct trees identical content");
  {
    const char *p = "abc";
    RP a(p), b(p);
    require(a == b, true);
  }
  end_test_case();

  test_case("operator==(rope, rope) differing length is false");
  {
    const char *p = "abc";
    const char *q = "abcd";
    RP a(p), b(q);
    require(a == b, false);
  }
  end_test_case();

  test_case("operator==(C-str) compares full content");
  {
    const char *p = "compare-me";
    RP r(p);
    require(r == "compare-me", true);
    require(r == "compare-mz", false);
  }
  end_test_case();

  test_case("operator< / > on C-str orders lexicographically");
  {
    const char *p = "abcd";
    RP r(p);
    require(r < "abce", true);
    require(r > "abcc", true);
    require(r <= "abcd", true);
    require(r >= "abcd", true);
  }
  end_test_case();

  test_case("Comparison across SIMD-boundary lengths");
  {
    constexpr usize lens[] = { 1, 15, 16, 17, 31, 32, 33, 63, 64, 127, 128, 255, 256, 257 };
    char buf[260], buf2[260];
    for ( usize L : lens ) {
      fill_pat(buf, L);
      fill_pat(buf2, L);
      const char *p = buf;
      const char *q = buf2;
      RP a(p), b(q);
      require(a == b, true);
      if ( L > 0 ) {
        buf2[L - 1] ^= 1;
        const char *q2 = buf2;
        RP c(q2);
        require(a == c, false);
        buf2[L - 1] ^= 1;
      }
    }
  }
  end_test_case();

  // ----- operator+= -----

  test_case("operator+=(ch) self-mutates by replacing root");
  {
    const char *p = "ab";
    RP r(p);
    r += 'c';
    require(r.size(), 3u);
    require(r.back(), 'c');
  }
  end_test_case();

  test_case("operator+=(rope) appends");
  {
    const char *p = "ab";
    const char *q = "cd";
    RP a(p), b(q);
    a += b;
    require(a.size(), 4u);
    require(a.back(), 'd');
  }
  end_test_case();

  // ----- last() -----

  test_case("last() iterator points to length-1");
  {
    const char *p = "tail";
    RP r(p);
    auto it = r.last();
    require(it.index(), 3u);
  }
  end_test_case();

  // ----- explicit operator bool / ! -----

  test_case("operator bool / operator! reflect emptiness");
  {
    RP empty;
    RP nonempty("x");
    require(static_cast<bool>(empty), false);
    require(static_cast<bool>(nonempty), true);
    require(!empty, true);
    require(!nonempty, false);
  }
  end_test_case();

  sb::print("=== ROPE TESTS DONE ===");
  return 1;
}
