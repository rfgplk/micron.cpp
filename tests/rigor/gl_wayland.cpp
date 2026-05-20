

#include "../../src/gfx/gl/gl.hpp"
#include "../../src/syscall.hpp"

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

void
note(const char *s) noexcept
{
  usize n = 0;
  while ( s[n] ) ++n;
  micron::syscall(SYS_write, 1, s, n);
  const char nl = '\n';
  micron::syscall(SYS_write, 1, &nl, 1);
}
}      // namespace

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== GL WAYLAND ===");

  const char *wdpy = micron::gl::env("WAYLAND_DISPLAY");
  if ( !wdpy || !*wdpy ) {
    sb::print("=== SKIPPED: WAYLAND_DISPLAY unset ===");
    return 1;
  }

  try {
    note("[1] explicit wayland_egl");

    micron::gl::display dpy(micron::gl::backend_tag_t::wayland);
    if ( dpy.backend() != micron::gl::backend_tag_t::wayland ) {
      sb::print("=== SKIPPED: Wayland socket not reachable, fell back to X11 ===");
      return 1;
    }
    note("[2] display ok");

    test_case("wayland display has compositor + shell bound");
    {
      auto *wd = dpy.as_wayland();
      require_true(wd != nullptr);
      require_true(wd->compositor() != nullptr);
      require_true(wd->shell() != nullptr);
      require_true(wd->raw_fd() >= 0);
    }
    end_test_case();
    note("[3] globals bound");

    micron::gl::context_hints hints;
    note("[4] constructing window");
    micron::gl::window win(dpy, 320, 240, "micron::gl wayland", hints);
    note("[5] window ok");

    test_case("wayland window surface chain wired");
    {
      auto *ww = win.as_wayland();
      require_true(ww != nullptr);
      require_true(ww->surface() != nullptr);
      require_true(ww->xsurface() != nullptr);
      require_true(ww->toplevel() != nullptr);
      require_true(win.wl_native() != nullptr);
    }
    end_test_case();
    note("[6] surface chain ok");

    micron::gl::context ctx(win, hints);
    note("[7] context ok");

    test_case("EGL context loads GL fn pointers");
    {
      require_true(ctx.make_current());
      require_true(micron::gl::glClear != nullptr);
      require_true(micron::gl::glDrawArrays != nullptr);
    }
    end_test_case();
    note("[8] gl loaded");

    ctx.set_swap_interval(1);

    test_case("clear + swap loop runs");
    {
      for ( int i = 0; i < 60; ++i ) {
        win.poll_events();
        if ( win.should_close() ) break;
        micron::gl::glViewport(0, 0, win.width(), win.height());
        float t = static_cast<float>(i) / 60.0f;
        micron::gl::glClearColor(0.10f + 0.4f * t, 0.20f, 0.30f, 1.0f);
        micron::gl::glClear(micron::gl::GL_COLOR_BUFFER_BIT);
        ctx.swap();
      }
      require_true(micron::gl::last_gl_error() == micron::gl::GL_NO_ERROR);
    }
    end_test_case();
    note("[9] swap loop done");
  } catch ( const micron::except::__base_exception &e ) {
    sb::print("=== SKIPPED: ===");
    sb::print(e.what());
    return 1;
  }

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
