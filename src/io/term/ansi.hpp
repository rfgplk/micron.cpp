//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// ECMA-48 std compliant list of terminal escape codes
// NOTE: all strings which accept a var/input currently use format style '{}' pattern matching, sed to %d if printf style is needed

namespace micron
{
namespace ansi
{
enum class group : usize {
  c0 = 0,      // C0 control codes (0x00–0x1F) + DEL (0x7F)
  c1,          // C1 controls
  fe,          // Fe/Fp/nF escape sequences
  csi,         // CSI
  sgr,         // SGR
  osc,         // OSC
  dcs,         // DCS
  pm,          // PM
  apc,         // APC
  charset,
  mouse,      // DECSET/DECRST private mouse tracking modes
  misc,
  __end
};

enum c0_code : usize {
  nul,      // 0x00  Null
  soh,      // 0x01  Start of Heading
  stx,      // 0x02  Start of Text
  etx,      // 0x03  End of Text
  eot,      // 0x04  End of Transmission
  enq,      // 0x05  Enquiry
  ack,      // 0x06  Acknowledge
  bel,      // 0x07  Bell / Alert
  bs,       // 0x08  Backspace
  ht,       // 0x09  Character Tabulation (Horizontal Tab)
  lf,       // 0x0A  Line Feed
  vt,       // 0x0B  Line Tabulation (Vertical Tab)
  ff,       // 0x0C  Form Feed
  cr,       // 0x0D  Carriage Return
  so,       // 0x0E  Shift Out
  si,       // 0x0F  Shift In
  dle,      // 0x10  Data Link Escape
  dc1,      // 0x11  Device Control 1 (XON)
  dc2,      // 0x12  Device Control 2
  dc3,      // 0x13  Device Control 3 (XOFF)
  dc4,      // 0x14  Device Control 4
  nak,      // 0x15  Negative Acknowledge
  syn,      // 0x16  Synchronous Idle
  etb,      // 0x17  End of Transmission Block
  can,      // 0x18  Cancel  (aborts in-progress escape sequence)
  em,       // 0x19  End of Medium
  sub,      // 0x1A  Substitute
  esc,      // 0x1B  Escape
  is4,      // 0x1C  Information Separator Four
  is3,      // 0x1D  Information Separator Three
  is2,      // 0x1E  Information Separator Two
  is1,      // 0x1F  Information Separator One
  del,      // 0x7F  Delete
  c0_count
};

constexpr const char *C0[] = {
  "\x00",      // nul
  "\x01",      // soh
  "\x02",      // stx
  "\x03",      // etx
  "\x04",      // eot
  "\x05",      // enq
  "\x06",      // ack
  "\x07",      // bel
  "\x08",      // bs
  "\x09",      // ht
  "\x0a",      // lf
  "\x0b",      // vt
  "\x0c",      // ff
  "\x0d",      // cr
  "\x0e",      // so
  "\x0f",      // si
  "\x10",      // dle
  "\x11",      // dc1
  "\x12",      // dc2
  "\x13",      // dc3
  "\x14",      // dc4
  "\x15",      // nak
  "\x16",      // syn
  "\x17",      // etb
  "\x18",      // can
  "\x19",      // em
  "\x1a",      // sub
  "\x1b",      // esc
  "\x1c",      // is4
  "\x1d",      // is3
  "\x1e",      // is2
  "\x1f",      // is1
  "\x7f"       // del
};

enum c1_code : usize {
  nel,        // ESC E  (U+0085)  Next Line
  hts,        // ESC H  (U+0088)  Character Tabulation Set
  ri,         // ESC M  (U+008D)  Reverse Line Feed (Reverse Index)
  ss2,        // ESC N  (U+008E)  Single-Shift Two
  ss3,        // ESC O  (U+008F)  Single-Shift Three
  dcs_o,      // ESC P  (U+0090)  Device Control String
  spa,        // ESC V  (U+0096)  Start of Protected Area
  epa,        // ESC W  (U+0097)  End of Protected Area
  sos,        // ESC X  (U+0098)  Start of String
  sci,        // ESC Z  (U+009A)  Single Character Introducer
  csi_o,      // ESC [  (U+009B)  Control Sequence Introducer
  st,         // ESC \  (U+009C)  String Terminator
  osc_o,      // ESC ]  (U+009D)  Operating System Command
  pm_o,       // ESC ^  (U+009E)  Privacy Message
  apc_o,      // ESC _  (U+009F)  Application Program Command
  c1_count
};

constexpr const char *C1[] = {
  "\x1bE",       // nel
  "\x1bH",       // hts
  "\x1bM",       // ri
  "\x1bN",       // ss2
  "\x1bO",       // ss3
  "\x1bP",       // dcs_o
  "\x1bV",       // spa
  "\x1bW",       // epa
  "\x1bX",       // sos
  "\x1bZ",       // sci
  "\x1b[",       // csi_o
  "\x1b\\",      // st
  "\x1b]",       // osc_o
  "\x1b^",       // pm_o
  "\x1b_"        // apc_o
};

enum fe_code : usize {
  s7c1t,      // ESC SP F:  select 7-bit C1 transmission
  s8c1t,      // ESC SP G:  select 8-bit C1 transmission

