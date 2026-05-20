//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../linux/dynamic.hpp"
#include "../../types.hpp"

#include "__bits/__x11_types.hpp"
#include "platform.hpp"

// X11 client + window-management backend. Loads libX11.so.6 via micron::dso
// (prefers an already-loaded host copy if the host process linked -lX11;
// otherwise the loader maps it fresh — though the host path is much safer
// for a stateful lib like X11).
//
// This module owns only "graphics-relevant" window state: size, title,
// close request, visual / colormap. Input events stay in io/uxin/ — we
// route them by sharing the X11 fd through gfx::display::raw_fd() so a
// single epoll loop can drive both.

namespace micron
{
namespace gfx
{
namespace platform
{

using PFN_XInitThreads = XStatus (*)(void);
using PFN_XOpenDisplay = XDisplay *(*)(const char *display_name);
using PFN_XCloseDisplay = int (*)(XDisplay *display);
using PFN_XDefaultScreen = int (*)(XDisplay *display);
using PFN_XDefaultRootWindow = XWindow (*)(XDisplay *display);
using PFN_XConnectionNumber = int (*)(XDisplay *display);
using PFN_XCreateColormap = XColormap (*)(XDisplay *display, XWindow w, XVisual_opaque *visual, int alloc);
using PFN_XCreateWindow
    = XWindow (*)(XDisplay *display, XWindow parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                  int depth, unsigned int c_class, XVisual_opaque *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
using PFN_XDestroyWindow = int (*)(XDisplay *display, XWindow w);
using PFN_XMapWindow = int (*)(XDisplay *display, XWindow w);
using PFN_XUnmapWindow = int (*)(XDisplay *display, XWindow w);
using PFN_XStoreName = int (*)(XDisplay *display, XWindow w, const char *window_name);
using PFN_XInternAtom = XAtom (*)(XDisplay *display, const char *atom_name, XBool only_if_exists);
using PFN_XSetWMProtocols = XStatus (*)(XDisplay *display, XWindow w, XAtom *protocols, int count);
using PFN_XPending = int (*)(XDisplay *display);
using PFN_XNextEvent = int (*)(XDisplay *display, XEvent *event);
using PFN_XFlush = int (*)(XDisplay *display);
using PFN_XSync = int (*)(XDisplay *display, XBool discard);
using PFN_XFree = int (*)(void *data);
using PFN_XResizeWindow = int (*)(XDisplay *display, XWindow w, unsigned int width, unsigned int height);
using PFN_XMatchVisualInfo = XStatus (*)(XDisplay *display, int screen, int depth, int c_class, XVisualInfo *vinfo_return);

// libX11 entry-point table. Constructed once per process, then borrowed by
// every x11_display_t / x11_window_t. POD aggregate by design — direct member
// access (lib.XOpenDisplay(...)). Throws library_error on missing required
// symbols.
struct x11_lib_t {
  host_dso host;
  PFN_XInitThreads XInitThreads = nullptr;
  PFN_XOpenDisplay XOpenDisplay = nullptr;
  PFN_XCloseDisplay XCloseDisplay = nullptr;
  PFN_XDefaultScreen XDefaultScreen = nullptr;
  PFN_XDefaultRootWindow XDefaultRootWindow = nullptr;
  PFN_XConnectionNumber XConnectionNumber = nullptr;
  PFN_XCreateColormap XCreateColormap = nullptr;
  PFN_XCreateWindow XCreateWindow = nullptr;
  PFN_XDestroyWindow XDestroyWindow = nullptr;
  PFN_XMapWindow XMapWindow = nullptr;
  PFN_XUnmapWindow XUnmapWindow = nullptr;
  PFN_XStoreName XStoreName = nullptr;
  PFN_XInternAtom XInternAtom = nullptr;
  PFN_XSetWMProtocols XSetWMProtocols = nullptr;
  PFN_XPending XPending = nullptr;
  PFN_XNextEvent XNextEvent = nullptr;
  PFN_XFlush XFlush = nullptr;
  PFN_XSync XSync = nullptr;
  PFN_XFree XFree = nullptr;
  PFN_XResizeWindow XResizeWindow = nullptr;
  PFN_XMatchVisualInfo XMatchVisualInfo = nullptr;

  x11_lib_t() : host("libX11.so.6")
  {
    XInitThreads = host.sym_as<PFN_XInitThreads>("XInitThreads");
    XOpenDisplay = host.sym_as<PFN_XOpenDisplay>("XOpenDisplay");
    XCloseDisplay = host.sym_as<PFN_XCloseDisplay>("XCloseDisplay");
    XDefaultScreen = host.sym_as<PFN_XDefaultScreen>("XDefaultScreen");
    XDefaultRootWindow = host.sym_as<PFN_XDefaultRootWindow>("XDefaultRootWindow");
    XConnectionNumber = host.sym_as<PFN_XConnectionNumber>("XConnectionNumber");
    XCreateColormap = host.sym_as<PFN_XCreateColormap>("XCreateColormap");
    XCreateWindow = host.sym_as<PFN_XCreateWindow>("XCreateWindow");
    XDestroyWindow = host.sym_as<PFN_XDestroyWindow>("XDestroyWindow");
    XMapWindow = host.sym_as<PFN_XMapWindow>("XMapWindow");
    XUnmapWindow = host.sym_as<PFN_XUnmapWindow>("XUnmapWindow");
    XStoreName = host.sym_as<PFN_XStoreName>("XStoreName");
    XInternAtom = host.sym_as<PFN_XInternAtom>("XInternAtom");
    XSetWMProtocols = host.sym_as<PFN_XSetWMProtocols>("XSetWMProtocols");
    XPending = host.sym_as<PFN_XPending>("XPending");
    XNextEvent = host.sym_as<PFN_XNextEvent>("XNextEvent");
    XFlush = host.sym_as<PFN_XFlush>("XFlush");
    XSync = host.sym_as<PFN_XSync>("XSync");
    XFree = host.sym_as<PFN_XFree>("XFree");
    XResizeWindow = host.sym_as<PFN_XResizeWindow>("XResizeWindow");
    XMatchVisualInfo = host.sym_as<PFN_XMatchVisualInfo>("XMatchVisualInfo");

    if ( !XOpenDisplay || !XCreateWindow || !XMapWindow || !XInternAtom || !XSetWMProtocols || !XPending || !XNextEvent
         || !XCloseDisplay ) {
      throw except::library_error("x11: required entry points missing in libX11.so.6");
    }
  }
};

class x11_display_t
{
private:
  XDisplay *__display = nullptr;
  XWindow __root = 0;
  int __screen = 0;
  i32 __fd = -1;
  x11_lib_t *__lib = nullptr;

public:
  ~x11_display_t() { close(); }

  x11_display_t() = default;

  x11_display_t(const x11_display_t &) = delete;
  x11_display_t(x11_display_t &&) = delete;

  x11_display_t &operator=(const x11_display_t &) = delete;
  x11_display_t &operator=(x11_display_t &&) = delete;

  XDisplay *
  display() const noexcept
  {
    return __display;
  }

  XWindow
  root() const noexcept
  {
    return __root;
  }

  int
  screen() const noexcept
  {
    return __screen;
  }

  i32
  raw_fd() const noexcept
  {
    return __fd;
  }

  x11_lib_t *
  lib() const noexcept
  {
    return __lib;
  }

  void
  open(x11_lib_t &x)
  {
    __lib = &x;
    if ( x.XInitThreads ) x.XInitThreads();
    __display = x.XOpenDisplay(nullptr);
    if ( !__display ) throw except::network_error("x11: XOpenDisplay returned null — $DISPLAY unset or server unreachable");
    __screen = x.XDefaultScreen(__display);
    __root = x.XDefaultRootWindow(__display);
    __fd = static_cast<i32>(x.XConnectionNumber(__display));
  }

  void
  close() noexcept
  {
    if ( __display && __lib ) __lib->XCloseDisplay(__display);
    __display = nullptr;
    __fd = -1;
  }
};

class x11_window_t
{
private:
  XWindow __window = 0;
  XColormap __cmap = 0;
  XAtom __wm_protocols = 0;
  XAtom __wm_delete_window = 0;
  i32 __width = 0;
  i32 __height = 0;
  i32 __x = 0;
  i32 __y = 0;
  bool __should_close = false;
  x11_display_t *__disp = nullptr;

  // Window-event callbacks installed by gfx::window. The owner pointer is
  // the gfx::window* erased to void* (gfx::window is forward-declared up the
  // include chain). All slots are nullable; an unset slot is silently
  // ignored on the corresponding event.
  void *__owner = nullptr;
  void (*__cb_resize)(void *, i32, i32) noexcept = nullptr;
  void (*__cb_close)(void *) noexcept = nullptr;
  void (*__cb_focus)(void *, bool) noexcept = nullptr;
  void (*__cb_expose)(void *) noexcept = nullptr;
  void (*__cb_visibility)(void *, bool) noexcept = nullptr;
  void (*__cb_move)(void *, i32, i32) noexcept = nullptr;

public:
  ~x11_window_t() { destroy(); }

  x11_window_t() = default;

  x11_window_t(const x11_window_t &) = delete;
  x11_window_t(x11_window_t &&) = delete;

  x11_window_t &operator=(const x11_window_t &) = delete;
  x11_window_t &operator=(x11_window_t &&) = delete;

  XWindow
  window() const noexcept
  {
    return __window;
  }

  i32
  width() const noexcept
  {
    return __width;
  }

  i32
  height() const noexcept
  {
    return __height;
  }

  bool
  should_close() const noexcept
  {
    return __should_close;
  }

  void
  set_owner(void *o) noexcept
  {
    __owner = o;
  }

  void
  set_resize_cb(void (*f)(void *, i32, i32) noexcept) noexcept
  {
    __cb_resize = f;
  }

  void
  set_close_cb(void (*f)(void *) noexcept) noexcept
  {
    __cb_close = f;
  }

  void
  set_focus_cb(void (*f)(void *, bool) noexcept) noexcept
  {
    __cb_focus = f;
  }

  void
  set_expose_cb(void (*f)(void *) noexcept) noexcept
  {
    __cb_expose = f;
  }

  void
  set_visibility_cb(void (*f)(void *, bool) noexcept) noexcept
  {
    __cb_visibility = f;
  }

  void
  set_move_cb(void (*f)(void *, i32, i32) noexcept) noexcept
  {
    __cb_move = f;
  }

  // Caller chooses the visual. For GL, gfx::gl::pick_x11_visual returns one
  // built from a GLX FBConfig; for Vulkan, the caller can build one from
  // XGetVisualInfo with screen depth, or pass nullptr to use the root
  // window's default visual (handled inside create() below — current API
  // requires a non-null visual, kept for backwards compat).
  void
  create(x11_display_t &dpy, XVisualInfo *vi, i32 w, i32 h, const char *title)
  {
    if ( !vi || !vi->visual ) throw except::invalid_argument("x11_window: null visual");
    __disp = &dpy;
    __width = w;
    __height = h;
    auto &x = *dpy.lib();

    __cmap = x.XCreateColormap(dpy.display(), dpy.root(), vi->visual, AllocNone);

    XSetWindowAttributes attrs{};
    attrs.colormap = __cmap;
    attrs.event_mask = StructureNotifyMask | ExposureMask | PropertyChangeMask | FocusChangeMask;
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    const unsigned long mask = CWColormap | CWEventMask | CWBackPixel | CWBorderPixel;

    __window = x.XCreateWindow(dpy.display(), dpy.root(), 0, 0, static_cast<unsigned>(w), static_cast<unsigned>(h), 0, vi->depth,
                               InputOutput, vi->visual, mask, &attrs);
    if ( !__window ) throw except::library_error("x11: XCreateWindow returned 0");

    if ( title ) x.XStoreName(dpy.display(), __window, title);

    // Subscribe to the WM_DELETE_WINDOW protocol so the close button fires
    // a ClientMessage instead of killing our connection.
    __wm_protocols = x.XInternAtom(dpy.display(), "WM_PROTOCOLS", 0);
    __wm_delete_window = x.XInternAtom(dpy.display(), "WM_DELETE_WINDOW", 0);
    XAtom protos[1] = { __wm_delete_window };
    x.XSetWMProtocols(dpy.display(), __window, protos, 1);

    x.XMapWindow(dpy.display(), __window);
    x.XFlush(dpy.display());
  }

  void
  set_title(const char *title) noexcept
  {
    if ( __disp && __disp->lib() && __window && title ) __disp->lib()->XStoreName(__disp->display(), __window, title);
  }

  void
  resize(i32 w, i32 h) noexcept
  {
    if ( __disp && __disp->lib() && __window )
      __disp->lib()->XResizeWindow(__disp->display(), __window, static_cast<unsigned>(w), static_cast<unsigned>(h));
  }

  void
  poll_events() noexcept
  {
    if ( !__disp || !__disp->lib() || !__disp->display() ) return;
    auto &x = *__disp->lib();
    while ( x.XPending(__disp->display()) ) {
      XEvent ev{};
      x.XNextEvent(__disp->display(), &ev);
      switch ( ev.type ) {
      case ConfigureNotify:
        if ( ev.xconfigure.window == __window ) {
          const bool size_changed = (ev.xconfigure.width != __width || ev.xconfigure.height != __height);
          const bool pos_changed = (ev.xconfigure.x != __x || ev.xconfigure.y != __y);
          __width = ev.xconfigure.width;
          __height = ev.xconfigure.height;
          __x = ev.xconfigure.x;
          __y = ev.xconfigure.y;
          if ( size_changed && __cb_resize ) __cb_resize(__owner, __width, __height);
          if ( pos_changed && __cb_move ) __cb_move(__owner, __x, __y);
        }
        break;
      case ClientMessage:
        if ( ev.xclient.message_type == __wm_protocols && static_cast<XAtom>(ev.xclient.data.l[0]) == __wm_delete_window ) {
          __should_close = true;
          if ( __cb_close ) __cb_close(__owner);
        }
        break;
      case Expose:
        if ( ev.xexpose.window == __window && __cb_expose ) __cb_expose(__owner);
        break;
      case FocusIn:
        if ( ev.xfocus.window == __window && __cb_focus ) __cb_focus(__owner, true);
        break;
      case FocusOut:
        if ( ev.xfocus.window == __window && __cb_focus ) __cb_focus(__owner, false);
        break;
      case MapNotify:
        if ( ev.xmap.window == __window && __cb_visibility ) __cb_visibility(__owner, true);
        break;
      case UnmapNotify:
        if ( ev.xunmap.window == __window && __cb_visibility ) __cb_visibility(__owner, false);
        break;
      default:
        break;
      }
    }
  }

  void
  destroy() noexcept
  {
    if ( !__disp || !__disp->lib() ) return;
    if ( __window ) __disp->lib()->XDestroyWindow(__disp->display(), __window);
    if ( __disp->display() ) __disp->lib()->XFlush(__disp->display());
    __window = 0;
    __cmap = 0;
  }
};

};      // namespace platform
};      // namespace gfx
};      // namespace micron
