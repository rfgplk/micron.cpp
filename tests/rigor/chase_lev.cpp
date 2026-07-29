//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/queue/chase_lev.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include "../support/mt.hpp"

namespace
{

constexpr u64 SEED_A = 0xC0FFEE1234567ULL;
constexpr u64 SEED_B = 0xDEADBEEFCAFE01ULL;
constexpr u64 SEED_C = 0x51EDBEEF0BADULL;
constexpr u64 SEED_D = 0xA5A5A5A50F0FULL;

[[gnu::always_inline]] inline u64
xs64(u64 &s) noexcept
{
  s ^= s << 13;
  s ^= s >> 7;
  s ^= s << 17;
  return s;
}

struct lifo_oracle {
  u64 *v = nullptr;
  usize cap = 0;
  usize n = 0;

  explicit lifo_oracle(usize c) : v(new u64[c]), cap(c) { }

  ~lifo_oracle() { delete[] v; }

  lifo_oracle(const lifo_oracle &) = delete;
  lifo_oracle &operator=(const lifo_oracle &) = delete;

  bool
  push(u64 x) noexcept
  {
    if ( n == cap ) return false;
    v[n++] = x;
    return true;
  }

  u64
  pop() noexcept
  {
    return n == 0 ? 0ULL : v[--n];
  }

  usize
  size() const noexcept
  {
    return n;
  }

  bool
  empty() const noexcept
  {
    return n == 0;
  }
};

#if defined(__micron_arch_width_32)
constexpr int MAX_K = 3;
#else
constexpr int MAX_K = 7;
#endif
constexpr int MAX_PARTS = MAX_K + 1;

struct part {
  u64 *got = nullptr;
  usize n = 0;
  u64 lost = 0;
  u64 empty_ = 0;
  char pad[64 - ((sizeof(void *) + 3 * sizeof(u64)) % 64)];
};

u8 *g_count = nullptr;

bool
merge_and_check(part *ps, int nparts, usize n_values)
{
  for ( usize i = 0; i <= n_values; ++i ) g_count[i] = 0;
  usize total = 0;
  for ( int p = 0; p < nparts; ++p ) {
    total += ps[p].n;
    for ( usize i = 0; i < ps[p].n; ++i ) {
      const u64 v = ps[p].got[i];
      if ( v == 0 || v > n_values ) {
        sb::print("  !! recorded a value outside [1,N]: ", v);
        return false;
      }
      if ( g_count[v] != 0 ) {
        sb::print("  !! DOUBLE TAKE of value ", v);
        return false;
      }
      g_count[v] = 1;
    }
  }
  if ( total != n_values ) {
    sb::print("  !! total recorded ", total, " != N ", n_values);
    return false;
  }
  for ( usize i = 1; i <= n_values; ++i ) {
    if ( g_count[i] != 1 ) {
      sb::print("  !! value LOST: ", i);
      return false;
    }
  }
  return true;
}

}      // namespace

static_assert(alignof(micron::chase_lev<u64, 1024>) >= micron::cache_line_size(), "chase_lev must be cache-line aligned");
static_assert(alignof(micron::chase_lev_grow<u64, 1024>) >= micron::cache_line_size(), "chase_lev_grow must be cache-line aligned");

static_assert(sizeof(micron::chase_lev<u64, 1024>) == 2 * micron::cache_line_size(), "chase_lev must be exactly two cache lines");
static_assert(sizeof(micron::chase_lev_grow<u64, 1024>) == 2 * micron::cache_line_size(), "chase_lev_grow must be exactly two cache lines");
static_assert(micron::chase_lev<u64, 1024>::capacity() == 1024ULL, "capacity is next_pow2(N)");
static_assert(micron::chase_lev<u64, 1000>::capacity() == 1024ULL, "capacity rounds up to a power of two");
static_assert(micron::chase_lev<u64, 5>::capacity() == 8ULL, "capacity rounds up to a power of two");

