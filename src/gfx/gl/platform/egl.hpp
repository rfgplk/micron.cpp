//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../except.hpp"
#include "../../../linux/dynamic.hpp"
#include "../../../memory/cstring.hpp"

#include "../../platform/platform.hpp"
#include "../__bits/__egl_types.hpp"

// GL EGL backend
namespace micron
{
namespace gfx
{
namespace gl
{
using namespace ::micron::gfx::platform;

struct egl_lib_t {
  host_dso host;

  PFN_eglGetDisplay eglGetDisplay = nullptr;
  PFN_eglGetPlatformDisplay eglGetPlatformDisplay = nullptr;
  PFN_eglGetPlatformDisplayEXT eglGetPlatformDisplayEXT = nullptr;
  PFN_eglInitialize eglInitialize = nullptr;
  PFN_eglTerminate eglTerminate = nullptr;
  PFN_eglBindAPI eglBindAPI = nullptr;
  PFN_eglQueryString eglQueryString = nullptr;
  PFN_eglChooseConfig eglChooseConfig = nullptr;
  PFN_eglGetConfigAttrib eglGetConfigAttrib = nullptr;
  PFN_eglCreateContext eglCreateContext = nullptr;
  PFN_eglDestroyContext eglDestroyContext = nullptr;
  PFN_eglCreateWindowSurface eglCreateWindowSurface = nullptr;
  PFN_eglCreatePlatformWindowSurface eglCreatePlatformWindowSurface = nullptr;
  PFN_eglCreatePlatformWindowSurfaceEXT eglCreatePlatformWindowSurfaceEXT = nullptr;
  PFN_eglCreatePbufferSurface eglCreatePbufferSurface = nullptr;
  PFN_eglDestroySurface eglDestroySurface = nullptr;
  PFN_eglMakeCurrent eglMakeCurrent = nullptr;
  PFN_eglSwapBuffers eglSwapBuffers = nullptr;
  PFN_eglSwapInterval eglSwapInterval = nullptr;
  PFN_eglGetProcAddress eglGetProcAddress = nullptr;
  PFN_eglGetCurrentContext eglGetCurrentContext = nullptr;
  PFN_eglGetCurrentSurface eglGetCurrentSurface = nullptr;
  PFN_eglGetError eglGetError = nullptr;

  bool ext_platform_wayland = false;
  bool ext_platform_x11 = false;

