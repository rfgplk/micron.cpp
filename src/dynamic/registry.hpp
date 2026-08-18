//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/elf/loader.hpp"
#include "../linux/sys/stat.hpp"
#include "../memory/actions.hpp"
#include "../memory/mman.hpp"
#include "../memory/mmap_bits.hpp"
#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

#include "flags.hpp"

namespace micron
{
namespace elf
{
namespace dl
{

inline constexpr usize max_modules = 256;
inline constexpr usize max_deps = dyn_info_t::max_needed;

struct dl_module {
  module_t mod{};

  u32 index = 0;
  u32 generation = 0;
  u32 refcount = 0;
  u64 load_order = 0;

  u64 dev = 0;
  u64 ino = 0;

  bool in_use = false;
  bool global = false;
  bool nodelete = false;
  bool relocated = false;
  bool initialized = false;
  bool deepbind = false;

  u32 dep_count = 0;
  u32 deps[max_deps] = {};

  const char *
  soname() const noexcept
  {
    return mod.dyn.soname ? mod.dyn.soname : mod.path.c_str();
  }
};

inline dl_module *__slots[max_modules] = {};
inline usize __slot_high = 0;
inline u32 __generation_next = 1;
inline u64 __load_seq = 1;
inline micron::mutex __registry_mtx;

inline dl_module *
__slot_alloc() noexcept
{
  for ( usize i = 0; i < max_modules; ++i ) {
    if ( __slots[i] && __slots[i]->in_use ) continue;
    dl_module *m = __slots[i];
    if ( !m ) {
      void *p
          = reinterpret_cast<void *>(micron::mmap(nullptr, sizeof(dl_module), prot_read | prot_write, map_private | map_anonymous, -1, 0));
      if ( mmap_failed(p) ) return nullptr;
      m = micron::construct_at(reinterpret_cast<dl_module *>(p));
      __slots[i] = m;
    } else {

      m = micron::construct_at(m);
    }
    m->index = static_cast<u32>(i);
    m->generation = __generation_next++;
    m->in_use = true;
    if ( i + 1 > __slot_high ) __slot_high = i + 1;
    return m;
  }
  return nullptr;
}

inline void
__slot_free(dl_module *m) noexcept
{
  if ( !m || !m->in_use ) return;
  micron::destroy_at(m);
  m->in_use = false;

  invalidate_host_modules();
}

inline dl_module *
__slot_at(u32 index, u32 generation) noexcept
{
  if ( index >= max_modules ) return nullptr;
  dl_module *m = __slots[index];
  if ( !m || !m->in_use ) return nullptr;
  if ( m->generation != generation ) return nullptr;
  return m;
}

inline dl_module *
__find_by_ident(u64 dev, u64 ino) noexcept
{
  if ( ino == 0 ) return nullptr;
  for ( usize i = 0; i < __slot_high; ++i ) {
    dl_module *m = __slots[i];
    if ( m && m->in_use && m->ino == ino && m->dev == dev ) return m;
  }
  return nullptr;
}

inline dl_module *
__find_by_soname(const char *name) noexcept
{
  if ( !name || !*name ) return nullptr;
  for ( usize i = 0; i < __slot_high; ++i ) {
    dl_module *m = __slots[i];
    if ( !m || !m->in_use ) continue;
    if ( m->mod.dyn.soname && micron::strcmp(m->mod.dyn.soname, name) == 0 ) return m;
  }
  return nullptr;
}

inline dl_module *
__find_by_address(const void *addr) noexcept
{
  const u8 *p = reinterpret_cast<const u8 *>(addr);
  for ( usize i = 0; i < __slot_high; ++i ) {
    dl_module *m = __slots[i];
    if ( !m || !m->in_use || !m->mod.load_base ) continue;
    if ( p >= m->mod.load_base && p < m->mod.load_base + m->mod.load_span ) return m;
  }
  return nullptr;
}

inline bool
__file_probe(const char *path, u64 &dev, u64 &ino, bool *native = nullptr) noexcept
{
  if ( native ) *native = false;
  const i32 fd = posix::open(path, posix::o_rdonly);
  if ( fd < 0 ) return false;

#if defined(__micron_arch_arm32) || defined(__micron_arch_x86)
  posix::stat64_t st{};
#else
  posix::stat_t st{};
#endif
  if ( posix::fstat(fd, st) < 0 ) {
    posix::close(fd);
    return false;
  }
  dev = static_cast<u64>(st.st_dev);
  ino = static_cast<u64>(st.st_ino);

  if ( native ) {
    u8 probe[ident_size + 4];
    if ( posix::pread(fd, probe, sizeof(probe), 0) == static_cast<max_t>(sizeof(probe)) ) {
      const half machine = static_cast<half>(static_cast<half>(probe[ident_size + 2]) | (static_cast<half>(probe[ident_size + 3]) << 8));
      *native = probe[ei_mag0] == mag0 && probe[ei_mag1] == static_cast<u8>(mag1) && probe[ei_mag2] == static_cast<u8>(mag2)
                && probe[ei_mag3] == static_cast<u8>(mag3) && probe[ei_class] == native_traits::ident_class
                && probe[ei_data] == (native_data == fmt_data::msb ? elfdata2msb : elfdata2lsb)
                && (expected_machine == 0 || machine == expected_machine);
    }
  }
  posix::close(fd);
  return true;
}

inline bool
__file_ident(const char *path, u64 &dev, u64 &ino) noexcept
{
  return __file_probe(path, dev, ino, nullptr);
}

inline dl_module *
__find_by_path(const char *path) noexcept
{
  if ( !path || !*path ) return nullptr;
  for ( usize i = 0; i < __slot_high; ++i ) {
    dl_module *m = __slots[i];
    if ( !m || !m->in_use ) continue;
    if ( micron::strcmp(m->mod.path.c_str(), path) == 0 ) return m;
  }
  return nullptr;
}

};      // namespace dl
};      // namespace elf
};      // namespace micron
