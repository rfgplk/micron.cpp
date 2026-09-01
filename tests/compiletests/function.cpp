//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate for src/function.hpp. Not run -- the point is that every instantiation below
// COMPILES, on every arch x ISA x EH cell verify_compile_*.duck sweeps this directory under.
//
// Three of the shapes here were hard compile errors before 2026-08-18, and none of them were reachable
// from any test in the tree:
//   - a mutable lambda / stateful functor  (__fn_vtable::call invoked through `const G *`)
//   - a move-only target                   (the copy slot used a runtime ternary, so the copy body
//                                           instantiated for every G instead of collapsing to nullptr)
//   - micron::fmap / bind / pure / ap over micron::function, every one of which wraps a mutable lambda
//
// The over-aligned and >192-byte shapes are here because they are the ones that go down the heap path,
// which is where the allocation/deallocation defects lived.

#include "../../src/function.hpp"

namespace mc = micron;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// layout invariants the heap path depends on

static_assert(mc::__function_smallobj_size == 48);
static_assert(mc::__function_smallobj_align == 16);
static_assert(mc::__function_smallobj_align >= alignof(void *));

// raising the SBO alignment to 16 must stay free on a 64-bit target
static_assert(sizeof(void *) != 8 || sizeof(mc::function<int(int)>) == 64);
static_assert(alignof(mc::function<int(int)>) >= 16);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// targets

struct stateful {
  int acc = 0;

  int
  operator()(int x) noexcept      // NOT const
  {
    acc += x;
    return acc;
  }
};

struct big_target {      // > 192 bytes: the size that used to overflow the copy block
  unsigned char pad[300];

  int
  operator()(int x) const noexcept
  {
    return x + static_cast<int>(pad[0]);
  }
};

struct alignas(64) over_aligned {      // align > abcmalloc's __hdr_offset (32)
  unsigned char pad[64];

  int
  operator()(int x) const noexcept
  {
    return x + static_cast<int>(pad[0]);
  }
};

struct alignas(128) very_over_aligned {
  unsigned char pad[256];

  int
  operator()(int x) const noexcept
  {
    return x + static_cast<int>(pad[0]);
  }
};

struct move_only {
  int v = 1;

  move_only() = default;
  move_only(const move_only &) = delete;
  move_only &operator=(const move_only &) = delete;
  move_only(move_only &&) = default;

  int
  operator()(int x) const noexcept
  {
    return v + x;
  }
};

struct big_move_only {
  unsigned char pad[300];

  big_move_only() = default;
  big_move_only(const big_move_only &) = delete;
  big_move_only &operator=(const big_move_only &) = delete;
  big_move_only(big_move_only &&) = default;

  int
  operator()(int x) const noexcept
  {
    return x + static_cast<int>(pad[0]);
  }
};

struct holder {
  int base = 0;

  int
  add(int x)
  {
    return base + x;
  }

  int
  cadd(int x) const
  {
    return base + x;
  }
};

static_assert(!mc::is_copy_constructible_v<move_only>);
static_assert(!mc::is_copy_constructible_v<big_move_only>);
static_assert(sizeof(big_target) > mc::__function_smallobj_size * 4);
static_assert(alignof(over_aligned) > 32);
static_assert(alignof(very_over_aligned) > 32);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// instantiations. everything is forced through the copy AND move paths, since the two used to have
// different (and differently broken) allocation rules.

template<typename F, typename T>
[[gnu::noinline]] static int
round_trip(T &&t, int arg)
{
  F a{ static_cast<T &&>(t) };
  F b(mc::move(a));      // move ctor
  F c;
  c = mc::move(b);      // move assign
  F d;
  d.swap(c);      // swap
  int r = d(arg);
  d = nullptr;      // operator=(nullptr_t)
  return r + (d == nullptr ? 1 : 0) + (d != nullptr ? 2 : 0) + (!d ? 4 : 0) + (static_cast<bool>(d) ? 8 : 0);
}

template<typename F, typename T>
[[gnu::noinline]] static int
round_trip_copyable(const T &t, int arg)
{
  F a{ t };
  F b(a);      // copy ctor  -- the 192-byte-constant path
  F c;
  c = a;      // copy assign
  F d{ a };
  d = b;
  int r = a(arg) + b(arg) + c(arg) + d(arg);
  const F &ca = a;
  r += ca(arg);      // const operator() -- must still reach a non-const target
  r += ca.is_noexcept() ? 1 : 0;
  r += ca.template target<T>() != nullptr ? 2 : 0;
  r += a.template target<T>() != nullptr ? 4 : 0;
  return r;
}