  egl_lib_t() : host("libEGL.so.1")
  {
    eglGetDisplay = host.sym_as<PFN_eglGetDisplay>("eglGetDisplay");
    eglGetPlatformDisplay = host.sym_as<PFN_eglGetPlatformDisplay>("eglGetPlatformDisplay");
    eglInitialize = host.sym_as<PFN_eglInitialize>("eglInitialize");
    eglTerminate = host.sym_as<PFN_eglTerminate>("eglTerminate");
    eglBindAPI = host.sym_as<PFN_eglBindAPI>("eglBindAPI");
    eglQueryString = host.sym_as<PFN_eglQueryString>("eglQueryString");
    eglChooseConfig = host.sym_as<PFN_eglChooseConfig>("eglChooseConfig");
    eglGetConfigAttrib = host.sym_as<PFN_eglGetConfigAttrib>("eglGetConfigAttrib");
    eglCreateContext = host.sym_as<PFN_eglCreateContext>("eglCreateContext");
    eglDestroyContext = host.sym_as<PFN_eglDestroyContext>("eglDestroyContext");
    eglCreateWindowSurface = host.sym_as<PFN_eglCreateWindowSurface>("eglCreateWindowSurface");
    eglCreatePbufferSurface = host.sym_as<PFN_eglCreatePbufferSurface>("eglCreatePbufferSurface");
    eglDestroySurface = host.sym_as<PFN_eglDestroySurface>("eglDestroySurface");
    eglMakeCurrent = host.sym_as<PFN_eglMakeCurrent>("eglMakeCurrent");
    eglSwapBuffers = host.sym_as<PFN_eglSwapBuffers>("eglSwapBuffers");
    eglSwapInterval = host.sym_as<PFN_eglSwapInterval>("eglSwapInterval");
    eglGetProcAddress = host.sym_as<PFN_eglGetProcAddress>("eglGetProcAddress");
    eglGetCurrentContext = host.sym_as<PFN_eglGetCurrentContext>("eglGetCurrentContext");
    eglGetCurrentSurface = host.sym_as<PFN_eglGetCurrentSurface>("eglGetCurrentSurface");
    eglGetError = host.sym_as<PFN_eglGetError>("eglGetError");

    if ( !eglGetDisplay || !eglInitialize || !eglChooseConfig || !eglCreateContext || !eglMakeCurrent || !eglSwapBuffers || !eglBindAPI
         || !eglGetProcAddress ) {
      throw except::library_error("egl: required entry points missing in libEGL.so.1");
    }

    eglGetPlatformDisplayEXT
        = reinterpret_cast<PFN_eglGetPlatformDisplayEXT>(reinterpret_cast<void *>(eglGetProcAddress("eglGetPlatformDisplayEXT")));
    eglCreatePlatformWindowSurface = reinterpret_cast<PFN_eglCreatePlatformWindowSurface>(
        reinterpret_cast<void *>(eglGetProcAddress("eglCreatePlatformWindowSurface")));
    eglCreatePlatformWindowSurfaceEXT = reinterpret_cast<PFN_eglCreatePlatformWindowSurfaceEXT>(
        reinterpret_cast<void *>(eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT")));

    const char *exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if ( exts ) {
      ext_platform_wayland = __ext_present(exts, "EGL_EXT_platform_wayland") || __ext_present(exts, "EGL_KHR_platform_wayland");
      ext_platform_x11 = __ext_present(exts, "EGL_EXT_platform_x11") || __ext_present(exts, "EGL_KHR_platform_x11");
    }
  }

  static bool
  __ext_present(const char *list, const char *name) noexcept
  {
    if ( !list || !name ) return false;
    const usize nlen = micron::strlen(name);
    if ( nlen == 0 ) return false;
    const char *p = list;
    while ( (p = micron::strstr(p, name)) != nullptr ) {
      const bool left_ok = (p == list) || (p[-1] == ' ');
      const bool right_ok = (p[nlen] == ' ' || p[nlen] == '\0');
      if ( left_ok && right_ok ) return true;
      p += nlen;
    }
    return false;
  }

  bool
  has_display_extension(EGLDisplay dpy, const char *name) noexcept
  {
    if ( !eglQueryString || dpy == EGL_NO_DISPLAY || !name ) return false;
    return __ext_present(eglQueryString(dpy, EGL_EXTENSIONS), name);
  }
};

struct egl_config_request_t {
  EGLint red_size = 8;
  EGLint green_size = 8;
  EGLint blue_size = 8;
  EGLint alpha_size = 8;
  EGLint depth_size = 24;
  EGLint stencil_size = 8;
  EGLint samples = 0;
  EGLint renderable = EGL_OPENGL_BIT;        // desktop GL, not ES
  EGLint surface_type = EGL_WINDOW_BIT;      // EGL_PBUFFER_BIT for off-screen
};

struct egl_context_request_t {
  EGLint major = 4;
  EGLint minor = 6;
  bool core_profile = true;
  bool forward_compat = true;
  bool debug = false;
  EGLContext share = EGL_NO_CONTEXT;      // non-null = share resources
};

class egl_context_t
{
  EGLDisplay __display = EGL_NO_DISPLAY;
  EGLConfig __config = nullptr;
  EGLContext __ctx = EGL_NO_CONTEXT;
  EGLSurface __surface = EGL_NO_SURFACE;
  EGLint __major = 0;
  EGLint __minor = 0;
  egl_lib_t *__lib = nullptr;

public:
  ~egl_context_t() { destroy(); }

  egl_context_t() = default;

  egl_context_t(const egl_context_t &) = delete;
  egl_context_t(egl_context_t &&) = delete;

  egl_context_t &operator=(const egl_context_t &) = delete;
  egl_context_t &operator=(egl_context_t &&) = delete;

  EGLDisplay
  display() const noexcept
  {
    return __display;
  }

  EGLConfig
  config() const noexcept
  {
    return __config;
  }

  EGLContext
  ctx() const noexcept
  {
    return __ctx;
  }

  EGLSurface
  surface() const noexcept
  {
    return __surface;
  }

  EGLint
  major() const noexcept
  {
    return __major;
  }

  EGLint
  minor() const noexcept
  {
    return __minor;
  }

  egl_lib_t *
  lib() const noexcept
  {
    return __lib;
  }

  void
  attach_surface(EGLSurface s) noexcept
  {
    __surface = s;
  }

  void
  open(egl_lib_t &e, EGLenum platform, void *native)
  {
    __lib = &e;
    if ( e.eglGetPlatformDisplayEXT ) {
      const EGLint attribs[] = { EGL_NONE };
      __display = e.eglGetPlatformDisplayEXT(platform, native, attribs);
    } else {
      __display = e.eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(native));
    }
    if ( __display == EGL_NO_DISPLAY ) throw except::library_error("egl: eglGetDisplay returned EGL_NO_DISPLAY");
    if ( !e.eglInitialize(__display, &__major, &__minor) ) throw except::library_error("egl: eglInitialize failed");
    if ( !e.eglBindAPI(EGL_OPENGL_API) ) throw except::library_error("egl: eglBindAPI(EGL_OPENGL_API) failed");
  }

  void
  choose_config(const egl_config_request_t &r)
  {
    if ( !__lib || __display == EGL_NO_DISPLAY ) throw except::logic_error("egl_context: open() not called");
    const EGLint attribs[] = {
      EGL_SURFACE_TYPE, r.surface_type, EGL_RENDERABLE_TYPE, r.renderable,          EGL_RED_SIZE, r.red_size,     EGL_GREEN_SIZE,
      r.green_size,     EGL_BLUE_SIZE,  r.blue_size,         EGL_ALPHA_SIZE,        r.alpha_size, EGL_DEPTH_SIZE, r.depth_size,
      EGL_STENCIL_SIZE, r.stencil_size, EGL_SAMPLE_BUFFERS,  r.samples > 0 ? 1 : 0, EGL_SAMPLES,  r.samples,      EGL_NONE,
    };
    EGLint n = 0;
    if ( !__lib->eglChooseConfig(__display, attribs, &__config, 1, &n) || n == 0 ) {
      throw except::library_error("egl: eglChooseConfig found no matching config");
    }
  }

  void
  create_context(const egl_context_request_t &r)
  {
    if ( !__lib || !__config ) throw except::logic_error("egl_context: choose_config not called");
    EGLint flags = 0;
    if ( r.debug ) flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
    if ( r.forward_compat ) flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
    const EGLint profile = r.core_profile ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;
    const EGLint attribs[] = {
      EGL_CONTEXT_MAJOR_VERSION,
      r.major,
      EGL_CONTEXT_MINOR_VERSION,
      r.minor,
      EGL_CONTEXT_OPENGL_PROFILE_MASK,
      profile,
      EGL_CONTEXT_FLAGS_KHR,
      flags,
      EGL_NONE,
    };
    __ctx = __lib->eglCreateContext(__display, __config, r.share, attribs);
    if ( __ctx == EGL_NO_CONTEXT ) throw except::library_error("egl: eglCreateContext failed");
  }

  void
  create_window_surface(void *native_window)
  {
    if ( !__lib || !__config ) throw except::logic_error("egl_context: choose_config not called");
    if ( __lib->eglCreatePlatformWindowSurfaceEXT ) {
      const EGLint attribs[] = { EGL_NONE };
      __surface = __lib->eglCreatePlatformWindowSurfaceEXT(__display, __config, native_window, attribs);
    } else {
      __surface = __lib->eglCreateWindowSurface(__display, __config, reinterpret_cast<EGLNativeWindowType>(native_window), nullptr);
    }
    if ( __surface == EGL_NO_SURFACE ) throw except::library_error("egl: eglCreateWindowSurface failed");
  }

  EGLSurface
  create_pbuffer_surface(i32 width, i32 height) noexcept
  {
    if ( !__lib || !__lib->eglCreatePbufferSurface || !__config ) return EGL_NO_SURFACE;
    const EGLint attribs[] = {
      EGL_WIDTH, static_cast<EGLint>(width), EGL_HEIGHT, static_cast<EGLint>(height), EGL_NONE,
    };
    return __lib->eglCreatePbufferSurface(__display, __config, attribs);
  }

  void
  destroy_surface(EGLSurface s) noexcept
  {
    if ( __lib && __lib->eglDestroySurface && s != EGL_NO_SURFACE ) __lib->eglDestroySurface(__display, s);
  }

  bool
  make_current() noexcept
  {
    return __lib && __lib->eglMakeCurrent(__display, __surface, __surface, __ctx) != EGL_FALSE;
  }

  void
  release() noexcept
  {
    if ( __lib ) __lib->eglMakeCurrent(__display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }

  void
  swap() noexcept
  {
    if ( __lib ) __lib->eglSwapBuffers(__display, __surface);
  }

  void
  set_swap_interval(EGLint interval) noexcept
  {
    if ( __lib && __lib->eglSwapInterval ) __lib->eglSwapInterval(__display, interval);
  }

  void *
  proc_address(const char *name) noexcept
  {
    if ( !__lib || !__lib->eglGetProcAddress ) return nullptr;
    return reinterpret_cast<void *>(__lib->eglGetProcAddress(name));
  }

  void
  destroy() noexcept
  {
    if ( !__lib ) return;
    if ( __surface != EGL_NO_SURFACE ) __lib->eglDestroySurface(__display, __surface);
    if ( __ctx != EGL_NO_CONTEXT ) __lib->eglDestroyContext(__display, __ctx);
    if ( __display != EGL_NO_DISPLAY ) __lib->eglTerminate(__display);
    __display = EGL_NO_DISPLAY;
    __ctx = EGL_NO_CONTEXT;
    __surface = EGL_NO_SURFACE;
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
