//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits-types.hpp"

namespace micron
{
// TODO: think about renaming all posix namespaces to linux (as in reality it references modern linux impl. not the posix
// spec)
namespace posix
{
using idtype_t = __u32_type;
using id_t = __u32_type;
using pid_t = __pid_t;
using key_t = __key_t;
using uid_t = __uid_t;
using gid_t = __gid_t;
using suseconds_t = __suseconds_t;
using suseconds64_t = __suseconds64_t;
using rlim_t = __rlim_t;
using mode_t = __mode_t;
using off_t = __off_t;
using off64_t = __off64_t;

using syscall_long_t = __syscall_slong_type;
using syscall_ulong_t = __syscall_ulong_type;
using blksize_t = __blksize_t_type;
using blkcnt_t = __blkcnt_t_type;
using nlink_t = __nlink_t_type;
using clock_t = __clock_t_type;

using dev_t = __dev_t;

using time_t = __time_t;
using ino_t = __ino_t;
using ino64_t = __ino64_t;

using daddr_t = __s32_type;
};


};
