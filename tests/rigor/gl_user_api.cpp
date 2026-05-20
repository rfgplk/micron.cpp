

#include "../../src/gfx/gl/gl.hpp"

#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

extern "C" void *XOpenDisplay(const char *);
extern "C" void *glXGetProcAddress(const unsigned char *);
extern "C" void *eglGetDisplay(void *);
extern "C" void *wl_display_connect(const char *);
extern "C" void *wl_egl_window_create(void *, int, int);

namespace
{
[[maybe_unused]] void *const __force_link[] = {
  reinterpret_cast<void *>(&XOpenDisplay),       reinterpret_cast<void *>(&glXGetProcAddress),    reinterpret_cast<void *>(&eglGetDisplay),
  reinterpret_cast<void *>(&wl_display_connect), reinterpret_cast<void *>(&wl_egl_window_create),
};
}

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== GL USER API ===");

  test_case("context_hints defaults are sane");
  {
    micron::gl::context_hints h{};
    require_true(h.major == 4);
    require_true(h.minor == 6);
    require_true(h.core_profile);
    require_true(h.forward_compat);
    require_true(!h.debug);
    require_true(h.double_buffer);
    require_true(h.depth_bits == 24);
    require_true(h.stencil_bits == 8);
  }
  end_test_case();

  test_case("gl_error_name covers every documented error");
  {
    auto eq = [](const char *a, const char *b) {
      while ( *a && *a == *b ) {
        ++a;
        ++b;
      }
      return *a == 0 && *b == 0;
    };
    require_true(eq(micron::gl::gl_error_name(micron::gl::GL_NO_ERROR), "GL_NO_ERROR"));
    require_true(eq(micron::gl::gl_error_name(micron::gl::GL_INVALID_ENUM), "GL_INVALID_ENUM"));
    require_true(eq(micron::gl::gl_error_name(micron::gl::GL_INVALID_VALUE), "GL_INVALID_VALUE"));
    require_true(eq(micron::gl::gl_error_name(micron::gl::GL_INVALID_OPERATION), "GL_INVALID_OPERATION"));
    require_true(eq(micron::gl::gl_error_name(micron::gl::GL_OUT_OF_MEMORY), "GL_OUT_OF_MEMORY"));
    require_true(eq(micron::gl::gl_error_name(micron::gl::GL_INVALID_FRAMEBUFFER_OPERATION), "GL_INVALID_FRAMEBUFFER_OPERATION"));
    require_true(eq(micron::gl::gl_error_name(0x9999), "GL_UNKNOWN_ERROR"));
  }
  end_test_case();

  test_case("last_gl_error returns GL_NO_ERROR pre-context");
  {

    require_true(micron::gl::last_gl_error() == micron::gl::GL_NO_ERROR);
  }
  end_test_case();

  test_case("display construction matches environment");
  {
    const char *dpy_env = micron::gl::env("DISPLAY");
    const char *wl_env = micron::gl::env("WAYLAND_DISPLAY");
    const bool can_open = (dpy_env && *dpy_env) || (wl_env && *wl_env);
    bool threw = false;
    try {
      micron::gl::display dpy;
      require_true(dpy.backend() != micron::gl::backend_tag_t::none);
      require_true(dpy.raw_fd() >= 0);

      if ( dpy.backend() == micron::gl::backend_tag_t::x11 ) {
        require_true(dpy.as_x11() != nullptr);
        require_true(dpy.as_wayland() == nullptr);
      } else {
        require_true(dpy.as_wayland() != nullptr);
        require_true(dpy.as_x11() == nullptr);
      }
    } catch ( const micron::except::__base_exception &e ) {
      threw = true;
      (void)e;
    }
    if ( can_open ) {
      require_true(!threw);
    } else {
      require_true(threw);
    }
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
