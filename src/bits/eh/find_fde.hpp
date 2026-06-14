//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh_dwarf)

#include "../../types.hpp"
#include "dwarf_enc.hpp"
#include "eh_debug.hpp"
#include "unwind.hpp"

// rerolling src/linux/elf we can't include it here

// the ELF header of the running executable, provided by the linker
extern "C" const byte __ehdr_start[] __attribute__((visibility("hidden")));

namespace micron::eh
{

#if defined(__micron_arch_width_64)
struct elf_ehdr {
  byte e_ident[16];
  u16 e_type;
  u16 e_machine;
  u32 e_version;
  u64 e_entry;
  u64 e_phoff;
  u64 e_shoff;
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
};

struct elf_phdr {
  u32 p_type;
  u32 p_flags;
  u64 p_offset;
  u64 p_vaddr;
  u64 p_paddr;
  u64 p_filesz;
  u64 p_memsz;
  u64 p_align;
};
#else
struct elf_ehdr {
  byte e_ident[16];
  u16 e_type;
  u16 e_machine;
  u32 e_version;
  u32 e_entry;
  u32 e_phoff;
  u32 e_shoff;
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
};

struct elf_phdr {
  u32 p_type;
  u32 p_offset;
  u32 p_vaddr;
  u32 p_paddr;
  u32 p_filesz;
  u32 p_memsz;
  u32 p_flags;
  u32 p_align;
};
#endif

constexpr u32 PT_LOAD = 1;
constexpr u32 PT_PHDR = 6;
constexpr u32 PT_GNU_EH_FRAME = 0x6474e550;

struct cie_info {
  u64 code_align;
  i64 data_align;
  u32 ra_column;
  u8 fde_enc;
  u8 lsda_enc;
  bool has_aug_data;
  bool has_lsda;
  bool signal_frame;
  _Unwind_Personality_Fn personality;
  const byte *cfi_begin;
  const byte *cfi_end;
};

struct fde_info {
  usize func_start;
  usize func_end;
  const byte *lsda;
  const byte *cfi_begin;
  const byte *cfi_end;
  cie_info cie;
};

struct eh_frame_state {
  const byte *eh_frame_hdr = nullptr;
  const byte *search_table = nullptr;
  usize fde_count = 0;
  u8 table_enc = DW_EH_PE_omit;
  usize hdr_base = 0;
  usize text_base = 0;
  usize data_base = 0;
  bool initialized = false;
  bool valid = false;
};

inline eh_frame_state &
__eh_state() noexcept
{
  static eh_frame_state s;
  return s;
}

inline void
register_eh_frame() noexcept
{
  eh_frame_state &s = __eh_state();
  if ( s.initialized ) return;
  s.initialized = true;

  const elf_ehdr *eh = reinterpret_cast<const elf_ehdr *>(__ehdr_start);
  const byte *phbase = __ehdr_start + eh->e_phoff;

  usize bias = 0;
  for ( u16 i = 0; i < eh->e_phnum; ++i ) {
    const elf_phdr *ph = reinterpret_cast<const elf_phdr *>(phbase + static_cast<usize>(i) * eh->e_phentsize);
    if ( ph->p_type == PT_PHDR ) {
      bias = reinterpret_cast<usize>(phbase) - static_cast<usize>(ph->p_vaddr);
      break;
    }
  }

  const elf_phdr *eh_frame_ph = nullptr;
  for ( u16 i = 0; i < eh->e_phnum; ++i ) {
    const elf_phdr *ph = reinterpret_cast<const elf_phdr *>(phbase + static_cast<usize>(i) * eh->e_phentsize);
    if ( ph->p_type == PT_GNU_EH_FRAME ) eh_frame_ph = ph;
    if ( ph->p_type == PT_LOAD && (ph->p_flags & 0x1 /*X*/) && s.text_base == 0 ) s.text_base = static_cast<usize>(ph->p_vaddr) + bias;
  }
  if ( !eh_frame_ph ) return;

  const byte *hdr = reinterpret_cast<const byte *>(static_cast<usize>(eh_frame_ph->p_vaddr) + bias);
  s.eh_frame_hdr = hdr;
  s.hdr_base = reinterpret_cast<usize>(hdr);
  s.data_base = bias;

  const byte *p = hdr;
  const u8 version = *p++;
  const u8 eh_frame_ptr_enc = *p++;
  const u8 fde_count_enc = *p++;
  const u8 table_enc = *p++;
  if ( version != 1 ) return;

  decode_ctx ctx{ 0, s.text_base, s.hdr_base, 0 };
  (void)read_encoded(eh_frame_ptr_enc, p, ctx);
  const usize count = read_encoded(fde_count_enc, p, ctx);

  if ( fde_count_enc == DW_EH_PE_omit || count == 0 ) return;

  s.search_table = p;
  s.fde_count = count;
  s.table_enc = table_enc;
  s.valid = true;

  __dbg_kv("[ehframe] hdr=", reinterpret_cast<usize>(hdr));
  __dbg_kv("[ehframe] count=", s.fde_count);
  __dbg_kv("[ehframe] table_enc=", s.table_enc);
  __dbg_kv("[ehframe] text_base=", s.text_base);
}

inline bool
parse_cie(const byte *cie_record, const eh_frame_state &s, cie_info &out) noexcept
{
  const byte *p = cie_record;
  u32 length = read_unaligned<u32>(p);
  p += 4;
  if ( length == 0 ) return false;
  if ( length == 0xffffffffu ) return false;      // 64-bit DWARF unsupported (not emitted by gcc here)
  const byte *cie_end = p + length;

  p += 4;
  const u8 ver = *p++;
  const char *aug = reinterpret_cast<const char *>(p);
  while ( *p ) ++p;
  ++p;

  if ( ver >= 4 ) {
    ++p;
    ++p;
  }

  out.code_align = read_uleb128(p);
  out.data_align = read_sleb128(p);
  out.ra_column = (ver == 1) ? static_cast<u32>(*p++) : static_cast<u32>(read_uleb128(p));

  out.fde_enc = DW_EH_PE_absptr;
  out.lsda_enc = DW_EH_PE_omit;
  out.has_aug_data = (aug[0] == 'z');
  out.has_lsda = false;
  out.signal_frame = false;
  out.personality = nullptr;

  if ( aug[0] == 'z' ) {
    const u64 aug_len = read_uleb128(p);
    const byte *aug_end = p + aug_len;
    decode_ctx ctx{ 0, s.text_base, s.data_base, 0 };
    for ( const char *a = aug + 1; *a; ++a ) {
      switch ( *a ) {
      case 'L':
        out.lsda_enc = *p++;
        out.has_lsda = true;
        break;
      case 'P': {
        const u8 penc = *p++;
        const usize pv = read_encoded(penc, p, ctx);
        out.personality = reinterpret_cast<_Unwind_Personality_Fn>(pv);
        break;
      }
      case 'R':
        out.fde_enc = *p++;
        break;
      case 'S':
        out.signal_frame = true;
        break;
      default:
        a = ""; /* unknown: stop, rely on aug_end */
        p = aug_end;
        break;
      }
    }
    p = aug_end;
  }

  out.cfi_begin = p;
  out.cfi_end = cie_end;
  return true;
}

inline bool
parse_fde(const byte *fde_record, const eh_frame_state &s, fde_info &out) noexcept
{
  const byte *p = fde_record;
  const u32 length = read_unaligned<u32>(p);
  p += 4;
  if ( length == 0 || length == 0xffffffffu ) return false;
  const byte *fde_end = p + length;

  const byte *cie_ptr_field = p;
  const u32 cie_off = read_unaligned<u32>(p);
  p += 4;
  if ( cie_off == 0 ) return false;      // this is a CIE, not an FDE
  const byte *cie_record = cie_ptr_field - cie_off;

  if ( !parse_cie(cie_record, s, out.cie) ) return false;

  decode_ctx ctx{ 0, s.text_base, s.data_base, 0 };
  const byte *pcfield = p;
  const usize initial_loc = read_encoded(out.cie.fde_enc, p, ctx);
  // address_range uses the same value format but is always absolute (no app base)
  const u8 range_enc = static_cast<u8>(out.cie.fde_enc & 0x0f);
  const usize range = read_encoded(range_enc, p, ctx);
  (void)pcfield;

  out.func_start = initial_loc;
  out.func_end = initial_loc + range;
  out.lsda = nullptr;

  if ( out.cie.signal_frame ) { /* handled by the CFI step (ip_before_insn) */
  }

  if ( out.cie.has_aug_data ) {
    const byte *q = p;
    const u64 aug_len = read_uleb128(q);
    const byte *aug_end = q + aug_len;
    if ( out.cie.has_lsda && out.cie.lsda_enc != DW_EH_PE_omit ) {
      decode_ctx lctx{ 0, s.text_base, s.data_base, out.func_start };
      const usize lsda = read_encoded(out.cie.lsda_enc, q, lctx);
      out.lsda = lsda ? reinterpret_cast<const byte *>(lsda) : nullptr;
    }
    p = aug_end;
  }

  out.cfi_begin = p;
  out.cfi_end = fde_end;
  return true;
}

inline bool
find_fde(usize pc, fde_info &out) noexcept
{
  eh_frame_state &s = __eh_state();
  if ( !s.initialized ) register_eh_frame();
  if ( !s.valid ) return false;

  const usize entry_field_sz = encoded_size(s.table_enc);
  if ( entry_field_sz == 0 ) return false;      // only fixed-width tables supported
  const usize entry_sz = entry_field_sz * 2;

  usize lo = 0, hi = s.fde_count;
  const byte *table = s.search_table;
  decode_ctx ctx{ 0, s.text_base, s.hdr_base, 0 };

  auto entry_loc = [&](usize idx) -> usize {
    const byte *e = table + idx * entry_sz;
    return read_encoded(s.table_enc, e, ctx);
  };

  if ( pc < entry_loc(0) ) return false;
  while ( hi - lo > 1 ) {
    const usize mid = lo + (hi - lo) / 2;
    if ( entry_loc(mid) <= pc )
      lo = mid;
    else
      hi = mid;
  }

  const byte *e = table + lo * entry_sz;
  (void)read_encoded(s.table_enc, e, ctx);      // skip initial_location
  const usize fde_addr = read_encoded(s.table_enc, e, ctx);
  if ( !fde_addr ) return false;

  if ( !parse_fde(reinterpret_cast<const byte *>(fde_addr), s, out) ) return false;
  __dbg_kv("[find_fde] pc=", pc);
  __dbg_kv("  func_start=", out.func_start);
  __dbg_kv("  func_end=", out.func_end);
  __dbg_kv("  personality=", reinterpret_cast<usize>(out.cie.personality));
  __dbg_kv("  lsda=", reinterpret_cast<usize>(out.lsda));
  if ( pc < out.func_start || pc >= out.func_end ) return false;
  return true;
}

};      // namespace micron::eh

#endif      // __micron_eh_dwarf