  ansi_level1,      // ESC SP L
  ansi_level2,      // ESC SP M
  ansi_level3,      // ESC SP N

  decsc,      // ESC 7:  DECSC save cursor + attributes
  decrc,      // ESC 8:  DECRC restore cursor + attributes

  decaln,      // ESC # 8:  fill screen with 'E'

  deckpam,      // ESC = :  DECKPAM application keypad
  deckpnm,      // ESC > :  DECKPNM numeric keypad

  ls2,       // ESC n:  Locking Shift 2
  ls3,       // ESC o:  Locking Shift 3
  ls1r,      // ESC ~:  Locking Shift 1 Right
  ls2r,      // ESC }:  Locking Shift 2 Right
  ls3r,      // ESC |:  Locking Shift 3 Right

  back_index,      // ESC 6:  reverse index in current column
  fwd_index,       // ESC 9:  forward index in current column

  fe_count
};

constexpr const char *FE[] = {
  "\x1b F",      // s7c1t       (ESC SP F)
  "\x1b G",      // s8c1t       (ESC SP G)
  "\x1b L",      // ansi_level1 (ESC SP L)
  "\x1b M",      // ansi_level2 (ESC SP M)
  "\x1b N",      // ansi_level3 (ESC SP N)
  "\x1b7",       // decsc
  "\x1b8",       // decrc
  "\x1b#8",      // decaln
  "\x1b=",       // deckpam
  "\x1b>",       // deckpnm
  "\x1bn",       // ls2
  "\x1bo",       // ls3
  "\x1b~",       // ls1r
  "\x1b}",       // ls2r
  "\x1b|",       // ls3r
  "\x1b6",       // back_index
  "\x1b9"        // fwd_index
};

enum csi_code : usize {
  cursor_up,                // CUU  ESC[{}A      move up N lines
  cursor_down,              // CUD  ESC[{}B      move down N lines
  cursor_forward,           // CUF  ESC[{}C      move right N columns
  cursor_back,              // CUB  ESC[{}D      move left N columns
  cursor_down_line,         // CNL  ESC[{}E      down N lines, column 1
  cursor_up_line,           // CPL  ESC[{}F      up N lines, column 1
  cursor_column,            // CHA  ESC[{}G      absolute column  (1-based)
  cursor_position,          // CUP  ESC[{};{}H   row, col         (1-based)
  cursor_position_hvp,      // HVP  ESC[{};{}f   identical to CUP
  line_pos_abs,             // VPA  ESC[{}d      absolute row     (1-based)
  line_pos_rel,             // VPR  ESC[{}e      relative row (down)
  horiz_pos_abs,            // HPA  ESC[{}`      absolute column  (synonym CHA)
  horiz_pos_rel,            // HPR  ESC[{}a      relative column (forward)
  horiz_tab_fwd,            // CHT  ESC[{}I      advance N tab stops
  horiz_tab_back,           // CBT  ESC[{}Z      retreat N tab stops
  save_cursor,              // SCP  ESC[s        ANSI save cursor position
  restore_cursor,           // RCP  ESC[u        ANSI restore cursor position

  tab_clear_current,      // TBC  ESC[0g       clear tab stop at cursor
  tab_clear_all,          // TBC  ESC[3g       clear all tab stops

  clear_screen_to_end,          // ED   ESC[0J       erase from cursor to screen end
  clear_screen_to_start,        // ED   ESC[1J       erase from screen start to cursor
  clear_screen_all,             // ED   ESC[2J       erase entire display
  clear_screen_scrollback,      // ED   ESC[3J       erase scrollback buffer (xterm)
  clear_line_to_right,          // EL   ESC[0K       erase from cursor to line end
  clear_line_to_left,           // EL   ESC[1K       erase from line start to cursor
  clear_line_all,               // EL   ESC[2K       erase entire line
  erase_chars,                  // ECH  ESC[{}X      erase N characters at cursor
  erase_field_to_end,           // EF   ESC[0N       erase in field  (ECMA-48 §8.3.40)
  erase_field_to_start,         // EF   ESC[1N
  erase_field_all,              // EF   ESC[2N
  erase_area_to_end,            // EA   ESC[0O       erase in area   (ECMA-48 §8.3.37)
  erase_area_to_start,          // EA   ESC[1O
  erase_area_all,               // EA   ESC[2O

  insert_chars,      // ICH  ESC[{}@      insert N blank characters
  delete_chars,      // DCH  ESC[{}P      delete N characters
  insert_lines,      // IL   ESC[{}L      insert N blank lines
  delete_lines,      // DL   ESC[{}M      delete N lines
  repeat_char,       // REP  ESC[{}b      repeat preceding graphic N times

  scroll_up,                // SU   ESC[{}S      scroll viewport up N lines (pan text up)
  scroll_down,              // SD   ESC[{}T      scroll viewport down N lines
  scroll_left,              // SL   ESC[{} @     scroll left N columns  (ECMA-48 Ann.C, SP @)
  scroll_right,             // SR   ESC[{} A     scroll right N columns (SP A)
  scroll_region_set,        // DECSTBM  ESC[{};{}r  set scrolling region (top, bottom)
  scroll_region_reset,      // DECSTBM  ESC[r       reset scrolling region to full screen

