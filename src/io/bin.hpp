//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../string/strings.hpp"
#include "../types.hpp"

#include "io.hpp"
#include "paths.hpp"
#include "posix/file.hpp"

namespace micron
{
namespace io
{

struct bin_match_t {
  bool found;
  usize offset;
  usize length;

  explicit
  operator bool(void) const noexcept
  {
    return found;
  }
};

inline constexpr bin_match_t no_match{ false, 0, 0 };

struct bin_range_t {
  const byte *ptr;
  usize len;

  const byte *
  begin(void) const noexcept
  {
    return ptr;
  }

  const byte *
  end(void) const noexcept
  {
    return ptr + len;
  }

  usize
  size(void) const noexcept
  {
    return len;
  }

  bool
  empty(void) const noexcept
  {
    return len == 0;
  }

  explicit
  operator bool(void) const noexcept
  {
    return ptr && len;
  }

  const byte &
  operator[](usize i) const noexcept
  {
    return ptr[i];
  }
};

struct bin_stats_t {
  usize total_bytes;
  usize zero_bytes;
  usize printable_bytes;
  usize high_bytes;
  usize freq[256];
  usize longest_zero_run;
  usize longest_nonzero_run;
  double entropy;
};

namespace __bin_impl
{

inline void
kmp_table(const byte *pat, usize pat_len, usize *table)
{
  if ( pat_len == 0 ) return;
  table[0] = 0;
  usize k = 0;
  for ( usize i = 1; i < pat_len; ) {
    if ( pat[i] == pat[k] ) {
      table[i] = k + 1;
      ++i;
      ++k;
    } else if ( k ) {
      k = table[k - 1];
    } else {
      table[i] = 0;
      ++i;
    }
  }
}

inline usize
kmp_search(const byte *hay, usize hay_len, const byte *pat, usize pat_len, const usize *table)
{
  if ( pat_len == 0 ) return 0;
  if ( pat_len > hay_len ) return hay_len;

  usize k = 0;
  for ( usize i = 0; i < hay_len; ) {
    if ( hay[i] == pat[k] ) {
      ++k;
      ++i;
      if ( k == pat_len ) return i - k;
    } else if ( k ) {
      k = table[k - 1];
    } else {
      ++i;
    }
  }
  return hay_len;
}

inline usize
reverse_search(const byte *hay, usize hay_len, const byte *pat, usize pat_len)
{
  if ( pat_len == 0 || pat_len > hay_len ) return hay_len;
  usize i = hay_len - pat_len;
  for ( ;; ) {
    bool ok = true;
    for ( usize j = 0; j < pat_len; ++j ) {
      if ( hay[i + j] != pat[j] ) {
        ok = false;
        break;
      }
    }
    if ( ok ) return i;
    if ( i == 0 ) break;
    --i;
  }
  return hay_len;
}

inline double
fast_log2(double x)
{
  if ( x <= 0.0 ) return 0.0;
  int exp = 0;
  while ( x >= 2.0 ) {
    x *= 0.5;
    ++exp;
  }
  while ( x < 1.0 ) {
    x *= 2.0;
    --exp;
  }
  double t = x - 1.0;
  double ln = t;
  double p = t * t;
  ln -= p / 2.0;
  p *= t;
  ln += p / 3.0;
  p *= t;
  ln -= p / 4.0;
  p *= t;
  ln += p / 5.0;
  return static_cast<double>(exp) + ln / 0.693147180559945;
}

}     // namespace __bin_impl

template <is_string T = micron::string> class binary : public io::file
{

  micron::sstr<io::max_name> __fname;
  micron::shared<micron::buffer> __buf;     // internal window buffer (nullable)
  usize __buf_sz;                           // capacity of *__buf
  usize __buf_valid;                        // populated bytes in *__buf
  posix::off_t __buf_off;                   // file offset of *__buf start
  usize __seek;                             // logical read cursor (file-relative)

  void
  __require_buf(void) const
  {
    if ( !__buf ) exc<except::filesystem_error>("micron::binary, no buffer allocated — open with a buffer size.");
  }

  void
  __require_window(usize off, usize len) const
  {
    __require_buf();
    if ( __buf_valid == 0 || off + len > __buf_valid )
      exc<except::io_error>("micron::binary, access outside current window — call fill() first.");
  }

  inline bin_match_t
  search(const byte *hay, usize hay_len, const byte *pat, usize pat_len)
  {
    if ( pat_len == 0 ) return { true, 0, 0 };
    if ( pat_len > hay_len ) return no_match;

    usize *table = reinterpret_cast<usize *>(__builtin_alloca(pat_len * sizeof(usize)));
    __bin_impl::kmp_table(pat, pat_len, table);

    usize pos = __bin_impl::kmp_search(hay, hay_len, pat, pat_len, table);
    if ( pos == hay_len ) return no_match;
    return { true, pos, pat_len };
  }

  inline bin_match_t
  search(bin_range_t range, const byte *pat, usize pat_len)
  {
    return search(range.ptr, range.len, pat, pat_len);
  }

  inline bin_match_t
  search(bin_range_t range, const char *pat)
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return search(range.ptr, range.len, reinterpret_cast<const byte *>(pat), l);
  }

