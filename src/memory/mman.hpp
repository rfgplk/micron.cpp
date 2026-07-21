//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/sys/types.hpp"
#include "../numerics.hpp"
#include "../syscall.hpp"
#include "../types.hpp"

#include "mmap_bits.hpp"

namespace micron
{
// NOTE: everything here must be inline so we avoid multiple definition ODRs

// NOTE: the raw mmap syscall returns -ERRNO on failure
inline __attribute__((always_inline)) addr_t *
mmap(addr_t *__restrict addr, usize len, int prot, int flags, int fd, posix::off_t offset)
{
#if defined(__micron_arch_width_32)
  // NOTE: 32-bit ABIs take the file offset in 4096-byte page units via mmap2 (__always__ 4096, independent of PAGE_SIZE)
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_mmap2, addr, len, prot, flags, fd, static_cast<unsigned long>(offset) >> 12));
#else
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_mmap, addr, len, prot, flags, fd, offset));
#endif
}

// addr_t *mmap64();
inline int
munmap(addr_t *__restrict addr, usize len)
{
  return (int)micron::syscall(SYS_munmap, addr, len);
}

inline int
mprotect(addr_t *addr, usize len, int prot)
{
  return (int)micron::syscall(SYS_mprotect, addr, len, prot);
}

inline int
msync(addr_t *addr, usize len, int flags)
{
  return (int)micron::syscall(SYS_msync, addr, len, flags);
}

inline int
madvise(addr_t *addr, usize len, int advice)
{

  return (int)micron::syscall(SYS_madvise, addr, len, advice);
};

inline int
mlock(const addr_t *addr, usize len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}

inline int
munlock(const addr_t *addr, usize len)
{
  return (int)micron::syscall(SYS_munlock, addr, len);
}

inline int
memfd_create(const char *name, unsigned int flags)
{
  return (int)micron::syscall(SYS_memfd_create, name, flags);
}

#if !defined(__micron_arch_arm32)
// >=5.14 (and CONFIG_SECRETMEM / secretmem.enable=1)
inline int
memfd_secret(const char *name, unsigned int flags)
{
  return (int)micron::syscall(SYS_memfd_secret, name, flags);
}
#endif

// mlockall flags (mcl_current / mcl_future / mcl_onfault) live in mmap_bits.hpp
inline int
mlockall(int flags)
{
  return static_cast<int>(micron::syscall(SYS_mlockall, flags));
}

inline int
munlockall(void)
{
  return static_cast<int>(micron::syscall(SYS_munlockall));
}

// mremap flags
constexpr int mremap_maymove = 1;
constexpr int mremap_fixed = 2;
constexpr int mremap_dontunmap = 4;      // >=5.7

inline addr_t *
mremap(addr_t *old_addr, usize old_size, usize new_size, int flags)
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_mremap, old_addr, old_size, new_size, flags, 0));
}

inline addr_t *
mremap(addr_t *old_addr, usize old_size, usize new_size, int flags, addr_t *new_addr)
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_mremap, old_addr, old_size, new_size, flags, new_addr));
}

inline int
mincore(addr_t *addr, usize length, unsigned char *vec)
{
  return static_cast<int>(micron::syscall(SYS_mincore, addr, length, vec));
}

template<typename T>
auto
mprotect(T *addr, usize __length, int prot)
{
  return micron::syscall(SYS_mprotect, addr, __length, prot);
}

template<typename T>
auto
brk(T *addr)
{
  return micron::syscall(SYS_brk, addr);
}

inline addr_t *
brk(nullptr_t t [[maybe_unused]])
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_brk, 0));
}

inline addr_t *
sbrk(intptr_t increment)
{
  addr_t *__program_break = nullptr;
  __program_break = brk(nullptr);

  if ( increment == 0 ) [[likely]]
    return __program_break;

  brk(reinterpret_cast<addr_t *>(reinterpret_cast<char *>(__program_break) + increment));
  return __program_break;
}

// Helper functions

template<typename T>
inline bool
mmap_failed(T *addr)
{
  // the kernel signals mmap failure by returning -errno in [-4095, -1]
  // this way we catch the whole range
  return (reinterpret_cast<uintptr_t>(addr) >= static_cast<uintptr_t>(-4095));
}

class __default_map
{
};

template<class C = __default_map, typename T>
inline T *
__as_map(usize sz)
{
  if constexpr ( micron::is_same_v<C, __default_map> ) {
    return reinterpret_cast<T *>(micron::mmap(0, sz, prot_read | prot_write, map_private | map_anonymous | map_stack, -1, 0));
  }
  return nullptr;
}

template<class C = __default_map>
inline addr_t *
addrmap(usize sz)
{
  return __as_map<C, addr_t>(sz);
}

template<class C = __default_map>
inline byte *
bytemap(usize sz)
{
  return __as_map<C, byte>(sz);
}

template<typename T>
inline auto
try_unmap(T *addr, usize sz)
{
  if ( addr != nullptr ) return munmap(reinterpret_cast<addr_t *>(addr), sz);
  return 1;
}

};      // namespace micron
