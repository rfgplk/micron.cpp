

#include "../../src/linux/elf/read.hpp"

#include "../../src/io/console.hpp"
#include "../../src/memory/cstring.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace me = micron::elf;
namespace mr = micron::elf::read;

namespace
{

constexpr usize buf_cap = 1024;
u8 img_buf[buf_cap] = {};
usize img_len = 0;

void
w8(usize at, u8 v)
{
  img_buf[at] = v;
}

void
w16(usize at, u16 v)
{
  img_buf[at] = static_cast<u8>(v & 0xff);
  img_buf[at + 1] = static_cast<u8>((v >> 8) & 0xff);
}

void
w32(usize at, u32 v)
{
  for ( usize i = 0; i < 4; ++i ) img_buf[at + i] = static_cast<u8>((v >> (8 * i)) & 0xff);
}

constexpr usize off_ehdr = 0;
constexpr usize off_phdr = 52;
constexpr usize n_phdr = 2;
constexpr usize off_dynstr = 128;
constexpr usize sz_dynstr = 48;
constexpr usize off_dynamic = 176;
constexpr usize n_dyn = 5;
constexpr usize sz_dynamic = n_dyn * 8;
constexpr usize off_rel = 224;
constexpr usize n_rel = 3;
constexpr usize sz_rel = n_rel * 8;
constexpr usize off_relr = 256;
constexpr usize n_relr = 3;
constexpr usize sz_relr = n_relr * 4;
constexpr usize off_shstr = 288;
constexpr usize sz_shstr = 48;
constexpr usize off_shdr = 352;
constexpr usize n_shdr = 6;
constexpr usize sz_shdr = 40;

constexpr u32 str_soname = 1;
constexpr u32 str_needed = 15;

constexpr u32 shs_dynstr = 1;
constexpr u32 shs_dynamic = 9;
constexpr u32 shs_rel = 18;
constexpr u32 shs_relr = 27;
constexpr u32 shs_shstrtab = 37;

void
build_image()
{
  for ( usize i = 0; i < buf_cap; ++i ) img_buf[i] = 0;

  w8(off_ehdr + me::ei_mag0, me::mag0);
  w8(off_ehdr + me::ei_mag1, static_cast<u8>(me::mag1));
  w8(off_ehdr + me::ei_mag2, static_cast<u8>(me::mag2));
  w8(off_ehdr + me::ei_mag3, static_cast<u8>(me::mag3));
  w8(off_ehdr + me::ei_class, me::elfclass32);
  w8(off_ehdr + me::ei_data, me::elfdata2lsb);
  w8(off_ehdr + me::ei_version, static_cast<u8>(me::ev_current));
  w8(off_ehdr + me::ei_osabi, me::elfosabi_gnu);

  w16(off_ehdr + 16, me::et_dyn);
  w16(off_ehdr + 18, me::em_386);
  w32(off_ehdr + 20, me::ev_current);
  w32(off_ehdr + 24, 0x1000u);
  w32(off_ehdr + 28, off_phdr);
  w32(off_ehdr + 32, off_shdr);
  w32(off_ehdr + 36, 0u);
  w16(off_ehdr + 40, 52);
  w16(off_ehdr + 42, 32);
  w16(off_ehdr + 44, n_phdr);
  w16(off_ehdr + 46, sz_shdr);
  w16(off_ehdr + 48, n_shdr);
  w16(off_ehdr + 50, 5);

  usize p = off_phdr;
  w32(p + 0, me::pt_load);
  w32(p + 4, 0u);
  w32(p + 8, 0u);
  w32(p + 12, 0u);
  w32(p + 16, buf_cap);
  w32(p + 20, buf_cap + 0x1000u);
  w32(p + 24, me::pf_r | me::pf_x);
  w32(p + 28, 0x1000u);

  p = off_phdr + 32;
  w32(p + 0, me::pt_dynamic);
  w32(p + 4, off_dynamic);
  w32(p + 8, off_dynamic);
  w32(p + 12, off_dynamic);
  w32(p + 16, sz_dynamic);
  w32(p + 20, sz_dynamic);
  w32(p + 24, me::pf_r | me::pf_w);
  w32(p + 28, 4u);

  const char *s0 = "\0libsynth.so.1\0libc.so.6\0";
  for ( usize i = 0; i < 25; ++i ) img_buf[off_dynstr + i] = static_cast<u8>(s0[i]);

  p = off_dynamic;
  w32(p + 0, static_cast<u32>(me::dt_soname));
  w32(p + 4, str_soname);
  w32(p + 8, static_cast<u32>(me::dt_needed));
  w32(p + 12, str_needed);
  w32(p + 16, static_cast<u32>(me::dt_strtab));
  w32(p + 20, off_dynstr);
  w32(p + 24, static_cast<u32>(me::dt_strsz));
  w32(p + 28, sz_dynstr);
  w32(p + 32, static_cast<u32>(me::dt_null));
  w32(p + 36, 0u);

  p = off_rel;
  w32(p + 0, 0x1100u);
  w32(p + 4, me::elf32_r_info(5, 1));
  w32(p + 8, 0x1200u);
  w32(p + 12, me::elf32_r_info(0, 8));
  w32(p + 16, 0x1300u);
  w32(p + 20, me::elf32_r_info(0x1234, 7));

  w32(off_relr + 0, 0x1000u);
  w32(off_relr + 4, 0x00000007u);
  w32(off_relr + 8, 0x00000003u);

  const char *s1 = "\0.dynstr\0.dynamic\0.rel.dyn\0.relr.dyn\0.shstrtab\0";
  for ( usize i = 0; i < 47; ++i ) img_buf[off_shstr + i] = static_cast<u8>(s1[i]);

  auto shdr = [](usize idx, u32 name, u32 type, u32 addr, u32 offset, u32 size, u32 link, u32 entsize) {
    const usize q = off_shdr + idx * sz_shdr;
    w32(q + 0, name);
    w32(q + 4, type);
    w32(q + 8, me::shf_alloc);
    w32(q + 12, addr);
    w32(q + 16, offset);
    w32(q + 20, size);
    w32(q + 24, link);
    w32(q + 28, 0u);
    w32(q + 32, 4u);
    w32(q + 36, entsize);
  };
  shdr(0, 0, me::sht_null, 0, 0, 0, 0, 0);
  shdr(1, shs_dynstr, me::sht_strtab, off_dynstr, off_dynstr, sz_dynstr, 0, 0);
  shdr(2, shs_dynamic, me::sht_dynamic, off_dynamic, off_dynamic, sz_dynamic, 1, 8);
  shdr(3, shs_rel, me::sht_rel, off_rel, off_rel, sz_rel, 0, 8);
  shdr(4, shs_relr, me::sht_relr, off_relr, off_relr, sz_relr, 0, 4);
  shdr(5, shs_shstrtab, me::sht_strtab, off_shstr, off_shstr, sz_shstr, 0, 0);

  img_len = off_shdr + n_shdr * sz_shdr;
}

bool
file_present(const char *path)
{
  auto r = mr::source::open(path);
  return r.is_first();
}

}      // namespace

