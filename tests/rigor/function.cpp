

#include "../../src/function.hpp"

#include "../snowball/snowball.hpp"

#include "../support/oracles.hpp"
#include "../support/tracked_types.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

template<usize N> struct sized_fn {
  static_assert(N >= 4, "payload must carry the seed");
  unsigned char pad[N];

  explicit sized_fn(int b) noexcept
  {
    for ( usize i = 0; i < N; ++i ) pad[i] = static_cast<unsigned char>((b + static_cast<int>(i) * 7) & 0xff);
  }

  int
  operator()(int x) const noexcept
  {
    int acc = x;
    for ( usize i = 0; i < N; ++i ) acc += static_cast<int>(pad[i]);
    return acc;
  }
};

static int
sized_oracle(usize n, int b, int x) noexcept
{
  int acc = x;
  for ( usize i = 0; i < n; ++i ) acc += static_cast<int>(static_cast<unsigned char>((b + static_cast<int>(i) * 7) & 0xff));
  return acc;
}

template<usize A> struct aligned_fn {
  alignas(A) unsigned char pad[A];

  explicit aligned_fn(int b) noexcept
  {
    for ( usize i = 0; i < A; ++i ) pad[i] = static_cast<unsigned char>((b + static_cast<int>(i) * 3) & 0xff);
  }

  int
  operator()(int x) const noexcept
  {
    int acc = x;
    for ( usize i = 0; i < A; ++i ) acc += static_cast<int>(pad[i]);
    return acc;
  }
};

static int
aligned_oracle(usize a, int b, int x) noexcept
{
  int acc = x;
  for ( usize i = 0; i < a; ++i ) acc += static_cast<int>(static_cast<unsigned char>((b + static_cast<int>(i) * 3) & 0xff));
  return acc;
}

template<usize N, int Tag> struct tracked_fn {
  mtest::Tracked<Tag> t;
  unsigned char pad[N];

  explicit tracked_fn(int v) : t(v)
  {
    for ( usize i = 0; i < N; ++i ) pad[i] = static_cast<unsigned char>(i & 0xff);
  }

  int
  operator()(int x) const noexcept
  {
    return t.v + x;
  }
};

template<usize A, int Tag> struct tracked_aligned_fn {
  alignas(A) unsigned char pad[A];
  mtest::Tracked<Tag> t;

  explicit tracked_aligned_fn(int v) : t(v)
  {
    for ( usize i = 0; i < A; ++i ) pad[i] = static_cast<unsigned char>(i & 0xff);
  }

  int
  operator()(int x) const noexcept
  {
    return t.v + x;
  }
};

struct counter_fn {
  int acc = 0;

  int
  operator()(int x) noexcept
  {
    acc += x;
    return acc;
  }
};

struct big_counter_fn {
  int acc = 0;
  unsigned char pad[256];

  big_counter_fn() noexcept
  {
    for ( usize i = 0; i < 256; ++i ) pad[i] = static_cast<unsigned char>(i & 0xff);
  }

  int
  operator()(int x) noexcept
  {
    acc += x + static_cast<int>(pad[0]);
    return acc;
  }
};

struct move_only_fn {
  int v;

  explicit move_only_fn(int x) noexcept : v(x) { }

  move_only_fn(const move_only_fn &) = delete;
  move_only_fn &operator=(const move_only_fn &) = delete;

  move_only_fn(move_only_fn &&o) noexcept : v(o.v) { o.v = 0; }

  int
  operator()(int x) const noexcept
  {
    return v + x;
  }
};

struct big_move_only_fn {
  int v;
  unsigned char pad[256];

  explicit big_move_only_fn(int x) noexcept : v(x)
  {
    for ( usize i = 0; i < 256; ++i ) pad[i] = static_cast<unsigned char>((x + static_cast<int>(i)) & 0xff);
  }

  big_move_only_fn(const big_move_only_fn &) = delete;
  big_move_only_fn &operator=(const big_move_only_fn &) = delete;

