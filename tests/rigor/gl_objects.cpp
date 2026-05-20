

#include "../../src/gfx/gl/gl.hpp"

#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{

using namespace micron::gl;

struct stub_state_t {
  GLuint next_handle = 1;
  GLuint last_created_shader = 0;
  GLuint last_created_program = 0;
  GLuint last_bound_buffer = 0;
  GLenum last_buffer_target = 0;
  GLsizeiptr last_buffer_size = 0;
  GLuint last_compiled_shader = 0;
  GLuint last_used_program = 0;
  GLuint last_deleted_shader = 0;
  GLuint last_deleted_program = 0;
  GLsizei delete_buffers_count = 0;
  GLsizei delete_vaos_count = 0;
  GLsizei delete_textures_count = 0;
  GLuint last_bound_vao = 0;
  GLuint last_bound_texture = 0;
  GLenum last_texture_target = 0;
  GLfloat last_clear_color[4] = { 0, 0, 0, 0 };
  GLbitfield last_clear_mask = 0;
  GLenum last_draw_mode = 0;
  GLsizei last_draw_count = 0;
};

stub_state_t g_stub{};

GLuint
__stub_create_shader(GLenum) noexcept
{
  g_stub.last_created_shader = g_stub.next_handle++;
  return g_stub.last_created_shader;
}

void
__stub_shader_source(GLuint, GLsizei, const GLchar *const *, const GLint *) noexcept
{
}

void
__stub_compile_shader(GLuint s) noexcept
{
  g_stub.last_compiled_shader = s;
}

void
__stub_get_shaderiv(GLuint, GLenum, GLint *v) noexcept
{
  *v = GL_TRUE;
}

void
__stub_get_shader_log(GLuint, GLsizei, GLsizei *, GLchar *) noexcept
{
}

void
__stub_delete_shader(GLuint s) noexcept
{
  g_stub.last_deleted_shader = s;
}

GLuint
__stub_create_program() noexcept
{
  g_stub.last_created_program = g_stub.next_handle++;
  return g_stub.last_created_program;
}

void
__stub_attach_shader(GLuint, GLuint) noexcept
{
}

void
__stub_link_program(GLuint) noexcept
{
}

void
__stub_get_programiv(GLuint, GLenum, GLint *v) noexcept
{
  *v = GL_TRUE;
}

void
__stub_get_program_log(GLuint, GLsizei, GLsizei *, GLchar *) noexcept
{
}

void
__stub_use_program(GLuint p) noexcept
{
  g_stub.last_used_program = p;
}

void
__stub_delete_program(GLuint p) noexcept
{
  g_stub.last_deleted_program = p;
}

void
__stub_gen_buffers(GLsizei n, GLuint *out) noexcept
{
  for ( GLsizei i = 0; i < n; ++i ) out[i] = g_stub.next_handle++;
}

void
__stub_delete_buffers(GLsizei n, const GLuint *) noexcept
{
  g_stub.delete_buffers_count += n;
}

void
__stub_bind_buffer(GLenum target, GLuint b) noexcept
{
  g_stub.last_buffer_target = target;
  g_stub.last_bound_buffer = b;
}

void
__stub_buffer_data(GLenum, GLsizeiptr size, const void *, GLenum) noexcept
{
  g_stub.last_buffer_size = size;
}

void
__stub_buffer_subdata(GLenum, GLintptr, GLsizeiptr, const void *) noexcept
{
}

void
__stub_gen_vaos(GLsizei n, GLuint *out) noexcept
{
  for ( GLsizei i = 0; i < n; ++i ) out[i] = g_stub.next_handle++;
}

void
__stub_delete_vaos(GLsizei n, const GLuint *) noexcept
{
  g_stub.delete_vaos_count += n;
}

void
__stub_bind_vao(GLuint v) noexcept
{
  g_stub.last_bound_vao = v;
}

void
__stub_enable_attrib(GLuint) noexcept
{
}

void
__stub_vertex_attrib_pointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *) noexcept
{
}

void
__stub_gen_textures(GLsizei n, GLuint *out) noexcept
{
  for ( GLsizei i = 0; i < n; ++i ) out[i] = g_stub.next_handle++;
}

