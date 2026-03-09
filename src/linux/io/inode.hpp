//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../sys/stat.hpp"
#include "io_structs.hpp"

constexpr static const usize __max_name_length = 256;

namespace micron
{
namespace posix
{

inline posix::dev_t
makedev(u32 major, u32 minor)
{
  return ((posix::dev_t)(major & 0xfff) << 8) | (posix::dev_t)(minor & 0xff) | ((posix::dev_t)(minor & 0xfff00) << 12);
}

inline u32
major(posix::dev_t dev)
{
  return (u32)((dev >> 8) & 0xfff);
}

inline u32
minor(posix::dev_t dev)
{
  return (u32)((dev & 0xff) | ((dev >> 12) & 0xfff00));
}

enum class node_types : i32 {
  socket = inode_sock,
  symlink = inode_link,
  regular_file = inode_reg,
  block_device = inode_blck,
  directory = inode_dir,
  character_device = inode_char,
  fifo = inode_fifo,
  not_found,
  unknown
};

namespace __impl
{

inline bool
__stat(const char *path, stat_t &out)
{
  return posix::stat(path, out) == 0;
}

inline bool
__lstat(const char *path, stat_t &out)
{
  return posix::lstat(path, out) == 0;
}

inline bool
__fstat(posix::fd_t fd, stat_t &out)
{
  return posix::fstat(fd, out) == 0;
}

inline bool
__fstatat(posix::fd_t dirfd, const char *path, stat_t &out, i32 flags = 0)
{
  return posix::fstatat(dirfd, path, out, flags) == 0;
}

inline bool
stat_is_reg(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_reg;
}

inline bool
stat_is_dir(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_dir;
}

inline bool
stat_is_lnk(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_link;
}

inline bool
stat_is_fifo(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_fifo;
}

inline bool
stat_is_sock(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_sock;
}

inline bool
stat_is_chr(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_char;
}

inline bool
stat_is_blk(const stat_t &b)
{
  return (b.st_mode & __format_mask) == inode_blck;
}

inline bool
mode_bit(const stat_t &b, u32 bit)
{
  return (b.st_mode & bit) != 0;
}

inline bool
mode_bit(const char *p, u32 bit)
{
  stat_t b{};
  return __stat(p, b) && (b.st_mode & bit);
}

inline bool
mode_bit(posix::fd_t fd, u32 bit)
{
  stat_t b{};
  return __fstat(fd, b) && (b.st_mode & bit);
}

inline bool
mode_bit(const char *p, u32 bit, stat_t &out)
{
  return __stat(p, out) && (out.st_mode & bit);
}

}     // namespace __impl

inline bool
verify(const char *str)
{
  return str != nullptr && str[0] != '\0';
}

template <is_string T>
inline bool
verify(const T &str)
{
  return str.cdata() != nullptr && str.size() != 0 && str[0] != '\0';
}

inline bool
is_absolute(const char *path)
{
  return path != nullptr && path[0] == '/';
}

template <is_string T>
inline bool
is_absolute(const T &path)
{
  return path.size() > 0 && path[0] == '/';
}

inline bool
is_relative(const char *path)
{
  return path != nullptr && path[0] != '/' && path[0] != '\0';
}

template <is_string T>
inline bool
is_relative(const T &path)
{
  return path.size() > 0 && path[0] != '/';
}

inline bool
is_dot_entry(const char *name)
{
  return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

inline bool
exists(const char *path)
{
  stat_t buf{};
  return __impl::__stat(path, buf);
}

inline bool
exists(const char *path, stat_t &buf)
{
  return __impl::__stat(path, buf);
}

template <is_string T>
inline bool
exists(const T &path)
{
  return exists(path.c_str());
}

template <is_string T>
inline bool
exists(const T &path, stat_t &buf)
{
  return exists(path.c_str(), buf);
}

inline bool
lexists(const char *path)
{
  stat_t buf{};
  return __impl::__lstat(path, buf);
}

inline bool
lexists(const char *path, stat_t &buf)
{
  return __impl::__lstat(path, buf);
}

template <is_string T>
inline bool
lexists(const T &path)
{
  return lexists(path.c_str());
}

template <is_string T>
inline bool
lexists(const T &path, stat_t &buf)
{
  return lexists(path.c_str(), buf);
}

inline bool
exists_at(posix::fd_t dirfd, const char *path)
{
  stat_t buf{};
  return __impl::__fstatat(dirfd, path, buf);
}

inline bool
exists_at(posix::fd_t dirfd, const char *path, stat_t &buf)
{
  return __impl::__fstatat(dirfd, path, buf);
}

inline bool
exists_at(i32 dirfd, const char *path)
{
  return exists_at(posix::fd_t{ dirfd }, path);
}

inline bool
exists_at(i32 dirfd, const char *path, stat_t &buf)
{
  return exists_at(posix::fd_t{ dirfd }, path, buf);
}

template <is_string T>
inline bool
exists_at(posix::fd_t dirfd, const T &path)
{
  return exists_at(dirfd, path.c_str());
}

template <is_string T>
inline bool
exists_at(posix::fd_t dirfd, const T &path, stat_t &buf)
{
  return exists_at(dirfd, path.c_str(), buf);
}

template <is_string T>
inline bool
exists_at(i32 dirfd, const T &path)
{
  return exists_at(posix::fd_t{ dirfd }, path.c_str());
}

template <is_string T>
inline bool
exists_at(i32 dirfd, const T &path, stat_t &buf)
{
  return exists_at(posix::fd_t{ dirfd }, path.c_str(), buf);
}

template <node_types Tp>
inline bool
is_inode_type(const stat_t &buf)
{
  return (buf.st_mode & __format_mask) == static_cast<i32>(Tp);
}

template <node_types Tp>
inline bool
is_inode_type(const char *path, stat_t &buf)
{
  bool ok = (Tp == node_types::symlink) ? __impl::__lstat(path, buf) : __impl::__stat(path, buf);
  if ( !ok )
    return false;
  return (buf.st_mode & __format_mask) == static_cast<i32>(Tp);
}

template <node_types Tp, is_string T>
inline bool
is_inode_type(const T &path, stat_t &buf)
{
  return is_inode_type<Tp>(path.c_str(), buf);
}

template <node_types Tp>
inline bool
is_inode_type(posix::fd_t fd, stat_t &buf)
{
  if ( !__impl::__fstat(fd, buf) )
    return false;
  return (buf.st_mode & __format_mask) == static_cast<i32>(Tp);
}

template <node_types Tp>
inline bool
is_inode_type(i32 fd, stat_t &buf)
{
  return is_inode_type<Tp>(posix::fd_t{ fd }, buf);
}

template <node_types Tp>
inline bool
is_inode_type(const char *path)
{
  stat_t buf{};
  return is_inode_type<Tp>(path, buf);
}

template <node_types Tp, is_string T>
inline bool
is_inode_type(const T &path)
{
  return is_inode_type<Tp>(path.c_str());
}

template <node_types Tp>
inline bool
is_inode_type(posix::fd_t fd)
{
  stat_t buf{};
  return is_inode_type<Tp>(fd, buf);
}

template <node_types Tp>
inline bool
is_inode_type(i32 fd)
{
  return is_inode_type<Tp>(posix::fd_t{ fd });
}

template <node_types Tp>
inline bool
is_inode_type_at(const stat_t &buf)
{
  return (buf.st_mode & __format_mask) == static_cast<i32>(Tp);
}

template <node_types Tp>
inline bool
is_inode_type_at(posix::fd_t dirfd, const char *path, stat_t &buf)
{
  i32 flags = (Tp == node_types::symlink) ? posix::at_symlink_nofollow : 0;
  if ( !__impl::__fstatat(dirfd, path, buf, flags) )
    return false;
  return (buf.st_mode & __format_mask) == static_cast<i32>(Tp);
}

template <node_types Tp>
inline bool
is_inode_type_at(i32 dirfd, const char *path, stat_t &buf)
{
  return is_inode_type_at<Tp>(posix::fd_t{ dirfd }, path, buf);
}

template <node_types Tp, is_string T>
inline bool
is_inode_type_at(posix::fd_t dirfd, const T &path, stat_t &buf)
{
  return is_inode_type_at<Tp>(dirfd, path.c_str(), buf);
}

template <node_types Tp, is_string T>
inline bool
is_inode_type_at(i32 dirfd, const T &path, stat_t &buf)
{
  return is_inode_type_at<Tp>(posix::fd_t{ dirfd }, path.c_str(), buf);
}

template <node_types Tp>
inline bool
is_inode_type_at(posix::fd_t dirfd, const char *path)
{
  stat_t buf{};
  return is_inode_type_at<Tp>(dirfd, path, buf);
}

template <node_types Tp>
inline bool
is_inode_type_at(i32 dirfd, const char *path)
{
  return is_inode_type_at<Tp>(posix::fd_t{ dirfd }, path);
}

template <node_types Tp, is_string T>
inline bool
is_inode_type_at(posix::fd_t dirfd, const T &path)
{
  return is_inode_type_at<Tp>(dirfd, path.c_str());
}

template <node_types Tp, is_string T>
inline bool
is_inode_type_at(i32 dirfd, const T &path)
{
  return is_inode_type_at<Tp>(posix::fd_t{ dirfd }, path.c_str());
}

inline node_types
get_type(const stat_t &buf)
{
  return static_cast<node_types>(buf.st_mode & __format_mask);
}

inline node_types
get_type(const char *path, stat_t &buf)
{
  if ( !__impl::__stat(path, buf) )
    return node_types::not_found;
  return static_cast<node_types>(buf.st_mode & __format_mask);
}

template <is_string T>
inline node_types
get_type(const T &path, stat_t &buf)
{
  return get_type(path.c_str(), buf);
}

inline node_types
get_type(posix::fd_t fd, stat_t &buf)
{
  if ( !__impl::__fstat(fd, buf) )
    return node_types::not_found;
  return static_cast<node_types>(buf.st_mode & __format_mask);
}

inline node_types
get_type(i32 fd, stat_t &buf)
{
  return get_type(posix::fd_t{ fd }, buf);
}

inline node_types
get_type(const char *path)
{
  stat_t buf{};
  return get_type(path, buf);
}

template <is_string T>
inline node_types
get_type(const T &path)
{
  return get_type(path.c_str());
}

inline node_types
get_type(posix::fd_t fd)
{
  stat_t buf{};
  return get_type(fd, buf);
}

inline node_types
get_type(i32 fd)
{
  return get_type(posix::fd_t{ fd });
}

inline node_types
get_type_at(const char *path)
{
  stat_t buf{};
  if ( !__impl::__stat(path, buf) )
    return node_types::not_found;
  return static_cast<node_types>(buf.st_mode & __format_mask);
}

inline node_types
get_type_at(posix::fd_t dirfd, const char *path, stat_t &buf)
{
  if ( !__impl::__fstatat(dirfd, path, buf) )
    return node_types::not_found;
  return static_cast<node_types>(buf.st_mode & __format_mask);
}

inline node_types
get_type_at(i32 dirfd, const char *path, stat_t &buf)
{
  return get_type_at(posix::fd_t{ dirfd }, path, buf);
}

template <is_string T>
inline node_types
get_type_at(posix::fd_t dirfd, const T &path, stat_t &buf)
{
  return get_type_at(dirfd, path.c_str(), buf);
}

template <is_string T>
inline node_types
get_type_at(i32 dirfd, const T &path, stat_t &buf)
{
  return get_type_at(posix::fd_t{ dirfd }, path.c_str(), buf);
}

inline node_types
get_type_at(posix::fd_t dirfd, const char *path)
{
  stat_t b{};
  return get_type_at(dirfd, path, b);
}

inline node_types
get_type_at(i32 dirfd, const char *path)
{
  return get_type_at(posix::fd_t{ dirfd }, path);
}

template <is_string T>
inline node_types
get_type_at(posix::fd_t dirfd, const T &path)
{
  return get_type_at(dirfd, path.c_str());
}

template <is_string T>
inline node_types
get_type_at(i32 dirfd, const T &path)
{
  return get_type_at(posix::fd_t{ dirfd }, path.c_str());
}

inline bool
is_virtual_file(const stat_t &buf)
{
  return __impl::stat_is_reg(buf) && micron::posix::major(buf.st_dev) == 0;
}

inline bool
is_virtual_file(posix::fd_t fd, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && is_virtual_file(buf);
}

inline bool
is_virtual_file(posix::fd_t fd)
{
  stat_t b{};
  return is_virtual_file(fd, b);
}

inline bool
is_virtual_file(i32 fd)
{
  return is_virtual_file(posix::fd_t{ fd });
}

inline bool
is_virtual_file(const char *path, stat_t &buf)
{
  return __impl::__stat(path, buf) && is_virtual_file(buf);
}

inline bool
is_virtual_file(const char *path)
{
  stat_t b{};
  return is_virtual_file(path, b);
}

template <is_string T>
inline bool
is_virtual_file(const T &path, stat_t &buf)
{
  return is_virtual_file(path.c_str(), buf);
}

template <is_string T>
inline bool
is_virtual_file(const T &path)
{
  return is_virtual_file(path.c_str());
}

#define MICRON_INODE_PREDICATE(fn_name_, node_type_)                                                                                       \
  /* (1) read-only */                                                                                                                      \
  inline bool fn_name_(const stat_t &buf) { return is_inode_type<node_type_>(buf); }                                                       \
  /* (2a) path, prefill buf */                                                                                                             \
  inline bool fn_name_(const char *p, stat_t &buf) { return is_inode_type<node_type_>(p, buf); }                                           \
  template <is_string T> bool fn_name_(const T &s, stat_t &buf) { return fn_name_(s.c_str(), buf); }                                       \
  /* (2b) fd, prefill buf */                                                                                                               \
  inline bool fn_name_(posix::fd_t fd, stat_t &buf) { return is_inode_type<node_type_>(fd, buf); }                                         \
  inline bool fn_name_(i32 fd, stat_t &buf) { return fn_name_(posix::fd_t{ fd }, buf); }                                                   \
  /* (3a) fd, internal buf */                                                                                                              \
  inline bool fn_name_(posix::fd_t fd) { return is_inode_type<node_type_>(fd); }                                                           \
  inline bool fn_name_(i32 fd) { return fn_name_(posix::fd_t{ fd }); }                                                                     \
  /* (3b) path, internal buf */                                                                                                            \
  inline bool fn_name_(const char *p) { return is_inode_type<node_type_>(p); }                                                             \
  template <is_string T> bool fn_name_(const T &s) { return fn_name_(s.c_str()); }

#define MICRON_INODE_AT_PREDICATE(fn_name_, node_type_)                                                                                    \
  /* (1) read-only */                                                                                                                      \
  inline bool fn_name_(const stat_t &buf) { return is_inode_type_at<node_type_>(buf); }                                                    \
  /* (2) dirfd + path, prefill buf */                                                                                                      \
  inline bool fn_name_(posix::fd_t d, const char *p, stat_t &buf) { return is_inode_type_at<node_type_>(d, p, buf); }                      \
  inline bool fn_name_(i32 d, const char *p, stat_t &buf) { return fn_name_(posix::fd_t{ d }, p, buf); }                                   \
  template <is_string T> bool fn_name_(posix::fd_t d, const T &s, stat_t &buf) { return fn_name_(d, s.c_str(), buf); }                     \
  template <is_string T> bool fn_name_(i32 d, const T &s, stat_t &buf) { return fn_name_(posix::fd_t{ d }, s.c_str(), buf); }              \
  /* (3) dirfd + path, internal buf */                                                                                                     \
  inline bool fn_name_(posix::fd_t d, const char *p) { return is_inode_type_at<node_type_>(d, p); }                                        \
  inline bool fn_name_(i32 d, const char *p) { return fn_name_(posix::fd_t{ d }, p); }                                                     \
  template <is_string T> bool fn_name_(posix::fd_t d, const T &s) { return fn_name_(d, s.c_str()); }                                       \
  template <is_string T> bool fn_name_(i32 d, const T &s) { return fn_name_(posix::fd_t{ d }, s.c_str()); }

MICRON_INODE_PREDICATE(is_file, node_types::regular_file)
MICRON_INODE_PREDICATE(is_dir, node_types::directory)
MICRON_INODE_PREDICATE(is_socket, node_types::socket)
MICRON_INODE_PREDICATE(is_symlink, node_types::symlink)
MICRON_INODE_PREDICATE(is_block_device, node_types::block_device)
MICRON_INODE_PREDICATE(is_char_device, node_types::character_device)
MICRON_INODE_PREDICATE(is_fifo, node_types::fifo)

MICRON_INODE_AT_PREDICATE(is_file_at, node_types::regular_file)
MICRON_INODE_AT_PREDICATE(is_dir_at, node_types::directory)
MICRON_INODE_AT_PREDICATE(is_socket_at, node_types::socket)
MICRON_INODE_AT_PREDICATE(is_symlink_at, node_types::symlink)
MICRON_INODE_AT_PREDICATE(is_block_device_at, node_types::block_device)
MICRON_INODE_AT_PREDICATE(is_char_device_at, node_types::character_device)
MICRON_INODE_AT_PREDICATE(is_fifo_at, node_types::fifo)

#undef MICRON_INODE_PREDICATE
#undef MICRON_INODE_AT_PREDICATE

inline bool
is_pipe(const stat_t &b)
{
  return is_fifo(b);
}

inline bool
is_regular_node(const stat_t &b)
{
  return is_file(b);
}

inline bool
is_pipe(const char *p, stat_t &buf)
{
  return is_fifo(p, buf);
}

inline bool
is_regular_node(const char *p, stat_t &buf)
{
  return is_file(p, buf);
}

inline bool
is_pipe(posix::fd_t fd, stat_t &buf)
{
  return is_fifo(fd, buf);
}

inline bool
is_regular_node(posix::fd_t fd, stat_t &buf)
{
  return is_file(fd, buf);
}

inline bool
is_pipe(i32 fd, stat_t &buf)
{
  return is_fifo(posix::fd_t{ fd }, buf);
}

inline bool
is_regular_node(i32 fd, stat_t &buf)
{
  return is_file(posix::fd_t{ fd }, buf);
}

inline bool
is_pipe(posix::fd_t fd)
{
  return is_fifo(fd);
}

inline bool
is_regular_node(posix::fd_t fd)
{
  return is_file(fd);
}

inline bool
is_pipe(i32 fd)
{
  return is_pipe(posix::fd_t{ fd });
}

inline bool
is_regular_node(i32 fd)
{
  return is_regular_node(posix::fd_t{ fd });
}

inline bool
is_pipe(const char *p)
{
  return is_fifo(p);
}

inline bool
is_regular_node(const char *p)
{
  return is_file(p);
}

template <is_string T>
bool
is_pipe(const T &s)
{
  return is_fifo(s);
}

template <is_string T>
bool
is_regular_node(const T &s)
{
  return is_file(s);
}

inline bool
is_pipe_at(const stat_t &b)
{
  return is_fifo_at(b);
}

inline bool
is_regular_at(const stat_t &b)
{
  return is_file_at(b);
}

inline bool
is_pipe_at(posix::fd_t d, const char *p, stat_t &buf)
{
  return is_fifo_at(d, p, buf);
}

inline bool
is_regular_at(posix::fd_t d, const char *p, stat_t &buf)
{
  return is_file_at(d, p, buf);
}

inline bool
is_pipe_at(posix::fd_t d, const char *p)
{
  return is_fifo_at(d, p);
}

inline bool
is_regular_at(posix::fd_t d, const char *p)
{
  return is_file_at(d, p);
}

inline bool
is_pipe_at(i32 d, const char *p)
{
  return is_fifo_at(posix::fd_t{ d }, p);
}

inline bool
is_regular_at(i32 d, const char *p)
{
  return is_file_at(posix::fd_t{ d }, p);
}

template <is_string T>
bool
is_pipe_at(posix::fd_t d, const T &s)
{
  return is_fifo_at(d, s);
}

template <is_string T>
bool
is_regular_at(posix::fd_t d, const T &s)
{
  return is_file_at(d, s);
}

inline bool
is_readable(const char *p)
{
  return micron::syscall(SYS_access, p, posix::r_ok) == 0;
}

inline bool
is_writable(const char *p)
{
  return micron::syscall(SYS_access, p, posix::w_ok) == 0;
}

inline bool
is_executable(const char *p)
{
  return micron::syscall(SYS_access, p, posix::x_ok) == 0;
}

template <is_string T>
bool
is_readable(const T &s)
{
  return is_readable(s.c_str());
}

template <is_string T>
bool
is_writable(const T &s)
{
  return is_writable(s.c_str());
}

template <is_string T>
bool
is_executable(const T &s)
{
  return is_executable(s.c_str());
}

inline bool
is_readable_at(posix::fd_t d, const char *p)
{
  return micron::syscall(SYS_faccessat2, d.fd, p, posix::r_ok, 0) == 0;
}

inline bool
is_writable_at(posix::fd_t d, const char *p)
{
  return micron::syscall(SYS_faccessat2, d.fd, p, posix::w_ok, 0) == 0;
}

inline bool
is_executable_at(posix::fd_t d, const char *p)
{
  return micron::syscall(SYS_faccessat2, d.fd, p, posix::x_ok, 0) == 0;
}

inline bool
is_readable_at(i32 d, const char *p)
{
  return is_readable_at(posix::fd_t{ d }, p);
}

inline bool
is_writable_at(i32 d, const char *p)
{
  return is_writable_at(posix::fd_t{ d }, p);
}

inline bool
is_executable_at(i32 d, const char *p)
{
  return is_executable_at(posix::fd_t{ d }, p);
}

template <is_string T>
bool
is_readable_at(posix::fd_t d, const T &s)
{
  return is_readable_at(d, s.c_str());
}

template <is_string T>
bool
is_writable_at(posix::fd_t d, const T &s)
{
  return is_writable_at(d, s.c_str());
}

template <is_string T>
bool
is_executable_at(posix::fd_t d, const T &s)
{
  return is_executable_at(d, s.c_str());
}

inline bool
has_setuid(const stat_t &b)
{
  return __impl::mode_bit(b, s_isuid);
}

inline bool
has_setgid(const stat_t &b)
{
  return __impl::mode_bit(b, s_isgid);
}

inline bool
has_sticky(const stat_t &b)
{
  return __impl::mode_bit(b, s_isvtx);
}

inline bool
has_setuid(const char *p, stat_t &buf)
{
  return __impl::mode_bit(p, s_isuid, buf);
}

inline bool
has_setgid(const char *p, stat_t &buf)
{
  return __impl::mode_bit(p, s_isgid, buf);
}

inline bool
has_sticky(const char *p, stat_t &buf)
{
  return __impl::mode_bit(p, s_isvtx, buf);
}

inline bool
has_setuid(posix::fd_t fd, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && __impl::mode_bit(buf, s_isuid);
}

inline bool
has_setgid(posix::fd_t fd, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && __impl::mode_bit(buf, s_isgid);
}

inline bool
has_sticky(posix::fd_t fd, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && __impl::mode_bit(buf, s_isvtx);
}

inline bool
has_setuid(i32 fd, stat_t &buf)
{
  return has_setuid(posix::fd_t{ fd }, buf);
}

inline bool
has_setgid(i32 fd, stat_t &buf)
{
  return has_setgid(posix::fd_t{ fd }, buf);
}

inline bool
has_sticky(i32 fd, stat_t &buf)
{
  return has_sticky(posix::fd_t{ fd }, buf);
}

inline bool
has_setuid(const char *p)
{
  return __impl::mode_bit(p, s_isuid);
}

inline bool
has_setgid(const char *p)
{
  return __impl::mode_bit(p, s_isgid);
}

inline bool
has_sticky(const char *p)
{
  return __impl::mode_bit(p, s_isvtx);
}

inline bool
has_setuid(posix::fd_t fd)
{
  return __impl::mode_bit(fd, s_isuid);
}

inline bool
has_setgid(posix::fd_t fd)
{
  return __impl::mode_bit(fd, s_isgid);
}

inline bool
has_sticky(posix::fd_t fd)
{
  return __impl::mode_bit(fd, s_isvtx);
}

inline bool
has_setuid(i32 fd)
{
  return has_setuid(posix::fd_t{ fd });
}

inline bool
has_setgid(i32 fd)
{
  return has_setgid(posix::fd_t{ fd });
}

inline bool
has_sticky(i32 fd)
{
  return has_sticky(posix::fd_t{ fd });
}

template <is_string T>
bool
has_setuid(const T &s)
{
  return has_setuid(s.c_str());
}

template <is_string T>
bool
has_setgid(const T &s)
{
  return has_setgid(s.c_str());
}

template <is_string T>
bool
has_sticky(const T &s)
{
  return has_sticky(s.c_str());
}

#define MICRON_MODE_BIT_PREDICATE(fn_, bit_)                                                                                               \
  inline bool fn_(const stat_t &b) { return __impl::mode_bit(b, bit_); }                                                                   \
  inline bool fn_(const char *p, stat_t &buf) { return __impl::mode_bit(p, bit_, buf); }                                                   \
  inline bool fn_(posix::fd_t fd, stat_t &buf) { return __impl::__fstat(fd, buf) && __impl::mode_bit(buf, bit_); }                         \
  inline bool fn_(i32 fd, stat_t &buf) { return fn_(posix::fd_t{ fd }, buf); }                                                             \
  inline bool fn_(const char *p) { return __impl::mode_bit(p, bit_); }                                                                     \
  inline bool fn_(posix::fd_t fd) { return __impl::mode_bit(fd, bit_); }                                                                   \
  inline bool fn_(i32 fd) { return fn_(posix::fd_t{ fd }); }                                                                               \
  template <is_string T> bool fn_(const T &s) { return fn_(s.c_str()); }

MICRON_MODE_BIT_PREDICATE(mode_user_read, s_irusr)
MICRON_MODE_BIT_PREDICATE(mode_user_write, s_iwusr)
MICRON_MODE_BIT_PREDICATE(mode_user_exec, s_ixusr)
MICRON_MODE_BIT_PREDICATE(mode_group_read, s_irgrp)
MICRON_MODE_BIT_PREDICATE(mode_group_write, s_iwgrp)
MICRON_MODE_BIT_PREDICATE(mode_group_exec, s_ixgrp)
MICRON_MODE_BIT_PREDICATE(mode_other_read, s_iroth)
MICRON_MODE_BIT_PREDICATE(mode_other_write, s_iwoth)
MICRON_MODE_BIT_PREDICATE(mode_other_exec, s_ixoth)

#undef MICRON_MODE_BIT_PREDICATE

inline bool
is_owned_by(const stat_t &buf, posix::uid_t uid)
{
  return buf.st_uid == uid;
}

inline bool
is_in_group(const stat_t &buf, posix::gid_t gid)
{
  return buf.st_gid == gid;
}

inline bool
is_owned_by(const char *path, posix::uid_t uid, stat_t &buf)
{
  return __impl::__stat(path, buf) && buf.st_uid == uid;
}

inline bool
is_in_group(const char *path, posix::gid_t gid, stat_t &buf)
{
  return __impl::__stat(path, buf) && buf.st_gid == gid;
}

inline bool
is_owned_by(posix::fd_t fd, posix::uid_t uid, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && buf.st_uid == uid;
}

inline bool
is_in_group(posix::fd_t fd, posix::gid_t gid, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && buf.st_gid == gid;
}

inline bool
is_owned_by(i32 fd, posix::uid_t uid, stat_t &buf)
{
  return is_owned_by(posix::fd_t{ fd }, uid, buf);
}

inline bool
is_in_group(i32 fd, posix::gid_t gid, stat_t &buf)
{
  return is_in_group(posix::fd_t{ fd }, gid, buf);
}

inline bool
is_owned_by(const char *path, posix::uid_t uid)
{
  stat_t b{};
  return is_owned_by(path, uid, b);
}

inline bool
is_in_group(const char *path, posix::gid_t gid)
{
  stat_t b{};
  return is_in_group(path, gid, b);
}

inline bool
is_owned_by(posix::fd_t fd, posix::uid_t uid)
{
  stat_t b{};
  return is_owned_by(fd, uid, b);
}

inline bool
is_in_group(posix::fd_t fd, posix::gid_t gid)
{
  stat_t b{};
  return is_in_group(fd, gid, b);
}

inline bool
is_owned_by(i32 fd, posix::uid_t uid)
{
  return is_owned_by(posix::fd_t{ fd }, uid);
}

inline bool
is_in_group(i32 fd, posix::gid_t gid)
{
  return is_in_group(posix::fd_t{ fd }, gid);
}

template <is_string T>
inline bool
is_owned_by(const T &p, posix::uid_t uid)
{
  return is_owned_by(p.c_str(), uid);
}

template <is_string T>
inline bool
is_in_group(const T &p, posix::gid_t gid)
{
  return is_in_group(p.c_str(), gid);
}

inline bool
is_empty_file(const stat_t &buf)
{
  return __impl::stat_is_reg(buf) && buf.st_size == 0;
}

inline bool
is_empty_file(posix::fd_t fd, stat_t &buf)
{
  return __impl::__fstat(fd, buf) && is_empty_file(buf);
}

inline bool
is_empty_file(posix::fd_t fd)
{
  stat_t b{};
  return is_empty_file(fd, b);
}

inline bool
is_empty_file(i32 fd, stat_t &buf)
{
  return is_empty_file(posix::fd_t{ fd }, buf);
}

inline bool
is_empty_file(i32 fd)
{
  return is_empty_file(posix::fd_t{ fd });
}

inline bool
is_empty_file(const char *path, stat_t &buf)
{
  return __impl::__stat(path, buf) && is_empty_file(buf);
}

inline bool
is_empty_file(const char *path)
{
  stat_t b{};
  return is_empty_file(path, b);
}

template <is_string T>
inline bool
is_empty_file(const T &s, stat_t &buf)
{
  return is_empty_file(s.c_str(), buf);
}

template <is_string T>
inline bool
is_empty_file(const T &s)
{
  return is_empty_file(s.c_str());
}

inline bool
is_empty_dir(posix::fd_t fd)
{
  posix::fd_t tmp{ static_cast<i32>(micron::syscall(SYS_dup, fd.fd)) };
  if ( !tmp )
    return false;
  micron::syscall(SYS_lseek, tmp.fd, 0, posix::seek_set);
  char buf[512];
  bool empty = true;
  for ( ;; ) {
    max_t n = micron::syscall(SYS_getdents64, tmp.fd, buf, sizeof(buf));
    if ( n <= 0 )
      break;
    usize pos = 0;
    while ( pos < static_cast<usize>(n) ) {
      auto *d = reinterpret_cast<posix::__linux_kernel_dirent64 *>(buf + pos);
      pos += d->d_reclen;
      if ( !is_dot_entry(d->d_name) ) {
        empty = false;
        goto done;
      }
    }
  }
done:
  micron::syscall(SYS_close, tmp.fd);
  return empty;
}

inline bool
is_empty_dir(i32 fd)
{
  return is_empty_dir(posix::fd_t{ fd });
}

inline bool
is_empty_dir(const char *path)
{
  posix::fd_t fd{ static_cast<i32>(
      micron::syscall(SYS_openat, posix::at_fdcwd, path, posix::o_rdonly | posix::o_directory | posix::o_cloexec, 0)) };
  if ( !fd )
    return false;
  bool r = is_empty_dir(fd);
  micron::syscall(SYS_close, fd.fd);
  return r;
}

template <is_string T>
inline bool
is_empty_dir(const T &s)
{
  return is_empty_dir(s.c_str());
}

inline bool
is_empty_node(const stat_t &buf)
{
  if ( __impl::stat_is_reg(buf) )
    return buf.st_size == 0;
  return false;
}

inline bool
is_empty_node(posix::fd_t fd, stat_t &buf)
{
  if ( !__impl::__fstat(fd, buf) )
    return false;
  if ( __impl::stat_is_reg(buf) )
    return buf.st_size == 0;
  if ( __impl::stat_is_dir(buf) )
    return is_empty_dir(fd);
  return false;
}

inline bool
is_empty_node(posix::fd_t fd)
{
  stat_t b{};
  return is_empty_node(fd, b);
}

inline bool
is_empty_node(i32 fd, stat_t &buf)
{
  return is_empty_node(posix::fd_t{ fd }, buf);
}

inline bool
is_empty_node(i32 fd)
{
  return is_empty_node(posix::fd_t{ fd });
}

inline bool
is_empty_node(const char *path, stat_t &buf)
{
  if ( !__impl::__stat(path, buf) )
    return false;
  if ( __impl::stat_is_reg(buf) )
    return buf.st_size == 0;
  if ( __impl::stat_is_dir(buf) )
    return is_empty_dir(path);
  return false;
}

inline bool
is_empty_node(const char *path)
{
  stat_t b{};
  return is_empty_node(path, b);
}

template <is_string T>
inline bool
is_empty_node(const T &s, stat_t &buf)
{
  return is_empty_node(s.c_str(), buf);
}

template <is_string T>
inline bool
is_empty_node(const T &s)
{
  return is_empty_node(s.c_str());
}

inline bool
has_content(const stat_t &b)
{
  return !is_empty_node(b);
}

inline bool
has_content(posix::fd_t fd)
{
  return !is_empty_node(fd);
}

inline bool
has_content(i32 fd)
{
  return !is_empty_node(posix::fd_t{ fd });
}

inline bool
has_content(const char *p)
{
  return !is_empty_node(p);
}

template <is_string T>
bool
has_content(const T &s)
{
  return !is_empty_node(s);
}

inline bool
is_mountpoint(const char *path)
{
  if ( path == nullptr || path[0] == '\0' )
    return false;
  if ( path[0] == '/' && path[1] == '\0' )
    return true;

  stat_t self{}, parent{};
  if ( !__impl::__stat(path, self) )
    return false;

  char par[posix::path_max];
  usize i = 0;
  while ( path[i] && i < posix::path_max - 1 ) {
    par[i] = path[i];
    ++i;
  }
  par[i] = '\0';

  while ( i > 1 && par[i - 1] == '/' )
    --i;
  while ( i > 1 && par[i - 1] != '/' )
    --i;
  if ( i > 1 && par[i - 1] == '/' )
    --i;
  if ( i == 0 ) {
    par[0] = '/';
    i = 1;
  }
  par[i] = '\0';

  if ( !__impl::__stat(par, parent) )
    return false;
  return self.st_dev != parent.st_dev;
}

template <is_string T>
inline bool
is_mountpoint(const T &s)
{
  return is_mountpoint(s.c_str());
}

inline bool
is_same_file(const stat_t &a, const stat_t &b)
{
  return a.st_dev == b.st_dev && a.st_ino == b.st_ino;
}

inline bool
is_same_file(const char *a, const char *b)
{
  stat_t sa{}, sb{};
  return __impl::__stat(a, sa) && __impl::__stat(b, sb) && is_same_file(sa, sb);
}

inline bool
is_same_file(posix::fd_t a, posix::fd_t b)
{
  stat_t sa{}, sb{};
  return __impl::__fstat(a, sa) && __impl::__fstat(b, sb) && is_same_file(sa, sb);
}

inline bool
is_same_file(i32 a, i32 b)
{
  return is_same_file(posix::fd_t{ a }, posix::fd_t{ b });
}

inline bool
is_same_file(posix::fd_t a, const char *b)
{
  stat_t sa{}, sb{};
  return __impl::__fstat(a, sa) && __impl::__stat(b, sb) && is_same_file(sa, sb);
}

inline bool
is_same_file(const char *a, posix::fd_t b)
{
  return is_same_file(b, a);
}

template <is_string A, is_string B>
inline bool
is_same_file(const A &a, const B &b)
{
  return is_same_file(a.c_str(), b.c_str());
}

template <is_string T>
inline bool
is_same_file(const T &a, const char *b)
{
  return is_same_file(a.c_str(), b);
}

template <is_string T>
inline bool
is_same_file(const char *a, const T &b)
{
  return is_same_file(a, b.c_str());
}

#define MICRON_STAT_QUERY(fn_, ret_t_, field_, sentinel_)                                                                                  \
  inline ret_t_ fn_(const stat_t &b) { return b.field_; }                                                                                  \
  inline ret_t_ fn_(posix::fd_t fd, stat_t &buf) { return __impl::__fstat(fd, buf) ? buf.field_ : sentinel_; }                             \
  inline ret_t_ fn_(i32 fd, stat_t &buf) { return fn_(posix::fd_t{ fd }, buf); }                                                           \
  inline ret_t_ fn_(posix::fd_t fd)                                                                                                        \
  {                                                                                                                                        \
    stat_t b{};                                                                                                                            \
    return fn_(fd, b);                                                                                                                     \
  }                                                                                                                                        \
  inline ret_t_ fn_(i32 fd) { return fn_(posix::fd_t{ fd }); }                                                                             \
  inline ret_t_ fn_(const char *p, stat_t &buf) { return __impl::__stat(p, buf) ? buf.field_ : sentinel_; }                                \
  inline ret_t_ fn_(const char *p)                                                                                                         \
  {                                                                                                                                        \
    stat_t b{};                                                                                                                            \
    return fn_(p, b);                                                                                                                      \
  }                                                                                                                                        \
  template <is_string T> ret_t_ fn_(const T &s, stat_t &buf) { return fn_(s.c_str(), buf); }                                               \
  template <is_string T> ret_t_ fn_(const T &s) { return fn_(s.c_str()); }

MICRON_STAT_QUERY(get_inode, posix::ino_t, st_ino, 0)
MICRON_STAT_QUERY(get_mode, posix::mode_t, st_mode, 0)
MICRON_STAT_QUERY(get_link_count, posix::nlink_t, st_nlink, 0)
MICRON_STAT_QUERY(get_uid, posix::uid_t, st_uid, static_cast<posix::uid_t>(-1))
MICRON_STAT_QUERY(get_gid, posix::gid_t, st_gid, static_cast<posix::gid_t>(-1))
MICRON_STAT_QUERY(get_device, posix::dev_t, st_dev, static_cast<posix::dev_t>(-1))
MICRON_STAT_QUERY(get_rdev, posix::dev_t, st_rdev, static_cast<posix::dev_t>(-1))
MICRON_STAT_QUERY(get_blksize, posix::blksize_t, st_blksize, static_cast<posix::blksize_t>(-1))
MICRON_STAT_QUERY(get_blocks, posix::blkcnt_t, st_blocks, static_cast<posix::blkcnt_t>(-1))
MICRON_STAT_QUERY(get_atime, posix::time_t, st_atime, static_cast<posix::time_t>(-1))
MICRON_STAT_QUERY(get_mtime, posix::time_t, st_mtime, static_cast<posix::time_t>(-1))
MICRON_STAT_QUERY(get_ctime, posix::time_t, st_ctime, static_cast<posix::time_t>(-1))

#undef MICRON_STAT_QUERY

inline posix::off_t
file_size(posix::fd_t fd)
{
  stat_t buf{};
  return __impl::__fstat(fd, buf) ? buf.st_size : -1;
}

inline posix::off_t
get_size(const stat_t &b)
{
  return b.st_size;
}

inline posix::off_t
get_size(posix::fd_t fd, stat_t &buf)
{
  return __impl::__fstat(fd, buf) ? buf.st_size : -1;
}

inline posix::off_t
get_size(i32 fd, stat_t &buf)
{
  return get_size(posix::fd_t{ fd }, buf);
}

inline posix::off_t
get_size(posix::fd_t fd)
{
  return posix::file_size(fd);
}

inline posix::off_t
get_size(i32 fd)
{
  return get_size(posix::fd_t{ fd });
}

inline posix::off_t
get_size(const char *p, stat_t &buf)
{
  return __impl::__stat(p, buf) ? buf.st_size : -1;
}

inline posix::off_t
get_size(const char *p)
{
  stat_t b{};
  return get_size(p, b);
}

template <is_string T>
posix::off_t
get_size(const T &s, stat_t &buf)
{
  return get_size(s.c_str(), buf);
}

template <is_string T>
posix::off_t
get_size(const T &s)
{
  return get_size(s.c_str());
}

inline posix::mode_t
get_permissions(const stat_t &b)
{
  return b.st_mode & ~static_cast<posix::mode_t>(__format_mask);
}

inline posix::mode_t
get_permissions(posix::fd_t fd, stat_t &buf)
{
  return get_mode(fd, buf) & ~static_cast<posix::mode_t>(__format_mask);
}

inline posix::mode_t
get_permissions(i32 fd, stat_t &buf)
{
  return get_permissions(posix::fd_t{ fd }, buf);
}

inline posix::mode_t
get_permissions(posix::fd_t fd)
{
  return get_mode(fd) & ~static_cast<posix::mode_t>(__format_mask);
}

inline posix::mode_t
get_permissions(i32 fd)
{
  return get_permissions(posix::fd_t{ fd });
}

inline posix::mode_t
get_permissions(const char *p, stat_t &buf)
{
  return get_mode(p, buf) & ~static_cast<posix::mode_t>(__format_mask);
}

inline posix::mode_t
get_permissions(const char *p)
{
  return get_mode(p) & ~static_cast<posix::mode_t>(__format_mask);
}

template <is_string T>
posix::mode_t
get_permissions(const T &s, stat_t &buf)
{
  return get_permissions(s.c_str(), buf);
}

template <is_string T>
posix::mode_t
get_permissions(const T &s)
{
  return get_permissions(s.c_str());
}

};     // namespace posix
};     // namespace micron
