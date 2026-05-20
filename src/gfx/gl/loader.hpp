//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/cstring.hpp"

#include "opengl.hpp"

// Two-track GL function loading:
//
//   1. Runtime full-table — populates every inline-variable PFN pointer in
//      opengl.hpp by walking gl_table[] in lockstep with __gl_fnptr_slots[].
//      Called once after the context is current; subsequent gl::glClear(...)
//      calls hit the resolved pointer directly with zero indirection cost.
//
//   2. Constexpr subset — `gl::subset<I0, I1, ...>` packs only the indices
//      the user names at compile time. Storage is exactly N pointers; no
//      ~100-pointer table linked in when you only need three calls.
//      Use the MICRON_GL_SYM(name) macro to translate a name to an index
//      at compile time:
//
//          gl::subset<MICRON_GL_SYM(glClear), MICRON_GL_SYM(glDrawArrays)> t;
//          t.load(get_proc);
//          auto clear = t.as<MICRON_GL_SYM(glClear), gl::PFNGLCLEARPROC>();
//          clear(gl::GL_COLOR_BUFFER_BIT);

namespace micron
{
namespace gfx
{
namespace gl
{

using get_proc_t = void *(*)(const char *);

inline get_proc_t __get_proc = nullptr;

inline void
__load_core_4_6(get_proc_t gp) noexcept
{
  __get_proc = gp;
  for ( usize i = 0; i < gl_table_size; ++i ) {
    *__gl_fnptr_slots[i] = gp(gl_table[i].name);
  }
}

inline void *
get_proc_address(const char *name) noexcept
{
  return __get_proc ? __get_proc(name) : nullptr;
}

// Returns true if `ext` is present, case-sensitive and word-bounded, in a
// space-separated extension list. GL ES-style. Used by the legacy
// GL_EXTENSIONS path here and by the EGL / GLX extension probes.
inline bool
__extension_in(const char *ext, const char *list) noexcept
{
  if ( !ext || !list ) return false;
  const usize elen = micron::strlen(ext);
  if ( elen == 0 ) return false;
  const char *p = list;
  while ( (p = micron::strstr(p, ext)) != nullptr ) {
    const bool left_ok = (p == list) || (p[-1] == ' ');
    const bool right_ok = (p[elen] == ' ' || p[elen] == '\0');
    if ( left_ok && right_ok ) return true;
    p += elen;
  }
  return false;
}

// Walks the GL 3.0+ extension enumeration via glGetStringi(GL_EXTENSIONS, i)
// when available. The caller must have a current context.
inline bool
has_extension(const char *name) noexcept
{
  if ( !glGetIntegerv || !glGetStringi || !glGetString ) return false;
  GLint n = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  if ( n > 0 ) {
    for ( GLint i = 0; i < n; ++i ) {
      const GLubyte *s = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
      if ( !s ) continue;
      if ( micron::strcmp(reinterpret_cast<const char *>(s), name) == 0 ) return true;
    }
    return false;
  }
  const GLubyte *legacy = glGetString(GL_EXTENSIONS);
  return __extension_in(name, reinterpret_cast<const char *>(legacy));
}

// --- Comptime subset ----------------------------------------------------
// Storage is sizeof...(Indices) pointers. Names are looked up at compile
// time via gl_table[Indices].name; the runtime cost of `load()` is one
// get_proc call per index.

template<u16... Indices> class subset
{
private:
  void *__fns[sizeof...(Indices)] = {};

public:
  static constexpr usize count = sizeof...(Indices);
  static constexpr u16 indices[count] = { Indices... };

  void
  load(get_proc_t gp) noexcept
  {
    usize k = 0;
    ((__fns[k++] = gp(gl_table[Indices].name)), ...);
  }

  // Compute the local array position of a globally-numbered index. consteval
  // so the cost lives entirely at compile time.
  template<u16 Idx>
  static consteval usize
  position_of() noexcept
  {
    static_assert(((Idx == Indices) || ...), "subset does not contain that index");
    usize pos = 0;
    bool found = false;
    auto check = [&](u16 idx) {
      if ( !found ) {
        if ( idx == Idx )
          found = true;
        else
          ++pos;
      }
    };
    ((check(Indices)), ...);
    return pos;
  }

  template<u16 Idx>
  void *
  raw() const noexcept
  {
    return __fns[position_of<Idx>()];
  }

  template<u16 Idx, class Fn>
  Fn
  as() const noexcept
  {
    return reinterpret_cast<Fn>(raw<Idx>());
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron

// Translate a name to its gl_table[] index at compile time. The argument must
// be a bare identifier matching exactly the spelling in gl_table[].
//
//     gl::subset<MICRON_GL_SYM(glClear)> t;
#define MICRON_GL_SYM(name) (::micron::gfx::gl::sym_index_of(#name))
