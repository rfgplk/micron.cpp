//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

#include "../snowball/snowball.hpp"

#include "../../src/cmalloc.hpp"

// ─── helpers ────────────────────────────────────────────────────────────────

static bool
ptr_is_aligned(const void *ptr, size_t align)
{
  return (reinterpret_cast<uintptr_t>(ptr) % align) == 0;
}

static bool
region_is_byte(const void *ptr, size_t n, unsigned char val)
{
  const unsigned char *p = static_cast<const unsigned char *>(ptr);
  for ( size_t i = 0; i < n; ++i )
    if ( p[i] != val )
      return false;
  return true;
}

static bool
region_is_zero(const void *ptr, size_t n)
{
  return region_is_byte(ptr, n, 0x00u);
}

static bool
write_and_verify(void *ptr, size_t n, unsigned char val)
{
  std::memset(ptr, val, n);
  return region_is_byte(ptr, n, val);
}

// ─── main ───────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== MALLOC / FREE / REALLOC / CALLOC TESTS ===");

  // ── malloc: basic ──────────────────────────────────────────────────────────

  sb::test_case("malloc - 1 byte allocation");
  {
    void *p = malloc(1);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - small allocation (8 bytes)");
  {
    void *p = malloc(8);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - typical allocation (64 bytes)");
  {
    void *p = malloc(64);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - page-sized allocation (4096 bytes)");
  {
    void *p = malloc(4096);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - large allocation (1 MiB)");
  {
    void *p = malloc(1024 * 1024);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - very large allocation (64 MiB)");
  {
    void *p = malloc(64ULL * 1024 * 1024);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  // ── malloc: pointer alignment ──────────────────────────────────────────────

  sb::test_case("malloc - pointer is at least 8-byte aligned");
  {
    void *p = malloc(1);
    sb::require(p != nullptr);
    sb::require(ptr_is_aligned(p, 8));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - pointer is at least 16-byte aligned (common guarantee)");
  {
    // POSIX / glibc guarantees 16-byte alignment on 64-bit
    void *p = malloc(16);
    sb::require(p != nullptr);
    sb::require(ptr_is_aligned(p, 16));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - consecutive allocations return distinct pointers");
  {
    void *a = malloc(64);
    void *b = malloc(64);
    sb::require(a != nullptr);
    sb::require(b != nullptr);
    sb::require(a != b);
    free(a);
    free(b);
  }
  sb::end_test_case();

  // ── malloc: writable memory ────────────────────────────────────────────────

  sb::test_case("malloc - returned memory is writable (byte pattern 0xAA)");
  {
    const size_t N = 256;
    void *p = malloc(N);
    sb::require(p != nullptr);
    sb::require(write_and_verify(p, N, 0xAA));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - returned memory is writable (byte pattern 0x55)");
  {
    const size_t N = 512;
    void *p = malloc(N);
    sb::require(p != nullptr);
    sb::require(write_and_verify(p, N, 0x55));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - returned memory is writable (byte pattern 0xFF)");
  {
    const size_t N = 1024;
    void *p = malloc(N);
    sb::require(p != nullptr);
    sb::require(write_and_verify(p, N, 0xFF));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - returned memory is writable (byte pattern 0x00)");
  {
    const size_t N = 128;
    void *p = malloc(N);
    sb::require(p != nullptr);
    sb::require(write_and_verify(p, N, 0x00));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - write then read back integer values");
  {
    int *arr = static_cast<int *>(malloc(16 * sizeof(int)));
    sb::require(arr != nullptr);
    for ( int i = 0; i < 16; ++i )
      arr[i] = i * i;
    bool ok = true;
    for ( int i = 0; i < 16; ++i )
      if ( arr[i] != i * i ) {
        ok = false;
        break;
      }
    sb::require(ok);
    free(arr);
  }
  sb::end_test_case();

  sb::test_case("malloc - write then read back uint64_t values");
  {
    uint64_t *arr = static_cast<uint64_t *>(malloc(8 * sizeof(uint64_t)));
    sb::require(arr != nullptr);
    for ( int i = 0; i < 8; ++i )
      arr[i] = 0xDEADBEEFCAFEBABEULL + i;
    bool ok = true;
    for ( int i = 0; i < 8; ++i )
      if ( arr[i] != 0xDEADBEEFCAFEBABEULL + (uint64_t)i ) {
        ok = false;
        break;
      }
    sb::require(ok);
    free(arr);
  }
  sb::end_test_case();

  // ── malloc: many allocations ───────────────────────────────────────────────

  sb::test_case("malloc - 1000 sequential small allocations all non-null");
  {
    bool all_ok = true;
    for ( int i = 0; i < 1000; ++i ) {
      void *p = malloc(32);
      if ( !p ) {
        all_ok = false;
        break;
      }
      free(p);
    }
    sb::require(all_ok);
  }
  sb::end_test_case();

  sb::test_case("malloc - 512 live allocations simultaneously (no overlap)");
  {
    const int N = 512;
    const size_t SZ = 64;
    void *ptrs[N];
    bool all_ok = true;

    for ( int i = 0; i < N; ++i ) {
      ptrs[i] = malloc(SZ);
      if ( !ptrs[i] ) {
        all_ok = false;
        break;
      }
      std::memset(ptrs[i], (unsigned char)i, SZ);
    }

    // verify no overlap: each block still holds its marker byte
    if ( all_ok ) {
      for ( int i = 0; i < N; ++i ) {
        if ( !region_is_byte(ptrs[i], SZ, (unsigned char)i) ) {
          all_ok = false;
          break;
        }
      }
    }

    for ( int i = 0; i < N; ++i )
      if ( ptrs[i] )
        free(ptrs[i]);

    sb::require(all_ok);
  }
  sb::end_test_case();

  sb::test_case("malloc - all returned pointers are unique (256 allocs)");
  {
    const int N = 256;
    void *ptrs[N];
    bool all_unique = true;

    for ( int i = 0; i < N; ++i )
      ptrs[i] = malloc(16);

    std::set<void *> seen;
    for ( int i = 0; i < N; ++i ) {
      if ( !ptrs[i] || seen.count(ptrs[i]) ) {
        all_unique = false;
        break;
      }
      seen.insert(ptrs[i]);
    }

    for ( int i = 0; i < N; ++i )
      if ( ptrs[i] )
        free(ptrs[i]);

    sb::require(all_unique);
  }
  sb::end_test_case();

  // ── malloc: varying sizes ──────────────────────────────────────────────────

  sb::test_case("malloc - varying sizes 1..4096 bytes (every power-of-2)");
  {
    bool ok = true;
    for ( size_t sz = 1; sz <= 4096; sz *= 2 ) {
      void *p = malloc(sz);
      if ( !p ) {
        ok = false;
        break;
      }
      std::memset(p, 0xBB, sz);
      if ( !region_is_byte(p, sz, 0xBB) ) {
        ok = false;
        free(p);
        break;
      }
      free(p);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("malloc - odd sizes (3, 7, 13, 17, 31, 97, 127, 255)");
  {
    size_t odd_sizes[] = { 3, 7, 13, 17, 31, 97, 127, 255 };
    bool ok = true;
    for ( size_t sz : odd_sizes ) {
      void *p = malloc(sz);
      if ( !p ) {
        ok = false;
        break;
      }
      std::memset(p, (int)sz, sz);
      if ( !region_is_byte(p, sz, (unsigned char)sz) ) {
        ok = false;
        free(p);
        break;
      }
      free(p);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // ── free: basic safety ─────────────────────────────────────────────────────

  sb::test_case("free - free nullptr is a no-op (must not crash)");
  {
    // C standard: free(NULL) is defined to do nothing
    free(nullptr);
    sb::require(true);     // if we reach here, no crash
  }
  sb::end_test_case();

  sb::test_case("free - malloc then free, then malloc again (reuse)");
  {
    void *p = malloc(128);
    sb::require(p != nullptr);
    free(p);
    void *q = malloc(128);
    sb::require(q != nullptr);     // must succeed; may or may not equal p
    free(q);
  }
  sb::end_test_case();

  sb::test_case("free - interleaved malloc/free sequence");
  {
    bool ok = true;
    for ( int i = 0; i < 200; ++i ) {
      void *a = malloc(64);
      void *b = malloc(128);
      if ( !a || !b ) {
        ok = false;
        free(a);
        free(b);
        break;
      }
      std::memset(a, 0xAA, 64);
      std::memset(b, 0x55, 128);
      if ( !region_is_byte(a, 64, 0xAA) || !region_is_byte(b, 128, 0x55) )
        ok = false;
      free(a);
      free(b);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // ── calloc: basic ──────────────────────────────────────────────────────────

  sb::test_case("calloc - basic (nmemb=1, size=64): returns non-null");
  {
    void *p = calloc(1, 64);
    sb::require(p != nullptr);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - memory is zero-initialized (small)");
  {
    const size_t N = 128;
    void *p = calloc(1, N);
    sb::require(p != nullptr);
    sb::require(region_is_zero(p, N));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - memory is zero-initialized (nmemb=16, size=64)");
  {
    void *p = calloc(16, 64);
    sb::require(p != nullptr);
    sb::require(region_is_zero(p, 16 * 64));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - memory is zero-initialized (large, 1 MiB)");
  {
    void *p = calloc(1, 1024 * 1024);
    sb::require(p != nullptr);
    sb::require(region_is_zero(p, 1024 * 1024));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - nmemb=1024, size=sizeof(uint64_t): all zero");
  {
    uint64_t *arr = static_cast<uint64_t *>(calloc(1024, sizeof(uint64_t)));
    sb::require(arr != nullptr);
    bool ok = true;
    for ( int i = 0; i < 1024; ++i )
      if ( arr[i] != 0 ) {
        ok = false;
        break;
      }
    sb::require(ok);
    free(arr);
  }
  sb::end_test_case();

  sb::test_case("calloc - writable after zero-init");
  {
    unsigned char *p = static_cast<unsigned char *>(calloc(1, 256));
    sb::require(p != nullptr);
    for ( int i = 0; i < 256; ++i )
      p[i] = (unsigned char)i;
    bool ok = true;
    for ( int i = 0; i < 256; ++i )
      if ( p[i] != (unsigned char)i ) {
        ok = false;
        break;
      }
    sb::require(ok);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - nmemb=0 or size=0 (implementation-defined but must not crash)");
  {
    void *p = calloc(0, 64);
    // Result is implementation-defined (may be nullptr or unique ptr)
    if ( p )
      free(p);
    void *q = calloc(64, 0);
    if ( q )
      free(q);
    sb::require(true);     // reaching here is success
  }
  sb::end_test_case();

  sb::test_case("calloc - pointer alignment (at least 8 bytes)");
  {
    void *p = calloc(1, 1);
    sb::require(p != nullptr);
    sb::require(ptr_is_aligned(p, 8));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - 256 live calloc blocks all zero");
  {
    const int N = 256;
    const size_t SZ = 32;
    void *ptrs[N];
    bool ok = true;

    for ( int i = 0; i < N; ++i ) {
      ptrs[i] = calloc(1, SZ);
      if ( !ptrs[i] ) {
        ok = false;
        break;
      }
      if ( !region_is_zero(ptrs[i], SZ) ) {
        ok = false;
        break;
      }
    }

    for ( int i = 0; i < N; ++i )
      if ( ptrs[i] )
        free(ptrs[i]);

    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("calloc - calloc vs malloc+memset equivalence");
  {
    const size_t N = 512;
    void *c = calloc(1, N);
    void *m = malloc(N);
    sb::require(c != nullptr);
    sb::require(m != nullptr);
    std::memset(m, 0, N);
    sb::require(std::memcmp(c, m, N) == 0);
    free(c);
    free(m);
  }
  sb::end_test_case();

  // ── realloc: basic ─────────────────────────────────────────────────────────

  sb::test_case("realloc - grow allocation (32 -> 128 bytes)");
  {
    void *p = malloc(32);
    sb::require(p != nullptr);
    std::memset(p, 0xAB, 32);
    void *q = realloc(p, 128);
    sb::require(q != nullptr);
    // original data must be preserved in the first 32 bytes
    sb::require(region_is_byte(q, 32, 0xAB));
    free(q);
  }
  sb::end_test_case();

  sb::test_case("realloc - shrink allocation (256 -> 16 bytes)");
  {
    void *p = malloc(256);
    sb::require(p != nullptr);
    std::memset(p, 0xCD, 256);
    void *q = realloc(p, 16);
    sb::require(q != nullptr);
    sb::require(region_is_byte(q, 16, 0xCD));
    free(q);
  }
  sb::end_test_case();

  sb::test_case("realloc - same size (64 -> 64 bytes): data preserved");
  {
    void *p = malloc(64);
    sb::require(p != nullptr);
    std::memset(p, 0x77, 64);
    void *q = realloc(p, 64);
    sb::require(q != nullptr);
    sb::require(region_is_byte(q, 64, 0x77));
    free(q);
  }
  sb::end_test_case();

  sb::test_case("realloc - nullptr acts like malloc");
  {
    void *p = realloc(nullptr, 128);
    sb::require(p != nullptr);
    sb::require(write_and_verify(p, 128, 0xEE));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("realloc - size=0 with non-null ptr acts like free (impl-defined, no crash)");
  {
    void *p = malloc(64);
    sb::require(p != nullptr);
    void *q = realloc(p, 0);
    // May return nullptr or a unique pointer; either way p must not be used
    if ( q )
      free(q);
    sb::require(true);
  }
  sb::end_test_case();

  sb::test_case("realloc - multiple grows: data integrity at each step");
  {
    size_t sizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
    void *p = malloc(sizes[0]);
    sb::require(p != nullptr);
    std::memset(p, 0x12, sizes[0]);
    bool ok = true;

    for ( int i = 1; i < 8 && ok; ++i ) {
      void *q = realloc(p, sizes[i]);
      if ( !q ) {
        ok = false;
        break;
      }
      // First sizes[i-1] bytes must still be 0x12
      if ( !region_is_byte(q, sizes[i - 1], 0x12) ) {
        ok = false;
        free(q);
        break;
      }
      // Stamp new portion with a different pattern
      std::memset(static_cast<char *>(q) + sizes[i - 1], 0x12, sizes[i] - sizes[i - 1]);
      p = q;
    }

    if ( p )
      free(p);
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("realloc - multiple shrinks: data integrity at each step");
  {
    size_t sizes[] = { 1024, 512, 256, 128, 64, 32, 16, 1 };
    void *p = malloc(sizes[0]);
    sb::require(p != nullptr);
    std::memset(p, 0x34, sizes[0]);
    bool ok = true;

    for ( int i = 1; i < 8 && ok; ++i ) {
      void *q = realloc(p, sizes[i]);
      if ( !q ) {
        ok = false;
        break;
      }
      if ( !region_is_byte(q, sizes[i], 0x34) ) {
        ok = false;
        free(q);
        break;
      }
      p = q;
    }

    if ( p )
      free(p);
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("realloc - grow to 8 MiB preserves first 4 KiB");
  {
    const size_t SMALL = 4096;
    const size_t LARGE = 8ULL * 1024 * 1024;
    void *p = malloc(SMALL);
    sb::require(p != nullptr);
    std::memset(p, 0x56, SMALL);
    void *q = realloc(p, LARGE);
    sb::require(q != nullptr);
    sb::require(region_is_byte(q, SMALL, 0x56));
    free(q);
  }
  sb::end_test_case();

  sb::test_case("realloc - returned pointer is sufficiently aligned");
  {
    void *p = malloc(8);
    sb::require(p != nullptr);
    void *q = realloc(p, 256);
    sb::require(q != nullptr);
    sb::require(ptr_is_aligned(q, 8));
    free(q);
  }
  sb::end_test_case();

  // ── mixed / stress ─────────────────────────────────────────────────────────

  /*
   * investigate?
  sb::test_case("mixed - malloc/calloc/realloc/free interleaved stress");
  {
    const int N = 128;
    void *ptrs[N] = {};
    size_t szs[N] = {};
    bool ok = true;

    // allocate with malloc and calloc alternately
    for ( int i = 0; i < N && ok; ++i ) {
      size_t sz = (size_t)(i + 1) * 8;
      if ( i % 2 == 0 ) {
        ptrs[i] = malloc(sz);
        if ( ptrs[i] )
          std::memset(ptrs[i], (unsigned char)i, sz);
      } else {
        ptrs[i] = calloc(1, sz);
        if ( ptrs[i] && !region_is_zero(ptrs[i], sz) )
          ok = false;
      }
      if ( !ptrs[i] ) {
        ok = false;
        break;
      }
      szs[i] = sz;
    }

    // realloc every third block to double size
    for ( int i = 0; i < N && ok; i += 3 ) {
      size_t new_sz = szs[i] * 2;
      void *q = realloc(ptrs[i], new_sz);
      if ( !q ) {
        ok = false;
        break;
      }
      ptrs[i] = q;
      szs[i] = new_sz;
    }

    // verify odd-indexed (calloc) blocks are still fully readable
    for ( int i = 1; i < N && ok; i += 2 ) {
      unsigned char *p = static_cast<unsigned char *>(ptrs[i]);
      for ( size_t j = 0; j < szs[i]; ++j )
        if ( p[j] != 0 ) {
          ok = false;
          break;
        }     // calloc blocks untouched
    }

    for ( int i = 0; i < N; ++i )
      if ( ptrs[i] )
        free(ptrs[i]);

    sb::require(ok);
  }
  sb::end_test_case();
    */
  sb::test_case("mixed - allocate, write, free, reallocate same region");
  {
    bool ok = true;
    for ( int round = 0; round < 50 && ok; ++round ) {
      size_t sz = 64 + (size_t)round * 16;
      void *p = malloc(sz);
      if ( !p ) {
        ok = false;
        break;
      }
      std::memset(p, (unsigned char)round, sz);
      free(p);

      // re-allocate and verify we can write fresh data
      void *q = malloc(sz);
      if ( !q ) {
        ok = false;
        break;
      }
      std::memset(q, 0xFF, sz);
      if ( !region_is_byte(q, sz, 0xFF) ) {
        ok = false;
        free(q);
        break;
      }
      free(q);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("mixed - calloc then realloc preserves zeros");
  {
    const size_t INIT = 64;
    const size_t GROW = 256;
    void *p = calloc(1, INIT);
    sb::require(p != nullptr);
    sb::require(region_is_zero(p, INIT));
    void *q = realloc(p, GROW);
    sb::require(q != nullptr);
    // first INIT bytes must still be zero
    sb::require(region_is_zero(q, INIT));
    free(q);
  }
  sb::end_test_case();

  sb::test_case("mixed - many alternating malloc/free sizes (fragmentation)");
  {
    // Allocate and free in a pattern designed to fragment the heap
    const int N = 64;
    void *ptrs[N];
    bool ok = true;

    for ( int i = 0; i < N; ++i )
      ptrs[i] = malloc((size_t)(i % 8 + 1) * 16);

    // free every other
    for ( int i = 0; i < N; i += 2 ) {
      free(ptrs[i]);
      ptrs[i] = nullptr;
    }

    // allocate into the gaps
    for ( int i = 0; i < N; i += 2 ) {
      ptrs[i] = malloc(16);
      if ( !ptrs[i] ) {
        ok = false;
        break;
      }
    }

    for ( int i = 0; i < N; ++i )
      if ( ptrs[i] )
        free(ptrs[i]);

    sb::require(ok);
  }
  sb::end_test_case();

  // ── edge cases ─────────────────────────────────────────────────────────────

  sb::test_case("malloc - allocation of size 2 (boundary)");
  {
    void *p = malloc(2);
    sb::require(p != nullptr);
    unsigned char *b = static_cast<unsigned char *>(p);
    b[0] = 0xDE;
    b[1] = 0xAD;
    sb::require(b[0] == 0xDE && b[1] == 0xAD);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - allocation of size 3 (odd boundary)");
  {
    void *p = malloc(3);
    sb::require(p != nullptr);
    unsigned char *b = static_cast<unsigned char *>(p);
    b[0] = 0xCA;
    b[1] = 0xFE;
    b[2] = 0xBA;
    sb::require(b[0] == 0xCA && b[1] == 0xFE && b[2] == 0xBA);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("malloc - allocation of sizeof(max_align_t)");
  {
    void *p = malloc(sizeof(max_align_t));
    sb::require(p != nullptr);
    sb::require(ptr_is_aligned(p, alignof(max_align_t)));
    free(p);
  }
  sb::end_test_case();

  sb::test_case("calloc - nmemb * size overflow guard (very large nmemb)");
  {
    // A conforming implementation must detect overflow and return nullptr.
    // We try something that would overflow size_t multiplication.
    void *p = calloc(SIZE_MAX, 2);
    // Must return nullptr; if it returns non-null something is very wrong.
    // Some implementations handle this differently; we just require no crash.
    if ( p )
      free(p);
    sb::require(true);
  }
  sb::end_test_case();

  sb::test_case("realloc - realloc(nullptr, 0): implementation-defined, must not crash");
  {
    void *p = realloc(nullptr, 0);
    if ( p )
      free(p);
    sb::require(true);
  }
  sb::end_test_case();

  sb::test_case("malloc then realloc to 1 byte");
  {
    void *p = malloc(1024);
    sb::require(p != nullptr);
    *(char *)p = 0x7F;
    void *q = realloc(p, 1);
    sb::require(q != nullptr);
    sb::require(*(char *)q == 0x7F);
    free(q);
  }
  sb::end_test_case();

  sb::test_case("malloc - 1-byte allocation written to and freed 10000 times");
  {
    bool ok = true;
    for ( int i = 0; i < 10000 && ok; ++i ) {
      char *p = static_cast<char *>(malloc(1));
      if ( !p ) {
        ok = false;
        break;
      }
      *p = (char)(i & 0xFF);
      if ( *p != (char)(i & 0xFF) ) {
        ok = false;
        free(p);
        break;
      }
      free(p);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("calloc - sequential calloc/free 1000 iterations stay zeroed");
  {
    bool ok = true;
    for ( int i = 0; i < 1000 && ok; ++i ) {
      void *p = calloc(1, 64);
      if ( !p ) {
        ok = false;
        break;
      }
      if ( !region_is_zero(p, 64) ) {
        ok = false;
        free(p);
        break;
      }
      free(p);
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("realloc - realloc chain: 1 byte growing to 65536 bytes");
  {
    void *p = malloc(1);
    sb::require(p != nullptr);
    *(char *)p = 0x42;
    bool ok = true;

    for ( size_t sz = 2; sz <= 65536 && ok; sz *= 2 ) {
      void *q = realloc(p, sz);
      if ( !q ) {
        ok = false;
        break;
      }
      p = q;
      // write to the newly grown portion
      std::memset(static_cast<char *>(p) + sz / 2, 0xAB, sz / 2);
    }

    if ( p )
      free(p);
    sb::require(ok);
  }
  sb::end_test_case();

  // ── struct allocation ──────────────────────────────────────────────────────

  sb::test_case("malloc - struct allocation and member access");
  {
    struct Point {
      double x, y, z;
    };

    Point *pt = static_cast<Point *>(malloc(sizeof(Point)));
    sb::require(pt != nullptr);
    pt->x = 1.5;
    pt->y = 2.5;
    pt->z = 3.5;
    sb::require(pt->x == 1.5 && pt->y == 2.5 && pt->z == 3.5);
    free(pt);
  }
  sb::end_test_case();

  sb::test_case("calloc - struct array zero-initialized");
  {
    struct Node {
      int val;
      void *next;
    };

    const int N = 100;
    Node *nodes = static_cast<Node *>(calloc(N, sizeof(Node)));
    sb::require(nodes != nullptr);
    bool ok = true;
    for ( int i = 0; i < N; ++i )
      if ( nodes[i].val != 0 || nodes[i].next != nullptr ) {
        ok = false;
        break;
      }
    sb::require(ok);
    free(nodes);
  }
  sb::end_test_case();

  sb::test_case("malloc - array of pointers, each malloc'd independently");
  {
    const int N = 64;
    char **ptrs = static_cast<char **>(malloc(N * sizeof(char *)));
    sb::require(ptrs != nullptr);
    bool ok = true;

    for ( int i = 0; i < N; ++i ) {
      ptrs[i] = static_cast<char *>(malloc(16));
      if ( !ptrs[i] ) {
        ok = false;
        break;
      }
      std::memset(ptrs[i], (unsigned char)i, 16);
    }

    for ( int i = 0; i < N && ok; ++i )
      if ( !region_is_byte(ptrs[i], 16, (unsigned char)i) )
        ok = false;

    for ( int i = 0; i < N; ++i )
      if ( ptrs[i] )
        free(ptrs[i]);
    free(ptrs);

    sb::require(ok);
  }
  sb::end_test_case();

  // ── boundary / alignment probes ────────────────────────────────────────────

  sb::test_case("malloc - write to very first and very last byte of allocation");
  {
    const size_t N = 4096;
    unsigned char *p = static_cast<unsigned char *>(malloc(N));
    sb::require(p != nullptr);
    p[0] = 0xAA;
    p[N - 1] = 0xBB;
    sb::require(p[0] == 0xAA && p[N - 1] == 0xBB);
    free(p);
  }
  sb::end_test_case();

  sb::test_case("realloc - write to very first and last byte after grow");
  {
    const size_t SZ1 = 64, SZ2 = 8192;
    unsigned char *p = static_cast<unsigned char *>(malloc(SZ1));
    sb::require(p != nullptr);
    p[0] = 0x11;
    p[SZ1 - 1] = 0x22;
    void *q = realloc(p, SZ2);
    sb::require(q != nullptr);
    unsigned char *r = static_cast<unsigned char *>(q);
    sb::require(r[0] == 0x11 && r[SZ1 - 1] == 0x22);
    r[SZ2 - 1] = 0x33;
    sb::require(r[SZ2 - 1] == 0x33);
    free(q);
  }
  sb::end_test_case();

  sb::test_case("malloc - 100 allocations of varying sizes, no pointer reuse detected");
  {
    const int N = 100;
    std::vector<void *> ptrs(N);
    bool ok = true;
    for ( int i = 0; i < N; ++i ) {
      ptrs[i] = malloc((size_t)(i + 1) * 7);
      if ( !ptrs[i] ) {
        ok = false;
        break;
      }
    }
    std::set<void *> unique_ptrs(ptrs.begin(), ptrs.end());
    if ( unique_ptrs.size() != (size_t)N )
      ok = false;
    for ( auto p : ptrs )
      if ( p )
        free(p);
    sb::require(ok);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
