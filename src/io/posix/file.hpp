//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "iosys.hpp"

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../string/strings.hpp"
#include "../../tuple.hpp"

#include "../../linux/sys/fcntl.hpp"
#include "../bits.hpp"

#include "utils.hpp"

// functions for handling posix/linux files
namespace micron
{
namespace io
{
enum stat_existing { STAT_OVERRIDE, STAT_EXISTING };

struct ipermissions {
  bool read;
  bool write;
  bool execute;
};

struct linux_permissions {
  ipermissions owner;
  ipermissions group;
  ipermissions others;
};     // less space efficient, but it really makes no sense to implement the archaic octal notation

enum class permission_types : int {
  owner_read,
  owner_write,
  owner_execute,
  group_read,
  group_write,
  group_execute,
  others_read,
  others_write,
  others_execute
};

enum class modes {
  quiet,
  large,
  largeread,
  create,
  read,
  readwrite,     // r+
  write,
  readwritecreate,     // w+
  append,
  appendread
};

// we don't have shadowing errors
constexpr modes qt = modes::quiet;
constexpr modes lrg = modes::large;
constexpr modes lrgr = modes::largeread;
constexpr modes cr = modes::create;
constexpr modes rd = modes::read;
constexpr modes rw = modes::readwrite;
constexpr modes rwc = modes::readwritecreate;
constexpr modes ap = modes::append;
constexpr modes ar = modes::appendread;

struct openmode {
  constexpr static const char *read = "r";
  constexpr static const char *readwrite = "r+";
  constexpr static const char *write = "w";
  constexpr static const char *readwritecreate = "w+";
  constexpr static const char *append = "a";
  constexpr static const char *appendread = "a+";

  int md;
};

// implementing all linux syscalls
struct file {
  micron::sstr<MAX_NAME_LENGTH> fname;
  // int fd;
  fd_t __handle;
  stat_t sd;

  // regardless
  ~file() { close(); }

  file(void) : fname(), __handle(-1), sd() {};

  file(const char *str) { _open_linux(str, modes::read); }

  file(const micron::sstr<MAX_NAME_LENGTH> &str) { _open_linux(str, modes::read); }

  file(const micron::string &str) { _open_linux(str, modes::read); }

  file(const char *str, const modes mode) { _open_linux(str, mode); }

  file(const micron::sstr<MAX_NAME_LENGTH> &str, const modes mode) { _open_linux(str, mode); }

  // file(const micron::string &str, const modes mode) { _open_linux(str, mode); }

  file(const file &o) : fname(o.fname), __handle(o.__handle), sd(o.sd) {}

  file(file &&o) : fname(micron::move(o.fname)), __handle(o.__handle), sd(o.sd)
  {
    o.__handle = -1;
    micron::zero(&sd);
  }

  file &
  operator=(file &&o)
  {
    fname = micron::move(o.fname);
    __handle = o.__handle;
    sd = o.sd;
    o.__handle = -1;
    micron::zero(&sd);
    return *this;
  }

  file &
  operator=(const file &o)
  {
    fname = o.fname;
    __handle = o.__handle;
    sd = o.sd;
    return *this;
  }

  void
  close()
  {
    if ( __handle.closed() )
      return;
    posix::close(__handle.fd);
    __handle.fd = -1;
  }

  // here come the functions
  template <bool B = STAT_EXISTING>
  inline void
  stat(void)
  {
    if constexpr ( B == STAT_EXISTING ) {
      if ( !is_zero(&sd) )
        return;
      alive();
      if ( posix::fstat(__handle.fd, sd) == -1 )     // fstat((fd), &sd) == -1 )
        exc<except::io_error>("micron::file, fstat failed.");
    } else if constexpr ( B == STAT_OVERRIDE ) {
      alive();
      if ( posix::fstat(__handle.fd, sd) == -1 )     // fstat((fd), &sd) == -1 )
        exc<except::io_error>("micron::file, fstat failed.");
    }
  }

  inline auto
  size(void)
  {
    alive();
    stat();
    return sd.st_size;
  }

  inline auto
  is_system_virtual(void)
  {
    alive();
    stat();
    return (micron::major(sd.st_dev) == 0);
  }

  inline auto
  owner(void)
  {
    alive();
    stat();
    return micron::tie({ sd.st_uid, sd.st_gid });
  }

  inline auto
  set_owner(void)
  {
  }

  inline auto
  access(void)
  {
    alive();
    stat();
    return micron::tie({ sd.st_atime, sd.st_mtime });
  }

  inline auto
  modified(void)
  {
    alive();
    stat();
    return sd.st_mtime;
  }