  inline bin_stats_t
  analyse(const byte *data, usize len)
  {
    binary<> dummy;

    bin_range_t r{ data, len };
    return dummy.analyse(r);
  }

  inline bin_stats_t
  analyse(bin_range_t range)
  {
    return analyse(range.ptr, range.len);
  }

  inline double
  entropy(const byte *data, usize len)
  {
    return analyse(data, len).entropy;
  }

  inline double
  entropy(bin_range_t range)
  {
    return entropy(range.ptr, range.len);
  }

public:
  static constexpr usize __default_buf_size = 65536u;     // 64 K default window

  ~binary() = default;

  binary() : io::file(), __fname(), __buf(nullptr), __buf_sz(0), __buf_valid(0), __buf_off(0), __seek(0) {}

  binary(const char *name, io::modes mode = io::modes::read, usize buf_sz = __default_buf_size)
      : io::file(name, mode), __fname(name), __buf(new micron::buffer(buf_sz)), __buf_sz(buf_sz), __buf_valid(0), __buf_off(0),
        __seek(mode == io::modes::append || mode == io::modes::appendread ? static_cast<usize>(size()) : 0)
  {
  }

  binary(const T &name, io::modes mode = io::modes::read, usize buf_sz = __default_buf_size) : binary(name.c_str(), mode, buf_sz) {}

  template <usize M>
  binary(const char (&name)[M], io::modes mode = io::modes::read, usize buf_sz = __default_buf_size)
      : binary(static_cast<const char *>(name), mode, buf_sz)
  {
  }

  binary(const binary &o)
      : io::file(o), __fname(o.__fname), __buf(o.__buf), __buf_sz(o.__buf_sz), __buf_valid(o.__buf_valid), __buf_off(o.__buf_off),
        __seek(o.__seek)
  {
  }

  binary(binary &&o) noexcept
      : io::file(micron::move(o)), __fname(micron::move(o.__fname)), __buf(micron::move(o.__buf)), __buf_sz(o.__buf_sz),
        __buf_valid(o.__buf_valid), __buf_off(o.__buf_off), __seek(o.__seek)
  {
    o.__buf_sz = 0;
    o.__buf_valid = 0;
    o.__buf_off = 0;
    o.__seek = 0;
  }

  binary &
  operator=(binary &&o) noexcept
  {
    io::file::operator=(micron::move(o));
    __fname = micron::move(o.__fname);
    __buf = micron::move(o.__buf);
    __buf_sz = o.__buf_sz;
    __buf_valid = o.__buf_valid;
    __buf_off = o.__buf_off;
    __seek = o.__seek;
    o.__buf_sz = 0;
    o.__buf_valid = 0;
    o.__buf_off = 0;
    o.__seek = 0;
    return *this;
  }

  binary &
  operator=(const binary &o)
  {
    io::file::operator=(o);
    __fname = o.__fname;
    __buf = o.__buf;
    __buf_sz = o.__buf_sz;
    __buf_valid = o.__buf_valid;
    __buf_off = o.__buf_off;
    __seek = o.__seek;
    return *this;
  }

  auto
  name(void) const
  {
    return __fname;
  }

  usize
  seek_pos(void) const
  {
    return __seek;
  }

  usize
  buf_capacity(void) const
  {
    return __buf_sz;
  }

  usize
  buf_valid(void) const
  {
    return __buf_valid;
  }

