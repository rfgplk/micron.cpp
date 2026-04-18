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

#include "../../linux/io/inode.hpp"
#include "../../linux/sys/fcntl.hpp"
#include "../../linux/sys/ioctl.hpp"
#include "../../linux/sys/limits.hpp"
#include "../bits.hpp"

#include "../../linux/io.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// posix files
// functions for handling posix/linux files
// part of the platform layer
// code in here is allowed to directly hook into syscalls and the like
namespace micron
{
namespace io
{
constexpr static const usize max_name = (posix::name_max + 1) * 2;

enum stat_existing { STAT_OVERRIDE, STAT_EXISTING };

struct ipermissions {
  bool read;
  bool write;
  bool execute;

  static constexpr ipermissions
  none() noexcept
  {
    return { false, false, false };
  }

  static constexpr ipermissions
  all() noexcept
  {
    return { true, true, true };
  }

  static constexpr ipermissions
  ro() noexcept
  {
    return { true, false, false };
  }

  static constexpr ipermissions
  rw() noexcept
  {
    return { true, true, false };
  }

  static constexpr ipermissions
  rx() noexcept
  {
    return { true, false, true };
  }

  static constexpr ipermissions
  wo() noexcept
  {
    return { false, true, false };
  }

  static constexpr ipermissions
  wx() noexcept
  {
    return { false, true, true };
  }

  static constexpr ipermissions
  xo() noexcept
  {
    return { false, false, true };
  }

  constexpr bool
  any(void) const noexcept
  {
    return read || write || execute;
  }

  constexpr bool
  full(void) const noexcept
  {
    return read && write && execute;
  }

  constexpr ipermissions
  with_read(void) const noexcept
  {
    return { true, write, execute };
  }

  constexpr ipermissions
  with_write(void) const noexcept
  {
    return { read, true, execute };
  }

  constexpr ipermissions
  with_execute(void) const noexcept
  {
    return { read, write, true };
  }

  constexpr ipermissions
  without_read(void) const noexcept
  {
    return { false, write, execute };
  }

  constexpr ipermissions
  without_write(void) const noexcept
  {
    return { read, false, execute };
  }

  constexpr ipermissions
  without_execute(void) const noexcept
  {
    return { read, write, false };
  }

  constexpr bool
  operator==(const ipermissions &o) const noexcept
  {
    return read == o.read && write == o.write && execute == o.execute;
  }

  constexpr bool
  operator!=(const ipermissions &o) const noexcept
  {
    return !(*this == o);
  }

  constexpr unsigned
  to_octal(void) const noexcept
  {
    return (read ? 4u : 0u) | (write ? 2u : 0u) | (execute ? 1u : 0u);
  }

  static constexpr ipermissions
  from_octal(unsigned v) noexcept
  {
    return { (v & 4u) != 0, (v & 2u) != 0, (v & 1u) != 0 };
  }
};

struct linux_permissions {
  ipermissions owner;
  ipermissions group;
  ipermissions others;

  static constexpr linux_permissions
  none() noexcept
  {
    return { ipermissions::none(), ipermissions::none(), ipermissions::none() };
  }

  static constexpr linux_permissions
  all() noexcept
  {
    return { ipermissions::all(), ipermissions::all(), ipermissions::all() };
  }

  static constexpr linux_permissions
  file_default() noexcept     // 0644
  {
    return { ipermissions::rw(), ipermissions::ro(), ipermissions::ro() };
  }

  static constexpr linux_permissions
  exec_default() noexcept     // 0755
  {
    return { ipermissions::all(), ipermissions::rx(), ipermissions::rx() };
  }

  static constexpr linux_permissions
  dir_default() noexcept     // 0755
  {
    return exec_default();
  }

  static constexpr linux_permissions
  private_file() noexcept     // 0600
  {
    return { ipermissions::rw(), ipermissions::none(), ipermissions::none() };
  }

  static constexpr linux_permissions
  private_exec() noexcept     // 0700
  {
    return { ipermissions::all(), ipermissions::none(), ipermissions::none() };
  }

  static constexpr linux_permissions
  world_readable() noexcept     // 0644
  {
    return file_default();
  }

  static constexpr linux_permissions
  world_readable_exec() noexcept     // 0755
  {
    return exec_default();
  }

  static constexpr linux_permissions
  from_mode(unsigned mode) noexcept
  {
    return { ipermissions::from_octal((mode >> 6) & 7u), ipermissions::from_octal((mode >> 3) & 7u), ipermissions::from_octal(mode & 7u) };
  }

  constexpr unsigned
  to_mode(void) const noexcept
  {
    return (owner.to_octal() << 6) | (group.to_octal() << 3) | others.to_octal();
  }

  constexpr posix::mode_t
  to_posix_mode(void) const noexcept
  {
    return (to_mode());
  }

  constexpr explicit
  operator posix::mode_t(void) const noexcept
  {
    return to_posix_mode();
  }

  bool setuid = false;
  bool setgid = false;
  bool sticky = false;

  constexpr unsigned
  full_mode(void) const noexcept
  {
    return to_mode() | (setuid ? static_cast<unsigned>(posix::s_isuid) : 0u) | (setgid ? static_cast<unsigned>(posix::s_isgid) : 0u)
           | (sticky ? static_cast<unsigned>(posix::s_isvtx) : 0u);
  }

  constexpr linux_permissions
  with_owner(ipermissions p) const noexcept
  {
    return { p, group, others, setuid, setgid, sticky };
  }

  constexpr linux_permissions
  with_group(ipermissions p) const noexcept
  {
    return { owner, p, others, setuid, setgid, sticky };
  }

  constexpr linux_permissions
  with_others(ipermissions p) const noexcept
  {
    return { owner, group, p, setuid, setgid, sticky };
  }

  constexpr linux_permissions
  with_setuid(bool v = true) const noexcept
  {
    return { owner, group, others, v, setgid, sticky };
  }

  constexpr linux_permissions
  with_setgid(bool v = true) const noexcept
  {
    return { owner, group, others, setuid, v, sticky };
  }