void
__stub_delete_textures(GLsizei n, const GLuint *) noexcept
{
  g_stub.delete_textures_count += n;
}

void
__stub_bind_texture(GLenum t, GLuint h) noexcept
{
  g_stub.last_texture_target = t;
  g_stub.last_bound_texture = h;
}

void
__stub_tex_storage_2d(GLenum, GLsizei, GLenum, GLsizei, GLsizei) noexcept
{
}

void
__stub_tex_parameter_i(GLenum, GLenum, GLint) noexcept
{
}

void
__stub_clear(GLbitfield mask) noexcept
{
  g_stub.last_clear_mask = mask;
}

void
__stub_clear_color(GLclampf r, GLclampf g, GLclampf b, GLclampf a) noexcept
{
  g_stub.last_clear_color[0] = r;
  g_stub.last_clear_color[1] = g;
  g_stub.last_clear_color[2] = b;
  g_stub.last_clear_color[3] = a;
}

void
__stub_draw_arrays(GLenum mode, GLint, GLsizei count) noexcept
{
  g_stub.last_draw_mode = mode;
  g_stub.last_draw_count = count;
}

void
install_stubs()
{
  using namespace micron::gl;
  glCreateShader = &__stub_create_shader;
  glShaderSource = &__stub_shader_source;
  glCompileShader = &__stub_compile_shader;
  glGetShaderiv = &__stub_get_shaderiv;
  glGetShaderInfoLog = &__stub_get_shader_log;
  glDeleteShader = &__stub_delete_shader;
  glCreateProgram = &__stub_create_program;
  glAttachShader = &__stub_attach_shader;
  glLinkProgram = &__stub_link_program;
  glGetProgramiv = &__stub_get_programiv;
  glGetProgramInfoLog = &__stub_get_program_log;
  glUseProgram = &__stub_use_program;
  glDeleteProgram = &__stub_delete_program;
  glGenBuffers = &__stub_gen_buffers;
  glDeleteBuffers = &__stub_delete_buffers;
  glBindBuffer = &__stub_bind_buffer;
  glBufferData = &__stub_buffer_data;
  glBufferSubData = &__stub_buffer_subdata;
  glGenVertexArrays = &__stub_gen_vaos;
  glDeleteVertexArrays = &__stub_delete_vaos;
  glBindVertexArray = &__stub_bind_vao;
  glEnableVertexAttribArray = &__stub_enable_attrib;
  glVertexAttribPointer = &__stub_vertex_attrib_pointer;
  glGenTextures = &__stub_gen_textures;
  glDeleteTextures = &__stub_delete_textures;
  glBindTexture = &__stub_bind_texture;
  glTexStorage2D = &__stub_tex_storage_2d;
  glTexParameteri = &__stub_tex_parameter_i;
  glClear = &__stub_clear;
  glClearColor = &__stub_clear_color;
  glDrawArrays = &__stub_draw_arrays;
}

}      // namespace

