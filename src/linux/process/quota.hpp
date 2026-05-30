//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../sys/quota.hpp"

#include "../../types.hpp"

namespace micron
{

enum class quota_type : int {
  user = posix::usrquota,
  group = posix::grpquota,
  project = posix::prjquota,
};

enum class quota_format : int {
  vfs_old = posix::qfmt_vfs_old,
  vfs_v0 = posix::qfmt_vfs_v0,
  ocfs2 = posix::qfmt_ocfs2,
  vfs_v1 = posix::qfmt_vfs_v1,
};

inline i32
quota_on(const char *device, quota_type t, quota_format fmt, const char *quota_file) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_quotaon, static_cast<int>(t)), device, static_cast<int>(fmt), const_cast<char *>(quota_file));
}

inline i32
quota_off(const char *device, quota_type t) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_quotaoff, static_cast<int>(t)), device, 0, nullptr);
}

inline i32
quota_sync(const char *device, quota_type t) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_sync, static_cast<int>(t)), device, 0, nullptr);
}

inline i32
quota_get_format(const char *device, quota_type t, u32 &fmt_out) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_getfmt, static_cast<int>(t)), device, 0, &fmt_out);
}

inline i32
get_quota(const char *device, quota_type t, u32 id, posix::dqblk_t &out) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_getquota, static_cast<int>(t)), device, static_cast<int>(id), &out);
}

inline i32
get_next_quota(const char *device, quota_type t, u32 id, posix::dqblk_t &out) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_getnextquota, static_cast<int>(t)), device, static_cast<int>(id), &out);
}

inline i32
set_quota(const char *device, quota_type t, u32 id, posix::dqblk_t &in) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_setquota, static_cast<int>(t)), device, static_cast<int>(id), &in);
}

inline i32
get_quota_info(const char *device, quota_type t, posix::dqinfo_t &out) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_getinfo, static_cast<int>(t)), device, 0, &out);
}

inline i32
set_quota_info(const char *device, quota_type t, posix::dqinfo_t &in) noexcept
{
  return posix::quotactl(posix::qcmd(posix::q_setinfo, static_cast<int>(t)), device, 0, &in);
}

};      // namespace micron
