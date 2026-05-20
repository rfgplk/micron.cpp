

#include "../../src/gfx/platform/platform.hpp"

#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== GL PLATFORM SELECT ===");

  test_case("env() reads /proc/self/environ");
  {

    const char *path = micron::gfx::platform::env("PATH");
    require_true(path != nullptr);
    require_true(path[0] != 0);
  }
  end_test_case();

  test_case("env() returns nullptr for unknown key");
  {
    const char *v = micron::gfx::platform::env("MICRON_TEST_ENV_THAT_DOES_NOT_EXIST_999");
    require_true(v == nullptr);
  }
  end_test_case();

  test_case("select_backend returns a sensible tag");
  {
    auto t = micron::gfx::platform::select_backend();
    const char *xdg = micron::gfx::platform::env("XDG_SESSION_TYPE");
    const char *wl = micron::gfx::platform::env("WAYLAND_DISPLAY");
    const char *dpy = micron::gfx::platform::env("DISPLAY");

    if ( !wl && !dpy ) {
      require_true(t == micron::gfx::platform::backend_tag_t::none);
    } else {
      require_true(t != micron::gfx::platform::backend_tag_t::none);
    }

    (void)xdg;
  }
  end_test_case();

  test_case("backend_name returns non-null strings for every tag");
  {
    require_true(micron::gfx::platform::backend_name(micron::gfx::platform::backend_tag_t::none) != nullptr);
    require_true(micron::gfx::platform::backend_name(micron::gfx::platform::backend_tag_t::x11) != nullptr);
    require_true(micron::gfx::platform::backend_name(micron::gfx::platform::backend_tag_t::wayland) != nullptr);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