[[gnu::noinline]] static int
instantiate_all(void)
{
  int r = 0;

  // plain lambdas, SBO
  r += round_trip_copyable<mc::function<int(int)>>([](int x) { return x + 1; }, 1);
  r += round_trip_copyable<mc::function<int(int)>>([](int x) noexcept { return x + 2; }, 1);

  // mutable lambda and stateful functor -- both need call() through G*, not const G*
  int cap = 0;
  auto mut = [cap](int x) mutable {
    cap += x;
    return cap;
  };
  r += round_trip_copyable<mc::function<int(int)>>(mut, 1);
  r += round_trip_copyable<mc::function<int(int)>>(stateful{}, 1);

  // heap shapes
  r += round_trip_copyable<mc::function<int(int)>>(big_target{}, 1);
  r += round_trip_copyable<mc::function<int(int)>>(over_aligned{}, 1);
  r += round_trip_copyable<mc::function<int(int)>>(very_over_aligned{}, 1);

  // move-only targets: copy slot must collapse to nullptr instead of instantiating
  r += round_trip<mc::function<int(int)>>(move_only{}, 1);
  r += round_trip<mc::function<int(int)>>(big_move_only{}, 1);

  // signature variety
  mc::function<void()> v0 = [] { };
  v0();
  mc::function<void(int)> v1 = [](int) { };
  v1(1);
  mc::function<int(int, long, char)> v3 = [](int a, long b, char c) { return a + static_cast<int>(b) + c; };
  r += v3(1, 2, 3);
  mc::function<int(const int &)> cref = [](const int &x) { return x; };
  const int one = 1;
  r += cref(one);
  mc::function<int(int &)> lref = [](int &x) { return ++x; };
  int mutable_one = 1;
  r += lref(mutable_one);
  mc::function<long(int)> widen = [](int x) { return x; };      // return-type conversion
  r += static_cast<int>(widen(1));

  // member function pointers, const and non-const
  holder h;
  mc::function<int(int)> fm = mc::bind_method(&holder::add, &h);
  r += fm(1);
  const holder ch;
  mc::function<int(int)> fcm = mc::bind_method(&holder::cadd, &ch);
  r += fcm(1);

  // free swap
  mc::function<int(int)> s1 = [](int x) { return x; };
  mc::function<int(int)> s2 = big_target{};
  mc::swap(s1, s2);
  r += s1(1) + s2(1);

  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the combinator layer. every one of these wraps a `mutable` lambda in a micron::function, so before
// the call-through-G* fix this whole section was uninstantiable dead code.

[[gnu::noinline]] static int
instantiate_combinators(void)
{
  int r = 0;

  mc::function<int(int)> g = [](int x) { return x * 2; };

  auto h = mc::fmap([](int y) { return y + 1; }, g);
  r += h(1);

  auto b = mc::bind(g, [](int y) { return mc::function<int(int)>{ [y](int x) { return y + x; } }; });
  r += b(1);

  auto p = mc::pure<int>(7);
  r += p(1);

  mc::function<mc::function<int(int)>(int)> nested = [](int a) { return mc::function<int(int)>{ [a](int x) { return a * x; } }; };
  auto s = mc::ap(nested, g);
  r += s(2);

  r += mc::compose_ltr(g, g)(1);
  r += mc::compose_rtl(g, g)(1);
  r += (1 | g);

  // the non-function-erasing combinators, none of which had a caller either
  r += mc::flip([](int a, long c) { return a + static_cast<int>(c); })(1L, 2);
  r += mc::partial([](int a, int c, int d) { return a + c + d; }, 1)(2, 3);
  r += mc::curry([](int a, int c) { return a + c; })(1)(2);
  r += mc::const_fn(3)(0, 0);
  r += mc::fix([](auto &&self, int n) -> int { return n <= 1 ? 1 : n * self(n - 1); })(4);
  r += mc::identity(5);

  auto lz = mc::lazy([] { return 9; });
  r += lz();
  r += mc::force(lz);

  return r;
}

int
main()
{
  return (instantiate_all() != 0 && instantiate_combinators() != 0) ? 1 : 0;
}
