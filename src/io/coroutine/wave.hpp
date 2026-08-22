//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../../memory/allocation/kmemory.hpp"
#include "../../tasks/coroutine/wave.hpp"

#include "__acore.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// coro io waves
// (whole small files; batch per enter)
//
// normal perop route does an open + read + close as three separate io_uring_enters; for small
// files the overhead is expensive;
// a wave stages all three as one linked chain per file, W files at a time
//
// NOTE: this is for many small files per worker; a file that overflows the slab is reported partial
// and must be reread conventionally

namespace micron
{
namespace io
{
namespace coro
{

#ifndef MICRON_CORO_WAVE_ITEMS
#define MICRON_CORO_WAVE_ITEMS 16u
#endif
#ifndef MICRON_CORO_WAVE_CAP
#define MICRON_CORO_WAVE_CAP (32u << 10)
#endif

inline constexpr u32 wave_items = MICRON_CORO_WAVE_ITEMS;
inline constexpr usize wave_cap = MICRON_CORO_WAVE_CAP;
inline constexpr u32 wave_sqes = 3;              // open, read, close
inline constexpr usize wave_name_cap = 256;      // NAME_MAX + 1

struct wave_result {
  const char *name = nullptr;
  const byte *data = nullptr;
  usize len = 0;
  i32 err = 0;
  i32 close_err = 0;
  bool unstaged = false;
  bool partial = false;
};

class wave
{
  micron::sstr<wave_name_cap> __name[wave_items]{};
  wave_result __res[wave_items]{};
  i32 __slot[wave_items]{};
  micron::uring::sqe __q[wave_items * wave_sqes]{};
  micron::coro::__io_wop __nodes[wave_items * wave_sqes]{};
  micron::coro::__io_wave __wv{};
  byte *__slab = nullptr;
  i32 __dirfd = -1;
  i32 __rid = -1;          // ring the slots and sqes went to
  u32 __n = 0;             // items pushed
  u32 __staged = 0;        // items that reached the ring
  u32 __seen = 0;          // items completed since construction
  u32 __missed = 0;        // which overflowed the slab
  u32 __stranded = 0;      // slots whose direct descriptor could not be evicted; should stay 0
  bool __inert = false;

  static constexpr usize __slab_sz = static_cast<usize>(wave_items) * wave_cap;

  // WARNING: the slab must be prefaulted before the ring reads into it;
  // io_uring cannot fault a page from the submit path
  bool
  __arm_slab() noexcept
  {
    if ( __slab != nullptr ) return true;
    void *__p = micron::mmap(nullptr, __slab_sz, prot_read | prot_write, map_private | map_anonymous, -1, 0);
    if ( micron::mmap_failed(reinterpret_cast<addr_t *>(__p)) ) return false;
    __slab = reinterpret_cast<byte *>(__p);
    usize __ps = micron::getpagesizelive();
    if ( __ps == 0 || __ps > __slab_sz ) __ps = micron::page_size;      // probe refused; fall back to the arch default
    if ( __ps == 0 ) __ps = 4096;
    for ( usize __o = 0; __o < __slab_sz; __o += __ps ) __slab[__o] = 0;
    return true;
  }

public:
  wave() noexcept
  {
    for ( u32 __i = 0; __i < wave_items; ++__i ) __slot[__i] = -1;
  }

  wave(const wave &) = delete;
  wave &operator=(const wave &) = delete;
  wave(wave &&) = delete;
  wave &operator=(wave &&) = delete;

  // WARNING: the kernel holds pointers into __nodes, so a live wave may not simply die
  ~wave() noexcept
  {
    (void)micron::coro::io::__wave_abandon(__wv);
    micron::coro::io::__wave_drain_cancel(__wv, __nodes, __staged * wave_sqes, __rid);
    if ( !micron::coro::io::__wave_settle(__wv) ) return;      // resumed frame never reported; leak, do not corrupt
    __reclaim_slots();
    if ( __slab != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(__slab), __slab_sz);
  }

  [[nodiscard]] static bool
  available() noexcept
  {
    return micron::coro::io::wave_available();
  }

  void
  begin(i32 dirfd) noexcept
  {
    __dirfd = dirfd;
    __n = 0;
    __staged = 0;
    __seen = 0;
    __missed = 0;
    __inert = false;
  }

  [[nodiscard]] bool
  full() const noexcept
  {
    return __n >= wave_items;
  }

  [[nodiscard]] bool
  empty() const noexcept
  {
    return __n == 0;
  }

  [[nodiscard]] bool
  inert() const noexcept
  {
    return __inert;
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __n;
  }

  // direct descriptors this wave could neither close nor safely recycle
  [[nodiscard]] u32
  stranded() const noexcept
  {
    return __stranded;
  }

  [[nodiscard]] const wave_result &
  operator[](usize i) const noexcept
  {
    return __res[i];
  }

