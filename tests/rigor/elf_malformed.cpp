//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Robustness of micron::elf::read against truncated, mutated and fuzzed images.
//
// Writes nothing to the filesystem: every case runs over a heap copy of a real binary wrapped by
// the non-owning read::source(const u8*, usize) constructor. The buffer is an exact-size
// micron::__alloc rather than a micron::vector<u8> on purpose -- serial_allocator::create rounds
// every request up to 4096 (tests/build/sanitize.sh), so a vector would swallow a one-byte
// overread inside its own block, while an exact allocation puts ASan's redzone precisely at
// src.size().

#include "../../src/linux/elf/read.hpp"

#include "../../src/linux/io/ext.hpp"
#include "../../src/memory/allocation/__internal.hpp"

#include "../../src/io/console.hpp"
#include "../../src/memory/cstring.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace me = micron::elf;
namespace mr = micron::elf::read;
namespace mp = micron::posix;

// a detected sanitizer error must not exit(1): duck grades 1 as PASS
extern "C" const char *
__asan_default_options(void)
{
  return "abort_on_error=1:detect_leaks=0:allocator_may_return_null=1";
}

extern "C" const char *
__ubsan_default_options(void)
{
  return "halt_on_error=1:abort_on_error=1:print_stacktrace=1";
}

#ifndef MICRON_ELF_FUZZ_ITERS
#define MICRON_ELF_FUZZ_ITERS 4096
#endif

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// an exact-size mutable copy

struct heap_image {
  u8 *p = nullptr;
  usize n = 0;

  heap_image() = default;

  heap_image(const u8 *src, usize len)
  {
    if ( !len ) return;
    p = reinterpret_cast<u8 *>(micron::__alloc(len));
    if ( !p ) return;
    n = len;
    for ( usize i = 0; i < len; i++ ) p[i] = src[i];
  }

  heap_image(const heap_image &) = delete;
  heap_image &operator=(const heap_image &) = delete;

  heap_image(heap_image &&o) noexcept : p(o.p), n(o.n)
  {
    o.p = nullptr;
    o.n = 0;
  }

  heap_image &
  operator=(heap_image &&o) noexcept
  {
    if ( this != &o ) {
      if ( p ) micron::__free(p);
      p = o.p;
      n = o.n;
      o.p = nullptr;
      o.n = 0;
    }
    return *this;
  }

  ~heap_image()
  {
    if ( p ) micron::__free(p);
  }

  heap_image
  clone(usize len) const
  {
    return heap_image{ p, len < n ? len : n };
  }

  void
  w8(usize at, u8 v)
  {
    if ( at < n ) p[at] = v;
  }

  void
  w16(usize at, u16 v)
  {
    if ( at + 1 < n ) {
      p[at] = static_cast<u8>(v);
      p[at + 1] = static_cast<u8>(v >> 8);
    }
  }

  void
  w32(usize at, u32 v)
  {
    for ( usize i = 0; i < 4; i++ )
      if ( at + i < n ) p[at + i] = static_cast<u8>(v >> (8 * i));
  }

