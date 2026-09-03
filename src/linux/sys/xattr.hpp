//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

#include "../io/sys.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// xattr vocabulary: flags and the security.* names
//
// NOTE: the {,l,f}{get,set,list,remove}xattr syscalls live in linux/io/sys.hpp

namespace micron
{
namespace posix
{

constexpr static const i32 xattr_create = 0x1;
constexpr static const i32 xattr_replace = 0x2;

constexpr static const char *xattr_security_prefix = "security.";
constexpr static const char *xattr_trusted_prefix = "trusted.";
constexpr static const char *xattr_system_prefix = "system.";
constexpr static const char *xattr_user_prefix = "user.";

constexpr static const char *xattr_name_selinux = "security.selinux";
constexpr static const char *xattr_name_caps = "security.capability";
constexpr static const char *xattr_name_ima = "security.ima";
constexpr static const char *xattr_name_evm = "security.evm";
constexpr static const char *xattr_name_apparmor = "security.apparmor";
constexpr static const char *xattr_name_smack = "security.SMACK64";

inline max_t
flistxattr(i32 fd, char *list, usize size)
{
  return static_cast<max_t>(micron::syscall(SYS_flistxattr, fd, list, size));
}

inline i32
lremovexattr(const char *path, const char *name)
{
  return static_cast<i32>(micron::syscall(SYS_lremovexattr, path, name));
}

};      // namespace posix
};      // namespace micron