  posix::off_t
  buf_offset(void) const
  {
    return __buf_off;
  }

  bool
  has__buffer(void) const
  {
    return __buf != nullptr;
  }

  const byte *
  buf_data(void) const
  {
    __require_buf();
    return __buf->begin();
  }

  binary &
  set(usize pos)
  {
    __seek = pos;
    return *this;
  }

  binary &
  set_start(void)
  {
    __seek = 0;
    return *this;
  }

  binary &
  set_end(void)
  {
    __seek = static_cast<usize>(size());
    return *this;
  }

  binary &
  skip(usize n)
  {
    __seek += n;
    return *this;
  }

  binary &
  rewind(usize n = 0)
  {
    __seek = n > __seek ? 0 : __seek - n;
    return *this;
  }

  void
  resize__buf(usize new_sz)
  {
    __buf = micron::shared<micron::buffer>(new micron::buffer(new_sz));
    __buf_sz = new_sz;
    __buf_valid = 0;
    __buf_off = 0;
  }

  max_t
  fill(posix::off_t off = 0)
  {
    __require_buf();
    posix::lseek(__handle.fd, off, posix::seek_set);
    max_t n = io::read(__handle.fd, *__buf, __buf_sz);
    if ( n > 0 ) {
      __buf_off = off;
      __buf_valid = static_cast<usize>(n);
    } else {
      __buf_valid = 0;
    }
    return n;
  }

  max_t
  advance(void)
  {
    __require_buf();
    posix::off_t next = __buf_off + static_cast<posix::off_t>(__buf_valid ? __buf_valid : __buf_sz);
    if ( static_cast<usize>(next) >= static_cast<usize>(size()) ) return 0;
    return fill(next);
  }

  bool
  at_end(void) const
  {
    if ( __buf_valid == 0 ) return true;
    return (static_cast<usize>(__buf_off) + __buf_valid) >= static_cast<usize>(size());
  }

  bool
  buf_covers(posix::off_t off, usize len) const noexcept
  {
    if ( !__buf || __buf_valid == 0 ) return false;
    return off >= __buf_off && (static_cast<usize>(off) + len) <= (static_cast<usize>(__buf_off) + __buf_valid);
  }

  max_t
  read_into(void *dst, usize len, posix::off_t off) const
  {
    return posix::pread(__handle.fd, dst, len, off);
  }

  max_t
  read_exact(void *dst, usize len, posix::off_t off) const
  {
    usize got = 0;
    byte *p = static_cast<byte *>(dst);
    while ( got < len ) {
      max_t r = posix::pread(__handle.fd, p + got, len - got, off + static_cast<posix::off_t>(got));
      if ( r <= 0 ) return r == 0 ? static_cast<max_t>(got) : r;
      got += static_cast<usize>(r);
    }
    return static_cast<max_t>(got);
  }

  max_t
  read__buffered(void *dst, usize len, posix::off_t off)
  {
    if ( buf_covers(off, len) ) {
      usize delta = static_cast<usize>(off - __buf_off);
      micron::memcpy(static_cast<byte *>(dst), __buf->begin() + delta, len);
      return static_cast<max_t>(len);
    }
    max_t n = fill(off);
    if ( n <= 0 ) return n;
    usize copy = static_cast<usize>(n) < len ? static_cast<usize>(n) : len;
    micron::memcpy(static_cast<byte *>(dst), __buf->begin(), copy);
    return static_cast<max_t>(copy);
  }

  max_t
  write_at(const void *src, usize len, posix::off_t off)
  {
    return posix::pwrite(__handle.fd, src, len, off);
  }

  bin_range_t
  window(usize offset = 0, usize len = usize(-1)) const noexcept
  {
    if ( !__buf || __buf_valid == 0 ) return { nullptr, 0 };
    if ( offset >= __buf_valid ) return { nullptr, 0 };
    usize avail = __buf_valid - offset;
    usize take = len < avail ? len : avail;
    return { __buf->begin() + offset, take };
  }

  bin_range_t
  window_at(posix::off_t file_off, usize len = usize(-1)) const noexcept
  {
    if ( !buf_covers(file_off, 0) ) return { nullptr, 0 };
    usize delta = static_cast<usize>(file_off - __buf_off);
    usize avail = __buf_valid - delta;
    usize take = len < avail ? len : avail;
    return { __buf->begin() + delta, take };
  }

