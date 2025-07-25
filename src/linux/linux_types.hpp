//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// #include <time.h>
#include "../bits-types.hpp"

namespace micron
{
// TODO: think about renaming all posix namespaces to linux (as in reality it references modern linux impl. not the posix
// spec)
namespace posix
{
using idtype_t = __U32_TYPE;
using id_t = __U32_TYPE;
using pid_t = __pid_t;
using key_t = __key_t;
using uid_t = __uid_t;
using gid_t = __gid_t;
using off_t = __off_t;
using off64_t = __off64_t;
using suseconds_t = __suseconds_t;
using suseconds64_t = __suseconds64_t;
using rlim_t = __U64_TYPE;
using mode_t = __U32_TYPE;

using syscall_long_t = __S64_TYPE;
using syscall_ulong_t = __U64_TYPE;

using dev_t = __dev_t;

using ino64_t = __U64_TYPE;

using daddr_t = __S32_TYPE;
};

};
