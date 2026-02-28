//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// think about moving to /memory

#include "../linux/sys/types.hpp"
#include "../numerics.hpp"
#include "../syscall.hpp"
#include "../types.hpp"

#include "mmap_bits.hpp"

namespace micron
{

// NOTE: mmap returns -1 on failure, will roll over
inline __attribute__((always_inline)) addr_t *
mmap(addr_t *__restrict addr, usize len, int prot, int flags, int fd, posix::off_t offset)
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_mmap, addr, len, prot, flags, fd, offset));
}

// addr_t *mmap64();
int
munmap(addr_t *__restrict addr, usize len)
{
  return (int)micron::syscall(SYS_munmap, addr, len);
}

int
mprotect(addr_t *addr, usize len, int prot)
{
  return (int)micron::syscall(SYS_mprotect, addr, len, prot);
}

int
msync(addr_t *addr, usize len, int flags)
{
  return (int)micron::syscall(SYS_msync, addr, len, flags);
}

int
madvise(addr_t *addr, usize len, int advice)
{

  return (int)micron::syscall(SYS_madvise, addr, len, advice);
};

int
mlock(const addr_t *addr, usize len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}

int
munlock(const addr_t *addr, usize len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}

int
memfd_create(const char *name, unsigned int flags)
{
  return (int)micron::syscall(SYS_memfd_create, name, flags);
}

int mlockall();
int munlockall();

template <typename T>
auto
mprotect(T *addr, usize __length, int prot)
{
  return micron::syscall(SYS_mprotect, addr, __length, prot);
}

template <typename T>
auto
brk(T *addr)
{
  return micron::syscall(SYS_brk, addr);
}

addr_t *
brk(nullptr_t t [[maybe_unused]])
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_brk, 0));
}

addr_t *
sbrk(intptr_t increment)
{
  addr_t *__program_break = nullptr;
  __program_break = brk(nullptr);

  if ( increment == 0 ) [[likely]]
    return __program_break;

  brk(__program_break + increment);
  return __program_break;
}

// Helper functions

template <typename T>
inline bool
mmap_failed(T *addr)
{
  return (reinterpret_cast<addr_t *>(addr) == map_failed);
}

class __default_map
{
};

template <class C = __default_map, typename T>
inline T *
__as_map(usize sz)
{
  if constexpr ( micron::is_same_v<C, __default_map> ) {
    return reinterpret_cast<T *>(micron::mmap(0, sz, prot_read | prot_write, map_private | map_anonymous | map_stack, -1, 0));
  }
  return nullptr;
}

template <class C = __default_map>
inline addr_t *
addrmap(usize sz)
{
  return __as_map<C, addr_t>(sz);
}

template <class C = __default_map>
inline byte *
bytemap(usize sz)
{
  return __as_map<C, byte>(sz);
}

template <typename T>
inline auto
try_unmap(T *addr, usize sz)
{
  if ( addr != nullptr )
    return munmap(reinterpret_cast<addr_t *>(addr), sz);
  return 1;
}

};     // namespace micron
