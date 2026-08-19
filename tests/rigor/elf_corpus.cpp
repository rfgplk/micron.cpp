//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Sweeps every ELF under /bin and /usr/lib64 through micron::elf::read and compares every answer
// against the oracle in this file -- which shares no code with src/: its own little-endian byte
// readers, its own literal record offsets, no micron::elf constant, no sizeof(ehdr_t), no
// __builtin_offsetof. A mirror of the implementation would prove nothing; the independence is the
// whole point.
//
// Mismatches are counted and logged, never aborted on: the sweep runs to the end and the require()
// on the grand total is what fails the run. sb::check() is NOT used -- it dumps a stack walk per
// call, which across 15k files is unreadable.

#include "../../src/linux/elf/read.hpp"

#include "../../src/linux/elf/hash.hpp"
#include "../../src/linux/io/ext.hpp"

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

// a detected sanitizer error must not exit(1): duck grades 1 as PASS (qemu.hh, __snowball_pass)
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

#ifndef MICRON_ELF_CORPUS_STRIDE
#define MICRON_ELF_CORPUS_STRIDE 1
#endif
#ifndef MICRON_ELF_CORPUS_MAX
#define MICRON_ELF_CORPUS_MAX 0
#endif
#ifndef MICRON_ELF_CORPUS_MAX_FILE_MB
#define MICRON_ELF_CORPUS_MAX_FILE_MB 0
#endif
#ifndef MICRON_ELF_CORPUS_MIN_FILES
#define MICRON_ELF_CORPUS_MIN_FILES 256
#endif

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the oracle -- independent of src/ by construction

namespace oracle
{

struct blob {
  const u8 *p = nullptr;
  u64 n = 0;

  // overflow-free: never forms off+len
  bool
  in(u64 off, u64 len) const noexcept
  {
    return p != nullptr && off <= n && len <= n - off;
  }

  u8
  rd8(u64 o) const noexcept
  {
    return p[o];
  }

  u16
  rd16(u64 o) const noexcept
  {
    return static_cast<u16>(static_cast<u16>(p[o]) | (static_cast<u16>(p[o + 1]) << 8));
  }

  u32
  rd32(u64 o) const noexcept
  {
    return static_cast<u32>(p[o]) | (static_cast<u32>(p[o + 1]) << 8) | (static_cast<u32>(p[o + 2]) << 16)
           | (static_cast<u32>(p[o + 3]) << 24);
  }

  u64
  rd64(u64 o) const noexcept
  {
    return static_cast<u64>(rd32(o)) | (static_cast<u64>(rd32(o + 4)) << 32);
  }
};

struct o_ehdr {
  bool ident_ok = false;      // magic + class in {1,2} + data in {1,2} + version == 1
  bool hdr_ok = false;        // ident_ok and the full ehdr fits
  u8 cls = 0, data = 0, ident_ver = 0, osabi = 0, abiver = 0;
  u16 type = 0, machine = 0;
  u32 e_version = 0, flags = 0;
  u64 entry = 0, phoff = 0, shoff = 0;
  u16 ehsize = 0, phentsize = 0, phnum = 0, shentsize = 0, shnum = 0, shstrndx = 0;
};

struct o_phdr {
  u32 type = 0, flags = 0;
  u64 offset = 0, vaddr = 0, paddr = 0, filesz = 0, memsz = 0, align = 0;
};

struct o_shdr {
  u32 name_off = 0, type = 0;
  u64 flags = 0, addr = 0, offset = 0, size = 0;
  u32 link = 0, info = 0;
  u64 addralign = 0, entsize = 0;
};

struct o_sym {
  u32 name_off = 0;
  u64 value = 0, size = 0;
  u8 info = 0, other = 0;
  u16 shndx = 0;
};

struct o_rel {
  u64 offset = 0, info = 0;
  i64 addend = 0;
};

struct o_dyn {
  i64 tag = 0;
  u64 val = 0;
};

// record sizes, spelled as literals on purpose
constexpr u64 sz_ehdr32 = 52, sz_ehdr64 = 64;
constexpr u64 sz_phdr32 = 32, sz_phdr64 = 56;
constexpr u64 sz_shdr32 = 40, sz_shdr64 = 64;
constexpr u64 sz_sym32 = 16, sz_sym64 = 24;
constexpr u64 sz_rel32 = 8, sz_rela32 = 12;
constexpr u64 sz_rel64 = 16, sz_rela64 = 24;
constexpr u64 sz_dyn32 = 8, sz_dyn64 = 16;

inline u64
sz_ehdr(u8 c)
{
  return c == 2 ? sz_ehdr64 : sz_ehdr32;
}

inline u64
sz_phdr(u8 c)
{
  return c == 2 ? sz_phdr64 : sz_phdr32;
}

inline u64
sz_shdr(u8 c)
{
  return c == 2 ? sz_shdr64 : sz_shdr32;
}

inline u64
sz_sym(u8 c)
{
  return c == 2 ? sz_sym64 : sz_sym32;
}

inline u64
sz_reloc(u8 c, bool rela)
{
  return c == 2 ? (rela ? sz_rela64 : sz_rel64) : (rela ? sz_rela32 : sz_rel32);
}

inline u64
sz_dyn(u8 c)
{
  return c == 2 ? sz_dyn64 : sz_dyn32;
}

inline bool
read_ehdr(const blob &b, o_ehdr &o)
{
  o = o_ehdr{};
  if ( !b.in(0, 16) ) return false;
  if ( b.rd8(0) != 0x7f || b.rd8(1) != 'E' || b.rd8(2) != 'L' || b.rd8(3) != 'F' ) return false;
  o.cls = b.rd8(4);
  o.data = b.rd8(5);
  o.ident_ver = b.rd8(6);
  o.osabi = b.rd8(7);
  o.abiver = b.rd8(8);
  if ( o.cls != 1 && o.cls != 2 ) return false;
  if ( o.data != 1 && o.data != 2 ) return false;
  if ( o.ident_ver != 1 ) return false;
  o.ident_ok = true;

  if ( !b.in(0, sz_ehdr(o.cls)) ) return false;
  o.hdr_ok = true;
  // LSB only; an MSB file is reported ident_ok and the caller skips content compare
  if ( o.data != 1 ) return true;

  o.type = b.rd16(16);
  o.machine = b.rd16(18);
  o.e_version = b.rd32(20);
  if ( o.cls == 2 ) {
    o.entry = b.rd64(24);
    o.phoff = b.rd64(32);
    o.shoff = b.rd64(40);
    o.flags = b.rd32(48);
    o.ehsize = b.rd16(52);
    o.phentsize = b.rd16(54);
    o.phnum = b.rd16(56);
    o.shentsize = b.rd16(58);
    o.shnum = b.rd16(60);
    o.shstrndx = b.rd16(62);
  } else {
    o.entry = b.rd32(24);
    o.phoff = b.rd32(28);
    o.shoff = b.rd32(32);
    o.flags = b.rd32(36);
    o.ehsize = b.rd16(40);
    o.phentsize = b.rd16(42);
    o.phnum = b.rd16(44);
    o.shentsize = b.rd16(46);
    o.shnum = b.rd16(48);
    o.shstrndx = b.rd16(50);
  }
  return true;
}

inline bool
read_phdr(const blob &b, u8 cls, u64 off, o_phdr &o)
{
  o = o_phdr{};
  if ( !b.in(off, sz_phdr(cls)) ) return false;
  if ( cls == 2 ) {
    o.type = b.rd32(off + 0);
    o.flags = b.rd32(off + 4);
    o.offset = b.rd64(off + 8);
    o.vaddr = b.rd64(off + 16);
    o.paddr = b.rd64(off + 24);
    o.filesz = b.rd64(off + 32);
    o.memsz = b.rd64(off + 40);
    o.align = b.rd64(off + 48);
  } else {
    // ELF32 puts p_flags LAST but one, not second
    o.type = b.rd32(off + 0);
    o.offset = b.rd32(off + 4);
    o.vaddr = b.rd32(off + 8);
    o.paddr = b.rd32(off + 12);
    o.filesz = b.rd32(off + 16);
    o.memsz = b.rd32(off + 20);
    o.flags = b.rd32(off + 24);
    o.align = b.rd32(off + 28);
  }
  return true;
}

inline bool
read_shdr(const blob &b, u8 cls, u64 off, o_shdr &o)
{
  o = o_shdr{};
  if ( !b.in(off, sz_shdr(cls)) ) return false;
  if ( cls == 2 ) {
    o.name_off = b.rd32(off + 0);
    o.type = b.rd32(off + 4);
    o.flags = b.rd64(off + 8);
    o.addr = b.rd64(off + 16);
    o.offset = b.rd64(off + 24);
    o.size = b.rd64(off + 32);
    o.link = b.rd32(off + 40);
    o.info = b.rd32(off + 44);
    o.addralign = b.rd64(off + 48);
    o.entsize = b.rd64(off + 56);
  } else {
    o.name_off = b.rd32(off + 0);
    o.type = b.rd32(off + 4);
    o.flags = b.rd32(off + 8);
    o.addr = b.rd32(off + 12);
    o.offset = b.rd32(off + 16);
    o.size = b.rd32(off + 20);
    o.link = b.rd32(off + 24);
    o.info = b.rd32(off + 28);
    o.addralign = b.rd32(off + 32);
    o.entsize = b.rd32(off + 36);
  }
  return true;
}

inline bool
read_sym(const blob &b, u8 cls, u64 off, o_sym &o)
{
  o = o_sym{};
  if ( !b.in(off, sz_sym(cls)) ) return false;
  if ( cls == 2 ) {
    o.name_off = b.rd32(off + 0);
    o.info = b.rd8(off + 4);
    o.other = b.rd8(off + 5);
    o.shndx = b.rd16(off + 6);
    o.value = b.rd64(off + 8);
    o.size = b.rd64(off + 16);
  } else {
    // ELF32 orders name,value,size,info,other,shndx -- nothing like the 64-bit record
    o.name_off = b.rd32(off + 0);
    o.value = b.rd32(off + 4);
    o.size = b.rd32(off + 8);
    o.info = b.rd8(off + 12);
    o.other = b.rd8(off + 13);
    o.shndx = b.rd16(off + 14);
  }
  return true;
}

inline bool
read_reloc(const blob &b, u8 cls, bool rela, u64 off, o_rel &o)
{
  o = o_rel{};
  if ( !b.in(off, sz_reloc(cls, rela)) ) return false;
  if ( cls == 2 ) {
    o.offset = b.rd64(off + 0);
    o.info = b.rd64(off + 8);
    if ( rela ) o.addend = static_cast<i64>(b.rd64(off + 16));
  } else {
    o.offset = b.rd32(off + 0);
    o.info = b.rd32(off + 4);
    if ( rela ) o.addend = static_cast<i64>(static_cast<i32>(b.rd32(off + 8)));
  }
  return true;
}

// r_info packs the symbol index in the TOP 24 bits on ELF32 and the top 32 on ELF64
inline u32
r_sym(u8 cls, u64 info)
{
  return cls == 2 ? static_cast<u32>(info >> 32) : static_cast<u32>(info >> 8);
}

inline u32
r_type(u8 cls, u64 info)
{
  return cls == 2 ? static_cast<u32>(info & 0xffffffffu) : static_cast<u32>(info & 0xffu);
}

inline bool
read_dyn(const blob &b, u8 cls, u64 off, o_dyn &o)
{
  o = o_dyn{};
  if ( !b.in(off, sz_dyn(cls)) ) return false;
  if ( cls == 2 ) {
    o.tag = static_cast<i64>(b.rd64(off + 0));
    o.val = b.rd64(off + 8);
  } else {
    // d_tag is a SIGNED word on ELF32
    o.tag = static_cast<i64>(static_cast<i32>(b.rd32(off + 0)));
    o.val = b.rd32(off + 4);
  }
  return true;
}

// mirrors read_cstr_at: clip at the limit, stop at NUL, never fail
inline micron::string
cstr(const blob &b, u64 off, u64 limit)
{
  micron::string out{};
  if ( b.p == nullptr || off >= b.n ) return out;
  const u64 avail = b.n - off;
  const u64 lim = limit < avail ? limit : avail;
  for ( u64 i = 0; i < lim; i++ ) {
    const u8 c = b.p[off + i];
    if ( c == 0 ) break;
    out.push_back(static_cast<char>(c));
  }
  return out;
}

// PT_LOAD only, half-open [vaddr, vaddr+filesz)
inline bool
vaddr_to_off(const micron::vector<o_phdr> &ph, u64 va, u64 &out)
{
  for ( const auto &s : ph ) {
    if ( s.type != 1 ) continue;
    if ( va >= s.vaddr && va < s.vaddr + s.filesz ) {
      out = s.offset + (va - s.vaddr);
      return true;
    }
  }
  return false;
}

};      // namespace oracle

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// failure accounting