  constexpr linux_permissions
  with_sticky(bool v = true) const noexcept
  {
    return { owner, group, others, setuid, setgid, v };
  }

  constexpr bool
  operator==(const linux_permissions &o) const noexcept
  {
    return owner == o.owner && group == o.group && others == o.others && setuid == o.setuid && setgid == o.setgid && sticky == o.sticky;
  }

  constexpr bool
  operator!=(const linux_permissions &o) const noexcept
  {
    return !(*this == o);
  }
};

inline constexpr linux_permissions perm_none = linux_permissions::none();
inline constexpr linux_permissions perm_all = linux_permissions::all();
inline constexpr linux_permissions perm_file_default = linux_permissions::file_default();
inline constexpr linux_permissions perm_exec_default = linux_permissions::exec_default();
inline constexpr linux_permissions perm_dir_default = linux_permissions::dir_default();
inline constexpr linux_permissions perm_private_file = linux_permissions::private_file();
inline constexpr linux_permissions perm_private_exec = linux_permissions::private_exec();

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

enum class modes { quiet, large, largeread, create, read, readwrite, write, readwritecreate, append, appendread };

// aliases
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

class file
{
protected:
  const stat_t &
  __sd(void) const noexcept
  {
    return sd;
  }

  inline bool
  __alive(void) const
  {
    if ( __handle.fd == posix::invalid_fd ) [[unlikely]] {
      exc<except::io_error>("micron::file, fd isn't open.");
      return false;
    } else if ( __handle.has_error() ) [[unlikely]] {
      errno = __handle.fd;
      exc<except::io_error>("micron::file, fd has an error.");
      return false;
    }
    return true;
  }

  inline __attribute__((always_inline)) long int
  __syscall_open(const char *str, const modes mode)
  {
    switch ( mode ) {
    case modes::largeread :
      return posix::open(str, posix::o_rdonly | posix::o_sync | posix::o_direct | posix::o_largefile);
    case modes::large :
      return posix::open(str, posix::o_rdwr | posix::o_create | posix::o_sync | posix::o_direct | posix::o_largefile, 0644u);
    case modes::quiet :
      return posix::open(str, posix::o_rdonly | posix::o_noatime, 0644u);
    case modes::create :
      return posix::open(str, posix::o_rdonly | posix::o_create | posix::o_excl, 0644u);
    case modes::read :
      return posix::open(str, posix::o_rdonly);
    case modes::readwrite :
      return posix::open(str, posix::o_rdwr | posix::o_sync);
    case modes::write :
      return posix::open(str, posix::o_wronly | posix::o_sync);
    case modes::readwritecreate :
      return posix::open(str, posix::o_rdwr | posix::o_create | posix::o_sync, 0644u);
    case modes::append :
      return posix::open(str, posix::o_wronly | posix::o_create | posix::o_append | posix::o_sync, 0644u);
    case modes::appendread :
      return posix::open(str, posix::o_rdwr | posix::o_create | posix::o_append | posix::o_sync, 0644u);
    }
    return -1;
  }

  template <typename T>
  inline __attribute__((always_inline)) void
  __open_linux(const T &str, const modes mode)
  {
    if ( !posix::verify(str) ) exc<except::io_error>("error in creating micron::file, malformed string.");

    if ( mode != modes::append && mode != modes::create && mode != modes::readwritecreate ) {
      if ( !posix::exists(str) ) exc<except::io_error>("micron::file file doesn't exist");
      // NOTE: allow opening character devices as a file
      if ( !posix::is_file(str) and !posix::is_char_device(str) ) exc<except::io_error>("micron::file file isn't a file (check type)");
    }

    if constexpr ( is_string<T> ) {
      __handle.fd = static_cast<int>(__syscall_open(str.c_str(), mode));
    } else {
      __handle.fd = static_cast<int>(__syscall_open(str, mode));
    }

    if ( __handle.has_error() ) exc<except::io_error>("micron::file failed to open");

    fname = str;
    micron::zero(&sd);
  }

public:
  micron::sstr<max_name> fname;
  fd_t __handle;
  // NOTE: complete and total hack, we technically need to mutate this within methods that should otherwise be const
  mutable stat_t sd;

  ~file() { close(); }

  file(void) : fname(), __handle(-1), sd() {}

  file(const char *str) { __open_linux(str, modes::read); }

  file(const micron::sstr<max_name> &str) { __open_linux(str, modes::read); }

  file(const micron::string &str) { __open_linux(str, modes::read); }

  file(const char *str, const modes mode) { __open_linux(str, mode); }

  file(const micron::sstr<max_name> &str, const modes mode) { __open_linux(str, mode); }

  template <is_string T> file(const T &str) { __open_linux(str, modes::read); }

  template <is_string T> file(const T &str, const modes mode) { __open_linux(str, mode); }

  file(const file &o) : fname(o.fname), __handle(o.__handle), sd(o.sd) {}

  file(file &&o) noexcept : fname(micron::move(o.fname)), __handle(o.__handle), sd(o.sd)
  {
    o.__handle = posix::invalid_fd;
    micron::zero(&o.sd);
  }

