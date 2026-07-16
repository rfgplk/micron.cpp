//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../maps/heap_swiss.hpp"
#include "../memory/pointers/unique.hpp"
#include "../memory_block.hpp"
#include "../mutex/mutex.hpp"
#include "../vector/vector.hpp"

#include "file.hpp"
#include "format.hpp"
#include "paths.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io::basic_filesystem
// path-keyed cache of open io::file handles plus the full set of
// path-based filesystem operations
//   * queries never open descriptors as a side effect
//   * only operator[]/open/create/at insert handles into the cache
//   * data-path operations return signed max_t (>= 0 count, < 0 negative errno) and never
//     throw; implicit opens (operator[]/apply on a missing entry) construct io::file, which
//     throws except::io_error on failure
//   * lock is a policy (micron::null_lock = single-threaded, zero cost; micron::mutex =
//     concurrent)
//   * Cap > 0 bounds the cache with LRU eviction of the coldest handle

namespace micron
{
namespace io
{

// address-stable cache entry; io::file lives in its own heap allocation via a unique_pointer, so heap_swiss_map rehashes (which move its
// entries) only relocates the unique_pointer
struct __fs_entry {
  micron::unique_pointer<io::file> fp;
  io::modes mode = io::modes::read;

  __fs_entry() = default;

  __fs_entry(micron::unique_pointer<io::file> &&p, io::modes m) : fp(micron::move(p)), mode(m) { }

  __fs_entry(__fs_entry &&) noexcept = default;
  __fs_entry &operator=(__fs_entry &&) noexcept = default;

  // copy duplicates the descriptor (io::file copy dup()s the fd)
  // WARNING: present ONLY to satisfy heap_swiss_maps is_copy_constructible_v
  __fs_entry(const __fs_entry &o)
      : fp(o.fp ? micron::unique_pointer<io::file>(*o.fp) : micron::unique_pointer<io::file>()), mode(o.mode) { }

