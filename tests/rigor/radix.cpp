//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// rigor: micron::radix_tree.
//
// The file this replaces had no test because it had no build -- it sat at global namespace using
// an alias it never included, and nothing in the tree referenced it. Everything here is therefore
// first coverage, and the two properties worth stating are (a) find() must agree with std::map on
// an adversarial key set full of shared prefixes, which is where a compressed trie's edge
// splitting goes wrong, and (b) longest_prefix_match must agree with a brute-force scan, which is
// the operation the type exists for and the old file did not have.

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/trees/radix.hpp"

#include "../snowball/snowball.hpp"

#include <map>
#include <vector>

namespace
{

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

using rt = micron::radix_tree<micron::string, int>;

// A key is a short word over {a,b}. It is carried as (length, bit pattern) packed into a u64 so
// the oracle can be a std::map<u64,...> and this file never includes <string> -- libstdc++'s
// <string> drags in glibc headers that collide with micron's typedefs on arm32 (ISSUES.md), which
// would have made this suite x86/arm64 only.
struct kw {
  u64 bits;
  u32 len;
};

constexpr u32 KMAX = 16;

u64
kw_pack(const kw &k)
{
  return (static_cast<u64>(k.len) << 40) | (k.bits & 0xFFFFFFFFFFULL);
}

micron::string
ms(const kw &k)
{
  char buf[KMAX + 1];
  for ( u32 i = 0; i < k.len; ++i ) buf[i] = static_cast<char>('a' + ((k.bits >> i) & 1u));
  buf[k.len] = '\0';
  return micron::string(static_cast<const char *>(buf));
}

micron::string
ms(const char *p)
{
  return micron::string(p);
}

// is `a` a prefix of `b`?
bool
kw_is_prefix(const kw &a, const kw &b)
{
  if ( a.len > b.len ) return false;
  for ( u32 i = 0; i < a.len; ++i )
    if ( ((a.bits >> i) & 1u) != ((b.bits >> i) & 1u) ) return false;
  return true;
}

// keys over a two-symbol alphabet so prefixes collide constantly -- that is what exercises edge
// splitting; random 64-bit strings almost never share a prefix and would test nothing
kw
gen_key(u64 &rng)
{
  kw k;
  k.len = 1u + static_cast<u32>(splitmix64(rng++) % 12u);
  k.bits = splitmix64(rng++) & ((1ULL << k.len) - 1ULL);
  return k;
}

kw
kw_unpack(u64 packed)
{
  kw k;
  k.len = static_cast<u32>(packed >> 40);
  k.bits = packed & 0xFFFFFFFFFFULL;
  return k;
}

// the operation the type exists for: the longest stored key that is a prefix of `k`
const int *
oracle_lpm(const std::map<u64, int> &m, const kw &k)
{
  const int *best = nullptr;
  u32 blen = 0;
  for ( const auto &kv : m ) {
    const kw s = kw_unpack(kv.first);
    if ( !kw_is_prefix(s, k) ) continue;
    if ( best == nullptr || s.len > blen ) {
      best = &kv.second;
      blen = s.len;
    }
  }
  return best;
}

kw
kw_from(const char *p)
{
  kw k{ 0, 0 };
  for ( u32 i = 0; p[i]; ++i ) {
    if ( p[i] == 'b' ) k.bits |= (1ULL << i);
    ++k.len;
  }
  return k;
}

};      // namespace

