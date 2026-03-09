//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/addr.hpp"

#include "../../syscall.hpp"
#include "../sys/fcntl.hpp"
#include "../sys/stat.hpp"

#include "../../types.hpp"
#include "../sys/types.hpp"

#include "io_structs.hpp"

#include "fd.hpp"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%
// syscalls go here

namespace micron
{
// specifically in here, they are declared posix, in ../io.hpp they're propagated to micron
namespace posix
{

// these structs need to be here so we can use them for the syscall overloads
template <typename P>
max_t
read(i32 fd, P *buf, usize cnt)
{
  return micron::syscall(SYS_read, fd, micron::voidify(buf), cnt);
}

template <typename P>
max_t
write(i32 fd, P *buf, usize cnt)
{
  return micron::syscall(SYS_write, fd, micron::voidify(buf), cnt);
}

template <typename P>
max_t
read(i32 fd, P &buf, usize cnt)
{
  return micron::syscall(SYS_read, fd, micron::real_addr_as<byte>(buf), cnt);
}

template <typename P>
max_t
write(i32 fd, P &buf, usize cnt)
{
  return micron::syscall(SYS_write, fd, micron::real_addr_as<byte>(buf), cnt);
}

template <typename P>
max_t
read(fd_t fd, P *buf, usize cnt)
{
  return micron::syscall(SYS_read, fd.fd, micron::voidify(buf), cnt);
}

template <typename P>
max_t
write(fd_t fd, P *buf, usize cnt)
{
  return micron::syscall(SYS_write, fd.fd, micron::voidify(buf), cnt);
}

template <typename P>
max_t
read(fd_t fd, P &buf, usize cnt)
{
  return micron::syscall(SYS_read, fd.fd, micron::real_addr_as<byte>(buf), cnt);
}

template <typename P>
max_t
write(fd_t fd, P &buf, usize cnt)
{
  return micron::syscall(SYS_write, fd.fd, micron::real_addr_as<byte>(buf), cnt);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// positionals

template <typename P>
max_t
pread(i32 fd, P *buf, usize cnt, off_t offset)
{
  return micron::syscall(SYS_pread64, fd, micron::voidify(buf), cnt, offset);
}

template <typename P>
max_t
pwrite(i32 fd, P *buf, usize cnt, off_t offset)
{
  return micron::syscall(SYS_pwrite64, fd, micron::voidify(buf), cnt, offset);
}

template <typename P>
max_t
pread(fd_t fd, P *buf, usize cnt, off_t offset)
{
  return micron::syscall(SYS_pread64, fd.fd, micron::voidify(buf), cnt, offset);
}

template <typename P>
max_t
pwrite(fd_t fd, P *buf, usize cnt, off_t offset)
{
  return micron::syscall(SYS_pwrite64, fd.fd, micron::voidify(buf), cnt, offset);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// scatter/gathers

template <typename T>
max_t
writev(i32 fd, const iovec_t<T> &iov, i32 iovcnt)
{
  return micron::syscall(SYS_writev, fd, &iov, iovcnt);
}

template <typename T>
max_t
readv(i32 fd, iovec_t<T> &iov, i32 iovcnt)
{
  return micron::syscall(SYS_readv, fd, &iov, iovcnt);
}

template <typename T>
max_t
preadv(i32 fd, const iovec_t<T> &iov, i32 iovcnt, off_t offset)
{
  return micron::syscall(SYS_preadv, fd, &iov, iovcnt, offset);
}

template <typename T>
max_t
pwritev(i32 fd, iovec_t<T> &iov, i32 iovcnt, off_t offset)
{
  return micron::syscall(SYS_pwritev, fd, &iov, iovcnt, offset);
}

template <typename T>
max_t
writev(fd_t fd, const iovec_t<T> &iov, i32 iovcnt)
{
  return micron::syscall(SYS_writev, fd.fd, &iov, iovcnt);
}

template <typename T>
max_t
readv(fd_t fd, iovec_t<T> &iov, i32 iovcnt)
{
  return micron::syscall(SYS_readv, fd.fd, &iov, iovcnt);
}

template <typename T>
max_t
preadv(fd_t fd, const iovec_t<T> &iov, i32 iovcnt, off_t offset)
{
  return micron::syscall(SYS_preadv, fd.fd, &iov, iovcnt, offset);
}

template <typename T>
max_t
pwritev(fd_t fd, iovec_t<T> &iov, i32 iovcnt, off_t offset)
{
  return micron::syscall(SYS_pwritev, fd.fd, &iov, iovcnt, offset);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pipes

i32
pipe(i32 *fd)
{
  return static_cast<i32>(micron::syscall(SYS_pipe, fd));
}

i32
pipe2(i32 *fd, i32 a)
{
  return static_cast<i32>(micron::syscall(SYS_pipe2, fd, a));
}

struct pipe_pair {
  fd_t read_end;
  fd_t write_end;
};

inline pipe_pair
make_pipe(i32 flags = 0)
{
  i32 fds[2];
  i32 r = (flags == 0) ? static_cast<i32>(micron::syscall(SYS_pipe, fds)) : static_cast<i32>(micron::syscall(SYS_pipe2, fds, flags));
  if ( r < 0 )
    return { fd_t{ r }, fd_t{ r } };
  return { fd_t{ fds[0] }, fd_t{ fds[1] } };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dups

i32
dup(i32 old)
{
  return static_cast<i32>(micron::syscall(SYS_dup, old));
}

i32
dup2(i32 old, i32 newfd)
{
  return static_cast<i32>(micron::syscall(SYS_dup2, old, newfd));
}

i32
dup3(i32 old, i32 newfd, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_dup3, old, newfd, flags));
}

fd_t
dup(fd_t old)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_dup, old.fd)) };
}

