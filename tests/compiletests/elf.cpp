//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Compile coverage for the whole ELF layer: built across the arch x opt x freestanding matrix,
// never run. The rows that matter most are --i386 and --arm, because until 2026-08 both
// linux/elf/header.hpp and header32.hpp #error'd on anything but a 64-bit host -- so the layer did
// not exist on half the arches micron claims to support, and nothing in the matrix noticed.
//
// Also instantiated here: the elf::read walkers for BOTH classes on every target, so a 64-bit build
// really emits the ELF32 paths (and vice versa) rather than merely parsing them.

#include "../../src/elf.hpp"

#include "../../src/linux/dynamic.hpp"
#include "../../src/linux/elf/host_modules.hpp"
#include "../../src/linux/elf/read.hpp"

namespace me = micron::elf;

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the on-disk record sizes, checked at compile time on every cell of the matrix. these are ABI, not
// implementation detail: elf::read walks the tables by byte offset, so a padded struct silently
// reads every field after the first one from the wrong place.

static_assert(sizeof(me::ehdr32_t) == 52);
static_assert(sizeof(me::phdr32_t) == 32);
static_assert(sizeof(me::shdr32_t) == 40);
static_assert(sizeof(me::dyn32_t) == 8);
static_assert(sizeof(me::sym32_t) == 16);
static_assert(sizeof(me::rel32_t) == 8);
static_assert(sizeof(me::rela32_t) == 12);

static_assert(sizeof(me::ehdr_t) == 64);
static_assert(sizeof(me::phdr_t) == 56);
static_assert(sizeof(me::shdr_t) == 64);
static_assert(sizeof(me::dyn_t) == 16);
static_assert(sizeof(me::sym_t) == 24);
static_assert(sizeof(me::rel_t) == 16);
static_assert(sizeof(me::rela_t) == 24);

// the version records are class-invariant; header.hpp static_asserts these too, repeated here
// because it is the property elf::read's chain walk depends on
static_assert(sizeof(me::verdef_t) == 20);
static_assert(sizeof(me::verdaux_t) == 8);
static_assert(sizeof(me::verneed_t) == 16);
static_assert(sizeof(me::vernaux_t) == 16);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the three class forks that are NOT typedef changes. get any of them wrong and the loader writes
// plausible garbage instead of failing, so they are asserted rather than tested.

using t32 = me::elf_traits<me::fmt_class::elf32>;
using t64 = me::elf_traits<me::fmt_class::elf64>;

// r_info packs differently per class: (sym << 8 | type) vs (sym << 32 | type)
static_assert(t32::r_sym(0x00123456u) == 0x1234u);
static_assert(t32::r_type(0x00123456u) == 0x56u);
static_assert(t64::r_sym(0x0000001200000034ull) == 0x12u);
static_assert(t64::r_type(0x0000001200000034ull) == 0x34u);
static_assert(me::elf32_r_info(0x1234u, 0x56u) == 0x00123456u);
static_assert(me::elf64_r_info(0x12u, 0x34u) == 0x0000001200000034ull);

// DT_GNU_HASH bloom words: 32-bit on ELF32, 64-bit on ELF64
static_assert(t32::bloom_bits == 32);
static_assert(t64::bloom_bits == 64);
static_assert(sizeof(t32::uword) == 4);
static_assert(sizeof(t64::uword) == 8);

// DT_RELR bitmap span: one word names 31 slots on ELF32 and 63 on ELF64
static_assert(t32::relr_bits == 31);
static_assert(t64::relr_bits == 63);

// the native class must agree with the pointer width, on every arch
static_assert(me::native_class == (sizeof(void *) == 8 ? me::fmt_class::elf64 : me::fmt_class::elf32));
static_assert(sizeof(me::native_traits::addr) == sizeof(void *));
static_assert(me::expected_machine != 0, "every arch micron builds for needs a loader backend");

// st_info / st_other splits are class-invariant
static_assert(me::elf_st_bind(0x12) == 1);
static_assert(me::elf_st_type(0x12) == 2);
static_assert(me::elf_st_visibility(0x03) == me::stv_protected);
static_assert(me::elf_ver_ndx(0x8003) == 3);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// force the emission of both class instantiations of every reader on every target

