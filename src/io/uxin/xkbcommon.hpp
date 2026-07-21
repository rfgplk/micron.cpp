//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../linux/dynamic.hpp"
#include "../../linux/io/sys.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/mmap_bits.hpp"
#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// libxkbcommon port
// loaded via dso

namespace micron
{
namespace uxin
{
namespace xkb
{

struct __xkb_context;
struct __xkb_keymap;
struct __xkb_state;
using context_t = __xkb_context;
using keymap_t = __xkb_keymap;
using state_t = __xkb_state;

using keysym_t = u32;
using keycode_t = u32;
using layout_index_t = u32;
using mod_index_t = u32;
using mod_mask_t = u32;
using level_index_t = u32;

inline constexpr u32 XKB_CONTEXT_NO_FLAGS = 0;
inline constexpr u32 XKB_KEYMAP_FORMAT_TEXT_V1 = 1;
inline constexpr u32 XKB_KEYMAP_COMPILE_NO_FLAGS = 0;
inline constexpr u32 XKB_KEY_UP = 0;
inline constexpr u32 XKB_KEY_DOWN = 1;
inline constexpr u32 XKB_STATE_MODS_DEPRESSED = (1u << 0);
inline constexpr u32 XKB_STATE_MODS_LATCHED = (1u << 1);
inline constexpr u32 XKB_STATE_MODS_LOCKED = (1u << 2);
inline constexpr u32 XKB_STATE_MODS_EFFECTIVE = (1u << 3);

inline constexpr keysym_t XKB_KEY_NoSymbol = 0;

using PFN_xkb_context_new = context_t *(*)(u32 flags);
using PFN_xkb_context_unref = void (*)(context_t *context);
using PFN_xkb_keymap_new_from_string = keymap_t *(*)(context_t * context, const char *str, u32 format, u32 flags);
using PFN_xkb_keymap_unref = void (*)(keymap_t *keymap);
using PFN_xkb_state_new = state_t *(*)(keymap_t * keymap);
using PFN_xkb_state_unref = void (*)(state_t *state);
using PFN_xkb_state_update_key = u32 (*)(state_t *state, keycode_t key, u32 direction);
using PFN_xkb_state_update_mask = u32 (*)(state_t *state, mod_mask_t depressed_mods, mod_mask_t latched_mods, mod_mask_t locked_mods,
                                          layout_index_t depressed_layout, layout_index_t latched_layout, layout_index_t locked_layout);
using PFN_xkb_state_key_get_one_sym = keysym_t (*)(state_t *state, keycode_t key);
using PFN_xkb_state_key_get_utf32 = u32 (*)(state_t *state, keycode_t key);
using PFN_xkb_state_key_get_utf8 = int (*)(state_t *state, keycode_t key, char *buffer, usize size);
using PFN_xkb_keysym_to_utf32 = u32 (*)(keysym_t keysym);
using PFN_xkb_keysym_get_name = int (*)(keysym_t keysym, char *buffer, usize size);

struct lib_t {
  host_dso host;
  PFN_xkb_context_new xkb_context_new = nullptr;
  PFN_xkb_context_unref xkb_context_unref = nullptr;
  PFN_xkb_keymap_new_from_string xkb_keymap_new_from_string = nullptr;
  PFN_xkb_keymap_unref xkb_keymap_unref = nullptr;
  PFN_xkb_state_new xkb_state_new = nullptr;
  PFN_xkb_state_unref xkb_state_unref = nullptr;
  PFN_xkb_state_update_key xkb_state_update_key = nullptr;
  PFN_xkb_state_update_mask xkb_state_update_mask = nullptr;
  PFN_xkb_state_key_get_one_sym xkb_state_key_get_one_sym = nullptr;
  PFN_xkb_state_key_get_utf32 xkb_state_key_get_utf32 = nullptr;
  PFN_xkb_state_key_get_utf8 xkb_state_key_get_utf8 = nullptr;
  PFN_xkb_keysym_to_utf32 xkb_keysym_to_utf32 = nullptr;
  PFN_xkb_keysym_get_name xkb_keysym_get_name = nullptr;

