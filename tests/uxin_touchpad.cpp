#include "../src/io/uxin/devices.hpp"
#include "../src/io/uxin/poll.hpp"
#include "../src/io/uxin/reader.hpp"
#include "../src/std.hpp"

void
motion(const micron::timeval_t &, u16 code, i32 val)
{
  if ( code == micron::abs_x )
    mc::console("touchpad X: ", val);
  else if ( code == micron::abs_y )
    mc::console("touchpad Y: ", val);
  else if ( code == micron::abs_mt_position_x )
    mc::console("  mt X: ", val);
  else if ( code == micron::abs_mt_position_y )
    mc::console("  mt Y: ", val);
}

void
button(const micron::timeval_t &, u16 code, i32 val)
{
  if ( code == micron::btn_touch )
    mc::console("finger ", val ? "down" : "up");
  else if ( code == micron::btn_left )
    mc::console("click ", val);
  else if ( code == micron::btn_tool_finger )
    mc::console("one finger ", val);
  else if ( code == micron::btn_tool_doubletap )
    mc::console("two fingers ", val);
  else if ( code == micron::btn_tool_tripletap )
    mc::console("three fingers ", val);
}

const char *
type_name(mc::uxin::type_t t)
{
  switch ( t ) {
  case mc::uxin::type_t::keyboard:
    return "keyboard";
  case mc::uxin::type_t::mouse:
    return "mouse";
  case mc::uxin::type_t::touchpad:
    return "touchpad";
  case mc::uxin::type_t::touchscreen:
    return "touchscreen";
  case mc::uxin::type_t::joystick:
    return "joystick";
  case mc::uxin::type_t::tablet:
    return "tablet";
  case mc::uxin::type_t::pointing_stick:
    return "pointing_stick";
  default:
    return "unknown";
  }
}

int
main()
{
  auto dev = mc::uxin::get_devices();
  for ( auto &n : dev ) mc::console(n.path, ": ", n.name, " [", type_name(n.type), "]");

  for ( auto &n : dev ) {
    if ( n.type == mc::uxin::type_t::touchpad ) {
      mc::console("binding touchpad: ", n.i_path);
      mc::uxin::bind_device(n);

      mc::uxin::poll(n, mc::uxin::prepare_generic_touchpad_sensor(motion), mc::uxin::prepare_generic_touchpad(button));
      return 0;
    }
  }
  mc::console("no touchpad detected");
  return 0;
}
