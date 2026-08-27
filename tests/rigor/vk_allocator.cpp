//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// the VkAllocationCallbacks shim, fuzzed against a shadow oracle.
//
// this needs no GPU, no libvulkan and no -vk: allocator.hpp reaches vulkan.hpp only for the record
// declarations, and the vk* entry points there are inline nullptr globals nothing here calls. the
// shims themselves are pure functions over abc::alloc.
//
// build the stats column too -- the counters are only worth having if they are checked:
//   duck test tests/rigor/vk_allocator.cpp -o bin/t -f
//   duck test tests/rigor/vk_allocator.cpp -o bin/t -f --def MICRON_VK_HOST_STATS

#include "../../src/gfx/vk/allocator.hpp"
#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

namespace v = micron::gfx::vk;
namespace h = micron::gfx::vk::__vk_host;
using scope_t = v::VkSystemAllocationScope;

using sb::end_test_case;
using sb::require;
using sb::test_case;

// fixed hex literal, never time-based
static u64 __rs = 0x243f6a8885a308d3ull;

static inline u64
__next(void)
{
  u64 x = __rs;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  __rs = x;
  return x;
}

static constexpr scope_t __scopes[5] = { scope_t::COMMAND, scope_t::OBJECT, scope_t::CACHE, scope_t::DEVICE, scope_t::INSTANCE };

// the oracle: what we asked for, and a byte pattern we can prove survived
struct shadow {
  void *p;
  usize size;
  usize align;
  scope_t scope;
  u8 seed;
};

static inline void
__fill(void *p, usize n, u8 seed)
{
  byte *b = reinterpret_cast<byte *>(p);
  for ( usize i = 0; i < n; ++i ) b[i] = byte((seed + u8(i * 31u)) & 0xffu);
}

static inline bool
__check(const void *p, usize n, u8 seed)
{
  const byte *b = reinterpret_cast<const byte *>(p);
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != byte((seed + u8(i * 31u)) & 0xffu) ) return false;
  return true;
}

