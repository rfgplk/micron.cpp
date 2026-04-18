//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "iosys.hpp"

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../memory_block.hpp"
#include "../../string/strings.hpp"
#include "../../types.hpp"

#include "../../linux/io.hpp"
#include "../../linux/io/inode.hpp"
#include "../../linux/sys/fcntl.hpp"
#include "../../linux/sys/ioctl.hpp"
#include "../bits.hpp"

#include "file.hpp"

namespace micron
{
namespace io
{
// NOTE: implementing this in here instead of linux/sys
inline constexpr u32 blkroset = 0x125dU;             // set read-only flag
inline constexpr u32 blkroget = 0x125eU;             // get read-only flag
inline constexpr u32 blkrrpart = 0x125fU;            // re-read partition table
inline constexpr u32 blkgetsize = 0x1260U;           // device size in 512-B sectors
inline constexpr u32 blkflsbuf = 0x1261U;            // flush buffer cache
inline constexpr u32 blkraset = 0x1262U;             // set readahead
inline constexpr u32 blkraget = 0x1263U;             // get readahead
inline constexpr u32 blksszget = 0x1268U;            // logical block size
inline constexpr u32 blkbszget = 0x80081270U;        // physical block size
inline constexpr u32 blkbszset = 0x40081271U;        // set physical block size
inline constexpr u32 blkgetsize64 = 0x80081272U;     // device size in bytes
inline constexpr u32 blkdiscard = 0x1277U;           // discard sectors
inline constexpr u32 blkiomin = 0x1278U;             // min I/O size
inline constexpr u32 blkioopt = 0x1279U;             // optimal I/O size
inline constexpr u32 blkalignoff = 0x127aU;          // alignment offset
inline constexpr u32 blkpbszget = 0x127bU;           // physical block size
inline constexpr u32 blkdiscardzero = 0x127cU;       // discard zeroes data
inline constexpr u32 blksecdiscard = 0x127dU;        // secure discard
inline constexpr u32 blkrotational = 0x127eU;        // rotational flag
inline constexpr u32 blkzeroout = 0x127fU;           // zero out range
inline constexpr u32 blksectget = 0x1267U;           // max sectors/req

struct block_info_t {
  u64 size_bytes;              // BLKGETSIZE64
  u64 size_sectors;            // BLKGETSIZE   (sectors)
  u32 logical_block_size;      // BLKSSZGET    (smallest unit)
  u32 physical_block_size;     // BLKPBSZGET   (physical smallest unit)
  u32 min_io_size;             // BLKIOMIN
  u32 optimal_io_size;         // BLKIOOPT
  i32 alignment_offset;        // BLKALIGNOFF
  u16 max_sectors;             // BLKSECTGET
  u64 readahead_sectors;       // BLKRAGET
  bool read_only;              // BLKROGET
  bool rotational;             // BLKROTATIONAL (does it physically spin, ie hdd or solid state)
  bool discard_zeroes;         // BLKDISCARDZEROES (ssd)
};

struct block : public file {

  static constexpr usize default_buf_size = 65536u;     // 64 KB

  micron::buffer _buf;
  posix::off_t _buf_off;
  usize _buf_valid;
  bool _buf_dirty;

  mutable block_info_t _info;
  mutable bool _info_valid;

  ~block(void)
  {
    if ( _buf_dirty ) flush_buf();
  }

  block() = delete;

  explicit block(const char *path, usize buf_sz = default_buf_size, modes m = modes::read)
      : file(), _buf(buf_sz), _buf_off(0), _buf_valid(0), _buf_dirty(false), _info{}, _info_valid(false)
  {
    __open_block(path, m);
  }

  template <is_string T>
  explicit block(const T &path, usize buf_sz = default_buf_size, modes m = modes::read) : block(path.c_str(), buf_sz, m)
  {
  }

  block(const block &o)
      : file(o), _buf(o._buf), _buf_off(o._buf_off), _buf_valid(o._buf_valid), _buf_dirty(false), _info(o._info), _info_valid(o._info_valid)
  {
  }