fd_t
dup2(fd_t old, fd_t newfd)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_dup2, old.fd, newfd.fd)) };
}

fd_t
dup3(fd_t old, fd_t newfd, i32 flags)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_dup3, old.fd, newfd.fd, flags)) };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// closes

auto
close(i32 fd)
{
  return static_cast<i32>(micron::syscall(SYS_close, fd));
}

auto
close(fd_t &fd)
{
  i32 r = static_cast<i32>(micron::syscall(SYS_close, fd.fd));
  fd.reset();
  return r;
}

i32
shutdown(i32 fd, i32 how)
{
  return static_cast<i32>(micron::syscall(SYS_shutdown, fd, how));
}

i32
shutdown(fd_t fd, i32 how)
{
  return static_cast<i32>(micron::syscall(SYS_shutdown, fd.fd, how));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// opens

auto
open(const char *name, i32 flags, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_open, name, flags, mode));
}

auto
open(const char *name, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_open, name, flags, 0));
}

fd_t
open_fd(const char *name, i32 flags, u32 mode = 0)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_open, name, flags, mode)) };
}

auto
openat(i32 dirfd, const char *pth, i32 flags, u32 mode [[maybe_unused]])
{
  return static_cast<i32>(micron::syscall(SYS_openat, dirfd, pth, flags));
}

fd_t
openat(fd_t dirfd, const char *pth, i32 flags, u32 mode = 0)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_openat, dirfd.fd, pth, flags, mode)) };
}

auto
creat(const char *pth, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_creat, pth, mode));
}

fd_t
creat_fd(const char *pth, u32 mode)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_creat, pth, mode)) };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// syncs

i32
sync(void)
{
  return static_cast<i32>(micron::syscall(SYS_sync));
}

u32
umask(mode_t __mask)
{
  return static_cast<u32>(micron::syscall(SYS_umask, __mask));
}

i32
fsync(i32 fd)
{
  return static_cast<i32>(micron::syscall(SYS_fsync, fd));
}

i32
fsync(fd_t fd)
{
  return static_cast<i32>(micron::syscall(SYS_fsync, fd.fd));
}

i32
syncfs(i32 fd)
{
  return static_cast<i32>(micron::syscall(SYS_syncfs, fd));
}

i32
syncfs(fd_t fd)
{
  return static_cast<i32>(micron::syscall(SYS_syncfs, fd.fd));
}

i32
fdatasync(i32 fd)
{
  return static_cast<i32>(micron::syscall(SYS_fdatasync, fd));
}

i32
fdatasync(fd_t fd)
{
  return static_cast<i32>(micron::syscall(SYS_fdatasync, fd.fd));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// truncs

auto
truncate(const char *path, posix::off_t length)
{
  return static_cast<i32>(micron::syscall(SYS_truncate, path, length));
}

auto
ftruncate(i32 fd, posix::off_t length)
{
  return static_cast<i32>(micron::syscall(SYS_ftruncate, fd, length));
}

auto
ftruncate(fd_t fd, posix::off_t length)
{
  return static_cast<i32>(micron::syscall(SYS_ftruncate, fd.fd, length));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dirs

max_t
getcwd(char *buf, usize size)
{
  return (micron::syscall(SYS_getcwd, buf, size));
}

i32
chdir(const char *path)
{
  return static_cast<i32>(micron::syscall(SYS_chdir, path));
}

i32
fchdir(i32 fd)
{
  return static_cast<i32>(micron::syscall(SYS_fchdir, fd));
}

i32
fchdir(fd_t fd)
{
  return static_cast<i32>(micron::syscall(SYS_fchdir, fd.fd));
}

i32
rmdir(const char *name)
{
  return static_cast<i32>(micron::syscall(SYS_rmdir, name));
}

auto
mkdir(const char *name, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_mkdir, name, mode));
}