  file &
  operator=(file &&o) noexcept
  {
    fname = micron::move(o.fname);
    __handle = o.__handle;
    sd = o.sd;
    o.__handle = posix::invalid_fd;
    micron::zero(&o.sd);
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

  // reopening
  file &
  operator=(const char *name)
  {
    if ( !name ) return *this;
    close();
    fname = name;
    reopen();
    return *this;
  }

  file &
  operator=(const micron::sstr<max_name> &name)
  {
    if ( name.empty() ) return *this;
    close();
    fname = name;
    reopen();
    return *this;
  }

  template <is_string T>
  file &
  operator=(const T &name)
  {
    if ( name.empty() ) return *this;
    close();
    fname = name;
    reopen();
    return *this;
  }

  void
  close(void)
  {
    if ( __handle.closed() ) return;
    posix::close(__handle.fd);
    __handle.fd = posix::invalid_fd.fd;
  }

  bool
  reopen(const modes mode = modes::read)
  {
    if ( fname.size() == 0 ) return false;
    close();
    __handle.fd = static_cast<int>(__syscall_open(fname.c_str(), mode));
    micron::zero(&sd);
    return !__handle.has_error();
  }

  bool
  reopen(const char *name, const modes mode)
  {
    if ( !name ) return false;
    fname = name;
    if ( fname.size() == 0 ) return false;
    close();
    __handle.fd = static_cast<int>(__syscall_open(fname.c_str(), mode));
    micron::zero(&sd);
    return !__handle.has_error();
  }

  template <bool B = STAT_EXISTING>
  inline void
  stat(void) const
  {
    if constexpr ( B == STAT_EXISTING ) {
      if ( !micron::is_zero(&sd) ) return;
      __alive();
      if ( posix::fstat(__handle, sd) == posix::invalid_fd ) exc<except::io_error>("micron::file, fstat failed.");
    } else if constexpr ( B == STAT_OVERRIDE ) {
      __alive();
      if ( posix::fstat(__handle, sd) == posix::invalid_fd ) exc<except::io_error>("micron::file, fstat failed.");
    }
  }

  inline auto
  size(void) const
  {
    __alive();
    stat();
    return sd.st_size;
  }

  inline auto
  is_system_virtual(void)
  {
    __alive();
    stat();
    return (posix::major(sd.st_dev) == 0);
  }

  inline auto
  owner(void)
  {
    __alive();
    stat();
    return micron::tie({ sd.st_uid, sd.st_gid });
  }

  inline auto
  access(void)
  {
    __alive();
    stat();
    return micron::tie({ sd.st_atime, sd.st_mtime });
  }

  inline auto
  modified(void)
  {
    __alive();
    stat();
    return sd.st_mtime;
  }

  inline linux_permissions
  permissions(void)
  {
    __alive();
    stat();
    linux_permissions perms = linux_permissions::from_mode(sd.st_mode);
    perms.setuid = (sd.st_mode & posix::s_isuid) != 0;
    perms.setgid = (sd.st_mode & posix::s_isgid) != 0;
    perms.sticky = (sd.st_mode & posix::s_isvtx) != 0;
    return perms;
  }

  inline void
  set_permissions(const linux_permissions &perms)
  {
    __alive();
    posix::fchmod(__handle, perms.full_mode());
    micron::zero(&sd);     // invalidate cached stat
  }

  inline const auto &
  name(void) const
  {
    return fname;
  }

  bool
  valid(void) const noexcept
  {
    return !__handle.closed();
  }

  explicit
  operator bool(void) const noexcept
  {
    return valid();
  }

  constexpr
  operator fd_t(void) const noexcept
  {
    return __handle;
  }

  fd_t
  fd(void) const noexcept
  {
    return __handle;
  }

  i32
  raw_fd(void) const noexcept
  {
    return __handle.fd;
  }

  bool
  is_owned(void) const noexcept
  {
    return true;
  }     // file always owns its fd

  posix::ino_t
  inode(void)
  {
    __alive();
    stat();
    return sd.st_ino;
  }

  posix::dev_t
  device(void)
  {
    __alive();
    stat();
    return sd.st_dev;
  }

  posix::dev_t
  rdev(void)
  {
    __alive();
    stat();
    return sd.st_rdev;
  }

  posix::nlink_t
  link_count(void)
  {
    __alive();
    stat();
    return sd.st_nlink;
  }

  posix::uid_t
  uid(void)
  {
    __alive();
    stat();
    return sd.st_uid;
  }

  posix::gid_t
  gid(void)
  {
    __alive();
    stat();
    return sd.st_gid;
  }

  posix::time_t
  atime(void)
  {
    __alive();
    stat();
    return sd.st_atime;
  }

  posix::time_t
  mtime(void)
  {
    __alive();
    stat();
    return sd.st_mtime;
  }

  posix::time_t
  ctime(void)
  {
    __alive();
    stat();
    return sd.st_ctime;
  }

  posix::blksize_t
  blksize(void)
  {
    __alive();
    stat();
    return sd.st_blksize;
  }

  posix::blkcnt_t
  blocks(void)
  {
    __alive();
    stat();
    return sd.st_blocks;
  }

  posix::mode_t
  mode(void)
  {
    __alive();
    stat();
    return sd.st_mode;
  }

  const stat_t &
  refresh_stat(void)
  {
    stat<STAT_OVERRIDE>();
    return sd;
  }

  bool
  is_regular(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_reg(__sd());
  }

  bool
  is_directory(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_dir(__sd());
  }

  bool
  is_symlink(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_lnk(__sd());
  }

  bool
  is_fifo(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_fifo(__sd());
  }

  bool
  is_socket(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_sock(__sd());
  }

  bool
  is_block_device(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_blk(__sd());
  }

  bool
  is_char_device(void)
  {
    __alive();
    stat();
    return posix::__impl::stat_is_chr(__sd());
  }

  bool
  is_pipe(void)
  {
    return is_fifo();
  }

  bool
  is_device(void)
  {
    return is_block_device() || is_char_device();
  }

  bool
  is_virtual(void)
  {
    return is_system_virtual();
  }

  bool
  readable(void)
  {
    return fname.size() && posix::is_readable(fname.c_str());
  }

  bool
  writable(void)
  {
    return fname.size() && posix::is_writable(fname.c_str());
  }

  bool
  executable(void)
  {
    return fname.size() && posix::is_executable(fname.c_str());
  }

  bool
  mode_user_read(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_irusr);
  }

  bool
  mode_user_write(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_iwusr);
  }

  bool
  mode_user_exec(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_ixusr);
  }

