//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__std.hpp"

#include "../except.hpp"
#include "../mutex/locks.hpp"
#include "stdout.hpp"

namespace micron
{

enum class style { bold = 1, dim = 2, italic = 3, underline = 4, blink = 5, reverse = 7, hidden = 8, reset = 0 };

enum class color {
  black = 30,
  red = 31,
  green = 32,
  yellow = 33,
  blue = 34,
  magenta = 35,
  cyan = 36,
  white = 37,
  reset = 39,
  fgblack = 40,
  fgred = 41,
  fggreen = 42,
  fgyellow = 43,
  fgblue = 44,
  fgmagenta = 45,
  fgcyan = 46,
  fgwhite = 47,
  fgreset = 49
};

struct ansi_colors {

  constexpr static const char *bold = "1";
  constexpr static const char *dim = "2";
  constexpr static const char *italic = "3";
  constexpr static const char *underline = "4";
  constexpr static const char *blink = "5";
  constexpr static const char *reverse = "7";
  constexpr static const char *hidden = "8";
  constexpr static const char *sreset = "0";

  constexpr static const char *black = "30";
  constexpr static const char *red = "31";
  constexpr static const char *green = "32";
  constexpr static const char *yellow = "33";
  constexpr static const char *blue = "34";
  constexpr static const char *magenta = "35";
  constexpr static const char *cyan = "36";
  constexpr static const char *white = "37";
  constexpr static const char *reset = "39";

  constexpr static const char *fgblack = "40";
  constexpr static const char *fgred = "41";
  constexpr static const char *fggreen = "42";
  constexpr static const char *fgyellow = "43";
  constexpr static const char *fgblue = "44";
  constexpr static const char *fgmagenta = "45";
  constexpr static const char *fgcyan = "46";
  constexpr static const char *fgwhite = "47";
  constexpr static const char *fgreset = "49";
};

inline void
set_color_reset(void)
{
  const char *buf = "\033[0m";
  io::print(buf);
};

inline void
set_color(color c, style s = style::bold)
{
  char buf[] = "\033[1;xxm";
  switch ( c ) {
  case color::black :
    buf[4] = ansi_colors::black[0];
    buf[5] = ansi_colors::black[1];
    break;
  case color::red :
    buf[4] = ansi_colors::red[0];
    buf[5] = ansi_colors::red[1];
    break;
  case color::green :
    buf[4] = ansi_colors::green[0];
    buf[5] = ansi_colors::green[1];
    break;
  case color::yellow :
    buf[4] = ansi_colors::yellow[0];
    buf[5] = ansi_colors::yellow[1];
    break;
  case color::blue :
    buf[4] = ansi_colors::blue[0];
    buf[5] = ansi_colors::blue[1];
    break;
  case color::magenta :
    buf[4] = ansi_colors::magenta[0];
    buf[5] = ansi_colors::magenta[1];
    break;
  case color::cyan :
    buf[4] = ansi_colors::cyan[0];
    buf[5] = ansi_colors::cyan[1];
    break;
  case color::white :
    buf[4] = ansi_colors::white[0];
    buf[5] = ansi_colors::white[1];
    break;
  case color::reset :
    buf[4] = ansi_colors::reset[0];
    buf[5] = ansi_colors::reset[1];
    break;
  case color::fgblack :
    buf[4] = ansi_colors::fgblack[0];
    buf[5] = ansi_colors::fgblack[1];
    break;
  case color::fgred :
    buf[4] = ansi_colors::fgred[0];
    buf[5] = ansi_colors::fgred[1];
    break;
  case color::fggreen :
    buf[4] = ansi_colors::fggreen[0];
    buf[5] = ansi_colors::fggreen[1];
    break;
  case color::fgyellow :
    buf[4] = ansi_colors::fgyellow[0];
    buf[5] = ansi_colors::fgyellow[1];
    break;
  case color::fgblue :
    buf[4] = ansi_colors::fgblue[0];
    buf[5] = ansi_colors::fgblue[1];
    break;
  case color::fgmagenta :
    buf[4] = ansi_colors::fgmagenta[0];
    buf[5] = ansi_colors::fgmagenta[1];
    break;
  case color::fgcyan :
    buf[4] = ansi_colors::fgcyan[0];
    buf[5] = ansi_colors::fgcyan[1];
    break;
  case color::fgwhite :
    buf[4] = ansi_colors::fgwhite[0];
    buf[5] = ansi_colors::fgwhite[1];
    break;
  case color::fgreset :
    buf[4] = ansi_colors::fgreset[0];
    buf[5] = ansi_colors::fgreset[1];
    break;
  };
  switch ( s ) {
  case style::bold :
    buf[2] = ansi_colors::bold[0];
    break;
  case style::dim :
    buf[2] = ansi_colors::dim[0];
    break;
  case style::italic :
    buf[2] = ansi_colors::italic[0];
    break;
  case style::underline :
    buf[2] = ansi_colors::underline[0];
    break;
  case style::blink :
    buf[2] = ansi_colors::blink[0];
    break;
  case style::reverse :
    buf[2] = ansi_colors::reverse[0];
    break;
  case style::hidden :
    buf[2] = ansi_colors::hidden[0];
    break;
  case style::reset :
    buf[2] = ansi_colors::sreset[0];
    break;
  };
  io::print(buf);
};

template <typename... T>
inline void
cerror(const T &...str)
{
  set_color(color::red, style::italic);
  set_color(color::red, style::bold);
  io::errorln(str...);
  set_color_reset();
  (exc_silent<except::standard_error>(str), ...);
}

template <typename... T>
inline void
__micron_log(const char *FILEMACRO, int line, const T &...str)
{
  set_color(color::cyan, style::italic);
  set_color(color::cyan, style::bold);
  io::print(FILEMACRO);
  io::print(":");
  set_color_reset();
  set_color(color::cyan, style::bold);
  io::print(line);
  io::print(": ");
  set_color_reset();
  set_color(color::yellow, style::bold);
  io::println(str...);
  set_color_reset();
  io::println("\n");
}

// has to be like this so we don't get conflicts with float/stl libs
#define infolog(x) __micron_log(__FILE__, __LINE__, x)

// WARNING: partially TS

// TODO: optimize

inline void
console_newline(void)
{
  io::println("\n");
}

template <typename... T>
  requires(micron::is_pointer_v<T> and ...)
inline void
console(const T &...str)
{
  io::println(str...);
}

template <typename... T>
inline void
console(const T &...str)
{
  io::println(str...);
}

template <typename... T>
inline void
consoled(const T &...str)
{
  io::print(str...);
}

template <typename... T>
inline void
console_bin(const T &...str)
{
  io::bin(str...);
}
};     // namespace micron
