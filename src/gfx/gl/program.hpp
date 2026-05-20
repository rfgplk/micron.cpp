//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "opengl.hpp"
#include "shader.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

class program
{
  GLuint __h = 0;

public:
  ~program() { reset(); }

  program()
  {
    if ( !glCreateProgram ) throw except::library_error("program: gl not loaded");
    __h = glCreateProgram();
    if ( !__h ) throw except::library_error("program: glCreateProgram returned 0");
  }

  template<typename First, typename... Rest>
    requires(is_shader_source_v<micron::remove_cvref_t<First>>)
  explicit program(const First &first, const Rest &...rest) : program()
  {
    shader stages[] = { shader(micron::remove_cvref_t<First>::kind, first.src), shader(micron::remove_cvref_t<Rest>::kind, rest.src)... };
    for ( auto &s : stages ) attach(s);
    link();
  }

  template<typename... Srcs>
    requires(sizeof...(Srcs) >= 1 && (is_shader_source_v<micron::remove_cvref_t<Srcs>> && ...))
  program(bool do_link, const Srcs &...srcs) : program()
  {
    shader stages[] = { shader(micron::remove_cvref_t<Srcs>::kind, srcs.src)... };
    for ( auto &s : stages ) attach(s);
    if ( do_link ) link();
  }

  program(const program &) = delete;

  program(program &&o) noexcept : __h(o.__h) { o.__h = 0; }

  program &operator=(const program &) = delete;

  program &
  operator=(program &&o) noexcept
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
    if ( __h && glDeleteProgram ) glDeleteProgram(__h);
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
  attach(const shader &s) noexcept
  {
    if ( __h && s.valid() && glAttachShader ) glAttachShader(__h, s.handle());
  }

  void
  detach(const shader &s) noexcept
  {
    if ( __h && s.valid() && glDetachShader ) glDetachShader(__h, s.handle());
  }

  void
  link()
  {
    if ( !__h ) throw except::logic_error("program: empty handle");
    glLinkProgram(__h);
    GLint ok = GL_FALSE;
    glGetProgramiv(__h, GL_LINK_STATUS, &ok);
    if ( ok == GL_FALSE ) {
      static char log[1024] = { 0 };
      GLsizei len = 0;
      glGetProgramInfoLog(__h, sizeof(log) - 1, &len, log);
      throw except::library_error(log[0] ? log : "program: link failed");
    }
  }

  void
  validate()
  {
    if ( !__h ) throw except::logic_error("program: empty handle");
    glValidateProgram(__h);
    GLint ok = GL_FALSE;
    glGetProgramiv(__h, GL_VALIDATE_STATUS, &ok);
    if ( ok == GL_FALSE ) throw except::library_error("program: validation failed");
  }

  void
  use() const noexcept
  {
    if ( __h && glUseProgram ) glUseProgram(__h);
  }

  GLint
  uniform_location(const char *name) const noexcept
  {
    if ( !__h || !glGetUniformLocation ) return -1;
    return glGetUniformLocation(__h, name);
  }

  GLint
  attrib_location(const char *name) const noexcept
  {
    if ( !__h || !glGetAttribLocation ) return -1;
    return glGetAttribLocation(__h, name);
  }

  void
  bind_attrib(GLuint index, const char *name) noexcept
  {
    if ( __h && glBindAttribLocation && name ) glBindAttribLocation(__h, index, name);
  }

  static void
  set_uniform(GLint loc, GLfloat v) noexcept
  {
    if ( glUniform1f && loc >= 0 ) glUniform1f(loc, v);
  }

  static void
  set_uniform(GLint loc, GLint v) noexcept
  {
    if ( glUniform1i && loc >= 0 ) glUniform1i(loc, v);
  }

  static void
  set_uniform(GLint loc, GLfloat x, GLfloat y) noexcept
  {
    if ( glUniform2f && loc >= 0 ) glUniform2f(loc, x, y);
  }

  static void
  set_uniform(GLint loc, GLfloat x, GLfloat y, GLfloat z) noexcept
  {
    if ( glUniform3f && loc >= 0 ) glUniform3f(loc, x, y, z);
  }

  static void
  set_uniform(GLint loc, GLfloat x, GLfloat y, GLfloat z, GLfloat w) noexcept
  {
    if ( glUniform4f && loc >= 0 ) glUniform4f(loc, x, y, z, w);
  }

  static void
  set_uniform_mat3(GLint loc, const GLfloat *m, bool transpose = false) noexcept
  {
    if ( glUniformMatrix3fv && loc >= 0 ) glUniformMatrix3fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, m);
  }

  static void
  set_uniform_mat4(GLint loc, const GLfloat *m, bool transpose = false) noexcept
  {
    if ( glUniformMatrix4fv && loc >= 0 ) glUniformMatrix4fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, m);
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x8B40 /* GL_PROGRAM */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