  next_page,          // NP   ESC[{}U      advance N pages
  prev_page,          // PP   ESC[{}V      go back N pages
  page_pos_abs,       // PPA  ESC[{} P     page position absolute (SP P)
  page_pos_fwd,       // PPR  ESC[{} Q     page position forward  (SP Q)
  page_pos_back,      // PPB  ESC[{} R     page position backward (SP R)

  insert_columns,      // DECIC  ESC[{}'}   insert N columns at cursor
  delete_columns,      // DECDC  ESC[{}'~   delete N columns at cursor

  fill_rect,               // DECFRA  ESC[{};{};{};{};{}x  fill rect with char (char,top,left,bot,right)
  erase_rect,              // DECERA  ESC[{};{};{};{}$z    erase rect         (top,left,bot,right)
  sel_erase_rect,          // DECSERA ESC[{};{};{};{}${    selective erase rect
  copy_rect,               // DECCRA  ESC[{};{};{};{};{};{};{};{}t  copy rect (srctop,srcl,srcbot,srcr,srcpg,dsttop,dstl,dstpg)
  change_attrs_rect,       // DECCARA ESC[{};{};{};{};{}$r  change SGR attrs in rect (top,left,bot,right,attr)
  reverse_attrs_rect,      // DECRARA ESC[{};{};{};{};{}$t  reverse attrs in rect

  cursor_position_report,      // CPR     ESC[6n    request cursor pos → ESC[row;colR
  device_status_report,        // DSR     ESC[5n    device status      → ESC[0n
  primary_da,                  // DA      ESC[c     primary device attributes
  secondary_da,                // DA2     ESC[>c    secondary device attributes
  tertiary_da,                 // DA3     ESC[=c    tertiary device attributes
  xtversion,                   // XTVERSION  ESC[>0q  xterm version query
  request_mode,                // DECRQM  ESC[{}$p  request ANSI mode state
  request_mode_private,        // DECRQM  ESC[?{}$p request DEC private mode state

  set_mode,        // SM  ESC[{}h   set ANSI mode (e.g. 4=insert, 20=LNM)
  reset_mode,      // RM  ESC[{}l   reset ANSI mode

  soft_reset,               // DECSTR  ESC[!p       soft terminal reset
  set_conformance,          // DECSCL  ESC[{};{}"p  set conformance level (level, mode)
  set_char_protection,      // DECSCA  ESC[{}"q     character protection attribute

  // 0,1=blinking block  2=steady block  3=blinking underline
  //     4=steady underline  5=blinking bar   6=steady bar
  cursor_style,      // DECSCUSR  ESC[{} q  (SP q)

  //   1=deiconify  2=iconify  3=move(x,y)  4=resize px(h,w)
  //   5=raise  6=lower  7=refresh  8=resize chars(rows,cols)
  //   9=maximize/restore  10=full-screen  11-20=queries
  window_ops,       // XTWINOPS  ESC[{}t
  window_ops2,      // XTWINOPS  ESC[{};{};{}t  (for 3-arg ops like move/resize)

