//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "opengl.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

inline const char *
gl_error_name(GLenum e) noexcept
{
  switch ( e ) {
  case GL_NO_ERROR:
    return "GL_NO_ERROR";
  case GL_INVALID_ENUM:
    return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:
    return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:
    return "GL_INVALID_OPERATION";
  case GL_OUT_OF_MEMORY:
    return "GL_OUT_OF_MEMORY";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  default:
    return "GL_UNKNOWN_ERROR";
  }
}

inline void
check_gl(const char *where)
{
  if ( !glGetError ) return;
  GLenum e = glGetError();
  if ( e != GL_NO_ERROR ) {
    (void)where;
    throw except::library_error(gl_error_name(e));
  }
}

inline GLenum
last_gl_error() noexcept
{
  if ( !glGetError ) return GL_NO_ERROR;
  return glGetError();
}

inline void
__default_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
                         const void *user) noexcept
{
  (void)source;
  (void)type;
  (void)id;
  (void)severity;
  (void)user;

  micron::syscall(SYS_write, 2, message, length > 0 ? static_cast<usize>(length) : 0);
  const char nl = '\n';
  micron::syscall(SYS_write, 2, &nl, 1);
}

inline void
install_default_debug_callback() noexcept
{
  if ( glDebugMessageCallback ) glDebugMessageCallback(&__default_debug_callback, nullptr);
  if ( glEnable ) glEnable(GL_DEBUG_OUTPUT);
}

};      // namespace gl
};      // namespace gfx
};      // namespace micron
