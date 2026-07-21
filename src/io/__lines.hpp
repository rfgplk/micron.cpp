//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory_block.hpp"
#include "../string/strings.hpp"
#include "../types.hpp"

#include "bits.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// line iterations for fp
// line policy:
//   a) terminator '\n'; one trailing '\r' before it is stripped ("a\r\r\n" -> "a\r")
//   b) the final unterminated line is emitted; a line with no terminator keeps its '\r'
//   c) a trailing '\n' does not produce an empty final line; empty file -> no lines
//   d) lines longer than the window grow the carry buffer (memory ~ longest line)

namespace micron
{
namespace io
{
namespace __lines
{
inline constexpr usize chunk = 4096;
};      // namespace __lines

// chunked line cursor
class __line_cursor
{
  i32 __fd;
  micron::buffer __chunk;
  usize __chunk_sz;
  usize __pos = 0;        // consume offset into the window
  usize __valid = 0;      // valid bytes in the window
  micron::string __line;
  i32 __err = 0;
  bool __eof = false;

public:
  explicit __line_cursor(i32 fd, usize chunk_sz = __lines::chunk)
      : __fd(fd), __chunk(chunk_sz ? chunk_sz : __lines::chunk), __chunk_sz(chunk_sz ? chunk_sz : __lines::chunk)
  {
  }

  ~__line_cursor()
  {
    if ( __fd >= 0 && __valid > __pos ) posix::lseek(__fd, -static_cast<posix::off64_t>(__valid - __pos), posix::seek_cur);
  }

  // the cursor owns a side effect on a borrowed fd, so it is neither copyable nor movable
  __line_cursor(const __line_cursor &) = delete;
  __line_cursor &operator=(const __line_cursor &) = delete;
  __line_cursor(__line_cursor &&) = delete;
  __line_cursor &operator=(__line_cursor &&) = delete;

  bool
  next()
  {
    if ( __eof || __err != 0 ) return false;
    __line.set_size(0);
    const char *base = reinterpret_cast<const char *>(__chunk.data());
    for ( ;; ) {
      for ( usize i = __pos; i < __valid; ++i ) {
        if ( base[i] != '\n' ) continue;
        if ( i > __pos ) __line.append(base + __pos, i - __pos);
        __pos = i + 1;
        if ( __line.size() && __line[__line.size() - 1] == '\r' ) __line.set_size(__line.size() - 1);
        return true;
      }
      if ( __valid > __pos ) __line.append(base + __pos, __valid - __pos);
      __pos = __valid;
      max_t r = posix::read(__fd, __chunk.data(), __chunk_sz);
      if ( r < 0 ) [[unlikely]] {
        if ( -r == error::interrupted ) continue;
        __err = static_cast<i32>(r);
        __line.set_size(0);
        return false;
      }
      if ( r == 0 ) {
        __eof = true;
        return !__line.empty();      // final unterminated line, emitted once ('\r' kept)
      }
      __pos = 0;
      __valid = static_cast<usize>(r);
    }
  }

  // valid until the next next(); reused storage
  const micron::string &
  line() const noexcept
  {
    return __line;
  }

  i32
  error() const noexcept
  {
    return __err;
  }

  bool
  at_eof() const noexcept
  {
    return __eof;
  }
};

class lines_range
{
  __line_cursor __cur;

public:
  struct sentinel {
  };

  class iterator
  {
    lines_range *__r;      // nullptr == exhausted

  public:
    using value_type = micron::string;

    explicit iterator(lines_range *r) noexcept : __r(r) { }

    // invalidated by ++ and by range destruction
    const micron::string &
    operator*() const noexcept
    {
      return __r->__cur.line();
    }

    const micron::string *
    operator->() const noexcept
    {
      return micron::addressof(__r->__cur.line());      // hstring overloads unary operator&
    }

    iterator &
    operator++()
    {
      if ( __r && !__r->__cur.next() ) __r = nullptr;
      return *this;
    }

    bool
    operator==(sentinel) const noexcept
    {
      return __r == nullptr;
    }

    bool
    operator!=(sentinel) const noexcept
    {
      return __r != nullptr;
    }
  };

  explicit lines_range(i32 fd, usize chunk_sz = __lines::chunk) : __cur(fd, chunk_sz) { }

  lines_range(const lines_range &) = delete;
  lines_range &operator=(const lines_range &) = delete;
  lines_range(lines_range &&) = delete;
  lines_range &operator=(lines_range &&) = delete;

  iterator
  begin()
  {
    return __cur.next() ? iterator(this) : iterator(nullptr);
  }

  sentinel
  end() const noexcept
  {
    return {};
  }

  // 0 = clean EOF so far, else -errno
  i32
  error() const noexcept
  {
    return __cur.error();
  }

  explicit
  operator bool() const noexcept
  {
    return __cur.error() == 0;
  }
};

// fold callable shapes over lines
// fn(R, raw line) -> R / fn(R, materialized line) -> R
template<typename Fn, typename R>
concept line_fold_raw
    = micron::is_invocable_v<Fn, R, const char *, usize> && micron::is_same_v<micron::invoke_result_t<Fn, R, const char *, usize>, R>;

template<typename Fn, typename R>
concept line_fold_str
    = micron::is_invocable_v<Fn, R, const micron::string &> && micron::is_same_v<micron::invoke_result_t<Fn, R, const micron::string &>, R>;

template<typename Fn>
  requires micron::invocable<Fn, const micron::string &>
inline usize
__split_lines_mem(const byte *p, usize n, Fn &&fn)
{
  const char *s = reinterpret_cast<const char *>(p);
  micron::string ln;
  usize count = 0;
  usize start = 0;
  for ( usize i = 0; i < n; ++i ) {
    if ( s[i] != '\n' ) continue;
    usize len = i - start;
    if ( len && s[start + len - 1] == '\r' ) --len;
    ln.set_size(0);
    if ( len ) ln.append(s + start, len);
    fn(static_cast<const micron::string &>(ln));
    ++count;
    start = i + 1;
  }
  if ( start < n ) {      // final unterminated line: '\r' kept
    ln.set_size(0);
    ln.append(s + start, n - start);
    fn(static_cast<const micron::string &>(ln));
    ++count;
  }
  return count;
}

};      // namespace io
};      // namespace micron
