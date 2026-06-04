//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"
#include "consts.hpp"
#include "hash.hpp"
#include "header.hpp"

namespace micron
{
namespace elf
{

struct module_t;

struct reloc_ctx_t {
  u8 *load_base = nullptr;
  const dyn_info_t *d = nullptr;
  void *(*resolve)(void *user, const char *name) = nullptr;
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

};      // namespace elf
};      // namespace micron

#if defined(__micron_arch_amd64)
#include "arch/reloc_amd64.hpp"
#elif defined(__micron_arch_arm64)
#include "arch/reloc_arm64.hpp"
#else
#error "micron::elf has no relocation backend for this arch"
#endif

namespace micron
{
namespace elf
{

#if defined(__micron_arch_amd64)

// amd64 reloc
inline reloc_result
apply_reloc(const reloc_ctx_t &ctx, const rela_t &r) noexcept
{
  const u32 type = elf_r_type(r.info);
  const u32 si = elf_r_sym(r.info);
  if ( ctx.d->symcount && si >= ctx.d->symcount ) return reloc_result::unresolved;
  u8 *const p = ctx.load_base + r.offset;
  const sxword a = r.addend;
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
    const char *name = ctx.d->strtab + ctx.d->symtab[si].name;
    void *s = ctx.resolve(ctx.user, name);
    if ( !s ) return elf_st_bind(ctx.d->symtab[si].info) == stb_weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u64 *>(p) = reinterpret_cast<u64>(s) + (type == r_x86_64_64 ? static_cast<u64>(a) : 0);
    return reloc_result::applied;
  }
  case r_x86_64_pc32:
  case r_x86_64_plt32: {
    const char *name = ctx.d->strtab + ctx.d->symtab[si].name;
    void *s = ctx.resolve(ctx.user, name);
    if ( !s ) return elf_st_bind(ctx.d->symtab[si].info) == stb_weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    i64 v = static_cast<i64>(reinterpret_cast<u64>(s)) + a - static_cast<i64>(reinterpret_cast<u64>(p));
    *reinterpret_cast<i32 *>(p) = static_cast<i32>(v);
    return reloc_result::applied;
  }
  case r_x86_64_dtpmod64:
    *reinterpret_cast<u64 *>(p) = ctx.tls_modid;
    return reloc_result::applied;
  case r_x86_64_dtpoff64: {
    const char *name = ctx.d->strtab + ctx.d->symtab[si].name;
    (void)name;
    // NOTE: resolves the offset within THIS module's TLS block only
    *reinterpret_cast<u64 *>(p) = ctx.d->symtab[si].value + static_cast<u64>(a);
    return reloc_result::applied;
  }
  case r_x86_64_tpoff64:
    // static (initial-exec/local-exec) TLS not implemented; refuse rather than leave a garbage offset.
    return reloc_result::unsupported;
  case r_x86_64_copy:
    return reloc_result::unsupported;
  default:
    return reloc_result::unsupported;
  }
}

#elif defined(__micron_arch_arm64)

inline reloc_result
apply_reloc(const reloc_ctx_t &ctx, const rela_t &r) noexcept
{
  const u32 type = elf_r_type(r.info);
  const u32 si = elf_r_sym(r.info);
  if ( ctx.d->symcount && si >= ctx.d->symcount ) return reloc_result::unresolved;
  u8 *const p = ctx.load_base + r.offset;
  const sxword a = r.addend;
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
    const char *name = ctx.d->strtab + ctx.d->symtab[si].name;
    void *s = ctx.resolve(ctx.user, name);
    if ( !s ) return elf_st_bind(ctx.d->symtab[si].info) == stb_weak ? reloc_result::skipped_weak : reloc_result::unresolved;
    *reinterpret_cast<u64 *>(p) = reinterpret_cast<u64>(s) + static_cast<u64>(a);
    return reloc_result::applied;
  }
  case r_aarch64_abs32: {
    const char *name = ctx.d->strtab + ctx.d->symtab[si].name;
    void *s = ctx.resolve(ctx.user, name);
    if ( !s ) return elf_st_bind(ctx.d->symtab[si].info) == stb_weak ? reloc_result::skipped_weak : reloc_result::unresolved;
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

#endif

};      // namespace elf
};      // namespace micron
