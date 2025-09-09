#pragma once

#include "../../string/format.hpp"
#include "../filesystem.hpp"
#include "../fsys.hpp"

namespace micron
{
namespace uxin
{

struct devices_t;

void
get_devices()
{
  auto ps = micron::io::path("/sys/class/input/").all();
  for ( auto &n : ps ) {
    if ( format::ends_with(n, "mouse") )
      micron::console(n);
  }
  fsys::system<micron::io::rd> sys;
  micron::ustr8 buf;
  sys["/sys/class/input/mouse0/device/uevent"] >> buf;
  auto ev = micron::format::find(buf, "EV");
  auto ev_eol = micron::format::find(buf, ev, "\n");
  auto key = micron::format::find(buf, "KEY");
  auto key_eol = micron::format::find(buf, key, "\n");
  auto rel = micron::format::find(buf, "REL");
  auto rel_eol = micron::format::find(buf, rel, "\n");
  auto ev_line = buf.substr(ev, ev_eol);
  auto key_line = buf.substr(key, key_eol);
  auto rel_line = buf.substr(rel, rel_eol);
  console(ev_line);
  console(key_line);
  console(rel_line);
}
auto bind_device();

};
};
