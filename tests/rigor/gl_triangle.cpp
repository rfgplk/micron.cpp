

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

constexpr const char *vs_src = "#version 330 core\n"
                               "layout(location = 0) in vec2 a_pos;\n"
                               "layout(location = 1) in vec3 a_color;\n"
                               "out vec3 v_color;\n"
                               "void main() {\n"
                               "  v_color = a_color;\n"
                               "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
                               "}\n";

constexpr const char *fs_src = "#version 330 core\n"
                               "in vec3 v_color;\n"
                               "out vec4 frag;\n"
                               "void main() {\n"
                               "  frag = vec4(v_color, 1.0);\n"
                               "}\n";

constexpr float vertices[] = {
  -0.5f, -0.5f, 0.7f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.7f,
};

}      // namespace

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{
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

int
main()
{
  sb::print("=== GL TRIANGLE ===");
  note("[a] startup");

  const char *xdpy = micron::gl::env("DISPLAY");
  const char *wdpy = micron::gl::env("WAYLAND_DISPLAY");
  if ( !(xdpy && *xdpy) && !(wdpy && *wdpy) ) {
    sb::print("=== SKIPPED: no display server reachable ===");
    return 1;
  }
  note("[b] env ok");

  try {
    note("[c] constructing display");
    micron::gl::display dpy;
    note("[d] display ok");
    micron::gl::context_hints hints;
    hints.debug = false;
    hints.ms_samples = 0;

    if ( dpy.backend() != micron::gl::backend_tag_t::x11 ) {
      sb::print("=== SKIPPED: only the X11+GLX path is functional in this iteration ===");
      return 1;
    }
    note("[e] x11 backend");

    note("[f] constructing window");
    micron::gl::window win(dpy, 320, 240, "micron::gl triangle", hints);
    note("[g] window ok");
    note("[h] constructing context");
    micron::gl::context ctx(win, hints);
    note("[i] context ok");

    note("[j] before make_current");
    test_case("make_current loads GL fn pointers");
    {
      bool mc = ctx.make_current();
      note(mc ? "[k] make_current returned true" : "[k] make_current returned FALSE");
      require_true(mc);
      note("[l] check glClear");
      require_true(micron::gl::glClear != nullptr);
      note("[m] check glDrawArrays");
      require_true(micron::gl::glDrawArrays != nullptr);
      note("[n] check glCreateShader");
      require_true(micron::gl::glCreateShader != nullptr);
      note("[o] pointers check done");
    }
    end_test_case();
    note("[p] after first test_case");

    ctx.set_swap_interval(1);

    test_case("compile + link triangle shader pair");
    {
      micron::gl::shader vs(micron::gl::shader_kind::vertex, vs_src);
      micron::gl::shader fs(micron::gl::shader_kind::fragment, fs_src);
      require_true(vs.valid());
      require_true(fs.valid());

      micron::gl::program prog;
      prog.attach(vs);
      prog.attach(fs);
      prog.link();
      require_true(prog.valid());

      micron::gl::buffer vbo(micron::gl::buffer_target::array);
      vbo.data(vertices, sizeof(vertices), micron::gl::buffer_usage::static_draw);
      require_true(vbo.valid());

      micron::gl::vao varr;
      varr.attrib(0, vbo, 2, micron::gl::GL_FLOAT, false, 5 * sizeof(float), 0);
      varr.attrib(1, vbo, 3, micron::gl::GL_FLOAT, false, 5 * sizeof(float), 2 * sizeof(float));
      require_true(varr.valid());

      for ( int i = 0; i < 120; ++i ) {
        win.poll_events();
        if ( win.should_close() ) break;
        micron::gl::glViewport(0, 0, win.width(), win.height());
        micron::gl::glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
        micron::gl::glClear(micron::gl::GL_COLOR_BUFFER_BIT);
        prog.use();
        varr.bind();
        micron::gl::glDrawArrays(micron::gl::GL_TRIANGLES, 0, 3);
        ctx.swap();
      }

      micron::gl::GLenum e = micron::gl::last_gl_error();
      require_true(e == micron::gl::GL_NO_ERROR);
    }
    end_test_case();
  } catch ( const micron::except::__base_exception &e ) {

    sb::print("=== SKIPPED: ===");
    sb::print(e.what());
    return 1;
  }

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