  lib_t() : host("libxkbcommon.so.0")
  {
    xkb_context_new = host.sym_as<PFN_xkb_context_new>("xkb_context_new");
    xkb_context_unref = host.sym_as<PFN_xkb_context_unref>("xkb_context_unref");
    xkb_keymap_new_from_string = host.sym_as<PFN_xkb_keymap_new_from_string>("xkb_keymap_new_from_string");
    xkb_keymap_unref = host.sym_as<PFN_xkb_keymap_unref>("xkb_keymap_unref");
    xkb_state_new = host.sym_as<PFN_xkb_state_new>("xkb_state_new");
    xkb_state_unref = host.sym_as<PFN_xkb_state_unref>("xkb_state_unref");
    xkb_state_update_key = host.sym_as<PFN_xkb_state_update_key>("xkb_state_update_key");
    xkb_state_update_mask = host.sym_as<PFN_xkb_state_update_mask>("xkb_state_update_mask");
    xkb_state_key_get_one_sym = host.sym_as<PFN_xkb_state_key_get_one_sym>("xkb_state_key_get_one_sym");
    xkb_state_key_get_utf32 = host.sym_as<PFN_xkb_state_key_get_utf32>("xkb_state_key_get_utf32");
    xkb_state_key_get_utf8 = host.sym_as<PFN_xkb_state_key_get_utf8>("xkb_state_key_get_utf8");
    xkb_keysym_to_utf32 = host.sym_as<PFN_xkb_keysym_to_utf32>("xkb_keysym_to_utf32");
    xkb_keysym_get_name = host.sym_as<PFN_xkb_keysym_get_name>("xkb_keysym_get_name");

    if ( !xkb_context_new || !xkb_keymap_new_from_string || !xkb_state_new || !xkb_state_update_key || !xkb_state_update_mask
         || !xkb_state_key_get_one_sym ) {
      exc<except::library_error>("xkbcommon: required entry points missing in libxkbcommon.so.0");
    }
  }
};

inline lib_t &
xkb_lib() noexcept(!micron::except::__use_exceptions)
{
  static lib_t lib;
  return lib;
}

class keymap_state
{
  context_t *__ctx = nullptr;
  keymap_t *__keymap = nullptr;
  state_t *__state = nullptr;

public:
  ~keymap_state() { reset(); }

  keymap_state() = default;
  keymap_state(const keymap_state &) = delete;
  keymap_state &operator=(const keymap_state &) = delete;

  // Parse a keymap from a memory-mapped wl_keyboard.keymap fd.
  void
  load(const char *keymap_string)
  {
    auto &xl = xkb_lib();
    reset();
    __ctx = xl.xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if ( !__ctx ) exc<except::library_error>("xkbcommon: xkb_context_new failed");
    __keymap = xl.xkb_keymap_new_from_string(__ctx, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if ( !__keymap ) {
      xl.xkb_context_unref(__ctx);
      __ctx = nullptr;
      exc<except::library_error>("xkbcommon: xkb_keymap_new_from_string failed — keymap may be malformed");
    }
    __state = xl.xkb_state_new(__keymap);
    if ( !__state ) {
      xl.xkb_keymap_unref(__keymap);
      __keymap = nullptr;
      xl.xkb_context_unref(__ctx);
      __ctx = nullptr;
      exc<except::library_error>("xkbcommon: xkb_state_new failed");
    }
  }

  // Apply the mask from a wl_keyboard.modifiers event.
  void
  update_mods(u32 depressed, u32 latched, u32 locked, u32 group) noexcept
  {
    if ( !__state ) return;
    xkb_lib().xkb_state_update_mask(__state, depressed, latched, locked, 0, 0, group);
  }

  // Apply a key press/release. evdev_code is the raw KEY_* from wl_keyboard;
  // xkbcommon expects keycode + 8 (X11 convention).
  void
  update_key(u32 evdev_code, bool pressed) noexcept
  {
    if ( !__state ) return;
    xkb_lib().xkb_state_update_key(__state, evdev_code + 8, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
  }

  keysym_t
  translate_key(u32 evdev_code) const noexcept
  {
    if ( !__state ) return XKB_KEY_NoSymbol;
    return xkb_lib().xkb_state_key_get_one_sym(__state, evdev_code + 8);
  }

  u32
  translate_utf32(u32 evdev_code) const noexcept
  {
    if ( !__state ) return 0;
    return xkb_lib().xkb_state_key_get_utf32(__state, evdev_code + 8);
  }

  bool
  valid() const noexcept
  {
    return __state != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  void
  reset() noexcept
  {
    if ( !__ctx && !__keymap && !__state ) return;
    auto &xl = xkb_lib();
    if ( __state ) xl.xkb_state_unref(__state);
    if ( __keymap ) xl.xkb_keymap_unref(__keymap);
    if ( __ctx ) xl.xkb_context_unref(__ctx);
    __state = nullptr;
    __keymap = nullptr;
    __ctx = nullptr;
  }
};

};      // namespace xkb
};      // namespace uxin
};      // namespace micron