  csi_count
};

constexpr const char *CSI[] = {
  // cursor movement
  "\x1b[{}A",         // cursor_up
  "\x1b[{}B",         // cursor_down
  "\x1b[{}C",         // cursor_forward
  "\x1b[{}D",         // cursor_back
  "\x1b[{}E",         // cursor_down_line
  "\x1b[{}F",         // cursor_up_line
  "\x1b[{}G",         // cursor_column
  "\x1b[{};{}H",      // cursor_position      (row, col)
  "\x1b[{};{}f",      // cursor_position_hvp  (row, col)
  "\x1b[{}d",         // line_pos_abs
  "\x1b[{}e",         // line_pos_rel
  "\x1b[{}`",         // horiz_pos_abs
  "\x1b[{}a",         // horiz_pos_rel
  "\x1b[{}I",         // horiz_tab_fwd
  "\x1b[{}Z",         // horiz_tab_back
  "\x1b[s",           // save_cursor
  "\x1b[u",           // restore_cursor

  // tab stops
  "\x1b[0g",      // tab_clear_current
  "\x1b[3g",      // tab_clear_all

  // erase
  "\x1b[0J",       // clear_screen_to_end
  "\x1b[1J",       // clear_screen_to_start
  "\x1b[2J",       // clear_screen_all
  "\x1b[3J",       // clear_screen_scrollback
  "\x1b[0K",       // clear_line_to_right
  "\x1b[1K",       // clear_line_to_left
  "\x1b[2K",       // clear_line_all
  "\x1b[{}X",      // erase_chars
  "\x1b[0N",       // erase_field_to_end
  "\x1b[1N",       // erase_field_to_start
  "\x1b[2N",       // erase_field_all
  "\x1b[0O",       // erase_area_to_end
  "\x1b[1O",       // erase_area_to_start
  "\x1b[2O",       // erase_area_all

  // insert / delete
  "\x1b[{}@",      // insert_chars
  "\x1b[{}P",      // delete_chars
  "\x1b[{}L",      // insert_lines
  "\x1b[{}M",      // delete_lines
  "\x1b[{}b",      // repeat_char

  // scroll
  "\x1b[{}S",         // scroll_up
  "\x1b[{}T",         // scroll_down
  "\x1b[{} @",        // scroll_left     (literal SP before @)
  "\x1b[{} A",        // scroll_right    (literal SP before A)
  "\x1b[{};{}r",      // scroll_region_set   (top, bottom)
  "\x1b[r",           // scroll_region_reset

  // page
  "\x1b[{}U",       // next_page
  "\x1b[{}V",       // prev_page
  "\x1b[{} P",      // page_pos_abs    (SP P)
  "\x1b[{} Q",      // page_pos_fwd    (SP Q)
  "\x1b[{} R",      // page_pos_back   (SP R)

  // column insert / delete
  "\x1b[{}'}",      // insert_columns
  "\x1b[{}'~",      // delete_columns

  // rectangular area
  "\x1b[{};{};{};{};{}x",               // fill_rect          (char,top,left,bot,right)
  "\x1b[{};{};{};{}$z",                 // erase_rect         (top,left,bot,right)
  "\x1b[{};{};{};{}${",                 // sel_erase_rect
  "\x1b[{};{};{};{};{};{};{};{}t",      // copy_rect
  "\x1b[{};{};{};{};{}$r",              // change_attrs_rect
  "\x1b[{};{};{};{};{}$t",              // reverse_attrs_rect

  // reports
  "\x1b[6n",         // cursor_position_report
  "\x1b[5n",         // device_status_report
  "\x1b[c",          // primary_da
  "\x1b[>c",         // secondary_da
  "\x1b[=c",         // tertiary_da
  "\x1b[>0q",        // xtversion
  "\x1b[{}$p",       // request_mode
  "\x1b[?{}$p",      // request_mode_private

  // mode
  "\x1b[{}h",      // set_mode
  "\x1b[{}l",      // reset_mode

  // conformance / reset
  "\x1b[!p",            // soft_reset
  "\x1b[{};{}\"p",      // set_conformance     (level, mode)
  "\x1b[{}\"q",         // set_char_protection (0=eraseable, 1=not)

  // cursor style
  "\x1b[{} q",      // cursor_style        (SP q)

  // window ops
  "\x1b[{}t",           // window_ops
  "\x1b[{};{};{}t"      // window_ops2
};

enum sgr_code : usize {
  reset,      // 0    all attributes off

  bold,                  // 1    bold / increased intensity
  dim,                   // 2    faint / decreased intensity
  italic,                // 3
  underline,             // 4    single underline
  blink_slow,            // 5    < 150 bpm
  blink_fast,            // 6    >= 150 bpm (rarely supported)
  inverse,               // 7    reverse video (swap fg/bg)
  hidden,                // 8    concealed / invisible
  strikethrough,         // 9    crossed-out
  double_underline,      // 21   (ECMA-48: doubly underlined)
  overline,              // 53

  bold_dim_off,                  // 22   normal intensity (resets both bold and dim)
  italic_off,                    // 23
  underline_off,                 // 24   resets single and double underline
  blink_off,                     // 25
  proportional_spacing_off,      // 26   ECMA-48: reserved for proportional spacing control
  inverse_off,                   // 27
  hidden_off,                    // 28
  strikethrough_off,             // 29
  framed_encircled_off,          // 54
  overline_off,                  // 55

  framed,         // 51
  encircled,      // 52

  ideogram_underline,             // 60   right side line (ideographic scripts)
  ideogram_double_underline,      // 61
  ideogram_overline,              // 62   left side line
  ideogram_double_overline,       // 63
  ideogram_stress,                // 64
  ideogram_off,                   // 65   cancel all ideogram attributes

  superscript,                    // 73
  subscript,                      // 74
  superscript_subscript_off,      // 75

  fg_black,
  fg_red,
  fg_green,
  fg_yellow,
  fg_blue,
  fg_magenta,
  fg_cyan,
  fg_white,
  fg_default,      // 39  reset to terminal default foreground

  bg_black,
  bg_red,
  bg_green,
  bg_yellow,
  bg_blue,
  bg_magenta,
  bg_cyan,
  bg_white,
  bg_default,      // 49  reset to terminal default background

  fg_bright_black,
  fg_bright_red,
  fg_bright_green,
  fg_bright_yellow,
  fg_bright_blue,
  fg_bright_magenta,
  fg_bright_cyan,
  fg_bright_white,

  bg_bright_black,
  bg_bright_red,
  bg_bright_green,
  bg_bright_yellow,
  bg_bright_blue,
  bg_bright_magenta,
  bg_bright_cyan,
  bg_bright_white,

  fg_256,      // 38;5;n
  bg_256,      // 48;5;n
  ul_256,      // 58;5;n   underline colour  (kitty / VTE ≥ 0.51)

  fg_rgb,      // 38;2;r;g;b
  bg_rgb,      // 48;2;r;g;b
  ul_rgb,      // 58;2;r;g;b   underline colour

  ul_reset,      // 59

