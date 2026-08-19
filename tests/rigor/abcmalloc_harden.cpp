//  Copyright (c) 2025 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/io/console.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../../src/memory/allocation/abcmalloc/sheet_header.hpp"
#include "../../src/std.hpp"

#include "../../src/bits/__pause.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr byte kPattern = 0xA7;

bool
all_bytes_are(const byte *p, usize n, byte v)
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] != v ) return false;
  return true;
}

void
fill(byte *p, usize n, byte v)
{
  for ( usize i = 0; i < n; ++i ) p[i] = v;
}

constexpr usize kBlocks = 64;
constexpr usize kSize = 96;

struct handoff {
  byte *ptrs[kBlocks];
  usize lens[kBlocks];
  micron::atomic_token<u32> filled{ 0 };
  micron::atomic_token<u32> freed{ 0 };
  bool sized;
};

void
remote_freer(handoff *h)
{
  while ( h->filled.get(micron::memory_order_acquire) != 1u ) __cpu_pause();
  for ( usize i = 0; i < kBlocks; ++i ) {
    if ( h->sized )
      abc::dealloc(h->ptrs[i], h->lens[i]);
    else
      abc::dealloc(h->ptrs[i]);
  }
  h->freed.store(1, micron::memory_order_release);
}

void
hand_off_and_free(handoff &h, bool sized)
{
  h.sized = sized;
  for ( usize i = 0; i < kBlocks; ++i ) {
    auto c = abc::balloc(kSize);
    h.ptrs[i] = c.ptr;
    h.lens[i] = c.len;
    fill(c.ptr, kSize, kPattern);
  }
  h.filled.store(1, micron::memory_order_release);
  micron::auto_thread<> t(remote_freer, &h);
  t.join();
  require_true(h.freed.get(micron::memory_order_acquire) == 1u);
}

};      // namespace

