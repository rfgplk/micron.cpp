#include "../src/io/uxin/devices.hpp"
#include "../src/io/uxin/poll.hpp"
#include "../src/io/uxin/reader.hpp"
#include "../src/std.hpp"

void
key_press(const timeval&, u16 x, i32 y)
{
  if ( x == micron::btn_left )
    mc::console("Pressed lmb, ", y);
}

int
main()
{
  auto dev = mc::uxin::get_devices();
  for ( auto &n : dev ) {
    mc::console(n.path, ": ", n.name, n.type == mc::uxin::type_t::keyboard ? "keyboard" : "mouse");
    if ( n.type == mc::uxin::type_t::mouse ) {
      mc::console(n.i_path);
      mc::uxin::bind_device(n);
      mc::uxin::poll(n, mc::uxin::prepare_generic_mouse(nullptr, key_press, nullptr));
      break;
    }
  }

  return 1;
}
