//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"

#ifndef stdin_fileno
#define stdin_fileno 0
#endif

#ifndef stdout_fileno
#define stdout_fileno 1
#endif

#ifndef stderr_fileno
#define stderr_fileno 2
#endif

#define MAX_NAME_LENGTH 256     // generally true for nearly all FSs

namespace micron
{
namespace io
{

enum class node_types : int {
  socket = S_IFSOCK,
  symlink = S_IFLNK,
  regular_file = S_IFREG,
  block_device = S_IFBLK,
  directory = S_IFDIR,
  character_device = S_IFCHR,
  fifo = S_IFIFO,
  not_found,
  unknown
};

bool
verify(const char *str)
{
  // verify to make sure the str isn't malformed or bogus
  if ( str == nullptr )
    return false;
  if ( str[0] == '\0' )
    return false;
  return true;
}
template <is_string T>
bool
verify(T str)
{
  // verify to make sure the str isn't malformed or bogus
  if ( str.cdata() == nullptr )
    return false;     // something went gigawrong
  if ( str.size() == 0 )
    return false;
  if ( str[0] == '\0' )
    return false;
  return true;
}
bool
exists(const char *str)
{
  stat_t buf;
  return (posix::stat(str, buf) == 0);
}

template <node_types Tp, is_string T>
inline bool
is_inode_type(const T &str)
{
  stat_t buf;
  if ( stat(str.c_str(), buf) != 0 )
    return false;     // doesn't exist, can't be
  if ( (buf.st_mode & S_IFMT) == (int)Tp )
    return true;
  return false;
}

template <node_types Tp>
inline bool
is_inode_type(const char *str)
{
  stat_t buf;
  if ( stat(str, buf) != 0 )
    return false;     // doesn't exist, can't be
  if ( (buf.st_mode & S_IFMT) == (int)Tp )
    return true;
  return false;
}
//

template <node_types Tp>
inline bool
is_inode_type(const int fd)
{
  stat_t buf;
  if ( posix::fstat(fd, buf) != 0 )
    return false;     // doesn't exist, can't be
  if ( (buf.st_mode & S_IFMT) == (int)Tp )
    return true;
  return false;
}
//
template <node_types Tp>
inline bool
is_inode_type_at(int fd, const char *str)
{
  stat_t buf;
  if ( posix::fstatat(fd, str, buf, 0) != 0 )
    return false;     // doesn't exist, can't be
  if ( (buf.st_mode & S_IFMT) == (int)Tp )
    return true;
  return false;
}

template <node_types Tp, is_string T>
inline bool
is_inode_type_at(int fd, const T &str)
{
  stat_t buf;
  if ( posix::fstatat(fd, str.c_str(), buf, 0) != 0 )
    return false;     // doesn't exist, can't be
  if ( (buf.st_mode & S_IFMT) == (int)Tp )
    return true;
  return false;
}
// from fd


auto
get_type_at(const char* str)
{
  stat_t buf;
  if ( stat(str, buf) != 0 )
    return node_types::not_found;     // doesn't exist, can't be
  return static_cast<node_types>(buf.st_mode & S_IFMT);
}

auto
get_type(const int fd)
{
  stat_t buf;
  if ( posix::fstat(fd, buf) != 0 )
    return node_types::not_found;     // doesn't exist, can't be
  return static_cast<node_types>(buf.st_mode & S_IFMT);
}

bool
is_virtual_file(const int fd)
{
  stat_t buf;
  if ( posix::fstat(fd, buf) != 0 )
    return false;     // doesn't exist, can't be
  if ( (buf.st_mode & S_IFMT) == (int)node_types::regular_file )
    if ( micron::major(buf.st_dev) == 0 )
      return true;
  return false;
}
bool
is_file(const int fd)
{
  return is_inode_type<node_types::regular_file>(fd);
}

bool
is_dir(const int fd)
{
  return is_inode_type<node_types::directory>(fd);
}

bool
is_socket(const int fd)
{
  return is_inode_type<node_types::socket>(fd);
}

bool
is_symlink(const int fd)
{
  return is_inode_type<node_types::symlink>(fd);
}

bool
is_block_device(const int fd)
{
  return is_inode_type<node_types::block_device>(fd);
}

bool
is_fifo(const int fd)
{
  return is_inode_type<node_types::fifo>(fd);
}

// from name

bool
is_file_at(int fd, const char *str)
{
  return is_inode_type_at<node_types::regular_file>(fd, str);
}
bool
is_file(const char *str)
{
  return is_inode_type<node_types::regular_file>(str);
}
bool
is_dir_at(int fd, const char *str)
{
  return is_inode_type_at<node_types::directory>(fd, str);
}
bool
is_dir(const char *str)
{
  return is_inode_type<node_types::directory>(str);
}
bool
is_socket_at(int fd, const char *str)
{
  return is_inode_type_at<node_types::socket>(fd, str);
}
bool
is_socket(const char *str)
{
  return is_inode_type<node_types::socket>(str);
}

bool
is_symlink_at(int fd, const char *str)
{
  return is_inode_type_at<node_types::symlink>(fd, str);
}
bool
is_symlink(const char *str)
{
  return is_inode_type<node_types::symlink>(str);
}

bool
is_block_device_at(int fd, const char *str)
{
  return is_inode_type_at<node_types::block_device>(fd, str);
}
bool
is_block_device(const char *str)
{
  return is_inode_type<node_types::block_device>(str);
}

bool
is_fifo_at(int fd, const char *str)
{
  return is_inode_type_at<node_types::fifo>(fd, str);
}
bool
is_fifo(const char *str)
{
  return is_inode_type<node_types::fifo>(str);
}

// for strings

template <is_string T>
bool
is_file_at(int fd, const T &str)
{
  return is_inode_type_at<node_types::regular_file>(fd, str);
}

template <is_string T>
bool
is_file(const T &str)
{
  return is_inode_type<node_types::regular_file>(str);
}

template <is_string T>
bool
is_dir_at(int fd, const T &str)
{
  return is_inode_type_at<node_types::directory>(fd, str);
}

template <is_string T>
bool
is_dir(const T &str)
{
  return is_inode_type<node_types::directory>(str);
}

template <is_string T>
bool
is_socket_at(int fd, const T &str)
{
  return is_inode_type_at<node_types::socket>(fd, str);
}

template <is_string T>
bool
is_socket(const T &str)
{
  return is_inode_type<node_types::socket>(str);
}

template <is_string T>
bool
is_symlink_at(int fd, const T &str)
{
  return is_inode_type_at<node_types::symlink>(fd, str);
}

template <is_string T>
bool
is_symlink(const T &str)
{
  return is_inode_type<node_types::symlink>(str);
}

template <is_string T>
bool
is_block_device_at(int fd, const T &str)
{
  return is_inode_type_at<node_types::block_device>(fd, str);
}

template <is_string T>
bool
is_block_device(const T &str)
{
  return is_inode_type<node_types::block_device>(str);
}

template <is_string T>
bool
is_fifo_at(int fd, const T &str)
{
  return is_inode_type_at<node_types::fifo>(fd, str);
}

template <is_string T>
bool
is_fifo(const T &str)
{
  return is_inode_type<node_types::fifo>(str);
}

};
};