enum class fc : u8 {
  open_taxonomy,
  open_reject,
  open_accept,
  ehdr_field,
  phdr_count,
  phdr_field,
  shdr_count,
  shdr_field,
  sec_name,
  sym_count,
  sym_field,
  sym_name,
  reloc_count,
  reloc_field,
  relr_set,
  dyn_count,
  dyn_field,
  dyn_string,
  versym,
  version_row,
  vaddr_map,
  interp,
  link_kind,
  name_table,
  hash_count,
  hash_lookup,
  hash_version,
  invariant,
  count_
};

constexpr const char *fc_name[static_cast<usize>(fc::count_)] = {
  "open_taxonomy", "open_reject", "open_accept", "ehdr_field", "phdr_count",  "phdr_field",   "shdr_count",
  "shdr_field",    "sec_name",    "sym_count",   "sym_field",  "sym_name",    "reloc_count",  "reloc_field",
  "relr_set",      "dyn_count",   "dyn_field",   "dyn_string", "versym",      "version_row",  "vaddr_map",
  "interp",        "link_kind",   "name_table",  "hash_count", "hash_lookup", "hash_version", "invariant",
};

static_assert(sizeof(fc_name) / sizeof(fc_name[0]) == static_cast<usize>(fc::count_), "keep fc and fc_name in step");

struct failure_log {
  static constexpr u64 detail_per_cat = 5;
  static constexpr usize max_msgs = 192;
  u64 hits[static_cast<usize>(fc::count_)] = {};
  u64 total = 0;

  // every `field` argument in this file is a string literal, so identity is the key
  const char *msg[max_msgs] = {};
  u64 msg_hits[max_msgs] = {};
  usize msg_n = 0;

  void
  tally(const char *field)
  {
    for ( usize i = 0; i < msg_n; i++ )
      if ( msg[i] == field ) {
        ++msg_hits[i];
        return;
      }
    if ( msg_n < max_msgs ) {
      msg[msg_n] = field;
      msg_hits[msg_n] = 1;
      ++msg_n;
    }
  }

  // returns true when this hit should print its detail
  bool
  bump(fc c)
  {
    const usize i = static_cast<usize>(c);
    ++hits[i];
    ++total;
    if ( hits[i] == detail_per_cat + 1 ) sb::print("  [", fc_name[i], "] further detail suppressed for this category");
    return hits[i] <= detail_per_cat;
  }

  void
  fail(fc c, const char *path, const char *field, u64 expect, u64 actual)
  {
    tally(field);
    if ( bump(c) ) sb::print("  [", fc_name[static_cast<usize>(c)], "] ", path, " :: ", field, " expected=", expect, " actual=", actual);
  }

  void
  fail_s(fc c, const char *path, const char *field, const char *expect, const char *actual)
  {
    tally(field);
    if ( bump(c) )
      sb::print("  [", fc_name[static_cast<usize>(c)], "] ", path, " :: ", field, " expected=\"", expect, "\" actual=\"", actual, "\"");
  }

  void
  note(fc c, const char *path, const char *field)
  {
    tally(field);
    if ( bump(c) ) sb::print("  [", fc_name[static_cast<usize>(c)], "] ", path, " :: ", field);
  }

  void
  report() const
  {
    sb::print("--- failures by category ---");
    for ( usize i = 0; i < static_cast<usize>(fc::count_); i++ )
      if ( hits[i] ) sb::print("    ", fc_name[i], ": ", hits[i]);
    if ( total == 0 ) sb::print("    (none)");
    if ( msg_n ) {
      sb::print("--- failures by check ---");
      for ( usize i = 0; i < msg_n; i++ ) sb::print("    ", msg_hits[i], "  ", msg[i]);
      if ( msg_n == max_msgs ) sb::print("    (message table full; some checks not listed)");
    }
    sb::print("--- grand total: ", total, " ---");
  }
};

// 0x-prefixed hex into a caller buffer; the gap report is unreadable in decimal
struct hexbuf {
  char b[19];

  explicit hexbuf(u64 v)
  {
    static const char *dig = "0123456789abcdef";
    usize i = sizeof(b) - 1;
    b[i] = 0;
    if ( v == 0 ) b[--i] = '0';
    while ( v ) {
      b[--i] = dig[v & 0xf];
      v >>= 4;
    }
    b[--i] = 'x';
    b[--i] = '0';
    for ( usize k = 0; i < sizeof(b); k++, i++ ) b[k] = b[i];
  }

  const char *
  c_str() const
  {
    return b;
  }
};

