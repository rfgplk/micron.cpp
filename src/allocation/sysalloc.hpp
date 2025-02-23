#pragma once

#include "../linux/linux_types.hpp"
#include "../syscall.hpp"
#include "../types.hpp"

namespace micron
{
namespace posix
{

template <typename T>
auto
mmap(T *addr, size_t length, int prot, int flags, int fd, off_t offset) -> T *
{
  return reinterpret_cast<T *>::syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
}

template <typename T>
auto
mremap(T *oldaddr, size_t oldlength, size_t new_size, int flags, T *new_addr) -> T *
{
  return reinterpret_cast<T *>::syscall(SYS_mremap, oldaddr, oldlength, new_size, flags, new_addr);
}

template <typename T>
auto
munmap(T *addr, size_t length)
{
  return ::syscall(SYS_munmap, length);
}

template <typename T>
auto
mprotect(T *addr, size_t length, int prot)
{
  return ::syscall(SYS_mprotect, addr, length, prot);
}

template <typename T>
auto
brk(T *addr)
{
  return ::syscall(SYS_brk, addr);
}

auto
sbrk(intptr_t inc)
{
  return ::syscall(SYS_sbrk, inc);
}
};

};