  sgr_count
};

constexpr const char *SGR[] = {
  "\x1b[0m",      // reset

  "\x1b[1m",       // bold
  "\x1b[2m",       // dim
  "\x1b[3m",       // italic
  "\x1b[4m",       // underline
  "\x1b[5m",       // blink_slow
  "\x1b[6m",       // blink_fast
  "\x1b[7m",       // inverse
  "\x1b[8m",       // hidden
  "\x1b[9m",       // strikethrough
  "\x1b[21m",      // double_underline
  "\x1b[53m",      // overline

  "\x1b[22m",      // bold_dim_off
  "\x1b[23m",      // italic_off
  "\x1b[24m",      // underline_off
  "\x1b[25m",      // blink_off
  "\x1b[26m",      // proportional_spacing_off
  "\x1b[27m",      // inverse_off
  "\x1b[28m",      // hidden_off
  "\x1b[29m",      // strikethrough_off
  "\x1b[54m",      // framed_encircled_off
  "\x1b[55m",      // overline_off

  "\x1b[51m",      // framed
  "\x1b[52m",      // encircled

  "\x1b[60m",      // ideogram_underline
  "\x1b[61m",      // ideogram_double_underline
  "\x1b[62m",      // ideogram_overline
  "\x1b[63m",      // ideogram_double_overline
  "\x1b[64m",      // ideogram_stress
  "\x1b[65m",      // ideogram_off

  "\x1b[73m",      // superscript
  "\x1b[74m",      // subscript
  "\x1b[75m",      // superscript_subscript_off

  "\x1b[30m",      // fg_black
  "\x1b[31m",      // fg_red
  "\x1b[32m",      // fg_green
  "\x1b[33m",      // fg_yellow
  "\x1b[34m",      // fg_blue
  "\x1b[35m",      // fg_magenta
  "\x1b[36m",      // fg_cyan
  "\x1b[37m",      // fg_white
  "\x1b[39m",      // fg_default

  "\x1b[40m",      // bg_black
  "\x1b[41m",      // bg_red
  "\x1b[42m",      // bg_green
  "\x1b[43m",      // bg_yellow
  "\x1b[44m",      // bg_blue
  "\x1b[45m",      // bg_magenta
  "\x1b[46m",      // bg_cyan
  "\x1b[47m",      // bg_white
  "\x1b[49m",      // bg_default

  "\x1b[90m",      // fg_bright_black
  "\x1b[91m",      // fg_bright_red
  "\x1b[92m",      // fg_bright_green
  "\x1b[93m",      // fg_bright_yellow
  "\x1b[94m",      // fg_bright_blue
  "\x1b[95m",      // fg_bright_magenta
  "\x1b[96m",      // fg_bright_cyan
  "\x1b[97m",      // fg_bright_white

  "\x1b[100m",      // bg_bright_black
  "\x1b[101m",      // bg_bright_red
  "\x1b[102m",      // bg_bright_green
  "\x1b[103m",      // bg_bright_yellow
  "\x1b[104m",      // bg_bright_blue
  "\x1b[105m",      // bg_bright_magenta
  "\x1b[106m",      // bg_bright_cyan
  "\x1b[107m",      // bg_bright_white

  "\x1b[38;5;{}m",      // fg_256             (index 0–255)
  "\x1b[48;5;{}m",      // bg_256
  "\x1b[58;5;{}m",      // ul_256

  "\x1b[38;2;{};{};{}m",      // fg_rgb             (red, green, blue)
  "\x1b[48;2;{};{};{}m",      // bg_rgb
  "\x1b[58;2;{};{};{}m",      // ul_rgb

  "\x1b[59m"      // ul_reset
};

enum osc_code : usize {
  set_icon_and_title,       // 0;text     set icon name and window title
  set_icon_name,            // 1;text     set icon name only
  set_title,                // 2;text     set window title only
  set_xprop,                // 3;prop=val set X11 window property
  set_color,                // 4;n;spec   set palette colour n to spec
  query_color,              // 4;n;?      query palette colour n -> OSC 4;n;rgb:rr/gg/bb
  change_fg,                // 10;spec    set default foreground colour
  query_fg,                 // 10;?       query default foreground
  change_bg,                // 11;spec    set default background colour
  query_bg,                 // 11;?       query default background
  change_cursor_color,      // 12;spec    set text cursor colour
  query_cursor_color,       // 12;?       query text cursor colour
  change_mouse_fg,          // 13;spec    set mouse foreground colour
  query_mouse_fg,           // 13;?
  change_mouse_bg,          // 14;spec    set mouse background colour
  query_mouse_bg,           // 14;?
  change_tek_fg,            // 15;spec    Tektronix foreground
  query_tek_fg,             // 15;?
  change_tek_bg,            // 16;spec    Tektronix background
  query_tek_bg,             // 16;?
  change_highlight_bg,      // 17;spec    highlight background
  change_tek_cursor,        // 18;spec    Tektronix cursor colour
  change_highlight_fg,      // 19;spec    highlight foreground
  set_font,                 // 50;spec    set VT font (xterm)
  clipboard_set,            // 52;c;data  set clipboard (base64-encoded)
  clipboard_query,          // 52;c;?     query clipboard
  reset_color,              // 104;n      reset palette colour n to default
  reset_fg,                 // 110        reset default foreground to default
  reset_bg,                 // 111        reset default background to default
  reset_cursor_color,       // 112        reset cursor colour to default
  reset_mouse_fg,           // 113
  reset_mouse_bg,           // 114
  hyperlink_set,            // 8;params;uri   start hyperlink (IETF-draft)
  hyperlink_end,            // 8;;             end hyperlink
  notify,                   // 9;msg           desktop notification (iTerm2/Hyper)
  shell_prompt_start,       // 133;A           FinalTerm / shell integration
  shell_prompt_end,         // 133;B
  shell_cmd_start,          // 133;C
  shell_cmd_end,            // 133;D;code      exit code appended
  iterm2,                   // 1337;key=val    iTerm2 proprietary extensions