failure_log flog{};

// The *_name tables document nullptr as "unknown", so a null is not a contract violation -- it is
// a coverage gap against what the world actually ships. Collect the DISTINCT values rather than
// counting 2319 repeats, report them loudly, and do not fail the run on them.
struct gap_log {
  static constexpr usize max_gaps = 64;
  const char *kind[max_gaps] = {};
  u64 value[max_gaps] = {};
  u64 hits[max_gaps] = {};
  micron::sstring<256> first[max_gaps] = {};
  usize n = 0;

  void
  add(const char *k, u64 v, const char *path)
  {
    for ( usize i = 0; i < n; i++ )
      if ( kind[i] == k && value[i] == v ) {
        ++hits[i];
        return;
      }
    if ( n < max_gaps ) {
      kind[n] = k;
      value[n] = v;
      hits[n] = 1;
      for ( usize j = 0; path[j] && j < 250; j++ ) first[n] += path[j];
      first[n].null_term();
      ++n;
    }
  }

  void
  report() const
  {
    if ( n == 0 ) {
      sb::print("--- name-table coverage: complete for every value in the corpus ---");
      return;
    }
    sb::print("--- name-table coverage gaps (reported, NOT failed: nullptr-for-unknown is the ", "documented contract) ---");
    for ( usize i = 0; i < n; i++ )
      sb::print("    ", kind[i], " has no name for ", hexbuf(value[i]).c_str(), " (", hits[i], " files, e.g. ", first[i].c_str(), ")");
    if ( n == max_gaps ) sb::print("    (gap table full; more may exist)");
  }
};

gap_log glog{};

struct walk_stats {
  u64 dirs = 0, dirs_denied = 0, dirs_too_deep = 0;
  u64 entries = 0, regular = 0, symlinks = 0, unknown_type = 0, other_type = 0;
  u64 open_denied = 0, open_empty = 0, open_mmap_failed = 0;
  u64 too_big = 0, non_elf = 0, strided_out = 0, capped_out = 0;
  u64 elf = 0, elf32 = 0, elf64 = 0, msb = 0;
  u64 et_rel = 0, et_exec = 0, et_dyn = 0, et_other = 0;
  u64 no_phdrs = 0, no_dynamic = 0, from_section = 0;
  u64 alloc_failed = 0;
  u64 checks = 0;
};

walk_stats wst{};

constexpr u32 corpus_max_depth = 16;
using pathbuf = micron::sstring<1024>;