  bool
  mode_group_read(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_irgrp);
  }

  bool
  mode_group_write(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_iwgrp);
  }

  bool
  mode_group_exec(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_ixgrp);
  }

  bool
  mode_other_read(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_iroth);
  }

  bool
  mode_other_write(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_iwoth);
  }

  bool
  mode_other_exec(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_ixoth);
  }

  bool
  has_setuid(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_isuid);
  }

  bool
  has_setgid(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_isgid);
  }

  bool
  has_sticky(void)
  {
    __alive();
    stat();
    return posix::__impl::mode_bit(__sd(), posix::s_isvtx);
  }

  i32
  set_owner(posix::uid_t uid, posix::gid_t gid)
  {
    __alive();
    i32 r = (micron::fchown(__handle.fd, uid, gid));
    if ( r == 0 ) micron::zero(&sd);
    return r;
  }

  bool
  is_owned_by(posix::uid_t uid)
  {
    __alive();
    stat();
    return sd.st_uid == uid;
  }

  bool
  is_in_group(posix::gid_t gid)
  {
    __alive();
    stat();
    return sd.st_gid == gid;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // containers
  template <is_iterable_container T>
  max_t
  read(T &buf) const
  {
    __alive();
    return posix::read(__handle.fd, buf.data(), buf.max_size());
  }

  template <is_iterable_container T>
  max_t
  write(const T &buf)
  {
    __alive();
    return posix::write(__handle.fd, buf.data(), buf.size());
  }

  template <is_iterable_container T>
  max_t
  read_all(T &buf) const
  {
    __alive();
    return posix::read_all(__handle, buf.data(), buf.max_size());
  }

  template <is_iterable_container T>
  max_t
  write_all(const T &buf)
  {
    __alive();
    return posix::write_all(__handle, buf.data(), buf.size());
  }

  template <is_iterable_container T>
  file &
  operator>>(T &buf)
  {
    rewind();
    read_all(buf);
    return *this;
  }

  template <is_string T>
  file &
  operator>>(T &str)
  {
    rewind();
    read_all(str);
    return *this;
  }

  template <is_iterable_container T>
  file &
  operator<<(const T &buf)
  {
    write(buf);
    return *this;
  }

  template <is_string T>
  file &
  operator<<(const T &str)
  {
    write(str);
    return *this;
  }

  file &
  operator<<(const char *str)
  {
    if ( !__alive() || str == nullptr ) return *this;
    posix::write(__handle.fd, str, micron::strlen(str));
    return *this;
  }

  template <is_iterable_container T>
  max_t
  pread(T &buf, usize len, posix::off_t offset) const
  {
    __alive();
    return micron::pread(__handle.fd, buf.data(), len, offset);
  }

  template <is_iterable_container T>
  max_t
  pwrite(const T &buf, usize len, posix::off_t offset)
  {
    __alive();
    return micron::pwrite(__handle.fd, buf.data(), len, offset);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // raw pointers
  template <typename T>

  [[deprecated("prefer using T& reference overloads")]] max_t
  read(T *buf, usize len) const
  {
    __alive();
    return posix::read(__handle.fd, buf, len);
  }

  template <typename T>

  [[deprecated("prefer using T& reference overloads")]] max_t
  write(const T *buf, usize len)
  {
    __alive();
    return posix::write(__handle.fd, buf, len);
  }

  template <typename T>

  [[deprecated("prefer using T& reference overloads")]] max_t
  read_all(T *buf, usize len) const
  {
    __alive();
    return posix::read_all(__handle, buf, len);
  }

  template <typename T>

  [[deprecated("prefer using T& reference overloads")]] max_t
  write_all(const T *buf, usize len)
  {
    __alive();
    return posix::write_all(__handle, buf, len);
  }

  template <typename T>

  [[deprecated("prefer using T& reference overloads")]] max_t
  pread(T *buf, usize len, posix::off_t offset) const
  {
    __alive();
    return micron::pread(__handle.fd, buf, len, offset);
  }

  template <typename T>
  [[deprecated("prefer using T& reference overloads")]] max_t
  pwrite(const T *buf, usize len, posix::off_t offset)
  {
    __alive();
    return micron::pwrite(__handle.fd, buf, len, offset);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // refs (preferred)

  template <typename T>
  max_t
  read(T &buf, usize len) const
  {
    __alive();
    return posix::read(__handle.fd, micron::voidify(buf), len);
  }

  template <typename T>
  max_t
  write(const T &buf, usize len)
  {
    __alive();
    return posix::write(__handle.fd, micron::voidify(buf), len);
  }

  template <typename T>
  max_t
  read_all(T &buf, usize len) const
  {
    __alive();
    return posix::read_all(__handle, micron::voidify(buf), len);
  }

  template <typename T>
  max_t
  write_all(const T &buf, usize len)
  {
    __alive();
    return posix::write_all(__handle, micron::voidify(buf), len);
  }

  template <typename T>
  max_t
  pread(T &buf, usize len, posix::off_t offset) const
  {
    __alive();
    return micron::pread(__handle.fd, micron::voidify(buf), len, offset);
  }

  template <typename T>
  max_t
  pwrite(const T &buf, usize len, posix::off_t offset)
  {
    __alive();
    return micron::pwrite(__handle.fd, micron::voidify(buf), len, offset);
  }

  posix::off_t
  seek_to(posix::off_t offset)
  {
    __alive();
    return posix::seek_to(__handle, offset);
  }

  posix::off_t
  seek_by(posix::off_t delta)
  {
    __alive();
    return posix::seek_by(__handle, delta);
  }

  posix::off_t
  seek_end(posix::off_t off = 0)
  {
    __alive();
    return posix::seek_to(__handle, posix::seek_end - off);
  }

  posix::off_t
  tell(void) const
  {
    __alive();
    return posix::tell(__handle);
  }

  void
  rewind(void)
  {
    __alive();
    seek_to(0);
  }

  i32
  truncate(posix::off_t length)
  {
    __alive();
    i32 r = posix::truncate(__handle, length);
    if ( r == 0 ) micron::zero(&sd);
    return r;
  }

  i32
  clear(void)
  {
    return truncate(0);
  }

  i32
  flush(void)
  {
    __alive();
    return posix::flush(__handle);
  }

  i32
  flush_data(void)
  {
    __alive();
    return posix::flush_data(__handle);
  }

  i32
  sync(void)
  {
    __alive();
    return flush();
  }

  i32
  chmod(posix::mode_t mode)
  {
    __alive();
    i32 r = (micron::fchmod(__handle.fd, mode));
    if ( r == 0 ) micron::zero(&sd);
    return r;
  }

  i32
  chmod(const linux_permissions &p)
  {
    return chmod(p.full_mode());
  }

  i32
  chown(posix::uid_t uid, posix::gid_t gid)
  {
    __alive();
    i32 r = (micron::fchown(__handle.fd, uid, gid));
    if ( r == 0 ) micron::zero(&sd);
    return r;
  }

  i32
  rename(const char *newpath)
  {
    __alive();
    if ( fname.size() == 0 ) return -1;
    i32 r = (micron::rename(fname.c_str(), newpath));
    if ( r == 0 ) fname = newpath;
    return r;
  }

  template <is_string T>
  i32
  rename(const T &newpath)
  {
    __alive();
    return rename(newpath.c_str());
  }

  i32
  unlink(void)
  {
    __alive();
    if ( fname.size() == 0 ) return -1;
    return micron::unlink(fname.c_str());
  }

  i32
  link(const char *newpath) const
  {
    __alive();
    if ( fname.size() == 0 ) return -1;
    return micron::link(fname.c_str(), newpath);
  }

  template <is_string T>
  i32
  link(const T &newpath) const
  {
    __alive();
    return link(newpath.c_str());
  }

  i32
  symlink(const char *linkpath) const
  {
    __alive();
    if ( fname.size() == 0 ) return -1;
    return micron::symlink(fname.c_str(), linkpath);
  }

  template <is_string T>
  i32
  symlink(const T &linkpath) const
  {
    __alive();
    return symlink(linkpath.c_str());
  }

  fd_t
  dup(void) const
  {
    __alive();
    return fd_t{ micron::dup(__handle.fd) };
  }

  fd_t
  dup2(i32 target) const
  {
    __alive();
    return fd_t{ micron::dup2(__handle.fd, target) };
  }

  i32
  get_status_flags(void) const
  {
    __alive();
    return posix::fcntl(__handle.fd, posix::f_getfl);
  }

  i32
  set_status_flags(i32 flags)
  {
    __alive();
    return posix::fcntl(__handle.fd, posix::f_setfl, flags);
  }

  i32
  get_fd_flags(void) const
  {
    __alive();
    return posix::fcntl(__handle.fd, posix::f_getfd);
  }

  i32
  set_fd_flags(i32 flags)
  {
    __alive();
    return posix::fcntl(__handle.fd, posix::f_setfd, flags);
  }

  bool
  is_cloexec(void) const
  {
    i32 f = get_fd_flags();
    return f >= 0 && (f & 1);     // FD_CLOEXEC == 1
  }

  i32
  set_cloexec(bool on = true)
  {
    i32 cur = get_fd_flags();
    if ( cur < 0 ) return cur;
    return set_fd_flags(on ? (cur | 1) : (cur & ~1));
  }

  bool
  is_nonblock(void) const
  {
    i32 f = get_status_flags();
    return f >= 0 && (f & posix::o_nonblock);
  }

  i32
  set_nonblock(bool on = true)
  {
    i32 cur = get_status_flags();
    if ( cur < 0 ) return cur;
    return set_status_flags(on ? (cur | posix::o_nonblock) : (cur & ~posix::o_nonblock));
  }

  i32
  set_append(bool on = true)
  {
    i32 cur = get_status_flags();
    if ( cur < 0 ) return cur;
    return set_status_flags(on ? (cur | posix::o_append) : (cur & ~posix::o_append));
  }

  template <typename... Args>
  auto
  fcntl(i32 cmd, Args &&...args)
  {
    __alive();
    return posix::fcntl(__handle.fd, cmd, args...);
  }

  i32
  lock_shared(void)
  {
    __alive();
    posix::flock_t fl{ 0, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return (posix::fcntl(__handle.fd, posix::f_setlkw, &fl));
  }

  i32
  lock_exclusive(void)
  {
    __alive();
    posix::flock_t fl{ 1, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return (posix::fcntl(__handle.fd, posix::f_setlkw, &fl));
  }

  i32
  try_lock_shared(void)
  {
    __alive();
    posix::flock_t fl{ 0, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return (posix::fcntl(__handle.fd, posix::f_setlk, &fl));
  }

  i32
  try_lock_exclusive(void)
  {
    __alive();
    posix::flock_t fl{ 1, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return (posix::fcntl(__handle.fd, posix::f_setlk, &fl));
  }

  i32
  unlock(void)
  {
    __alive();
    posix::flock_t fl{ 2, static_cast<i16>(posix::seek_set), 0, 0, 0 };
    return (posix::fcntl(__handle.fd, posix::f_setlk, &fl));
  }

  i32
  lock_range(posix::off64_t start, posix::off64_t end_pos, bool exclusive = true, bool wait = true)
  {
    __alive();
    posix::flock_t fl{ static_cast<i16>(exclusive ? 1 : 0), static_cast<i16>(posix::seek_set), start, end_pos, 0 };
    i32 cmd = wait ? posix::f_setlkw : posix::f_setlk;
    return (posix::fcntl(__handle.fd, cmd, &fl));
  }

  i32
  unlock_range(posix::off64_t start, posix::off64_t end_pos)
  {
    __alive();
    posix::flock_t fl{ 2, static_cast<i16>(posix::seek_set), start, end_pos, 0 };
    return (posix::fcntl(__handle.fd, posix::f_setlk, &fl));
  }

  i32
  allocate(posix::off_t offset, posix::off_t len, i32 mode_flags = 0)
  {
    __alive();
    i32 r = (micron::fallocate(__handle.fd, mode_flags, offset, len));
    if ( r == 0 ) micron::zero(&sd);
    return r;
  }

  i32
  preallocate(posix::off_t len)
  {
    return allocate(0, len, 0);
  }

  i32
  punch_hole(posix::off_t off, posix::off_t len)
  {
    return allocate(off, len, 0x03);
  }

  i32
  zero_range(posix::off_t off, posix::off_t len)
  {
    return allocate(off, len, 0x10);
  }

  max_t
  copy_to(fd_t dst, usize count) const
  {
    __alive();
    return posix::copy_fd(dst, __handle, count);
  }

  max_t
  copy_to(const file &dst, usize count) const
  {
    return copy_to(dst.__handle, count);
  }

  max_t
  copy_from(fd_t src, usize count)
  {
    __alive();
    max_t r = posix::copy_fd(__handle, src, count);
    if ( r > 0 ) micron::zero(&sd);
    return r;
  }

  max_t
  splice_to(fd_t dst, usize count, i32 flags = 0) const
  {
    __alive();
    return micron::splice(__handle.fd, nullptr, dst.fd, nullptr, count, flags);
  }

  max_t
  splice_from(fd_t src, usize count, i32 flags = 0)
  {
    __alive();
    return micron::splice(src.fd, nullptr, __handle.fd, nullptr, count, flags);
  }

  max_t
  copy_range_to(file &dst, usize count, posix::off_t src_off = -1, posix::off_t dst_off = -1) const
  {
    __alive();
    posix::off_t *sp = (src_off < 0) ? nullptr : &src_off;
    posix::off_t *dp = (dst_off < 0) ? nullptr : &dst_off;
    return micron::copy_file_range(__handle.fd, sp, dst.__handle.fd, dp, count, 0u);
  }

  bool
  is_same_as(const file &o) const
  {
    __alive();
    return posix::is_same_file(__handle, o.__handle);
  }

  bool
  is_same_as(const char *path) const
  {
    return fname.size() && posix::is_same_file(fname.c_str(), path);
  }

  template <is_string T>
  bool
  is_same_as(const T &p) const
  {
    __alive();
    return is_same_as(p.c_str());
  }

  max_t
  getxattr(const char *attr_name, void *value, usize sz) const
  {
    __alive();
    return posix::fgetxattr(__handle.fd, attr_name, value, sz);
  }

  i32
  setxattr(const char *attr_name, const void *value, usize sz, i32 flags = 0)
  {
    __alive();
    return (posix::fsetxattr(__handle.fd, attr_name, value, sz, flags));
  }

  i32
  removexattr(const char *attr_name)
  {
    __alive();
    return (posix::fremovexattr(__handle.fd, attr_name));
  }

  max_t
  listxattr(char *list, usize sz) const
  {
    __alive();
    return posix::flistxattr(__handle.fd, list, sz);
  }
};

struct virtual_file : public file {

  virtual_file() = default;

  explicit virtual_file(const char *path) : file(path, modes::read) {}

  template <is_string T> explicit virtual_file(const T &path) : virtual_file(path.c_str()) {}

  template <is_iterable_container T>
  max_t
  read_once(T &buf)
  {
    seek_to(0);
    return read(buf);
  }

  template <is_iterable_container T>
  max_t
  read_at_zero(T &buf, usize len) const
  {
    return read(buf, len, 0);
  }

  template <typename T>
  max_t
  read_once(T *buf, usize len)
  {
    seek_to(0);
    return read(buf, len);
  }

  template <typename T>
  max_t
  read_at_zero(T *buf, usize len) const
  {
    return pread(buf, len, 0);
  }

  bool
  is_virtual_fs(void)
  {
    return is_system_virtual();
  }
};

struct regular_file : public file {

  regular_file() = default;

  explicit regular_file(const char *path, const modes mode = modes::read) : file(path, mode) {}

  template <is_string T> explicit regular_file(const T &path, const modes mode = modes::read) : file(path.c_str(), mode) {}

  static regular_file
  for_reading(const char *path)
  {
    return regular_file{ path, modes::read };
  }

  static regular_file
  for_writing(const char *path)
  {
    return regular_file{ path, modes::write };
  }

  static regular_file
  for_appending(const char *path)
  {
    return regular_file{ path, modes::append };
  }

  static regular_file
  for_rdwr(const char *path)
  {
    return regular_file{ path, modes::readwrite };
  }

  static regular_file
  create_new(const char *path)
  {
    return regular_file{ path, modes::create };
  }

  template <is_string T>
  static regular_file
  for_reading(const T &p)
  {
    return for_reading(p.c_str());
  }

  template <is_string T>
  static regular_file
  for_writing(const T &p)
  {
    return for_writing(p.c_str());
  }

  template <is_string T>
  static regular_file
  for_appending(const T &p)
  {
    return for_appending(p.c_str());
  }

  template <is_string T>
  static regular_file
  for_rdwr(const T &p)
  {
    return for_rdwr(p.c_str());
  }

  template <is_string T>
  static regular_file
  create_new(const T &p)
  {
    return create_new(p.c_str());
  }

  constexpr
  operator fd_t(void) const noexcept
  {
    return __handle;
  }

  bool
  is_empty(void)
  {
    return size() == 0;
  }

  bool
  is_sparse(void)
  {
    return blocks() < (size() / 512);
  }
};

struct symlink_file {
protected:
  mutable stat_t _st;
  mutable bool _st_dirty;
  char __path[posix::path_max];
  bool _st_valid;

  void
  _set_path(const char *src) noexcept
  {
    if ( !src ) {
      __path[0] = '\0';
      return;
    }
    usize i = 0;
    while ( src[i] && i < posix::path_max - 1 ) {
      __path[i] = src[i];
      ++i;
    }
    __path[i] = '\0';
  }

  void
  _load_stat(void) const
  {
    if ( !_st_dirty ) return;
    _st_dirty = false;
    if ( __path[0] ) posix::__impl::__lstat(__path, _st);
  }

public:
  symlink_file() noexcept : _st{}, _st_dirty(true), _st_valid(false) { __path[0] = '\0'; }

  explicit symlink_file(const char *path) noexcept : _st{}, _st_dirty(true), _st_valid(false) { _set_path(path); }

  template <is_string T> explicit symlink_file(const T &path) noexcept : symlink_file(path.c_str()) {}

  symlink_file(const symlink_file &) = delete;
  symlink_file &operator=(const symlink_file &) = delete;

  symlink_file(symlink_file &&o) noexcept : _st(o._st), _st_dirty(o._st_dirty), _st_valid(o._st_valid)
  {
    _set_path(o.__path);
    o.__path[0] = '\0';
  }

  const char *
  path(void) const noexcept
  {
    return __path;
  }

  bool
  valid(void) const noexcept
  {
    return __path[0] != '\0';
  }

  explicit
  operator bool(void) const noexcept
  {
    return valid();
  }

  bool
  exists(void) const
  {
    return posix::lexists(__path);
  }

  posix::ino_t
  inode(void) const
  {
    _load_stat();
    return _st.st_ino;
  }

  posix::dev_t
  device(void) const
  {
    _load_stat();
    return _st.st_dev;
  }

  posix::uid_t
  uid(void) const
  {
    _load_stat();
    return _st.st_uid;
  }

  posix::gid_t
  gid(void) const
  {
    _load_stat();
    return _st.st_gid;
  }

  posix::time_t
  atime(void) const
  {
    _load_stat();
    return _st.st_atime;
  }

  posix::time_t
  mtime(void) const
  {
    _load_stat();
    return _st.st_mtime;
  }

  posix::time_t
  ctime(void) const
  {
    _load_stat();
    return _st.st_ctime;
  }

  posix::off_t
  target_size(void) const
  {
    _load_stat();
    return _st.st_size;
  }

  linux_permissions
  permissions(void) const
  {
    _load_stat();
    linux_permissions p = linux_permissions::from_mode(_st.st_mode);
    p.setuid = (_st.st_mode & posix::s_isuid) != 0;
    p.setgid = (_st.st_mode & posix::s_isgid) != 0;
    p.sticky = (_st.st_mode & posix::s_isvtx) != 0;
    return p;
  }

  i32
  create(const char *target)
  {
    i32 r = (posix::symlink(target, __path));
    if ( r == 0 ) _st_dirty = true;
    return r;
  }

  template <is_string T>
  i32
  create(const T &target)
  {
    return create(target.c_str());
  }

  max_t
  read_target(char *buf, usize sz) const
  {
    return posix::readlink(__path, buf, sz);
  }

  micron::sstr<posix::path_max>
  target(void) const
  {
    char buf[posix::path_max];
    max_t n = read_target(buf, posix::path_max - 1);
    if ( n <= 0 ) return micron::sstr<posix::path_max>{};
    buf[n] = '\0';
    return micron::sstr<posix::path_max>{ buf };
  }

  i32
  unlink(void)
  {
    return (posix::unlink(__path));
  }

  i32
  chown(posix::uid_t uid, posix::gid_t gid)
  {
    i32 r = (micron::fchownat(posix::at_fdcwd, __path, uid, gid, posix::at_symlink_nofollow));
    if ( r == 0 ) _st_dirty = true;
    return r;
  }

  static symlink_file
  create_link(const char *target, const char *link_path)
  {
    posix::symlink(target, link_path);
    return symlink_file{ link_path };
  }

  template <is_string T, is_string U>
  static symlink_file
  create_link(const T &target, const U &link_path)
  {
    return create_link(target.c_str(), link_path.c_str());
  }
};

struct directory_file : public file {

  directory_file() = default;

  explicit directory_file(const char *path) : file(path, modes::read) {}

  template <is_string T> explicit directory_file(const T &path) : directory_file(path.c_str()) {}

  posix::__impl_dir
  next(void)
  {
    return posix::readdir(__handle);
  }

  void
  rewind(void)
  {
    seek_to(0);
  }

  template <typename Fn>
  void
  for_each(Fn &&fn)
  {
    if ( fname.size() ) posix::for_each_entry(fname.c_str(), fn);
  }

  bool
  is_empty_dir(void)
  {
    return posix::is_empty_dir(__handle);
  }

  fd_t
  open_child(const char *name, i32 flags = posix::o_rdonly | posix::o_cloexec, u32 mode = 0)
  {
    return posix::open_at(__handle, name, flags, mode);
  }

  template <is_string T>
  fd_t
  open_child(const T &name, i32 flags = posix::o_rdonly | posix::o_cloexec, u32 mode = 0)
  {
    return open_child(name.c_str(), flags, mode);
  }

  i32
  make_child_dir(const char *name, const linux_permissions &p = perm_dir_default)
  {
    return posix::mkdir_at(__handle, name, p.to_mode());
  }

  template <is_string T>
  i32
  make_child_dir(const T &name, const linux_permissions &p = perm_dir_default)
  {
    return make_child_dir(name.c_str(), p);
  }

  i32
  remove_child(const char *name)
  {
    return (micron::unlinkat(__handle.fd, name, 0));
  }

  i32
  remove_child_dir(const char *name)
  {
    return (micron::unlinkat(__handle.fd, name, posix::at_removedir));
  }

  template <is_string T>
  i32
  remove_child(const T &s)
  {
    return remove_child(s.c_str());
  }

  template <is_string T>
  i32
  remove_child_dir(const T &s)
  {
    return remove_child_dir(s.c_str());
  }

  bool
  child_is_file(const char *n) const
  {
    return posix::is_file_at(__handle, n);
  }

  bool
  child_is_dir(const char *n) const
  {
    return posix::is_dir_at(__handle, n);
  }

  bool
  child_is_link(const char *n) const
  {
    return posix::is_symlink_at(__handle, n);
  }

  bool
  child_is_fifo(const char *n) const
  {
    return posix::is_fifo_at(__handle, n);
  }

  bool
  child_exists(const char *n) const
  {
    return posix::exists_at(__handle, n);
  }

  template <is_string T>
  bool
  child_is_file(const T &s) const
  {
    return child_is_file(s.c_str());
  }

  template <is_string T>
  bool
  child_is_dir(const T &s) const
  {
    return child_is_dir(s.c_str());
  }

  template <is_string T>
  bool
  child_is_link(const T &s) const
  {
    return child_is_link(s.c_str());
  }

  template <is_string T>
  bool
  child_is_fifo(const T &s) const
  {
    return child_is_fifo(s.c_str());
  }

  template <is_string T>
  bool
  child_exists(const T &s) const
  {
    return child_exists(s.c_str());
  }

  static directory_file
  create(const char *path, const linux_permissions &p = perm_dir_default)
  {
    posix::mkdir(path, p.to_mode());
    return directory_file{ path };
  }

  template <is_string T>
  static directory_file
  create(const T &path, const linux_permissions &p = perm_dir_default)
  {
    return create(path.c_str(), p);
  }

  static directory_file
  create_all(const char *path, const linux_permissions &p = perm_dir_default)
  {
    posix::mkdir_p(path, p.to_mode());
    return directory_file{ path };
  }

  template <is_string T>
  static directory_file
  create_all(const T &path, const linux_permissions &p = perm_dir_default)
  {
    return create_all(path.c_str(), p);
  }
};

struct fifo_file : public file {

  fifo_file() = default;

  explicit fifo_file(const char *path, const modes mode = modes::read) : file(path, mode) {}

  template <is_string T> explicit fifo_file(const T &path, const modes mode = modes::read) : file(path.c_str(), mode) {}

  static fifo_file
  create(const char *path, const linux_permissions &p = perm_file_default)
  {
    posix::mkfifo(path, p.to_posix_mode());
    return fifo_file{ path, modes::readwrite };
  }

  template <is_string T>
  static fifo_file
  create(const T &path, const linux_permissions &p = perm_file_default)
  {
    return create(path.c_str(), p);
  }

  static fifo_file
  open_read(const char *p)
  {
    return fifo_file{ p, modes::read };
  }

  static fifo_file
  open_write(const char *p)
  {
    return fifo_file{ p, modes::write };
  }

  template <is_string T>
  static fifo_file
  open_read(const T &s)
  {
    return open_read(s.c_str());
  }

  template <is_string T>
  static fifo_file
  open_write(const T &s)
  {
    return open_write(s.c_str());
  }
};

struct device_file : public file {

  device_file() = default;

  explicit device_file(const char *path, const modes mode = modes::readwrite) : file(path, mode) {}

  template <is_string T> explicit device_file(const T &path, const modes mode = modes::readwrite) : file(path.c_str(), mode) {}

  u32
  major_num(void)
  {
    return posix::major(rdev());
  }

  u32
  minor_num(void)
  {
    return posix::minor(rdev());
  }

  bool
  is_block(void)
  {
    return is_block_device();
  }

  bool
  is_char(void)
  {
    return is_char_device();
  }

  template <typename Arg>
  i32
  ioctl(u64 request, Arg &&arg)
  {
    return (posix::ioctl(__handle.fd, request, arg));
  }

  i32
  ioctl(u64 request)
  {
    return (posix::ioctl(__handle.fd, request));
  }

  static i32
  create_node(const char *path, bool block, u32 major_n, u32 minor_n, const linux_permissions &p = linux_permissions::from_mode(0600))
  {
    posix::dev_t dev = posix::makedev(major_n, minor_n);
    u32 type = block ? static_cast<u32>(posix::inode_blck) : static_cast<u32>(posix::inode_char);
    return (micron::mknod(path, p.to_mode() | type, dev));
  }

  template <is_string T>
  static i32
  create_node(const T &path, bool block, u32 major_n, u32 minor_n, const linux_permissions &p = linux_permissions::from_mode(0600))
  {
    return create_node(path.c_str(), block, major_n, minor_n, p);
  }
};

struct socket_file {
protected:
  char __path[posix::path_max];

  void
  _set_path(const char *src) noexcept
  {
    if ( !src ) {
      __path[0] = '\0';
      return;
    }
    usize i = 0;
    while ( src[i] && i < posix::path_max - 1 ) {
      __path[i] = src[i];
      ++i;
    }
    __path[i] = '\0';
  }

public:
  socket_file() noexcept { __path[0] = '\0'; }

  explicit socket_file(const char *path) noexcept { _set_path(path); }

  template <is_string T> explicit socket_file(const T &path) noexcept : socket_file(path.c_str()) {}

  socket_file(const socket_file &) = delete;
  socket_file &operator=(const socket_file &) = delete;
  socket_file(socket_file &&) = default;
  socket_file &operator=(socket_file &&) = default;

  const char *
  path(void) const noexcept
  {
    return __path;
  }

  bool
  valid(void) const noexcept
  {
    return __path[0] != '\0';
  }

  explicit
  operator bool(void) const noexcept
  {
    return valid();
  }

  bool
  exists(void) const
  {
    return posix::is_socket(__path);
  }

  posix::uid_t
  uid(void) const
  {
    return posix::get_uid(__path);
  }

  posix::gid_t
  gid(void) const
  {
    return posix::get_gid(__path);
  }

  linux_permissions
  permissions(void) const
  {
    return linux_permissions::from_mode(posix::get_permissions(__path));
  }

  i32
  chmod(const linux_permissions &p)
  {
    return (posix::chmod(__path, p.to_mode()));
  }

  i32
  unlink(void)
  {
    return (posix::unlink(__path));
  }
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// functions

inline regular_file
open_file(const char *path, const modes mode = modes::read)
{
  return regular_file{ path, mode };
}

template <is_string T>
inline regular_file
open_file(const T &path, const modes mode = modes::read)
{
  return regular_file{ path.c_str(), mode };
}

inline regular_file
create_file(const char *path)
{
  return regular_file::create_new(path);
}

template <is_string T>
inline regular_file
create_file(const T &path)
{
  return regular_file::create_new(path.c_str());
}

inline directory_file
open_dir_file(const char *path)
{
  return directory_file{ path };
}

template <is_string T>
inline directory_file
open_dir_file(const T &path)
{
  return directory_file{ path.c_str() };
}

inline symlink_file
open_symlink(const char *path)
{
  return symlink_file{ path };
}

template <is_string T>
inline symlink_file
open_symlink(const T &path)
{
  return symlink_file{ path.c_str() };
}

};     // namespace io
};     // namespace micron