auto
mkdirat(i32 dirfd, const char *name, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_mkdirat, dirfd, name, mode));
}

auto
mkdirat(fd_t dirfd, const char *name, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_mkdirat, dirfd.fd, name, mode));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ch*s

auto
chroot(const char *path)
{
  return static_cast<i32>(micron::syscall(SYS_chroot, path));
}

i32
chmod(const char *name, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_chmod, name, mode));
}

i32
fchmod(i32 fd, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_fchmod, fd, mode));
}

i32
fchmod(fd_t fd, u32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_fchmod, fd.fd, mode));
}

i32
fchmodat(i32 dirfd, const char *name, u32 mode, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_fchmodat, dirfd, name, mode, flags));
}

i32
fchmodat(fd_t dirfd, const char *name, u32 mode, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_fchmodat, dirfd.fd, name, mode, flags));
}

auto
chown(const char *name, uid_t owner, gid_t group) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_chown, name, owner, group));
}

auto
fchown(i32 fd, uid_t owner, gid_t group) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fchown, fd, owner, group));
}

auto
fchown(fd_t fd, uid_t owner, gid_t group) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fchown, fd.fd, owner, group));
}

auto
lchown(const char *name, uid_t owner, gid_t group) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_lchown, name, owner, group));
}

auto
fchownat(i32 dirfd, const char *name, uid_t owner, gid_t group, i32 flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fchownat, dirfd, name, owner, group, flags));
}

auto
fchownat(fd_t dirfd, const char *name, uid_t owner, gid_t group, i32 flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fchownat, dirfd.fd, name, owner, group, flags));
}

auto
flock(i32 fd, i32 op) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_flock, fd, op));
}

auto
flock(fd_t fd, i32 op) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_flock, fd.fd, op));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// seeks
// off_t == long int
posix::off_t
lseek(i32 fd, posix::off_t offset, i32 whence)
{
  return (micron::syscall(SYS_lseek, fd, offset, whence));
}

posix::off_t
lseek(fd_t fd, posix::off_t offset, i32 whence)
{
  return (micron::syscall(SYS_lseek, fd.fd, offset, whence));
}

i32
fallocate(i32 fd, i32 mode, posix::off_t offset, posix::off_t len)
{
  return static_cast<i32>(micron::syscall(SYS_fallocate, fd, mode, offset, len));
}

i32
fallocate(fd_t fd, i32 mode, posix::off_t offset, posix::off_t len)
{
  return static_cast<i32>(micron::syscall(SYS_fallocate, fd.fd, mode, offset, len));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// renames

i32
rename(const char *__restrict oldpath, const char *__restrict newpath)
{
  return static_cast<i32>(micron::syscall(SYS_rename, oldpath, newpath));
}

i32
renameat(i32 oldfd, const char *__restrict oldpath, i32 newfd, const char *__restrict newpath)
{
  return static_cast<i32>(micron::syscall(SYS_renameat, oldfd, oldpath, newfd, newpath));
}

i32
renameat(fd_t oldfd, const char *__restrict oldpath, fd_t newfd, const char *__restrict newpath)
{
  return static_cast<i32>(micron::syscall(SYS_renameat, oldfd.fd, oldpath, newfd.fd, newpath));
}

i32
renameat2(i32 oldfd, const char *__restrict oldpath, i32 newfd, const char *__restrict newpath, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_renameat2, oldfd, oldpath, newfd, newpath, flags));
}

i32
renameat2(fd_t oldfd, const char *__restrict oldpath, fd_t newfd, const char *__restrict newpath, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_renameat2, oldfd.fd, oldpath, newfd.fd, newpath, flags));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// links

auto
link(const char *oldpath, const char *newpath)
{
  return static_cast<i32>(micron::syscall(SYS_link, oldpath, newpath));
}

auto
linkat(i32 olddirfd, const char *oldpath, i32 newdirfd, const char *newpath, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_linkat, olddirfd, oldpath, newdirfd, newpath, flags));
}

auto
linkat(fd_t olddirfd, const char *oldpath, fd_t newdirfd, const char *newpath, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_linkat, olddirfd.fd, oldpath, newdirfd.fd, newpath, flags));
}