  osc_count
};

constexpr const char *OSC[] = {
  "\x1b]0;{}\x07",          // set_icon_and_title  ({} = text)
  "\x1b]1;{}\x07",          // set_icon_name
  "\x1b]2;{}\x07",          // set_title
  "\x1b]3;{}={}\x07",       // set_xprop           (property, value)
  "\x1b]4;{};{}\x07",       // set_color           (index, colorspec)
  "\x1b]4;{};?\x07",        // query_color         (index)
  "\x1b]10;{}\x07",         // change_fg
  "\x1b]10;?\x07",          // query_fg
  "\x1b]11;{}\x07",         // change_bg
  "\x1b]11;?\x07",          // query_bg
  "\x1b]12;{}\x07",         // change_cursor_color
  "\x1b]12;?\x07",          // query_cursor_color
  "\x1b]13;{}\x07",         // change_mouse_fg
  "\x1b]13;?\x07",          // query_mouse_fg
  "\x1b]14;{}\x07",         // change_mouse_bg
  "\x1b]14;?\x07",          // query_mouse_bg
  "\x1b]15;{}\x07",         // change_tek_fg
  "\x1b]15;?\x07",          // query_tek_fg
  "\x1b]16;{}\x07",         // change_tek_bg
  "\x1b]16;?\x07",          // query_tek_bg
  "\x1b]17;{}\x07",         // change_highlight_bg
  "\x1b]18;{}\x07",         // change_tek_cursor
  "\x1b]19;{}\x07",         // change_highlight_fg
  "\x1b]50;{}\x07",         // set_font
  "\x1b]52;c;{}\x07",       // clipboard_set       (base64 data)
  "\x1b]52;c;?\x07",        // clipboard_query
  "\x1b]104;{}\x07",        // reset_color         (index)
  "\x1b]110;\x07",          // reset_fg
  "\x1b]111;\x07",          // reset_bg
  "\x1b]112;\x07",          // reset_cursor_color
  "\x1b]113;\x07",          // reset_mouse_fg
  "\x1b]114;\x07",          // reset_mouse_bg
  "\x1b]8;{};{}\x07",       // hyperlink_set       (params e.g. "id=x", uri)
  "\x1b]8;;\x07",           // hyperlink_end
  "\x1b]9;{}\x07",          // notify
  "\x1b]133;A\x07",         // shell_prompt_start
  "\x1b]133;B\x07",         // shell_prompt_end
  "\x1b]133;C\x07",         // shell_cmd_start
  "\x1b]133;D;{}\x07",      // shell_cmd_end       (exit code)
  "\x1b]1337;{}\x07"        // iterm2              (e.g. "File=name=x:...")
};

enum charset_code : usize {
  g0_ascii,                // ESC ( B   US ASCII
  g0_line_drawing,         // ESC ( 0   VT100 Special Graphics (line-drawing)
  g0_alt_rom,              // ESC ( 1   Alternate Character ROM Standard
  g0_alt_rom_special,      // ESC ( 2   Alternate Character ROM Special Graphics
  g0_british,              // ESC ( A   UK (replaces # with £)
  g0_dutch,                // ESC ( 4   Dutch
  g0_finnish,              // ESC ( 5   Finnish
  g0_norwegian,            // ESC ( 6   Norwegian / Danish
  g0_swedish,              // ESC ( 7   Swedish
  g0_swiss,                // ESC ( =   Swiss
  g0_french_canadian,      // ESC ( Q   French Canadian
  g0_french,               // ESC ( R   French (Belgian/French)
  g0_german,               // ESC ( K   German
  g0_italian,              // ESC ( Y   Italian
  g0_spanish,              // ESC ( Z   Spanish

  g1_ascii,
  g1_line_drawing,
  g1_alt_rom,

  g2_ascii,
  g2_line_drawing,

  g3_ascii,
  g3_line_drawing,

