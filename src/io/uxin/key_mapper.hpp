//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{

namespace uxin
{

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr T
__ev_code_to_integral(const u16 c)
{
  switch ( c ) {
  case key_esc :
    return 0x1b;
    break;
  case key_1 :
    return '1';
    break;
  case key_2 :
    return '2';
    break;
  case key_3 :
    return '3';
    break;
  case key_4 :
    return '4';
    break;
  case key_5 :
    return '5';
    break;
  case key_6 :
    return '6';
    break;
  case key_7 :
    return '7';
    break;
  case key_8 :
    return '8';
    break;
  case key_9 :
    return '9';
    break;
  case key_0 :
    return '0';
    break;
  case key_minus :
    return '-';
    break;
  case key_equal :
    return '=';
    break;
  case key_backspace :
    return 0x8;
    break;
  case key_tab :
    return 0x9;
    break;
  case key_q :
    return 'q';
    break;
  case key_w :
    return 'w';
    break;
  case key_e :
    return 'e';
    break;
  case key_r :
    return 'r';
    break;
  case key_t :
    return 't';
    break;
  case key_y :
    return 'y';
    break;
  case key_u :
    return 'u';
    break;
  case key_i :
    return 'i';
    break;
  case key_o :
    return 'o';
    break;
  case key_p :
    return 'p';
    break;
  case key_leftbrace :
    return '{';
    break;
  case key_rightbrace :
    return '}';
    break;
  case key_enter :
    return 0x0;
    break;
  case key_leftctrl :
    return 0x0;
    break;
  case key_a :
    return 'a';
    break;
  case key_s :
    return 's';
    break;
  case key_d :
    return 'd';
    break;
  case key_f :
    return 'f';
    break;
  case key_g :
    return 'g';
    break;
  case key_h :
    return 'h';
    break;
  case key_j :
    return 'j';
    break;
  case key_k :
    return 'k';
    break;
  case key_l :
    return 'l';
    break;
  case key_semicolon :
    return ';';
    break;
  case key_apostrophe :
    return '"';
    break;
  case key_grave :
    return '`';
    break;
  case key_leftshift :
    return 0x15;
    break;
  case key_backslash :
    return '\\';
    break;
  case key_z :
    return 'z';
    break;
  case key_x :
    return 'x';
    break;
  case key_c :
    return 'c';
    break;
  case key_v :
    return 'v';
    break;
  case key_b :
    return 'b';
    break;
  case key_n :
    return 'n';
    break;
  case key_m :
    return 'm';
    break;
  case key_comma :
    return ',';
    break;
  case key_dot :
    return '.';
    break;
  case key_slash :
    return '/';
    break;
  case key_rightshift :
    return 0x14;
    break;
  case key_kpasterisk :
    return '*';
    break;
  case key_leftalt :
    return 0x0;
    break;
  case key_space :
    return ' ';
    break;
  case key_capslock :
    return 0x0;
    break;
  case key_f1 :
    return 119;     // not really valid, but doing it like htis
    break;
  case key_f2 :
    return 118;
    break;
  case key_f3 :
    return 117;
    break;
  case key_f4 :
    return 116;
    break;
  case key_f5 :
    return 115;
    break;
  case key_f6 :
    return 114;
    break;
  case key_f7 :
    return 113;
    break;
  case key_f8 :
    return 112;
    break;
  case key_f9 :
    return 111;
    break;
  case key_f10 :
    return 122;
    break;
  case key_numlock :
    return 0x0;
    break;
  case key_scrolllock :
    return 0x0;
    break;
  case key_kp7 :
    return '7';
    break;
  case key_kp8 :
    return '8';
    break;
  case key_kp9 :
    return '9';
    break;
  case key_kpminus :
    return '-';
    break;
  case key_kp4 :
    return '4';
    break;
  case key_kp5 :
    return '5';
    break;
  case key_kp6 :
    return '6';
    break;
  case key_kpplus :
    return '+';
    break;
  case key_kp1 :
    return '1';
    break;
  case key_kp2 :
    return '2';
    break;
  case key_kp3 :
    return '3';
    break;
  case key_kp0 :
    return '0';
    break;
  case key_kpdot :
    return '.';
    break;
  case key_f11 :
    return 121;
    break;
  case key_f12 :
    return 120;
    break;
  case key_kpslash :
    return '/';
    break;
  case key_linefeed :
    return '\r';
    break;
  case key_up :
    return 127;
    break;
  case key_pageup :
    return 126;
    break;
  case key_left :
    return 125;
    break;
  case key_right :
    return 124;
    break;
  case key_down :
    return 123;
    break;
  case key_kpequal :
    return '=';
    break;
  case key_kpcomma :
    return ',';
    break;
  }
  return 0x0;
}

};

};
