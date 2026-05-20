//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "buffer.hpp"
#include "opengl.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

class vao
{
  GLuint __h = 0;

public:
  ~vao() { reset(); }

  vao()
  {
    if ( !glGenVertexArrays ) throw except::library_error("vao: gl not loaded");
    glGenVertexArrays(1, &__h);
    if ( !__h ) throw except::library_error("vao: glGenVertexArrays returned 0");
  }

  vao(const vao &) = delete;

  vao(vao &&o) noexcept : __h(o.__h) { o.__h = 0; }

  vao &operator=(const vao &) = delete;

  vao &
  operator=(vao &&o) noexcept
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
    if ( __h && glDeleteVertexArrays ) glDeleteVertexArrays(1, &__h);
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
  bind() const noexcept
  {
    if ( __h && glBindVertexArray ) glBindVertexArray(__h);
  }

  static void
  unbind() noexcept
  {
    if ( glBindVertexArray ) glBindVertexArray(0);
  }

  void
  attrib(GLuint index, const buffer &vbo, GLint components, GLenum type, bool normalized, GLsizei stride, GLintptr offset) noexcept
  {
    if ( !__h || !glVertexAttribPointer ) return;
    bind();
    vbo.bind();
    glVertexAttribPointer(index, components, type, normalized ? GL_TRUE : GL_FALSE, stride, reinterpret_cast<const void *>(offset));
    glEnableVertexAttribArray(index);
  }

  void
  attrib_int(GLuint index, const buffer &vbo, GLint components, GLenum type, GLsizei stride, GLintptr offset) noexcept
  {
    if ( !__h || !glVertexAttribIPointer ) return;
    bind();
    vbo.bind();
    glVertexAttribIPointer(index, components, type, stride, reinterpret_cast<const void *>(offset));
    glEnableVertexAttribArray(index);
  }

  void
  enable(GLuint index) const noexcept
  {
    if ( __h && glEnableVertexAttribArray ) {
      bind();
      glEnableVertexAttribArray(index);
    }
  }

  void
  disable(GLuint index) const noexcept
  {
    if ( __h && glDisableVertexAttribArray ) {
      bind();
      glDisableVertexAttribArray(index);
    }
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x8074 /* GL_VERTEX_ARRAY */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