template<usize N>
inline void
path_append(micron::sstring<N> &p, const char *s)
{
  for ( usize i = 0; s[i]; i++ ) p += s[i];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-file comparison

inline void
cmp(fc c, const char *path, const char *field, u64 expect, u64 actual)
{
  ++wst.checks;
  if ( expect != actual ) flog.fail(c, path, field, expect, actual);
}

inline void
cmp_str(fc c, const char *path, const char *field, const micron::string &expect, const micron::string &actual)
{
  ++wst.checks;
  if ( micron::strcmp(expect.c_str(), actual.c_str()) != 0 ) flog.fail_s(c, path, field, expect.c_str(), actual.c_str());
}

inline void
want(fc c, const char *path, const char *field, bool ok)
{
  ++wst.checks;
  if ( !ok ) flog.note(c, path, field);
}

// resolve the section count the way the reader does: e_shnum, or sh[0].sh_size when it is zero
inline void
oracle_sections(const oracle::blob &b, const oracle::o_ehdr &oe, micron::vector<oracle::o_shdr> &out, u64 &shstrndx)
{
  shstrndx = oe.shstrndx;
  if ( oe.shoff == 0 ) return;

  u64 shnum = oe.shnum;
  if ( shnum == 0 || shstrndx == 0xffff ) {
    oracle::o_shdr s0{};
    if ( oracle::read_shdr(b, oe.cls, oe.shoff, s0) ) {
      if ( shnum == 0 ) shnum = s0.size;
      if ( shstrndx == 0xffff ) shstrndx = s0.link;
    }
  }
  if ( shnum == 0 ) return;

  for ( u64 i = 0; i < shnum; i++ ) {
    oracle::o_shdr s{};
    const u64 off = oe.shoff + i * static_cast<u64>(oe.shentsize);
    if ( !oracle::read_shdr(b, oe.cls, off, s) ) break;
    out.push_back(s);
  }
}

inline micron::string
oracle_sec_name(const oracle::blob &b, const micron::vector<oracle::o_shdr> &secs, u64 shstrndx, u32 name_off)
{
  if ( shstrndx >= secs.size() ) return micron::string{};
  const u64 so = secs[static_cast<usize>(shstrndx)].offset;
  const u64 ss = secs[static_cast<usize>(shstrndx)].size;
  const u64 lim = ss > name_off ? ss - name_off : 0;
  return oracle::cstr(b, so + name_off, lim);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the hash layer, exercised over a FILE image
//
// micron::elf::{gnu_lookup,sysv_lookup,count_dynsyms,sym_version_index} take a dyn_info<C> whose
// members are raw pointers into a live, relocated mapping. A file image can stand in for that: the
// dynamic tags carry vaddrs, vaddr_to_offset turns each one into a file offset, and the load bias
// is uniform so every pointer lands on the same bytes it would after loading. That lets the
// class-generic hash layer be driven against 15k real binaries -- including the 5 real i386 ones
// from a 64-bit host -- without loading a single one.
//
// It is only sound behind gates. Those functions trust their input completely: sysv_lookup indexes
// symtab[i] for i < nchain, both lookups walk strtab + st_name unbounded, and the gnu-hash chain
// walk is capped per bucket rather than by the table. Everything below is gated against the FILE
// size (not sh_size: 381 corpus files have a .gnu.hash whose computed extent exceeds sh_size while
// none exceeds the file) and against the alignment each pointer is about to be cast to.

template<me::fmt_class C> struct dyn_view {
  me::dyn_info<C> d{};
  bool usable = false;        // symtab + strtab gated
  bool hash_ok = false;       // sysv_lookup legal
  bool gnu_ok = false;        // gnu_lookup legal
  bool count_ok = false;      // count_dynsyms legal
  bool versym_ok = false;
  u64 truth = 0;      // .dynsym entry count from the section header -- the answer being checked
};

template<me::fmt_class C>
void
check_hash_layer(const char *path, const mr::source &src, const mr::image &img, const micron::vector<mr::segment_row> &segs,
                 const micron::vector<oracle::o_shdr> &osec, const mr::dynamic_info &dyn, const micron::vector<mr::section_row> &secs)
{
  using tr = me::elf_traits<C>;
  if ( img.hdr.data != me::native_data ) return;      // dyn_info does no byteswapping
  if ( !dyn.present ) return;

  // the truth: .dynsym sh_size / sh_entsize
  const oracle::o_shdr *dynsym_sec = nullptr;
  for ( const auto &s : osec )
    if ( s.type == 11 ) {
      dynsym_sec = &s;
      break;
    }
  if ( dynsym_sec == nullptr || dynsym_sec->entsize == 0 ) return;

  dyn_view<C> v{};
  v.truth = dynsym_sec->size / dynsym_sec->entsize;

  const u64 flen = src.size();
  const auto off_of = [&](u64 va, u64 &out) -> bool {
    const auto r = mr::vaddr_to_offset(segs, va);
    if ( r.is_second() ) return false;
    out = r.cast<u64>();
    return true;
  };
  const auto ptr_at = [&](u64 off, u64 need, usize align) -> const u8 * {
    if ( off >= flen || need > flen - off ) return nullptr;
    const u8 *q = src.data() + off;
    if ( align && (reinterpret_cast<uintptr_t>(q) % align) != 0 ) return nullptr;
    return q;
  };

  u64 va_symtab = 0, va_strtab = 0, va_hash = 0, va_gnu = 0, va_versym = 0, strsz = 0;
  bool has_symtab = false, has_strtab = false, has_hash = false, has_gnu = false, has_versym = false;
  for ( const auto &e : dyn.entries ) {
    switch ( e.tag ) {
    case me::dt_symtab:
      va_symtab = e.val;
      has_symtab = true;
      break;
    case me::dt_strtab:
      va_strtab = e.val;
      has_strtab = true;
      break;
    case me::dt_strsz:
      strsz = e.val;
      break;
    case me::dt_hash:
      va_hash = e.val;
      has_hash = true;
      break;
    case me::dt_gnu_hash:
      va_gnu = e.val;
      has_gnu = true;
      break;
    case me::dt_versym:
      va_versym = e.val;
      has_versym = true;
      break;
    default:
      break;
    }
  }
  if ( !has_symtab || !has_strtab || strsz == 0 ) return;

  u64 o = 0;
  const u8 *p_str = off_of(va_strtab, o) ? ptr_at(o, strsz, 1) : nullptr;
  if ( p_str == nullptr || p_str[strsz - 1] != 0 ) return;      // must be NUL-terminated to walk

  const u8 *p_sym = off_of(va_symtab, o) ? ptr_at(o, v.truth * sizeof(typename tr::sym), alignof(typename tr::sym)) : nullptr;
  if ( p_sym == nullptr ) return;

  v.d.strtab = reinterpret_cast<const char *>(p_str);
  v.d.strsz = strsz;
  v.d.symtab = reinterpret_cast<const typename tr::sym *>(p_sym);
  v.d.symcount = v.truth;

  // gate 2: both lookups walk strtab + st_name with no bound of their own
  for ( u64 i = 0; i < v.truth; i++ )
    if ( static_cast<u64>(v.d.symtab[i].name) >= strsz ) return;
  v.usable = true;

  if ( has_hash && off_of(va_hash, o) ) {
    if ( const u8 *ph = ptr_at(o, 8, alignof(me::word)) ) {
      const me::word *h = reinterpret_cast<const me::word *>(ph);
      const u64 nb = h[0], nc = h[1];
      // gate 1: sysv_lookup dereferences symtab[i] for i < nchain
      if ( nb && nc <= v.truth && ptr_at(o, 8 + 4 * (nb + nc), alignof(me::word)) ) {
        v.d.hash = h;
        v.hash_ok = true;
      }
    }
  }

  if ( has_gnu && off_of(va_gnu, o) ) {
    using bloom_t = typename tr::uword;
    if ( const u8 *pg = ptr_at(o, 16, alignof(bloom_t)) ) {
      const me::word *gh = reinterpret_cast<const me::word *>(pg);
      const u64 nb = gh[0], bias = gh[1], bl = gh[2];
      // the header, bloom words and buckets always exist; the CHAIN array does not when the
      // module exports nothing -- .gnu.hash is then just 16 + one bloom word + one bucket
      const u64 need_head = 16 + bl * sizeof(bloom_t) + nb * 4;
      if ( nb && bl && bias <= v.truth && ptr_at(o, need_head, alignof(bloom_t)) ) {
        const bloom_t *bloom = reinterpret_cast<const bloom_t *>(gh + 4);
        const me::word *buckets = reinterpret_cast<const me::word *>(bloom + bl);
        const me::word *chain = buckets + nb;

        u64 max_bucket = 0;
        bool in_range = true;
        for ( u64 i = 0; i < nb; i++ ) {
          if ( buckets[i] == 0 ) continue;
          if ( buckets[i] < bias || buckets[i] >= v.truth ) in_range = false;
          if ( buckets[i] > max_bucket ) max_bucket = buckets[i];
        }

        if ( max_bucket == 0 ) {
          // every bucket empty: gnu_lookup returns at `idx < symbias` and count_dynsyms' walk
          // reads nothing at all, so both are safe without a chain to gate. This is exactly the
          // export-less shape, and gating it away would hide the case worth testing.
          v.d.gnu_hash = gh;
          v.gnu_ok = true;
          v.count_ok = true;
        } else {
          // gate 3: count_dynsyms' chain walk is bounded per bucket by 1<<24, not by the table.
          // It is safe only once every bucket starts inside [bias, truth) and the chain is
          // terminated -- together those prove the walk stops.
          const u64 need = need_head + (v.truth - bias) * 4;
          const bool gated = v.truth > bias && ptr_at(o, need, alignof(bloom_t)) != nullptr;
          if ( gated ) {
            v.d.gnu_hash = gh;
            v.gnu_ok = true;
            v.count_ok = in_range && (chain[v.truth - bias - 1] & 1) != 0;
          }
        }
      }
    }
  }

  if ( has_versym && off_of(va_versym, o) )
    if ( const u8 *pv = ptr_at(o, v.truth * 2, alignof(me::half)) ) {
      v.d.versym = reinterpret_cast<const me::half *>(pv);
      v.versym_ok = true;
    }

  // ---- what the gates bought --------------------------------------------
  if ( v.hash_ok || v.count_ok )
    cmp(fc::hash_count, path, "count_dynsyms vs the .dynsym section header", v.truth, me::count_dynsyms<C>(v.d));

  if ( v.usable && (v.hash_ok || v.gnu_ok) ) {
    // every defined, non-local, named symbol must be findable by name. verified reachable for
    // every such symbol across the corpus by an independent chain walk, so a miss is a real bug.
    // NOTE: match on the RETURNED symbol's name, not its index -- versioned symbols share a name
    // across several slots and the reader returns the first chain hit.
    u64 bias = v.gnu_ok ? v.d.gnu_hash[1] : 0;
    for ( u64 i = 1; i < v.truth; i++ ) {
      const auto &sy = v.d.symtab[i];
      if ( sy.shndx == me::shn_undef ) continue;
      if ( me::elf_st_bind(sy.info) == me::stb_local ) continue;
      if ( sy.name == 0 ) continue;
      if ( !v.hash_ok && i < bias ) continue;      // gnu hash covers only [symoffset, symcount)
      const char *nm = v.d.strtab + sy.name;
      const auto *got = me::lookup_sym<C>(v.d, nm);
      ++wst.checks;
      if ( got == nullptr )
        flog.note(fc::hash_lookup, path, "lookup_sym missed a defined exported symbol");
      else if ( micron::strcmp(v.d.strtab + got->name, nm) != 0 )
        flog.note(fc::hash_lookup, path, "lookup_sym returned a symbol with a different name");
    }
    ++wst.checks;
    if ( me::lookup_sym<C>(v.d, "\x01__micron_no_such_symbol_ever__") != nullptr )
      flog.note(fc::hash_lookup, path, "lookup_sym invented a symbol that is not there");
  }

  if ( v.usable && v.versym_ok ) {
    const mr::section_row *vs = mr::find_section_by_type(secs, me::sht_gnu_versym);
    if ( vs ) {
      const micron::vector<me::half> read_vs = mr::read_versym(img, *vs);
      const u64 n = read_vs.size() < v.truth ? read_vs.size() : v.truth;
      for ( u64 i = 0; i < n; i++ )
        cmp(fc::hash_version, path, "sym_version_index vs the reader's versym array", me::elf_ver_ndx(read_vs[static_cast<usize>(i)]),
            me::sym_version_index<C>(v.d, &v.d.symtab[i]));
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void
check_elf(const char *path, const mr::source &src, const oracle::o_ehdr &oe, const mr::image &img)
{
  const oracle::blob b{ src.data(), src.size() };
  const u8 cls = oe.cls;

  // ---- A. header ------------------------------------------------------
  cmp(fc::ehdr_field, path, "cls", cls, img.is64() ? 2u : 1u);
  cmp(fc::ehdr_field, path, "data", oe.data, static_cast<u64>(img.hdr.data == me::fmt_data::lsb ? 1 : 2));
  cmp(fc::ehdr_field, path, "osabi", oe.osabi, img.hdr.osabi);
  cmp(fc::ehdr_field, path, "abiversion", oe.abiver, img.hdr.abiversion);
  cmp(fc::ehdr_field, path, "type", oe.type, img.hdr.type);
  cmp(fc::ehdr_field, path, "machine", oe.machine, img.hdr.machine);
  cmp(fc::ehdr_field, path, "version", oe.e_version, img.hdr.version);
  cmp(fc::ehdr_field, path, "entry", oe.entry, img.hdr.entry);
  cmp(fc::ehdr_field, path, "phoff", oe.phoff, img.hdr.phoff);
  cmp(fc::ehdr_field, path, "shoff", oe.shoff, img.hdr.shoff);
  cmp(fc::ehdr_field, path, "flags", oe.flags, img.hdr.flags);
  cmp(fc::ehdr_field, path, "ehsize", oe.ehsize, img.hdr.ehsize);
  cmp(fc::ehdr_field, path, "phentsize", oe.phentsize, img.hdr.phentsize);
  cmp(fc::ehdr_field, path, "phnum", oe.phnum, img.hdr.phnum);
  cmp(fc::ehdr_field, path, "shentsize", oe.shentsize, img.hdr.shentsize);
  cmp(fc::ehdr_field, path, "shnum", oe.shnum, img.hdr.shnum);
  cmp(fc::ehdr_field, path, "shstrndx", oe.shstrndx, img.hdr.shstrndx);

  // ehdr invariants, verified to hold on every file in the corpus
  want(fc::invariant, path, "ehsize matches the class", oe.ehsize == (cls == 2 ? 64 : 52));
  want(fc::invariant, path, "shoff != 0", oe.shoff != 0);
  if ( oe.phnum ) want(fc::invariant, path, "phentsize matches the class", oe.phentsize == (cls == 2 ? 56 : 32));
  if ( oe.shnum ) {
    want(fc::invariant, path, "shentsize matches the class", oe.shentsize == (cls == 2 ? 64 : 40));
    want(fc::invariant, path, "shstrndx < shnum", oe.shstrndx < oe.shnum);
  }

  // ---- B. segments ----------------------------------------------------
  micron::vector<oracle::o_phdr> oph{};
  if ( oe.phoff && oe.phnum ) {
    for ( u64 i = 0; i < oe.phnum; i++ ) {
      oracle::o_phdr p{};
      if ( !oracle::read_phdr(b, cls, oe.phoff + i * static_cast<u64>(oe.phentsize), p) ) break;
      oph.push_back(p);
    }
  }
  const micron::vector<mr::segment_row> segs = mr::walk_segments(img);
  cmp(fc::phdr_count, path, "segment count", oph.size(), segs.size());
  if ( oph.empty() ) ++wst.no_phdrs;

  const usize nseg = oph.size() < segs.size() ? oph.size() : segs.size();
  for ( usize i = 0; i < nseg; i++ ) {
    const auto &e = oph[i];
    const auto &a = segs[i];
    cmp(fc::phdr_field, path, "p_type", e.type, a.type);
    cmp(fc::phdr_field, path, "p_flags", e.flags, a.flags);
    cmp(fc::phdr_field, path, "p_offset", e.offset, a.offset);
    cmp(fc::phdr_field, path, "p_vaddr", e.vaddr, a.vaddr);
    cmp(fc::phdr_field, path, "p_paddr", e.paddr, a.paddr);
    cmp(fc::phdr_field, path, "p_filesz", e.filesz, a.filesz);
    cmp(fc::phdr_field, path, "p_memsz", e.memsz, a.memsz);
    cmp(fc::phdr_field, path, "p_align", e.align, a.align);
  }

  // segment invariants
  u64 prev_load_va = 0;
  bool seen_load = false;
  for ( const auto &p : oph ) {
    if ( p.type != 0x6474e551 )      // PT_GNU_STACK carries no file range
      want(fc::invariant, path, "phdr file range inside the file", b.in(p.offset, p.filesz));
    if ( p.type == 1 ) {
      want(fc::invariant, path, "PT_LOAD filesz <= memsz", p.filesz <= p.memsz);
      if ( p.align ) want(fc::invariant, path, "PT_LOAD vaddr congruent to offset", (p.vaddr % p.align) == (p.offset % p.align));
      if ( seen_load ) want(fc::invariant, path, "PT_LOADs ascending by vaddr", p.vaddr >= prev_load_va);
      prev_load_va = p.vaddr;
      seen_load = true;
    }
    if ( p.type == 6 ) {      // PT_PHDR
      want(fc::invariant, path, "PT_PHDR offset == e_phoff", p.offset == oe.phoff);
      want(fc::invariant, path, "PT_PHDR size == phnum*phentsize", p.filesz == static_cast<u64>(oe.phnum) * static_cast<u64>(oe.phentsize));
    }
  }

  // ---- C. vaddr_to_offset ---------------------------------------------
  for ( const auto &p : oph ) {
    if ( p.type != 1 || p.filesz == 0 ) continue;
    const u64 probes[3] = { p.vaddr, p.vaddr + p.filesz / 2, p.vaddr + p.filesz - 1 };
    for ( u64 v : probes ) {
      u64 want_off = 0;
      const bool ok = oracle::vaddr_to_off(oph, v, want_off);
      const auto got = mr::vaddr_to_offset(segs, v);
      ++wst.checks;
      if ( ok != got.is_first() )
        flog.fail(fc::vaddr_map, path, "vaddr mapped-ness", static_cast<u64>(ok), static_cast<u64>(got.is_first()));
      else if ( ok && got.cast<u64>() != want_off )
        flog.fail(fc::vaddr_map, path, "vaddr -> offset", want_off, got.cast<u64>());
    }
  }

  // ---- D. interp / link kind ------------------------------------------
  const oracle::o_phdr *interp = nullptr;
  const oracle::o_phdr *dynseg = nullptr;
  for ( const auto &p : oph ) {
    if ( p.type == 3 && !interp ) interp = &p;
    if ( p.type == 2 && !dynseg ) dynseg = &p;
  }
  {
    const auto got = mr::read_interp(img, segs);
    ++wst.checks;
    if ( (interp != nullptr) != got.is_first() )
      flog.fail(fc::interp, path, "PT_INTERP presence", static_cast<u64>(interp != nullptr), static_cast<u64>(got.is_first()));
    else if ( interp )
      cmp_str(fc::interp, path, "interp", oracle::cstr(b, interp->offset, interp->filesz), got.cast<micron::string>());
  }
  {
    const mr::link_kind wantk = interp ? mr::link_kind::dynamic : (dynseg ? mr::link_kind::static_pie : mr::link_kind::static_exec);
    cmp(fc::link_kind, path, "classify_link", static_cast<u64>(wantk), static_cast<u64>(mr::classify_link(segs)));
  }

  // ---- E. sections ----------------------------------------------------
  micron::vector<oracle::o_shdr> osec{};
  u64 oshstrndx = 0;
  oracle_sections(b, oe, osec, oshstrndx);
  const micron::vector<mr::section_row> secs = mr::walk_sections(img);
  cmp(fc::shdr_count, path, "section count", osec.size(), secs.size());

  const usize nsec = osec.size() < secs.size() ? osec.size() : secs.size();
  for ( usize i = 0; i < nsec; i++ ) {
    const auto &e = osec[i];
    const auto &a = secs[i];
    cmp(fc::shdr_field, path, "sh_type", e.type, a.type);
    cmp(fc::shdr_field, path, "sh_flags", e.flags, a.flags);
    cmp(fc::shdr_field, path, "sh_addr", e.addr, a.addr);
    cmp(fc::shdr_field, path, "sh_offset", e.offset, a.offset);
    cmp(fc::shdr_field, path, "sh_size", e.size, a.size);
    cmp(fc::shdr_field, path, "sh_link", e.link, a.link);
    cmp(fc::shdr_field, path, "sh_info", e.info, a.info);
    cmp(fc::shdr_field, path, "sh_addralign", e.addralign, a.addralign);
    cmp(fc::shdr_field, path, "sh_entsize", e.entsize, a.entsize);
    cmp_str(fc::sec_name, path, "sh_name", oracle_sec_name(b, osec, oshstrndx, e.name_off), a.name);

    // section invariants (SHT_NOBITS occupies no file space)
    if ( e.type != 8 && e.type != 0 ) want(fc::invariant, path, "section file range inside the file", b.in(e.offset, e.size));
    if ( e.type == 2 || e.type == 11 ) {
      want(fc::invariant, path, "symtab entsize matches the class", e.entsize == (cls == 2 ? 24u : 16u));
      if ( e.entsize ) want(fc::invariant, path, "symtab size is a whole number of entries", (e.size % e.entsize) == 0);
    }
    if ( e.type == 4 ) want(fc::invariant, path, "rela entsize matches the class", e.entsize == (cls == 2 ? 24u : 12u));
    if ( e.type == 9 ) want(fc::invariant, path, "rel entsize matches the class", e.entsize == (cls == 2 ? 16u : 8u));
    if ( e.type == 19 ) want(fc::invariant, path, "relr entsize matches the class", e.entsize == (cls == 2 ? 8u : 4u));
    if ( e.link ) want(fc::invariant, path, "sh_link < shnum", e.link < osec.size());

    // an allocated, non-empty, file-backed section must be reachable through a PT_LOAD.
    // Two narrowings, both learned from real files rather than assumed:
    //   sh_size > 0 -- an EMPTY section legitimately sits at p_vaddr+p_filesz, one past the
    //     half-open end, and 437 sections in the corpus do exactly that.
    //   seen_load   -- an ET_REL object has no program headers at all, so nothing can resolve
    //     through a PT_LOAD (/usr/lib64/gcrt1.o carries SHF_ALLOC sections with a nonzero addr).
    if ( seen_load && (e.flags & 0x2) && e.addr && e.type != 8 && e.size > 0 ) {
      u64 wo = 0;
      const bool ok = oracle::vaddr_to_off(oph, e.addr, wo);
      want(fc::invariant, path, "alloc section vaddr resolves to its file offset", ok && wo == e.offset);
    }
  }

  // at most one of each singleton section type -- what makes find_section_by_type correct
  {
    const u32 singles[] = { 2u, 11u, 6u, 5u, 19u, 0x6ffffff6u, 0x6ffffffdu, 0x6ffffffeu, 0x6fffffffu };
    for ( u32 t : singles ) {
      u64 k = 0;
      for ( const auto &s : osec )
        if ( s.type == t ) k++;
      want(fc::invariant, path, "at most one section of each singleton type", k <= 1);
    }
  }

  // ---- F. symbols -----------------------------------------------------
  const micron::vector<mr::symbol_row> syms = mr::walk_symbols(img, secs);
  {
    // the reader walks the FIRST SHT_SYMTAB then the FIRST SHT_DYNSYM, concatenated
    const u32 kinds[2] = { 2u, 11u };
    usize cursor = 0;
    for ( usize k = 0; k < 2; k++ ) {
      const oracle::o_shdr *st = nullptr;
      for ( const auto &s : osec )
        if ( s.type == kinds[k] ) {
          st = &s;
          break;
        }
      u64 n = 0;
      const u64 esz = oracle::sz_sym(cls);
      if ( st && st->size >= esz ) n = st->size / esz;

      const oracle::o_shdr *str = (st && st->link < osec.size()) ? &osec[static_cast<usize>(st->link)] : nullptr;
      for ( u64 i = 0; i < n; i++ ) {
        oracle::o_sym s{};
        if ( !oracle::read_sym(b, cls, st->offset + i * esz, s) ) break;
        if ( cursor >= syms.size() ) {
          flog.note(fc::sym_count, path, "reader produced fewer symbols than the oracle");
          break;
        }
        const auto &a = syms[cursor++];
        cmp(fc::sym_field, path, "sym table kind", static_cast<u64>(k), static_cast<u64>(a.table));
        cmp(fc::sym_field, path, "st_value", s.value, a.value);
        cmp(fc::sym_field, path, "st_size", s.size, a.size);
        cmp(fc::sym_field, path, "st_bind", static_cast<u64>(s.info >> 4), a.bind);
        cmp(fc::sym_field, path, "st_type", static_cast<u64>(s.info & 0xf), a.type);
        cmp(fc::sym_field, path, "st_visibility", static_cast<u64>(s.other & 0x3), a.visibility);
        cmp(fc::sym_field, path, "st_shndx", s.shndx, a.shndx);
        cmp(fc::sym_field, path, "defined", static_cast<u64>(s.shndx != 0), static_cast<u64>(a.defined));
        // the reader leaves the name empty for st_name == 0 rather than reading strtab[0]
        micron::string wantn{};
        if ( str && s.name_off != 0 ) {
          const u64 lim = str->size > s.name_off ? str->size - s.name_off : 0;
          wantn = oracle::cstr(b, str->offset + s.name_off, lim);
        }
        cmp_str(fc::sym_name, path, "st_name", wantn, a.name);
        if ( str && kinds[k] == 11u ) want(fc::invariant, path, "dynsym st_name inside .dynstr", static_cast<u64>(s.name_off) < str->size);
      }
    }
    cmp(fc::sym_count, path, "symbol count", cursor, syms.size());
  }

  // ---- G. relocations --------------------------------------------------
  for ( usize si = 0; si < nsec; si++ ) {
    const auto &s = osec[si];
    if ( s.type != 4 && s.type != 9 ) continue;
    const bool rela = s.type == 4;
    const u64 esz = oracle::sz_reloc(cls, rela);
    const u64 n = s.size < esz ? 0 : s.size / esz;

    const micron::vector<mr::reloc_row> rows = mr::walk_relocs(img, secs[si]);
    u64 produced = 0;
    for ( u64 i = 0; i < n; i++ ) {
      oracle::o_rel r{};
      if ( !oracle::read_reloc(b, cls, rela, s.offset + i * esz, r) ) break;
      if ( produced >= rows.size() ) break;
      const auto &a = rows[static_cast<usize>(produced)];
      cmp(fc::reloc_field, path, "r_offset", r.offset, a.offset);
      cmp(fc::reloc_field, path, "r_sym", oracle::r_sym(cls, r.info), a.sym);
      cmp(fc::reloc_field, path, "r_type", oracle::r_type(cls, r.info), a.type);
      cmp(fc::reloc_field, path, "r_addend", static_cast<u64>(r.addend), static_cast<u64>(a.addend));
      cmp(fc::reloc_field, path, "has_addend", static_cast<u64>(rela), static_cast<u64>(a.has_addend));
      produced++;
    }
    cmp(fc::reloc_count, path, "reloc count", produced, rows.size());

    // every relocation names a symbol inside the table sh_link points at
    if ( s.link < osec.size() ) {
      const auto &st = osec[static_cast<usize>(s.link)];
      const u64 sesz = oracle::sz_sym(cls);
      const u64 nsym = sesz && st.size >= sesz ? st.size / sesz : 0;
      for ( const auto &a : rows ) want(fc::invariant, path, "reloc symbol index inside its symbol table", a.sym < nsym || nsym == 0);
    }
  }

  // ---- H. RELR ---------------------------------------------------------
  for ( usize si = 0; si < nsec; si++ ) {
    const auto &s = osec[si];
    if ( s.type != 19 ) continue;
    const u64 esz = cls == 2 ? 8 : 4;
    const u64 bits = esz * 8 - 1;      // 63 on ELF64, 31 on ELF32
    const u64 n = s.size < esz ? 0 : s.size / esz;

    const mr::relr_out got = mr::walk_relr(img, secs[si]);
    micron::vector<u64> expect{};
    u64 where = 0;
    for ( u64 i = 0; i < n; i++ ) {
      const u64 off = s.offset + i * esz;
      if ( !b.in(off, esz) ) break;
      const u64 e = cls == 2 ? b.rd64(off) : b.rd32(off);
      if ( (e & 1) == 0 ) {
        where = e;
        expect.push_back(where);
        where += esz;
        continue;
      }
      u64 bm = e >> 1;
      for ( u64 k = 0; bm != 0; k++, bm >>= 1 )
        if ( bm & 1 ) expect.push_back(where + k * esz);
      where += bits * esz;
    }
    cmp(fc::relr_set, path, "relr address count", expect.size(), got.at.size());
    const usize nr = expect.size() < got.at.size() ? expect.size() : got.at.size();
    for ( usize i = 0; i < nr; i++ ) cmp(fc::relr_set, path, "relr address", expect[i], got.at[i]);
    // NOT asserted: that a RELR address is pointer-aligned. /bin/nvme carries 4-aligned base
    // entries in .relr.dyn and readelf decodes them the same way the reader does, so real
    // linkers do emit them and the "obvious" alignment invariant is simply false.
    for ( usize i = 0; i < got.at.size(); i++ ) {
      u64 wo = 0;
      want(fc::invariant, path, "relr address lies in a PT_LOAD", oracle::vaddr_to_off(oph, got.at[i], wo));
    }
  }

  // ---- I. dynamic ------------------------------------------------------
  const mr::dynamic_info dyn = mr::read_dynamic(img, segs, secs);
  {
    const oracle::o_shdr *dynsec = nullptr;
    for ( const auto &s : osec )
      if ( s.type == 6 ) {
        dynsec = &s;
        break;
      }
    u64 base = 0, span = 0;
    bool present = false, from_section = false;
    if ( dynseg ) {
      base = dynseg->offset;
      span = dynseg->filesz;
      present = true;
    } else if ( dynsec && dynsec->type != 8 ) {
      base = dynsec->offset;
      span = dynsec->size;
      present = true;
      from_section = true;
    }
    cmp(fc::dyn_field, path, "dynamic present", static_cast<u64>(present), static_cast<u64>(dyn.present));
    cmp(fc::dyn_field, path, "dynamic from_section", static_cast<u64>(from_section), static_cast<u64>(dyn.from_section));
    if ( !present ) ++wst.no_dynamic;
    if ( from_section ) ++wst.from_section;

    micron::vector<oracle::o_dyn> od{};
    const u64 esz = oracle::sz_dyn(cls);
    if ( present && span >= esz ) {
      const u64 n = span / esz;
      for ( u64 i = 0; i < n; i++ ) {
        oracle::o_dyn e{};
        if ( !oracle::read_dyn(b, cls, base + i * esz, e) ) break;
        od.push_back(e);
        if ( e.tag == 0 ) break;
      }
    }
    cmp(fc::dyn_count, path, "dynamic entry count", od.size(), dyn.entries.size());
    const usize nd = od.size() < dyn.entries.size() ? od.size() : dyn.entries.size();
    for ( usize i = 0; i < nd; i++ ) {
      cmp(fc::dyn_field, path, "d_tag", static_cast<u64>(od[i].tag), static_cast<u64>(dyn.entries[i].tag));
      cmp(fc::dyn_field, path, "d_val", od[i].val, dyn.entries[i].val);
    }

    // strings, resolved through the oracle's own DT_STRTAB
    u64 strtab_va = 0, strsz = 0;
    bool have_str = false;
    micron::vector<u64> needed{};
    u64 soname = ~0ull, rpath = ~0ull, runpath = ~0ull;
    for ( const auto &e : od ) {
      if ( e.tag == 5 ) {
        strtab_va = e.val;
        have_str = true;
      } else if ( e.tag == 10 )
        strsz = e.val;
      else if ( e.tag == 1 )
        needed.push_back(e.val);
      else if ( e.tag == 14 )
        soname = e.val;
      else if ( e.tag == 15 )
        rpath = e.val;
      else if ( e.tag == 29 )
        runpath = e.val;
    }
    if ( have_str ) {
      u64 stroff = 0;
      bool resolved = oracle::vaddr_to_off(oph, strtab_va, stroff);
      if ( !resolved ) {
        for ( usize i = 0; i < osec.size(); i++ ) {
          if ( micron::strcmp(oracle_sec_name(b, osec, oshstrndx, osec[i].name_off).c_str(), ".dynstr") == 0 ) {
            stroff = osec[i].offset;
            if ( strsz == 0 ) strsz = osec[i].size;
            resolved = true;
            break;
          }
        }
      }
      if ( resolved ) {
        const u64 limit = strsz != 0 ? strsz : (64ull << 10);
        const auto res = [&](u64 no) { return oracle::cstr(b, stroff + no, limit > no ? limit - no : 0); };
        cmp(fc::dyn_string, path, "DT_NEEDED count", needed.size(), dyn.needed.size());
        const usize nn = needed.size() < dyn.needed.size() ? needed.size() : dyn.needed.size();
        for ( usize i = 0; i < nn; i++ ) cmp_str(fc::dyn_string, path, "DT_NEEDED", res(needed[i]), dyn.needed[i]);
        if ( soname != ~0ull ) cmp_str(fc::dyn_string, path, "DT_SONAME", res(soname), dyn.soname);
        if ( rpath != ~0ull ) cmp_str(fc::dyn_string, path, "DT_RPATH", res(rpath), dyn.rpath);
        if ( runpath != ~0ull ) cmp_str(fc::dyn_string, path, "DT_RUNPATH", res(runpath), dyn.runpath);
      }
    }
  }

  // ---- J. versions -----------------------------------------------------
  for ( usize si = 0; si < nsec; si++ ) {
    const auto &s = osec[si];
    if ( s.type == 0x6fffffff ) {      // SHT_GNU_VERSYM
      const micron::vector<me::half> vs = mr::read_versym(img, secs[si]);
      const u64 n = s.size < 2 ? 0 : s.size / 2;
      cmp(fc::versym, path, "versym count", n, vs.size());
      for ( usize i = 0; i < vs.size(); i++ ) {
        const u64 off = s.offset + static_cast<u64>(i) * 2;
        if ( !b.in(off, 2) ) break;
        cmp(fc::versym, path, "versym entry", b.rd16(off), vs[i]);
      }
      // one half per .dynsym entry
      for ( const auto &d : osec )
        if ( d.type == 11 && d.entsize ) want(fc::invariant, path, "versym length == dynsym count", n == d.size / d.entsize);
    }
    if ( s.type == 0x6ffffffd ) {      // SHT_GNU_VERDEF
      const micron::vector<mr::version_row> rows = mr::read_verdef(img, secs, secs[si]);
      const oracle::o_shdr *str = s.link < osec.size() ? &osec[static_cast<usize>(s.link)] : nullptr;
      micron::vector<mr::version_row> expect{};
      if ( str ) {
        u64 at = s.offset;
        const u64 end = s.offset + s.size;
        for ( usize g = 0; g < 4096 && at + 20 <= end && b.in(at, 20); g++ ) {
          const u16 flags = b.rd16(at + 2), ndx = b.rd16(at + 4), cnt = b.rd16(at + 6);
          const u32 aux = b.rd32(at + 12), next = b.rd32(at + 16);
          if ( cnt != 0 && aux != 0 && at + aux + 8 <= end && b.in(at + aux, 8) ) {
            const u32 no = b.rd32(at + aux);
            mr::version_row r{};
            r.index = static_cast<u16>(ndx & 0x7fff);
            r.flags = flags;
            r.is_need = false;
            if ( no < str->size ) r.name = oracle::cstr(b, str->offset + no, str->size - no);
            expect.push_back(micron::move(r));
          }
          if ( next == 0 ) break;
          at += next;
        }
      }
      cmp(fc::version_row, path, "verdef row count", expect.size(), rows.size());
      const usize nv = expect.size() < rows.size() ? expect.size() : rows.size();
      for ( usize i = 0; i < nv; i++ ) {
        cmp(fc::version_row, path, "verdef index", expect[i].index, rows[i].index);
        cmp(fc::version_row, path, "verdef flags", expect[i].flags, rows[i].flags);
        cmp_str(fc::version_row, path, "verdef name", expect[i].name, rows[i].name);
      }
    }
    if ( s.type == 0x6ffffffe ) {      // SHT_GNU_VERNEED
      const micron::vector<mr::version_row> rows = mr::read_verneed(img, secs, secs[si]);
      const oracle::o_shdr *str = s.link < osec.size() ? &osec[static_cast<usize>(s.link)] : nullptr;
      micron::vector<mr::version_row> expect{};
      if ( str ) {
        const auto nm = [&](u32 off) {
          micron::string t{};
          if ( off < str->size ) t = oracle::cstr(b, str->offset + off, str->size - off);
          return t;
        };
        u64 at = s.offset;
        const u64 end = s.offset + s.size;
        for ( usize g = 0; g < 4096 && at + 16 <= end && b.in(at, 16); g++ ) {
          const u16 cnt = b.rd16(at + 2);
          const u32 file = b.rd32(at + 4), aux = b.rd32(at + 8), next = b.rd32(at + 12);
          const micron::string from = nm(file);
          u64 a = at + aux;
          for ( u16 k = 0; k < cnt && a + 16 <= end && b.in(a, 16); k++ ) {
            const u16 flags = b.rd16(a + 4), other = b.rd16(a + 6);
            const u32 no = b.rd32(a + 8), anext = b.rd32(a + 12);
            mr::version_row r{};
            r.index = static_cast<u16>(other & 0x7fff);
            r.flags = flags;
            r.is_need = true;
            r.name = nm(no);
            r.file = from;
            expect.push_back(micron::move(r));
            if ( anext == 0 ) break;
            a += anext;
          }
          if ( next == 0 ) break;
          at += next;
        }
      }
      cmp(fc::version_row, path, "verneed row count", expect.size(), rows.size());
      const usize nv = expect.size() < rows.size() ? expect.size() : rows.size();
      for ( usize i = 0; i < nv; i++ ) {
        cmp(fc::version_row, path, "verneed index", expect[i].index, rows[i].index);
        cmp(fc::version_row, path, "verneed flags", expect[i].flags, rows[i].flags);
        cmp_str(fc::version_row, path, "verneed name", expect[i].name, rows[i].name);
        cmp_str(fc::version_row, path, "verneed file", expect[i].file, rows[i].file);
      }
    }
  }

  // ---- K. name tables --------------------------------------------------
  // every value the corpus actually contains must have a name; a gap is data, not a crash
  ++wst.checks;
  if ( mr::etype_name(oe.type) == nullptr ) glog.add("etype_name", oe.type, path);
  if ( mr::osabi_name(oe.osabi) == nullptr ) glog.add("osabi_name", oe.osabi, path);
  if ( mr::machine_name(oe.machine) == nullptr ) glog.add("machine_name", oe.machine, path);
  for ( const auto &p : oph )
    if ( mr::pt_name(p.type) == nullptr ) glog.add("pt_name", p.type, path);
  for ( const auto &s : osec )
    if ( mr::sht_name(s.type) == nullptr ) glog.add("sht_name", s.type, path);
  for ( const auto &e : dyn.entries )
    if ( mr::dt_tag_name(oe.machine, e.tag) == nullptr ) glog.add("dt_tag_name", static_cast<u64>(e.tag), path);
  for ( usize si = 0; si < nsec; si++ ) {
    const auto &s = osec[si];
    if ( s.type != 4 && s.type != 9 ) continue;
    for ( const auto &r : mr::walk_relocs(img, secs[si]) )
      if ( mr::reloc_type_name(oe.machine, r.type) == nullptr ) glog.add("reloc_type_name", r.type, path);
  }

  // ---- L. the hash layer ------------------------------------------------
  if ( cls == 2 )
    check_hash_layer<me::fmt_class::elf64>(path, src, img, segs, osec, dyn, secs);
  else
    check_hash_layer<me::fmt_class::elf32>(path, src, img, segs, osec, dyn, secs);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the sweep

u64 stride_counter = 0;
u64 parsed = 0;

void
on_file(const char *path)
{
  auto sr = mr::source::open(path);
  if ( sr.is_second() ) {
    const char *e = sr.cast<const char *>();
    if ( micron::strcmp(e, "elf::read: open failed") == 0 )
      ++wst.open_denied;
    else if ( micron::strcmp(e, "elf::read: empty or unseekable file") == 0 )
      ++wst.open_empty;
    else if ( micron::strcmp(e, "elf::read: mmap failed") == 0 )
      ++wst.open_mmap_failed;
    else
      flog.fail_s(fc::open_taxonomy, path, "source::open error", "one of the three documented strings", e);
    return;
  }

  const mr::source &src = sr.cast<mr::source>();

#if MICRON_ELF_CORPUS_MAX_FILE_MB
  if ( src.size() > static_cast<usize>(MICRON_ELF_CORPUS_MAX_FILE_MB) * 1024u * 1024u ) {
    ++wst.too_big;
    return;
  }
#endif

  const oracle::blob b{ src.data(), src.size() };
  oracle::o_ehdr oe{};
  oracle::read_ehdr(b, oe);

  auto ir = mr::open_image(src);

  if ( !oe.ident_ok || !oe.hdr_ok ) {
    // a script, a linker script, a directory, anything that is not an ELF: rejection is REQUIRED
    ++wst.non_elf;
    ++wst.checks;
    if ( ir.is_first() ) flog.note(fc::open_accept, path, "open_image accepted a file the oracle rejects");
    return;
  }

  ++wst.checks;
  if ( ir.is_second() ) {
    flog.fail_s(fc::open_reject, path, "open_image", "accept", ir.cast<const char *>());
    return;
  }

  ++wst.elf;
  ++parsed;
  if ( oe.cls == 2 )
    ++wst.elf64;
  else
    ++wst.elf32;
  if ( oe.type == 1 )
    ++wst.et_rel;
  else if ( oe.type == 2 )
    ++wst.et_exec;
  else if ( oe.type == 3 )
    ++wst.et_dyn;
  else
    ++wst.et_other;

  if ( oe.data != 1 ) {
    // the oracle is little-endian by design; nothing in the corpus is MSB
    ++wst.msb;
    return;
  }

  check_elf(path, src, oe, ir.cast<mr::image>());
}

void
walk_corpus(const char *root)
{
  micron::vector<pathbuf> queue{};
  micron::vector<u32> depth{};
  {
    pathbuf p{};
    path_append(p, root);
    p.null_term();
    queue.push_back(p);
    depth.push_back(0);
  }

  for ( usize qi = 0; qi < queue.size(); qi++ ) {
    const pathbuf dir = queue[qi];
    const u32 d = depth[qi];

    mp::dir_t fd = mp::opendir(dir.c_str());
    if ( !fd.open() ) {
      ++wst.dirs_denied;
      continue;
    }
    ++wst.dirs;

    // a fresh context per directory: readdir_r only refills when bufpos >= nread, so a reused
    // one replays the tail of the previous directory
    mp::readdir_ctx ctx{};
    for ( ;; ) {
      mp::__impl_dir e = mp::readdir_r(fd, ctx);
      if ( e.type == mp::dt_end ) break;
      ++wst.entries;

      const char *nm = e.d_name.c_str();
      if ( nm[0] == '.' && (nm[1] == 0 || (nm[1] == '.' && nm[2] == 0)) ) continue;

      pathbuf p = dir;
      if ( p.size() && p[p.size() - 1] != '/' ) p += '/';
      path_append(p, nm);
      p.null_term();

      if ( e.type == mp::dt_dir ) {
        // d_type reports a symlink-to-directory as DT_LNK, so only real directories are
        // enqueued and the walk is acyclic by construction
        if ( d + 1 >= corpus_max_depth ) {
          ++wst.dirs_too_deep;
          continue;
        }
        queue.push_back(p);
        depth.push_back(d + 1);
        continue;
      }
      if ( e.type == mp::dt_lnk ) {
        ++wst.symlinks;
        continue;
      }
      if ( e.type == mp::dt_unknown )
        ++wst.unknown_type;
      else if ( e.type != mp::dt_reg ) {
        ++wst.other_type;
        continue;
      }

      ++wst.regular;
      if ( (stride_counter++ % static_cast<u64>(MICRON_ELF_CORPUS_STRIDE)) != 0 ) {
        ++wst.strided_out;
        continue;
      }
#if MICRON_ELF_CORPUS_MAX
      if ( parsed >= static_cast<u64>(MICRON_ELF_CORPUS_MAX) ) {
        ++wst.capped_out;
        continue;
      }
#endif
      // A 32-bit address space cannot hold a full-corpus sweep: micron's default allocator rounds
      // every request up to 4096, so one binary with ~100k symbols costs hundreds of MB in name
      // strings alone and reserve() raises except::critical_error. Contain it per file so one big
      // binary cannot end the sweep, and count it -- the count is required to be zero, so the
      // cross-arch rows must be tuned (stride / size cap) rather than quietly losing coverage.
      try {
        on_file(p.c_str());
      } catch ( ... ) {
        ++wst.alloc_failed;
      }
    }
    mp::closedir(fd);
  }
}

};      // namespace

int
main(void)
{
  sb::print("=== ELF CORPUS SWEEP ===");

  constexpr const char *roots[] = { "/bin", "/usr/lib64" };

  // Deliberately a require, not a skip. micron is Linux-only and /bin always exists, so an
  // unreadable root means the directory plumbing is broken, not that the box is unusual -- and a
  // skip here would turn that into a silent, instant PASS. It already nearly did: aarch64 used
  // the amd64 O_DIRECTORY value, every opendir returned -EINVAL, and the sweep "passed" in 20ms
  // having parsed nothing.
  test_case("both corpus roots are readable");
  {
    for ( const char *r : roots ) {
      mp::dir_t fd = mp::opendir(r);
      if ( !fd.open() ) sb::print("cannot open corpus root: ", r);
      require_true(fd.open());
      mp::closedir(fd);
    }
  }
  end_test_case();

  for ( const char *r : roots ) walk_corpus(r);

  sb::print("--- census ---");
  sb::print("    dirs=", wst.dirs, " denied=", wst.dirs_denied, " too_deep=", wst.dirs_too_deep);
  sb::print("    entries=", wst.entries, " regular=", wst.regular, " symlinks=", wst.symlinks, " unknown_type=", wst.unknown_type,
            " other_type=", wst.other_type);
  sb::print("    open_denied=", wst.open_denied, " open_empty=", wst.open_empty, " open_mmap_failed=", wst.open_mmap_failed,
            " too_big=", wst.too_big);
  sb::print("    strided_out=", wst.strided_out, " capped_out=", wst.capped_out);
  sb::print("    non_elf=", wst.non_elf, " elf=", wst.elf, " elf32=", wst.elf32, " elf64=", wst.elf64, " msb=", wst.msb);
  sb::print("    et_rel=", wst.et_rel, " et_exec=", wst.et_exec, " et_dyn=", wst.et_dyn, " et_other=", wst.et_other);
  sb::print("    no_phdrs=", wst.no_phdrs, " no_dynamic=", wst.no_dynamic, " from_section=", wst.from_section,
            " alloc_failed=", wst.alloc_failed);
  sb::print("    field comparisons run=", wst.checks);

  glog.report();
  flog.report();

  test_case("the sweep actually saw a corpus");
  {
    require_true(wst.elf >= static_cast<u64>(MICRON_ELF_CORPUS_MIN_FILES));
    require_true(wst.elf64 > 0);
  }
  end_test_case();

  test_case("no file was dropped because an allocation failed");
  {
    require(wst.alloc_failed, 0ull);
  }
  end_test_case();

  test_case("readdir reported a usable d_type for every entry");
  {
    require(wst.unknown_type, 0ull);
  }
  end_test_case();

  test_case("the corpus is little-endian only, as the oracle assumes");
  {
    require(wst.msb, 0ull);
  }
  end_test_case();

  test_case("non-ELF files were seen and every one of them was rejected by open_image");
  {
    require_true(wst.non_elf > 0);
  }
  end_test_case();

  for ( usize i = 0; i < static_cast<usize>(fc::count_); i++ ) {
    micron::sstring<160> lbl{};
    path_append(lbl, "no corpus failures in category: ");
    path_append(lbl, fc_name[i]);
    lbl.null_term();
    test_case(lbl.c_str());
    require(flog.hits[i], 0ull);
    end_test_case();
  }

  test_case("the corpus sweep produced zero failures in total");
  {
    require(flog.total, 0ull);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
