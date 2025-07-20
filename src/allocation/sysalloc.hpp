//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
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
  return reinterpret_cast<T *>micron::syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
}

template <typename T>
auto
mremap(T *oldaddr, size_t oldlength, size_t new_size, int flags, T *new_addr) -> T *
{
  return reinterpret_cast<T *>micron::syscall(SYS_mremap, oldaddr, oldlength, new_size, flags, new_addr);
}

template <typename T>
auto
munmap(T *addr, size_t length)
{
  return micron::syscall(SYS_munmap, length);
}

template <typename T>
auto
mprotect(T *addr, size_t length, int prot)
{
  return micron::syscall(SYS_mprotect, addr, length, prot);
}

template <typename T>
auto
brk(T *addr)
{
  return micron::syscall(SYS_brk, addr);
}

auto
sbrk(intptr_t inc)
{
  return micron::syscall(SYS_sbrk, inc);
}
};

};
