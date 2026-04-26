#include "../src/std.hpp"

#include "../src/io/console.hpp"

#include "../src/string/format.hpp"

#include "../src/io/term/ansi.hpp"

int
main(void)
{
  // WTF? mc::console(mc::ansi::FE[mc::ansi::fe_code::decaln]);
  mc::console(mc::ansi::CSI[mc::ansi::csi_code::clear_screen_all]);
  // mc::console(mc::format::format(mc::ansi::OSC[mc::ansi::osc_code::set_title], "testing"));
  mc::console(mc::format::format(mc::ansi::CSI[mc::ansi::csi_code::cursor_style], 6));
  mc::console(mc::format::format(mc::ansi::CSI[mc::ansi::csi_code::cursor_up], 20));
  mc::console(mc::ansi::C0[mc::ansi::c0_code::bel]);
  //mc::console(mc::format::format(mc::ansi::CSI[mc::ansi::csi_code::cursor_forward], 50));
  for ( ;; ) mc::sleep(100);
  return 1;
}
