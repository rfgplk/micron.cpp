//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "../../slice.hpp"

namespace micron::uxin::keymap
{
inline constexpr u32 unknown_symbol = 0xffffffffu;

constexpr bool
same(raw_slice<const char> text, const char *name) noexcept
{
  usize i = 0;
  for ( ; i < text.len && name[i]; ++i )
    if ( text.ptr[i] != name[i] ) return false;
  return i == text.len && !name[i];
}

constexpr u32
unicode_symbol(u32 cp) noexcept
{
  return cp <= 0xff ? cp : cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff) ? cp | 0x1000000u : unknown_symbol;
}

struct named_symbol {
  const char *name;
  u32 value;
};

inline constexpr named_symbol symbols[] = {
  { "NoSymbol", 0 },
  { "VoidSymbol", 0xffffff },
  { "space", 32 },
  { "exclam", '!' },
  { "quotedbl", '"' },
  { "numbersign", '#' },
  { "dollar", '$' },
  { "percent", '%' },
  { "ampersand", '&' },
  { "apostrophe", '\'' },
  { "quoteright", '\'' },
  { "parenleft", '(' },
  { "parenright", ')' },
  { "asterisk", '*' },
  { "plus", '+' },
  { "comma", ',' },
  { "minus", '-' },
  { "period", '.' },
  { "slash", '/' },
  { "colon", ':' },
  { "semicolon", ';' },
  { "less", '<' },
  { "equal", '=' },
  { "greater", '>' },
  { "question", '?' },
  { "at", '@' },
  { "bracketleft", '[' },
  { "backslash", '\\' },
  { "bracketright", ']' },
  { "asciicircum", '^' },
  { "underscore", '_' },
  { "grave", '`' },
  { "quoteleft", '`' },
  { "braceleft", '{' },
  { "bar", '|' },
  { "braceright", '}' },
  { "asciitilde", '~' },
  { "nobreakspace", 0xa0 },
  { "exclamdown", 0xa1 },
  { "cent", 0xa2 },
  { "sterling", 0xa3 },
  { "currency", 0xa4 },
  { "yen", 0xa5 },
  { "brokenbar", 0xa6 },
  { "section", 0xa7 },
  { "diaeresis", 0xa8 },
  { "copyright", 0xa9 },
  { "ordfeminine", 0xaa },
  { "guillemotleft", 0xab },
  { "notsign", 0xac },
  { "hyphen", 0xad },
  { "registered", 0xae },
  { "macron", 0xaf },
  { "degree", 0xb0 },
  { "plusminus", 0xb1 },
  { "twosuperior", 0xb2 },
  { "threesuperior", 0xb3 },
  { "acute", 0xb4 },
  { "mu", 0xb5 },
  { "paragraph", 0xb6 },
  { "periodcentered", 0xb7 },
  { "cedilla", 0xb8 },
  { "onesuperior", 0xb9 },
  { "masculine", 0xba },
  { "guillemotright", 0xbb },
  { "onequarter", 0xbc },
  { "onehalf", 0xbd },
  { "threequarters", 0xbe },
  { "questiondown", 0xbf },
  { "Agrave", 0xc0 },
  { "Aacute", 0xc1 },
  { "Acircumflex", 0xc2 },
  { "Atilde", 0xc3 },
  { "Adiaeresis", 0xc4 },
  { "Aring", 0xc5 },
  { "AE", 0xc6 },
  { "Ccedilla", 0xc7 },
  { "Egrave", 0xc8 },
  { "Eacute", 0xc9 },
  { "Ecircumflex", 0xca },
  { "Ediaeresis", 0xcb },
  { "Igrave", 0xcc },
  { "Iacute", 0xcd },
  { "Icircumflex", 0xce },
  { "Idiaeresis", 0xcf },
  { "ETH", 0xd0 },
  { "Ntilde", 0xd1 },
  { "Ograve", 0xd2 },
  { "Oacute", 0xd3 },
  { "Ocircumflex", 0xd4 },
  { "Otilde", 0xd5 },
  { "Odiaeresis", 0xd6 },
  { "multiply", 0xd7 },
  { "Oslash", 0xd8 },
  { "Ugrave", 0xd9 },
  { "Uacute", 0xda },
  { "Ucircumflex", 0xdb },
  { "Udiaeresis", 0xdc },
  { "Yacute", 0xdd },
  { "THORN", 0xde },
  { "ssharp", 0xdf },
  { "agrave", 0xe0 },
  { "aacute", 0xe1 },
  { "acircumflex", 0xe2 },
  { "atilde", 0xe3 },
  { "adiaeresis", 0xe4 },
  { "aring", 0xe5 },
  { "ae", 0xe6 },
  { "ccedilla", 0xe7 },
  { "egrave", 0xe8 },
  { "eacute", 0xe9 },
  { "ecircumflex", 0xea },
  { "ediaeresis", 0xeb },
  { "igrave", 0xec },
  { "iacute", 0xed },
  { "icircumflex", 0xee },
  { "idiaeresis", 0xef },
  { "eth", 0xf0 },
  { "ntilde", 0xf1 },
  { "ograve", 0xf2 },
  { "oacute", 0xf3 },
  { "ocircumflex", 0xf4 },
  { "otilde", 0xf5 },
  { "odiaeresis", 0xf6 },
  { "division", 0xf7 },
  { "oslash", 0xf8 },
  { "ugrave", 0xf9 },
  { "uacute", 0xfa },
  { "ucircumflex", 0xfb },
  { "udiaeresis", 0xfc },
  { "yacute", 0xfd },
  { "thorn", 0xfe },
  { "ydiaeresis", 0xff },
  { "BackSpace", 0xff08 },
  { "Tab", 0xff09 },
  { "Linefeed", 0xff0a },
  { "Clear", 0xff0b },
  { "Return", 0xff0d },
  { "Pause", 0xff13 },
  { "Scroll_Lock", 0xff14 },
  { "Sys_Req", 0xff15 },
  { "Escape", 0xff1b },
  { "Multi_key", 0xff20 },
  { "Home", 0xff50 },
  { "Left", 0xff51 },
  { "Up", 0xff52 },
  { "Right", 0xff53 },
  { "Down", 0xff54 },
  { "Prior", 0xff55 },
  { "Page_Up", 0xff55 },
  { "Next", 0xff56 },
  { "Page_Down", 0xff56 },
  { "End", 0xff57 },
  { "Begin", 0xff58 },
  { "Select", 0xff60 },
  { "Print", 0xff61 },
  { "Execute", 0xff62 },
  { "Insert", 0xff63 },
  { "Undo", 0xff65 },
  { "Redo", 0xff66 },
  { "Menu", 0xff67 },
  { "Find", 0xff68 },
  { "Cancel", 0xff69 },
  { "Help", 0xff6a },
  { "Break", 0xff6b },
  { "Mode_switch", 0xff7e },
  { "Num_Lock", 0xff7f },
  { "KP_Space", 0xff80 },
  { "KP_Tab", 0xff89 },
  { "KP_Enter", 0xff8d },
  { "KP_Home", 0xff95 },
  { "KP_Left", 0xff96 },
  { "KP_Up", 0xff97 },
  { "KP_Right", 0xff98 },
  { "KP_Down", 0xff99 },
  { "KP_Prior", 0xff9a },
  { "KP_Next", 0xff9b },
  { "KP_End", 0xff9c },
  { "KP_Begin", 0xff9d },
  { "KP_Insert", 0xff9e },
  { "KP_Delete", 0xff9f },
  { "KP_Equal", 0xffbd },
  { "KP_Multiply", 0xffaa },
  { "KP_Add", 0xffab },
  { "KP_Separator", 0xffac },
  { "KP_Subtract", 0xffad },
  { "KP_Decimal", 0xffae },
  { "KP_Divide", 0xffaf },
  { "Shift_L", 0xffe1 },
  { "Shift_R", 0xffe2 },
  { "Control_L", 0xffe3 },
  { "Control_R", 0xffe4 },
  { "Caps_Lock", 0xffe5 },
  { "Shift_Lock", 0xffe6 },
  { "Meta_L", 0xffe7 },
  { "Meta_R", 0xffe8 },
  { "Alt_L", 0xffe9 },
  { "Alt_R", 0xffea },
  { "Super_L", 0xffeb },
  { "Super_R", 0xffec },
  { "Hyper_L", 0xffed },
  { "Hyper_R", 0xffee },
  { "Delete", 0xffff },
  { "ISO_Level3_Shift", 0xfe03 },
  { "ISO_Level3_Latch", 0xfe04 },
  { "ISO_Level3_Lock", 0xfe05 },
  { "ISO_Next_Group", 0xfe08 },
  { "ISO_Prev_Group", 0xfe0a },
  { "ISO_First_Group", 0xfe0c },
  { "ISO_Last_Group", 0xfe0e },
  { "ISO_Level5_Shift", 0xfe11 },
  { "ISO_Level5_Latch", 0xfe12 },
  { "ISO_Level5_Lock", 0xfe13 },
  { "ISO_Left_Tab", 0xfe20 },
  { "dead_grave", 0xfe50 },
  { "dead_acute", 0xfe51 },
  { "dead_circumflex", 0xfe52 },
  { "dead_tilde", 0xfe53 },
  { "dead_macron", 0xfe54 },
  { "dead_breve", 0xfe55 },
  { "dead_abovedot", 0xfe56 },
  { "dead_diaeresis", 0xfe57 },
  { "dead_abovering", 0xfe58 },
  { "dead_doubleacute", 0xfe59 },
  { "dead_caron", 0xfe5a },
  { "dead_cedilla", 0xfe5b },
  { "dead_ogonek", 0xfe5c },
  { "dead_iota", 0xfe5d },
  { "dead_voiced_sound", 0xfe5e },
  { "dead_semivoiced_sound", 0xfe5f },
  { "dead_belowdot", 0xfe60 },
  { "dead_hook", 0xfe61 },
  { "dead_horn", 0xfe62 },
  { "dead_stroke", 0xfe63 },
  { "OE", 0x13bc },
  { "oe", 0x13bd },
  { "Ydiaeresis", 0x13be },
  { "EuroSign", 0x20ac },
};

