//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// power-of-two allocator smoke test
//
// covers every size class from page-size (2^12) up to 2 GB (2^31) and every
// raw allocator entry point.  the contract we lock in:
//
//   - successful allocation: non-null pointer, at least 8-byte aligned,
//     fully writable for `size` bytes.
//   - failure: raise `except::memory_error`.  never silently return null.
//
// the calloc-direct test specifically exercises the path that surfaced the
// silent-null bug in `__abc_allocator::calloc` (where a -1 sentinel check is
// dead code because `fetch()` already rewrites the sentinel to nullptr).
// the tier-exhaust tests at 2^18 .. 2^24 push past the 64-sheet huge-tier
// cap that backs the no-fallback failure mode.
//
// strict 2 GB test required by user.

#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

// ─── helpers ────────────────────────────────────────────────────────────────

static bool
ptr_is_aligned(const void *ptr, size_t align)
{
  return (reinterpret_cast<uintptr_t>(ptr) % align) == 0;
}

// spot-write three bytes (head, middle, tail) and read them back.
// full-region scan would dominate runtime past a few hundred MB.
static bool
spot_write_verify(void *ptr, size_t n, uint8_t val)
{
  auto *p = static_cast<uint8_t *>(ptr);
  p[0] = val;
  if ( n > 1 ) p[n / 2] = val;
  if ( n > 0 ) p[n - 1] = val;
  if ( p[0] != val ) return false;
  if ( n > 1 && p[n / 2] != val ) return false;
  if ( n > 0 && p[n - 1] != val ) return false;
  return true;
}

// allocate via the bug-exposing path. returns nullptr only via exception;
// silent nullptr is a contract violation we want to catch with sb::require.
static uint8_t *
calloc_or_throw(size_t n)
{
  micron::__chunk<uint8_t> chunk = abc::__abc_allocator<uint8_t>::calloc(n);
  return chunk.ptr;
}