  big_move_only_fn(big_move_only_fn &&o) noexcept : v(o.v)
  {
    for ( usize i = 0; i < 256; ++i ) pad[i] = o.pad[i];
    o.v = 0;
  }

  int
  operator()(int x) const noexcept
  {
    return v + x + static_cast<int>(pad[1]);
  }
};

struct member_target {
  int base = 100;

  int
  add(int x)
  {
    return base + x;
  }

  int
  cadd(int x) const
  {
    return base + x + 1;
  }
};

template<usize N>
static void
size_case(int seed)
{
  const int b = seed;
  const int want1 = sized_oracle(N, b, 1);
  const int want7 = sized_oracle(N, b, 7);

  mc::function<int(int)> f{ sized_fn<N>(b) };
  require(f(1), want1);
  require(f(7), want7);

  mc::function<int(int)> c(f);
  require(c(1), want1);
  require(f(1), want1);

  mc::function<int(int)> d{ sized_fn<7>(b ^ 0x5a) };
  d = c;
  require(d(7), want7);

  mc::function<int(int)> m(mc::move(c));
  require(m(1), want1);
  require_false(static_cast<bool>(c));

  mc::function<int(int)> e;
  e = mc::move(m);
  require(e(7), want7);
  require_false(static_cast<bool>(m));

  mc::function<int(int)> z;
  e.swap(z);
  require_false(static_cast<bool>(e));
  require(z(1), want1);
  mc::swap(e, z);
  require(e(1), want1);

  e = sized_fn<N>(b + 3);
  require(e(1), sized_oracle(N, b + 3, 1));
  e = nullptr;
  require_true(e == nullptr);
}

template<usize A>
static void
align_case(int seed)
{
  const int want = aligned_oracle(A, seed, 5);

  mc::function<int(int)> f{ aligned_fn<A>(seed) };
  require(f(5), want);

  aligned_fn<A> *p = f.template target<aligned_fn<A>>();
  require_true(p != nullptr);
  require(reinterpret_cast<uintptr_t>(p) % A, static_cast<uintptr_t>(0));

  mc::function<int(int)> c(f);
  require(c(5), want);
  aligned_fn<A> *q = c.template target<aligned_fn<A>>();
  require_true(q != nullptr);
  require_true(q != p);
  require(reinterpret_cast<uintptr_t>(q) % A, static_cast<uintptr_t>(0));

  mc::function<int(int)> m(mc::move(c));
  require(m(5), want);
  require(reinterpret_cast<uintptr_t>(m.template target<aligned_fn<A>>()) % A, static_cast<uintptr_t>(0));

  mc::function<int(int)> a2;
  a2 = m;
  require(a2(5), want);
  a2 = mc::move(m);
  require(a2(5), want);
}

