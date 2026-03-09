//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../string/strings.hpp"
#include "io.hpp"

#include "paths.hpp"
#include "posix/file.hpp"

#include "bin.hpp"
#include "stream.hpp"

#include "../math/generic.hpp"

#include "console.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// high level, platform independent file abstraction

namespace micron
{
namespace fsys
{

template <is_string T = micron::string> class file : public io::file
{
  T data;
  micron::sstr<io::max_name> fname;
  usize seek;
  usize buffer_sz;
  micron::shared<micron::buffer> bf;

  void
  __need_fd(const char *where) const
  {
    if ( __handle.closed() )
      exc<except::filesystem_error>(where);
  }

  max_t
  __pread(void *dst, usize len, posix::off_t off) const
  {
    return posix::pread(__handle.fd, dst, len, off);
  }

  max_t
  __pread_exact(void *dst, usize len, posix::off_t off) const
  {
    usize got = 0;
    byte *p = static_cast<byte *>(dst);
    while ( got < len ) {
      max_t r = posix::pread(__handle.fd, p + got, len - got, off + static_cast<posix::off_t>(got));
      if ( r <= 0 )
        return r == 0 ? static_cast<max_t>(got) : r;
      got += static_cast<usize>(r);
    }
    return static_cast<max_t>(got);
  }

  static constexpr usize __kmp_stack_limit = 1024;

  static void
  __kmp_table(const byte *pat, usize plen, usize *tbl)
  {
    if ( plen == 0 )
      return;
    tbl[0] = 0;
    usize k = 0;
    for ( usize i = 1; i < plen; ) {
      if ( pat[i] == pat[k] ) {
        tbl[i] = k + 1;
        ++i;
        ++k;
      } else if ( k ) {
        k = tbl[k - 1];
      } else {
        tbl[i] = 0;
        ++i;
      }
    }
  }

  static usize
  __kmp_search(const byte *hay, usize hlen, const byte *pat, usize plen, const usize *tbl)
  {
    if ( plen == 0 )
      return 0;
    if ( plen > hlen )
      return hlen;
    usize k = 0;
    for ( usize i = 0; i < hlen; ) {
      if ( hay[i] == pat[k] ) {
        ++k;
        ++i;
        if ( k == plen )
          return i - k;
      } else if ( k ) {
        k = tbl[k - 1];
      } else {
        ++i;
      }
    }
    return hlen;
  }

public:
  ~file() = default;

  file() : io::file(), data(), fname(), seek(0), buffer_sz(0), bf(nullptr) {}

  file(const T &name, const io::modes mode)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append || mode == io::modes::appendread ? size() : 0),
        buffer_sz(0), bf(nullptr)
  {
  }

  file(const char *name, const io::modes mode)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append || mode == io::modes::appendread ? size() : 0),
        buffer_sz(0), bf(nullptr)
  {
  }