  block(block &&o) noexcept
      : file(micron::move(o)), _buf(micron::move(o._buf)), _buf_off(o._buf_off), _buf_valid(o._buf_valid), _buf_dirty(o._buf_dirty),
        _info(o._info), _info_valid(o._info_valid)
  {
    o._buf_off = 0;
    o._buf_valid = 0;
    o._buf_dirty = false;
    o._info_valid = false;
  }

  block &
  operator=(block &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( _buf_dirty ) flush_buf();
    file::operator=(micron::move(o));
    _buf = micron::move(o._buf);
    _buf_off = o._buf_off;
    _buf_valid = o._buf_valid;
    _buf_dirty = o._buf_dirty;
    _info = o._info;
    _info_valid = o._info_valid;
    o._buf_off = 0;
    o._buf_valid = 0;
    o._buf_dirty = false;
    o._info_valid = false;
    return *this;
  }

  block &
  operator=(const block &o)
  {
    if ( this == &o ) return *this;
    if ( _buf_dirty ) flush_buf();
    file::operator=(o);
    _buf = o._buf;
    _buf_off = o._buf_off;
    _buf_valid = o._buf_valid;
    _buf_dirty = false;
    _info = o._info;
    _info_valid = o._info_valid;
    return *this;
  }

  template <typename Arg>
  i32
  ioctl_t(u32 req, Arg *arg) const
  {
    alive_c();
    return static_cast<i32>(posix::ioctl(__handle.fd, req, arg));
  }

  i32
  ioctl_t(u32 req) const
  {
    alive_c();
    return static_cast<i32>(posix::ioctl(__handle.fd, req));
  }

  const micron::buffer &
  view(void) const noexcept
  {
    return _buf;
  }

  micron::buffer &
  view() noexcept
  {
    return _buf;
  }

  posix::off_t
  view_offset(void) const noexcept
  {
    return _buf_off;
  }

  usize
  view_valid(void) const noexcept
  {
    return _buf_valid;
  }

  usize
  view_capacity(void) const noexcept
  {
    return _buf.size();
  }

  bool
  view_dirty(void) const noexcept
  {
    return _buf_dirty;
  }

  void
  invalidate_buf() noexcept
  {
    _buf_valid = 0;
    _buf_dirty = false;
  }

  void
  resize_buf(usize new_sz)
  {
    if ( _buf_dirty ) flush_buf();
    _buf.recreate(new_sz);
    invalidate_buf();
  }

  max_t
  fill(posix::off_t dev_offset = 0)
  {
    __alive();
    if ( _buf_dirty ) flush_buf();
    max_t n = posix::pread(__handle.fd, _buf.begin(), _buf.size(), dev_offset);
    if ( n > 0 ) {
      _buf_off = dev_offset;
      _buf_valid = static_cast<usize>(n);
    } else {
      _buf_valid = 0;
    }
    return n;
  }

  max_t
  flush_buf(void)
  {
    if ( !_buf_dirty || _buf_valid == 0 ) return 0;
    __alive();
    max_t n = posix::pwrite(__handle.fd, _buf.begin(), _buf_valid, _buf_off);
    if ( n >= 0 ) _buf_dirty = false;
    return n;
  }

  bool
  buf_covers(posix::off_t offset, usize size) const noexcept
  {
    if ( _buf_valid == 0 ) return false;
    return offset >= _buf_off && (static_cast<u64>(offset) + size) <= (static_cast<u64>(_buf_off) + _buf_valid);
  }

  max_t
  write_buf(const void *src, usize size, posix::off_t dev_offset)
  {
    if ( !buf_covers(dev_offset, size) ) return -1;
    usize delta = static_cast<usize>(dev_offset - _buf_off);
    micron::memcpy(_buf.begin() + delta, static_cast<const byte *>(src), size);
    _buf_dirty = true;
    return static_cast<max_t>(size);
  }

