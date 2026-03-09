//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/addr.hpp"

#include "../syscall.hpp"
#include "sys/fcntl.hpp"
#include "sys/stat.hpp"

#include "../types.hpp"
#include "sys/types.hpp"

#include "io/inode.hpp"
#include "io/io_structs.hpp"

#include "io/ext.hpp"     // for non syscall fns
#include "io/sys.hpp"     // for syscalls

namespace micron
{
using posix::stat_t;

using posix::fstat;
using posix::fstatat;
using posix::lstat;
using posix::stat;

using posix::dir_t;
using posix::fd_t;
using posix::inotify_event_t;
using posix::invalid_fd;
using posix::iovec_t;

using posix::major;
using posix::makedev;
using posix::minor;

// basic syscalls
using posix::pread;
using posix::preadv;
using posix::pwrite;
using posix::pwritev;
using posix::read;
using posix::readv;
using posix::write;
using posix::writev;

// pipes
using posix::make_pipe;
using posix::pipe;
using posix::pipe2;
using posix::pipe_pair;

// dups
using posix::dup;
using posix::dup2;
using posix::dup3;

// closes / shutdown
using posix::close;
using posix::shutdown;

// opens
using posix::creat;
using posix::creat_fd;
using posix::open;
using posix::open_fd;
using posix::openat;

// syncs
using posix::fdatasync;
using posix::fsync;
using posix::sync;
using posix::syncfs;
using posix::umask;

// truncs
using posix::ftruncate;
using posix::truncate;

// dirs
using posix::chdir;
using posix::fchdir;
using posix::getcwd;
using posix::mkdir;
using posix::mkdirat;
using posix::rmdir;

// ch*s
using posix::chmod;
using posix::chown;
using posix::chroot;
using posix::fchmod;
using posix::fchmodat;
using posix::fchown;
using posix::fchownat;
using posix::flock;
using posix::lchown;

// seeks
using posix::fallocate;
using posix::lseek;

// renames
using posix::rename;
using posix::renameat;
using posix::renameat2;

// links
using posix::link;
using posix::linkat;
using posix::readlink;
using posix::readlinkat;
using posix::symlink;
using posix::symlinkat;

// unlinks / access
using posix::access;
using posix::faccessat;
using posix::unlink;
using posix::unlinkat;

// ents / getdents
using posix::getdents;
using posix::getdents64;

// mks
using posix::mkfifo;
using posix::mknod;
using posix::mknodat;

// sendfile / splice
using posix::copy_file_range;
using posix::sendfile;
using posix::signalfd;
using posix::splice;
using posix::tee;

// inotify
using posix::inotify_add_watch;
using posix::inotify_init;
using posix::inotify_init1;
using posix::inotify_rm_watch;

using posix::in_access;
using posix::in_all_events;
using posix::in_attrib;
using posix::in_cloexec;
using posix::in_close;
using posix::in_close_nowrite;
using posix::in_close_write;
using posix::in_create;
using posix::in_delete;
using posix::in_delete_self;
using posix::in_modify;
using posix::in_move;
using posix::in_move_self;
using posix::in_moved_from;
using posix::in_moved_to;
using posix::in_nonblock;
using posix::in_open;

// attrs / xattr
using posix::fgetxattr;
using posix::flistxattr;
using posix::fremovexattr;
using posix::fsetxattr;
using posix::getxattr;
using posix::lgetxattr;
using posix::listxattr;
using posix::llistxattr;
using posix::lsetxattr;
using posix::removexattr;
using posix::setxattr;

// from inode
using posix::node_types;

using posix::is_absolute;
using posix::is_dot_entry;
using posix::is_relative;
using posix::verify;

using posix::exists;
using posix::exists_at;
using posix::lexists;

using posix::get_type;
using posix::get_type_at;
using posix::is_inode_type;
using posix::is_inode_type_at;

using posix::is_virtual_file;

using posix::is_block_device;
using posix::is_char_device;
using posix::is_dir;
using posix::is_fifo;
using posix::is_file;
using posix::is_pipe;
// NOTE: conflicts with the conflict
using posix::is_regular_node;
using posix::is_socket;
using posix::is_symlink;

using posix::is_block_device_at;
using posix::is_char_device_at;
using posix::is_dir_at;
using posix::is_fifo_at;
using posix::is_file_at;
using posix::is_pipe_at;
using posix::is_regular_at;
using posix::is_socket_at;
using posix::is_symlink_at;

using posix::is_executable;
using posix::is_readable;
using posix::is_writable;

using posix::is_executable_at;
using posix::is_readable_at;
using posix::is_writable_at;

using posix::has_setgid;
using posix::has_setuid;
using posix::has_sticky;

using posix::mode_group_exec;
using posix::mode_group_read;
using posix::mode_group_write;
using posix::mode_other_exec;
using posix::mode_other_read;
using posix::mode_other_write;
using posix::mode_user_exec;
using posix::mode_user_read;
using posix::mode_user_write;

using posix::is_in_group;
using posix::is_owned_by;

using posix::has_content;
// NOTE: conflicts with the type trait
using posix::is_empty_dir;
using posix::is_empty_file;
using posix::is_empty_node;

using posix::is_mountpoint;

using posix::is_same_file;

using posix::get_device;
using posix::get_gid;
using posix::get_inode;
using posix::get_link_count;
using posix::get_mode;
using posix::get_permissions;
using posix::get_size;
using posix::get_uid;

};     // namespace micron
