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
#include "posix/utils.hpp"

#include "entry.hpp"
#include "file.hpp"

namespace micron
{
namespace fsys
{
template <is_string T>
auto
open_file(const io::path_t &path, const io::modes mode)
{
  fsys::file<T> f(path, mode);
  return f;
}
template <is_string T>
void
open_dir(const io::path_t &path, const io::modes mode)
{
  // fsys::dir<T> f(path, mode);
  // return f;
}

bool
exists(const io::path_t &path)
{
  return (posix::access(path.c_str(), posix::access_ok) == 0);
}

bool
valid_path(const io::path_t &path)
{
  return (posix::access(path.c_str(), posix::access_ok) == 0);
}

bool
readable_at(const io::path_t &path)
{
  return (posix::access(path.c_str(), posix::read_ok) == 0);
}

bool
writeable_at(const io::path_t &path)
{
  return (posix::access(path.c_str(), posix::write_ok) == 0);
}

bool
executable_at(const io::path_t &path)
{
  return (posix::access(path.c_str(), posix::execute_ok) == 0);
}

void
rename(const io::path_t &from, const io::path_t &to)
{
  posix::rename(from.c_str(), to.c_str());
}
// provide both
void
move(const io::path_t &from, const io::path_t &to)
{
  posix::rename(from.c_str(), to.c_str());
}

void
copy(const io::path_t &from, const io::path_t &to)
{
  auto from_f = open_file<micron::str8>(from, io::modes::read);
  auto to_f = open_file<micron::str8>(to, io::modes::readwritecreate);
  micron::str8 buf;
  from_f >> buf;
  to_f = buf;
}

void
make(const io::path_t &name)
{
  open_file<micron::str8>(name, io::modes::create);
}

template <typename... Paths>
void
copy_list(const io::path_t &from, const Paths &...to)
{
  (copy(from, to), ...);
}
// FILE TYPE FUNCS
auto
file_type_at(const io::path_t &p)
{
  return io::get_type_at(p.c_str());
}
};
};
