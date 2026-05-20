//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "opengl.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

class debug_group
{
private:
  bool __active = false;

public:
  ~debug_group() noexcept
  {
    if ( __active && glPopDebugGroup ) glPopDebugGroup();
  }

  debug_group() = default;

  explicit debug_group(const char *message, GLuint id = 0) noexcept
  {
    if ( !glPushDebugGroup || !message ) return;
    glPushDebugGroup(0x824A /* GL_DEBUG_SOURCE_APPLICATION */, id, -1, message);
    __active = true;
  }

  debug_group(const debug_group &) = delete;

  debug_group(debug_group &&o) noexcept : __active(o.__active) { o.__active = false; }

  debug_group &operator=(const debug_group &) = delete;

  debug_group &
  operator=(debug_group &&o) noexcept
  {
    if ( this != &o ) {
      if ( __active && glPopDebugGroup ) glPopDebugGroup();
      __active = o.__active;
      o.__active = false;
    }
    return *this;
  }

  bool
  active() const noexcept
  {
    return __active;
  }
};

inline void
debug_marker(const char *message, GLuint id = 0) noexcept
{
  if ( !glDebugMessageInsert || !message ) return;
  glDebugMessageInsert(0x824A /* GL_DEBUG_SOURCE_APPLICATION */, 0x824B /* GL_DEBUG_TYPE_MARKER */, id,
                       0x826B /* GL_DEBUG_SEVERITY_NOTIFICATION */, -1, message);
}

};      // namespace gl
};      // namespace gfx
};      // namespace micron
