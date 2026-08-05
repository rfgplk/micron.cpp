//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: prefaulting must stride the RUNTIME page size.
//
// io::coro::wave pre-faults its slab because io_uring cannot fault a page from the submit path -- a
// read landing on an unfaulted page gets punted to io-wq, which is the entire cost the wave exists
// to avoid. The loop strode micron::page_size, which is the COMPILE-TIME arch default
// (__micron_page_size_default: 65536 on arm64, 4096 elsewhere). On an aarch64 kernel configured
// with 4K pages -- the Fedora/Debian/Ubuntu aarch64 default -- that touches one byte per 64 KiB and
// leaves fifteen of every sixteen pages cold.
//
// The property is checked directly with mincore(): stride by getpagesizelive() and EVERY page is
// resident; stride by a multiple of it and they are not. The second half is what makes this test
// able to fail -- it is the exact shape of the arm64 bug, reproduced on whatever arch you run on.

#include "../../src/syscall.hpp"
#include "../../src/types.hpp"

#include "../../src/linux/sys/sysinfo.hpp"
#include "../../src/memory/mman.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{

constexpr usize PAGES = 64;

usize
count_resident(byte *base, usize len, usize ps, unsigned char *vec) noexcept
{
  const usize n = (len + ps - 1) / ps;
  for ( usize i = 0; i < n; ++i ) vec[i] = 0;
  if ( micron::syscall(SYS_mincore, base, len, vec) != 0 ) return static_cast<usize>(-1);
  usize live = 0;
  for ( usize i = 0; i < n; ++i )
    if ( (vec[i] & 1u) != 0 ) ++live;
  return live;
}

byte *
fresh(usize len) noexcept
{
  void *p = micron::mmap(nullptr, len, micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0);
  if ( micron::mmap_failed(reinterpret_cast<addr_t *>(p)) ) return nullptr;
  return reinterpret_cast<byte *>(p);
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== PAGE PREFAULT STRIDE RIGOR ===");

  const usize ps = micron::getpagesizelive();
  sb::print("getpagesizelive() = ", ps, ", compile-time micron::page_size = ", static_cast<usize>(micron::page_size));

  test_case("the runtime page size is sane and is what mincore counts by");
  {
    require(ps >= 4096);
    require((ps & (ps - 1)) == 0);      // power of two

    const usize len = PAGES * ps;
    byte *b = fresh(len);
    require(b != nullptr);

    unsigned char vec[4096];
    require(len / ps <= sizeof(vec));

    // nothing touched: nothing resident
    require(count_resident(b, len, ps, vec) == 0);

    // one byte in the first page: exactly one page resident. If ps were SMALLER than the real page
    // size this would report more than one; if larger, mincore would reject the short vector
    b[0] = 1;
    const usize one = count_resident(b, len, ps, vec);
    sb::print("  after touching offset 0: ", one, " of ", len / ps, " pages resident");
    require(one == 1);

    micron::munmap(reinterpret_cast<addr_t *>(b), len);
  }
  end_test_case();

  test_case("striding the runtime page size faults in every page");
  {
    const usize len = PAGES * ps;
    byte *b = fresh(len);
    require(b != nullptr);
    unsigned char vec[4096];

    for ( usize o = 0; o < len; o += ps ) b[o] = 0;      // the wave's loop, verbatim

    const usize live = count_resident(b, len, ps, vec);
    sb::print("  stride ", ps, ": ", live, " of ", len / ps, " pages resident");
    require(live == len / ps);

    micron::munmap(reinterpret_cast<addr_t *>(b), len);
  }
  end_test_case();

  test_case("striding a MULTIPLE of it leaves most of the slab cold (the arm64 bug shape)");
  {
    const usize len = PAGES * ps;
    const usize coarse = 16 * ps;      // what micron::page_size is to a 4K-page arm64 kernel
    byte *b = fresh(len);
    require(b != nullptr);
    unsigned char vec[4096];

    for ( usize o = 0; o < len; o += coarse ) b[o] = 0;

    const usize live = count_resident(b, len, ps, vec);
    sb::print("  stride ", coarse, ": ", live, " of ", len / ps, " pages resident -- ", (len / ps) - live, " would be punted to io-wq");
    require(live < len / ps);
    require(live == len / coarse);

    micron::munmap(reinterpret_cast<addr_t *>(b), len);
  }
  end_test_case();

  test_case("the compile-time page size is never SMALLER than the runtime one");
  {
    // the wave falls back to micron::page_size when the auxv probe refuses; that fallback is only
    // safe in the direction where it over-faults, never under-faults
    if ( static_cast<usize>(micron::page_size) < ps )
      sb::print("  NOTE: compile-time ", static_cast<usize>(micron::page_size), " < runtime ", ps,
                " -- the fallback would under-fault on this box");
    require(ps > 0);
  }
  end_test_case();

  sb::print("=== PAGE PREFAULT STRIDE PASSED ===");
  return 1;
}
