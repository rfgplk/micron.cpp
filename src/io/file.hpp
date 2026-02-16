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

#include "entry.hpp"

#include "console.hpp"

namespace micron
{
namespace fsys
{
// opens file on construct, closes on destruct
// does NOT load data until load() or read() are called
template <is_string T = micron::string> class file : public io::file     // if not using posix systems swap this out
{
  T data;
  micron::sstr<io::max_name> fname;
  size_t seek;                           // seek start
  size_t buffer_sz;                      // size of the internal buffer
  micron::shared<micron::buffer> bf;     // internal buffer for buffering
public:
  ~file() = default;
  file() : io::file(), data(), fname(), seek(0), buffer_sz(0), bf(nullptr) {}
  file(const T &name, const io::modes mode)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append or mode == io::modes::appendread ? size() : 0),
        buffer_sz(0), bf(nullptr)
  {
  }
  file(const char *name, const io::modes mode)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append or mode == io::modes::appendread ? size() : 0),
        buffer_sz(0), bf(nullptr)
  {
  }
  template <size_t M>
  file(const char (&name)[M], const io::modes mode)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append or mode == io::modes::appendread ? size() : 0),
        buffer_sz(0), bf(nullptr)
  {
  }

  file(const T &name, const io::modes mode, const size_t _bf)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append or mode == io::modes::appendread ? size() : 0),
        buffer_sz(_bf), bf(new micron::buffer(_bf))
  {
  }
  file(const char *name, const io::modes mode, const size_t _bf)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append or mode == io::modes::appendread ? size() : 0),
        buffer_sz(_bf), bf(new micron::buffer(_bf))
  {
  }
  template <size_t M>
  file(const char (&name)[M], const io::modes mode, const size_t _bf)
      : io::file(name, mode), data(), fname(name), seek(mode == io::modes::append or mode == io::modes::appendread ? size() : 0),
        buffer_sz(_bf), bf(new micron::buffer(_bf))
  {
  }

  file(const T &name) : io::file(name, io::modes::read), data(), fname(name), seek(0), buffer_sz(0), bf(nullptr) {}
  file(const char *name) : io::file(name, io::modes::read), data(), fname(name), seek(0), buffer_sz(0), bf(nullptr) {}
  template <size_t M> file(const char (&name)[M]) : io::file(name, io::modes::read), data(), fname(name), seek(0), buffer_sz(0), bf(nullptr)
  {
  }
  // reopens file in place
  void
  reopen(const T &name, const io::modes mode, const size_t _bf)
  {
    sync();
    close();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append or mode == io::modes::appendread ? size() : 0);
    buffer_sz = _bf;
    bf = micron::move(buffer(new micron::buffer(_bf)));
  }
  void
  reopen(const char *name, const io::modes mode, const size_t _bf)
  {
    sync();
    close();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append or mode == io::modes::appendread ? size() : 0);
    buffer_sz = _bf;
    bf = micron::move(buffer(new micron::buffer(_bf)));
  }
  void
  reopen(const T &name, const io::modes mode)
  {
    sync();
    close();
    data.clear();
    io::file::operator=(micron::move(io::file(name, mode)));
    fname = name;
    seek = (mode == io::modes::append or mode == io::modes::appendread ? size() : 0);
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
    seek = (mode == io::modes::append or mode == io::modes::appendread ? size() : 0);
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
    // reset internal container
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
  // read into operators
  file &
  operator>>(T &str)
  {
    str = load_and_pull();
    return *this;
  }
  // write to operators, both via << and =
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
  set(const size_t s)
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
  size_t
  seek_pos(void) const
  {
    return seek;
  }
  // load buffered, starting from seek
  void
  read_bytes(size_t sz)
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");

    data.reserve(sz + 1);
    do {
      posix::lseek(__handle.fd, seek, seek_set);
      ssize_t bytes_read = io::read(__handle.fd, &(*bf), ((buffer_sz > sz) ? sz : buffer_sz));
      if ( bytes_read == 0 )
        break;
      if ( bytes_read == -1 ) [[unlikely]]
        exc<except::io_error>("micron::fsys::file error reading file");
      seek += static_cast<size_t>(bytes_read);
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
    return;
  }
  // full file
  void
  load(void)
  {
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");
    auto s = size();
    data.reserve(s + 1);

    posix::lseek(__handle.fd, 0, seek_set);
    ssize_t read_bytes = 0;
    do {
      read_bytes = io::read(__handle.fd, &data, s);
      if ( read_bytes < (s) )
        break;
    } while ( read_bytes );
    data._buf_set_length(s);
  }
  // this is needed since if the file is virtual, it doesn't have the appropriate stat inodes
  void
  load_kernel(void)
  {
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");
    posix::lseek(__handle.fd, 0, seek_set);
    size_t s = 0;
    ssize_t read_bytes = 0;
    do {
      // On success, the number of bytes read is returned (zero indicates end of file), and the file position is advanced
      // by this number.
      read_bytes = io::read(__handle.fd, &data[s], 1);
      s += read_bytes;
      if ( s == data.max_size() )
        data.reserve(data.max_size() * 3);
    } while ( read_bytes );
    if ( s )
      data._buf_set_length(s);
  }
  // will write to wherever the seek is (so be careful)
  // writes the contents of data to the file
  // if data hasn't changed nothing is different
  // if it isn't opened for writing throw an error
  // don't reset data on write
  void
  write(void)
  {
    // nothing to write cancel
    if ( !data )
      return;
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open");
    posix::lseek(__handle.fd, seek, seek_set);
    ssize_t sz = io::write(__handle.fd, &data, data.size());
    if ( sz == -1 )
      exc<except::filesystem_error>("micron::fsys::file wasn't able to write to fd");
    if ( sz < (ssize_t)data.size() )
      exc<except::filesystem_error>("micron::fsys::file wasn't able to write to fd");
    seek += sz;
    posix::lseek(__handle.fd, seek, seek_set);
  }
  auto
  buffer_size() const
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    return buffer_sz;
  }
  // load buffer
  void
  load_buffer(const byte *b, const size_t n)
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    if ( n > buffer_sz )
      exc<except::filesystem_error>("micron::fsys::file trying to load buffer with too much data");
    micron::bytecpy(&(*bf), b, n);
  }
  void
  write_bytes(size_t sz)
  {
    if ( !bf )
      exc<except::filesystem_error>("micron::fsys::file trying to use file buffering despite the file being opened in unbuffered mode");
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open'");

    posix::lseek(__handle.fd, seek, seek_set);
    ssize_t bytes_written = io::write(__handle.fd, &(*bf), ((buffer_sz > sz) ? sz : buffer_sz));
    if ( bytes_written == -1 ) [[unlikely]]
      exc<except::io_error>("micron::fsys::file error writing to file");
    seek += static_cast<size_t>(bytes_written);
    sz -= ((buffer_sz > sz) ? sz : buffer_sz);
    do {
    } while ( sz );
  }
  // saves the entire file and wipes data
  // this is useful if you want to reset the container later anyways
  void
  flush(void)
  {
    // nothing to write cancel
    if ( !data )
      return;
    if ( __handle.closed() )
      exc<except::filesystem_error>("micron::fsys::file fd isn't open");
    posix::lseek(__handle.fd, seek, seek_set);
    size_t sz = io::write(__handle.fd, &data, data.size());
    if ( sz < data.size() )
      exc<except::filesystem_error>("micron::fsys::file wasn't able to write to fd");
    seek += sz;
    posix::lseek(__handle.fd, seek, seek_set);
    clear();
  }
  size_t
  count(void) const
  {
    return data.size();
  }
  void
  push_copy(const T &str)
  {
    data = str;
  }

  // pushes a new container into data, for writing
  void
  push(T &&str)
  {
    data = micron::move(str);
  }
  // pulls the string out of the file for later use
  auto
  pull(void)
  {
    auto tmp = micron::move(data);
    data = micron::ustr8();
    return tmp;
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
  // get will only return a const ref
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
  void
  sync(void) const
  {
    posix::fsync(__handle.fd);
    posix::syncfs(__handle.fd);
  }
};
};
};
