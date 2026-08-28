

#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

extern "C" {
void *aligned_alloc(usize, usize) noexcept;
int posix_memalign(void **, usize, usize) noexcept;
void *memalign(usize, usize) noexcept;
void *valloc(usize) noexcept;
void *pvalloc(usize) noexcept;
}

namespace
{

constexpr usize alignments[] = { 8, 16, 32, 64, 128, 256, 512, 1024, 4096 };
constexpr usize sizes[] = { 1, 7, 32, 100, 333, 4096, 65536 };

[[nodiscard]] bool
is_aligned(const void *p, usize a) noexcept
{
  return (reinterpret_cast<uintptr_t>(p) & (a - 1)) == 0;
}

void
stamp(void *p, usize n, byte v) noexcept
{
  byte *b = reinterpret_cast<byte *>(p);
  for ( usize i = 0; i < n; ++i ) b[i] = static_cast<byte>(v + static_cast<byte>(i & 0xFFu));
}

[[nodiscard]] bool
stamped(const void *p, usize n, byte v) noexcept
{
  const byte *b = reinterpret_cast<const byte *>(p);
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != static_cast<byte>(v + static_cast<byte>(i & 0xFFu)) ) return false;
  return true;
}

[[nodiscard]] byte *
block_of(void *p) noexcept
{
  byte *const raw = abc::__aligned_base_of(reinterpret_cast<byte *>(p));
  return raw != nullptr ? raw : reinterpret_cast<byte *>(p);
}

static byte g_static_bytes[64]{};

};      // namespace

int
main()
{
  using namespace snowball;

  test_case("the C aligned family returns aligned, fully writable memory");
  {
    for ( usize a : alignments ) {
      for ( usize n : sizes ) {
        void *p = nullptr;
        require(posix_memalign(&p, a, n) == 0);
        require(p != nullptr);
        require(is_aligned(p, a));
        stamp(p, n, 0x11u);
        require(stamped(p, n, 0x11u));
        free(p);

        void *m = memalign(a, n);
        require(m != nullptr);
        require(is_aligned(m, a));
        stamp(m, n, 0x22u);
        require(stamped(m, n, 0x22u));
        free(m);

        void *c = aligned_alloc(a, n);
        require(c != nullptr);
        require(is_aligned(c, a));
        stamp(c, n, 0x33u);
        require(stamped(c, n, 0x33u));
        free(c);
      }
    }

    void *v = valloc(1000);
    require(v != nullptr);
    require(is_aligned(v, abc::__system_pagesize));
    stamp(v, 1000, 0x44u);
    require(stamped(v, 1000, 0x44u));
    free(v);

    void *pv = pvalloc(1000);
    require(pv != nullptr);
    require(is_aligned(pv, abc::__system_pagesize));
    stamp(pv, 1000, 0x55u);
    require(stamped(pv, 1000, 0x55u));
    free(pv);
  }
  end_test_case();

  test_case("a plain free() releases the block behind an over-aligned pointer");
  {
    usize over_aligned = 0;
    for ( usize a : alignments ) {
      for ( usize n : sizes ) {
        void *p = nullptr;
        require(posix_memalign(&p, a, n) == 0);
        byte *const base = block_of(p);
        if ( base != reinterpret_cast<byte *>(p) ) ++over_aligned;
        require(abc::is_present(base));
        free(p);
        require(!abc::is_present(base));
      }
    }

    require(over_aligned != 0);
    sb::print("  over-aligned (interior-pointer) cases exercised: ", over_aligned, "\n");
  }
  end_test_case();

  test_case("ten thousand aligned alloc/free pairs do not grow the heap");
  {
    constexpr usize reps = 10000;
    void *warm = nullptr;
    require(posix_memalign(&warm, 4096, 8192) == 0);
    free(warm);

    const usize before = abc::musage();
    for ( usize i = 0; i < reps; ++i ) {
      void *p = nullptr;
      require(posix_memalign(&p, 4096, 8192) == 0);
      require(p != nullptr);
      if ( p == nullptr ) return 0;
      stamp(p, 64, static_cast<byte>(i));
      free(p);
    }
    const usize after = abc::musage();
    sb::print("  musage before ", before, " after ", after, " over ", reps, " x 8 KiB at 4096-byte alignment\n");

    require(after <= before + (1ULL << 20));
  }
  end_test_case();

  test_case("realloc on a posix_memalign pointer preserves the contents");
  {
    void *p = nullptr;
    require(posix_memalign(&p, 256, 400) == 0);
    stamp(p, 400, 0x66u);
    byte *const base = block_of(p);

    void *grown = realloc(p, 1200);
    require(grown != nullptr);
    require(stamped(grown, 400, 0x66u));
    require(!abc::is_present(base));

    void *shrunk = realloc(grown, 64);
    require(shrunk != nullptr);
    require(stamped(shrunk, 64, 0x66u));
    free(shrunk);
  }
  end_test_case();

  test_case("__aligned_base_of refuses everything that is not an over-aligned pointer");
  {
    byte stack_bytes[64]{};
    require(abc::__aligned_base_of(stack_bytes + 32) == nullptr);
    require(abc::__aligned_base_of(g_static_bytes + 32) == nullptr);
    require(abc::__aligned_base_of(nullptr) == nullptr);

    void *plain = malloc(4096);
    require(plain != nullptr);
    require(abc::__aligned_base_of(reinterpret_cast<byte *>(plain)) == nullptr);
    free(plain);

    void *small = nullptr;
    require(posix_memalign(&small, sizeof(void *), 64) == 0);
    require(abc::__aligned_base_of(reinterpret_cast<byte *>(small)) == nullptr);
    free(small);

    void *bad = reinterpret_cast<void *>(static_cast<uintptr_t>(1));
    require(posix_memalign(&bad, 24, 64) != 0);
    require(posix_memalign(&bad, 2, 64) != 0);
    require(posix_memalign(nullptr, 64, 64) != 0);
  }
  end_test_case();

  test_case("a prefix copied to another address stops validating");
  {
    void *p = nullptr;
    require(posix_memalign(&p, 4096, 4096) == 0);
    byte *const ptr = reinterpret_cast<byte *>(p);
    require(abc::__aligned_base_of(ptr) != nullptr);

    const abc::__aligned_prefix good = *reinterpret_cast<const abc::__aligned_prefix *>(ptr - sizeof(abc::__aligned_prefix));

    byte *const elsewhere = ptr + 64;
    abc::__aligned_prefix saved = *reinterpret_cast<const abc::__aligned_prefix *>(elsewhere - sizeof(abc::__aligned_prefix));
    *reinterpret_cast<abc::__aligned_prefix *>(elsewhere - sizeof(abc::__aligned_prefix)) = good;
    require(abc::__aligned_base_of(elsewhere) == nullptr);
    *reinterpret_cast<abc::__aligned_prefix *>(elsewhere - sizeof(abc::__aligned_prefix)) = saved;

    require(abc::__aligned_base_of(ptr) != nullptr);
    free(p);
  }
  end_test_case();

  sb::print("=== ABCMALLOC C-ABI ALIGNED FAMILY PASSED ===\n");
  return 1;
}
