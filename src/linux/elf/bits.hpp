//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"

#if !defined(__micron_os_linux)
#error "micron::elf is linux-only"
#endif

namespace micron
{
namespace elf
{

using addr64 = u64;
using off64 = u64;
using xword = u64;
using sxword = i64;      // hehe
using word = u32;
using sword = i32;
using half = u16;

enum class fmt_class : u8 { invalid = 0, elf32 = 1, elf64 = 2 };
enum class fmt_data : u8 { invalid = 0, lsb = 1, msb = 2 };

inline constexpr u8 ident_size = 16;
inline constexpr u8 mag0 = 0x7f;
inline constexpr char mag1 = 'E';
inline constexpr char mag2 = 'L';
inline constexpr char mag3 = 'F';

inline constexpr u8 ei_mag0 = 0;
inline constexpr u8 ei_mag1 = 1;
inline constexpr u8 ei_mag2 = 2;
inline constexpr u8 ei_mag3 = 3;
inline constexpr u8 ei_class = 4;
inline constexpr u8 ei_data = 5;
inline constexpr u8 ei_version = 6;
inline constexpr u8 ei_osabi = 7;
inline constexpr u8 ei_abiversion = 8;

};      // namespace elf
};      // namespace micron
