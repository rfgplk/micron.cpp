//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../types.hpp"

#include "opengl.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

enum class shader_kind : GLenum {
  vertex = GL_VERTEX_SHADER,
  fragment = GL_FRAGMENT_SHADER,
  geometry = GL_GEOMETRY_SHADER,
  tess_control = GL_TESS_CONTROL_SHADER,
  tess_evaluation = GL_TESS_EVALUATION_SHADER,
  compute = GL_COMPUTE_SHADER,
};

class shader
{
private:
  GLuint __h = 0;

public:
  ~shader() { reset(); }

  shader() = default;

  explicit shader(shader_kind k)
  {
    if ( !glCreateShader ) throw except::library_error("shader: gl not loaded");
    __h = glCreateShader(static_cast<GLenum>(k));
    if ( !__h ) throw except::library_error("shader: glCreateShader returned 0");
  }

  shader(shader_kind k, const char *src) : shader(k) { compile(src); }

  template<micron::has_cstr S> shader(shader_kind k, const S &src) : shader(k) { compile(src.c_str()); }

  shader(const shader &) = delete;

  shader(shader &&o) noexcept : __h(o.__h) { o.__h = 0; }

  shader &operator=(const shader &) = delete;

  shader &
  operator=(shader &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      o.__h = 0;
    }
    return *this;
  }

  void
  reset() noexcept
  {
    if ( __h && glDeleteShader ) glDeleteShader(__h);
    __h = 0;
  }

  GLuint
  handle() const noexcept
  {
    return __h;
  }

  bool
  valid() const noexcept
  {
    return __h != 0;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  void
  compile(const char *src)
  {
    if ( !__h ) throw except::logic_error("shader: empty handle");
    const GLchar *arr[1] = { src };
    glShaderSource(__h, 1, arr, nullptr);
    glCompileShader(__h);
    GLint ok = GL_FALSE;
    glGetShaderiv(__h, GL_COMPILE_STATUS, &ok);
    if ( ok == GL_FALSE ) {
      static char log[1024] = { 0 };
      GLsizei len = 0;
      glGetShaderInfoLog(__h, sizeof(log) - 1, &len, log);
      throw except::library_error(log[0] ? log : "shader: compile failed");
    }
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x8B31 /* GL_SHADER */, __h, -1, name);
  }
};

template<shader_kind K> struct shader_source {
  static constexpr shader_kind kind = K;
  const char *src = nullptr;

  constexpr shader_source(const char *s) noexcept : src(s) { }

  template<micron::has_cstr S> constexpr shader_source(const S &s) noexcept : src(s.c_str()) { }
};

using vert = shader_source<shader_kind::vertex>;
using frag = shader_source<shader_kind::fragment>;
using geom = shader_source<shader_kind::geometry>;
using tesc = shader_source<shader_kind::tess_control>;
using tese = shader_source<shader_kind::tess_evaluation>;
using comp = shader_source<shader_kind::compute>;

template<typename> inline constexpr bool is_shader_source_v = false;
template<shader_kind K> inline constexpr bool is_shader_source_v<shader_source<K>> = true;

};      // namespace gl
};      // namespace gfx
};      // namespace micron
