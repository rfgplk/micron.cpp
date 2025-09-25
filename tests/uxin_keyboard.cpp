#include "../src/io/uxin/uxin.hpp"
#include "../src/std.hpp"

void
key_press(const micron::timeval_t &, u16 x, i32 y)
{
  mc::console("Keypress detected: ", x, " - with state: ", y);
  mc::console("which is: ", mc::uxin::__ev_code_to_integral<char>(x));
}

int
main()
{
  try {
    auto device_type = mc::uxin::type_t::keyboard;
    auto input_handle = mc::uxin::open_nonblock(device_type);
    mc::uxin::read_rt(input_handle, mc::uxin::prepare_listener<key_press, nullptr, nullptr>(device_type));
  } catch ( mc::except::library_error &e ) {
    mc::console(e.what());
  }

  return 1;
}