auto
symlink(const char *target, const char *linkpath)
{
  return static_cast<i32>(micron::syscall(SYS_symlink, target, linkpath));
}

auto
symlinkat(const char *target, i32 newdirfd, const char *linkpath)
{
  return static_cast<i32>(micron::syscall(SYS_symlinkat, target, newdirfd, linkpath));
}

auto
symlinkat(const char *target, fd_t newdirfd, const char *linkpath)
{
  return static_cast<i32>(micron::syscall(SYS_symlinkat, target, newdirfd.fd, linkpath));
}

// max_t == long int
max_t
readlink(const char *path, char *buf, usize bufsiz)
{
  return micron::syscall(SYS_readlink, path, buf, bufsiz);
}

max_t
readlinkat(i32 dirfd, const char *path, char *buf, usize bufsiz)
{
  return micron::syscall(SYS_readlinkat, dirfd, path, buf, bufsiz);
}

max_t
readlinkat(fd_t dirfd, const char *path, char *buf, usize bufsiz)
{
  return micron::syscall(SYS_readlinkat, dirfd.fd, path, buf, bufsiz);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unlinks

i32
unlink(const char *path)
{
  return static_cast<i32>(micron::syscall(SYS_unlink, path));
}

i32
unlinkat(i32 dirfd, const char *path, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_unlinkat, dirfd, path, flags));
}

i32
unlinkat(fd_t dirfd, const char *path, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_unlinkat, dirfd.fd, path, flags));
}

i32
access(const char *path, i32 mode)
{
  return static_cast<i32>(micron::syscall(SYS_access, path, mode));
}

i32
faccessat(i32 dirfd, const char *name, i32 mode, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_faccessat2, dirfd, name, mode, flags));
}

i32
faccessat(fd_t dirfd, const char *name, i32 mode, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_faccessat2, dirfd.fd, name, mode, flags));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ents

max_t
getdents(i32 dirfd, __linux_kernel_dirent &dirp, u32 count)
{
  return (micron::syscall(SYS_getdents, dirfd, &dirp, count));
}

max_t
getdents64(i32 dirfd, __linux_kernel_dirent64 &dirp, u32 count)
{
  return (micron::syscall(SYS_getdents64, dirfd, &dirp, count));
}

max_t
getdents64(i32 dirfd, void *dirp, u32 count)
{
  return (micron::syscall(SYS_getdents64, dirfd, dirp, count));
}

max_t
getdents64(fd_t dirfd, void *dirp, u32 count)
{
  return (micron::syscall(SYS_getdents64, dirfd.fd, dirp, count));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mks

i32
mkfifo(const char *path, posix::mode_t mode)
{
  return static_cast<i32>(micron::syscall(SYS_mknod, path, mode | S_IFIFO, 0));
}

i32
mknod(const char *path, posix::mode_t mode, posix::dev_t dev)
{
  return static_cast<i32>(micron::syscall(SYS_mknod, path, mode, dev));
}

i32
mknodat(i32 dirfd, const char *path, posix::mode_t mode, posix::dev_t dev)
{
  return static_cast<i32>(micron::syscall(SYS_mknodat, dirfd, path, mode, dev));
}

i32
mknodat(fd_t dirfd, const char *path, posix::mode_t mode, posix::dev_t dev)
{
  return static_cast<i32>(micron::syscall(SYS_mknodat, dirfd.fd, path, mode, dev));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sendfile + splices

max_t
sendfile(i32 out_fd, i32 in_fd, posix::off_t *offset, usize count)
{
  return micron::syscall(SYS_sendfile, out_fd, in_fd, offset, count);
}

max_t
sendfile(fd_t out_fd, fd_t in_fd, posix::off_t *offset, usize count)
{
  return micron::syscall(SYS_sendfile, out_fd.fd, in_fd.fd, offset, count);
}

max_t
splice(i32 fd_in, posix::off_t *off_in, i32 fd_out, posix::off_t *off_out, usize len, u32 flags)
{
  return micron::syscall(SYS_splice, fd_in, off_in, fd_out, off_out, len, flags);
}

max_t
splice(fd_t fd_in, posix::off_t *off_in, fd_t fd_out, posix::off_t *off_out, usize len, u32 flags)
{
  return micron::syscall(SYS_splice, fd_in.fd, off_in, fd_out.fd, off_out, len, flags);
}

max_t
tee(i32 fd_in, i32 fd_out, usize len, u32 flags)
{
  return micron::syscall(SYS_tee, fd_in, fd_out, len, flags);
}

max_t
tee(fd_t fd_in, fd_t fd_out, usize len, u32 flags)
{
  return micron::syscall(SYS_tee, fd_in.fd, fd_out.fd, len, flags);
}

max_t
copy_file_range(i32 fd_in, posix::off_t *off_in, i32 fd_out, posix::off_t *off_out, usize len, u32 flags)
{
  return micron::syscall(SYS_copy_file_range, fd_in, off_in, fd_out, off_out, len, flags);
}

max_t
copy_file_range(fd_t fd_in, posix::off_t *off_in, fd_t fd_out, posix::off_t *off_out, usize len, u32 flags)
{
  return micron::syscall(SYS_copy_file_range, fd_in.fd, off_in, fd_out.fd, off_out, len, flags);
}

// NOTE: don't global shadow
fd_t
signalfd(fd_t fd, const void *__mask, usize sizemask, i32 flags)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_signalfd4, fd.fd, __mask, sizemask, flags)) };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// inotifys

