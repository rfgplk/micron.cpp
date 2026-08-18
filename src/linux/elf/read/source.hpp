//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../memory/cmemory.hpp"
#include "../../../memory/mman.hpp"
#include "../../../memory/mmap_bits.hpp"
#include "../../../string/strings.hpp"
#include "../../../sum.hpp"
#include "../../io/sys.hpp"
#include "../../sys/fcntl.hpp"

#include "../bits.hpp"
#include "../host_modules.hpp"

namespace micron
{
namespace elf
{
namespace read
{

struct span_t {
  const u8 *ptr = nullptr;
  usize len = 0;
};

class source
{
  const u8 *__base = nullptr;
  usize __len = 0;
  void *__map = nullptr;
  usize __map_len = 0;

public:
  constexpr source() = default;

  constexpr source(const u8 *p, usize n) noexcept : __base(p), __len(n) { }

  source(const source &) = delete;
  source &operator=(const source &) = delete;

  source(source &&o) noexcept : __base(o.__base), __len(o.__len), __map(o.__map), __map_len(o.__map_len)
  {
    o.__base = nullptr;
    o.__len = 0;
    o.__map = nullptr;
    o.__map_len = 0;
  }

  source &
  operator=(source &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __base = o.__base;
      __len = o.__len;
      __map = o.__map;
      __map_len = o.__map_len;
      o.__base = nullptr;
      o.__len = 0;
      o.__map = nullptr;
      o.__map_len = 0;
    }
    return *this;
  }

  ~source() { reset(); }

  void
  reset() noexcept
  {
    if ( __map && __map_len ) {
      micron::munmap(reinterpret_cast<addr_t *>(__map), __map_len);
      // the mapping was pathed and therefore listed in /proc/self/maps. the exec-bit gate in
      // __build_host_dyn keeps a read-only inspection map out of the host table, but the snapshot is
      // latched and never refreshed on its own -- drop it rather than rely on a single filter.
      micron::elf::invalidate_host_modules();
    }
    __base = nullptr;
    __len = 0;
    __map = nullptr;
    __map_len = 0;
  }

  bool
  valid() const noexcept
  {
    return __base != nullptr && __len != 0;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  const u8 *
  data() const noexcept
  {
    return __base;
  }

  usize
  size() const noexcept
  {
    return __len;
  }

  span_t
  at(u64 off, u64 want) const noexcept
  {
    if ( !__base || off >= __len ) return span_t{};
    const u64 avail = static_cast<u64>(__len) - off;
    return span_t{ __base + off, static_cast<usize>(want < avail ? want : avail) };
  }

  static micron::option<source, const char *> open(const char *path) noexcept;
};

inline micron::option<source, const char *>
source::open(const char *path) noexcept
{
  using res = micron::option<source, const char *>;
  if ( !path || !*path ) return res{ "elf::read: empty path" };

  const i32 fd = posix::open(path, posix::o_rdonly);
  if ( fd < 0 ) return res{ "elf::read: open failed" };

  const max_t end = posix::lseek(fd, 0, posix::seek_end);
  if ( end <= 0 ) {
    posix::close(fd);
    return res{ "elf::read: empty or unseekable file" };
  }
  const usize len = static_cast<usize>(end);

  void *p = reinterpret_cast<void *>(micron::mmap(nullptr, len, prot_read, map_private, fd, 0));
  posix::close(fd);
  if ( mmap_failed(p) ) return res{ "elf::read: mmap failed" };

  source s;
  s.__base = reinterpret_cast<const u8 *>(p);
  s.__len = len;
  s.__map = p;
  s.__map_len = len;
  return res{ micron::move(s) };
}

template<typename T>
inline T
rd(const u8 *base, u64 off, fmt_data d) noexcept
{
  T v{};
  micron::memcpy(reinterpret_cast<u8 *>(&v), base + off, sizeof(T));
  return micron::elf::from_file_order(v, d);
}

inline micron::string
read_cstr_at(const source &src, u64 off, u64 limit) noexcept
{
  micron::string out{};
  const span_t sp = src.at(off, limit);
  for ( usize i = 0; i < sp.len; i++ ) {
    if ( sp.ptr[i] == 0 ) break;
    out.push_back(static_cast<char>(sp.ptr[i]));
  }
  return out;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
