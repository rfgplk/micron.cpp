//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/addr.hpp"

#include "bits.hpp"
#include "consts.hpp"
#include "hash.hpp"
#include "header.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// relocation
//
// RELA carries its addend; REL does not; amd64 and arm64 are RELA-only; i386 and armv7 are REL-only

namespace micron
{
namespace elf
{

struct module_t;

struct reloc_view {
  u64 offset = 0;      // r_offset, a vaddr relative to load_base
  u32 sym = 0;         // index into .dynsym; stn_undef means "no symbol"
  u32 type = 0;
  i64 addend = 0;      // authoritative only when rela is true
  bool rela = false;
};

using resolve_fn = void *(*)(void *user, const char *name, u32 sym_index);

struct reloc_ctx_t {
  u8 *load_base = nullptr;
  const dyn_info_t *d = nullptr;
  resolve_fn resolve = nullptr;
  void *user = nullptr;
  u64 tls_modid = 0;
  i64 tls_offset = 0;
};

// we'll distinguish a tolerable miss from a hard failure (thread_local et al)
enum class reloc_result : u8 {
  applied,           // written successfully
  skipped_weak,      // symbol unresolved but weak
  unresolved,        // non-weak symbol could not be resolved
  unsupported,       // relocation type not implemented
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// record -> view

template<fmt_class C>
inline reloc_view
reloc_view_of(const typename elf_traits<C>::rela &r) noexcept
{
  using tr = elf_traits<C>;
  reloc_view v;
  v.offset = static_cast<u64>(r.offset);
  v.sym = tr::r_sym(r.info);
  v.type = tr::r_type(r.info);
  v.addend = static_cast<i64>(static_cast<typename tr::sxwd>(r.addend));
  v.rela = true;
  return v;
}

template<fmt_class C>
inline reloc_view
reloc_view_of(const typename elf_traits<C>::rel &r) noexcept
{
  using tr = elf_traits<C>;
  reloc_view v;
  v.offset = static_cast<u64>(r.offset);
  v.sym = tr::r_sym(r.info);
  v.type = tr::r_type(r.info);
  v.addend = 0;      // implicit; the backend reads it out of the slot
  v.rela = false;
  return v;
}

};      // namespace elf
};      // namespace micron

#if defined(__micron_arch_amd64)
#include "arch/reloc_amd64.hpp"
#elif defined(__micron_arch_arm64)
#include "arch/reloc_arm64.hpp"
#elif defined(__micron_arch_x86)
#include "arch/reloc_i386.hpp"
#elif defined(__micron_arch_arm32)
#include "arch/reloc_arm32.hpp"
#else
#error "micron::elf has no relocation backend for this arch"
#endif

namespace micron
{
namespace elf
{

inline i64
__implicit_addend(const u8 *p) noexcept
{
  if constexpr ( native_class == fmt_class::elf32 )
    return static_cast<i64>(static_cast<i32>(*micron::ptr_cast<const u32 *>(p)));
  else
    return static_cast<i64>(*micron::ptr_cast<const u64 *>(p));
}

inline void *
__resolve_for(const reloc_ctx_t &ctx, u32 si, bool &weak) noexcept
{
  weak = false;
  if ( !ctx.d || !ctx.d->symtab || !ctx.d->strtab ) return nullptr;
  const char *name = ctx.d->strtab + ctx.d->symtab[si].name;
  weak = elf_st_bind(ctx.d->symtab[si].info) == stb_weak;
  return ctx.resolve ? ctx.resolve(ctx.user, name, si) : nullptr;
}

#if defined(__micron_arch_amd64)

// amd64 reloc
inline reloc_result
apply_reloc(const reloc_ctx_t &ctx, const reloc_view &r) noexcept
{
  const u32 type = r.type;
  const u32 si = r.sym;
  if ( !ctx.d ) return reloc_result::unresolved;
  if ( si != stn_undef && (!ctx.d->symtab || si >= ctx.d->symcount) ) return reloc_result::unresolved;
  u8 *const p = ctx.load_base + r.offset;
  const i64 a = r.rela ? r.addend : __implicit_addend(p);
  const u8 *const b = ctx.load_base;

  switch ( type ) {
  case r_x86_64_none:
    return reloc_result::applied;
  case r_x86_64_relative:
    *reinterpret_cast<u64 *>(p) = reinterpret_cast<u64>(b) + static_cast<u64>(a);
    return reloc_result::applied;
  case r_x86_64_irelative: {
    using ifn = u64 (*)();
    ifn fn = reinterpret_cast<ifn>(reinterpret_cast<u64>(b) + static_cast<u64>(a));
    *reinterpret_cast<u64 *>(p) = fn();
    return reloc_result::applied;
  }
  case r_x86_64_glob_dat:
  case r_x86_64_jump_slot:
  case r_x86_64_64: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u64 *>(p) = reinterpret_cast<u64>(s) + (type == r_x86_64_64 ? static_cast<u64>(a) : 0);
    return reloc_result::applied;
  }
  case r_x86_64_pc32:
  case r_x86_64_plt32: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    i64 v = static_cast<i64>(reinterpret_cast<u64>(s)) + a - static_cast<i64>(reinterpret_cast<u64>(p));
    *reinterpret_cast<i32 *>(p) = static_cast<i32>(v);
    return reloc_result::applied;
  }
  case r_x86_64_dtpmod64:
    *reinterpret_cast<u64 *>(p) = ctx.tls_modid;
    return reloc_result::applied;
  case r_x86_64_dtpoff64:
    // NOTE: resolves the offset within THIS module's TLS block only
    *reinterpret_cast<u64 *>(p) = ctx.d->symtab[si].value + static_cast<u64>(a);
    return reloc_result::applied;
  case r_x86_64_tpoff64:
    // WARNING: static (initial-exec/local-exec) TLS is __NOT__ implemented
    return reloc_result::unsupported;
  case r_x86_64_copy:
    return reloc_result::unsupported;
  default:
    return reloc_result::unsupported;
  }
}

#elif defined(__micron_arch_arm64)

inline reloc_result
apply_reloc(const reloc_ctx_t &ctx, const reloc_view &r) noexcept
{
  const u32 type = r.type;
  const u32 si = r.sym;
  if ( !ctx.d ) return reloc_result::unresolved;
  if ( si != stn_undef && (!ctx.d->symtab || si >= ctx.d->symcount) ) return reloc_result::unresolved;
  u8 *const p = ctx.load_base + r.offset;
  const i64 a = r.rela ? r.addend : __implicit_addend(p);
  const u8 *const b = ctx.load_base;

  switch ( type ) {
  case r_aarch64_none:
    return reloc_result::applied;
  case r_aarch64_relative:
    *reinterpret_cast<u64 *>(p) = reinterpret_cast<u64>(b) + static_cast<u64>(a);
    return reloc_result::applied;
  case r_aarch64_irelative: {
    using ifn = u64 (*)();
    ifn fn = reinterpret_cast<ifn>(reinterpret_cast<u64>(b) + static_cast<u64>(a));
    *reinterpret_cast<u64 *>(p) = fn();
    return reloc_result::applied;
  }
  case r_aarch64_glob_dat:
  case r_aarch64_jump_slot:
  case r_aarch64_abs64: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u64 *>(p) = reinterpret_cast<u64>(s) + static_cast<u64>(a);
    return reloc_result::applied;
  }
  case r_aarch64_abs32: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u32 *>(p) = static_cast<u32>(reinterpret_cast<u64>(s) + static_cast<u64>(a));
    return reloc_result::applied;
  }
  case r_aarch64_tls_dtpmod:
    *reinterpret_cast<u64 *>(p) = ctx.tls_modid;
    return reloc_result::applied;
  case r_aarch64_tls_dtprel:
    *reinterpret_cast<u64 *>(p) = ctx.d->symtab[si].value + static_cast<u64>(a);
    return reloc_result::applied;
  case r_aarch64_tls_tprel:
    return reloc_result::unsupported;
  case r_aarch64_tlsdesc:
    return reloc_result::unsupported;
  case r_aarch64_copy:
    return reloc_result::unsupported;
  default:
    return reloc_result::unsupported;
  }
}