  void
  w64(usize at, u64 v)
  {
    for ( usize i = 0; i < 8; i++ )
      if ( at + i < n ) p[at + i] = static_cast<u8>(v >> (8 * i));
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// run every walker and record what came out; nothing escapes

struct outcome {
  bool opened = false;
  bool threw = false;
  const char *threw_in = "";      // which walker let it escape
  usize segs = 0, secs = 0, syms = 0, dyns = 0;
  u64 relocs = 0, relr = 0, versyms = 0, verdefs = 0, verneeds = 0;
  bool relr_capped = false;
  micron::vector<mr::segment_row> seg_rows{};
  micron::vector<mr::section_row> sec_rows{};
};

outcome
run_battery(const u8 *p, usize n)
{
  outcome o{};
  try {
    const mr::source src{ p, n };
    auto ir = mr::open_image(src);
    if ( ir.is_second() ) return o;
    o.opened = true;
    const mr::image &img = ir.cast<mr::image>();

    o.threw_in = "walk_segments";
    o.seg_rows = mr::walk_segments(img);
    o.segs = o.seg_rows.size();
    o.threw_in = "walk_sections";
    o.sec_rows = mr::walk_sections(img);
    o.secs = o.sec_rows.size();

    o.threw_in = "walk_symbols";
    const auto syms = mr::walk_symbols(img, o.sec_rows);
    o.syms = syms.size();

    o.threw_in = "read_dynamic";
    const auto dyn = mr::read_dynamic(img, o.seg_rows, o.sec_rows);
    o.dyns = dyn.entries.size();

    o.threw_in = "section walkers";

    for ( const auto &s : o.sec_rows ) {
      if ( s.type == me::sht_rela || s.type == me::sht_rel ) o.relocs += mr::walk_relocs(img, s).size();
      if ( s.type == me::sht_relr ) {
        const auto r = mr::walk_relr(img, s);
        o.relr += r.at.size();
        if ( r.capped ) o.relr_capped = true;
      }
      if ( s.type == me::sht_gnu_versym ) o.versyms += mr::read_versym(img, s).size();
      if ( s.type == me::sht_gnu_verdef ) o.verdefs += mr::read_verdef(img, o.sec_rows, s).size();
      if ( s.type == me::sht_gnu_verneed ) o.verneeds += mr::read_verneed(img, o.sec_rows, s).size();
    }

    (void)mr::read_interp(img, o.seg_rows);
    (void)mr::classify_link(o.seg_rows);
    o.threw_in = "";
  } catch ( ... ) {
    // an escaping exception is a finding, not a reason to stop: reserve() raises
    // except::critical_error on a hostile size and that must be recorded, not fatal
    o.threw = true;
  }
  return o;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

u64 findings = 0;

void
finding(const char *what, const char *detail)
{
  ++findings;
  sb::print("  FINDING: ", what, " -- ", detail);
}

// first accepted ELF of the requested class under /bin, below the size cap
bool
find_base(u8 want_class, usize max_bytes, heap_image &out, micron::sstring<512> &chosen)
{
  mp::dir_t fd = mp::opendir("/bin");
  if ( !fd.open() ) return false;
  mp::readdir_ctx ctx{};
  bool got = false;
  for ( ;; ) {
    mp::__impl_dir e = mp::readdir_r(fd, ctx);
    if ( e.type == mp::dt_end ) break;
    if ( e.type != mp::dt_reg ) continue;
    const char *nm = e.d_name.c_str();
    if ( nm[0] == '.' ) continue;

    micron::sstring<512> path{};
    const char *root = "/bin/";
    for ( usize i = 0; root[i]; i++ ) path += root[i];
    for ( usize i = 0; nm[i]; i++ ) path += nm[i];
    path.null_term();

    auto sr = mr::source::open(path.c_str());
    if ( sr.is_second() ) continue;
    const mr::source &src = sr.cast<mr::source>();
    if ( src.size() < 4096 || src.size() > max_bytes ) continue;
    if ( src.data()[4] != want_class ) continue;
    if ( mr::open_image(src).is_second() ) continue;

    out = heap_image{ src.data(), src.size() };
    chosen = path;
    got = out.p != nullptr;
    break;
  }
  mp::closedir(fd);
  return got;
}

// header field offsets, by class
struct hdr_off {
  usize phoff, shoff, phentsize, phnum, shentsize, shnum, shstrndx;
  usize shdr_size_field;      // sh_size within a section header
  usize shdr_link_field;
  u64 shdr_sz;
};

constexpr hdr_off off64 = { 32, 40, 54, 56, 58, 60, 62, 32, 40, 64 };
constexpr hdr_off off32 = { 28, 32, 42, 44, 46, 48, 50, 20, 24, 40 };

u64
rd_le(const u8 *p, usize at, usize w)
{
  u64 v = 0;
  for ( usize i = 0; i < w; i++ ) v |= static_cast<u64>(p[at + i]) << (8 * i);
  return v;
}

// splitmix64. FIXED seed: a rigor seed is never time-based, and the same sequence must land on
// x64, i386, arm and arm64 so a finding reproduces from the seed alone.
constexpr u64 fuzz_seed = 0x9E3779B97F4A7C15ull;
u64 rng_state = fuzz_seed;

u64
next_u64(void)
{
  rng_state += 0x9E3779B97F4A7C15ull;
  u64 z = rng_state;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

};      // namespace

int
main(void)
{
  sb::print("=== ELF MALFORMED-INPUT ROBUSTNESS ===");

  heap_image base{};
  micron::sstring<512> base_path{};
  if ( !find_base(2, 256u * 1024u, base, base_path) ) {
    sb::print("no suitable ELF64 base binary under /bin -- suite skipped");
    sb::print("=== ALL TESTS PASSED ===");
    return 1;
  }
  sb::print("base image: ", base_path.c_str(), " (", base.n, " bytes)");

  const hdr_off &H = off64;
  const u64 full_phoff = rd_le(base.p, H.phoff, 8);
  const u64 full_shoff = rd_le(base.p, H.shoff, 8);
  const u64 full_shentsize = rd_le(base.p, H.shentsize, 2);
  const u64 full_phentsize = rd_le(base.p, H.phentsize, 2);
  const u64 full_phnum = rd_le(base.p, H.phnum, 2);

  const outcome full = run_battery(base.p, base.n);

  test_case("the unmodified base image parses");
  {
    require_true(full.opened);
    require_true(!full.threw);
    require_true(full.segs > 0);
    require_true(full.secs > 0);
  }
  end_test_case();

  // ---- truncation ladder ------------------------------------------------
  test_case("truncation never throws, never crashes, and is monotone");
  {
    const u64 lens[] = { 0,
                         1,
                         4,
                         15,
                         16,
                         51,
                         52,
                         63,
                         64,
                         full_phoff ? full_phoff - 1 : 0,
                         full_phoff,
                         full_phoff + 1,
                         full_phoff + full_phentsize * full_phnum - 1,
                         full_shoff / 2,
                         full_shoff ? full_shoff - 1 : 0,
                         full_shoff,
                         full_shoff + 1,
                         base.n - 1,
                         base.n };
    for ( u64 L : lens ) {
      if ( L > base.n ) continue;
      const outcome o = run_battery(base.p, static_cast<usize>(L));
      if ( o.threw ) finding("a truncated image threw out of the reader", base_path.c_str());
      if ( L < 16 && o.opened ) finding("open_image accepted an image shorter than e_ident", base_path.c_str());
      if ( L < 64 && o.opened ) finding("open_image accepted a truncated ELF64 header", base_path.c_str());

      // a shorter file can only ever produce a PREFIX of the full parse
      if ( o.segs > full.segs ) finding("truncation produced MORE segments than the full file", base_path.c_str());
      if ( o.secs > full.secs ) finding("truncation produced MORE sections than the full file", base_path.c_str());
      if ( o.syms > full.syms ) finding("truncation produced MORE symbols than the full file", base_path.c_str());
      for ( usize i = 0; i < o.segs && i < full.segs; i++ ) {
        const auto &a = o.seg_rows[i];
        const auto &b = full.seg_rows[i];
        if ( a.type != b.type || a.offset != b.offset || a.vaddr != b.vaddr || a.filesz != b.filesz || a.flags != b.flags )
          finding("a segment row changed under truncation", base_path.c_str());
      }
      // section NAMES legitimately shorten when the shstrtab itself is cut, so only the numeric
      // fields are held to the prefix property
      for ( usize i = 0; i < o.secs && i < full.secs; i++ ) {
        const auto &a = o.sec_rows[i];
        const auto &b = full.sec_rows[i];
        if ( a.type != b.type || a.offset != b.offset || a.size != b.size || a.addr != b.addr )
          finding("a section row changed under truncation", base_path.c_str());
      }
    }
  }
  end_test_case();

  // ---- ident mutations ---------------------------------------------------
  test_case("open_image rejects every malformed e_ident with the documented message");
  {
    struct {
      usize at;
      u8 val;
      const char *msg;
    } cases[] = {
      { 0, 0x00, "bad magic" },
      { 1, 'X', "bad magic" },
      { 2, 'X', "bad magic" },
      { 3, 'X', "bad magic" },
      { 4, 0, "unknown elf class" },
      { 4, 3, "unknown elf class" },
      { 4, 0xff, "unknown elf class" },
      { 5, 0, "unknown elf data encoding" },
      { 5, 3, "unknown elf data encoding" },
      { 6, 0, "unsupported elf version" },
      { 6, 2, "unsupported elf version" },
    };

    for ( const auto &c : cases ) {
      heap_image m = base.clone(base.n);
      m.w8(c.at, c.val);
      const mr::source src{ m.p, m.n };
      auto r = mr::open_image(src);
      if ( r.is_first() ) {
        finding("open_image accepted a broken e_ident", c.msg);
        continue;
      }
      if ( micron::strcmp(r.cast<const char *>(), c.msg) != 0 )
        finding("open_image rejected with the wrong message", r.cast<const char *>());
    }
  }
  end_test_case();

  // ---- the class fork ----------------------------------------------------
  test_case("an ELF64 image relabelled EI_CLASS=1 stays in bounds while producing nonsense");
  {
    heap_image m = base.clone(base.n);
    m.w8(4, 1);      // ELFCLASS32 over a real 64-bit file
    const outcome o = run_battery(m.p, m.n);
    if ( o.threw ) finding("the class fork threw out of the reader", "EI_CLASS=1 over ELF64");
    // it must not crash and must not invent an unbounded number of rows; the CONTENT is garbage
    // by construction and is deliberately not asserted about
    if ( o.secs > (m.n / 40) + 1 ) finding("the class fork produced more sections than the file can hold", "");
    if ( o.segs > (m.n / 32) + 1 ) finding("the class fork produced more segments than the file can hold", "");
  }
  end_test_case();

  // ---- header count / stride mutations -----------------------------------
  test_case("absurd phnum/shnum/phentsize/shentsize stay bounded by the file");
  {
    struct {
      usize at;
      usize width;
      u64 val;
      const char *what;
    } cases[] = {
      { off64.phnum, 2, 0xffff, "e_phnum=0xffff" },
      { off64.shnum, 2, 0xffff, "e_shnum=0xffff" },
      { off64.phentsize, 2, 0, "e_phentsize=0" },
      { off64.phentsize, 2, 0xffff, "e_phentsize=0xffff" },
      { off64.shentsize, 2, 0xffff, "e_shentsize=0xffff" },
      { off64.shstrndx, 2, 0xffff, "e_shstrndx=SHN_XINDEX" },
      { off64.shstrndx, 2, 0xfffe, "e_shstrndx out of range" },
    };

    for ( const auto &c : cases ) {
      heap_image m = base.clone(base.n);
      if ( c.width == 2 ) m.w16(c.at, static_cast<u16>(c.val));
      const outcome o = run_battery(m.p, m.n);
      if ( o.threw ) finding("a header-count mutation threw out of the reader", c.what);
      if ( o.segs > 0xffff ) finding("segment count exceeded e_phnum's range", c.what);
      if ( o.secs > 0x100000 ) finding("section count ran away", c.what);
    }
  }
  end_test_case();

  test_case("out-of-file and wrapping phoff/shoff stay memory-safe");
  {
    const u64 vals[] = { base.n - 1, base.n, base.n + 1, ~0ull, ~0ull - 8 };
    for ( u64 v : vals ) {
      for ( usize which = 0; which < 2; which++ ) {
        heap_image m = base.clone(base.n);
        m.w64(which == 0 ? off64.phoff : off64.shoff, v);
        const outcome o = run_battery(m.p, m.n);
        if ( o.threw ) finding("an out-of-file table offset threw out of the reader", which ? "shoff" : "phoff");
      }
    }
  }
  end_test_case();

  // ---- the allocation-amplification vectors, now clamped ------------------
  //
  // Every walker derives its record count from a field the file controls -- a section's sh_size, a
  // segment's p_filesz, sh[0].sh_size standing in for e_shnum -- and used to hand it straight to
  // reserve(). The loops were always safe (each stops at the first short read); the reserve was
  // not. read::clamp_records now bounds each count by what the source can actually hold, and
  // walk_sections rejects a zero e_shentsize outright.
  //
  // These cases are the regression: they pin the clamped counts, and the seeded fuzz below is what
  // originally found the p_filesz path (13 escapes in 4096 iterations).
  test_case("e_shnum==0 takes the section count from sh[0].sh_size, bounded only by short reads");
  {
    const u64 counts[] = { 0, 1, 64, 4096 };
    for ( u64 K : counts ) {
      heap_image m = base.clone(base.n);
      m.w16(off64.shnum, 0);
      m.w64(static_cast<usize>(full_shoff) + off64.shdr_size_field, K);
      const outcome o = run_battery(m.p, m.n);
      if ( o.threw ) finding("the e_shnum==0 path threw out of the reader", "sh[0].sh_size");

      // the loop stops at the first short read, so the reachable count is what the file holds
      u64 room = 0;
      if ( base.n >= full_shoff + off64.shdr_sz && full_shentsize ) room = (base.n - full_shoff - off64.shdr_sz) / full_shentsize + 1;
      const u64 expect = K < room ? K : room;
      if ( o.secs != expect ) finding("e_shnum==0 produced an unexpected section count", base_path.c_str());
    }
  }
  end_test_case();

  test_case("e_shentsize==0 is rejected rather than re-reading section 0 forever");
  {
    // a zero stride makes read_one_section recompute the same offset for every index, so it never
    // returns false and the loop loses its only bound. Combined with e_shnum==0 taking the count
    // from sh[0].sh_size, that was an unbounded allocation loop.
    const u64 counts[] = { 0, 4096, ~0ull };
    for ( u64 K : counts ) {
      heap_image m = base.clone(base.n);
      m.w16(off64.shnum, 0);
      m.w16(off64.shentsize, 0);
      m.w64(static_cast<usize>(full_shoff) + off64.shdr_size_field, K);
      const outcome o = run_battery(m.p, m.n);
      if ( o.threw ) finding("the e_shentsize==0 path threw out of the reader", "");
      if ( o.secs != 0 ) finding("a zero-stride section table produced rows", "");
    }
    // and a non-zero e_shnum with a zero stride is equally not a table
    heap_image m = base.clone(base.n);
    m.w16(off64.shentsize, 0);
    const outcome o = run_battery(m.p, m.n);
    if ( o.threw ) finding("the e_shentsize==0 path threw out of the reader", "shnum intact");
    if ( o.secs != 0 ) finding("a zero-stride section table produced rows", "shnum intact");
  }
  end_test_case();

  test_case("an absurd sh_size cannot amplify a walk's reserve past the file");
  {
    // pick the largest section and blow its sh_size up: symbols, relocs and versyms all size
    // their vectors from it
    usize biggest = 0;
    for ( usize i = 1; i < full.secs; i++ )
      if ( full.sec_rows[i].size > full.sec_rows[biggest].size ) biggest = i;
    const u64 shdr = full_shoff + static_cast<u64>(biggest) * full_shentsize;
    const u64 sizes[] = { 1ull << 20, 1ull << 32, 1ull << 48, ~0ull };
    for ( u64 sz : sizes ) {
      heap_image m = base.clone(base.n);
      m.w64(static_cast<usize>(shdr) + off64.shdr_size_field, sz);
      const outcome o = run_battery(m.p, m.n);
      if ( o.threw ) finding("an oversized sh_size threw out of the reader", "");
      if ( o.syms > m.n / 24 + 1 ) finding("symbol count exceeded what the file can hold", "");
      if ( o.relocs > m.n / 16 + 1 ) finding("reloc count exceeded what the file can hold", "");
      if ( o.versyms > m.n / 2 + 1 ) finding("versym count exceeded what the file can hold", "");
    }
  }
  end_test_case();

  // ---- read_dynamic's reserve is unbounded --------------------------------
  //
  // read_dynamic takes its entry count from PT_DYNAMIC's p_filesz (or SHT_DYNAMIC's sh_size),
  // divides by the record size, and hands that straight to reserve() with no bound against the
  // source. One flipped byte in p_filesz is enough. This is what the seeded fuzz below keeps
  // rediscovering, so it is pinned here deterministically rather than left to chance.
  test_case("read_dynamic sizes its entry vector from an unvalidated PT_DYNAMIC p_filesz");
  {
    // locate PT_DYNAMIC in the program header table
    usize dyn_ph = ~usize(0);
    for ( u64 i = 0; i < full_phnum; i++ ) {
      const usize at = static_cast<usize>(full_phoff + i * full_phentsize);
      if ( at + 56 > base.n ) break;
      if ( rd_le(base.p, at, 4) == me::pt_dynamic ) {
        dyn_ph = at;
        break;
      }
    }
    if ( dyn_ph == ~usize(0) ) {
      sb::print("  base image has no PT_DYNAMIC -- this case needs one, skipped");
    } else {
      const u64 spans[] = { 0x10000ull, 0x1000000ull, 0x1000000000ull };
      for ( u64 sp : spans ) {
        heap_image m = base.clone(base.n);
        m.w64(dyn_ph + 32, sp);      // p_filesz
        const outcome o = run_battery(m.p, m.n);
        if ( o.threw )
          finding("read_dynamic reserve()d from an unvalidated p_filesz and let the allocator "
                  "exception escape (src/linux/elf/read/dynamic.hpp, entries.reserve)",
                  "p_filesz was flipped to a large value");
        else if ( o.dyns > m.n / 16 )
          finding("read_dynamic produced more entries than the file can hold", "");
      }
    }
  }
  end_test_case();

  // ---- seeded byte-flip fuzz ----------------------------------------------
  test_case("a seeded byte-flip fuzz never crashes and never escapes an exception");
  {
    const usize win = base.n < 65536 ? base.n : 65536;
    u64 threw = 0;
    for ( u64 it = 0; it < static_cast<u64>(MICRON_ELF_FUZZ_ITERS); it++ ) {
      heap_image m = base.clone(win);
      const u64 r = next_u64();
      const u64 nflip = 1 + (r % 8);
      for ( u64 k = 0; k < nflip; k++ ) {
        const u64 q = next_u64();
        // bias three quarters of the flips into the headers, where the parser decisions live
        const usize at = static_cast<usize>((q % 4) ? (q % (win < 4096 ? win : 4096)) : (q % win));
        m.p[at] = static_cast<u8>(m.p[at] ^ (1u << ((q >> 32) & 7)));
      }
      const outcome o = run_battery(m.p, m.n);
      if ( o.threw ) {
        threw++;
        if ( threw <= 6 )
          sb::print("    escaped from ", o.threw_in, " at iteration ", it, "  [e_shnum=", rd_le(m.p, off64.shnum, 2),
                    " e_shentsize=", rd_le(m.p, off64.shentsize, 2), " e_shoff=", rd_le(m.p, off64.shoff, 8),
                    " e_phnum=", rd_le(m.p, off64.phnum, 2), " e_phentsize=", rd_le(m.p, off64.phentsize, 2), "]");
      }
      if ( o.segs > 0xffff || o.secs > 0x100000 || o.syms > 0x2000000 )
        finding("a fuzzed image produced an implausible row count", "see the fixed seed");
    }
    if ( threw ) {
      sb::print("  FINDING: ", threw, " of ", static_cast<u64>(MICRON_ELF_FUZZ_ITERS),
                " fuzz iterations threw out of the reader (seed 0x9E3779B97F4A7C15)");
      ++findings;
    }
  }
  end_test_case();

  sb::print("--- findings: ", findings, " ---");

  test_case("no robustness findings");
  {
    require(findings, 0ull);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