  max_t
  read_buffered(void *dst, usize size, posix::off_t offset)
  {
    __alive();
    if ( buf_covers(offset, size) ) {
      usize delta = static_cast<usize>(offset - _buf_off);
      micron::memcpy(static_cast<byte *>(dst), _buf.begin() + delta, size);
      return static_cast<max_t>(size);
    }
    max_t n = fill(offset);
    if ( n <= 0 ) return n;
    usize copy = static_cast<usize>(n) < size ? static_cast<usize>(n) : size;
    micron::memcpy(static_cast<byte *>(dst), _buf.begin(), copy);
    return static_cast<max_t>(copy);
  }

  max_t
  read_block(u64 block_no)
  {
    __alive();
    posix::off_t off = __block_off(block_no);
    return fill(off);
  }

  max_t
  read_blocks(u64 block_no, u32 count)
  {
    __alive();
    u32 bsz = __bsz();
    usize total = static_cast<usize>(count) * static_cast<usize>(bsz);
    if ( _buf.size() < total ) resize_buf(total);
    return fill(__block_off(block_no));
  }

  max_t
  read_into(void *dst, usize size, posix::off_t offset) const
  {
    alive_c();
    return posix::pread(__handle.fd, dst, size, offset);
  }

  max_t
  read_block_into(void *dst, u64 block_no) const
  {
    alive_c();
    u32 bsz = __bsz();
    return posix::pread(__handle.fd, dst, bsz, __block_off(block_no));
  }

  max_t
  read_blocks_into(void *dst, u64 block_no, u32 count) const
  {
    alive_c();
    u32 bsz = __bsz();
    return posix::pread(__handle.fd, dst, static_cast<usize>(count) * bsz, __block_off(block_no));
  }

  max_t
  read_all_into(void *dst) const
  {
    alive_c();
    return posix::read_all(__handle, dst, static_cast<usize>(device_size()));
  }

  max_t
  write_at(const void *src, usize size, posix::off_t offset)
  {
    __alive();
    return posix::pwrite(__handle.fd, src, size, offset);
  }

  max_t
  write_block_from(const void *src, usize size, u64 block_no)
  {
    __alive();
    return posix::pwrite(__handle.fd, src, size, __block_off(block_no));
  }

  max_t
  write_block(u64 block_no)
  {
    __alive();
    posix::off_t off = __block_off(block_no);
    max_t n = posix::pwrite(__handle.fd, _buf.begin(), _buf_valid, off);
    if ( n >= 0 ) {
      _buf_off = off;
      _buf_dirty = false;
    }
    return n;
  }

  i32
  commit(void)
  {
    return static_cast<i32>(flush_buf());
  }

  template <typename T>
  max_t
  readv_into(posix::iovec_t<T> &iov, i32 count) const
  {
    alive_c();
    return posix::readv(__handle, iov, count);
  }

  template <typename T>
  max_t
  writev_from(const posix::iovec_t<T> &iov, i32 count)
  {
    __alive();
    return posix::writev(__handle, iov, count);
  }

  void
  query_info(void) const
  {
    alive_c();
    u64 sz64 = 0;
    u64 szsec = 0;
    i32 lbsz = 512;
    u32 pbsz = 512;
    u32 iomin = 0;
    u32 ioopt = 0;
    i32 alignoff = 0;
    u16 maxsect = 0;
    u64 ra = 0;
    i32 ro = 0;
    u16 rotflag = 0;
    u32 dzflag = 0;

    posix::ioctl(__handle.fd, blkgetsize64, &sz64);
    posix::ioctl(__handle.fd, blkgetsize, &szsec);
    posix::ioctl(__handle.fd, blksszget, &lbsz);
    posix::ioctl(__handle.fd, blkpbszget, &pbsz);
    posix::ioctl(__handle.fd, blkiomin, &iomin);
    posix::ioctl(__handle.fd, blkioopt, &ioopt);
    posix::ioctl(__handle.fd, blkalignoff, &alignoff);
    posix::ioctl(__handle.fd, blksectget, &maxsect);
    posix::ioctl(__handle.fd, blkraget, &ra);
    posix::ioctl(__handle.fd, blkroget, &ro);
    posix::ioctl(__handle.fd, blkrotational, &rotflag);
    posix::ioctl(__handle.fd, blkdiscardzero, &dzflag);

    _info.size_bytes = sz64;
    _info.size_sectors = szsec;
    _info.logical_block_size = static_cast<u32>(lbsz > 0 ? lbsz : 512);
    _info.physical_block_size = pbsz ? pbsz : _info.logical_block_size;
    _info.min_io_size = iomin ? iomin : _info.logical_block_size;
    _info.optimal_io_size = ioopt ? ioopt : _info.min_io_size;
    _info.alignment_offset = alignoff;
    _info.max_sectors = maxsect;
    _info.readahead_sectors = ra;
    _info.read_only = (ro != 0);
    _info.rotational = (rotflag != 0);
    _info.discard_zeroes = (dzflag != 0);
    _info_valid = true;
  }