#elif defined(__micron_arch_x86)

// i386
inline reloc_result
apply_reloc(const reloc_ctx_t &ctx, const reloc_view &r) noexcept
{
  const u32 type = r.type;
  const u32 si = r.sym;
  if ( !ctx.d ) return reloc_result::unresolved;
  if ( si != stn_undef && (!ctx.d->symtab || si >= ctx.d->symcount) ) return reloc_result::unresolved;
  u8 *const p = ctx.load_base + r.offset;
  const i64 a = r.rela ? r.addend : __implicit_addend(p);
  const u32 b = static_cast<u32>(reinterpret_cast<uintptr_t>(ctx.load_base));

  switch ( type ) {
  case r_386_none:
    return reloc_result::applied;
  case r_386_relative:
    *reinterpret_cast<u32 *>(p) = b + static_cast<u32>(a);
    return reloc_result::applied;
  case r_386_irelative: {
    using ifn = u32 (*)();
    ifn fn = reinterpret_cast<ifn>(static_cast<uintptr_t>(b + static_cast<u32>(a)));
    *reinterpret_cast<u32 *>(p) = fn();
    return reloc_result::applied;
  }
  case r_386_glob_dat:
  case r_386_jmp_slot: {
    // the addend is ignored for these two even in REL form
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u32 *>(p) = static_cast<u32>(reinterpret_cast<uintptr_t>(s));
    return reloc_result::applied;
  }
  case r_386_32: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u32 *>(p) = static_cast<u32>(reinterpret_cast<uintptr_t>(s)) + static_cast<u32>(a);
    return reloc_result::applied;
  }
  case r_386_pc32:
  case r_386_plt32: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    const u32 sv = static_cast<u32>(reinterpret_cast<uintptr_t>(s));
    *reinterpret_cast<u32 *>(p) = sv + static_cast<u32>(a) - static_cast<u32>(reinterpret_cast<uintptr_t>(p));
    return reloc_result::applied;
  }
  case r_386_tls_dtpmod32:
    *reinterpret_cast<u32 *>(p) = static_cast<u32>(ctx.tls_modid);
    return reloc_result::applied;
  case r_386_tls_dtpoff32:
    *reinterpret_cast<u32 *>(p) = static_cast<u32>(ctx.d->symtab[si].value) + static_cast<u32>(a);
    return reloc_result::applied;
  case r_386_tls_tpoff:
  case r_386_tls_ie:
  case r_386_tls_gotie:
  case r_386_tls_le:
  case r_386_tls_tpoff32:
  case r_386_tls_ie_32:
  case r_386_tls_le_32:
  case r_386_tls_desc:
    return reloc_result::unsupported;
  case r_386_copy:
    return reloc_result::unsupported;
  default:
    return reloc_result::unsupported;
  }
}

