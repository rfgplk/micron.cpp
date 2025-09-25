#include "../src/io/uxin/uxin.hpp"
#include "../src/std.hpp"

void
mouse_move(const micron::timeval_t &, u16 x, i32 y)
{
  if ( x == micron::abs_x ) {
    mc::console("Mouse movement detected, x: ", y);
  }
  if ( x == micron::abs_y ) {
    mc::console("Mouse movement detected, y: ", y);
  }
}

int
main()
{
  try {
  auto device_type = mc::uxin::type_t::mouse;
  auto input_handle = mc::uxin::open(device_type);
  mc::uxin::read(input_handle, mc::uxin::prepare_listener(device_type));
  }
  catch(mc::except::memory_error& e)
  {
    mc::console(e.what());
  }
  // auto dev = mc::uxin::get_devices();
  // for ( auto &n : dev ) {
  //   mc::console(n.path, ": ", n.name, n.type == mc::uxin::type_t::keyboard ? "keyboard" : "mouse");
  //   if ( n.type == mc::uxin::type_t::mouse ) {
  //     mc::console(n.i_path);
  //     mc::uxin::nonblock_bind_device(n);
  //     mc::uxin::poll(n, mc::uxin::prepare_generic_mouse_sensor_abs(nullptr, mouse_move, nullptr));
  //     break;
  //   }
  // }

  return 1;
}