struct legacy_symbol {
  const char *name;
  u32 symbol, unicode;
};

inline constexpr legacy_symbol latin[] = {
  { "Aogonek", 0x1a1, 0x104 },      { "aogonek", 0x1b1, 0x105 },      { "Lstroke", 0x1a3, 0x141 },      { "lstroke", 0x1b3, 0x142 },
  { "Lcaron", 0x1a5, 0x13d },       { "lcaron", 0x1b5, 0x13e },       { "Sacute", 0x1a6, 0x15a },       { "sacute", 0x1b6, 0x15b },
  { "Scaron", 0x1a9, 0x160 },       { "scaron", 0x1b9, 0x161 },       { "Scedilla", 0x1aa, 0x15e },     { "scedilla", 0x1ba, 0x15f },
  { "Tcaron", 0x1ab, 0x164 },       { "tcaron", 0x1bb, 0x165 },       { "Zacute", 0x1ac, 0x179 },       { "zacute", 0x1bc, 0x17a },
  { "Zcaron", 0x1ae, 0x17d },       { "zcaron", 0x1be, 0x17e },       { "Zabovedot", 0x1af, 0x17b },    { "zabovedot", 0x1bf, 0x17c },
  { "Racute", 0x1c0, 0x154 },       { "racute", 0x1e0, 0x155 },       { "Abreve", 0x1c3, 0x102 },       { "abreve", 0x1e3, 0x103 },
  { "Lacute", 0x1c5, 0x139 },       { "lacute", 0x1e5, 0x13a },       { "Cacute", 0x1c6, 0x106 },       { "cacute", 0x1e6, 0x107 },
  { "Ccaron", 0x1c8, 0x10c },       { "ccaron", 0x1e8, 0x10d },       { "Eogonek", 0x1ca, 0x118 },      { "eogonek", 0x1ea, 0x119 },
  { "Ecaron", 0x1cc, 0x11a },       { "ecaron", 0x1ec, 0x11b },       { "Dcaron", 0x1cf, 0x10e },       { "dcaron", 0x1ef, 0x10f },
  { "Dstroke", 0x1d0, 0x110 },      { "dstroke", 0x1f0, 0x111 },      { "Nacute", 0x1d1, 0x143 },       { "nacute", 0x1f1, 0x144 },
  { "Ncaron", 0x1d2, 0x147 },       { "ncaron", 0x1f2, 0x148 },       { "Odoubleacute", 0x1d5, 0x150 }, { "odoubleacute", 0x1f5, 0x151 },
  { "Rcaron", 0x1d8, 0x158 },       { "rcaron", 0x1f8, 0x159 },       { "Uring", 0x1d9, 0x16e },        { "uring", 0x1f9, 0x16f },
  { "Udoubleacute", 0x1db, 0x170 }, { "udoubleacute", 0x1fb, 0x171 }, { "Tcedilla", 0x1de, 0x162 },     { "tcedilla", 0x1fe, 0x163 },
};