  template <usize M>
  file(const char (&name)[M], const io::modes mode)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append || mode == io::modes::appendread ? size() : 0),
        buffer_sz(0), bf(nullptr)
  {
  }

  file(const T &name, const io::modes mode, const usize _bf)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append || mode == io::modes::appendread ? size() : 0),
        buffer_sz(_bf), bf(new micron::buffer(_bf))
  {
  }

  file(const char *name, const io::modes mode, const usize _bf)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append || mode == io::modes::appendread ? size() : 0),
        buffer_sz(_bf), bf(new micron::buffer(_bf))
  {
  }

  template <usize M>
  file(const char (&name)[M], const io::modes mode, const usize _bf)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append || mode == io::modes::appendread ? size() : 0),
        buffer_sz(_bf), bf(new micron::buffer(_bf))
  {
  }

  file(const T &name) : io::file(name, io::modes::read), data(), fname(name), seek(0), buffer_sz(0), bf(nullptr) {}

  file(const char *name) : io::file(name, io::modes::read), data(), fname(name), seek(0), buffer_sz(0), bf(nullptr) {}

  template <usize M> file(const char (&name)[M]) : io::file(name, io::modes::read), data(), fname(name), seek(0), buffer_sz(0), bf(nullptr)
  {
  }

  void
  reopen(const T &name, const io::modes mode, const usize _bf)
  {
    sync();
    close();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append || mode == io::modes::appendread ? size() : 0);
    buffer_sz = _bf;
    bf = micron::shared<micron::buffer>(new micron::buffer(_bf));
  }

  void
  reopen(const char *name, const io::modes mode, const usize _bf)
  {
    sync();
    close();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append || mode == io::modes::appendread ? size() : 0);
    buffer_sz = _bf;
    bf = micron::shared<micron::buffer>(new micron::buffer(_bf));
  }

  void
  reopen(const T &name, const io::modes mode)
  {
    sync();
    close();
    data.clear();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append || mode == io::modes::appendread ? size() : 0);
    buffer_sz = 0;
    bf = nullptr;
  }

  void
  reopen(const char *name, const io::modes mode)
  {
    sync();
    close();
    data.clear();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append || mode == io::modes::appendread ? size() : 0);
    buffer_sz = 0;
    bf = nullptr;
  }

  void
  reopen(const T &name)
  {
    sync();
    close();
    data.clear();
    io::file::operator=(micron::move(io::file(name, io::modes::read)));
    fname = name;
    seek = 0;
    buffer_sz = 0;
    bf = nullptr;
  }

  void
  reopen(const char *name)
  {
    sync();
    close();
    data.clear();
    io::file::operator=(micron::move(io::file(name, io::modes::read)));
    fname = name;
    seek = 0;
    buffer_sz = 0;
    bf = nullptr;
  }

  void
  clear(void)
  {
    data.clear();
    data._set_buf_length(0);
  }

  file(const file &o) : io::file(o), data(o.data), fname(o.fname), seek(o.seek), buffer_sz(o.buffer_sz), bf(o.bf) {}

  file(file &&o)
      : io::file(micron::move(o)), data(micron::move(o.data)), fname(micron::move(o.fname)), seek(o.seek), buffer_sz(o.buffer_sz), bf(o.bf)
  {
    o.seek = 0;
    o.buffer_sz = 0;
    o.bf = nullptr;
  }

  bool
  operator==(const file &o)
  {
    return __handle.fd == o.__handle.fd;
  }

  file &
  operator=(const file &o)
  {
    io::file::operator=(o);
    data = o.data;
    fname = o.fname;
    seek = o.seek;
    buffer_sz = o.buffer_sz;
    bf = o.bf;
    return *this;
  }

  file &
  operator=(file &&o)
  {
    io::file::operator=(micron::move(o));
    data = micron::move(o.data);
    fname = micron::move(o.fname);
    seek = o.seek;
    buffer_sz = o.buffer_sz;
    bf = o.bf;
    o.seek = 0;
    o.buffer_sz = 0;
    o.bf = nullptr;
    return *this;
  }

  file &
  operator>>(T &str)
  {
    str = load_and_pull();
    return *this;
  }

  file &
  operator<<(T &&str)
  {
    push(str);
    write();
    return *this;
  }

  file &
  operator<<(const T &str)
  {
    push_copy(str);
    write();
    return *this;
  }

  file &
  operator=(T &&str)
  {
    push(str);
    write();
    return *this;
  }

  file &
  operator=(const T &str)
  {
    push_copy(str);
    write();
    return *this;
  }

  file &
  set(const usize s)
  {
    seek = s;
    return *this;
  }

  file &
  set_start(void)
  {
    seek = 0;
    return *this;
  }

  file &
  set_end(void)
  {
    seek = size();
    return *this;
  }

  usize
  seek_pos(void) const
  {
    return seek;
  }

  void
  read_bytes(usize sz)
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");

    data.reserve(sz + 1);
    do {
      posix::lseek(__handle.fd, seek, posix::seek_set);
      max_t bytes_read = io::read(__handle.fd, *bf, ((buffer_sz > sz) ? sz : buffer_sz));
      if ( bytes_read == 0 )
        break;
      if ( bytes_read == -1 ) [[unlikely]]
        exc<except::io_error>("micron::fsys::file error reading file");
      seek += static_cast<usize>(bytes_read);
      data.append(*bf, ((buffer_sz > sz) ? sz : buffer_sz));
      sz -= ((buffer_sz > sz) ? sz : buffer_sz);
    } while ( sz );
  }

  void
  swap(file &o)
  {
    micron::swap(data, o.data);
    micron::swap(fname, o.fname);
    micron::swap(seek, o.seek);
    micron::swap(buffer_sz, o.buffer_sz);
    micron::swap(bf, o.bf);
  }

  void
  load(void)
  {
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");
    auto s = size();
    data.reserve(s + 1);
    posix::lseek(__handle.fd, 0, posix::seek_set);
    max_t read_bytes = 0;
    do {
      read_bytes = io::read(__handle.fd, data, s);
      if ( read_bytes < (s) )
        break;
    } while ( read_bytes );
    data._buf_set_length(s);
  }

  void
  load_kernel(void)
  {
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");
    posix::lseek(__handle.fd, 0, posix::seek_set);
    usize s = 0;
    max_t read_bytes = 0;
    do {
      read_bytes = io::read(__handle.fd, data[s], 1);
      s += read_bytes;
      if ( s == data.max_size() )
        data.reserve(data.max_size() * 3);
    } while ( read_bytes );
    if ( s )
      data._buf_set_length(s);
  }

  void
  write(void)
  {
    if ( !data )
      return;
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open");
    posix::lseek(__handle.fd, seek, posix::seek_set);
    max_t sz = io::write(__handle.fd, &data, data.size());
    if ( sz == -1 )
      exc<except::filesystem_error>("micron::fsys::file wasn't able to write to fd");
    if ( sz < (max_t)data.size() )
      exc<except::filesystem_error>("micron::fsys::file wasn't able to write to fd");
    seek += sz;
    posix::lseek(__handle.fd, seek, posix::seek_set);
  }

  auto
  buffer_size() const
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    return buffer_sz;
  }

  void
  load_buffer(const byte *b, const usize n)
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    if ( n > buffer_sz )
      exc<except::filesystem_error>("micron::fsys::file trying to load buffer with too much data");
    micron::bytecpy(&(*bf), b, n);
  }

  void
  write_bytes(usize sz)
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");
    posix::lseek(__handle.fd, seek, posix::seek_set);
    max_t bytes_written = io::write(__handle.fd, &(*bf), ((buffer_sz > sz) ? sz : buffer_sz));
    if ( bytes_written == -1 ) [[unlikely]]
      exc<except::io_error>("micron::fsys::file error writing to file");
    seek += static_cast<usize>(bytes_written);
    sz -= ((buffer_sz > sz) ? sz : buffer_sz);
    do {
    } while ( sz );
  }

  void
  flush(void)
  {
    if ( !data )
      return;
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open");
    posix::lseek(__handle.fd, seek, posix::seek_set);
    usize sz = io::write(__handle.fd, &data, data.size());
    if ( sz < data.size() )
      exc<except::filesystem_error>("micron::fsys::file wasn't able to write to fd");
    seek += sz;
    posix::lseek(__handle.fd, seek, posix::seek_set);
    clear();
  }

  usize
  count(void) const
  {
    return data.size();
  }

  void
  push_copy(const T &str)
  {
    data = str;
  }

  void
  push(T &&str)
  {
    data = micron::move(str);
  }

  auto
  pull(void)
  {
    auto t = micron::move(data);
    data = T();
    return t;
  }

  const auto &
  get(void) const
  {
    return data;
  }

  auto
  name(void) const
  {
    return fname;
  }

  auto
  get_fd(void) const
  {
    return __handle.fd;
  }

  auto
  load_and_pull(void)
  {
    if ( is_system_virtual() )
      load_kernel();
    else
      load();
    auto tmp = micron::move(data);
    data = T();
    return tmp;
  }

  void
  sync(void) const
  {
    posix::fsync(__handle.fd);
    posix::syncfs(__handle.fd);
  }

  byte
  read_u8(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u8");
    byte v;
    __pread_exact(&v, 1, off);
    return v;
  }

  i8
  read_i8(posix::off_t off) const
  {
    return static_cast<i8>(read_u8(off));
  }

  u16
  read_u16le(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u16le");
    byte p[2];
    __pread_exact(p, 2, off);
    return static_cast<u16>(p[0]) | (static_cast<u16>(p[1]) << 8);
  }

  u16
  read_u16be(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u16be");
    byte p[2];
    __pread_exact(p, 2, off);
    return (static_cast<u16>(p[0]) << 8) | static_cast<u16>(p[1]);
  }

  i16
  read_i16le(posix::off_t off) const
  {
    return static_cast<i16>(read_u16le(off));
  }

  i16
  read_i16be(posix::off_t off) const
  {
    return static_cast<i16>(read_u16be(off));
  }

  u32
  read_u32le(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u32le");
    byte p[4];
    __pread_exact(p, 4, off);
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) | (static_cast<u32>(p[3]) << 24);
  }

  u32
  read_u32be(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u32be");
    byte p[4];
    __pread_exact(p, 4, off);
    return (static_cast<u32>(p[0]) << 24) | (static_cast<u32>(p[1]) << 16) | (static_cast<u32>(p[2]) << 8) | static_cast<u32>(p[3]);
  }

  i32
  read_i32le(posix::off_t off) const
  {
    return static_cast<i32>(read_u32le(off));
  }

  i32
  read_i32be(posix::off_t off) const
  {
    return static_cast<i32>(read_u32be(off));
  }

  u64
  read_u64le(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u64le");
    byte p[8];
    __pread_exact(p, 8, off);
    return static_cast<u64>(p[0]) | (static_cast<u64>(p[1]) << 8) | (static_cast<u64>(p[2]) << 16) | (static_cast<u64>(p[3]) << 24)
           | (static_cast<u64>(p[4]) << 32) | (static_cast<u64>(p[5]) << 40) | (static_cast<u64>(p[6]) << 48)
           | (static_cast<u64>(p[7]) << 56);
  }

  u64
  read_u64be(posix::off_t off) const
  {
    __need_fd("fsys::file::read_u64be");
    byte p[8];
    __pread_exact(p, 8, off);
    return (static_cast<u64>(p[0]) << 56) | (static_cast<u64>(p[1]) << 48) | (static_cast<u64>(p[2]) << 40) | (static_cast<u64>(p[3]) << 32)
           | (static_cast<u64>(p[4]) << 24) | (static_cast<u64>(p[5]) << 16) | (static_cast<u64>(p[6]) << 8) | static_cast<u64>(p[7]);
  }

  i64
  read_i64le(posix::off_t off) const
  {
    return static_cast<i64>(read_u64le(off));
  }

  i64
  read_i64be(posix::off_t off) const
  {
    return static_cast<i64>(read_u64be(off));
  }

  micron::buffer
  slice(posix::off_t off, usize len) const
  {
    __need_fd("fsys::file::slice");
    micron::buffer out(len);
    max_t n = __pread_exact(out.begin(), len, off);
    if ( n < 0 || static_cast<usize>(n) < len )
      exc<except::io_error>("fsys::file::slice: short read.");
    return out;
  }

  void
  slice_into(micron::buffer &dst, posix::off_t off, usize len) const
  {
    __need_fd("fsys::file::slice_into");
    if ( dst.size() < len )
      dst.resize(len);
    max_t n = __pread_exact(dst.begin(), len, off);
    if ( n < 0 || static_cast<usize>(n) < len )
      exc<except::io_error>("fsys::file::slice_into: short read.");
  }

  void
  hex_at(char *dst, posix::off_t off, usize len) const
  {
    static constexpr char hex[] = "0123456789abcdef";
    __need_fd("fsys::file::hex_at");
    micron::buffer tmp(len);
    __pread_exact(tmp.begin(), len, off);
    for ( usize i = 0; i < len; ++i ) {
      byte b = tmp[i];
      dst[i * 3 + 0] = hex[(b >> 4) & 0xf];
      dst[i * 3 + 1] = hex[b & 0xf];
      dst[i * 3 + 2] = ' ';
    }
  }

  static constexpr usize default_search_window = 65536u;

  io::bin_match_t
  search(const byte *pat, usize plen, usize window_sz = default_search_window) const
  {
    __need_fd("fsys::file::search");
    if ( plen == 0 )
      return { true, 0, 0 };

    micron::buffer tbl_buf(plen * sizeof(usize));
    usize *tbl = reinterpret_cast<usize *>(tbl_buf.begin());
    __kmp_table(pat, plen, tbl);

    usize overlap = plen > 1 ? plen - 1 : 0;
    usize work_cap = window_sz + overlap;
    micron::buffer work(work_cap);

    posix::off_t off = 0;
    usize file_sz = static_cast<usize>(size());
    usize work_fill = 0;

    while ( static_cast<usize>(off) < file_sz ) {
      if ( overlap && work_fill >= overlap )
        micron::memcpy(work.begin(), work.begin() + (work_fill - overlap), overlap);

      max_t n = posix::pread(__handle.fd, work.begin() + overlap, window_sz, off);
      if ( n <= 0 )
        break;

      work_fill = overlap + static_cast<usize>(n);
      usize pos = __kmp_search(work.begin(), work_fill, pat, plen, tbl);

      if ( pos < work_fill ) {
        usize file_pos = static_cast<usize>(off) + pos;
        if ( pos >= overlap )
          file_pos = static_cast<usize>(off) + (pos - overlap);
        return { true, file_pos, plen };
      }
      off += static_cast<posix::off_t>(n);
    }
    return io::no_match;
  }

  io::bin_match_t
  search(const char *pat, usize window_sz = default_search_window) const
  {
    usize l = 0;
    while ( pat[l] )
      ++l;
    return search(reinterpret_cast<const byte *>(pat), l, window_sz);
  }

  template <is_string Tp>
  io::bin_match_t
  search(const Tp &pat, usize window_sz = default_search_window) const
  {
    return search(reinterpret_cast<const byte *>(pat.c_str()), pat.size(), window_sz);
  }

  io::bin_match_t
  search_file(const byte *p, usize l, usize w = default_search_window) const
  {
    return search(p, l, w);
  }

  io::bin_match_t
  search_file(const char *p, usize w = default_search_window) const
  {
    return search(p, w);
  }

  template <is_string Tp>
  io::bin_match_t
  search_file(const Tp &p, usize w = default_search_window) const
  {
    return search(p, w);
  }

  micron::vector<io::bin_match_t>
  find_all(const byte *pat, usize plen, usize window_sz = default_search_window) const
  {
    __need_fd("fsys::file::find_all");
    micron::vector<io::bin_match_t> out;
    if ( plen == 0 )
      return out;

    micron::buffer tbl_buf(plen * sizeof(usize));
    usize *tbl = reinterpret_cast<usize *>(tbl_buf.begin());
    __kmp_table(pat, plen, tbl);

    usize overlap = plen > 1 ? plen - 1 : 0;
    usize work_cap = window_sz + overlap;
    micron::buffer work(work_cap);

    posix::off_t off = 0;
    usize file_sz = static_cast<usize>(size());
    usize work_fill = 0;

    while ( static_cast<usize>(off) < file_sz ) {
      if ( overlap && work_fill >= overlap )
        micron::memcpy(work.begin(), work.begin() + (work_fill - overlap), overlap);

      max_t n = posix::pread(__handle.fd, work.begin() + overlap, window_sz, off);
      if ( n <= 0 )
        break;

      work_fill = overlap + static_cast<usize>(n);
      usize scan = 0;

      while ( scan < work_fill ) {
        usize pos = __kmp_search(work.begin() + scan, work_fill - scan, pat, plen, tbl);
        if ( pos == work_fill - scan )
          break;

        usize abs = static_cast<usize>(off) + (scan + pos);
        if ( (scan + pos) >= overlap )
          abs = static_cast<usize>(off) + ((scan + pos) - overlap);

        out.push_back({ true, abs, plen });
        scan += pos + plen;
      }
      off += static_cast<posix::off_t>(n);
    }
    return out;
  }

  micron::vector<io::bin_match_t>
  find_all(const char *pat, usize window_sz = default_search_window) const
  {
    usize l = 0;
    while ( pat[l] )
      ++l;
    return find_all(reinterpret_cast<const byte *>(pat), l, window_sz);
  }

  template <is_string Tp>
  micron::vector<io::bin_match_t>
  find_all(const Tp &pat, usize window_sz = default_search_window) const
  {
    return find_all(reinterpret_cast<const byte *>(pat.c_str()), pat.size(), window_sz);
  }

  io::bin_stats_t
  analyse(usize window_sz = default_search_window) const
  {
    __need_fd("fsys::file::analyse");
    io::bin_stats_t st{};
    usize freq[256] = {};
    usize total = 0;
    usize cur_zero = 0, cur_nz = 0;

    micron::buffer win(window_sz);
    posix::off_t off = 0;
    usize file_sz = static_cast<usize>(size());

    while ( static_cast<usize>(off) < file_sz ) {
      max_t n = posix::pread(__handle.fd, win.begin(), window_sz, off);
      if ( n <= 0 )
        break;
      const byte *p = win.begin();
      for ( max_t i = 0; i < n; ++i ) {
        byte b = p[i];
        freq[b]++;
        total++;
        if ( b == 0x00 ) {
          st.zero_bytes++;
          if ( ++cur_zero > st.longest_zero_run )
            st.longest_zero_run = cur_zero;
          cur_nz = 0;
        } else {
          if ( ++cur_nz > st.longest_nonzero_run )
            st.longest_nonzero_run = cur_nz;
          cur_zero = 0;
        }
        if ( b >= 0x20 && b <= 0x7e )
          st.printable_bytes++;
        if ( b >= 0x80 )
          st.high_bytes++;
      }
      off += n;
    }

    st.total_bytes = total;
    for ( usize i = 0; i < 256; ++i )
      st.freq[i] = freq[i];

    double H = 0.0;
    for ( usize i = 0; i < 256; ++i ) {
      if ( !freq[i] )
        continue;
      double pi = static_cast<double>(freq[i]) / static_cast<double>(total);
      H -= pi * math::log2(pi);
    }
    st.entropy = H;

    return st;
  }

  double
  entropy(usize window_sz = default_search_window) const
  {
    __need_fd("fsys::file::entropy");
    usize freq[256] = {};
    usize total = 0;
    micron::buffer win(window_sz);
    posix::off_t off = 0;
    usize file_sz = static_cast<usize>(size());

    while ( static_cast<usize>(off) < file_sz ) {
      max_t n = posix::pread(__handle.fd, win.begin(), window_sz, off);
      if ( n <= 0 )
        break;
      const byte *p = win.begin();
      for ( max_t i = 0; i < n; ++i ) {
        freq[p[i]]++;
        total++;
      }
      off += n;
    }

    double H = 0.0;
    for ( usize i = 0; i < 256; ++i ) {
      if ( !freq[i] )
        continue;
      double pi = static_cast<double>(freq[i]) / static_cast<double>(total);
      H -= pi * math::log2(pi);
    }
    return H;
  }

  template <int SZ, int CK>
  void
  to_stream(io::stream<SZ, CK> &s) const
  {
    __need_fd("fsys::file::to_stream");
    s << __handle;
  }

  template <int SZ, int CK>
  void
  from_stream(io::stream<SZ, CK> &s)
  {
    __need_fd("fsys::file::from_stream");
    if ( s.empty() )
      return;
    posix::lseek(__handle.fd, static_cast<posix::off_t>(seek), posix::seek_set);
    max_t n = io::write(__handle.fd, s.data(), static_cast<usize>(s.size()));
    if ( n > 0 )
      seek += static_cast<usize>(n);
    s.rewind();
  }

  template <int SZ, int CK>
  void
  flush_to_stream(io::stream<SZ, CK> &s) const
  {
    if ( !data )
      return;
    s << data;
  }

  template <io::encode_fn Fn, is_string Tp>
  void
  write_encoded(Fn &&fn, const Tp &src)
  {
    __need_fd("fsys::file::write_encoded");
    usize src_len = src.size() * sizeof(typename Tp::value_type);
    micron::buffer out(src_len * 4 + 64);
    usize out_len = fn(out.begin(), reinterpret_cast<const byte *>(src.c_str()), src_len);
    posix::lseek(__handle.fd, static_cast<posix::off_t>(seek), posix::seek_set);
    max_t n = io::write(__handle.fd, out.begin(), out_len);
    if ( n > 0 )
      seek += static_cast<usize>(n);
  }

  template <io::encode_fn Fn>
  void
  write_encoded(Fn &&fn, const byte *src, usize src_len)
  {
    __need_fd("fsys::file::write_encoded");
    micron::buffer out(src_len * 4 + 64);
    usize out_len = fn(out.begin(), src, src_len);
    posix::lseek(__handle.fd, static_cast<posix::off_t>(seek), posix::seek_set);
    max_t n = io::write(__handle.fd, out.begin(), out_len);
    if ( n > 0 )
      seek += static_cast<usize>(n);
  }

  template <io::encode_fn Fn, is_string Tp>
  void
  append_encoded(Fn &&fn, const Tp &src)
  {
    __need_fd("fsys::file::append_encoded");
    usize src_len = src.size() * sizeof(typename Tp::value_type);
    micron::buffer out(src_len * 4 + 64);
    usize out_len = fn(out.begin(), reinterpret_cast<const byte *>(src.c_str()), src_len);
    posix::off_t eof = size();
    io::write(__handle.fd, out.begin(), out_len);
    (void)eof;
  }

  io::path
  as_path() const
  {
    return io::path(fname.c_str());
  }

  io::path_t
  fullpath() const
  {
    io::path p(fname.c_str());
    return p.canonical();
  }

  io::path_t
  basename() const
  {
    return io::path(fname.c_str()).basename();
  }

  io::path_t
  extension() const
  {
    return io::path(fname.c_str()).extension();
  }

  io::path_t
  stem() const
  {
    return io::path(fname.c_str()).stem();
  }

  bool
  empty()
  {
    return size() == 0;
  }

  bool
  is_regular()
  {
    return posix::is_file(__handle);
  }

  bool
  is_virtual()
  {
    return is_system_virtual();
  }

  posix::mode_t
  permissions_mode()
  {
    return mode();
  }

  io::linux_permissions
  perms()
  {
    return permissions();
  }

  posix::uid_t
  owner()
  {
    return uid();
  }

  posix::gid_t
  group()
  {
    return gid();
  }

  posix::ino_t
  inode_number()
  {
    return inode();
  }

  posix::nlink_t
  hard_links()
  {
    return link_count();
  }

  posix::time_t
  modified_time()
  {
    return mtime();
  }

  posix::time_t
  accessed_time()
  {
    return atime();
  }

  posix::time_t
  changed_time()
  {
    return ctime();
  }

  bool
  owned_by_me() const
  {
    return posix::is_owned_by(__handle, posix::getuid());
  }

  void
  truncate(usize new_size)
  {
    __need_fd("fsys::file::truncate");
    if ( posix::ftruncate(__handle.fd, static_cast<posix::off_t>(new_size)) != 0 )
      exc<except::filesystem_error>("fsys::file::truncate failed.");
    usize cur = static_cast<usize>(size());
    if ( seek > new_size )
      seek = new_size;
    (void)cur;
  }

  template <is_string U>
  void
  append_file(const file<U> &other, usize chunk_sz = 65536u)
  {
    __need_fd("fsys::file::append_file");
    if ( other.__handle.closed() )
      exc<except::filesystem_error>("fsys::file::append_file: source file is closed.");

    posix::off_t src_off = 0;
    usize src_sz = static_cast<usize>(other.size());
    posix::off_t dst_off = size();

    micron::buffer win(chunk_sz);
    while ( static_cast<usize>(src_off) < src_sz ) {
      max_t n = posix::pread(other.__handle.fd, win.begin(), chunk_sz, src_off);
      if ( n <= 0 )
        break;
      posix::pwrite(__handle.fd, win.begin(), static_cast<usize>(n), dst_off);
      src_off += n;
      dst_off += n;
    }
  }

  void
  append_raw(const byte *data_ptr, usize len)
  {
    __need_fd("fsys::file::append_raw");
    posix::off_t dst_off = size();
    posix::pwrite(__handle.fd, data_ptr, len, dst_off);
  }

  template <is_string Tp>
  void
  append_raw(const Tp &str)
  {
    append_raw(reinterpret_cast<const byte *>(str.c_str()), str.size() * sizeof(typename Tp::value_type));
  }

  void
  copy_to(const char *dest_path, usize chunk_sz = 65536u) const
  {
    __need_fd("fsys::file::copy_to");
    posix::fd_t out{ (posix::openat(posix::at_fdcwd, dest_path, posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec,
                                    posix::mode_file)) };
    if ( !out )
      exc<except::filesystem_error>("fsys::file::copy_to: failed to open destination.");

    usize src_sz = static_cast<usize>(size());
    posix::off_t src_off = 0;
    micron::buffer win(chunk_sz);

    while ( static_cast<usize>(src_off) < src_sz ) {
      max_t n = posix::pread(__handle.fd, win.begin(), chunk_sz, src_off);
      if ( n <= 0 )
        break;
      posix::write(out.fd, win.begin(), static_cast<usize>(n));
      src_off += n;
    }
    posix::close(out.fd);
  }

  template <is_string Tp>
  void
  copy_to(const Tp &dest_path, usize chunk_sz = 65536u) const
  {
    copy_to(dest_path.c_str(), chunk_sz);
  }

  template <is_string U>
  i32
  compare_to(const file<U> &other, usize chunk_sz = 65536u) const
  {
    __need_fd("fsys::file::compare_to");
    if ( other.__handle.closed() )
      exc<except::filesystem_error>("fsys::file::compare_to: other file is closed.");

    usize a_sz = static_cast<usize>(size());
    usize b_sz = static_cast<usize>(other.size());
    usize scan = 0;
    micron::buffer wa(chunk_sz), wb(chunk_sz);

    while ( scan < a_sz && scan < b_sz ) {
      usize take = chunk_sz < (a_sz - scan) ? chunk_sz : (a_sz - scan);
      take = take < (b_sz - scan) ? take : (b_sz - scan);

      max_t na = posix::pread(__handle.fd, wa.begin(), take, static_cast<posix::off_t>(scan));
      max_t nb = posix::pread(other.__handle.fd, wb.begin(), take, static_cast<posix::off_t>(scan));
      if ( na <= 0 || nb <= 0 )
        break;

      usize cmp_n = static_cast<usize>(na) < static_cast<usize>(nb) ? static_cast<usize>(na) : static_cast<usize>(nb);

      for ( usize i = 0; i < cmp_n; ++i ) {
        if ( wa[i] < wb[i] )
          return -1;
        if ( wa[i] > wb[i] )
          return 1;
      }
      scan += cmp_n;
    }

    if ( a_sz < b_sz )
      return -1;
    if ( a_sz > b_sz )
      return 1;
    return 0;
  }

  template <io::intercept_fn Fn>
  void
  load_intercepted(Fn &&fn, usize chunk_sz = 65536u)
  {
    __need_fd("fsys::file::load_intercepted");
    posix::lseek(__handle.fd, 0, posix::seek_set);
    usize file_sz = static_cast<usize>(size());
    data.reserve(file_sz + 1);

    micron::buffer win(chunk_sz);
    posix::off_t off = 0;

    while ( static_cast<usize>(off) < file_sz ) {
      max_t n = posix::pread(__handle.fd, win.begin(), chunk_sz, off);
      if ( n <= 0 )
        break;
      fn(win.begin(), static_cast<usize>(n));
      data.append(*reinterpret_cast<const T *>(win.begin()), static_cast<usize>(n));
      off += n;
    }
  }

  void
  atomic_replace(const byte *new_data, usize new_len)
  {
    if ( fname.empty() )
      exc<except::filesystem_error>("fsys::file::atomic_replace: no filename recorded.");

    micron::sstr<io::max_name> tmp_name(fname);
    tmp_name += ".tmp.";
    u64 pid = static_cast<u64>(posix::getpid());
    char pid_buf[24];
    usize pi = 0;
    if ( pid == 0 ) {
      pid_buf[pi++] = '0';
    } else {
      u64 p = pid;
      while ( p ) {
        pid_buf[pi++] = '0' + static_cast<char>(p % 10);
        p /= 10;
      }
    }
    for ( usize l = 0, r = pi - 1; l < r; ++l, --r ) {
      char c = pid_buf[l];
      pid_buf[l] = pid_buf[r];
      pid_buf[r] = c;
    }
    pid_buf[pi] = '\0';
    tmp_name += pid_buf;

    posix::fd_t tmp{ (posix::openat(posix::at_fdcwd, tmp_name.c_str(),
                                    posix::o_wronly | posix::o_create | posix::o_trunc | posix::o_cloexec, posix::mode_file)) };
    if ( !tmp )
      exc<except::filesystem_error>("fsys::file::atomic_replace: failed to create temp file.");

    usize written = 0;
    while ( written < new_len ) {
      max_t n = posix::write(tmp.fd, new_data + written, new_len - written);
      if ( n <= 0 ) {
        posix::close(tmp.fd);
        posix::unlink(tmp_name.c_str());
        exc<except::io_error>("fsys::file::atomic_replace: write to temp failed.");
      }
      written += static_cast<usize>(n);
    }
    posix::fsync(tmp.fd);
    posix::close(tmp.fd);

    if ( posix::rename(tmp_name.c_str(), fname.c_str()) != 0 ) {
      posix::unlink(tmp_name.c_str());
      exc<except::filesystem_error>("fsys::file::atomic_replace: rename failed.");
    }
  }

  template <is_string Tp>
  void
  atomic_replace(const Tp &str)
  {
    atomic_replace(reinterpret_cast<const byte *>(str.c_str()), str.size() * sizeof(typename Tp::value_type));
  }

  void
  atomic_replace(const micron::buffer &buf)
  {
    atomic_replace(buf.begin(), buf.size());
  }
};

};     // namespace fsys
};     // namespace micron
