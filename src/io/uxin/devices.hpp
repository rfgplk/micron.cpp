//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../io/posix/iosys.hpp"
#include "../../linux/sys/event_codes.hpp"

#include "../../string/format.hpp"
#include "../../string/unistring.hpp"
#include "../../vector/fvector.hpp"
#include "../filesystem.hpp"
#include "../fsys.hpp"

namespace micron
{
namespace uxin
{

auto
__parse_device(const char *s, const micron::ustr8 &buf)
{
  auto tk = micron::format::find(buf, s);
  auto del = micron::format::find(buf, tk, '=');
  auto del_eol = micron::format::find(buf, del, '\n');
  return buf.substr(del + 1, del_eol);
}

bool
__has_alnum_keys(const micron::ustr8 &buf)
{
  size_t __count = 0;
  auto _start = micron::format::find(buf, ' ');
  auto _ss = buf.substr(buf.begin(), _start);
  while ( _start != nullptr ) {
    i64 k = hex_string_to_int64(_ss);
    byte __first = key_1;
    byte __last = key_m;
    for ( ; __first < __last; __first++ ) {
      if ( k & (1 << __first) )
        __count++;
    }
    auto _next = micron::format::find(buf, _start + 1, ' ');
    if ( _next == nullptr )
      break;
    _ss = micron::move(buf.substr(_start + 1, _next));
    _start = _next;
  }
  return (__count > 27);
}

enum class type_t : char { keyboard, mouse };

struct device_t {
  ustr8 i_path;
  ustr8 path;
  ustr8 name;
  ustr8 phys;
  type_t type;
  int bound_fd;
};

fvector<device_t>
get_devices()
{
  micron::fvector<device_t> vc;
  fsys::system<micron::io::rd> sys;
  auto ps = micron::io::path("/sys/class/input/").all();
  for ( auto &n : ps ) {
    auto loc = format::find(n, "event");
    if ( loc != nullptr ) {
      // /sys/class/input/[eventX]/device/uevent
      micron::ustr8 pt = "/sys/class/input/" + n + "/device/uevent";
      micron::ustr8 buf;
      sys[pt] >> buf;

      if ( !format::contains(buf, "EV") )
        continue;
      i64 ev = hex_string_to_int64(__parse_device("EV", buf));
      if ( ev & (1 << ev_key) )     // likely keyboard
      {
        if ( format::contains(buf, "KEY") ) {
          if ( __has_alnum_keys(__parse_device("KEY", buf)) )     // likely keyboard, bypass parse
          {
            auto name = __parse_device("NAME", buf);
            auto phys = __parse_device("PHYS", buf);
            vc.emplace_back("/dev/input/" + n, "/sys/class/input/" + n, name, phys, type_t::keyboard, -1);
            continue;
          }
        }
      }
      if ( !format::contains(buf, "REL") )
        continue;
      if ( (ev & (1 << ev_rel) and ev & (1 << ev_abs)) and ev & (1 << ev_syn) )     // likely mouse
      {
        auto name = __parse_device("NAME", buf);
        auto phys = __parse_device("PHYS", buf);
        vc.emplace_back("/dev/input/" + n, "/sys/class/input/" + n, name, phys, type_t::mouse, -1);
        continue;
      }
    }
  }
  return vc;
}

// start reading from device
int
bind_device(device_t &dev)
{
  // WARNING: need either CAP_SYS_ADMIN or CAP_SYS_RAWIO to be able to read raw character devices.
  // WILL FAIL OTHERWISE
  if ( dev.i_path.empty() )
    return -1;
  int fd = static_cast<int>(posix::open(dev.i_path.c_str(), O_RDONLY));
  if ( fd < 0 )
    return fd;
  dev.bound_fd = fd;
  return fd;
}
void
unbind_device(device_t &dev)
{
  if ( dev.bound_fd != -1 )
    posix::close(dev.bound_fd);
  dev.bound_fd = -1;
}
};
};