int
main()
{
  sb::print("=== GL OBJECTS ===");
  install_stubs();

  test_case("shader create + compile + destroy");
  {
    micron::gl::shader vs(micron::gl::shader_kind::vertex, "void main(){}");
    require_true(vs.valid());
    require_true(g_stub.last_created_shader != 0);
    GLuint h = vs.handle();
    require_true(g_stub.last_compiled_shader == h);

    micron::gl::shader vs2(static_cast<micron::gl::shader &&>(vs));
    require_true(vs2.handle() == h);
    require_true(vs.handle() == 0);
  }

  require_true(g_stub.last_deleted_shader != 0);
  end_test_case();

  test_case("program create + link + use + destroy");
  {
    g_stub.last_deleted_program = 0;
    micron::gl::program prog;
    require_true(prog.valid());
    GLuint h = prog.handle();
    micron::gl::shader vs(micron::gl::shader_kind::vertex, "void main(){}");
    micron::gl::shader fs(micron::gl::shader_kind::fragment, "void main(){}");
    prog.attach(vs);
    prog.attach(fs);
    prog.link();
    prog.use();
    require_true(g_stub.last_used_program == h);
  }
  require_true(g_stub.last_deleted_program != 0);
  end_test_case();

  test_case("buffer gen + data + bind + destroy");
  {
    g_stub.delete_buffers_count = 0;
    micron::gl::buffer vbo(micron::gl::buffer_target::array);
    require_true(vbo.valid());
    float xs[6] = { 0, 0, 1, 0, 0, 1 };
    vbo.data(xs, sizeof(xs), micron::gl::buffer_usage::static_draw);
    require_true(g_stub.last_buffer_size == static_cast<GLsizeiptr>(sizeof(xs)));
    require_true(g_stub.last_buffer_target == micron::gl::GL_ARRAY_BUFFER);
    require_true(g_stub.last_bound_buffer == vbo.handle());
  }
  require_true(g_stub.delete_buffers_count == 1);
  end_test_case();

  test_case("vao gen + attrib + bind + destroy");
  {
    g_stub.delete_vaos_count = 0;
    g_stub.delete_buffers_count = 0;
    micron::gl::vao varr;
    require_true(varr.valid());
    micron::gl::buffer vbo(micron::gl::buffer_target::array);
    float xs[6] = { 0 };
    vbo.data(xs, sizeof(xs), micron::gl::buffer_usage::static_draw);
    varr.attrib(0, vbo, 2, micron::gl::GL_FLOAT, false, 0, 0);
    varr.bind();
    require_true(g_stub.last_bound_vao == varr.handle());
  }
  require_true(g_stub.delete_vaos_count == 1);
  require_true(g_stub.delete_buffers_count == 1);
  end_test_case();

  test_case("texture create + bind + destroy");
  {
    g_stub.delete_textures_count = 0;
    micron::gl::texture tex(micron::gl::tex_target::d2);
    require_true(tex.valid());
    tex.storage_2d(1, micron::gl::GL_RGBA8, 64, 64);
    tex.bind();
    require_true(g_stub.last_texture_target == micron::gl::GL_TEXTURE_2D);
    require_true(g_stub.last_bound_texture == tex.handle());
  }
  require_true(g_stub.delete_textures_count == 1);
  end_test_case();

  test_case("draw chain: program use + vao bind + drawArrays");
  {
    micron::gl::program prog;
    micron::gl::shader vs(micron::gl::shader_kind::vertex, "void main(){}");
    micron::gl::shader fs(micron::gl::shader_kind::fragment, "void main(){}");
    prog.attach(vs);
    prog.attach(fs);
    prog.link();

    micron::gl::vao varr;
    micron::gl::buffer vbo(micron::gl::buffer_target::array);
    float xs[6] = { 0 };
    vbo.data(xs, sizeof(xs), micron::gl::buffer_usage::static_draw);
    varr.attrib(0, vbo, 2, micron::gl::GL_FLOAT, false, 0, 0);

    micron::gl::glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    micron::gl::glClear(micron::gl::GL_COLOR_BUFFER_BIT);
    prog.use();
    varr.bind();
    micron::gl::glDrawArrays(micron::gl::GL_TRIANGLES, 0, 3);

    require_true(g_stub.last_clear_mask == micron::gl::GL_COLOR_BUFFER_BIT);
    require_true(g_stub.last_clear_color[0] > 0.09f && g_stub.last_clear_color[0] < 0.11f);
    require_true(g_stub.last_used_program == prog.handle());
    require_true(g_stub.last_bound_vao == varr.handle());
    require_true(g_stub.last_draw_mode == micron::gl::GL_TRIANGLES);
    require_true(g_stub.last_draw_count == 3);
  }
  end_test_case();

  test_case("move-only enforcement: copy doesn't compile");
  {

    static_assert(!__is_constructible(micron::gl::shader, const micron::gl::shader &));
    static_assert(!__is_constructible(micron::gl::program, const micron::gl::program &));
    static_assert(!__is_constructible(micron::gl::buffer, const micron::gl::buffer &));
    static_assert(!__is_constructible(micron::gl::vao, const micron::gl::vao &));
    static_assert(!__is_constructible(micron::gl::texture, const micron::gl::texture &));
    static_assert(__is_constructible(micron::gl::shader, micron::gl::shader &&));
    static_assert(__is_constructible(micron::gl::vao, micron::gl::vao &&));
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