  charset_count
};

constexpr const char *CHARSET[] = {
  "\x1b(B",      // g0_ascii
  "\x1b(0",      // g0_line_drawing
  "\x1b(1",      // g0_alt_rom
  "\x1b(2",      // g0_alt_rom_special
  "\x1b(A",      // g0_british
  "\x1b(4",      // g0_dutch
  "\x1b(5",      // g0_finnish
  "\x1b(6",      // g0_norwegian
  "\x1b(7",      // g0_swedish
  "\x1b(=",      // g0_swiss
  "\x1b(Q",      // g0_french_canadian
  "\x1b(R",      // g0_french
  "\x1b(K",      // g0_german
  "\x1b(Y",      // g0_italian
  "\x1b(Z",      // g0_spanish

  "\x1b)B",      // g1_ascii
  "\x1b)0",      // g1_line_drawing
  "\x1b)1",      // g1_alt_rom

  "\x1b*B",      // g2_ascii
  "\x1b*0",      // g2_line_drawing

  "\x1b+B",      // g3_ascii
  "\x1b+0"       // g3_line_drawing
};

enum dcs_code : usize {
  dcs_open,              // ESC P               raw opener (data follows)
  dcs_close,             // ESC \               String Terminator
  sixel,                 // ESC P q <data> ST   sixel graphics
  decrqss,               // ESC P $ q <Pt> ST   request selection/setting
  decudk,                // ESC P <f>;<l>|<key>/<str>;... ST  user-defined keys
  decdld,                // ESC P <Pfn>;<Pcn>;<Pe>;<Pcmw>;<Pss>;<Pt>;<Pcmh>;<Pcss>{<data> ST
  tmux_passthrough,      // ESC P tmux ; <esc-doubled data> ST
  dcs_count
};

constexpr const char *DCS[] = {
  "\x1bP",                                      // dcs_open
  "\x1b\\",                                     // dcs_close
  "\x1bPq{}\x1b\\",                             // sixel             ({} = sixel payload)
  "\x1bP$q{}\x1b\\",                            // decrqss           ({} = setting function string)
  "\x1bP{};{}|{}\x1b\\",                        // decudk            (clear, lock, key/str pairs)
  "\x1bP{};{};{};{};{};{};{};{}{{}\x1b\\",      // decdld (8 numeric params, data)
  "\x1bPtmux;{}\x1b\\"                          // tmux_passthrough  ({} = ESC-escaped sequence)
};

enum pm_code : usize {
  pm_open,       // ESC ^   begin PM string
  pm_close,      // ESC \   String Terminator
  pm_count
};

constexpr const char *PM[] = {
  "\x1b^",      // pm_open
  "\x1b\\"      // pm_close  (same ST as DCS/OSC)
};

enum apc_code : usize {
  apc_open,            // ESC _           raw opener
  apc_close,           // ESC \           String Terminator
  kitty_graphics,      // ESC _ G<keys>;<base64> ST:  kitty graphics protocol
  apc_count
};

constexpr const char *APC[] = {
  "\x1b_",                 // apc_open
  "\x1b\\",                // apc_close
  "\x1b_G{};{}\x1b\\"      // kitty_graphics  (key=val pairs, base64 payload)
};

// mouse modes

enum mouse_code : usize {
  mouse_x10_on,          // ?9h    X10 mouse — button-press only (legacy)
  mouse_x10_off,         // ?9l
  mouse_normal_on,       // ?1000h VT200 — press + release, no motion
  mouse_normal_off,      // ?1000l
  mouse_button_on,       // ?1002h button-event tracking (motion when button held)
  mouse_button_off,      // ?1002l
  mouse_any_on,          // ?1003h any-event tracking (all motion)
  mouse_any_off,         // ?1003l

  mouse_focus_on,       // ?1004h focus-in / focus-out events
  mouse_focus_off,      // ?1004l

  mouse_utf8_on,             // ?1005h UTF-8 extended coords (deprecated — avoid)
  mouse_utf8_off,            // ?1005l
  mouse_sgr_on,              // ?1006h SGR extended coordinates (recommended)
  mouse_sgr_off,             // ?1006l
  mouse_urxvt_on,            // ?1015h URXVT decimal extended coordinates
  mouse_urxvt_off,           // ?1015l
  mouse_sgr_pixels_on,       // ?1016h SGR pixel-coordinate mode (xterm)
  mouse_sgr_pixels_off,      // ?1016l

  mouse_count
};

constexpr const char *MOUSE[] = {
  "\x1b[?9h",         // mouse_x10_on
  "\x1b[?9l",         // mouse_x10_off
  "\x1b[?1000h",      // mouse_normal_on
  "\x1b[?1000l",      // mouse_normal_off
  "\x1b[?1002h",      // mouse_button_on
  "\x1b[?1002l",      // mouse_button_off
  "\x1b[?1003h",      // mouse_any_on
  "\x1b[?1003l",      // mouse_any_off
  "\x1b[?1004h",      // mouse_focus_on
  "\x1b[?1004l",      // mouse_focus_off
  "\x1b[?1005h",      // mouse_utf8_on
  "\x1b[?1005l",      // mouse_utf8_off
  "\x1b[?1006h",      // mouse_sgr_on
  "\x1b[?1006l",      // mouse_sgr_off
  "\x1b[?1015h",      // mouse_urxvt_on
  "\x1b[?1015l",      // mouse_urxvt_off
  "\x1b[?1016h",      // mouse_sgr_pixels_on
  "\x1b[?1016l"       // mouse_sgr_pixels_off
};

enum misc_code : usize {
  show_cursor,      // ?25h   DECTCEM  text cursor enable
  hide_cursor,      // ?25l

