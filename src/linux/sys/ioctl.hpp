//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../type_traits.hpp"

namespace micron
{

template <typename... Args>
// TODO: reintroduce this, and resolve type conflicts  requires((micron::is_same_v<Args, unsigned long> && ...))
int
ioctl(int fd, Args... ops)
{
  return static_cast<int>(micron::syscall(SYS_ioctl, fd, ops...));
}

constexpr static const u32 __ioc_nrbits = 8;
constexpr static const u32 __ioc_typebits = 8;
constexpr static const u32 __ioc_sizebits = 14;
constexpr static const u32 __ioc_dirbits = 2;

constexpr static const u64 __ioc_nrmask = ((1 << __ioc_nrbits) - 1);
constexpr static const u64 __ioc_typemask = ((1 << __ioc_typebits) - 1);
constexpr static const u64 __ioc_sizemask = ((1 << __ioc_sizebits) - 1);
constexpr static const u64 __ioc_dirmask = ((1 << __ioc_dirbits) - 1);

constexpr static const u32 __ioc_nrshift = 0;
constexpr static const u64 __ioc_typeshift = (__ioc_nrshift + __ioc_nrbits);
constexpr static const u64 __ioc_sizeshift = (__ioc_typeshift + __ioc_typebits);
constexpr static const u64 __ioc_dirshift = (__ioc_sizeshift + __ioc_sizebits);

constexpr static const u64 __ioc_none = 0;
constexpr static const u64 __ioc_write = 1;
constexpr static const u64 __ioc_read = 2;

template <typename T>
consteval size_t
__sizeof_type()
{
  return sizeof(T);
}

consteval u64
io_request(u64 dir, u64 type, u64 nr, u64 size)
{
  return (((dir) << __ioc_dirshift) | ((type) << __ioc_typeshift) | ((nr) << __ioc_nrshift)
          | ((size) << __ioc_sizeshift));
}

consteval u64
io_default_command(u64 type, u64 nr)
{
  return io_request(__ioc_none, type, nr, 0);
}

template <typename T>
consteval u64
io_read_command(u64 type, u64 nr)
{
  return io_request(__ioc_read, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_write_command(u64 type, u64 nr)
{
  return io_request(__ioc_write, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_readwrite_command(u64 type, u64 nr)
{
  return io_request(__ioc_write | __ioc_read, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_read_bad_command(u64 type, u64 nr)
{
  return io_request(__ioc_read, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_write_bad_command(u64 type, u64 nr)
{
  return io_request(__ioc_write, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_readwrite_bad_command(u64 type, u64 nr)
{
  return io_request(__ioc_read | __ioc_write, type, nr, __sizeof_type<T>());
}

consteval u64
io_get_dir(u64 nr)
{
  return (((nr) >> __ioc_dirshift) & __ioc_dirmask);
}

consteval u64
io_get_type(u64 nr)
{
  return (((nr) >> __ioc_typeshift) & __ioc_typemask);
}

consteval u64
io_get_nr(u64 nr)
{
  return (((nr) >> __ioc_nrshift) & __ioc_nrmask);
}

consteval u64
io_get_size(u64 nr)
{
  return (((nr) >> __ioc_sizeshift) & __ioc_sizemask);
}

#define ioc_in (__ioc_write << __ioc_dirshift)
#define ioc_out (__ioc_read << __ioc_dirshift)
#define ioc_inout ((__ioc_write | __ioc_read) << __ioc_dirshift)
#define iocsize_mask (__ioc_sizemask << __ioc_sizeshift)
#define iocsize_shift (__ioc_sizeshift)

};
