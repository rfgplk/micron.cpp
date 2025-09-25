//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../string/strings.hpp"
#include "io.hpp"
#include "paths.hpp"
#include "../type_traits.hpp"

namespace micron
{
namespace io
{
// wrapper for unidirectional pipes, to be used for procs
class upipe
{
  enum class utype { upipe_reader = 0, upipe_writer = 1 };
  inline constexpr auto
  ic(utype t) -> int
  {
    return static_cast<int>(t);
  }
  int fd[2];     // pipe in and out, 0 read 1 write
  utype tp;

public:
  ~upipe()
  {
    posix::close(fd[ic(utype::upipe_reader)]);
    posix::close(fd[ic(utype::upipe_writer)]);
  }
  upipe(utype t)
  {
    if ( posix::pipe(fd) == -1 )
      throw except::io_error("micron::pipe failed to open pipe");
    tp = t;
  }
  upipe()
  {
    if ( posix::pipe(fd) == -1 )
      throw except::io_error("micron::pipe failed to open pipe");
    tp = utype::upipe_writer;
  }
  upipe(const upipe &) = default;
  upipe(upipe &&) = default;
  upipe &operator=(const upipe &) = default;
  upipe &operator=(upipe &&) = default;
  void
  make_reader(void)
  {
    tp = utype::upipe_reader;
  }
  void
  make_writer(void)
  {
    tp = utype::upipe_writer;
  }
  auto
  get(void) const
  {
    return fd;
  }
  // read/write to T
  template <is_string T>
  void
  operator()(T &t)
  {
    if ( tp == utype::upipe_reader )
      while ( read(fd[ic(utype::upipe_reader)], &t[0], 1024 > t.size() ? t.size() : 1024) > 0 ) {
      }
    else if ( tp == utype::upipe_writer )
      while ( write(fd[ic(utype::upipe_reader)], &t[0], t.size()) > 0 ) {
      }
  }
  template <typename T>
    requires(micron::is_object_v<T>) && (micron::is_convertible_v<T, typename T::value_type>)
  void
  operator()(T &t)
  {
    if ( tp == utype::upipe_reader )
      while ( read(fd[ic(utype::upipe_reader)], &t[0], 1024 > t.size() ? t.size() : 1024) > 0 ) {
      }
    else if ( tp == utype::upipe_writer )
      while ( write(fd[ic(utype::upipe_reader)], &t[0], t.size()) > 0 ) {
      }
  }
  void
  operator()(byte *t, size_t sz)
  {
    if ( tp == utype::upipe_reader )
      while ( read(fd[ic(utype::upipe_reader)], t, sz) > 0 ) {
      }
    else if ( tp == utype::upipe_writer )
      while ( write(fd[ic(utype::upipe_reader)], t, sz) > 0 ) {
      }
  }
};

// wrapper for creating a named pipe
class npipe
{
  micron::string pipe_name;     // name
  int fd; //fd of open pipe
public:
  ~npipe() { micron::unlink(pipe_name.c_str()); posix::close(fd); }
  npipe(const micron::string &str, int perms = 0666) : pipe_name(str)
  {
    if ( micron::mkfifo(pipe_name.c_str(), perms) == -1 )
      throw except::io_error("micron::npipe(mkfifo) failed to create pipe");
    fd = posix::open(pipe_name.c_str(), o_rdwr);
    if(fd == -1)
      throw except::io_error("micron::npipe(open) failed to open pipe file");
  }
  npipe(const npipe &) = default;
  npipe(npipe &&) = default;
  npipe &operator=(const npipe &) = default;
  npipe &operator=(npipe &&) = default;
  auto
  get(void) const
  {
    return fd;
  }
  // read/write to T
  template <typename T>
    requires(micron::is_object_v<T>) && (micron::is_convertible_v<T, typename T::value_type>)
  void
  write(T &t)
  {
    //write(fd, t, t.size());
  }
  void
  write(byte *t, size_t sz)
  {
    //write(fd, t, sz); 
  }
  // read/write to T
  template <typename T>
    requires(micron::is_object_v<T>) && (micron::is_convertible_v<T, typename T::value_type>)
  void
  read(T &t)
  {
    //read(fd, t, t.size());
  }
  void
  read(byte *t, size_t sz)
  {
    //read(fd, t, sz); 
  }
};

};
};