  void
  invalidate_info(void) const noexcept
  {
    _info_valid = false;
  }

  const block_info_t &
  info(void) const
  {
    if ( !_info_valid ) query_info();
    return _info;
  }

  u64
  device_size(void) const
  {
    return info().size_bytes;
  }

  u64
  sector_count(void) const
  {
    return info().size_sectors;
  }

  u32
  block_size(void) const
  {
    return info().logical_block_size;
  }

  u32
  physical_block_size(void) const
  {
    return info().physical_block_size;
  }

  u32
  min_io_size(void) const
  {
    return info().min_io_size;
  }

  u32
  optimal_io_size(void) const
  {
    return info().optimal_io_size;
  }

  i32
  alignment_offset(void) const
  {
    return info().alignment_offset;
  }

  u16
  max_sectors(void) const
  {
    return info().max_sectors;
  }

  bool
  is_read_only(void) const
  {
    return info().read_only;
  }

  bool
  is_rotational(void) const
  {
    return info().rotational;
  }

  bool
  discard_zeroes(void) const
  {
    return info().discard_zeroes;
  }

  u64
  block_count(void) const
  {
    u32 bsz = block_size();
    return bsz ? device_size() / bsz : 0;
  }

  posix::off_t
  block_offset(u64 n) const
  {
    return static_cast<posix::off_t>(n) * static_cast<posix::off_t>(block_size());
  }

  bool
  is_aligned(posix::off_t offset) const
  {
    u32 bsz = block_size();
    return bsz && (static_cast<u64>(offset) % bsz) == 0;
  }

  bool
  in_range(posix::off_t offset) const
  {
    return offset >= 0 && static_cast<u64>(offset) < device_size();
  }

  bool
  in_range(posix::off_t offset, u64 len) const
  {
    return offset >= 0 && (static_cast<u64>(offset) + len) <= device_size();
  }

  u64
  get_readahead(void) const
  {
    alive_c();
    u64 ra = 0;
    posix::ioctl(__handle.fd, blkraget, &ra);
    return ra;
  }

  i32
  set_readahead(u64 sectors)
  {
    __alive();
    i32 r = static_cast<i32>(posix::ioctl(__handle.fd, blkraset, sectors));
    if ( r == 0 && _info_valid ) _info.readahead_sectors = sectors;
    return r;
  }

  max_t
  do_readahead(posix::off_t offset, usize count) const
  {
    alive_c();
    return micron::syscall(SYS_readahead, __handle.fd, static_cast<posix::off_t>(offset), count);
  }

  max_t
  readahead_blocks(u64 first_block, u32 count) const
  {
    alive_c();
    u32 bsz = __bsz();
    usize total = static_cast<usize>(count) * static_cast<usize>(bsz);
    return do_readahead(__block_off(first_block), total);
  }

  i32
  fadvise(i32 advice) const
  {
    return posix::fadvise(__handle.fd, 0, 0, advice);
  }

  i32
  advise_sequential(void) const
  {
    return fadvise(posix::fadv_sequential);
  }