int
main()
{
  sb::print("=== VK HOST ALLOCATOR ===");

  const v::host_alloc_stats_t base = v::host_alloc_stats();
  sb::print("stats compiled in: ", int(base.enabled));

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("alignment sweep, every power of two through 4096");
  {
    for ( usize a = 1; a <= 4096; a <<= 1 ) {
      for ( usize s = 1; s <= 300; s += 37 ) {
        void *p = h::__alloc(s, a, scope_t::OBJECT);
        require(p != nullptr);
        // the shim floors the effective alignment at __min_align, so it may be STRONGER than asked
        const usize eff = a > h::__min_align ? a : h::__min_align;
        require(reinterpret_cast<uintptr_t>(p) % eff == 0);
        require(h::__size_of(p) == s);      // the REQUESTED size, not abc's granted one
        h::__free(p);
      }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("header survives a full-width write to the payload");
  {
    // an off-by-one in the header offsets shows up as the payload trampling scope or offset_to_raw,
    // which then only detonates at free. write every byte, then read the header back.
    for ( usize a = 1; a <= 256; a <<= 1 ) {
      for ( usize s = 1; s <= 129; s += 16 ) {
        void *p = h::__alloc(s, a, scope_t::INSTANCE);
        require(p != nullptr);
        __fill(p, s, 0xa5u);
        require(h::__size_of(p) == s);
        require(h::__scope_of(p) == scope_t::INSTANCE);
        require(__check(p, s, 0xa5u));
        h::__free(p);
      }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("scope round-trips through the header for all five values");
  {
    for ( u32 i = 0; i < 5; ++i ) {
      void *p = h::__alloc(64, 16, __scopes[i]);
      require(p != nullptr);
      require(h::__scope_of(p) == __scopes[i]);
      h::__free(p);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("rejected inputs return null rather than a bad block");
  {
    require(h::__alloc(0, 16, scope_t::OBJECT) == nullptr);      // size 0
    require(h::__alloc(64, 0, scope_t::OBJECT) == nullptr);      // align 0
    require(h::__alloc(64, 3, scope_t::OBJECT) == nullptr);      // align not a power of two
    require(h::__alloc(64, 24, scope_t::OBJECT) == nullptr);
    require(h::__alloc(~usize(0), 16, scope_t::OBJECT) == nullptr);      // size + header overflows
    require(h::__alloc(~usize(0) - 16, 16, scope_t::OBJECT) == nullptr);
    h::__free(nullptr);      // must be a no-op, not a fault
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("realloc preserves min(old,new) bytes, grow and shrink");
  {
    for ( usize old_s = 8; old_s <= 512; old_s <<= 1 ) {
      for ( usize new_s = 8; new_s <= 512; new_s <<= 1 ) {
        void *p = h::__alloc(old_s, 32, scope_t::CACHE);
        require(p != nullptr);
        __fill(p, old_s, 0x3cu);
        void *q = h::__realloc(p, new_s, 32, scope_t::CACHE);
        require(q != nullptr);
        require(h::__size_of(q) == new_s);
        require(reinterpret_cast<uintptr_t>(q) % 32 == 0);
        require(__check(q, old_s < new_s ? old_s : new_s, 0x3cu));
        h::__free(q);
      }
    }
    // null original behaves as a fresh alloc; size 0 frees and yields null
    void *r = h::__realloc(nullptr, 128, 16, scope_t::DEVICE);
    require(r != nullptr);
    require(h::__size_of(r) == 128);
    require(h::__scope_of(r) == scope_t::DEVICE);
    require(h::__realloc(r, 0, 16, scope_t::DEVICE) == nullptr);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("a FAILED realloc leaves the original intact");
  {
    // the spec is explicit: if reallocation fails the original allocation must be unmodified and
    // still owned by the caller. __realloc allocates before it frees, so this holds -- but nothing
    // asserted it, and the obvious "optimization" of freeing first would pass every other case here.
    void *p = h::__alloc(256, 32, scope_t::CACHE);
    require(p != nullptr);
    __fill(p, 256, 0x7eu);

    const v::host_alloc_stats_t s0 = v::host_alloc_stats();
    require(h::__realloc(p, ~usize(0), 32, scope_t::CACHE) == nullptr);
    require(h::__size_of(p) == 256);
    require(h::__scope_of(p) == scope_t::CACHE);
    require(__check(p, 256, 0x7eu));
    if ( s0.enabled ) require(v::host_alloc_stats().live_blocks == s0.live_blocks);
    h::__free(p);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("randomized interleave against the shadow oracle");
  {
    constexpr u32 __slots = 192;
    shadow live[__slots]{};
    for ( u32 i = 0; i < __slots; ++i ) live[i].p = nullptr;

    for ( u32 step = 0; step < 20000; ++step ) {
      const u32 slot = u32(__next() % __slots);
      if ( live[slot].p == nullptr ) {
        const usize size = usize(__next() % 1024u) + 1u;
        const usize align = usize(1) << (__next() % 9u);      // 1 .. 256
        const scope_t sc = __scopes[__next() % 5u];
        const u8 seed = u8(__next() & 0xffu);
        void *p = h::__alloc(size, align, sc);
        require(p != nullptr);
        const usize eff = align > h::__min_align ? align : h::__min_align;
        require(reinterpret_cast<uintptr_t>(p) % eff == 0);
        __fill(p, size, seed);
        live[slot] = shadow{ p, size, align, sc, seed };
      } else if ( (__next() & 3u) == 0u ) {
        // reallocate in place of a free, so the realloc path sees a live oracle entry too
        const usize ns = usize(__next() % 1024u) + 1u;
        shadow &s = live[slot];
        void *q = h::__realloc(s.p, ns, s.align, s.scope);
        require(q != nullptr);
        require(__check(q, s.size < ns ? s.size : ns, s.seed));
        s.p = q;
        if ( ns > s.size ) __fill(q, ns, s.seed);
        s.size = ns;
      } else {
        shadow &s = live[slot];
        require(h::__size_of(s.p) == s.size);
        require(h::__scope_of(s.p) == s.scope);
        require(__check(s.p, s.size, s.seed));      // nothing else scribbled on it
        h::__free(s.p);
        s.p = nullptr;
      }
    }
    for ( u32 i = 0; i < __slots; ++i )
      if ( live[i].p != nullptr ) {
        require(__check(live[i].p, live[i].size, live[i].seed));
        h::__free(live[i].p);
      }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the public callback table routes to the same shims");
  {
    const v::VkAllocationCallbacks *cb = v::host_allocation_callbacks();
    require(cb != nullptr);
    require(cb->pfnAllocation != nullptr and cb->pfnFree != nullptr and cb->pfnReallocation != nullptr);
    require(cb->pfnInternalAllocation != nullptr and cb->pfnInternalFree != nullptr);

    void *p = cb->pfnAllocation(cb->pUserData, 200, 64, scope_t::INSTANCE);
    require(p != nullptr);
    require(reinterpret_cast<uintptr_t>(p) % 64 == 0);
    require(h::__size_of(p) == 200);
    require(h::__scope_of(p) == scope_t::INSTANCE);
    __fill(p, 200, 0x5au);
    void *q = cb->pfnReallocation(cb->pUserData, p, 400, 64, scope_t::INSTANCE);
    require(q != nullptr);
    require(__check(q, 200, 0x5au));
    cb->pfnFree(cb->pUserData, q);
    cb->pfnFree(cb->pUserData, nullptr);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("internal-allocation notifications are counted");
  {
    // a driver that goes around us to its own malloc tells us only through these two callbacks.
    // they are the entire reason "the ICD leaked ~1000 B of its own" is a measurement rather than
    // a guess, so a silently-wrong counter here would be worse than no counter at all.
    const v::VkAllocationCallbacks *cb = v::host_allocation_callbacks();
    const v::host_alloc_stats_t s0 = v::host_alloc_stats();

    cb->pfnInternalAllocation(cb->pUserData, 4096, v::VkInternalAllocationType::EXECUTABLE, scope_t::DEVICE);
    cb->pfnInternalFree(cb->pUserData, 1024, v::VkInternalAllocationType::EXECUTABLE, scope_t::DEVICE);

    const v::host_alloc_stats_t s1 = v::host_alloc_stats();
    if ( s0.enabled ) {
      require(s1.internal_allocs == s0.internal_allocs + 1);
      require(s1.internal_alloc_bytes == s0.internal_alloc_bytes + 4096ull);
      require(s1.internal_frees == s0.internal_frees + 1);
      require(s1.internal_free_bytes == s0.internal_free_bytes + 1024ull);
      // internal traffic is the driver's own heap, so it must NOT move our block accounting
      require(s1.live_blocks == s0.live_blocks);
      require(s1.allocs == s0.allocs);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("deleters default to OUR allocator, not the driver's");
  {
    // a nullptr default here means an object created through host_allocation_callbacks() gets freed
    // by the driver's allocator, past a header it does not know about
    const v::buffer_deleter bd{};
    const v::instance_deleter id{};
    const v::device_deleter dd{};
    require(bd.alloc == v::host_allocation_callbacks());
    require(id.alloc == v::host_allocation_callbacks());
    require(dd.alloc == v::host_allocation_callbacks());
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("counters balance and attribute by scope");
  {
    const v::host_alloc_stats_t before = v::host_alloc_stats();
    if ( !before.enabled ) {
      sb::print("  (MICRON_VK_HOST_STATS off -- accounting checks skipped)");
      require(before.live_blocks == 0 and before.allocs == 0);      // must not fake numbers
    } else {
      // everything above balanced, so the live set is back where it started
      require(before.live_blocks == 0);
      require(before.live_bytes == 0);
      require(before.foreign_frees == 0);
      require(before.allocs == before.frees);

      constexpr u32 __n = 64;
      void *held[__n]{};
      for ( u32 i = 0; i < __n; ++i ) held[i] = h::__alloc(128, 16, scope_t::INSTANCE);

      const v::host_alloc_stats_t mid = v::host_alloc_stats();
      require(mid.live_blocks == before.live_blocks + __n);
      require(mid.live_bytes == before.live_bytes + u64(__n) * 128ull);
      require(mid.scope_blocks[4] == before.scope_blocks[4] + __n);      // INSTANCE == 4
      require(mid.scope_bytes[4] == before.scope_bytes[4] + u64(__n) * 128ull);
      require(mid.scope_blocks[0] == before.scope_blocks[0]);      // COMMAND untouched
      require(mid.peak_bytes >= mid.live_bytes);
      require(mid.allocs - mid.frees == mid.live_blocks);      // the global identity

      // a realloc that changes scope must MOVE the block between buckets
      void *m = h::__realloc(held[0], 128, 16, scope_t::COMMAND);
      require(m != nullptr);
      held[0] = m;
      const v::host_alloc_stats_t moved = v::host_alloc_stats();
      require(moved.scope_blocks[4] == before.scope_blocks[4] + __n - 1);
      require(moved.scope_blocks[0] == before.scope_blocks[0] + 1);
      require(moved.live_blocks == before.live_blocks + __n);
      require(moved.reallocs == mid.reallocs + 1);

      for ( u32 i = 0; i < __n; ++i ) h::__free(held[i]);

      const v::host_alloc_stats_t after = v::host_alloc_stats();
      require(after.live_blocks == 0);
      require(after.live_bytes == 0);
      require(after.scope_blocks[4] == 0);
      require(after.scope_blocks[0] == 0);
      require(after.foreign_frees == 0);
      require(after.allocs - after.frees == after.live_blocks);

      // a block whose magic does not match was not handed out by us -- or was handed back already.
      // free() still deallocs it (a stats build must not behave differently from a release build),
      // but it must not decrement counters it never incremented.
      void *odd = h::__alloc(64, 16, scope_t::OBJECT);
      require(odd != nullptr);
      *reinterpret_cast<usize *>(reinterpret_cast<byte *>(odd) - h::__off_magic) = 0xdeadu;
      const v::host_alloc_stats_t f0 = v::host_alloc_stats();
      h::__free(odd);
      const v::host_alloc_stats_t f1 = v::host_alloc_stats();
      require(f1.foreign_frees == f0.foreign_frees + 1);
      require(f1.live_blocks == f0.live_blocks);      // NOT decremented
      require(f1.frees == f0.frees);
      sb::print("  peak host bytes over the whole run: ", after.peak_bytes);
    }
  }
  end_test_case();

  sb::print("=== VK HOST ALLOCATOR OK ===");
  return 1;
}
