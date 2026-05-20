

#include "../../src/gfx/gl/platform/egl.hpp"
#include "../../src/gfx/gl/platform/glx.hpp"
#include "../../src/gfx/platform/platform.hpp"
#include "../../src/gfx/platform/wayland.hpp"
#include "../../src/gfx/platform/wayland_xdg_shell.hpp"
#include "../../src/gfx/platform/x11.hpp"

#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== GL PLATFORM COMPILE ===");

  test_case("type sizeof sanity");
  {

    static_assert(sizeof(unsigned long) >= 4);

    static_assert(sizeof(micron::gfx::gl::EGLDisplay) == sizeof(void *));
    static_assert(sizeof(micron::gfx::gl::EGLContext) == sizeof(void *));
    static_assert(sizeof(micron::gfx::gl::EGLSurface) == sizeof(void *));

    static_assert(sizeof(micron::gfx::gl::GLXContext) == sizeof(void *));
    static_assert(sizeof(micron::gfx::gl::GLXFBConfig) == sizeof(void *));

    micron::gfx::platform::wl_display *p = nullptr;
    (void)p;
    require_true(true);
  }
  end_test_case();

  test_case("backend tags compile");
  {
    static_assert(static_cast<unsigned>(micron::gfx::platform::backend_tag_t::none) == 0);
    static_assert(static_cast<unsigned>(micron::gfx::platform::backend_tag_t::x11) == 1);
    static_assert(static_cast<unsigned>(micron::gfx::platform::backend_tag_t::wayland) == 2);
    static_assert(micron::gfx::gl::glx_context_t::tag == micron::gfx::platform::backend_tag_t::x11);
    static_assert(micron::gfx::platform::wayland_window_t::tag == micron::gfx::platform::backend_tag_t::wayland);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
