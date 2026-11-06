//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <micron/errno.hpp>
#include <micron/linux/io/fd.hpp>
#include <micron/linux/sys/fcntl.hpp>
#include <micron/syscall.hpp>
#include <micron/types.hpp>

// added for compatibility

namespace micron::posix
{
struct unix_address {
  u16 family = 1;
  char path[108]{};
};

inline fd_t
unix_stream_socket()
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_socket, 1, 1 | o_cloexec, 0)) };
}

inline i32
unix_connect(fd_t fd, const unix_address &address, u32 length)
{
  if ( length < 3 || length > sizeof(unix_address) ) return -error::invalid_arg;
  return static_cast<i32>(micron::syscall(SYS_connect, fd.fd, &address, length));
}
};      // namespace micron::posix
