//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../linux/elf/elf.hpp"

namespace micron
{
namespace elf
{

inline handle_t
open(const char *path)
{
  return handle_t::open_path(path);
}

template<is_string T>
inline handle_t
open(const T &path)
{
  return handle_t::open_path(path.c_str());
}

inline handle_t
open_soname(const char *soname, const char *runpath = nullptr)
{
  return handle_t::open(soname, runpath);
}

template<is_string T>
inline handle_t
open_soname(const T &soname, const char *runpath = nullptr)
{
  return handle_t::open(soname.c_str(), runpath);
}

};      // namespace elf
};      // namespace micron
