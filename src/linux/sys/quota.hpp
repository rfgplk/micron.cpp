//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

namespace micron
{
namespace posix
{

// disk-quota control
constexpr int quota_subcmdshift = 8;
constexpr int quota_subcmdmask = 0x00ff;

constexpr int
qcmd(int subcmd, int type)
{
  return (subcmd << quota_subcmdshift) | (type & quota_subcmdmask);
}

// quota types
constexpr int usrquota = 0;
constexpr int grpquota = 1;
constexpr int prjquota = 2;
constexpr int maxquotas = 3;

// generic VFS quota subcommands (0x800000 | n)
constexpr int q_sync = 0x800001;
constexpr int q_quotaon = 0x800002;
constexpr int q_quotaoff = 0x800003;
constexpr int q_getfmt = 0x800004;
constexpr int q_getinfo = 0x800005;
constexpr int q_setinfo = 0x800006;
constexpr int q_getquota = 0x800007;
constexpr int q_setquota = 0x800008;
constexpr int q_getnextquota = 0x800009;

// on-disk quota formats
constexpr int qfmt_vfs_old = 1;
constexpr int qfmt_vfs_v0 = 2;
constexpr int qfmt_ocfs2 = 3;
constexpr int qfmt_vfs_v1 = 4;

// dqblk.dqb_valid bits (QIF_*)
constexpr u32 qif_blimits = 1;
constexpr u32 qif_space = 2;
constexpr u32 qif_ilimits = 4;
constexpr u32 qif_inodes = 8;
constexpr u32 qif_btime = 16;
constexpr u32 qif_itime = 32;
constexpr u32 qif_limits = (qif_blimits | qif_ilimits);
constexpr u32 qif_usage = (qif_space | qif_inodes);
constexpr u32 qif_times = (qif_btime | qif_itime);
constexpr u32 qif_all = (qif_limits | qif_usage | qif_times);

// dqinfo.dqi_valid bits (IIF_*)
constexpr u32 iif_bgrace = 1;
constexpr u32 iif_igrace = 2;
constexpr u32 iif_flags = 4;
constexpr u32 iif_all = (iif_bgrace | iif_igrace | iif_flags);

// dqinfo.dqi_flags (DQF_*)
constexpr u32 dqf_root_squash = 1;
constexpr u32 dqf_sys_file = 0x10000;

struct dqblk_t {
  u64 dqb_bhardlimit;      // absolute limit on disk quota blocks
  u64 dqb_bsoftlimit;      // preferred limit on disk quota blocks
  u64 dqb_curspace;        // current quota block count (bytes)
  u64 dqb_ihardlimit;      // maximum number of allocated inodes
  u64 dqb_isoftlimit;      // preferred inode limit
  u64 dqb_curinodes;       // current number of allocated inodes
  u64 dqb_btime;           // time limit for excessive disk use
  u64 dqb_itime;           // time limit for excessive files
  u32 dqb_valid;           // bitmask of QIF_* fields that are valid
};

struct dqinfo_t {
  u64 dqi_bgrace;      // seconds before the block soft limit becomes a hard limit
  u64 dqi_igrace;      // seconds before the inode soft limit becomes a hard limit
  u32 dqi_flags;       // DQF_* flags
  u32 dqi_valid;       // bitmask of IIF_* fields that are valid
};

inline auto
quotactl(int cmd, const char *special, int id, void *addr) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_quotactl, cmd, special, id, addr));
}

};      // namespace posix
};      // namespace micron
