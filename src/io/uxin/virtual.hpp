//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"

#include "../../linux/sys/event_codes.hpp"
#include "../../linux/sys/input.hpp"
#include "../../linux/sys/ioctl.hpp"
#include "../../types.hpp"
#include "../posix/iosys.hpp"

#include "../../errno.hpp"

namespace micron
{
namespace uxin
{
io::fd_t
make_uinput(void)
{
  int r = 0;
  io::fd_t handle(posix::open("/dev/uinput", o_nonblock | o_wronly));
  if ( handle.has_error() )
    set_errno(5);
  return handle;
}

template <typename... Args>
  requires((micron::is_same_v<Args, byte> && ...))
void
set_events(const io::fd_t &handle, Args... args)
{
  int r = 0;
  if ( r = (micron::ioctl(handle.fd, micron::ui_set_evbit(), args), ...); r != 0 ) {
    set_errno(-r);
  }
}

template <typename... Args>
  requires((micron::is_same_v<Args, byte> && ...))
void
set_keys(const io::fd_t &handle, Args... args)
{
  if ( r = (micron::ioctl(handle.fd, micron::ui_set_keybit(), args), ...); r != 0 )
    set_errno(-r);
}

void
start_device(const io::fd_t &handle, const uinput_setup_t &usetup)
{
  if ( r = micron::ioctl(handle.fd, ui_dev_setup(), &usetup); r != 0 ) {
    set_errno(-r);
  }
  if ( r = micron::ioctl(handle.fd, ui_dev_create()); r != 0 ) {
    set_errno(-r);
  }
}

void
start_device(const io::fd_t &handle, const char *name, u16 vendor, u16 product, u16 version)
{
  uinput_setup_t usetup{ .id = { .bustype = bus_usb, .vendor = vendor, .product = product, .version = version },
                         .name = name,
                         .ff_effects_max = 0 };
  if ( r = micron::ioctl(handle.fd, ui_dev_setup(), &usetup); r != 0 ) {
    set_errno(-r);
  }
  if ( r = micron::ioctl(handle.fd, ui_dev_create()); r != 0 ) {
    set_errno(-r);
  }
}

};
};