  bin_range_t
  as_range(void) const noexcept
  {
    return window(0, __buf_valid);
  }

  byte
  read_u8(usize off) const
  {
    __require_window(off, 1);
    return (*__buf)[off];
  }

  u16
  read_u16le(usize off) const
  {
    __require_window(off, 2);
    const byte *p = __buf->begin() + off;
    return static_cast<u16>(p[0]) | (static_cast<u16>(p[1]) << 8);
  }

  u16
  read_u16be(usize off) const
  {
    __require_window(off, 2);
    const byte *p = __buf->begin() + off;
    return (static_cast<u16>(p[0]) << 8) | static_cast<u16>(p[1]);
  }

  u32
  read_u32le(usize off) const
  {
    __require_window(off, 4);
    const byte *p = __buf->begin() + off;
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) | (static_cast<u32>(p[3]) << 24);
  }

  u32
  read_u32be(usize off) const
  {
    __require_window(off, 4);
    const byte *p = __buf->begin() + off;
    return (static_cast<u32>(p[0]) << 24) | (static_cast<u32>(p[1]) << 16) | (static_cast<u32>(p[2]) << 8) | static_cast<u32>(p[3]);
  }

  u64
  read_u64le(usize off) const
  {
    __require_window(off, 8);
    const byte *p = __buf->begin() + off;
    return static_cast<u64>(p[0]) | (static_cast<u64>(p[1]) << 8) | (static_cast<u64>(p[2]) << 16) | (static_cast<u64>(p[3]) << 24)
           | (static_cast<u64>(p[4]) << 32) | (static_cast<u64>(p[5]) << 40) | (static_cast<u64>(p[6]) << 48)
           | (static_cast<u64>(p[7]) << 56);
  }

  u64
  read_u64be(usize off) const
  {
    __require_window(off, 8);
    const byte *p = __buf->begin() + off;
    return (static_cast<u64>(p[0]) << 56) | (static_cast<u64>(p[1]) << 48) | (static_cast<u64>(p[2]) << 40) | (static_cast<u64>(p[3]) << 32)
           | (static_cast<u64>(p[4]) << 24) | (static_cast<u64>(p[5]) << 16) | (static_cast<u64>(p[6]) << 8) | static_cast<u64>(p[7]);
  }

  i8
  read_i8(usize off) const
  {
    return static_cast<i8>(read_u8(off));
  }

  i16
  read_i16le(usize off) const
  {
    return static_cast<i16>(read_u16le(off));
  }

  i16
  read_i16be(usize off) const
  {
    return static_cast<i16>(read_u16be(off));
  }

  i32
  read_i32le(usize off) const
  {
    return static_cast<i32>(read_u32le(off));
  }

  i32
  read_i32be(usize off) const
  {
    return static_cast<i32>(read_u32be(off));
  }

  i64
  read_i64le(usize off) const
  {
    return static_cast<i64>(read_u64le(off));
  }

  i64
  read_i64be(usize off) const
  {
    return static_cast<i64>(read_u64be(off));
  }

  void
  read_bytes(byte *dst, usize n, usize off) const
  {
    __require_window(off, n);
    micron::memcpy(dst, __buf->begin() + off, n);
  }

  micron::buffer
  slice(posix::off_t off, usize len) const
  {
    micron::buffer out(len);
    max_t n = read_exact(out.begin(), len, off);
    if ( n < 0 || static_cast<usize>(n) < len ) exc<except::io_error>("micron::binary::slice failed to read requested range.");
    return out;
  }

  void
  slice_into(micron::buffer &dst, posix::off_t off, usize len) const
  {
    if ( dst.size() < len ) dst.resize(len);
    max_t n = read_exact(dst.begin(), len, off);
    if ( n < 0 || static_cast<usize>(n) < len ) exc<except::io_error>("micron::binary::slice_into failed to read requested range.");
  }

