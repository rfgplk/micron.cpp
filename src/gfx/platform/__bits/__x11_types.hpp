//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"

// Forward declarations of every X11 type we touch from gfx/platform/x11.hpp.
// Layouts mirror /usr/include/X11/Xlib.h so we can build XSetWindowAttributes
// and inspect XConfigureEvent / XClientMessageEvent without including Xlib
// (which would pull in libc).
//
// XID, Atom, etc. are spelled `unsigned long` in Xlib, which matches the
// host machine word on every Linux ABI we care about; using the plain type
// keeps the wire-level layout 1:1 with what libX11 sees.

namespace micron
{
namespace gfx
{
namespace platform
{

using XID = unsigned long;
using XAtom = unsigned long;
using XColormap = XID;
using XCursor = XID;
using XPixmap = XID;
using XTime = unsigned long;
using XVisualID = unsigned long;
using XWindow = XID;
using XDrawable = XID;
using XBool = int;
using XStatus = int;

struct __X11Display;
using XDisplay = __X11Display;      // opaque; libX11 owns the layout

// XVisualInfo — we read `visual` and `visualid`; libX11 fills the rest.
struct XVisual_opaque;

struct XVisualInfo {
  XVisual_opaque *visual;
  XVisualID visualid;
  int screen;
  int depth;
  int c_class;
  unsigned long red_mask;
  unsigned long green_mask;
  unsigned long blue_mask;
  int colormap_size;
  int bits_per_rgb;
};

// XSetWindowAttributes — passed to XCreateWindow. We typically set
// background_pixel, border_pixel, colormap, event_mask, and override_redirect.
struct XSetWindowAttributes {
  XPixmap background_pixmap;
  unsigned long background_pixel;
  XPixmap border_pixmap;
  unsigned long border_pixel;
  int bit_gravity;
  int win_gravity;
  int backing_store;
  unsigned long backing_planes;
  unsigned long backing_pixel;
  XBool save_under;
  long event_mask;
  long do_not_propagate_mask;
  XBool override_redirect;
  XColormap colormap;
  XCursor cursor;
};

// CWxxx flags for XCreateWindow attribute mask.
inline constexpr unsigned long CWBackPixmap = 1L << 0;
inline constexpr unsigned long CWBackPixel = 1L << 1;
inline constexpr unsigned long CWBorderPixmap = 1L << 2;
inline constexpr unsigned long CWBorderPixel = 1L << 3;
inline constexpr unsigned long CWBitGravity = 1L << 4;
inline constexpr unsigned long CWWinGravity = 1L << 5;
inline constexpr unsigned long CWBackingStore = 1L << 6;
inline constexpr unsigned long CWBackingPlanes = 1L << 7;
inline constexpr unsigned long CWBackingPixel = 1L << 8;
inline constexpr unsigned long CWOverrideRedirect = 1L << 9;
inline constexpr unsigned long CWSaveUnder = 1L << 10;
inline constexpr unsigned long CWEventMask = 1L << 11;
inline constexpr unsigned long CWDontPropagate = 1L << 12;
inline constexpr unsigned long CWColormap = 1L << 13;
inline constexpr unsigned long CWCursor = 1L << 14;

// Event masks (XSelectInput). Graphics-only events; pointer/keyboard masks
// go through io/uxin/ if the user wants input.
inline constexpr long NoEventMask = 0L;
inline constexpr long StructureNotifyMask = 1L << 17;
inline constexpr long ExposureMask = 1L << 15;
inline constexpr long PropertyChangeMask = 1L << 22;
inline constexpr long FocusChangeMask = 1L << 21;
inline constexpr long EnterWindowMask = 1L << 4;
inline constexpr long LeaveWindowMask = 1L << 5;

// Window classes for XCreateWindow.
inline constexpr int InputOutput = 1;
inline constexpr int InputOnly = 2;

// Visual class.
inline constexpr int TrueColor = 4;

// AllocNone / AllocAll for XCreateColormap.
inline constexpr int AllocNone = 0;
inline constexpr int AllocAll = 1;

// Event types we care about (XEvent.type values).
inline constexpr int KeyPress = 2;
inline constexpr int KeyRelease = 3;
inline constexpr int ButtonPress = 4;
inline constexpr int ButtonRelease = 5;
inline constexpr int MotionNotify = 6;
inline constexpr int EnterNotify = 7;
inline constexpr int LeaveNotify = 8;
inline constexpr int FocusIn = 9;
inline constexpr int FocusOut = 10;
inline constexpr int Expose = 12;
inline constexpr int MapNotify = 19;
inline constexpr int UnmapNotify = 18;
inline constexpr int ConfigureNotify = 22;
inline constexpr int ClientMessage = 33;

// Specific event structs (XEvent is a union over all of these; type is at
// offset 0 in every variant, so casting through type is sound).

struct XConfigureEvent {
  int type;
  unsigned long serial;
  XBool send_event;
  XDisplay *display;
  XWindow event;
  XWindow window;
  int x, y;
  int width, height;
  int border_width;
  XWindow above;
  XBool override_redirect;
};

struct XClientMessageEvent {
  int type;
  unsigned long serial;
  XBool send_event;
  XDisplay *display;
  XWindow window;
  XAtom message_type;
  int format;

  union {
    char b[20];
    short s[10];
    long l[5];
  } data;
};

struct XFocusChangeEvent {
  int type;
  unsigned long serial;
  XBool send_event;
  XDisplay *display;
  XWindow window;
  int mode;
  int detail;
};

struct XExposeEvent {
  int type;
  unsigned long serial;
  XBool send_event;
  XDisplay *display;
  XWindow window;
  int x, y;
  int width, height;
  int count;
};

struct XMapEvent {
  int type;
  unsigned long serial;
  XBool send_event;
  XDisplay *display;
  XWindow event;
  XWindow window;
  XBool override_redirect;
};

struct XUnmapEvent {
  int type;
  unsigned long serial;
  XBool send_event;
  XDisplay *display;
  XWindow event;
  XWindow window;
  XBool from_configure;
};

// sizeof(XEvent) on x86_64 is 192 bytes; on i686 it is 96. We use a generous
// upper bound so the buffer can hold any concrete subtype regardless of
// architecture, and rely on the `type` field at offset 0 for dispatch.
struct XEvent {
  union {
    int type;
    XConfigureEvent xconfigure;
    XClientMessageEvent xclient;
    XFocusChangeEvent xfocus;
    XExposeEvent xexpose;
    XMapEvent xmap;
    XUnmapEvent xunmap;
    char __pad[256];
  };
};

};      // namespace platform
};      // namespace gfx
};      // namespace micron