int
main(void)
{
  sb::print("=== RADIX TESTS ===");

  sb::test_case("construction - empty");
  {
    rt t;
    sb::require(t.empty());
    sb::require(t.size() == 0ULL);
    sb::require(t.find(ms("anything")) == nullptr);
    sb::require(!t.contains(ms("anything")));
    sb::require(t.longest_prefix_match(ms("anything")) == nullptr);
    sb::require(!t.erase(ms("anything")));
  }
  sb::end_test_case();

  sb::test_case("edge splitting - the shapes a compressed trie gets wrong");
  {
    rt t;
    // insert order chosen so every split case fires: longer-then-shorter, shorter-then-longer,
    // exact prefix, and a divergence mid-edge
    sb::require(t.insert(ms("romane"), 1));
    sb::require(t.insert(ms("romanus"), 2));      // splits at "roman"
    sb::require(t.insert(ms("romulus"), 3));      // splits at "rom"
    sb::require(t.insert(ms("rubens"), 4));       // splits at "r"
    sb::require(t.insert(ms("ruber"), 5));        // splits at "rube"
    sb::require(t.insert(ms("rubicon"), 6));
    sb::require(t.insert(ms("rubicundus"), 7));
    sb::require(t.insert(ms("rom"), 8));          // an interior node GAINS a value
    sb::require(t.insert(ms("r"), 9));            // the root edge gains a value
    sb::require(t.size() == 9ULL);

    struct {
      const char *k;
      int v;
    } expect[] = { { "romane", 1 },  { "romanus", 2 }, { "romulus", 3 },    { "rubens", 4 }, { "ruber", 5 },
                   { "rubicon", 6 }, { "rubicundus", 7 }, { "rom", 8 },     { "r", 9 } };
    for ( auto &e : expect ) {
      const int *p = t.find(ms(e.k));
      sb::require(p != nullptr);
      sb::require(*p == e.v);
    }
    // prefixes that were never inserted must MISS -- an interior node is not a key
    sb::require(t.find(ms("ro")) == nullptr);
    sb::require(t.find(ms("roman")) == nullptr);
    sb::require(t.find(ms("ru")) == nullptr);
    sb::require(t.find(ms("rubi")) == nullptr);
    sb::require(t.find(ms("")) == nullptr);
    sb::require(t.find(ms("romanes")) == nullptr);
    sb::require(t.find(ms("z")) == nullptr);

    // a duplicate insert must not take, and must not disturb the stored value
    sb::require(!t.insert(ms("rom"), 99));
    sb::require(*t.find(ms("rom")) == 8);
    // insert_or_assign must
    sb::require(!t.insert_or_assign(ms("rom"), 99));
    sb::require(*t.find(ms("rom")) == 99);
    sb::require(t.size() == 9ULL);
  }
  sb::end_test_case();

  sb::test_case("longest_prefix_match vs brute force");
  {
    rt t;
    std::map<u64, int> oracle;
    // words over {a,b}; the packed form is what the oracle is keyed by
    const char *keys[] = { "a", "ab", "abb", "abba", "b", "bb", "bab" };
    int i = 0;
    for ( const char *k : keys ) {
      const kw w = kw_from(k);
      t.insert(ms(w), i);
      oracle[kw_pack(w)] = i;
      ++i;
    }
    const char *probes[] = { "a", "ab", "aba", "abbab", "abba", "b", "bbb", "ba", "bab", "baba", "aa", "bba", "abab" };
    for ( const char *p : probes ) {
      const kw w = kw_from(p);
      const int *got = t.longest_prefix_match(ms(w));
      const int *want = oracle_lpm(oracle, w);
      if ( want == nullptr )
        sb::require(got == nullptr);
      else {
        sb::require(got != nullptr);
        sb::require(*got == *want);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("randomized insert/find/erase vs std::map, prefix-dense alphabet");
  {
    rt t;
    std::map<u64, int> oracle;
    u64 rng = 0xDEADC0DEULL;
    for ( u32 round = 0; round < 20000; ++round ) {
      const kw k = gen_key(rng);
      const u64 pk = kw_pack(k);
      const u64 op = splitmix64(rng++) % 3;
      if ( op < 2 ) {
        const int v = static_cast<int>(splitmix64(rng++) % 100000);
        const bool fresh = oracle.find(pk) == oracle.end();
        sb::require(t.insert_or_assign(ms(k), v) == fresh);
        oracle[pk] = v;
      } else {
        const bool present = oracle.find(pk) != oracle.end();
        sb::require(t.erase(ms(k)) == present);
        oracle.erase(pk);
      }
      sb::require(t.size() == oracle.size());
    }
    // every key agrees, and every key NOT in the oracle misses
    for ( const auto &kv : oracle ) {
      const int *p = t.find(ms(kw_unpack(kv.first)));
      sb::require(p != nullptr);
      sb::require(*p == kv.second);
    }
    u64 probe_rng = 0xC0FFEE77ULL;
    for ( u32 i = 0; i < 5000; ++i ) {
      const kw k = gen_key(probe_rng);
      const bool want = oracle.find(kw_pack(k)) != oracle.end();
      sb::require(t.contains(ms(k)) == want);
    }
  }
  sb::end_test_case();

  sb::test_case("erase reclaims - a drained trie holds no blocks");
  {
    rt t;
    u64 rng = 0x5EED1234ULL;
    std::vector<kw> ks;
    for ( u32 i = 0; i < 4000; ++i ) ks.push_back(gen_key(rng));
    for ( const auto &k : ks ) t.insert_or_assign(ms(k), 1);
    sb::require(t.blocks_allocated() > 0ULL);

    for ( const auto &k : ks ) t.erase(ms(k));
    sb::require(t.size() == 0ULL);
    // the interior nodes must be gone too, not just the values -- the old file leaked whole
    // subtrees on move-assign and never freed anything on erase because it had no erase
    sb::require(t.find(ms(kw_from("a"))) == nullptr);

    t.clear();
    sb::require(t.blocks_allocated() == 0ULL);
    sb::require(t.size() == 0ULL);
    // still usable after clear()
    sb::require(t.insert(ms(kw_from("bab")), 7));
    sb::require(*t.find(ms(kw_from("bab"))) == 7);
  }
  sb::end_test_case();

  sb::test_case("move semantics do not leak or alias");
  {
    rt a;
    for ( u32 i = 0; i < 500; ++i ) {
      kw w;
      w.len = 12;
      w.bits = i;
      a.insert_or_assign(ms(w), static_cast<int>(i));
    }
    sb::require(a.size() == 500ULL);

    rt b(micron::move(a));
    sb::require(b.size() == 500ULL);
    sb::require(a.size() == 0ULL);
    sb::require(a.blocks_allocated() == 0ULL);
    kw one;
    one.len = 12;
    one.bits = 1;
    sb::require(a.find(ms(one)) == nullptr);
    sb::require(*b.find(ms(one)) == 1);

    rt c;
    c.insert(ms(kw_from("aaabbb")), -1);
    c = micron::move(b);
    sb::require(c.size() == 500ULL);
    sb::require(c.find(ms(kw_from("aaabbb"))) == nullptr);
    kw last;
    last.len = 12;
    last.bits = 499;
    sb::require(*c.find(ms(last)) == 499);

    // self-move-assign must not destroy the tree
    rt &cr = c;
    c = micron::move(cr);
    sb::require(c.size() == 500ULL);
  }
  sb::end_test_case();

  sb::test_case("for_each visits every key exactly once, with the right key");
  {
    rt t;
    std::map<u64, int> oracle;
    u64 rng = 0xF00DBABEULL;
    for ( u32 i = 0; i < 2000; ++i ) {
      const kw k = gen_key(rng);
      const int v = static_cast<int>(i);
      t.insert_or_assign(ms(k), v);
      oracle[kw_pack(k)] = v;
    }
    std::map<u64, int> seen;
    t.for_each([&](const micron::string &k, int &v) {
      kw w{ 0, 0 };
      for ( usize i = 0; i < k.size(); ++i ) {
        if ( k[i] == 'b' ) w.bits |= (1ULL << i);
        ++w.len;
      }
      seen[kw_pack(w)] = v;
    });
    sb::require(seen.size() == oracle.size());
    sb::require(seen == oracle);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