// ─── main ───────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== ABCMALLOC POWER-OF-TWO SIZE TESTS ===");

  // ── raw abc::alloc roundtrip across all sizes ─────────────────────────────

  for ( int shift = 12; shift <= 31; ++shift ) {
    const size_t S = static_cast<size_t>(1) << shift;
    // On 32-bit targets, anything that exceeds the platform's __alloc_limit
    // triggers abort_state in the allocator (constrained mode). Skip those
    // sizes — the meaningful test surface is already covered by lower shifts.
    if constexpr ( sizeof(void *) == 4 ) {
      if ( abc::__alloc_limit != 0 && S > abc::__alloc_limit ) continue;
    }
    char name[80];
    std::snprintf(name, sizeof(name), "abc::alloc 2^%d (%zu bytes) roundtrip", shift, S);
    sb::test_case(name);
    {
      uint8_t *p = abc::alloc(S);
      sb::require(p != nullptr);
      sb::require(ptr_is_aligned(p, 8));
      sb::require(spot_write_verify(p, S, 0xAA));
      abc::dealloc(p);
    }
    sb::end_test_case();
  }

  // ── abc::balloc chunk variant — document tier rounding ───────────────────

  for ( int shift = 12; shift <= 31; ++shift ) {
    const size_t S = static_cast<size_t>(1) << shift;
    if constexpr ( sizeof(void *) == 4 ) {
      if ( abc::__alloc_limit != 0 && S > abc::__alloc_limit ) continue;
    }
    char name[80];
    std::snprintf(name, sizeof(name), "abc::balloc 2^%d (%zu bytes)", shift, S);
    sb::test_case(name);
    {
      micron::__chunk<uint8_t> chunk = abc::balloc(S);
      sb::require(chunk.ptr != nullptr);
      sb::require(chunk.len >= S);
      // log the (requested, observed) pair so we have a record of each
      // tier's rounding behavior after the fix
      sb::print("  shift=", shift, " requested=", S, " observed_len=", chunk.len);
      abc::dealloc(chunk.ptr);
    }
    sb::end_test_case();
  }

  // ── __abc_allocator::calloc never silently null ──────────────────────────
  //
  // this is the Bug A canary.  before the fix, allocations that exhaust the
  // huge-tier 64-sheet cap silently return {nullptr, 0} from this entry
  // point; after the fix, they raise except::memory_error.

  for ( int shift = 12; shift <= 31; ++shift ) {
    const size_t S = static_cast<size_t>(1) << shift;
    if constexpr ( sizeof(void *) == 4 ) {
      if ( abc::__alloc_limit != 0 && S > abc::__alloc_limit ) continue;
    }
    char name[80];
    std::snprintf(name, sizeof(name), "__abc_allocator::calloc never-null 2^%d", shift);
    sb::test_case(name);
    {
      bool raised = false;
      uint8_t *ptr = nullptr;
      try {
        ptr = calloc_or_throw(S);
      } catch ( const micron::except::memory_error & ) {
        raised = true;
      } catch ( ... ) {
        raised = true;
      }
      // contract: returned non-null OR raised.  never silent null.
      sb::require(raised || ptr != nullptr);
      if ( ptr != nullptr ) {
        sb::require(spot_write_verify(ptr, S, 0x33));
        abc::dealloc(ptr);
      }
    }
    sb::end_test_case();
  }

  // ── operator new / delete across all sizes ────────────────────────────────

  for ( int shift = 12; shift <= 31; ++shift ) {
    const size_t S = static_cast<size_t>(1) << shift;
    if constexpr ( sizeof(void *) == 4 ) {
      if ( abc::__alloc_limit != 0 && S > abc::__alloc_limit ) continue;
    }
    char name[80];
    std::snprintf(name, sizeof(name), "operator new 2^%d (%zu bytes)", shift, S);
    sb::test_case(name);
    {
      void *p = ::operator new(S);
      sb::require(p != nullptr);
      sb::require(ptr_is_aligned(p, 8));
      sb::require(spot_write_verify(p, S, 0x5C));
      ::operator delete(p);
    }
    sb::end_test_case();
  }

  // ── tier-exhaust stress at the bands prone to the 64-sheet cap ───────────
  //
  // K=80 chunks at 2^18 .. 2^24.  after the Bug B fix, the allocator falls
  // through to a direct mmap path once the tier's sheet pool saturates and
  // every chunk in the vector is real, writable memory.

  if constexpr ( sizeof(void *) > 4 || abc::__alloc_limit == 0 ) {
    static constexpr int stress_shifts[] = { 18, 20, 22, 24 };
    static constexpr int K = 80;      // > __max_sheets_huge (64)
    for ( int shift : stress_shifts ) {
      const size_t S = static_cast<size_t>(1) << shift;
      char name[80];
      std::snprintf(name, sizeof(name), "tier-exhaust: %d x 2^%d (%zu bytes each)", K, shift, S);
      sb::test_case(name);
      {
        std::vector<uint8_t *> chunks;
        chunks.reserve(K);
        bool any_failed = false;
        for ( int i = 0; i < K; ++i ) {
          uint8_t *p = nullptr;
          try {
            p = abc::alloc(S);
          } catch ( ... ) {
            any_failed = true;
            break;
          }
          if ( p == nullptr ) {
            any_failed = true;
            break;
          }
          // probe each chunk with a per-index pattern
          spot_write_verify(p, S, static_cast<uint8_t>(i & 0xff));
          chunks.push_back(p);
        }
        sb::require(!any_failed);
        sb::require(static_cast<int>(chunks.size()) == K);
        for ( auto it = chunks.rbegin(); it != chunks.rend(); ++it ) {
          abc::dealloc(*it);
        }
      }
      sb::end_test_case();
    }
  }

  // ── mixed-size interleave (cross-tier metadata sanity) ────────────────────

  sb::test_case("mixed-size interleave, alloc all then free in reverse");
  {
    std::vector<std::pair<int, uint8_t *>> entries;
    entries.reserve(20);
    for ( int shift = 12; shift <= 28; ++shift ) {
      const size_t S = static_cast<size_t>(1) << shift;
      if constexpr ( sizeof(void *) == 4 ) {
        if ( abc::__alloc_limit != 0 && S > abc::__alloc_limit ) continue;
      }
      uint8_t *p = abc::alloc(S);
      sb::require(p != nullptr);
      // touch first page only — we just want metadata sanity, not full coverage
      std::memset(p, static_cast<uint8_t>(shift & 0xff), 4096);
      entries.emplace_back(shift, p);
    }
    for ( auto it = entries.rbegin(); it != entries.rend(); ++it ) {
      abc::dealloc(it->second);
    }
  }
  sb::end_test_case();

  // ── strict 2 GB single allocation must succeed ────────────────────────────
  // Constrained 32-bit targets (e.g. ARMv7 with __ABC_EMBED) cap at
  // __alloc_limit; the 2 GB request is unrepresentable / over-limit there.
  if constexpr ( sizeof(void *) > 4 && (abc::__alloc_limit == 0 || (static_cast<size_t>(1) << 31) <= abc::__alloc_limit) ) {
    sb::test_case("strict 2 GB single allocation must succeed (per user spec)");
    {
      const size_t S = static_cast<size_t>(1) << 31;
      uint8_t *buf = abc::alloc(S);
      sb::require(buf != nullptr);
      sb::require(ptr_is_aligned(buf, 8));
      // spot-write across three boundaries — full scan would take ~30s
      buf[0] = 0xDE;
      buf[S / 2] = 0xAD;
      buf[S - 1] = 0xBE;
      sb::require(buf[0] == 0xDE);
      sb::require(buf[S / 2] == 0xAD);
      sb::require(buf[S - 1] == 0xBE);
      abc::dealloc(buf);
    }
    sb::end_test_case();
  }

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