  cursor_keys_app,         // ?1h    DECCKM   application cursor-key sequences
  cursor_keys_normal,      // ?1l             normal (ANSI) cursor-key sequences

  col_132,                // ?3h    DECCOLM  switch to 132-column mode
  col_80,                 // ?3l             switch back to 80-column mode
  smooth_scroll_on,       // ?4h    DECSCLM  smooth (slow) scroll
  smooth_scroll_off,      // ?4l
  reverse_video_on,       // ?5h    DECSCNM  whole-screen reverse video
  reverse_video_off,      // ?5l
  origin_mode_on,         // ?6h    DECOM    cursor relative to scroll region
  origin_mode_off,        // ?6l
  auto_wrap_on,           // ?7h    DECAWM   auto wrap at right margin
  auto_wrap_off,          // ?7l
  auto_repeat_on,         // ?8h    DECARM   auto-repeat key
  auto_repeat_off,        // ?8l

  appl_keypad_on,       // ?66h   DECNKM   application numeric keypad
  appl_keypad_off,      // ?66l
  backarrow_bs,         // ?67h   DECBKM   backarrow key sends BS
  backarrow_del,        // ?67l            backarrow key sends DEL

  local_echo_off,      // ?12h   SRM      disable local echo (send-receive on)
  local_echo_on,       // ?12l            enable local echo

  print_ff_on,               // ?18h   DECPFF   send form-feed after print job
  print_ff_off,              // ?18l
  print_fullscreen_on,       // ?19h   DECPEX   print full screen (vs. scroll region)
  print_fullscreen_off,      // ?19l

  alt_screen_47_on,         // ?47h   switch to alternate screen (legacy, no save)
  alt_screen_47_off,        // ?47l
  alt_screen_1047_on,       // ?1047h switch to alternate screen + clear
  alt_screen_1047_off,      // ?1047l
  alt_screen_on,            // ?1049h save cursor + clear + switch to alt screen
  alt_screen_off,           // ?1049l restore cursor + switch to normal screen

  bracketed_paste_on,       // ?2004h bracketed paste mode
  bracketed_paste_off,      // ?2004l

  no_clear_col_change_on,       // ?95h   DECNCSM  do not clear on DECCOLM change
  no_clear_col_change_off,      // ?95l

  sync_update_on,       // ?2026h begin synchronized screen update
  sync_update_off,      // ?2026l end   synchronized screen update

  grapheme_cluster_on,       // ?2027h treat grapheme clusters as single cells
  grapheme_cluster_off,      // ?2027l

  in_band_resize_on,       // ?2048h send in-band resize events (XTWINOPS 48)
  in_band_resize_off,      // ?2048l

  misc_count
};

constexpr const char *MISC[] = {
  "\x1b[?25h",      // show_cursor
  "\x1b[?25l",      // hide_cursor

  "\x1b[?1h",      // cursor_keys_app
  "\x1b[?1l",      // cursor_keys_normal

  "\x1b[?3h",      // col_132
  "\x1b[?3l",      // col_80
  "\x1b[?4h",      // smooth_scroll_on
  "\x1b[?4l",      // smooth_scroll_off
  "\x1b[?5h",      // reverse_video_on
  "\x1b[?5l",      // reverse_video_off
  "\x1b[?6h",      // origin_mode_on
  "\x1b[?6l",      // origin_mode_off
  "\x1b[?7h",      // auto_wrap_on
  "\x1b[?7l",      // auto_wrap_off
  "\x1b[?8h",      // auto_repeat_on
  "\x1b[?8l",      // auto_repeat_off

  "\x1b[?66h",      // appl_keypad_on
  "\x1b[?66l",      // appl_keypad_off
  "\x1b[?67h",      // backarrow_bs
  "\x1b[?67l",      // backarrow_del

  "\x1b[?12h",      // local_echo_off
  "\x1b[?12l",      // local_echo_on

  "\x1b[?18h",      // print_ff_on
  "\x1b[?18l",      // print_ff_off
  "\x1b[?19h",      // print_fullscreen_on
  "\x1b[?19l",      // print_fullscreen_off

  "\x1b[?47h",        // alt_screen_47_on
  "\x1b[?47l",        // alt_screen_47_off
  "\x1b[?1047h",      // alt_screen_1047_on
  "\x1b[?1047l",      // alt_screen_1047_off
  "\x1b[?1049h",      // alt_screen_on
  "\x1b[?1049l",      // alt_screen_off

  "\x1b[?2004h",      // bracketed_paste_on
  "\x1b[?2004l",      // bracketed_paste_off

  "\x1b[?95h",      // no_clear_col_change_on
  "\x1b[?95l",      // no_clear_col_change_off

  "\x1b[?2026h",      // sync_update_on
  "\x1b[?2026l",      // sync_update_off

  "\x1b[?2027h",      // grapheme_cluster_on
  "\x1b[?2027l",      // grapheme_cluster_off

  "\x1b[?2048h",      // in_band_resize_on
  "\x1b[?2048l"       // in_band_resize_off
};

};      // namespace ansi
};      // namespace micron