int
main()
{
  sb::print("=== FUNCTION.HPP RIGOR SUITE ===");

  test_case("layout: SBO is 48 bytes at alignment 16");
  {
    require(mc::__function_smallobj_size, static_cast<usize>(48));
    require(mc::__function_smallobj_align, static_cast<usize>(16));

    if constexpr ( sizeof(void *) == 8 ) require(sizeof(mc::function<int(int)>), static_cast<usize>(64));
    require_true(alignof(mc::function<int(int)>) >= 16);
  }
  end_test_case();

  test_case("size ladder 8..1024 through copy/move/assign/swap/reassign");
  {
    size_case<8>(0x11);
    size_case<40>(0x22);
    size_case<48>(0x33);
    size_case<49>(0x44);
    size_case<64>(0x55);
    size_case<192>(0x66);
    size_case<193>(0x77);
    size_case<256>(0x88);
    size_case<1024>(0x99);
  }
  end_test_case();

  test_case("alignment ladder 8..128 (32 is abcmalloc's __hdr_offset)");
  {
    align_case<8>(0x1a);
    align_case<16>(0x2b);
    align_case<32>(0x3c);
    align_case<64>(0x4d);
    align_case<128>(0x5e);
  }
  end_test_case();

  test_case("no leaks: every ctor/copy/move is matched by a dtor");
  {

    mtest::Tracked<1>::reset();
    {
      mc::function<int(int)> f{ tracked_fn<8, 1>(11) };
      mc::function<int(int)> c(f);
      mc::function<int(int)> m(mc::move(c));
      mc::function<int(int)> a;
      a = f;
      a = mc::move(m);
      require(f(1), 12);
      require(a(1), 12);
      require_true(mtest::Tracked<1>::live() > 0);
    }
    require(mtest::Tracked<1>::live(), static_cast<usize>(0));

    mtest::Tracked<2>::reset();
    {
      mc::function<int(int)> f{ tracked_fn<252, 2>(21) };
      mc::function<int(int)> c(f);
      mc::function<int(int)> m(mc::move(c));
      mc::function<int(int)> a;
      a = f;
      a = mc::move(m);
      require(f(2), 23);
      require(a(2), 23);
      mc::function<int(int)> s;
      s.swap(a);
      require(s(2), 23);
    }
    require(mtest::Tracked<2>::live(), static_cast<usize>(0));

    mtest::Tracked<3>::reset();
    {
      mc::function<int(int)> f{ tracked_aligned_fn<64, 3>(31) };
      mc::function<int(int)> c(f);
      mc::function<int(int)> a;
      a = mc::move(c);
      require(f(3), 34);
      require(a(3), 34);
      require(reinterpret_cast<uintptr_t>(f.template target<tracked_aligned_fn<64, 3>>()) % 64, static_cast<uintptr_t>(0));
    }
    require(mtest::Tracked<3>::live(), static_cast<usize>(0));

    mtest::Tracked<4>::reset();
    {
      mc::function<int(int)> f;
      for ( int i = 0; i < 64; ++i ) {
        if ( i & 1 )
          f = tracked_fn<252, 4>(i);
        else
          f = tracked_fn<8, 4>(i);
        require(f(0), i);
      }
    }
    require(mtest::Tracked<4>::live(), static_cast<usize>(0));
  }
  end_test_case();

  test_case("mutable lambdas and stateful functors are storable and mutate");
  {

    int captured = 0;
    mc::function<int(int)> f = [captured](int x) mutable -> int {
      captured += x;
      return captured;
    };
    require(f(1), 1);
    require(f(2), 3);
    require(f(3), 6);

    const mc::function<int(int)> &cf = f;
    require(cf(4), 10);
    require(f(0), 10);

    mc::function<int(int)> g{ counter_fn{} };
    require(g(5), 5);
    require(g(5), 10);
    counter_fn *cp = g.template target<counter_fn>();
    require_true(cp != nullptr);
    require(cp->acc, 10);

    mc::function<int(int)> h{ big_counter_fn{} };
    require(h(1), 1);
    require(h(1), 2);

    mc::function<int(int)> hc(h);
    require(hc(1), 3);
    require(h(1), 3);
    require(hc(1), 4);
  }
  end_test_case();

  test_case("move-only targets are storable");
  {
    static_assert(!mc::is_copy_constructible_v<move_only_fn>, "move_only_fn must be move-only");
    static_assert(!mc::is_copy_constructible_v<big_move_only_fn>, "big_move_only_fn must be move-only");

    mc::function<int(int)> f{ move_only_fn(40) };
    require(f(2), 42);

    mc::function<int(int)> m(mc::move(f));
    require(m(2), 42);
    require_false(static_cast<bool>(f));

    mc::function<int(int)> a;
    a = mc::move(m);
    require(a(2), 42);

    mc::function<int(int)> big{ big_move_only_fn(10) };
    const int want = 10 + 2 + 11;
    require(big(2), want);
    mc::function<int(int)> bm(mc::move(big));
    require(bm(2), want);
    bm.swap(a);
    require(a(2), want);
    require(bm(2), 42);
  }
  end_test_case();

  test_case("wrapper mechanics: empty, nullptr, self-assign, target, is_noexcept, bind_method");
  {
    mc::function<int(int)> e;
    require_false(static_cast<bool>(e));
    require_true(!e);
    require_true(e == nullptr);
    require_true(nullptr == e);
    require_false(e != nullptr);

    mc::function<int(int)> f = [](int x) { return x + 1; };
    require_true(static_cast<bool>(f));
    require_false(!f);
    require_true(f != nullptr);
    require_false(f == nullptr);

    f = nullptr;
    require_true(f == nullptr);

    f = sized_fn<256>(0x5a);
    const int want = sized_oracle(256, 0x5a, 3);
    f = f;
    require(f(3), want);
    mc::function<int(int)> &fr = f;
    f = mc::move(fr);
    require(f(3), want);
    f.swap(f);
    require(f(3), want);

    mc::function<int(int)> m1(mc::move(f));
    mc::function<int(int)> m2(mc::move(f));
    require(m1(3), want);
    require_false(static_cast<bool>(m2));

    mc::function<int(int)> t{ sized_fn<8>(1) };
    require_true(t.template target<sized_fn<8>>() != nullptr);
    require_true(t.template target<sized_fn<9>>() == nullptr);
    require_true(t.template target<counter_fn>() == nullptr);
    const mc::function<int(int)> &ct = t;
    require_true(ct.template target<sized_fn<8>>() != nullptr);
    require_true(ct.template target<sized_fn<9>>() == nullptr);
    require_true(e.template target<sized_fn<8>>() == nullptr);

    mc::function<int()> ne = []() noexcept { return 1; };
    mc::function<int()> ex = []() { return 2; };
    require_true(ne.is_noexcept());
    require_false(ex.is_noexcept());
    require_false(mc::function<int()>{}.is_noexcept());

    member_target obj;
    auto bm = mc::bind_method(&member_target::add, &obj);
    mc::function<int(int)> fb = bm;
    require(fb(5), 105);
    const member_target cobj;
    auto bcm = mc::bind_method(&member_target::cadd, &cobj);
    mc::function<int(int)> fcb = bcm;
    require(fcb(5), 106);

    int sink = 0;
    mc::function<void(int)> fv = [&sink](int x) { sink += x; };
    fv(4);
    fv(5);
    require(sink, 9);
  }
  end_test_case();

  test_case("combinators over micron::function (all wrap mutable lambdas)");
  {

    mc::function<int(int)> g = [](int x) { return x * 2; };

    auto h = mc::fmap([](int y) { return y + 1; }, g);
    require(h(5), 11);

    auto b = mc::bind(g, [](int y) { return mc::function<int(int)>{ [y](int x) { return y + x; } }; });
    require(b(5), 15);

    auto p = mc::pure<int>(7);
    require(p(123), 7);

    mc::function<mc::function<int(int)>(int)> ff = [](int a) { return mc::function<int(int)>{ [a](int x) { return a * x; } }; };
    mc::function<int(int)> fg = [](int x) { return x + 3; };
    auto s = mc::ap(ff, fg);
    require(s(4), 28);

    mc::function<int(int)> dbl = [](int x) { return x * 2; };
    mc::function<int(int)> inc = [](int x) { return x + 1; };
    auto ltr = mc::compose_ltr(dbl, inc);
    auto rtl = mc::compose_rtl(dbl, inc);
    require(ltr(5), 11);
    require(rtl(5), 12);
    require((3 | dbl), 6);

    require(mc::const_fn(42)(0), 42);
    auto k = mc::const_fn(7);
    require(k(1, 2, 3), 7);
  }
  end_test_case();

#if defined(__EXCEPTIONS)
  test_case("a throwing target ctor must not orphan the heap block");
  {
    using Thr = mtest::Throwing<mtest::throw_on::copy_ctor, 7>;

    struct throwing_fn {
      Thr t;
      unsigned char pad[256];

      explicit throwing_fn(int v) : t()
      {
        t.v = v;
        for ( usize i = 0; i < 256; ++i ) pad[i] = static_cast<unsigned char>(i & 0xff);
      }

      int
      operator()(int x) const
      {
        return t.v + x;
      }
    };

    Thr::reset();
    {
      mc::function<int(int)> f{ throwing_fn(5) };
      require(f(1), 6);

      Thr::arm(0);
      bool raised = false;
      try {
        mc::function<int(int)> c(f);
        (void)c;
      } catch ( ... ) {
        raised = true;
      }
      Thr::disarm();
      require_true(raised);

      require(f(1), 6);
      f = throwing_fn(9);
      require(f(1), 10);
    }
    require(Thr::ctor, Thr::dtor);
  }
  end_test_case();

  test_case("copying a move-only target raises rather than corrupting");
  {
    mc::function<int(int)> f{ move_only_fn(40) };
    bool raised = false;
    try {
      mc::function<int(int)> c(f);
      (void)c;
    } catch ( ... ) {
      raised = true;
    }
    require_true(raised);
    require(f(2), 42);
  }
  end_test_case();
#endif

  test_case("seeded fuzz: random size/align classes through random op sequences");
  {
    constexpr int kPool = 6;
    prng rng(0x9E3779B97F4A7C15ULL);

    mtest::Tracked<9>::reset();
    {
      mc::function<int(int)> pool[kPool];
      int expect[kPool] = { 0 };
      for ( int i = 0; i < kPool; ++i ) expect[i] = -1;

      for ( usize round = 0; round < 10000; ++round ) {
        const int i = static_cast<int>(rng.next_in(kPool));
        const int j = static_cast<int>(rng.next_in(kPool));
        const u64 op = rng.next_in(7);

        switch ( op ) {
        case 0: {
          const int seed = static_cast<int>(rng.next_in(1000));
          switch ( rng.next_in(4) ) {
          case 0:
            pool[i] = tracked_fn<8, 9>(seed);
            break;
          case 1:
            pool[i] = tracked_fn<252, 9>(seed);
            break;
          case 2:
            pool[i] = tracked_aligned_fn<64, 9>(seed);
            break;
          default:
            pool[i] = tracked_aligned_fn<16, 9>(seed);
            break;
          }
          expect[i] = seed;
          break;
        }
        case 1:
          if ( expect[j] < 0 ) {
            pool[i] = nullptr;
          } else {
            pool[i] = pool[j];
          }
          expect[i] = expect[j];
          break;
        case 2:
          pool[i] = mc::move(pool[j]);
          expect[i] = expect[j];
          if ( i != j ) expect[j] = -1;
          break;
        case 3: {
          mc::function<int(int)> tmp(pool[j]);
          pool[i] = mc::move(tmp);
          expect[i] = expect[j];
          break;
        }
        case 4:
          pool[i].swap(pool[j]);
          {
            const int t = expect[i];
            expect[i] = expect[j];
            expect[j] = t;
          }
          break;
        case 5:
          pool[i] = nullptr;
          expect[i] = -1;
          break;
        default:
          for ( int k = 0; k < kPool; ++k ) {
            if ( expect[k] < 0 ) {
              require_false(static_cast<bool>(pool[k]));
            } else {
              require_true(static_cast<bool>(pool[k]));
              require(pool[k](3), expect[k] + 3);
            }
          }
          break;
        }
      }

      for ( int k = 0; k < kPool; ++k ) {
        if ( expect[k] < 0 )
          require_false(static_cast<bool>(pool[k]));
        else
          require(pool[k](3), expect[k] + 3);
      }
    }
    require(mtest::Tracked<9>::live(), static_cast<usize>(0));
  }
  end_test_case();

  sb::print("=== FUNCTION.HPP RIGOR SUITE PASSED ===");
  return 1;
}