  i32
  advise_random(void) const
  {
    return fadvise(posix::fadv_random);
  }

  i32
  advise_normal(void) const
  {
    return fadvise(posix::fadv_normal);
  }

  i32
  advise_willneed(posix::off_t offset, posix::off_t len) const
  {
    return posix::fadvise(__handle.fd, offset, len, posix::fadv_willneed);
  }

  i32
  advise_dontneed(posix::off_t offset, posix::off_t len) const
  {
    return posix::fadvise(__handle.fd, offset, len, posix::fadv_dontneed);
  }

  i32
  advise_noreuse(posix::off_t offset, posix::off_t len) const
  {
    return posix::fadvise(__handle.fd, offset, len, posix::fadv_noreuse);
  }

  i32
  advise_willneed_block(u64 first_block, u32 count) const
  {
    return posix::fadvise(__handle.fd, __block_off(first_block), static_cast<posix::off_t>(static_cast<usize>(count) * __bsz()),
                          posix::fadv_willneed);
  }

  i32
  advise_dontneed_block(u64 first_block, u32 count) const
  {
    return posix::fadvise(__handle.fd, __block_off(first_block), static_cast<posix::off_t>(static_cast<usize>(count) * __bsz()),
                          posix::fadv_dontneed);
  }

  i32
  discard(posix::off_t offset, u64 len)
  {
    __alive();
    u64 range[2] = { static_cast<u64>(offset), len };
    return static_cast<i32>(posix::ioctl(__handle.fd, blkdiscard, range));
  }

  i32
  secure_discard(posix::off_t offset, u64 len)
  {
    __alive();
    u64 range[2] = { static_cast<u64>(offset), len };
    return static_cast<i32>(posix::ioctl(__handle.fd, blksecdiscard, range));
  }

  i32
  zero_out(posix::off_t offset, u64 len)
  {
    __alive();
    u64 range[2] = { static_cast<u64>(offset), len };
    return static_cast<i32>(posix::ioctl(__handle.fd, blkzeroout, range));
  }

  i32
  discard_block(u64 n)
  {
    return discard(block_offset(n), __bsz());
  }

  i32
  discard_blocks(u64 n, u32 c)
  {
    return discard(block_offset(n), static_cast<u64>(c) * __bsz());
  }

  i32
  zero_block(u64 n)
  {
    return zero_out(block_offset(n), __bsz());
  }

  i32
  zero_blocks(u64 n, u32 c)
  {
    return zero_out(block_offset(n), static_cast<u64>(c) * __bsz());
  }

  i32
  flush_cache(void)
  {
    __alive();
    return static_cast<i32>(posix::ioctl(__handle.fd, blkflsbuf));
  }

  i32
  reread_partitions(void)
  {
    __alive();
    invalidate_info();
    return static_cast<i32>(posix::ioctl(__handle.fd, blkrrpart));
  }

  i32
  set_read_only(bool on = true)
  {
    __alive();
    i32 val = on ? 1 : 0;
    i32 r = static_cast<i32>(posix::ioctl(__handle.fd, blkroset, &val));
    if ( r == 0 && _info_valid ) _info.read_only = on;
    return r;
  }

  i32
  flush_data(void)
  {
    __alive();
    return static_cast<i32>(posix::fdatasync(__handle.fd));
  }

  i32
  fsync(void)
  {
    __alive();
    return static_cast<i32>(posix::fsync(__handle.fd));
  }

  posix::off_t
  seek_to_block(u64 n)
  {
    __alive();
    return posix::seek_to(__handle, block_offset(n));
  }

  u64
  current_block(void) const
  {
    alive_c();
    posix::off_t pos = posix::tell(__handle);
    u32 bsz = __bsz();
    return bsz ? static_cast<u64>(pos) / static_cast<u64>(bsz) : 0;
  }

  max_t
  advance(void)
  {
    __alive();
    posix::off_t next = _buf_off + static_cast<posix::off_t>(_buf_valid ? _buf_valid : _buf.size());
    if ( static_cast<u64>(next) >= device_size() ) return 0;
    return fill(next);
  }