#elif defined(__micron_arch_arm32)

// armv7-a
inline reloc_result
apply_reloc(const reloc_ctx_t &ctx, const reloc_view &r) noexcept
{
  const u32 type = r.type;
  const u32 si = r.sym;
  if ( !ctx.d ) return reloc_result::unresolved;
  if ( si != stn_undef && (!ctx.d->symtab || si >= ctx.d->symcount) ) return reloc_result::unresolved;
  u8 *const p = ctx.load_base + r.offset;
  const i64 a = r.rela ? r.addend : __implicit_addend(p);
  const u32 b = static_cast<u32>(reinterpret_cast<uintptr_t>(ctx.load_base));

  switch ( type ) {
  case r_arm_none:
    return reloc_result::applied;
  case r_arm_relative:
    *micron::ptr_cast<u32 *>(p) = b + static_cast<u32>(a);
    return reloc_result::applied;
  case r_arm_irelative: {
    using ifn = u32 (*)();
    ifn fn = reinterpret_cast<ifn>(static_cast<uintptr_t>(b + static_cast<u32>(a)));
    *micron::ptr_cast<u32 *>(p) = fn();
    return reloc_result::applied;
  }
  case r_arm_glob_dat:
  case r_arm_jump_slot: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *micron::ptr_cast<u32 *>(p) = static_cast<u32>(reinterpret_cast<uintptr_t>(s));
    return reloc_result::applied;
  }
  case r_arm_abs32:
  case r_arm_target1: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *micron::ptr_cast<u32 *>(p) = static_cast<u32>(reinterpret_cast<uintptr_t>(s)) + static_cast<u32>(a);
    return reloc_result::applied;
  }
  case r_arm_rel32: {
    bool weak = false;
    void *s = __resolve_for(ctx, si, weak);
    if ( !s ) return weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    const u32 sv = static_cast<u32>(reinterpret_cast<uintptr_t>(s));
    *micron::ptr_cast<u32 *>(p) = sv + static_cast<u32>(a) - static_cast<u32>(reinterpret_cast<uintptr_t>(p));
    return reloc_result::applied;
  }
  case r_arm_tls_dtpmod32:
    *micron::ptr_cast<u32 *>(p) = static_cast<u32>(ctx.tls_modid);
    return reloc_result::applied;
  case r_arm_tls_dtpoff32:
    *micron::ptr_cast<u32 *>(p) = static_cast<u32>(ctx.d->symtab[si].value) + static_cast<u32>(a);
    return reloc_result::applied;
  case r_arm_tls_tpoff32:
  case r_arm_tls_ie32:
  case r_arm_tls_le32:
  case r_arm_tls_desc:
    return reloc_result::unsupported;
  case r_arm_copy:
    return reloc_result::unsupported;
  default:
    return reloc_result::unsupported;
  }
}

#endif

};      // namespace elf
};      // namespace micron