int
main()
{

  if constexpr ( abc::__default_zero_on_free ) {
    test_case("zero-on-free: sized same-thread free actually zeroes");
    {
      byte *p = abc::alloc(kSize);
      require_true(p != nullptr);
      fill(p, kSize, kPattern);
      abc::dealloc(p, kSize);

      require_true(all_bytes_are(p, kSize, 0));
    }
    end_test_case();

    test_case("zero-on-free: SIZE-LESS free scrubs too");
    {

      byte *p = abc::alloc(kSize);
      require_true(p != nullptr);
      fill(p, kSize, kPattern);
      abc::free(p);
      require_true(all_bytes_are(p, kSize, 0));
    }
    end_test_case();

    test_case("zero-on-free: cross-thread free scrubs (was intra-thread only)");
    {

      handoff h{};
      hand_off_and_free(h, true);

      (void)abc::alloc(kSize);
      for ( usize i = 0; i < kBlocks; ++i ) require_true(all_bytes_are(h.ptrs[i], kSize, 0));
    }
    end_test_case();

    test_case("zero-on-free: cross-thread SIZE-LESS free scrubs");
    {
      handoff h{};
      hand_off_and_free(h, false);
      (void)abc::alloc(kSize);
      for ( usize i = 0; i < kBlocks; ++i ) require_true(all_bytes_are(h.ptrs[i], kSize, 0));
    }
    end_test_case();

    test_case("zero-on-free: over-sized free is clamped, not an overrun");
    {

      byte *victim = abc::alloc(100);
      byte *sentinel = abc::alloc(100);
      require_true(victim != nullptr and sentinel != nullptr);
      fill(sentinel, 100, kPattern);
      abc::dealloc(victim, 4096);
      require_true(all_bytes_are(sentinel, 100, kPattern));
      abc::dealloc(sentinel, 100);
    }
    end_test_case();

    test_case("zero-on-free: foreign pointer is not written through");
    {

      byte stack_block[128];
      fill(stack_block, sizeof(stack_block), kPattern);
      require_true(abc::__owner_of(stack_block) == nullptr);
      abc::dealloc(stack_block);
      require_true(all_bytes_are(stack_block, sizeof(stack_block), kPattern));
    }
    end_test_case();
  }

  if constexpr ( abc::__default_poison_on_free ) {
    test_case("poison-on-free: freed block carries the poison byte");
    {
      byte *p = abc::alloc(kSize);
      require_true(p != nullptr);
      fill(p, kSize, kPattern);
      abc::dealloc(p, kSize);
      require_true(all_bytes_are(p, kSize, abc::__default_poison_byte));
    }
    end_test_case();

    test_case("poison-on-free: cross-thread free poisons (was intra-thread only)");
    {
      handoff h{};
      hand_off_and_free(h, true);
      (void)abc::alloc(kSize);
      for ( usize i = 0; i < kBlocks; ++i ) require_true(all_bytes_are(h.ptrs[i], kSize, abc::__default_poison_byte));
    }
    end_test_case();

    test_case("poison-on-free: foreign pointer is not written through");
    {
      byte stack_block[128];
      fill(stack_block, sizeof(stack_block), kPattern);
      require_true(abc::__owner_of(stack_block) == nullptr);
      abc::dealloc(stack_block);
      require_true(all_bytes_are(stack_block, sizeof(stack_block), kPattern));
    }
    end_test_case();
  }

  if constexpr ( abc::__default_redzone ) {
    test_case("redzone: user pointer keeps natural alignment");
    {

      for ( usize n = 1; n <= 512; n += 7 ) {
        byte *p = abc::alloc(n);
        require_true(p != nullptr);
        require_true((reinterpret_cast<uintptr_t>(p) & 15u) == 0);
        abc::dealloc(p, n);
      }
    }
    end_test_case();

    test_case("redzone: the __class_medium boundary band round-trips");
    {

      const usize lo = abc::__class_medium - 4 * abc::__default_redzone_size;
      const usize hi = abc::__class_medium + 2 * abc::__default_redzone_size;
      for ( usize n = lo; n <= hi; ++n ) {
        byte *p = abc::alloc(n);
        require_true(p != nullptr);
        fill(p, n, kPattern);
        require_true(all_bytes_are(p, n, kPattern));
        abc::dealloc(p, n);
      }
    }
    end_test_case();

    test_case("redzone: boundary band survives a size-less free too");
    {
      const usize lo = abc::__class_medium - 4 * abc::__default_redzone_size;
      const usize hi = abc::__class_medium + 2 * abc::__default_redzone_size;
      for ( usize n = lo; n <= hi; ++n ) {
        byte *p = abc::alloc(n);
        require_true(p != nullptr);
        fill(p, n, kPattern);
        abc::free(p);
      }
    }
    end_test_case();

    test_case("redzone: in-place realloc re-arms the trailing canary");
    {

      for ( usize base = 32; base <= 256; base += 32 ) {
        byte *p = reinterpret_cast<byte *>(abc::malloc(base));
        require_true(p != nullptr);
        const usize grown = abc::query_size(p);
        require_true(grown >= base);
        byte *q = reinterpret_cast<byte *>(abc::realloc(p, grown));
        require_true(q != nullptr);
        fill(q, grown, kPattern);
        abc::dealloc(q, grown);
      }
    }
    end_test_case();

    test_case("redzone: cross-thread free verifies canaries");
    {
      handoff h{};
      hand_off_and_free(h, true);
      (void)abc::alloc(kSize);
    }
    end_test_case();
  }

  if constexpr ( abc::__default_enforce_provenance ) {
    test_case("provenance: legitimate cross-thread frees still pass the gate");
    {

      handoff h{};
      hand_off_and_free(h, true);
      handoff h2{};
      hand_off_and_free(h2, false);
      (void)abc::alloc(kSize);
    }
    end_test_case();

    test_case("provenance: is_valid_block agrees across threads");
    {
      byte *p = abc::alloc(kSize);
      require_true(p != nullptr);
      require_true(abc::within(p));
      require_true(abc::__owner_of(p) != nullptr);
      abc::dealloc(p, kSize);
    }
    end_test_case();
  }

  test_case("cross-thread free round-trips under every build");
  {
    handoff h{};
    hand_off_and_free(h, true);
    handoff h2{};
    hand_off_and_free(h2, false);
    for ( usize i = 0; i < kBlocks; ++i ) {
      byte *p = abc::alloc(kSize);
      require_true(p != nullptr);
      abc::dealloc(p, kSize);
    }
  }
  end_test_case();

  micron::console("=== ALL ABCMALLOC HARDEN TESTS PASSED ===\n");
  return 1;
}