  void
  rewind_view() noexcept
  {
    invalidate_buf();
    _buf_off = 0;
  }

  bool
  at_end(void) const
  {
    if ( _buf_valid == 0 ) return true;
    return (static_cast<u64>(_buf_off) + _buf_valid) >= device_size();
  }

  static block
  open_ro(const char *path, usize buf_sz = default_buf_size)
  {
    return block{ path, buf_sz, modes::read };
  }

  static block
  open_rw(const char *path, usize buf_sz = default_buf_size)
  {
    return block{ path, buf_sz, modes::readwrite };
  }

  static block
  open_direct(const char *path, usize buf_sz = default_buf_size)
  {
    return block{ path, buf_sz, modes::largeread };
  }

  template <is_string T>
  static block
  open_ro(const T &p, usize b = default_buf_size)
  {
    return open_ro(p.c_str(), b);
  }

  template <is_string T>
  static block
  open_rw(const T &p, usize b = default_buf_size)
  {
    return open_rw(p.c_str(), b);
  }

  template <is_string T>
  static block
  open_direct(const T &p, usize b = default_buf_size)
  {
    return open_direct(p.c_str(), b);
  }

private:
  inline void
  alive_c(void) const
  {
    if ( __handle.fd == -1 ) exc<except::io_error>("micron::block, fd isn't open.");
  }

  u32
  __bsz(void) const
  {
    if ( !_info_valid ) query_info();
    return _info.logical_block_size ? _info.logical_block_size : 512u;
  }

  posix::off_t
  __block_off(u64 n) const
  {
    return static_cast<posix::off_t>(n) * static_cast<posix::off_t>(__bsz());
  }

  void
  __open_block(const char *str, const modes mode)
  {
    if ( !posix::verify(str) ) exc<except::io_error>("micron::block, malformed path.");
    if ( !posix::exists(str) ) exc<except::io_error>("micron::block, path doesn't exist.");
    if ( !posix::is_block_device(str) ) exc<except::io_error>("micron::block, path is not a block device.");

    i32 flags = 0;
    switch ( mode ) {
    case modes::read :
    case modes::quiet :
      flags = posix::o_rdonly | posix::o_cloexec;
      break;
    case modes::largeread :
      flags = posix::o_rdonly | posix::o_direct | posix::o_largefile | posix::o_cloexec;
      break;
    case modes::large :
      flags = posix::o_rdwr | posix::o_direct | posix::o_largefile | posix::o_cloexec;
      break;
    case modes::readwrite :
      flags = posix::o_rdwr | posix::o_sync | posix::o_cloexec;
      break;
    default :
      flags = posix::o_rdonly | posix::o_cloexec;
      break;
    }

    __handle.fd = static_cast<i32>(posix::open(str, flags));
    if ( __handle.has_error() ) exc<except::io_error>("micron::block, failed to open device.");

    fname = str;
    micron::zero(&sd);
  }
};

inline block
open_block_ro(const char *path, usize buf_sz = block::default_buf_size)
{
  return block::open_ro(path, buf_sz);
}

inline block
open_block_rw(const char *path, usize buf_sz = block::default_buf_size)
{
  return block::open_rw(path, buf_sz);
}

inline block
open_block_direct(const char *path, usize buf_sz = block::default_buf_size)
{
  return block::open_direct(path, buf_sz);
}

template <is_string T>
inline block
open_block_ro(const T &path, usize buf_sz = block::default_buf_size)
{
  return block::open_ro(path.c_str(), buf_sz);
}

template <is_string T>
inline block
open_block_rw(const T &path, usize buf_sz = block::default_buf_size)
{
  return block::open_rw(path.c_str(), buf_sz);
}

template <is_string T>
inline block
open_block_direct(const T &path, usize buf_sz = block::default_buf_size)
{
  return block::open_direct(path.c_str(), buf_sz);
}

};     // namespace io
};     // namespace micron
