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

enum class query_target : GLenum {
  samples_passed = 0x8914,                       // GL_SAMPLES_PASSED
  any_samples_passed = 0x8C2F,                   // GL_ANY_SAMPLES_PASSED
  any_samples_passed_conservative = 0x8D6A,      // GL_ANY_SAMPLES_PASSED_CONSERVATIVE
  primitives_generated = 0x8C87,                 // GL_PRIMITIVES_GENERATED
  time_elapsed = 0x88BF,                         // GL_TIME_ELAPSED
  timestamp = 0x8E28,                            // GL_TIMESTAMP
};

class query
{
  GLuint __h = 0;
  query_target __target = query_target::samples_passed;

public:
  ~query() { reset(); }

  query() = default;

  explicit query(query_target t) : __target(t)
  {
    if ( !glGenQueries ) throw except::library_error("query: gl not loaded");
    glGenQueries(1, &__h);
    if ( !__h ) throw except::library_error("query: glGenQueries returned 0");
  }

  query(const query &) = delete;

  query(query &&o) noexcept : __h(o.__h), __target(o.__target) { o.__h = 0; }

  query &operator=(const query &) = delete;

  query &
  operator=(query &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      __target = o.__target;
      o.__h = 0;
    }
    return *this;
  }

  void
  reset() noexcept
  {
    if ( __h && glDeleteQueries ) glDeleteQueries(1, &__h);
    __h = 0;
  }

  GLuint
  handle() const noexcept
  {
    return __h;
  }

  query_target
  target() const noexcept
  {
    return __target;
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
  begin() noexcept
  {
    if ( __h && glBeginQuery ) glBeginQuery(static_cast<GLenum>(__target), __h);
  }

  void
  end() noexcept
  {
    if ( glEndQuery ) glEndQuery(static_cast<GLenum>(__target));
  }

  bool
  available() const noexcept
  {
    GLuint r = 0;
    if ( __h && glGetQueryObjectuiv ) glGetQueryObjectuiv(__h, 0x8867 /* GL_QUERY_RESULT_AVAILABLE */, &r);
    return r != 0;
  }

  GLuint
  result_u32() const noexcept
  {
    GLuint r = 0;
    if ( __h && glGetQueryObjectuiv ) glGetQueryObjectuiv(__h, 0x8866 /* GL_QUERY_RESULT */, &r);
    return r;
  }

  GLint
  result_i32() const noexcept
  {
    GLint r = 0;
    if ( __h && glGetQueryObjectiv ) glGetQueryObjectiv(__h, 0x8866 /* GL_QUERY_RESULT */, &r);
    return r;
  }

  GLuint64
  result_u64() const noexcept
  {
    GLuint64 r = 0;
    if ( __h && glGetQueryObjectui64v ) glGetQueryObjectui64v(__h, 0x8866 /* GL_QUERY_RESULT */, &r);
    return r;
  }

  GLint64
  result_i64() const noexcept
  {
    GLint64 r = 0;
    if ( __h && glGetQueryObjecti64v ) glGetQueryObjecti64v(__h, 0x8866 /* GL_QUERY_RESULT */, &r);
    return r;
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x8253 /* GL_QUERY */, __h, -1, name);
  }
};

inline void
query_timestamp(query &q) noexcept
{
  if ( q.handle() && glQueryCounter ) glQueryCounter(q.handle(), 0x8E28 /* GL_TIMESTAMP */);
}

};      // namespace gl
};      // namespace gfx
};      // namespace micron