constexpr u32
symbol_from_name(raw_slice<const char> name) noexcept
{
  if ( name.len == 1 ) return static_cast<u8>(name.ptr[0]);
  if ( name.len >= 5 && name.ptr[0] == 'U' ) {
    u32 cp = 0;
    bool hexadecimal = true, overflow = false;
    for ( usize i = 1; i < name.len; ++i ) {
      const char c = name.ptr[i];
      const u32 digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
      if ( digit == 16 ) {
        hexadecimal = false;
        break;
      }
      if ( cp > (0x10ffffu - digit) / 16 ) {
        overflow = true;
        break;
      }
      cp = cp * 16 + digit;
    }
    if ( overflow ) return unknown_symbol;
    if ( hexadecimal ) return unicode_symbol(cp);
  }
  if ( name.len >= 2 && name.len <= 3 && name.ptr[0] == 'F' ) {
    u32 n = 0;
    for ( usize i = 1; i < name.len; ++i ) {
      if ( name.ptr[i] < '0' || name.ptr[i] > '9' ) return unknown_symbol;
      n = n * 10 + name.ptr[i] - '0';
    }
    if ( n >= 1 && n <= 35 ) return 0xffbd + n;
  }
  if ( name.len == 4 && name.ptr[0] == 'K' && name.ptr[1] == 'P' && name.ptr[2] == '_' && name.ptr[3] >= '0' && name.ptr[3] <= '9' )
    return 0xffb0 + name.ptr[3] - '0';
  for ( const auto &entry : symbols )
    if ( same(name, entry.name) ) return entry.value;
  for ( const auto &entry : latin )
    if ( same(name, entry.name) ) return entry.symbol;
  return unknown_symbol;
}

constexpr u32
symbol_unicode(u32 symbol) noexcept
{
  if ( (symbol >= 32 && symbol <= 126) || (symbol >= 0xa0 && symbol <= 0xff) ) return symbol;
  if ( symbol >= 0x1000100 && symbol <= 0x110ffff ) {
    const u32 cp = symbol & 0xffffff;
    return cp >= 0xd800 && cp <= 0xdfff ? 0 : cp;
  }
  if ( symbol >= 0xffb0 && symbol <= 0xffb9 ) return '0' + symbol - 0xffb0;
  if ( symbol == 0xff80 ) return ' ';
  if ( symbol >= 0xffaa && symbol <= 0xffaf ) {
    constexpr char operators[] = "*+,-./";
    return operators[symbol - 0xffaa];
  }
  if ( symbol == 0xffbd ) return '=';
  if ( symbol == 0x20ac ) return symbol;
  if ( symbol >= 0x13bc && symbol <= 0x13be ) {
    constexpr u32 extra[] = { 0x152, 0x153, 0x178 };
    return extra[symbol - 0x13bc];
  }
  for ( const auto &entry : latin )
    if ( entry.symbol == symbol ) return entry.unicode;
  return 0;
}
}      // namespace micron::uxin::keymap