  bin_match_t
  search(const byte *pat, usize pat_len) const
  {
    __require_buf();
    if ( __buf_valid == 0 || pat_len == 0 ) return no_match;
    if ( pat_len > __buf_valid ) return no_match;

    usize *table = reinterpret_cast<usize *>(__builtin_alloca(pat_len * sizeof(usize)));
    __bin_impl::kmp_table(pat, pat_len, table);

    const byte *hay = __buf->begin();
    usize pos = __bin_impl::kmp_search(hay, __buf_valid, pat, pat_len, table);
    if ( pos == __buf_valid ) return no_match;

    return { true, static_cast<usize>(__buf_off) + pos, pat_len };
  }

  bin_match_t
  search(const char *pat) const
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return search(reinterpret_cast<const byte *>(pat), l);
  }

  template <is_string Tp>
  bin_match_t
  search(const Tp &pat) const
  {
    return search(reinterpret_cast<const byte *>(pat.c_str()), pat.size());
  }

  bin_match_t
  search_back(const byte *pat, usize pat_len) const
  {
    __require_buf();
    if ( __buf_valid == 0 || pat_len == 0 ) return no_match;

    const byte *hay = __buf->begin();
    usize pos = __bin_impl::reverse_search(hay, __buf_valid, pat, pat_len);
    if ( pos == __buf_valid ) return no_match;

    return { true, static_cast<usize>(__buf_off) + pos, pat_len };
  }

  bin_match_t
  search_back(const char *pat) const
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return search_back(reinterpret_cast<const byte *>(pat), l);
  }

  bin_match_t
  search_byte(byte val) const noexcept
  {
    if ( !__buf || __buf_valid == 0 ) return no_match;
    const byte *p = __buf->begin();
    for ( usize i = 0; i < __buf_valid; ++i ) {
      if ( p[i] == val ) return { true, static_cast<usize>(__buf_off) + i, 1 };
    }
    return no_match;
  }

  bin_match_t
  search_byte_back(byte val) const noexcept
  {
    if ( !__buf || __buf_valid == 0 ) return no_match;
    const byte *p = __buf->begin();
    usize i = __buf_valid;
    while ( i-- ) {
      if ( p[i] == val ) return { true, static_cast<usize>(__buf_off) + i, 1 };
    }
    return no_match;
  }

  bin_match_t
  search_file(const byte *pat, usize pat_len)
  {
    __require_buf();
    if ( pat_len == 0 ) return { true, 0, 0 };

    usize *table = reinterpret_cast<usize *>(__builtin_alloca(pat_len * sizeof(usize)));
    __bin_impl::kmp_table(pat, pat_len, table);

    usize overlap = pat_len > 1 ? pat_len - 1 : 0;
    usize work_sz = __buf_sz + overlap;
    micron::buffer work(work_sz);

    posix::off_t file_off = 0;
    usize file_size = static_cast<usize>(size());
    usize work_fill = 0;

    while ( static_cast<usize>(file_off) < file_size ) {

      if ( overlap && work_fill >= overlap ) micron::memcpy(work.begin(), work.begin() + (work_fill - overlap), overlap);

      max_t n = posix::pread(__handle.fd, work.begin() + overlap, __buf_sz, file_off);
      if ( n <= 0 ) break;

      work_fill = overlap + static_cast<usize>(n);

      usize pos = __bin_impl::kmp_search(work.begin(), work_fill, pat, pat_len, table);
      if ( pos < work_fill ) {

        usize file_pos = static_cast<usize>(file_off) + pos;
        if ( pos >= overlap ) file_pos = static_cast<usize>(file_off) + (pos - overlap);
        return { true, file_pos, pat_len };
      }

      file_off += static_cast<posix::off_t>(n);
    }

    return no_match;
  }

  bin_match_t
  search_file(const char *pat)
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return search_file(reinterpret_cast<const byte *>(pat), l);
  }

  template <is_string Tp>
  bin_match_t
  search_file(const Tp &pat)
  {
    return search_file(reinterpret_cast<const byte *>(pat.c_str()), pat.size());
  }

  bin_match_t
  search_file(byte val)
  {
    const byte p[1] = { val };
    return search_file(p, 1);
  }

  micron::vector<bin_match_t>
  find_all(const byte *pat, usize pat_len)
  {
    __require_buf();
    micron::vector<bin_match_t> out;
    if ( pat_len == 0 ) return out;

    usize *table = reinterpret_cast<usize *>(__builtin_alloca(pat_len * sizeof(usize)));
    __bin_impl::kmp_table(pat, pat_len, table);

    usize overlap = pat_len > 1 ? pat_len - 1 : 0;
    usize work_sz = __buf_sz + overlap;
    micron::buffer work(work_sz);

    posix::off_t file_off = 0;
    usize file_size = static_cast<usize>(size());
    usize work_fill = 0;

    while ( static_cast<usize>(file_off) < file_size ) {
      if ( overlap && work_fill >= overlap ) micron::memcpy(work.begin(), work.begin() + (work_fill - overlap), overlap);

      max_t n = posix::pread(__handle.fd, work.begin() + overlap, __buf_sz, file_off);
      if ( n <= 0 ) break;

      work_fill = overlap + static_cast<usize>(n);

      usize scan = 0;
      while ( scan < work_fill ) {
        usize pos = __bin_impl::kmp_search(work.begin() + scan, work_fill - scan, pat, pat_len, table);
        if ( pos == work_fill - scan ) break;

        usize abs = static_cast<usize>(file_off) + (scan + pos);
        if ( (scan + pos) >= overlap ) abs = static_cast<usize>(file_off) + ((scan + pos) - overlap);

        out.push_back({ true, abs, pat_len });
        scan += pos + pat_len;
      }

      file_off += static_cast<posix::off_t>(n);
    }
    return out;
  }

  micron::vector<bin_match_t>
  find_all(const char *pat)
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return find_all(reinterpret_cast<const byte *>(pat), l);
  }

  template <is_string Tp>
  micron::vector<bin_match_t>
  find_all(const Tp &pat)
  {
    return find_all(reinterpret_cast<const byte *>(pat.c_str()), pat.size());
  }

  i32
  compare(usize off, const byte *other, usize len) const
  {
    __require_window(off, len);
    const byte *p = __buf->begin() + off;
    for ( usize i = 0; i < len; ++i ) {
      if ( p[i] < other[i] ) return -1;
      if ( p[i] > other[i] ) return 1;
    }
    return 0;
  }

  i32
  compare(usize off, const char *other) const
  {
    usize l = 0;
    while ( other[l] ) ++l;
    return compare(off, reinterpret_cast<const byte *>(other), l);
  }

  bool
  starts_with(const byte *pat, usize pat_len) const noexcept
  {
    if ( !__buf || __buf_valid < pat_len ) return false;
    return compare(0, pat, pat_len) == 0;
  }

  bool
  starts_with(const char *pat) const noexcept
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return starts_with(reinterpret_cast<const byte *>(pat), l);
  }

  bool
  ends_with(const byte *pat, usize pat_len) const noexcept
  {
    if ( !__buf || __buf_valid < pat_len ) return false;
    usize off = __buf_valid - pat_len;
    return compare(off, pat, pat_len) == 0;
  }

  bool
  ends_with(const char *pat) const noexcept
  {
    usize l = 0;
    while ( pat[l] ) ++l;
    return ends_with(reinterpret_cast<const byte *>(pat), l);
  }

  usize
  count_byte(byte val) const noexcept
  {
    if ( !__buf || __buf_valid == 0 ) return 0;
    const byte *p = __buf->begin();
    usize n = 0;
    for ( usize i = 0; i < __buf_valid; ++i )
      if ( p[i] == val ) ++n;
    return n;
  }

  usize
  count_byte_file(byte val)
  {
    __require_buf();
    usize count = 0;
    posix::off_t off = 0;
    usize file_sz = static_cast<usize>(size());
    while ( static_cast<usize>(off) < file_sz ) {
      max_t n = posix::pread(__handle.fd, __buf->begin(), __buf_sz, off);
      if ( n <= 0 ) break;
      const byte *p = __buf->begin();
      for ( max_t i = 0; i < n; ++i )
        if ( p[i] == val ) ++count;
      off += n;
    }
    return count;
  }

  struct run_t {
    byte val;
    usize off;
    usize len;
  };

  run_t
  longest_run(void) const noexcept
  {
    run_t best{ 0, 0, 0 };
    if ( !__buf || __buf_valid == 0 ) return best;
    const byte *p = __buf->begin();
    usize start = 0;
    while ( start < __buf_valid ) {
      byte cur = p[start];
      usize end = start + 1;
      while ( end < __buf_valid && p[end] == cur ) ++end;
      usize len = end - start;
      if ( len > best.len ) {
        best = { cur, start, len };
      }
      start = end;
    }
    return best;
  }

  usize
  skip_byte(byte val, usize from = 0) const noexcept
  {
    if ( !__buf ) return 0;
    const byte *p = __buf->begin();
    while ( from < __buf_valid && p[from] == val ) ++from;
    return from;
  }

  usize
  find_byte(byte val, usize from = 0) const noexcept
  {
    if ( !__buf ) return __buf_valid;
    const byte *p = __buf->begin();
    for ( usize i = from; i < __buf_valid; ++i )
      if ( p[i] == val ) return i;
    return __buf_valid;
  }

  bin_stats_t
  analyse(bin_range_t range = {}) const noexcept
  {
    bin_stats_t st{};
    const byte *p;
    usize n;
    if ( range.ptr && range.len ) {
      p = range.ptr;
      n = range.len;
    } else {
      if ( !__buf || __buf_valid == 0 ) return st;
      p = __buf->begin();
      n = __buf_valid;
    }

    st.total_bytes = n;
    usize cur_zero = 0, cur_nz = 0;

    for ( usize i = 0; i < n; ++i ) {
      byte b = p[i];
      st.freq[b]++;
      if ( b == 0x00 ) {
        st.zero_bytes++;
        if ( ++cur_zero > st.longest_zero_run ) st.longest_zero_run = cur_zero;
        if ( cur_nz ) {
          cur_nz = 0;
        }
      } else {
        if ( ++cur_nz > st.longest_nonzero_run ) st.longest_nonzero_run = cur_nz;
        if ( cur_zero ) {
          cur_zero = 0;
        }
      }
      if ( b >= 0x20 && b <= 0x7e ) st.printable_bytes++;
      if ( b >= 0x80 ) st.high_bytes++;
    }

    double H = 0.0;
    for ( usize i = 0; i < 256; ++i ) {
      if ( st.freq[i] == 0 ) continue;
      double pi = static_cast<double>(st.freq[i]) / static_cast<double>(n);
      H -= pi * __bin_impl::fast_log2(pi);
    }
    st.entropy = H;

    return st;
  }

  double
  file_entropy(void)
  {
    __require_buf();
    usize freq[256] = {};
    usize total = 0;
    posix::off_t off = 0;
    usize file_sz = static_cast<usize>(size());

    while ( static_cast<usize>(off) < file_sz ) {
      max_t n = posix::pread(__handle.fd, __buf->begin(), __buf_sz, off);
      if ( n <= 0 ) break;
      const byte *p = __buf->begin();
      for ( max_t i = 0; i < n; ++i ) freq[p[i]]++;
      total += static_cast<usize>(n);
      off += n;
    }

    double H = 0.0;
    for ( usize i = 0; i < 256; ++i ) {
      if ( !freq[i] ) continue;
      double pi = static_cast<double>(freq[i]) / static_cast<double>(total);
      H -= pi * __bin_impl::fast_log2(pi);
    }
    return H;
  }

  void
  hex_string(char *dst, usize off, usize len) const
  {
    __require_window(off, len);
    static constexpr char hex[] = "0123456789abcdef";
    const byte *p = __buf->begin() + off;
    for ( usize i = 0; i < len; ++i ) {
      dst[i * 3 + 0] = hex[(p[i] >> 4) & 0xf];
      dst[i * 3 + 1] = hex[p[i] & 0xf];
      dst[i * 3 + 2] = ' ';
    }
  }

  void
  reopen(const char *name, io::modes mode = io::modes::read)
  {
    sync();
    close();
    io::file::operator=(micron::move(io::file(name, mode)));
    __fname = name;
    __seek = (mode == io::modes::append || mode == io::modes::appendread ? static_cast<usize>(size()) : 0);
    __buf_valid = 0;
    __buf_off = 0;
  }

  void
  reopen(const T &name, io::modes mode = io::modes::read)
  {
    reopen(name.c_str(), mode);
  }

  void
  sync(void) const
  {
    posix::fsync(__handle.fd);
    posix::syncfs(__handle.fd);
  }

  bool
  operator==(const binary &o) const noexcept
  {
    return __handle.fd == o.__handle.fd;
  }

  bool
  operator!=(const binary &o) const noexcept
  {
    return !(*this == o);
  }
};
};     // namespace io
};     // namespace micron