fd_t
inotify_init(void)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_inotify_init)) };
}

fd_t
inotify_init1(i32 flags)
{
  return fd_t{ static_cast<i32>(micron::syscall(SYS_inotify_init1, flags)) };
}

i32
inotify_add_watch(fd_t fd, const char *path, u32 __mask)
{
  return static_cast<i32>(micron::syscall(SYS_inotify_add_watch, fd.fd, path, __mask));
}

i32
inotify_rm_watch(fd_t fd, i32 wd)
{
  return static_cast<i32>(micron::syscall(SYS_inotify_rm_watch, fd.fd, wd));
}

// inotify event masks
constexpr u32 in_access = 0x00000001;
constexpr u32 in_modify = 0x00000002;
constexpr u32 in_attrib = 0x00000004;
constexpr u32 in_close_write = 0x00000008;
constexpr u32 in_close_nowrite = 0x00000010;
constexpr u32 in_close = in_close_write | in_close_nowrite;
constexpr u32 in_open = 0x00000020;
constexpr u32 in_moved_from = 0x00000040;
constexpr u32 in_moved_to = 0x00000080;
constexpr u32 in_move = in_moved_from | in_moved_to;
constexpr u32 in_create = 0x00000100;
constexpr u32 in_delete = 0x00000200;
constexpr u32 in_delete_self = 0x00000400;
constexpr u32 in_move_self = 0x00000800;
constexpr u32 in_all_events = 0x00000FFF;
constexpr u32 in_nonblock = 04000;
constexpr u32 in_cloexec = 02000000;

// TODO: fix ISO c++ pedantry
struct inotify_event_t {
  i32 wd;
  u32 mask;
  u32 cookie;
  u32 len;
  char name[];
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// attrs

max_t
getxattr(const char *path, const char *name, void *value, usize size)
{
  return micron::syscall(SYS_getxattr, path, name, value, size);
}

max_t
lgetxattr(const char *path, const char *name, void *value, usize size)
{
  return micron::syscall(SYS_lgetxattr, path, name, value, size);
}

max_t
fgetxattr(i32 fd, const char *name, void *value, usize size)
{
  return micron::syscall(SYS_fgetxattr, fd, name, value, size);
}

max_t
fgetxattr(fd_t fd, const char *name, void *value, usize size)
{
  return micron::syscall(SYS_fgetxattr, fd.fd, name, value, size);
}

i32
setxattr(const char *path, const char *name, const void *value, usize size, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_setxattr, path, name, value, size, flags));
}

i32
lsetxattr(const char *path, const char *name, const void *value, usize size, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_lsetxattr, path, name, value, size, flags));
}

i32
fsetxattr(i32 fd, const char *name, const void *value, usize size, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_fsetxattr, fd, name, value, size, flags));
}

i32
fsetxattr(fd_t fd, const char *name, const void *value, usize size, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_fsetxattr, fd.fd, name, value, size, flags));
}

max_t
listxattr(const char *path, char *list, usize size)
{
  return micron::syscall(SYS_listxattr, path, list, size);
}

max_t
llistxattr(const char *path, char *list, usize size)
{
  return micron::syscall(SYS_llistxattr, path, list, size);
}

max_t
flistxattr(fd_t fd, char *list, usize size)
{
  return micron::syscall(SYS_flistxattr, fd.fd, list, size);
}

i32
removexattr(const char *path, const char *name)
{
  return static_cast<i32>(micron::syscall(SYS_removexattr, path, name));
}

i32
fremovexattr(fd_t fd, const char *name)
{
  return static_cast<i32>(micron::syscall(SYS_fremovexattr, fd.fd, name));
}

};     // namespace posix
};     // namespace micron