  [[nodiscard]] bool
  push(const char *name) noexcept
  {
    if ( __n >= wave_items ) return false;
    if ( name == nullptr || name[0] == 0 ) return false;
    usize __l = 0;
    while ( name[__l] != 0 )
      if ( ++__l >= wave_name_cap ) return false;      // sstr<N> needs strlen < N, room for the nul
    __name[__n] = micron::sstr<wave_name_cap>(name);
    ++__n;
    return true;
  }

  [[nodiscard]] micron::task<i32>
  run()
  {
    if ( __n == 0 ) co_return 0;
    if ( !__arm_slab() ) co_return -error::out_of_memory;

    for ( u32 __i = 0; __i < __n; ++__i ) {
      __res[__i] = wave_result{};
      __res[__i].name = __name[__i].c_str();
      __slot[__i] = -1;
    }

    u32 __k = 0;
    __staged = 0;
    __rid = micron::coro::io::wave_ring_id();
    const u32 __room = micron::coro::io::wave_room();
    for ( u32 __i = 0; __i < __n; ++__i ) {
      if ( __k + wave_sqes > __room ) break;
      const i32 __s = micron::coro::io::wave_slot_acquire();
      if ( __s < 0 ) break;
      __slot[__i] = __s;

      micron::uring::sqe *__s0 = &__q[__k];
      micron::uring::sqe *__s1 = &__q[__k + 1];
      micron::uring::sqe *__s2 = &__q[__k + 2];

      micron::uring::prep_openat_direct(__s0, __dirfd, __name[__i].c_str(), posix::o_rdonly | posix::o_nofollow, 0, static_cast<u32>(__s));
      __s0->flags |= micron::uring::sqe_io_link;

      micron::uring::prep_read(__s1, __s, __slab + static_cast<usize>(__i) * wave_cap, static_cast<u32>(wave_cap), 0);
      micron::uring::sqe_set_fixed_file(__s1);
      __s1->flags |= micron::uring::sqe_io_hardlink;

      micron::uring::prep_close_direct(__s2, static_cast<u32>(__s));

      for ( u32 __j = 0; __j < wave_sqes; ++__j ) {
        __nodes[__k + __j].__idx = static_cast<u16>(__i);
        __nodes[__k + __j].__step = static_cast<u8>(__j);
        __nodes[__k + __j].__res = 0;      // a batch that never reaches the ring must not read as the last one's
      }
      __k += wave_sqes;
      __staged = __i + 1;
    }

    i32 __rc = 0;
    if ( __k != 0 ) {
      micron::coro::io::__wave_awaitable __aw{ __q, __nodes, &__wv, __k, 0 };
      __rc = co_await __aw;
    }

    __reclaim_slots();

    if ( __rc == 0 ) {
      for ( u32 __i = 0; __i < __k; __i += wave_sqes ) {
        const u32 __it = __nodes[__i].__idx;
        const i32 __oerr = __nodes[__i].__res;
        const i32 __rres = __nodes[__i + 1].__res;
        __res[__it].close_err = __oerr < 0 ? 0 : __nodes[__i + 2].__res;
        if ( __oerr < 0 ) {
          __res[__it].err = __oerr;
          continue;
        }
        if ( __rres < 0 ) {
          __res[__it].err = __rres;
          continue;
        }
        __res[__it].data = __slab + static_cast<usize>(__it) * wave_cap;
        __res[__it].len = static_cast<usize>(__rres);
        __res[__it].partial = static_cast<usize>(__rres) >= wave_cap;
      }
    } else {
      for ( u32 __i = 0; __i < __staged; ++__i ) __res[__i].err = __rc;
    }
    for ( u32 __i = __staged; __i < __n; ++__i ) __res[__i].unstaged = true;

    __note();
    __wv.__fin.store(1, micron::memory_order_release);
    co_return __rc;
  }

  void
  clear() noexcept
  {
    __n = 0;
    __staged = 0;
  }

private:
  [[nodiscard]] bool
  __slot_still_open(u32 __i) const noexcept
  {
    if ( __i >= __staged ) return false;      // never reached the ring, so nothing was opened
    const u32 __b = __i * wave_sqes;
    return __nodes[__b].__res >= 0 && __nodes[__b + 2].__res != 0;
  }

  // WARNING: a slot goes back in the free bitmap only once it is known empty; hardlink keeps the close attached to a failed read, but a
  // severed chain, a cancelled batch or a ring that went away can still leave one installed
  void
  __reclaim_slots() noexcept
  {
    for ( u32 __i = 0; __i < wave_items; ++__i ) {
      if ( __slot[__i] < 0 ) continue;
      if ( __slot_still_open(__i) && !micron::coro::io::wave_slot_close(__rid, __slot[__i]) ) {
        __slot[__i] = -1;
        ++__stranded;
        continue;
      }
      micron::coro::io::wave_slot_release(__rid, __slot[__i]);
      __slot[__i] = -1;
    }
  }

  void
  __note() noexcept
  {
    __seen += __staged;
    for ( u32 __i = 0; __i < __staged; ++__i )
      if ( __res[__i].partial ) ++__missed;
    if ( __seen >= wave_items && __missed * 2u > __seen ) __inert = true;
  }
};

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