// the reader dispatches on img.hdr.cls at RUNTIME -- a 64-bit build must read an ELF32 file, so
// this is deliberately not a compile-time axis. driving it with both classes here is what forces
// both sides of every is64() branch to be emitted on every target.
void
instantiate_reader(me::read::source &src, me::fmt_class cls)
{
  me::read::image img{};
  img.src = &src;
  img.hdr.cls = cls;
  img.hdr.data = me::fmt_data::lsb;
  img.hdr.phentsize = static_cast<u16>(cls == me::fmt_class::elf64 ? sizeof(me::phdr_t) : sizeof(me::phdr32_t));
  img.hdr.shentsize = static_cast<u16>(cls == me::fmt_class::elf64 ? sizeof(me::shdr_t) : sizeof(me::shdr32_t));
  const auto segs = me::read::walk_segments(img);
  const auto secs = me::read::walk_sections(img);
  (void)me::read::walk_symbols(img, secs);
  (void)me::read::read_dynamic(img, segs, secs);
  (void)me::read::read_interp(img, segs);
  (void)me::read::classify_link(segs);
  for ( const auto &s : secs ) {
    (void)me::read::walk_relocs(img, s);
    (void)me::read::walk_relr(img, s);
    (void)me::read::read_versym(img, s);
    (void)me::read::read_verdef(img, secs, s);
    (void)me::read::read_verneed(img, secs, s);
  }
  (void)me::read::find_segment(segs, me::pt_dynamic);
  (void)me::read::find_section(secs, ".dynstr");
  (void)me::read::find_section_by_type(secs, me::sht_dynsym);
}

void
instantiate_names()
{
  (void)me::read::etype_name(me::et_dyn);
  (void)me::read::machine_name(me::em_386);
  (void)me::read::osabi_name(me::elfosabi_gnu);
  (void)me::read::dt_tag_name(me::dt_needed);
  // the processor range is per-machine: the same tag number is DT_ARM_EXIDX on one arch and
  // DT_X86_64_PLTSZ on another, so it needs the overload that takes e_machine
  (void)me::read::dt_tag_name(me::em_arm, me::dt_arm_exidx);
  (void)me::read::dt_tag_name(me::em_x86_64, 0x70000001);
  (void)me::read::reloc_type_name(me::em_386, 8);
  (void)me::read::reloc_type_name(me::em_arm, 23);
  (void)me::read::reloc_type_name(me::em_x86_64, 8);
  (void)me::read::reloc_type_name(me::em_aarch64, 1027);
  (void)me::read::sht_name(me::sht_relr);
  (void)me::read::pt_name(me::pt_tls);
}

// the loader surface. never called here -- linking it is the point.
void
instantiate_loader()
{
  me::dyn_info_t d{};
  (void)me::lookup_sym(d, "x");
  (void)me::gnu_lookup(d, "x");
  (void)me::sysv_lookup(d, "x");
  (void)me::count_dynsyms(d);
  (void)me::sym_version_index(d, static_cast<const me::nsym_t *>(nullptr));
  (void)me::sym_version_hidden(d, static_cast<const me::nsym_t *>(nullptr));

  // real storage, not a null deref: this file has a main() and nothing stops someone running it
  me::reloc_ctx_t ctx{};
  ctx.d = &d;
  me::reloc_view v{};
  me::nrela_t ra{};
  me::nrel_t re{};
  (void)me::reloc_view_of<me::native_class>(ra);
  (void)me::reloc_view_of<me::native_class>(re);
  // apply_reloc WRITES through load_base + r_offset, so it is emitted but never entered
  if ( ctx.load_base != nullptr ) (void)me::apply_reloc(ctx, v);

  (void)me::resolve_soname("libc.so.6", nullptr);
  (void)me::__file_is_native_elf("/dev/null");
}

}      // namespace

int
main()
{
  me::read::source src{};
  instantiate_reader(src, me::fmt_class::elf32);
  instantiate_reader(src, me::fmt_class::elf64);
  (void)me::read::open_image(src);
  (void)me::read::source::open("/dev/null");
  instantiate_names();
  instantiate_loader();
  return 1;
}