  inline auto
  permissions(void)
  {
    alive();
    stat();
    struct linux_permissions perms
        = { { false, false, false }, { false, false, false }, { false, false, false } };     // explicitly init to zero
    if ( sd.st_mode & S_IRUSR )
      perms.owner.read = true;
    if ( sd.st_mode & S_IWUSR )
      perms.owner.write = true;
    if ( sd.st_mode & S_IXUSR )
      perms.owner.execute = true;
    if ( sd.st_mode & S_IRGRP )
      perms.group.read = true;
    if ( sd.st_mode & S_IWGRP )
      perms.group.write = true;
    if ( sd.st_mode & S_IXGRP )
      perms.group.execute = true;
    if ( sd.st_mode & S_IROTH )
      perms.others.read = true;
    if ( sd.st_mode & S_IWOTH )
      perms.others.write = true;
    if ( sd.st_mode & S_IXOTH )
      perms.others.execute = true;
    return perms;
  }

  inline void
  set_permissions(const linux_permissions &perms)
  {
    alive();
    unsigned int mode = 0;
    if ( perms.owner.read )
      mode |= S_IRUSR;
    if ( perms.owner.write )
      mode |= S_IWUSR;
    if ( perms.owner.execute )
      mode |= S_IXUSR;
    if ( perms.group.read )
      mode |= S_IRGRP;
    if ( perms.group.write )
      mode |= S_IWGRP;
    if ( perms.group.execute )
      mode |= S_IXGRP;
    if ( perms.others.read )
      mode |= S_IROTH;
    if ( perms.others.write )
      mode |= S_IWOTH;
    if ( perms.others.execute )
      mode |= S_IXOTH;

    posix::fchmod(__handle.fd, mode);
  }

  inline const auto &
  name() const
  {
    return fname;
  }

private:
  inline bool
  alive(void) const
  {
    if ( __handle.fd == -1 ) {
      exc<except::io_error>("micron::file, fd isn't open.");
      return false;
    }
    return true;
  }

  inline __attribute((always_inline)) long int
  _syscall_open(const char *str, const modes mode)
  {
    switch ( mode ) {
    case modes::largeread :
      return posix::open(str, o_rdonly | o_sync | o_direct | o_largefile);
      break;
    case modes::large :
      return posix::open(str, o_rdwr | o_create | o_sync | o_direct | o_largefile, 0644);
      break;
    case modes::quiet :
      return posix::open(str, o_rdonly | o_noatime, 0644);
      break;
    case modes::create :
      return posix::open(str, o_rdonly | o_create | o_excl, 0644);
      break;
    case modes::read :
      return posix::open(str, o_rdonly);
      break;
    case modes::readwrite :
      return posix::open(str, o_rdwr | o_sync);
      break;
    case modes::write :
      return posix::open(str, o_wronly | o_sync);
      break;
    case modes::readwritecreate :
      return posix::open(str, o_rdwr | o_create | o_sync, 0644);
      break;
    case modes::append :
      return posix::open(str, o_wronly | o_create | o_sync, 0644);
      break;
    case modes::appendread :
      return posix::open(str, o_rdwr | o_create | o_sync, 0644);
      break;
    }
    return -1;
  }

  template <typename T>
  inline __attribute__((always_inline)) void
  _open_linux(const T &str, const modes mode)
  {
    if constexpr ( micron::is_string_ascii<T> ) {
      if ( !verify(str) )
        exc<except::io_error>("error in creating micron::file, malformed string.");
      if ( mode != modes::append and mode != modes::create and mode != modes::readwritecreate ) {
        if ( !exists(str.c_str()) )
          exc<except::io_error>("micron::file file doesn't exist");
        if ( !is_file(str.c_str()) )
          exc<except::io_error>("micron::file file isn't a file (check type)");
      }
      __handle.fd = static_cast<int>(_syscall_open(str.c_str(), mode));
      // fd = open(str.c_str(), );
      if ( __handle.has_error() )
        exc<except::io_error>("micron::file failed to open");
      fname = str;
      micron::zero(&sd);
    } else {
      if ( !verify(str) )
        exc<except::io_error>("error in creating micron::file, malformed string.");
      if ( mode != modes::append and mode != modes::create and mode != modes::readwritecreate ) {
        if ( !exists(str) )
          exc<except::io_error>("micron::file file doesn't exist");
        if ( !is_file(str) )
          exc<except::io_error>("micron::file file isn't a file (check type)");
      }
      // fd = open(str, "r");
      __handle.fd = static_cast<int>(_syscall_open(str, mode));
      if ( __handle.has_error() )
        exc<except::io_error>("micron::file failed to open");
      fname = str;
      micron::zero(&sd);
    }
  }
};
};     // namespace io

};     // namespace micron