int
main(void)
{
  sb::print("=== CHASE_LEV TESTS ===");

  sb::test_case("T1 single-thread LIFO, fixed");
  {
    micron::chase_lev<u64, 1024> d;
    sb::require(d.empty());
    sb::require(d.size() == 0ULL);
    for ( u64 i = 1; i <= 500; ++i ) sb::require(d.push_bottom(i));
    sb::require(!d.empty());
    sb::require(d.size() == 500ULL);
    for ( u64 i = 500; i >= 1; --i ) sb::require(d.pop_bottom() == i);
    sb::require(d.empty());
    sb::require(d.pop_bottom() == 0ULL);
    sb::require(d.empty());
    sb::require(d.pop_bottom() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("T1 single-thread LIFO, growable");
  {
    micron::chase_lev_grow<u64, 1024> d;
    for ( u64 i = 1; i <= 500; ++i ) sb::require(d.push_bottom(i));
    sb::require(d.size() == 500ULL);
    for ( u64 i = 500; i >= 1; --i ) sb::require(d.pop_bottom() == i);
    sb::require(d.empty());
    sb::require(d.pop_bottom() == 0ULL);
    sb::require(d.pop_bottom() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("T2 full / empty boundaries (fixed, cap 8)");
  {
    micron::chase_lev<u64, 8> d;
    sb::require(d.capacity() == 8ULL);
    for ( u64 i = 1; i <= 8; ++i ) {
      sb::require(d.push_bottom(i));
      sb::require(d.size() == i);
    }
    sb::require(!d.push_bottom(9));
    sb::require(d.size() == 8ULL);
    sb::require(!d.push_bottom(9));
    sb::require(d.pop_bottom() == 8ULL);
    sb::require(d.push_bottom(9));
    sb::require(d.pop_bottom() == 9ULL);
    for ( u64 i = 7; i >= 1; --i ) sb::require(d.pop_bottom() == i);
    sb::require(d.empty());
  }
  sb::end_test_case();

  sb::test_case("T2 growable never reports full");
  {
    micron::chase_lev_grow<u64, 4> d;
    for ( u64 i = 1; i <= 5000; ++i ) sb::require(d.push_bottom(i));
    sb::require(d.size() == 5000ULL);
    sb::require(d.capacity() >= 5000ULL);
    for ( u64 i = 5000; i >= 1; --i ) sb::require(d.pop_bottom() == i);
    sb::require(d.empty());
  }
  sb::end_test_case();

  sb::test_case("T3 wraparound, cap 8, 200k ops");
  {
    micron::chase_lev<u64, 8> d;
    for ( u64 r = 1; r <= 200000; ++r ) {
      sb::require(d.push_bottom(r));
      sb::require(d.pop_bottom() == r);
    }
    sb::require(d.empty());
  }
  sb::end_test_case();

  sb::test_case("T3 wraparound, ring kept half full");
  {
    micron::chase_lev<u64, 8> d;
    for ( u64 i = 1; i <= 4; ++i ) sb::require(d.push_bottom(i));

    for ( u64 r = 5; r <= 100000; r += 2 ) {
      sb::require(d.push_bottom(r));
      sb::require(d.push_bottom(r + 1));
      sb::require(d.pop_bottom() == r + 1);
      sb::require(d.pop_bottom() == r);
    }
    sb::require(d.size() == 4ULL);
    for ( u64 i = 4; i >= 1; --i ) sb::require(d.pop_bottom() == i);
  }
  sb::end_test_case();

  sb::test_case("T4 randomized fuzz vs LIFO oracle, fixed cap 8 (max wrap pressure)");
  {
    micron::chase_lev<u64, 8> d;
    lifo_oracle o(8);
    u64 s = SEED_A;
    u64 tag = 0;
    for ( u32 step = 0; step < 400000; ++step ) {
      if ( (xs64(s) & 0xFFu) < 140u ) {
        const u64 v = ++tag;
        sb::require(d.push_bottom(v) == o.push(v));
      } else {
        sb::require(d.pop_bottom() == o.pop());
      }
      sb::require(d.size() == o.size());
      sb::require(d.empty() == o.empty());
    }
  }
  sb::end_test_case();

  sb::test_case("T4 randomized fuzz vs LIFO oracle, growable init 4 (max grow pressure)");
  {
    micron::chase_lev_grow<u64, 4> d;
    lifo_oracle o(400001);
    u64 s = SEED_B;
    u64 tag = 0;

    for ( u32 step = 0; step < 400000; ++step ) {
      if ( (xs64(s) & 0xFFu) < 165u ) {
        const u64 v = ++tag;
        sb::require(d.push_bottom(v));
        sb::require(o.push(v));
      } else {
        sb::require(d.pop_bottom() == o.pop());
      }
      sb::require(d.size() == o.size());
      sb::require(d.empty() == o.empty());
    }
    sb::print("  grew to capacity ", d.capacity(), ", peak depth ", o.size());
    sb::require(d.capacity() > 4ULL);
  }
  sb::end_test_case();

  sb::test_case("T4 randomized fuzz vs LIFO oracle, growable init 1024 (production shape)");
  {
    micron::chase_lev_grow<u64, 1024> d;
    lifo_oracle o(400001);
    u64 s = SEED_C;
    u64 tag = 0;
    for ( u32 step = 0; step < 400000; ++step ) {
      if ( (xs64(s) & 0xFFu) < 140u ) {
        const u64 v = ++tag;
        sb::require(d.push_bottom(v));
        sb::require(o.push(v));
      } else {
        sb::require(d.pop_bottom() == o.pop());
      }
      sb::require(d.size() == o.size());
    }
  }
  sb::end_test_case();

  sb::test_case("T7 API landmines");
  {
    micron::chase_lev<u64, 8> d;
    sb::require(d.push_bottom(0ULL));
    sb::require(d.size() == 1ULL);
    sb::require(d.pop_bottom() == 0ULL);
    sb::require(d.empty());

    micron::steal_status st = micron::steal_status::got;
    sb::require(d.steal_top(st) == 0ULL);
    sb::require(st == micron::steal_status::empty);

    sb::require(d.push_bottom(42ULL));
    st = micron::steal_status::empty;
    sb::require(d.steal_top(st) == 42ULL);
    sb::require(st == micron::steal_status::got);
    sb::require(d.empty());
  }
  sb::end_test_case();

  sb::test_case("T8 steal drains top-first (FIFO from the thief end) while owner is quiescent");
  {
    micron::chase_lev<u64, 1024> d;
    for ( u64 i = 1; i <= 64; ++i ) sb::require(d.push_bottom(i));

    for ( u64 i = 1; i <= 32; ++i ) {
      micron::steal_status st = micron::steal_status::empty;
      sb::require(d.steal_top(st) == i);
      sb::require(st == micron::steal_status::got);
    }
    for ( u64 i = 64; i >= 33; --i ) sb::require(d.pop_bottom() == i);
    sb::require(d.empty());
    micron::steal_status st = micron::steal_status::got;
    sb::require(d.steal_top(st) == 0ULL);
    sb::require(st == micron::steal_status::empty);
  }
  sb::end_test_case();

  {
    constexpr usize N = 1u << 16;
    g_count = new u8[N + 1];
    part *ps = new part[MAX_PARTS];
    for ( int i = 0; i < MAX_PARTS; ++i ) ps[i].got = new u64[N + 1];

    const int ks[3] = { 1, 3, MAX_K };
    const int nks = (MAX_K == 3) ? 2 : 3;
    for ( int ki = 0; ki < nks; ++ki ) {
      const int K = ks[ki];

      {
        sb::test_case("T5 exact partition, FIXED cap 1024, 1 owner + K thieves");
        micron::chase_lev<u64, 1024> d;
        micron::atomic_token<u32> done{ 0 };
        for ( int i = 0; i < MAX_PARTS; ++i ) {
          ps[i].n = 0;
          ps[i].lost = 0;
          ps[i].empty_ = 0;
        }

        mtest::parallel(K + 1, [&](int tid) {
          part &me = ps[tid];
          if ( tid == 0 ) {
            for ( u64 i = 1; i <= N; ) {
              if ( d.push_bottom(i) ) {
                ++i;
              } else {
                const u64 v = d.pop_bottom();
                if ( v != 0 ) me.got[me.n++] = v;
              }
            }
            for ( ;; ) {
              const u64 v = d.pop_bottom();
              if ( v == 0 ) break;
              me.got[me.n++] = v;
            }
            done.store(1, micron::memory_order_release);
          } else {
            u32 quiet = 0;
            for ( ;; ) {
              micron::steal_status st = micron::steal_status::empty;
              const u64 v = d.steal_top(st);
              if ( st == micron::steal_status::got ) {
                me.got[me.n++] = v;
                quiet = 0;
              } else {
                if ( st == micron::steal_status::lost )
                  ++me.lost;
                else
                  ++me.empty_;
                if ( done.get(micron::memory_order_acquire) != 0 && ++quiet > 2 ) break;
              }
            }
          }
        });
        sb::require(merge_and_check(ps, K + 1, N));
        sb::require(d.empty());
        sb::end_test_case();
      }

      {
        sb::test_case("T5 exact partition, GROWABLE init 1024, 1 owner + K thieves");
        micron::chase_lev_grow<u64, 1024> d;
        micron::atomic_token<u32> done{ 0 };
        for ( int i = 0; i < MAX_PARTS; ++i ) {
          ps[i].n = 0;
          ps[i].lost = 0;
          ps[i].empty_ = 0;
        }

        mtest::parallel(K + 1, [&](int tid) {
          part &me = ps[tid];
          if ( tid == 0 ) {
            for ( u64 i = 1; i <= N; ++i ) sb::require(d.push_bottom(i));
            for ( ;; ) {
              const u64 v = d.pop_bottom();
              if ( v == 0 ) break;
              me.got[me.n++] = v;
            }
            done.store(1, micron::memory_order_release);
          } else {
            u32 quiet = 0;
            for ( ;; ) {
              micron::steal_status st = micron::steal_status::empty;
              const u64 v = d.steal_top(st);
              if ( st == micron::steal_status::got ) {
                me.got[me.n++] = v;
                quiet = 0;
              } else {
                if ( st == micron::steal_status::lost )
                  ++me.lost;
                else
                  ++me.empty_;
                if ( done.get(micron::memory_order_acquire) != 0 && ++quiet > 2 ) break;
              }
            }
          }
        });
        u64 stolen = 0, lost = 0;
        for ( int i = 1; i <= K; ++i ) {
          stolen += ps[i].n;
          lost += ps[i].lost;
        }
        sb::print("  K=", static_cast<u64>(K), " owner=", static_cast<u64>(ps[0].n), " stolen=", stolen, " lost-CAS=", lost);
        sb::require(merge_and_check(ps, K + 1, N));
        sb::require(d.empty());
        sb::end_test_case();
      }
    }

    {
      sb::test_case("T5b adversarial depth-1 race, 1 owner + MAX_K thieves");
      constexpr usize M = 1u << 15;
      micron::chase_lev<u64, 1024> d;
      micron::atomic_token<u32> done{ 0 };
      constexpr int K = MAX_K;
      for ( int i = 0; i < MAX_PARTS; ++i ) {
        ps[i].n = 0;
        ps[i].lost = 0;
        ps[i].empty_ = 0;
      }

      mtest::parallel(K + 1, [&](int tid) {
        part &me = ps[tid];
        if ( tid == 0 ) {
          for ( u64 i = 1; i <= M; ++i ) {
            while ( !d.push_bottom(i) ) {
            }
            const u64 v = d.pop_bottom();
            if ( v != 0 ) me.got[me.n++] = v;
          }
          for ( ;; ) {
            const u64 v = d.pop_bottom();
            if ( v == 0 ) break;
            me.got[me.n++] = v;
          }
          done.store(1, micron::memory_order_release);
        } else {
          u32 quiet = 0;
          for ( ;; ) {
            micron::steal_status st = micron::steal_status::empty;
            const u64 v = d.steal_top(st);
            if ( st == micron::steal_status::got ) {
              me.got[me.n++] = v;
              quiet = 0;
            } else {
              if ( st == micron::steal_status::lost )
                ++me.lost;
              else
                ++me.empty_;
              if ( done.get(micron::memory_order_acquire) != 0 && ++quiet > 2 ) break;
            }
          }
        }
      });

      u64 stolen = 0, lost = 0;
      for ( int i = 1; i <= K; ++i ) {
        stolen += ps[i].n;
        lost += ps[i].lost;
      }
      sb::print("  owner=", static_cast<u64>(ps[0].n), " stolen=", stolen, " lost-CAS=", lost);
      sb::require(merge_and_check(ps, K + 1, M));
      sb::require(d.empty());

      sb::require(lost > 0);
      sb::end_test_case();
    }

    for ( int i = 0; i < MAX_PARTS; ++i ) delete[] ps[i].got;
    delete[] ps;
    delete[] g_count;
    g_count = nullptr;
  }

  sb::test_case("T6 grow under concurrent steal, growable init 4 + 3 thieves");
  {
    constexpr usize N = 1u << 18;
    {
      micron::chase_lev_grow<u64, 4> d;
      constexpr int K = 3;
      micron::atomic_token<u32> done{ 0 };
      u8 *count = new u8[N + 1];
      for ( usize i = 0; i <= N; ++i ) count[i] = 0;
      part *ps = new part[K + 1];
      for ( int i = 0; i <= K; ++i ) {
        ps[i].got = new u64[N + 1];
        ps[i].n = 0;
        ps[i].lost = 0;
      }

      mtest::parallel(K + 1, [&](int tid) {
        part &me = ps[tid];
        if ( tid == 0 ) {
          for ( u64 i = 1; i <= N; ++i ) sb::require(d.push_bottom(i));
          for ( ;; ) {
            const u64 v = d.pop_bottom();
            if ( v == 0 ) break;
            me.got[me.n++] = v;
          }
          done.store(1, micron::memory_order_release);
        } else {
          u32 quiet = 0;
          for ( ;; ) {
            micron::steal_status st = micron::steal_status::empty;
            const u64 v = d.steal_top(st);
            if ( st == micron::steal_status::got ) {
              me.got[me.n++] = v;
              quiet = 0;
            } else if ( done.get(micron::memory_order_acquire) != 0 && ++quiet > 2 )
              break;
          }
        }
      });

      usize total = 0;
      bool ok = true;
      for ( int p = 0; p <= K; ++p ) {
        total += ps[p].n;
        for ( usize i = 0; i < ps[p].n; ++i ) {
          const u64 v = ps[p].got[i];
          if ( v == 0 || v > N || count[v] != 0 ) {
            ok = false;
            break;
          }
          count[v] = 1;
        }
      }
      for ( usize i = 1; i <= N && ok; ++i )
        if ( count[i] != 1 ) ok = false;
      sb::print("  capacity after grow = ", d.capacity(), ", recorded ", static_cast<u64>(total));
      sb::require(ok);
      sb::require(total == N);
      sb::require(d.capacity() >= 4096ULL);
      sb::require(d.empty());

      for ( int i = 0; i <= K; ++i ) delete[] ps[i].got;
      delete[] ps;
      delete[] count;
    }
  }
  sb::end_test_case();

  sb::test_case("T6b repeated grow/destroy reaches a memory steady state");
  {
    constexpr usize N = 1u << 16;
    constexpr usize ROUNDS = 6;

    usize usage[ROUNDS] = {};
    for ( usize r = 0; r < ROUNDS; ++r ) {
      {
        micron::chase_lev_grow<u64, 4> d;
        for ( u64 i = 1; i <= N; ++i ) sb::require(d.push_bottom(i));
        sb::require(d.capacity() >= N);
        for ( u64 i = N; i >= 1; --i ) sb::require(d.pop_bottom() == i);
      }
      usage[r] = abc::musage();
    }
    sb::print("  musage after round 2 = ", usage[1], ", after round ", static_cast<u64>(ROUNDS), " = ", usage[ROUNDS - 1]);

    const usize drift = usage[ROUNDS - 1] > usage[1] ? usage[ROUNDS - 1] - usage[1] : 0ULL;
    sb::print("  drift over ", static_cast<u64>(ROUNDS - 2), " warm rounds = ", drift, " bytes");
    sb::require(drift < (1ULL << 20));
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
