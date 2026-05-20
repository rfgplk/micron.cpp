

#include "../../src/gfx/gl/gl.hpp"

#include "../../src/io/console.hpp"
#include "../../src/string/istring.hpp"
#include "../../src/string/rope.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/string.hpp"
#include "../../src/string/strings.hpp"
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
  -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f,
};

namespace mg = micron::gl;
static_assert(mg::vert::kind == mg::shader_kind::vertex);
static_assert(mg::frag::kind == mg::shader_kind::fragment);
static_assert(mg::geom::kind == mg::shader_kind::geometry);
static_assert(mg::tesc::kind == mg::shader_kind::tess_control);
static_assert(mg::tese::kind == mg::shader_kind::tess_evaluation);
static_assert(mg::comp::kind == mg::shader_kind::compute);
static_assert(mg::is_shader_source_v<mg::vert>);
static_assert(mg::is_shader_source_v<mg::frag>);
static_assert(!mg::is_shader_source_v<bool>);
static_assert(!mg::is_shader_source_v<const char *>);

static_assert(micron::has_cstr<micron::sstring<8>>);
static_assert(micron::has_cstr<micron::string>);
static_assert(micron::has_cstr<micron::istring<char>>);
static_assert(micron::has_cstr<micron::rope<>>);
static_assert(!micron::is_string<micron::istring<char>>);
static_assert(micron::is_constructible_v<mg::vert, micron::sstring<8> &>);
static_assert(micron::is_constructible_v<mg::vert, micron::string &>);
static_assert(micron::is_constructible_v<mg::frag, micron::istring<char> &>);
static_assert(micron::is_constructible_v<mg::frag, micron::rope<> &>);

}      // namespace

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== GL PORCELAIN ===");

  test_case("shader_source wrappers accept const char* and every micron string");
  {
    micron::gl::vert a{ vs_src };
    require_true(a.src == vs_src);
    require_true(a.kind == micron::gl::shader_kind::vertex);

    micron::sstring<64> ss("hello");
    micron::string st("hello");
    micron::istring<char> is("hello");
    micron::rope<> rp("hello");

    micron::gl::frag b{ ss };
    micron::gl::vert c{ st };
    micron::gl::geom d{ is };
    micron::gl::comp e{ rp };

    require_true(b.src == ss.c_str());
    require_true(c.src == st.c_str());
    require_true(d.src == is.c_str());
    require_true(e.src != nullptr);
    require_true(b.kind == micron::gl::shader_kind::fragment);
    require_true(c.kind == micron::gl::shader_kind::vertex);
    require_true(d.kind == micron::gl::shader_kind::geometry);
    require_true(e.kind == micron::gl::shader_kind::compute);
  }
  end_test_case();

  const char *xdpy = micron::gl::env("DISPLAY");
  const char *wdpy = micron::gl::env("WAYLAND_DISPLAY");
  if ( !(xdpy && *xdpy) && !(wdpy && *wdpy) ) {
    sb::print("=== SKIPPED: no display server reachable ===");
    return 1;
  }

  try {
    micron::gl::context_hints hints;
    hints.debug = false;
    hints.ms_samples = 0;

    micron::gl::window win(320, 240, "micron::gl porcelain", hints);

    if ( win.dpy().backend() != micron::gl::backend_tag_t::x11 ) {
      sb::print("=== SKIPPED: only the X11+GLX path is functional in this iteration ===");
      return 1;
    }

    micron::gl::context ctx(win, true, hints);

    test_case("porcelain context made itself current");
    {
      require_true(micron::gl::glCreateShader != nullptr);
      require_true(micron::gl::glClear != nullptr);
      require_true(micron::gl::glDrawArrays != nullptr);
    }
    end_test_case();

    ctx.set_swap_interval(1);

    test_case("porcelain program: variadic attach + link");
    {

      micron::gl::program prog(micron::gl::vert{ vs_src }, micron::gl::frag{ fs_src });
      require_true(prog.valid());

      micron::gl::program prog2(false, micron::gl::vert{ vs_src });
      require_true(prog2.valid());

      micron::sstring<256> fs_str(fs_src);
      micron::gl::program prog3(micron::gl::vert{ vs_src }, micron::gl::frag{ fs_str });
      require_true(prog3.valid());

      const char *fs_ptr = fs_src;
      micron::istring<char> fs_imm(fs_ptr);
      micron::gl::program prog4(micron::gl::vert{ vs_src }, micron::gl::frag{ fs_imm });
      require_true(prog4.valid());

      micron::gl::buffer vbo(micron::gl::buffer_target::array);
      vbo.data(vertices, sizeof(vertices), micron::gl::buffer_usage::static_draw);
      require_true(vbo.valid());

      micron::gl::vao varr;
      varr.attrib(0, vbo, 2, micron::gl::GL_FLOAT, false, 5 * sizeof(float), 0);
      varr.attrib(1, vbo, 3, micron::gl::GL_FLOAT, false, 5 * sizeof(float), 2 * sizeof(float));
      require_true(varr.valid());

      for ( int i = 0; i < 60; ++i ) {
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

      require_true(micron::gl::last_gl_error() == micron::gl::GL_NO_ERROR);
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
