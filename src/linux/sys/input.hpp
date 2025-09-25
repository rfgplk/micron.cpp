//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../types.hpp"

#include "input_ids.hpp"
#include "ioctl.hpp"

#include "time.hpp"

constexpr u32 ev_version = 0x010001;

struct input_event {
  micron::timeval_t time;
  u16 type;
  u16 code;
  i32 value;
};

namespace micron
{

constexpr static const u8 uinput_version = 5;
constexpr static const u8 uinput_max_name_size = 80;
constexpr static const u8 uinput_ioctl_magic = 'U';

struct input_id_t {
  u16 bustype;
  u16 vendor;
  u16 product;
  u16 version;
};

struct input_absinfo_t {
  i32 value;
  i32 minimum;
  i32 maximum;
  i32 fuzz;
  i32 flat;
  i32 resolution;
};
struct input_keymap_entry_t {
  u8 flags;
  u8 len;
  u16 index;
  u32 keycode;
  u8 scancode[32];
};

struct input_mask_t {
  u32 type;
  u32 codes_size;
  u64 codes_ptr;
};

struct uinput_setup_t {
  input_id_t id;
  u8 name[uinput_max_name_size];
  u32 ff_effects_max;
};

struct uinput_abs_setup_t {
  u16 code; /* axis code */
  input_absinfo_t absinfo;
};

consteval u64
ui_dev_setup(void)
{
  return io_write_command<uinput_setup_t>(uinput_ioctl_magic, 3);
}

consteval u64
ui_abs_setup(void)
{
  return io_write_command<uinput_abs_setup_t>(uinput_ioctl_magic, 4);
}

consteval u64
ui_set_evbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 100);
}

consteval u64
ui_set_keybit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 101);
}

consteval u64
ui_set_relbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 102);
}

consteval u64
ui_set_absbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 103);
}

consteval u64
ui_set_mscbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 104);
}

consteval u64
ui_set_ledbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 105);
}

consteval u64
ui_set_sndbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 106);
}

consteval u64
ui_set_ffbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 107);
}

consteval u64
ui_set_phys(void)
{
  return io_write_command<char *>(uinput_ioctl_magic, 108);
}

consteval u64
ui_set_swbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 109);
}

consteval u64
ui_set_propbit(void)
{
  return io_write_command<int>(uinput_ioctl_magic, 110);
}

consteval u64
ui_dev_create(void)
{
  return io_default_command(uinput_ioctl_magic, 1);
}

consteval u64
ui_dev_dest(void)
{
  return io_default_command(uinput_ioctl_magic, 2);
}

};