  __fs_entry &
  operator=(const __fs_entry &o)
  {
    fp = o.fp ? micron::unique_pointer<io::file>(*o.fp) : micron::unique_pointer<io::file>();
    mode = o.mode;
    return *this;
  }
};

inline constexpr bool
__mode_can_read(const io::modes m) noexcept
{
  // every mode maps to O_RDONLY or O_RDWR except the two write-only ones
  return m != io::modes::write && m != io::modes::append;
}

inline constexpr bool
__mode_can_write(const io::modes m) noexcept
{
  return m == io::modes::large || m == io::modes::readwrite || m == io::modes::write || m == io::modes::readwritecreate
         || m == io::modes::append || m == io::modes::appendread;
}

inline constexpr bool
__mode_is_append(const io::modes m) noexcept
{
  return m == io::modes::append || m == io::modes::appendread;
}

inline constexpr bool
__mode_has_trunc(const io::modes m) noexcept
{
  return m == io::modes::large || m == io::modes::write || m == io::modes::readwritecreate;
}

inline constexpr bool
__mode_has_excl(const io::modes m) noexcept
{
  return m == io::modes::create;
}

inline constexpr bool
__mode_is_direct(const io::modes m) noexcept
{
  return m == io::modes::largeread || m == io::modes::large;
}

inline constexpr bool
__mode_is_noatime(const io::modes m) noexcept
{
  return m == io::modes::quiet;
}

inline constexpr bool
__mode_serviceable(const io::modes cached, const io::modes want) noexcept
{
  // O_TRUNC must empty the file and O_EXCL must fail when it exists
  if ( __mode_has_trunc(want) || __mode_has_excl(want) ) return false;
  if ( __mode_is_direct(want) != __mode_is_direct(cached) ) return false;
  if ( __mode_is_noatime(want) && !__mode_is_noatime(cached) ) return false;
  if ( __mode_is_append(want) ) return __mode_can_write(cached);                                   // append
  if ( __mode_can_write(want) ) return __mode_can_write(cached) && !__mode_is_append(cached);      // positioned write
  return __mode_can_read(cached);                                                                  // read
}

template<io::modes DefaultMode = io::modes::read, class Lock = micron::null_lock, usize Cap = 0> class basic_filesystem
{
  static constexpr bool __single_threaded = micron::is_same_v<Lock, micron::null_lock>;

  micron::heap_swiss_map<micron::string, __fs_entry> __open;
  mutable Lock __mtx;
  micron::vector<micron::string> __recency;      // only maintained when Cap > 0

  struct __scope {
    Lock &m;

    __scope(Lock &l) : m(l) { m.lock(); }

    ~__scope() { m.unlock(); }
  };

  static micron::string
  __key(const io::path_t &p)
  {
    io::path_t pr = io::prune(p);
    return micron::string(pr.c_str());
  }

  void
  __touch(const micron::string &k)
  {
    if constexpr ( Cap > 0 ) {
      for ( usize i = 0; i < __recency.size(); i++ ) {
        if ( __recency[i] == k ) {
          for ( usize j = i; j + 1 < __recency.size(); j++ ) __recency[j] = micron::move(__recency[j + 1]);
          __recency.pop_back();
          break;
        }
      }
      __recency.push_back(k);
      if ( __recency.size() > Cap ) {
        // flush the coldest handle before auto-evicting it, matching close()/evict() and the dtor
        if ( __fs_entry *e = __open.find(__recency[0]); e != nullptr && e->fp ) e->fp->flush_data();
        __open.erase(__recency[0]);
        for ( usize j = 0; j + 1 < __recency.size(); j++ ) __recency[j] = micron::move(__recency[j + 1]);
        __recency.pop_back();
      }
    }
  }

  void
  __drop_recency(const micron::string &k)
  {
    if constexpr ( Cap > 0 ) {
      for ( usize i = 0; i < __recency.size(); i++ ) {
        if ( __recency[i] == k ) {
          for ( usize j = i; j + 1 < __recency.size(); j++ ) __recency[j] = micron::move(__recency[j + 1]);
          __recency.pop_back();
          return;
        }
      }
    }
  }

  io::file &
  __get_or_open(const micron::string &k, io::modes m)
  {
    if ( __fs_entry *e = __open.find(k); e != nullptr ) {
      // cache hit
      if ( !__mode_serviceable(e->mode, m) ) {
        *e->fp = io::file(k.c_str(), m);
        e->mode = m;
      }
      __touch(k);
      return *e->fp;
    }
    auto ins = __open.emplace(k, __fs_entry{ micron::unique_pointer<io::file>(k.c_str(), m), m });
    __touch(k);
    return *ins.b->fp;
  }

public:
  using lock_type = Lock;

  ~basic_filesystem()
  {
    __open.for_each([](const micron::string &, __fs_entry &e) {
      if ( e.fp ) e.fp->flush_data();
    });
  }

  basic_filesystem() : __open(), __mtx(), __recency() { }

  basic_filesystem(const basic_filesystem &) = delete;
  basic_filesystem &operator=(const basic_filesystem &) = delete;

  basic_filesystem(basic_filesystem &&o) noexcept : __open(micron::move(o.__open)), __mtx(), __recency(micron::move(o.__recency)) { }

  basic_filesystem &
  operator=(basic_filesystem &&o) noexcept
  {
    if ( this == &o ) return *this;
    __open = micron::move(o.__open);
    __recency = micron::move(o.__recency);
    return *this;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // handle cache
  io::file &
  operator[](const io::path_t &p)
    requires __single_threaded
  {
    return __get_or_open(__key(p), DefaultMode);
  }

  io::file &
  operator[](const io::path_t &p, const io::modes m)
    requires __single_threaded
  {
    return __get_or_open(__key(p), m);
  }

  io::file &
  open(const io::path_t &p, const io::modes m = DefaultMode)
    requires __single_threaded
  {
    return __get_or_open(__key(p), m);
  }

  io::file &
  create(const io::path_t &p, const io::modes m = io::modes::readwritecreate)
    requires __single_threaded
  {
    return __get_or_open(__key(p), m);
  }

  io::file &
  at(const io::path_t &p)
    requires __single_threaded
  {
    __fs_entry *e = __open.find(__key(p));
    if ( e == nullptr || !e->fp ) exc<except::filesystem_error>("micron::io::filesystem::at(): path not open");
    return *e->fp;
  }

  template<typename Fn>
  auto
  apply(const io::path_t &p, const io::modes m, Fn &&fn)
  {
    __scope g(__mtx);
    return fn(__get_or_open(__key(p), m));
  }

  template<typename Fn>
  auto
  apply(const io::path_t &p, Fn &&fn)
  {
    return apply(p, DefaultMode, static_cast<Fn &&>(fn));
  }

  bool
  is_open(const io::path_t &p) const
  {
    __scope g(__mtx);
    return __open.contains(__key(p));
  }

  usize
  count(void) const
  {
    __scope g(__mtx);
    return __open.size();
  }

  void
  close(const io::path_t &p)
  {
    __scope g(__mtx);
    micron::string k = __key(p);
    if ( __fs_entry *e = __open.find(k); e != nullptr && e->fp ) e->fp->flush_data();
    __drop_recency(k);
    __open.erase(k);      // io::file dtor closes the fd
  }

  void
  evict(const io::path_t &p)
  {
    close(p);
  }

  void
  clear(void)
  {
    __scope g(__mtx);
    __open.for_each([](const micron::string &, __fs_entry &e) {
      if ( e.fp ) e.fp->flush_data();
    });
    __open.clear();
    if constexpr ( Cap > 0 ) __recency.clear();
  }

  micron::vector<micron::string>
  list(void) const
  {
    __scope g(__mtx);
    micron::vector<micron::string> out;
    out.reserve(__open.size() + 1);
    __open.for_each([&out](const micron::string &k, const __fs_entry &) { out.push_back(k); });
    return out;
  }

  max_t
  sync(const io::path_t &p)
  {
    __scope g(__mtx);
    __fs_entry *e = __open.find(__key(p));
    if ( e == nullptr || !e->fp ) return -error::bad_fd;
    return e->fp->flush();
  }

  void
  sync(void)
  {
    __scope g(__mtx);
    __open.for_each([](const micron::string &, __fs_entry &e) {
      if ( e.fp ) e.fp->flush();
    });
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // universal data path
  template<typename C>
  max_t
  read(const io::path_t &p, C &out)
  {
    __scope g(__mtx);
    io::file &f = __get_or_open(__key(p), DefaultMode);
    if ( max_t r = f.seek_to(0); r < 0 ) return r;
    if constexpr ( (is_string<C> || is_contiguous_container<C>) && micron::is_trivially_copyable_v<typename C::value_type> ) {
      out = f.template read<C>();
      return static_cast<max_t>(out.size() * sizeof(typename C::value_type));
    } else {
      return f.read(out);      // framed tiers already consume the whole remainder
    }
  }

  template<typename C>
  max_t
  write(const io::path_t &p, const C &c)
  {
    __scope g(__mtx);
    io::file &f = __get_or_open(__key(p), io::modes::readwritecreate);
    if ( max_t r = f.seek_to(0); r < 0 ) return r;
    max_t n = f.write(c);
    if ( n >= 0 ) f.truncate(static_cast<posix::off64_t>(n));
    return n;
  }

  template<typename C>
  max_t
  append(const io::path_t &p, const C &c)
  {
    __scope g(__mtx);
    io::file &f = __get_or_open(__key(p), io::modes::appendread);
    if ( max_t r = f.seek_end(); r < 0 ) return r;
    return f.write(c);
  }

  max_t
  truncate(const io::path_t &p, usize n)
  {
    __scope g(__mtx);
    io::file &f = __get_or_open(__key(p), io::modes::readwrite);
    return f.truncate(static_cast<posix::off64_t>(n));
  }

  template<typename C>
  i32
  atomic_replace(const io::path_t &p, const C &c)
  {
    __scope g(__mtx);
    io::file &f = __get_or_open(__key(p), io::modes::readwrite);
    return f.atomic_replace(c);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // path queries
  bool
  exists(const io::path_t &p) const
  {
    return posix::exists(p.c_str());
  }

  bool
  lexists(const io::path_t &p) const
  {
    return posix::lexists(p.c_str());
  }

  bool
  accessible(const io::path_t &p) const
  {
    return posix::access(p.c_str(), posix::access_ok) == 0;
  }

  bool
  readable(const io::path_t &p) const
  {
    return posix::is_readable(p.c_str());
  }

  bool
  writable(const io::path_t &p) const
  {
    return posix::is_writable(p.c_str());
  }

  bool
  executable(const io::path_t &p) const
  {
    return posix::is_executable(p.c_str());
  }

  bool
  is_regular_file(const io::path_t &p) const
  {
    return posix::is_file(p.c_str());
  }

  bool
  is_directory(const io::path_t &p) const
  {
    return posix::is_dir(p.c_str());
  }

  bool
  is_symlink(const io::path_t &p) const
  {
    return posix::is_symlink(p.c_str());
  }

  bool
  is_socket(const io::path_t &p) const
  {
    return posix::is_socket(p.c_str());
  }

  bool
  is_fifo(const io::path_t &p) const
  {
    return posix::is_fifo(p.c_str());
  }

  bool
  is_pipe(const io::path_t &p) const
  {
    return is_fifo(p);
  }

  bool
  is_block_device(const io::path_t &p) const
  {
    return posix::is_block_device(p.c_str());
  }

  bool
  is_char_device(const io::path_t &p) const
  {
    return posix::is_char_device(p.c_str());
  }

  bool
  is_device(const io::path_t &p) const
  {
    return is_block_device(p) || is_char_device(p);
  }

  posix::node_types
  file_type(const io::path_t &p) const
  {
    stat_t st{};
    return posix::get_type(p.c_str(), st);
  }

  bool
  same_file(const io::path_t &a, const io::path_t &b) const
  {
    return posix::is_same_file(a.c_str(), b.c_str());
  }

  posix::off64_t
  file_size(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return -1;
    return st.st_size;
  }

  posix::off64_t
  lsize(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::lstat(p.c_str(), st) != 0 ) return -1;
    return st.st_size;
  }

  time64_t
  mtime(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return 0;
    return st.st_mtime;
  }

  time64_t
  atime(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return 0;
    return st.st_atime;
  }

  time64_t
  ctime(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return 0;
    return st.st_ctime;
  }

  posix::ino64_t
  inode(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return 0;
    return st.st_ino;
  }

  posix::nlink_t
  hard_links(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return 0;
    return st.st_nlink;
  }

  posix::uid_t
  owner(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return static_cast<posix::uid_t>(-1);
    return st.st_uid;
  }

  posix::gid_t
  group(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return static_cast<posix::gid_t>(-1);
    return st.st_gid;
  }

  io::linux_permissions
  permissions(const io::path_t &p) const
  {
    stat_t st{};
    if ( posix::stat(p.c_str(), st) != 0 ) return io::linux_permissions::none();
    io::linux_permissions perms = io::linux_permissions::from_mode(st.st_mode);
    perms.setuid = (st.st_mode & posix::s_isuid) != 0;
    perms.setgid = (st.st_mode & posix::s_isgid) != 0;
    perms.sticky = (st.st_mode & posix::s_isvtx) != 0;
    return perms;
  }

  max_t
  set_permissions(const io::path_t &p, const io::linux_permissions &perms)
  {
    return posix::chmod(p.c_str(), perms.full_mode());
  }

  max_t
  chmod(const io::path_t &p, const io::linux_permissions &perms)
  {
    return set_permissions(p, perms);
  }

  max_t
  chown(const io::path_t &p, posix::uid_t uid, posix::gid_t gid)
  {
    return posix::chown(p.c_str(), uid, gid);
  }

  max_t
  lchown(const io::path_t &p, posix::uid_t uid, posix::gid_t gid)
  {
    return posix::lchown(p.c_str(), uid, gid);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // path mutations
  max_t
  rename(const io::path_t &from, const io::path_t &to)
  {
    __scope g(__mtx);
    micron::string kf = __key(from), kt = __key(to);
    __drop_recency(kf);
    __open.erase(kf);
    __drop_recency(kt);
    __open.erase(kt);
    return posix::rename(from.c_str(), to.c_str());
  }

  max_t
  move(const io::path_t &from, const io::path_t &to)
  {
    max_t r = rename(from, to);
    if ( r == 0 || -r != error::cross_device_link ) return r;
    // EXDEV: byte copy then unlink the source (non-atomic across filesystems)
    if ( max_t c = copy(from, to); c < 0 ) return c;
    return unlink(from);
  }

  // copy_file_range -> sendfile -> pread/pwrite loop
  max_t
  copy(const io::path_t &from, const io::path_t &to)
  {
    __scope g(__mtx);
    {
      micron::string kt = __key(to);
      __drop_recency(kt);
      __open.erase(kt);
    }
    posix::fd_t src{ static_cast<i32>(posix::open(from.c_str(), posix::o_rdonly | posix::o_cloexec)) };
    if ( !src ) return src.fd;
    stat_t st{};
    if ( posix::fstat(src, st) < 0 ) {      // was ignored -> dst could be created with mode 0
      posix::close(src.fd);
      return -error::io_error;
    }
    {
      stat_t dst_st{};      // refuse same-file copy: O_TRUNC would empty the shared inode before the read
      if ( posix::stat(to.c_str(), dst_st) == 0 && dst_st.st_dev == st.st_dev && dst_st.st_ino == st.st_ino ) {
        posix::close(src.fd);
        return -error::invalid_arg;
      }
    }
    posix::fd_t dst{ static_cast<i32>(
        posix::open(to.c_str(), posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec, st.st_mode & 0777u)) };
    if ( !dst ) {
      posix::close(src.fd);
      return dst.fd;
    }

    max_t total = 0;
    bool cfr_ok = true, sf_ok = true;
    for ( ;; ) {
      max_t n = -1;
      if ( cfr_ok ) {
        n = posix::copy_file_range(src.fd, nullptr, dst.fd, nullptr, 1u << 20, 0u);
        if ( n < 0 && (-n == error::cross_device_link || -n == error::invalid_arg || -n == error::bad_syscall) ) {
          cfr_ok = false;
          continue;
        }
      } else if ( sf_ok ) {
        n = posix::sendfile(dst, src, nullptr, 1u << 20);
        if ( n < 0 && (-n == error::invalid_arg || -n == error::bad_syscall) ) {
          sf_ok = false;
          continue;
        }
      } else {
        byte buf[65536];
        n = posix::read(src.fd, buf, sizeof(buf));
        if ( n > 0 ) {
          const usize want = static_cast<usize>(n);
          usize off = 0;
          while ( off < want ) {
            max_t w = posix::write(dst.fd, buf + off, want - off);
            if ( w < 0 ) {
              if ( -w == error::interrupted ) continue;
              n = w;
              break;
            }
            if ( w == 0 ) {      // no progress on a > 0 request: treat as I/O failure, never silent truncation
              n = -error::io_error;
              break;
            }
            off += static_cast<usize>(w);
          }
        }
      }
      if ( n < 0 && -n == error::interrupted ) continue;
      if ( n < 0 ) {
        posix::close(src.fd);
        posix::close(dst.fd);
        return n;
      }
      if ( n == 0 ) break;
      total += n;
    }
    posix::fsync(dst.fd);
    posix::close(src.fd);
    posix::close(dst.fd);
    return total;
  }

  template<typename... P>
  max_t
  copy_list(const io::path_t &from, P &&...to)
  {
    max_t total = 0;
    max_t rs[] = { copy(from, static_cast<P &&>(to))... };
    for ( max_t r : rs ) {
      if ( r < 0 ) return r;
      total += r;
    }
    return total;
  }

  max_t
  unlink(const io::path_t &p)
  {
    __scope g(__mtx);
    micron::string k = __key(p);
    __drop_recency(k);
    __open.erase(k);
    return posix::unlink(p.c_str());
  }

  max_t
  remove(const io::path_t &p)
  {
    return unlink(p);
  }

  max_t
  mkdir(const io::path_t &p, u32 mode = posix::mode_dir)
  {
    return posix::mkdir(p.c_str(), mode);
  }

  max_t
  mkdir_p(const io::path_t &p, u32 mode = posix::mode_dir)
  {
    return posix::mkdir_p(p.c_str(), mode);
  }

  max_t
  rmdir(const io::path_t &p)
  {
    return posix::rmdir(p.c_str());
  }

  max_t
  symlink(const io::path_t &target, const io::path_t &linkpath)
  {
    return posix::symlink(target.c_str(), linkpath.c_str());
  }

  max_t
  hardlink(const io::path_t &oldpath, const io::path_t &newpath)
  {
    return posix::link(oldpath.c_str(), newpath.c_str());
  }

  max_t
  readlink(const io::path_t &p, char *buf, usize cap) const
  {
    return posix::readlink(p.c_str(), buf, cap);
  }

  io::path_t
  readlink(const io::path_t &p) const
  {
    char buf[posix::path_max];
    max_t n = posix::readlink(p.c_str(), buf, sizeof(buf) - 1);
    if ( n < 0 ) return io::path_t();
    buf[n] = '\0';
    return io::path_t(buf);
  }

  max_t
  touch(const io::path_t &p)
  {
    if ( !posix::exists(p.c_str()) ) {
      posix::fd_t fd{ static_cast<i32>(
          posix::openat(posix::at_fdcwd, p.c_str(), posix::o_wronly | posix::o_create | posix::o_cloexec, posix::mode_file)) };
      if ( !fd ) return fd.fd;
      posix::close(fd.fd);
      return 0;
    }
    return posix::utimensat(posix::at_fdcwd, p.c_str(), nullptr, 0);      // both timestamps -> now
  }

  max_t
  utimes(const io::path_t &p, time64_t atime_s, time64_t mtime_s)
  {
    timespec_t ts[2]{};
    ts[0].tv_sec = atime_s;
    ts[1].tv_sec = mtime_s;
    return posix::utimensat(posix::at_fdcwd, p.c_str(), ts, 0);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // xattr passthroughs
  max_t
  xattr_get(const io::path_t &p, const char *name, void *value, usize sz) const
  {
    return posix::getxattr(p.c_str(), name, value, sz);
  }

  max_t
  xattr_set(const io::path_t &p, const char *name, const void *value, usize sz, i32 flags = 0)
  {
    return posix::setxattr(p.c_str(), name, value, sz, flags);
  }

  max_t
  xattr_list(const io::path_t &p, char *out, usize sz) const
  {
    return posix::listxattr(p.c_str(), out, sz);
  }

  max_t
  xattr_remove(const io::path_t &p, const char *name)
  {
    return posix::removexattr(p.c_str(), name);
  }

  io::file
  tmpfile(const io::path_t &dir)
  {
    i32 fd = static_cast<i32>(
        posix::openat(posix::at_fdcwd, dir.c_str(), posix::o_tmpfile | posix::o_rdwr | posix::o_cloexec, posix::mode_file));
    if ( fd < 0 )      // O_TMPFILE unsupported (common on tmpfs/overlay/network) or dir not writable: surface it
      exc<except::io_error>("micron::io::filesystem::tmpfile: O_TMPFILE open failed");
    return io::file(fd_t{ fd }, dir.c_str());
  }
};

template<io::modes DefaultMode = io::modes::read, usize Cap = 0> using filesystem = basic_filesystem<DefaultMode, micron::null_lock, Cap>;

template<io::modes DefaultMode = io::modes::read, usize Cap = 0>
using concurrent_filesystem = basic_filesystem<DefaultMode, micron::mutex, Cap>;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io::rooted_filesystem
// every operation resolves relative to one pinned directory fd via the *at syscall family

class rooted_filesystem
{
  micron::heap_swiss_map<io::path_t, __fs_entry> __open;
  fd_t __root;
  io::path_t __root_path;

  io::file &
  __get_or_open(const io::path_t &k, io::modes m, const open_opts &opts = {})
  {
    if ( __fs_entry *e = __open.find(k); e != nullptr ) {
      if ( !__mode_serviceable(e->mode, m) ) {
        i32 fd = static_cast<i32>(posix::openat(__root, k.c_str(), compose_open_flags(m, opts), opts.perms));
        if ( fd < 0 )      // reopen failed: surface it rather than silently returning a stale wrong-mode handle
          exc<except::filesystem_error>("micron::io::rooted_filesystem: reopen for requested mode failed");
        *e->fp = io::file(fd_t{ fd }, k.c_str());
        e->mode = m;
      }
      return *e->fp;
    }
    i32 fd = static_cast<i32>(posix::openat(__root, k.c_str(), compose_open_flags(m, opts), opts.perms));
    if ( fd < 0 )      // do NOT cache a failed open: a poisoned entry denies the path permanently even after it appears
      exc<except::filesystem_error>("micron::io::rooted_filesystem: openat failed");
    auto ins = __open.emplace(k, __fs_entry{ micron::unique_pointer<io::file>(fd_t{ fd }, k.c_str()), m });
    return *ins.b->fp;
  }

public:
  ~rooted_filesystem()
  {
    __open.for_each([](const io::path_t &, __fs_entry &e) {
      if ( e.fp ) e.fp->flush_data();
    });
    if ( __root.fd >= 0 ) posix::close(__root.fd);
  }

  rooted_filesystem() = delete;

  explicit rooted_filesystem(const io::path_t &root) : __open(), __root(posix::invalid_fd), __root_path(io::prune(root))
  {
    i32 fd = static_cast<i32>(posix::open(__root_path.c_str(), posix::o_path | posix::o_directory | posix::o_cloexec));
    if ( fd < 0 ) exc<except::filesystem_error>("micron::io::rooted_filesystem: cannot open root directory");
    __root.fd = fd;
  }

  rooted_filesystem(const rooted_filesystem &) = delete;
  rooted_filesystem &operator=(const rooted_filesystem &) = delete;

  rooted_filesystem(rooted_filesystem &&o) noexcept
      : __open(micron::move(o.__open)), __root(o.__root), __root_path(micron::move(o.__root_path))
  {
    o.__root = posix::invalid_fd;
  }

  rooted_filesystem &
  operator=(rooted_filesystem &&o) noexcept
  {
    if ( this == &o ) return *this;
    __open.clear();
    if ( __root.fd >= 0 ) posix::close(__root.fd);
    __open = micron::move(o.__open);
    __root = o.__root;
    __root_path = micron::move(o.__root_path);
    o.__root = posix::invalid_fd;
    return *this;
  }

  const io::path_t &
  root(void) const
  {
    return __root_path;
  }

  fd_t
  root_fd(void) const
  {
    return __root;
  }

  io::file &
  operator[](const io::path_t &rel, const io::modes m = io::modes::read)
  {
    return __get_or_open(io::prune(rel), m);
  }

  io::file &
  open(const io::path_t &rel, const io::modes m = io::modes::read, const open_opts &opts = {})
  {
    return __get_or_open(io::prune(rel), m, opts);
  }

  bool
  is_open(const io::path_t &rel) const
  {
    return __open.contains(io::prune(rel));
  }

  void
  close(const io::path_t &rel)
  {
    io::path_t k = io::prune(rel);
    // flush before dropping
    if ( __fs_entry *e = __open.find(k); e != nullptr && e->fp ) e->fp->flush_data();
    __open.erase(k);
  }

  void
  clear(void)
  {
    __open.for_each([](const io::path_t &, __fs_entry &e) {
      if ( e.fp ) e.fp->flush_data();
    });
    __open.clear();
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // *at-based queries
  bool
  exists(const io::path_t &rel) const
  {
    return posix::faccessat(__root.fd, rel.c_str(), posix::access_ok, 0) == 0;
  }

  posix::node_types
  file_type(const io::path_t &rel) const
  {
    stat_t st{};
    if ( posix::fstatat(__root, rel.c_str(), st, 0) != 0 ) return posix::node_types::not_found;
    return posix::get_type(st);
  }

  bool
  is_regular_file(const io::path_t &rel) const
  {
    return file_type(rel) == posix::node_types::regular_file;
  }

  bool
  is_directory(const io::path_t &rel) const
  {
    return file_type(rel) == posix::node_types::directory;
  }

  bool
  is_symlink(const io::path_t &rel) const
  {
    stat_t st{};
    if ( posix::fstatat(__root, rel.c_str(), st, posix::at_symlink_nofollow) != 0 ) return false;
    return posix::__impl::stat_is_lnk(st);
  }

  posix::off64_t
  file_size(const io::path_t &rel) const
  {
    stat_t st{};
    if ( posix::fstatat(__root, rel.c_str(), st, 0) != 0 ) return -1;
    return st.st_size;
  }

  time64_t
  mtime(const io::path_t &rel) const
  {
    stat_t st{};
    if ( posix::fstatat(__root, rel.c_str(), st, 0) != 0 ) return 0;
    return st.st_mtime;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // *at-based mutations
  max_t
  rename(const io::path_t &from, const io::path_t &to)
  {
    __open.erase(io::prune(from));
    __open.erase(io::prune(to));
    return posix::renameat(__root.fd, from.c_str(), __root.fd, to.c_str());
  }

  max_t
  unlink(const io::path_t &rel)
  {
    __open.erase(io::prune(rel));
    return posix::unlinkat(__root.fd, rel.c_str(), 0);
  }

  max_t
  mkdir(const io::path_t &rel, u32 mode = posix::mode_dir)
  {
    return posix::mkdirat(__root.fd, rel.c_str(), mode);
  }

  max_t
  rmdir(const io::path_t &rel)
  {
    return posix::unlinkat(__root.fd, rel.c_str(), posix::at_removedir);
  }

  max_t
  symlink(const io::path_t &target, const io::path_t &linkpath)
  {
    return posix::symlinkat(target.c_str(), __root.fd, linkpath.c_str());
  }

  max_t
  hardlink(const io::path_t &oldpath, const io::path_t &newpath)
  {
    return posix::linkat(__root.fd, oldpath.c_str(), __root.fd, newpath.c_str(), 0);
  }

  max_t
  readlink(const io::path_t &rel, char *buf, usize cap) const
  {
    return posix::readlinkat(__root.fd, rel.c_str(), buf, cap);
  }

  max_t
  chmod(const io::path_t &rel, const io::linux_permissions &perms)
  {
    return posix::fchmodat(__root.fd, rel.c_str(), perms.full_mode(), 0);
  }

  max_t
  chown(const io::path_t &rel, posix::uid_t uid, posix::gid_t gid)
  {
    return posix::fchownat(__root.fd, rel.c_str(), uid, gid, 0);
  }

  max_t
  touch(const io::path_t &rel)
  {
    if ( !exists(rel) ) {
      i32 fd = static_cast<i32>(posix::openat(__root, rel.c_str(), posix::o_wronly | posix::o_create | posix::o_cloexec, posix::mode_file));
      if ( fd < 0 ) return fd;
      posix::close(fd);
      return 0;
    }
    return posix::utimensat(__root.fd, rel.c_str(), nullptr, 0);
  }

  io::file
  tmpfile(void)
  {
    i32 fd = static_cast<i32>(posix::openat(__root, ".", posix::o_tmpfile | posix::o_rdwr | posix::o_cloexec, posix::mode_file));
    if ( fd < 0 )      // O_TMPFILE unsupported or root not writable: surface it, don't adopt a buried errno
      exc<except::io_error>("micron::io::rooted_filesystem::tmpfile: O_TMPFILE open failed");
    return io::file(fd_t{ fd }, __root_path.c_str());
  }
};

};      // namespace io

};      // namespace micron
