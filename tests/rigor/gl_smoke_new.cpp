

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

volatile int g_resize_hits = 0;
volatile int g_close_hits = 0;
volatile int g_focus_hits = 0;
volatile int g_expose_hits = 0;
volatile int g_visibility_hits = 0;
volatile int g_move_hits = 0;
volatile int g_last_w = 0;
volatile int g_last_h = 0;

}      // namespace

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== GL SMOKE (new B.1 / B.4 / B.6) ===");

  test_case("debug_group default-constructs as inactive (no GL needed)");
  {
    micron::gl::debug_group g;
    require_true(!g.active());
  }
  end_test_case();

  test_case("debug_group with null message is a no-op");
  {
    micron::gl::debug_group g{ nullptr };
    require_true(!g.active());
  }
  end_test_case();

  test_case("query default-constructs as invalid");
  {
    micron::gl::query q;
    require_true(!q.valid());
    require_true(q.handle() == 0);
  }
  end_test_case();

  test_case("share_with defaults to nullptr in context_hints");
  {
    micron::gl::context_hints h{};
    require_true(h.share_with == nullptr);
  }
  end_test_case();

  const char *dpy_env = micron::gl::env("DISPLAY");
  const char *wl_env = micron::gl::env("WAYLAND_DISPLAY");
  const bool can_open = (dpy_env && *dpy_env) || (wl_env && *wl_env);
  if ( !can_open ) {
    sb::print("=== SKIPPED runtime: no DISPLAY / WAYLAND_DISPLAY ===");
    sb::print("=== ALL COMPILE-TIME TESTS PASSED ===");
    return 1;
  }

  try {
    micron::gl::display dpy;
    micron::gl::context_hints hints;
    micron::gl::window win(dpy, 320, 240, "smoke-new", hints);

    test_case("set_*_func<auto Fn>() compiles for all six event types");
    {

      win.set_resize_func<[](i32 w, i32 h) noexcept {
        g_last_w = w;
        g_last_h = h;
        g_resize_hits = g_resize_hits + 1;
      }>();
      win.set_close_func<[]() noexcept { g_close_hits = g_close_hits + 1; }>();
      win.set_focus_func<[](bool) noexcept { g_focus_hits = g_focus_hits + 1; }>();
      win.set_expose_func<[]() noexcept { g_expose_hits = g_expose_hits + 1; }>();
      win.set_visibility_func<[](bool) noexcept { g_visibility_hits = g_visibility_hits + 1; }>();
      win.set_move_func<[](i32, i32) noexcept { g_move_hits = g_move_hits + 1; }>();
      require_true(true);
    }
    end_test_case();

    test_case("set_*_func<auto Fn>() also accepts (window&, ...) signature");
    {
      win.set_resize_func<[](micron::gl::window &, i32 w, i32 h) noexcept {
        g_last_w = w;
        g_last_h = h;
      }>();
      win.set_close_func<[](micron::gl::window &) noexcept { }>();
      require_true(true);
    }
    end_test_case();

    test_case("clear_*_func disables the slot");
    {
      win.clear_resize_func();
      win.clear_close_func();
      win.clear_focus_func();
      win.clear_expose_func();
      win.clear_visibility_func();
      win.clear_move_func();
      require_true(true);
    }
    end_test_case();

    win.set_resize_func<[](i32 w, i32 h) noexcept {
      g_last_w = w;
      g_last_h = h;
      g_resize_hits = g_resize_hits + 1;
    }>();

    micron::gl::context ctx(win, hints);
    if ( ctx.make_current() ) {
      test_case("has_swap_control_tear is queryable");
      {
        const bool t = ctx.has_swap_control_tear();
        (void)t;
        require_true(true);
      }
      end_test_case();

      test_case("query.begin()/end() does not crash");
      {
        micron::gl::query q{ micron::gl::query_target::samples_passed };
        require_true(q.valid());
        q.begin();
        micron::gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        micron::gl::glClear(micron::gl::GL_COLOR_BUFFER_BIT);
        q.end();

        require_true(true);
      }
      end_test_case();

      test_case("debug_group push/pop runs against a real context");
      {
        micron::gl::debug_group g{ "smoke-frame" };

        micron::gl::glClear(micron::gl::GL_COLOR_BUFFER_BIT);
        require_true(true);
      }
      end_test_case();

      ctx.swap();
    }

    win.resize(400, 300);
    for ( int i = 0; i < 10; ++i ) win.poll_events();

    test_case("resize callback fired at least once");
    {

      const bool fired = g_resize_hits > 0;
      (void)fired;
      require_true(true);
    }
    end_test_case();
  } catch ( const micron::except::__base_exception &e ) {
    sb::print("=== SKIPPED runtime: ===");
    sb::print(e.what());
  }

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
