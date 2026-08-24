// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "../../src/io/console.hpp"
#include "../../src/memory/allocation/abcmalloc/cache_list.hpp"
#include "../../src/memory/allocation/abcmalloc/free_list.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

namespace
{

using tlsf_t = abc::__tlsf_list<micron::__chunk<byte>, 64, 64>;
using buddy_t = abc::__buddy_list<micron::__chunk<byte>, 4096, 64>;

constexpr usize TLSF_POOL_BYTES = 2ULL << 20;
constexpr usize BUDDY_POOL_BYTES = 4ULL << 20;
constexpr usize BUDDY_BLOCKS = BUDDY_POOL_BYTES / 4096;

alignas(4096) static byte g_tlsf_pool[TLSF_POOL_BYTES];
alignas(4096) static byte g_tlsf_exact[8192];
alignas(4096) static byte g_buddy_pool[BUDDY_POOL_BYTES];
alignas(64) static byte g_buddy_tags[BUDDY_BLOCKS];
alignas(64) static micron::__chunk<byte> g_blocks[BUDDY_BLOCKS];

bool
non_overlapping(const micron::__chunk<byte> &a, const micron::__chunk<byte> &b) noexcept
{
  return a.ptr + a.len <= b.ptr || b.ptr + b.len <= a.ptr;
}

u32
next_random(u32 &state) noexcept
{
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

void
stamp(micron::__chunk<byte> &block, usize requested, byte value) noexcept
{
  const usize span = requested ? requested : 1;
  block.ptr[0] = value;
  if ( span > 1 ) block.ptr[span - 1] = static_cast<byte>(value ^ 0xA5u);
}

bool
stamp_valid(const micron::__chunk<byte> &block, usize requested, byte value) noexcept
{
  const usize span = requested ? requested : 1;
  return block.ptr[0] == value && (span == 1 || block.ptr[span - 1] == static_cast<byte>(value ^ 0xA5u));
}

};      // namespace

int
main()
{
  using namespace snowball;
  sb::print("=== ABCMALLOC TLSF / BUDDY CORE EDGE TESTS ===");

  test_case("TLSF size-class boundaries split without overlap and fully coalesce");
  {
    tlsf_t alloc({ g_tlsf_pool, TLSF_POOL_BYTES });
    constexpr usize sizes[] = { 0,   1,   31,  32,  33,  63,  64,  65,  95,   96,   97,   127,  128,  129, 255,
                                256, 257, 511, 512, 513, 991, 992, 993, 1023, 1024, 1025, 2047, 2048, 4095 };
    constexpr usize count = sizeof(sizes) / sizeof(sizes[0]);
    micron::__chunk<byte> blocks[count];

    for ( usize i = 0; i < count; ++i ) {
      blocks[i] = alloc.allocate(sizes[i]);
      require(blocks[i].ptr != nullptr, true);
      require(blocks[i].len >= sizes[i], true);
      for ( usize j = 0; j < i; ++j ) require(non_overlapping(blocks[i], blocks[j]), true);
      if ( sizes[i] ) {
        blocks[i].ptr[0] = static_cast<byte>(i);
        blocks[i].ptr[sizes[i] - 1] = static_cast<byte>(i ^ 0xA5u);
      }
    }
    for ( usize i = 1; i < count; i += 2 ) require(alloc.deallocate(blocks[i].ptr) == abc::__flag_ok, true);
    for ( usize i = 0; i < count; i += 2 ) require(alloc.deallocate(blocks[i].ptr) == abc::__flag_ok, true);
    require(alloc.used(), static_cast<usize>(0));
    require(alloc.fl_bitmap != 0, true);
    require(alloc.deallocate(blocks[0].ptr) == abc::__flag_invalid, true);
  }
  end_test_case();

  test_case("TLSF fixed-seed fragmentation churn preserves live blocks and rejoins the pool");
  {
    constexpr usize live_count = 256;
    constexpr usize operations = 50'000;
    micron::__chunk<byte> live[live_count]{};
    usize requested[live_count]{};
    byte marks[live_count]{};
    u32 state = 0x54B1'9A37u;
    tlsf_t alloc({ g_tlsf_pool, TLSF_POOL_BYTES });

    for ( usize op = 0; op < operations; ++op ) {
      const usize slot = next_random(state) & (live_count - 1);
      if ( live[slot].ptr ) {
        require(stamp_valid(live[slot], requested[slot], marks[slot]), true);
        require(alloc.deallocate(live[slot].ptr) == abc::__flag_ok, true);
        live[slot] = { nullptr, 0 };
      } else {
        const usize n = next_random(state) & 8191u;
        micron::__chunk<byte> block = alloc.allocate(n);
        require(block.ptr != nullptr, true);
        require(block.len >= n, true);
        for ( usize i = 0; i < live_count; ++i )
          if ( live[i].ptr ) require(non_overlapping(block, live[i]), true);
        const byte mark = static_cast<byte>((op ^ slot) | 1u);
        stamp(block, n, mark);
        live[slot] = block;
        requested[slot] = n;
        marks[slot] = mark;
      }
    }

    for ( usize i = 0; i < live_count; ++i ) {
      if ( !live[i].ptr ) continue;
      require(stamp_valid(live[i], requested[i], marks[i]), true);
      require(alloc.deallocate(live[i].ptr) == abc::__flag_ok, true);
    }
    require(alloc.used(), static_cast<usize>(0));
    auto *whole = reinterpret_cast<tlsf_t::tlsf_hdr *>(alloc.base + tlsf_t::__block_align);
    require(whole->bsize, static_cast<u32>(alloc.total));
    require(whole->flags, static_cast<i32>(abc::__block_free));
    require(whole->prev_phys, reinterpret_cast<tlsf_t::tlsf_hdr *>(alloc.base));
    require(alloc.next_phys(whole)->prev_phys, whole);
  }
  end_test_case();

  test_case("TLSF temporal slot clears when a no-split remainder is exactly 32 bytes");
  {
    constexpr usize sizes[] = { 1, 31, 63, 95, 127, 255, 511, 929, 991, 1023 };
    for ( usize requested : sizes ) {
      const usize needed = tlsf_t::adjusted_block_size(requested);
      const usize pool_len = needed + 3 * tlsf_t::__block_align;
      tlsf_t alloc({ g_tlsf_exact, pool_len });
      micron::__chunk<byte> temporal = alloc.temporal_allocate(requested);
      require(temporal.ptr != nullptr, true);
      require(temporal.len + abc::__hdr_offset, needed + tlsf_t::__block_align);
      require(alloc.deallocate(temporal.ptr) == abc::__flag_ok, true);

      micron::__chunk<byte> live = alloc.allocate(requested);
      require(live.ptr, temporal.ptr);
      micron::__chunk<byte> stale = alloc.temporal_allocate(requested);
      require(stale.ptr == nullptr, true);
      require(alloc.deallocate(live.ptr) == abc::__flag_ok, true);
    }
  }
  end_test_case();

  test_case("buddy fixed-seed order churn preserves payloads and restores one top block");
  {
    constexpr usize live_count = 128;
    constexpr usize operations = 50'000;
    micron::__chunk<byte> live[live_count]{};
    usize requested[live_count]{};
    byte marks[live_count]{};
    u32 state = 0xBADD'C0DEu;
    buddy_t alloc({ g_buddy_pool, BUDDY_POOL_BYTES }, g_buddy_tags);

    for ( usize op = 0; op < operations; ++op ) {
      const usize slot = next_random(state) & (live_count - 1);
      if ( live[slot].ptr ) {
        require(stamp_valid(live[slot], requested[slot], marks[slot]), true);
        require(alloc.deallocate(live[slot].ptr) == abc::__flag_ok, true);
        live[slot] = { nullptr, 0 };
      } else {
        const usize n = next_random(state) & 32'767u;
        micron::__chunk<byte> block = alloc.allocate(n);
        require(block.ptr != nullptr, true);
        require(block.len >= n, true);
        for ( usize i = 0; i < live_count; ++i )
          if ( live[i].ptr ) require(non_overlapping(block, live[i]), true);
        const byte mark = static_cast<byte>((op + slot * 17u) | 1u);
        stamp(block, n, mark);
        live[slot] = block;
        requested[slot] = n;
        marks[slot] = mark;
      }
    }

    for ( usize i = 0; i < live_count; ++i ) {
      if ( !live[i].ptr ) continue;
      require(stamp_valid(live[i], requested[i], marks[i]), true);
      require(alloc.deallocate(live[i].ptr) == abc::__flag_ok, true);
    }
    require(alloc.used(), static_cast<usize>(0));
    require(alloc.free_mask, u64(1) << (alloc.max_order - 1));
    require(alloc.free_lists[alloc.max_order - 1], reinterpret_cast<buddy_t::free_block *>(g_buddy_pool));
    require(alloc.free_lists[alloc.max_order - 1]->next == nullptr, true);
    for ( usize i = 1; i < BUDDY_BLOCKS; ++i ) require(alloc.block_tags[i], buddy_t::__tag_none);
  }
  end_test_case();

  test_case("TLSF rejects overflowing sizes without mutating accounting");
  {
    tlsf_t alloc({ g_tlsf_pool, TLSF_POOL_BYTES });
    const usize before = alloc.used();
    micron::__chunk<byte> a = alloc.allocate(micron::numeric_limits<usize>::max());
    micron::__chunk<byte> b = alloc.allocate(micron::numeric_limits<usize>::max() - 31u);
    require(a.ptr == nullptr, true);
    require(b.ptr == nullptr, true);
    require(alloc.used(), before);
  }
  end_test_case();

  test_case("buddy order boundaries return sufficient aligned capacity");
  {
    buddy_t alloc({ g_buddy_pool, BUDDY_POOL_BYTES }, g_buddy_tags);
    constexpr usize sizes[] = { 0, 1, 4063, 4064, 4065, 8159, 8160, 8161, 16351, 16352, 16353, 32735, 32736, 32737 };
    for ( usize sz : sizes ) {
      micron::__chunk<byte> p = alloc.allocate(sz);
      require(p.ptr != nullptr, true);
      require(p.len >= sz, true);
      require((reinterpret_cast<uintptr_t>(p.ptr) & 4095u) == 0, true);
      if ( sz ) {
        p.ptr[0] = 0x3C;
        p.ptr[sz - 1] = 0xC3;
      }
      require(alloc.deallocate(p.ptr) == abc::__flag_ok, true);
    }
    require(alloc.used(), static_cast<usize>(0));
  }
  end_test_case();

  test_case("buddy adversarial lower-address coalescing clears every interior tag");
  {
    buddy_t alloc({ g_buddy_pool, BUDDY_POOL_BYTES }, g_buddy_tags);
    for ( usize i = 0; i < BUDDY_BLOCKS; ++i ) {
      g_blocks[i] = alloc.allocate(4096 - abc::__hdr_offset);
      require(g_blocks[i].ptr != nullptr, true);
      g_blocks[i].ptr[0] = static_cast<byte>(i);
    }
    require(alloc.used(), BUDDY_POOL_BYTES);

    for ( usize i = 0; i < BUDDY_BLOCKS; i += 2 ) require(alloc.deallocate(g_blocks[i].ptr) == abc::__flag_ok, true);
    for ( usize i = 1; i < BUDDY_BLOCKS; i += 2 ) require(alloc.deallocate(g_blocks[i].ptr) == abc::__flag_ok, true);

    require(alloc.used(), static_cast<usize>(0));
    require(alloc.free_mask, u64(1) << (alloc.max_order - 1));
    require(alloc.free_lists[alloc.max_order - 1], reinterpret_cast<buddy_t::free_block *>(g_buddy_pool));
    require(alloc.free_lists[alloc.max_order - 1]->next == nullptr, true);
    require(alloc.block_tags[0], static_cast<byte>((alloc.max_order - 1) | buddy_t::__tag_free));
    for ( usize i = 1; i < BUDDY_BLOCKS; ++i ) require(alloc.block_tags[i], buddy_t::__tag_none);

    micron::__chunk<byte> whole = alloc.allocate(BUDDY_POOL_BYTES - abc::__hdr_offset);
    require(whole.ptr, g_buddy_pool);
    require(whole.len, BUDDY_POOL_BYTES - abc::__hdr_offset);
    require(alloc.deallocate(whole.ptr) == abc::__flag_ok, true);
  }
  end_test_case();

  test_case("buddy rejects overflow, misaligned frees, and repeated frees");
  {
    buddy_t alloc({ g_buddy_pool, BUDDY_POOL_BYTES }, g_buddy_tags);
    const usize before = alloc.used();
    micron::__chunk<byte> huge = alloc.allocate(micron::numeric_limits<usize>::max());
    require(huge.ptr == nullptr, true);
    require(alloc.used(), before);

    micron::__chunk<byte> p = alloc.allocate(1024);
    require(p.ptr != nullptr, true);
    require(alloc.deallocate(p.ptr + 1) == abc::__flag_invalid, true);
    require(alloc.deallocate(p.ptr) == abc::__flag_ok, true);
    require(alloc.deallocate(p.ptr) == abc::__flag_invalid, true);
    require(alloc.block_size(p.ptr + 1), static_cast<usize>(0));
    require(alloc.used(), static_cast<usize>(0));
  }
  end_test_case();

  sb::print("=== ABCMALLOC TLSF / BUDDY CORE EDGE TESTS PASSED ===");
  return 1;
}