int
main()
{
  sb::print("=== ELF32 READER RIGOR ===");

  build_image();
  mr::source synth(img_buf, img_len);

  test_case("open_image identifies a synthetic ELF32 as elf32/lsb/386/dyn");
  {
    auto r = mr::open_image(synth);
    require_true(r.is_first());
    const mr::image &img = r.cast<mr::image>();
    require_true(img.ok());
    require_false(img.is64());
    require(img.hdr.cls == me::fmt_class::elf32);
    require(img.hdr.data == me::fmt_data::lsb);
    require(img.hdr.type, static_cast<u16>(me::et_dyn));
    require(img.hdr.machine, static_cast<u16>(me::em_386));
    require(img.hdr.osabi, static_cast<u8>(me::elfosabi_gnu));
    require(img.hdr.entry, 0x1000ull);
    require(img.hdr.phnum, static_cast<u16>(n_phdr));
    require(img.hdr.shnum, static_cast<u16>(n_shdr));
    require(img.hdr.phentsize, static_cast<u16>(32));
    require(img.hdr.shentsize, static_cast<u16>(40));
  }
  end_test_case();

  test_case("phdr32 p_flags is read at field 7, not the ELF64 field-2 position");
  {
    auto r = mr::open_image(synth);
    require_true(r.is_first());
    const auto segs = mr::walk_segments(r.cast<mr::image>());
    require(segs.size(), static_cast<usize>(2));

    require(segs[0].type, static_cast<u32>(me::pt_load));
    require(segs[0].flags, static_cast<u32>(me::pf_r | me::pf_x));
    require(segs[0].offset, 0ull);
    require(segs[0].filesz, static_cast<u64>(buf_cap));
    require_true(segs[0].memsz > segs[0].filesz);
    require(segs[0].align, 0x1000ull);

    require(segs[1].type, static_cast<u32>(me::pt_dynamic));
    require(segs[1].flags, static_cast<u32>(me::pf_r | me::pf_w));
    require(segs[1].offset, static_cast<u64>(off_dynamic));
    require(segs[1].vaddr, static_cast<u64>(off_dynamic));
  }
  end_test_case();

  test_case("vaddr_to_offset maps through PT_LOAD");
  {
    auto r = mr::open_image(synth);
    const auto segs = mr::walk_segments(r.cast<mr::image>());
    auto off = mr::vaddr_to_offset(segs, off_dynstr);
    require_true(off.is_first());
    require(off.cast<u64>(), static_cast<u64>(off_dynstr));

    require_true(mr::vaddr_to_offset(segs, buf_cap + 16).is_second());
  }
  end_test_case();

  test_case("sections resolve their names through e_shstrndx");
  {
    auto r = mr::open_image(synth);
    const auto secs = mr::walk_sections(r.cast<mr::image>());
    require(secs.size(), static_cast<usize>(n_shdr));
    require_true(micron::strcmp(secs[1].name.c_str(), ".dynstr") == 0);
    require_true(micron::strcmp(secs[3].name.c_str(), ".rel.dyn") == 0);
    require_true(micron::strcmp(secs[4].name.c_str(), ".relr.dyn") == 0);
    require_true(mr::find_section(secs, ".dynamic") != nullptr);
    require_true(mr::find_section(secs, ".not.here") == nullptr);
    require_true(mr::find_section_by_type(secs, me::sht_relr) != nullptr);
  }
  end_test_case();

  test_case("REL rows use the ELF32 r_info split, not the ELF64 one");
  {
    auto r = mr::open_image(synth);
    const mr::image &img = r.cast<mr::image>();
    const auto secs = mr::walk_sections(img);
    const mr::section_row *rel = mr::find_section(secs, ".rel.dyn");
    require_true(rel != nullptr);

    const auto rows = mr::walk_relocs(img, *rel);
    require(rows.size(), static_cast<usize>(n_rel));

    require(rows[0].offset, 0x1100ull);
    require(rows[0].sym, 5u);
    require(rows[0].type, 1u);
    require_false(rows[0].has_addend);

    require(rows[1].offset, 0x1200ull);
    require(rows[1].sym, 0u);
    require(rows[1].type, 8u);

    require(rows[2].offset, 0x1300ull);
    require(rows[2].sym, 0x1234u);
    require(rows[2].type, 7u);

    require(rows[0].addend, static_cast<i64>(0));
    require(rows[2].addend, static_cast<i64>(0));
  }
  end_test_case();

  test_case("RELR bitmaps stride 31 slots on ELF32, not 63");
  {
    auto r = mr::open_image(synth);
    const mr::image &img = r.cast<mr::image>();
    const auto secs = mr::walk_sections(img);
    const mr::section_row *relr = mr::find_section(secs, ".relr.dyn");
    require_true(relr != nullptr);

    const auto out = mr::walk_relr(img, *relr);
    require_false(out.capped);
    require(out.at.size(), static_cast<usize>(4));
    require(out.at[0], 0x1000ull);
    require(out.at[1], 0x1004ull);
    require(out.at[2], 0x1008ull);

    require(out.at[3], 0x1080ull);
  }
  end_test_case();

  test_case("dyn32 d_tag is signed, and DT_STRTAB resolves through PT_LOAD");
  {
    auto r = mr::open_image(synth);
    const mr::image &img = r.cast<mr::image>();
    const auto segs = mr::walk_segments(img);
    const auto secs = mr::walk_sections(img);
    const auto dyn = mr::read_dynamic(img, segs, secs);

    require_true(dyn.present);
    require_false(dyn.from_section);
    require(dyn.entries.size(), static_cast<usize>(n_dyn));
    require(dyn.entries[0].tag, static_cast<i64>(me::dt_soname));
    require(dyn.entries[4].tag, static_cast<i64>(me::dt_null));
    require_true(micron::strcmp(dyn.soname.c_str(), "libsynth.so.1") == 0);
    require(dyn.needed.size(), static_cast<usize>(1));
    require_true(micron::strcmp(dyn.needed[0].c_str(), "libc.so.6") == 0);
    require_true(dyn.rpath.empty());
    require_true(dyn.runpath.empty());
  }
  end_test_case();

  test_case("a truncated or non-elf buffer is rejected, never read past");
  {
    mr::source empty_src(img_buf, 4);
    require_true(mr::open_image(empty_src).is_second());

    u8 junk[64] = {};
    mr::source junk_src(junk, sizeof(junk));
    require_true(mr::open_image(junk_src).is_second());

    u8 badclass[64] = {};
    badclass[0] = me::mag0;
    badclass[1] = 'E';
    badclass[2] = 'L';
    badclass[3] = 'F';
    badclass[me::ei_class] = 7;
    mr::source bad_src(badclass, sizeof(badclass));
    require_true(mr::open_image(bad_src).is_second());

    mr::source short_src(img_buf, 40);
    require_true(mr::open_image(short_src).is_second());
  }
  end_test_case();

  test_case("spans clip at the end of the source rather than running off it");
  {
    mr::source s(img_buf, 32);
    require(s.at(0, 64).len, static_cast<usize>(32));
    require(s.at(30, 64).len, static_cast<usize>(2));
    require(s.at(32, 8).len, static_cast<usize>(0));
    require(s.at(1000, 8).len, static_cast<usize>(0));
    require_true(s.at(32, 8).ptr == nullptr);
  }
  end_test_case();

  const char *l32 = "/usr/lib/libwayland-client.so.0";
  const char *l64 = "/lib64/libwayland-client.so.0";

  if ( !file_present(l32) || !file_present(l64) ) {
    sb::print("multilib libwayland-client.so.0 not present on this box -- real-file half skipped");
    sb::print("=== ALL TESTS PASSED ===");
    return 1;
  }

  test_case("a real i386 .so reads as elf32/386 and is NOT native on a 64-bit build");
  {
    auto sr = mr::source::open(l32);
    require_true(sr.is_first());
    const mr::source &src = sr.cast<mr::source>();
    auto ir = mr::open_image(src);
    require_true(ir.is_first());
    const mr::image &img = ir.cast<mr::image>();

    require(img.hdr.cls == me::fmt_class::elf32, true);
    require(img.hdr.machine, static_cast<u16>(me::em_386));
    require(img.hdr.type, static_cast<u16>(me::et_dyn));
    require(img.hdr.phentsize, static_cast<u16>(sizeof(me::phdr32_t)));
    require(img.hdr.shentsize, static_cast<u16>(sizeof(me::shdr32_t)));
    require_true(img.is_native() == (me::native_class == me::fmt_class::elf32 && me::expected_machine == me::em_386));
  }
  end_test_case();

  test_case("a real i386 .so: 142 dynsyms, 59 .rel.dyn rows, all REL, all R_386");
  {
    auto sr = mr::source::open(l32);
    const mr::source &src = sr.cast<mr::source>();
    auto ir = mr::open_image(src);
    const mr::image &img = ir.cast<mr::image>();
    const auto secs = mr::walk_sections(img);

    const mr::section_row *dynsym = mr::find_section_by_type(secs, me::sht_dynsym);
    require_true(dynsym != nullptr);
    require(dynsym->size / sizeof(me::sym32_t), static_cast<u64>(142));

    const mr::section_row *rel = mr::find_section(secs, ".rel.dyn");
    require_true(rel != nullptr);
    const auto rows = mr::walk_relocs(img, *rel);
    require(rows.size(), static_cast<usize>(59));

    bool any_nonzero_sym = false;
    for ( const auto &r : rows ) {
      require_false(r.has_addend);

      require_true(r.type < 64u);
      require_true(r.sym < 142u);
      if ( r.sym != 0 ) any_nonzero_sym = true;
    }

    require_true(any_nonzero_sym);
  }
  end_test_case();

  test_case("a real i386 .so: .rel.plt is 75 REL rows and DT_PLTREL says DT_REL");
  {
    auto sr = mr::source::open(l32);
    const mr::source &src = sr.cast<mr::source>();
    auto ir = mr::open_image(src);
    const mr::image &img = ir.cast<mr::image>();
    const auto segs = mr::walk_segments(img);
    const auto secs = mr::walk_sections(img);

    const mr::section_row *relplt = mr::find_section(secs, ".rel.plt");
    require_true(relplt != nullptr);
    require(mr::walk_relocs(img, *relplt).size(), static_cast<usize>(75));

    const auto dyn = mr::read_dynamic(img, segs, secs);
    require_true(dyn.present);
    bool saw_pltrel = false;
    for ( const auto &e : dyn.entries ) {
      if ( e.tag == me::dt_pltrel ) {
        require(static_cast<i64>(e.val), static_cast<i64>(me::dt_rel));
        saw_pltrel = true;
      }
    }
    require_true(saw_pltrel);
  }
  end_test_case();

  test_case("a real i386 .so: soname, DT_NEEDED, and a 4-byte-entry .relr.dyn");
  {
    auto sr = mr::source::open(l32);
    const mr::source &src = sr.cast<mr::source>();
    auto ir = mr::open_image(src);
    const mr::image &img = ir.cast<mr::image>();
    const auto segs = mr::walk_segments(img);
    const auto secs = mr::walk_sections(img);
    const auto dyn = mr::read_dynamic(img, segs, secs);

    require_true(micron::strcmp(dyn.soname.c_str(), "libwayland-client.so.0") == 0);
    bool saw_libc = false;
    for ( const auto &n : dyn.needed )
      if ( micron::strcmp(n.c_str(), "libc.so.6") == 0 ) saw_libc = true;
    require_true(saw_libc);

    const mr::section_row *relr = mr::find_section(secs, ".relr.dyn");
    require_true(relr != nullptr);
    require(relr->entsize, static_cast<u64>(4));
    const auto out = mr::walk_relr(img, *relr);
    require_false(out.capped);
    require_true(out.at.size() > 0);

    u64 hi = 0;
    for ( const auto &s : segs )
      if ( s.type == me::pt_load && s.vaddr + s.memsz > hi ) hi = s.vaddr + s.memsz;
    for ( u64 a : out.at ) require_true(a < hi);
  }
  end_test_case();

  test_case("a real i386 .so: versym has one entry per dynsym, verneed names GLIBC_");
  {
    auto sr = mr::source::open(l32);
    const mr::source &src = sr.cast<mr::source>();
    auto ir = mr::open_image(src);
    const mr::image &img = ir.cast<mr::image>();
    const auto secs = mr::walk_sections(img);

    const mr::section_row *vs = mr::find_section_by_type(secs, me::sht_gnu_versym);
    require_true(vs != nullptr);
    require(mr::read_versym(img, *vs).size(), static_cast<usize>(142));

    const mr::section_row *vn = mr::find_section_by_type(secs, me::sht_gnu_verneed);
    require_true(vn != nullptr);
    const auto needs = mr::read_verneed(img, secs, *vn);
    require_true(needs.size() > 0);
    bool saw_glibc = false;
    for ( const auto &v : needs ) {
      require_true(v.is_need);
      if ( micron::strncmp(v.name.c_str(), "GLIBC_", 6) == 0 ) saw_glibc = true;
    }
    require_true(saw_glibc);
  }
  end_test_case();

  test_case("the same soname in the other class reads as elf64/RELA");
  {
    auto sr = mr::source::open(l64);
    require_true(sr.is_first());
    const mr::source &src = sr.cast<mr::source>();
    auto ir = mr::open_image(src);
    require_true(ir.is_first());
    const mr::image &img = ir.cast<mr::image>();

    require_true(img.is64());
    require(img.hdr.machine, static_cast<u16>(me::em_x86_64));
    require(img.hdr.phentsize, static_cast<u16>(sizeof(me::phdr_t)));

    const auto segs = mr::walk_segments(img);
    const auto secs = mr::walk_sections(img);
    require_true(mr::find_segment(segs, me::pt_load) != nullptr);
    require_true(mr::find_segment(segs, me::pt_dynamic) != nullptr);

    const mr::section_row *rela = mr::find_section(secs, ".rela.dyn");
    require_true(rela != nullptr);
    const auto rows = mr::walk_relocs(img, *rela);
    require_true(rows.size() > 0);
    for ( const auto &r : rows ) {
      require_true(r.has_addend);
      require_true(r.type < 64u);
    }

    const auto dyn = mr::read_dynamic(img, segs, secs);
    require_true(micron::strcmp(dyn.soname.c_str(), "libwayland-client.so.0") == 0);
    for ( const auto &e : dyn.entries )
      if ( e.tag == me::dt_pltrel ) require(static_cast<i64>(e.val), static_cast<i64>(me::dt_rela));
  }
  end_test_case();

  test_case("both classes agree on what the library exports");
  {
    auto s32 = mr::source::open(l32);
    auto s64 = mr::source::open(l64);
    auto i32r = mr::open_image(s32.cast<mr::source>());
    auto i64r = mr::open_image(s64.cast<mr::source>());
    const mr::image &a = i32r.cast<mr::image>();
    const mr::image &b = i64r.cast<mr::image>();

    const auto syms_a = mr::walk_symbols(a, mr::walk_sections(a));
    const auto syms_b = mr::walk_symbols(b, mr::walk_sections(b));
    require_true(syms_a.size() > 0);
    require_true(syms_b.size() > 0);

    auto has_defined_func = [](const auto &v, const char *name) {
      for ( const auto &s : v )
        if ( micron::strcmp(s.name.c_str(), name) == 0 && s.defined && s.type == me::stt_func ) return true;
      return false;
    };

    require_true(has_defined_func(syms_a, "wl_display_connect"));
    require_true(has_defined_func(syms_b, "wl_display_connect"));
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
