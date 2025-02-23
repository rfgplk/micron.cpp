#pragma once

#include <time.h>
#include <signal.h>
#include "../types.hpp"

namespace micron
{
// TODO: think about renaming all posix namespaces to linux (as in reality it references modern linux impl. not the posix
// spec)
namespace posix
{
using idtype_t = u32;
using id_t = u32;
using dev_t = u64;
using pid_t = i32;
using key_t = i32;
using clockid_t = i32;
using uid_t = u32;
using gid_t = u32;
using rlim_t = u64;
using mode_t = u32;

using syscall_long_t = i64;
using syscall_ulong_t = u64;

using off_t = i64;
using off64_t = i64;

using ino64_t = u64;

using daddr_t = i32;
using key_t = i32;
};

};
