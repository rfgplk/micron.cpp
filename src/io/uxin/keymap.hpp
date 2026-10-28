//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../errno.hpp"
#include "../../string/sstring.hpp"

#include "../../vector.hpp"

#include "../../simd/strings.hpp"

#include "../../sort/sort.hpp"

#include "keymap_symbols.hpp"

namespace micron::uxin::keymap
{
namespace detail
{
enum class token_kind : u8 { end, word, string, key, number, punctuation };

struct token {
  token_kind kind{};
  raw_slice<const char> text{};
  u32 value{};
  char punctuation{};

  constexpr bool
  is(const char *s) const noexcept
  {
    return same(text, s);
  }
};

template<bool Accelerated> struct lexer {
  raw_slice<const char> bytes;
  usize at = 0;
  i32 error = 0;

  token
  next() noexcept
  {
    const auto word = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; };
    for ( ;; ) {
      while ( at < bytes.len && (bytes.ptr[at] == ' ' || bytes.ptr[at] == '\t' || bytes.ptr[at] == '\r' || bytes.ptr[at] == '\n') ) ++at;
      if ( at == bytes.len || (at + 1 == bytes.len && !bytes.ptr[at]) ) return {};
      if ( at + 1 < bytes.len && bytes.ptr[at] == '/' && bytes.ptr[at + 1] == '/' ) {
        at += 2;
        if constexpr ( Accelerated )
          at += micron::simd::find_first_elem<char>(bytes.ptr + at, bytes.len - at, '\n');
        else
          while ( at < bytes.len && bytes.ptr[at] != '\n' ) ++at;
      } else if ( at + 1 < bytes.len && bytes.ptr[at] == '/' && bytes.ptr[at + 1] == '*' ) {
        at += 2;
        while ( at + 1 < bytes.len && !(bytes.ptr[at] == '*' && bytes.ptr[at + 1] == '/') ) ++at;
        if ( at + 1 >= bytes.len ) {
          error = -micron::error::invalid_arg;
          return {};
        }
        at += 2;
      } else
        break;
    }
    const usize start = at;
    const char c = bytes.ptr[at++];
    if ( c == '"' || c == '<' ) {
      const char close = c == '<' ? '>' : '"';
      while ( at < bytes.len && bytes.ptr[at] != close ) {
        if ( !bytes.ptr[at] || bytes.ptr[at] == '\n' ) {
          error = -micron::error::invalid_arg;
          return {};
        }
        if ( bytes.ptr[at] == '\\' ) {
          ++at;
          if ( at >= bytes.len ) {
            error = -micron::error::invalid_arg;
            return {};
          }
        }
        ++at;
      }
      if ( at == bytes.len ) {
        error = -micron::error::invalid_arg;
        return {};
      }
      token out{ c == '<' ? token_kind::key : token_kind::string, { bytes.ptr + start + 1, at - start - 1 } };
      ++at;
      return out;
    }
    if ( c >= '0' && c <= '9' ) {
      u32 base = 10, value = c - '0';
      if ( c == '0' && at < bytes.len && (bytes.ptr[at] == 'x' || bytes.ptr[at] == 'X') ) {
        base = 16;
        value = 0;
        ++at;
      }
      const usize digits = at;
      while ( at < bytes.len ) {
        const char d = bytes.ptr[at];
        const u32 n = d >= '0' && d <= '9' ? d - '0' : d >= 'a' && d <= 'f' ? d - 'a' + 10 : d >= 'A' && d <= 'F' ? d - 'A' + 10 : 16;
        if ( n >= base ) break;
        if ( value > (~0u - n) / base ) {
          error = -micron::error::overflow;
          return {};
        }
        value = value * base + n;
        ++at;
      }
      if ( (base == 16 && digits == at) || (at < bytes.len && word(bytes.ptr[at])) ) {
        error = -micron::error::invalid_arg;
        return {};
      }
      return { token_kind::number, { bytes.ptr + start, at - start }, value };
    }
    if ( word(c) ) {
      while ( at < bytes.len && word(bytes.ptr[at]) ) ++at;
      return { token_kind::word, { bytes.ptr + start, at - start } };
    }
    if ( !c ) {
      error = -micron::error::invalid_arg;
      return {};
    }
    return { token_kind::punctuation, { bytes.ptr + start, 1 }, 0, c };
  }
};

struct type_entry {
  u32 mask = 0, preserve = 0;
  u8 level = 0;
};

struct key_type {
  sstring<64> name{};
  u32 modifiers = 0;
  vector<type_entry> entries{};
};

struct group {
  sstring<64> type{};
  vector<u32> symbols{};
};

constexpr bool
alphabetic_pair(u32 lower, u32 upper) noexcept
{
  const auto small = [](u32 cp) {
    return (cp >= 'a' && cp <= 'z') || (cp >= 0xe0 && cp <= 0xff && cp != 0xf7) || cp == 0xdf || cp == 0x131 || cp == 0x17f
           || (cp >= 0x100 && cp <= 0x12f && (cp & 1)) || (cp >= 0x132 && cp <= 0x137 && (cp & 1))
           || (cp >= 0x139 && cp <= 0x148 && !(cp & 1)) || (cp >= 0x14a && cp <= 0x177 && (cp & 1))
           || (cp >= 0x179 && cp <= 0x17e && !(cp & 1)) || (cp >= 0x3b1 && cp <= 0x3c9) || (cp >= 0x430 && cp <= 0x45f);
  };
  const auto capital = [](u32 cp) {
    return (cp >= 'A' && cp <= 'Z') || (cp >= 0xc0 && cp <= 0xde && cp != 0xd7) || cp == 0x130 || cp == 0x178
           || (cp >= 0x100 && cp <= 0x12f && !(cp & 1)) || (cp >= 0x132 && cp <= 0x137 && !(cp & 1))
           || (cp >= 0x139 && cp <= 0x148 && (cp & 1)) || (cp >= 0x14a && cp <= 0x177 && !(cp & 1))
           || (cp >= 0x179 && cp <= 0x17e && (cp & 1)) || (cp >= 0x391 && cp <= 0x3a9 && cp != 0x3a2) || (cp >= 0x400 && cp <= 0x42f);
  };
  return small(lower) && capital(upper);
}

inline const char *
inferred_type(const group &group) noexcept
{
  const usize count = group.symbols.size();
  if ( count < 2 || count > 4 ) return "ONE_LEVEL";
  const bool letters = alphabetic_pair(symbol_unicode(group.symbols[0]), symbol_unicode(group.symbols[1]));
  const bool keypad
      = (group.symbols[0] >= 0xff80 && group.symbols[0] <= 0xffbd) || (group.symbols[1] >= 0xff80 && group.symbols[1] <= 0xffbd);
  if ( count == 2 ) return letters ? "ALPHABETIC" : keypad ? "KEYPAD" : "TWO_LEVEL";
  if ( letters )
    return count == 4 && alphabetic_pair(symbol_unicode(group.symbols[2]), symbol_unicode(group.symbols[3])) ? "FOUR_LEVEL_ALPHABETIC"
                                                                                                             : "FOUR_LEVEL_SEMIALPHABETIC";
  return keypad ? "FOUR_LEVEL_KEYPAD" : "FOUR_LEVEL";
}

struct key {
  sstring<32> name{};
  u32 code = 0, virtuals = 0, real_mods = 0;
  group groups[4]{};
  u8 group_count = 0, policy = 0, redirect = 0;
  bool repeat = true, repeat_explicit = false;
};

struct alias {
  sstring<32> name{}, target{};
};

struct virtual_mod {
  sstring<64> name{};
  u32 bindings = 0;
};

struct interpret {
  u32 symbol = 0, virtuals = 0, mods = 0;
  u8 match = 0;
  bool any = false, first = false, repeat = true;
};

struct mod_entry {
  sstring<32> name{};
  u32 symbol = 0, mask = 0;
  bool by_name = false;
};

struct map {
  vector<key> keys{};
  vector<u32> code_index{};
  vector<key_type> types{};
  vector<virtual_mod> virtuals{};
  vector<alias> aliases{};
  vector<interpret> interpretations{};
  vector<mod_entry> modmap{};

  key *
  named(raw_slice<const char> name) noexcept
  {
    for ( usize depth = 0; depth <= aliases.size(); ++depth ) {
      for ( auto &entry : keys )
        if ( same(name, entry.name.c_str()) ) return &entry;
      bool found = false;
      for ( const auto &entry : aliases )
        if ( same(name, entry.name.c_str()) ) {
          name = { entry.target.data(), entry.target.size() };
          found = true;
          break;
        }
      if ( !found ) break;
    }
    return nullptr;
  }

  const key *
  code(u32 value) const noexcept
  {
    usize first = 0, last = code_index.size();
    while ( first < last ) {
      const usize mid = first + (last - first) / 2;
      const auto &entry = keys[code_index[mid]];
      if ( entry.code < value )
        first = mid + 1;
      else if ( entry.code > value )
        last = mid;
      else
        return &entry;
    }
    return nullptr;
  }

  u32
  expanded(u32 mask) const noexcept
  {
    for ( usize i = 0; i < virtuals.size(); ++i )
      if ( (mask & (1u << (8 + i))) || (virtuals[i].bindings && (mask & virtuals[i].bindings) == virtuals[i].bindings) )
        mask |= (1u << (8 + i)) | virtuals[i].bindings;
    return mask;
  }
};

template<bool Accelerated> struct parser {
  lexer<Accelerated> input;
  token current{};
  map &out;
  i32 error = 0;
  sstring<64> default_type[4]{};
  bool default_repeat = true;

  parser(raw_slice<const char> bytes, map &target) : input{ bytes }, out(target) { advance(); }

  void
  advance() noexcept
  {
    current = input.next();
    if ( input.error ) error = input.error;
  }

  bool
  take(char c) noexcept
  {
    if ( current.punctuation != c ) return false;
    advance();
    return true;
  }

  bool
  expect(char c) noexcept
  {
    if ( take(c) ) return true;
    error = -micron::error::invalid_arg;
    return false;
  }

  template<usize N>
  bool
  name(sstring<N> &to)
  {
    if ( current.text.len >= N || current.kind == token_kind::end ) {
      error = -micron::error::message_too_long;
      return false;
    }
    to.clear();
    if ( !to.try_append(current.text.ptr, current.text.len) ) {
      error = -micron::error::message_too_long;
      return false;
    }
    advance();
    return true;
  }

  void
  skip() noexcept
  {
    u32 depth = 0;
    while ( !error && current.kind != token_kind::end ) {
      const char c = current.punctuation;
      if ( !depth && c == '}' ) return;
      if ( !depth && c == ';' ) {
        advance();
        return;
      }
      if ( c == '{' || c == '[' || c == '(' ) {
        if ( ++depth > 32 ) {
          error = -micron::error::overflow;
          return;
        }
      } else if ( c == '}' || c == ']' || c == ')' ) {
        if ( !depth ) {
          error = -micron::error::invalid_arg;
          return;
        }
        --depth;
      }
      advance();
    }
    if ( !error ) error = -micron::error::invalid_arg;
  }

  u32
  modifier(raw_slice<const char> value)
  {
    constexpr const char *real[] = { "Shift", "Lock", "Control", "Mod1", "Mod2", "Mod3", "Mod4", "Mod5" };
    if ( same(value, "none") || same(value, "None") ) return 0;
    if ( same(value, "all") || same(value, "All") ) return ~0u;
    for ( u32 i = 0; i < 8; ++i )
      if ( same(value, real[i]) ) return 1u << i;
    for ( usize i = 0; i < out.virtuals.size(); ++i )
      if ( same(value, out.virtuals[i].name.c_str()) ) return 1u << (8 + i);
    error = -micron::error::invalid_arg;
    return 0;
  }

  u32
  mask()
  {
    u32 value = 0;
    do {
      if ( current.kind == token_kind::number )
        value |= current.value;
      else if ( current.kind == token_kind::word )
        value |= modifier(current.text);
      else {
        error = -micron::error::invalid_arg;
        break;
      }
      advance();
    } while ( take('+') );
    return value;
  }

  u32
  index(const char *prefix, u32 maximum) noexcept
  {
    if ( current.kind == token_kind::number ) {
      const u32 value = current.value;
      advance();
      if ( !value || value > maximum ) {
        error = -micron::error::overflow;
        return 0;
      }
      return value - 1;
    }
    const auto text = current.text;
    usize i = 0;
    while ( prefix[i] && i < text.len && prefix[i] == text.ptr[i] ) ++i;
    u32 value = 0;
    if ( prefix[i] || i == text.len ) {
      error = -micron::error::invalid_arg;
      return 0;
    }
    for ( ; i < text.len; ++i ) {
      if ( text.ptr[i] < '0' || text.ptr[i] > '9' || value > 100 ) {
        error = -micron::error::invalid_arg;
        return 0;
      }
      value = value * 10 + text.ptr[i] - '0';
    }
    if ( !value || value > maximum ) {
      error = -micron::error::overflow;
      return 0;
    }
    advance();
    return value - 1;
  }

  void
  virtual_modifiers()
  {
    advance();
    do {
      if ( current.kind != token_kind::word ) {
        error = -micron::error::invalid_arg;
        return;
      }
      usize slot = 0;
      for ( ; slot < out.virtuals.size(); ++slot )
        if ( same(current.text, out.virtuals[slot].name.c_str()) ) break;
      if ( slot == out.virtuals.size() ) {
        if ( slot >= 24 ) {
          error = -micron::error::overflow;
          return;
        }
        out.virtuals.emplace_back();
        name(out.virtuals.back().name);
      } else
        advance();
      if ( take('=') ) out.virtuals[slot].bindings |= mask();
    } while ( take(',') );
    expect(';');
  }

  u32
  symbol() noexcept
  {
    const u32 value = current.kind == token_kind::number ? current.value : symbol_from_name(current.text);
    advance();
    return value;
  }

  void
  keycodes()
  {
    while ( !error && current.kind != token_kind::end && !take('}') ) {
      if ( current.kind == token_kind::key ) {
        if ( out.keys.size() >= 8192 ) {
          error = -micron::error::overflow;
          return;
        }
        key entry;
        name(entry.name);
        expect('=');
        if ( current.kind != token_kind::number ) {
          error = -micron::error::invalid_arg;
          return;
        }
        entry.code = current.value;
        advance();
        expect(';');
        for ( const auto &old : out.keys )
          if ( old.code == entry.code || old.name == entry.name ) error = -micron::error::invalid_arg;
        out.keys.push_back(micron::move(entry));
      } else if ( current.is("alias") ) {
        if ( out.aliases.size() >= 8192 ) {
          error = -micron::error::overflow;
          return;
        }
        advance();
        alias entry;
        name(entry.name);
        expect('=');
        name(entry.target);
        expect(';');
        out.aliases.push_back(micron::move(entry));
      } else
        skip();
    }
  }

  void
  types()
  {
    while ( !error && current.kind != token_kind::end && !take('}') ) {
      if ( current.is("virtual_modifiers") ) {
        virtual_modifiers();
        continue;
      }
      if ( !current.is("type") ) {
        skip();
        continue;
      }
      advance();
      if ( out.types.size() >= 256 ) {
        error = -micron::error::overflow;
        return;
      }
      key_type entry;
      name(entry.name);
      expect('{');
      while ( !error && current.kind != token_kind::end && !take('}') ) {
        if ( current.is("modifiers") ) {
          advance();
          expect('=');
          entry.modifiers = mask();
          expect(';');
        } else if ( current.is("map") || current.is("preserve") ) {
          const bool preserve = current.is("preserve");
          advance();
          expect('[');
          const u32 bits = mask();
          expect(']');
          expect('=');
          usize at = 0;
          for ( ; at < entry.entries.size(); ++at )
            if ( entry.entries[at].mask == bits ) break;
          if ( at == entry.entries.size() ) {
            if ( at >= 256 ) {
              error = -micron::error::overflow;
              return;
            }
            entry.entries.push_back({ .mask = bits });
          }
          if ( preserve )
            entry.entries[at].preserve = mask();
          else
            entry.entries[at].level = static_cast<u8>(index("Level", 32));
          expect(';');
        } else
          skip();
      }
      expect(';');
      out.types.push_back(micron::move(entry));
    }
  }

  void
  compatibility()
  {
    bool default_repeat = true, default_first = false;
    while ( !error && current.kind != token_kind::end && !take('}') ) {
      if ( current.is("virtual_modifiers") ) {
        virtual_modifiers();
        continue;
      }
      if ( !current.is("interpret") ) {
        skip();
        continue;
      }
      advance();
      if ( take('.') ) {
        if ( current.is("repeat") || current.is("useModMapMods") ) {
          const bool repeat = current.is("repeat");
          advance();
          expect('=');
          if ( repeat )
            default_repeat = current.is("True") || current.is("true") || current.is("Yes");
          else
            default_first = current.is("level1") || current.is("Level1");
          advance();
          expect(';');
        } else
          skip();
        continue;
      }
      interpret entry;
      entry.repeat = default_repeat;
      entry.first = default_first;
      entry.any = current.is("Any");
      entry.symbol = symbol();
      if ( take('+') ) {
        if ( current.is("Any") ) {
          entry.match = 1;
          entry.mods = ~0u;
          advance();
        } else {
          entry.match = current.is("Exactly")       ? 2
                        : current.is("AllOf")       ? 3
                        : current.is("NoneOf")      ? 4
                        : current.is("AnyOfOrNone") ? 5
                        : current.is("AnyOf")       ? 1
                                                    : 0;
          if ( !entry.match ) {
            error = -micron::error::op_not_supported;
            return;
          }
          advance();
          expect('(');
          entry.mods = mask();
          expect(')');
        }
      }
      expect('{');
      while ( !error && current.kind != token_kind::end && !take('}') ) {
        if ( current.is("virtualModifier") ) {
          advance();
          expect('=');
          entry.virtuals = mask();
          expect(';');
        } else if ( current.is("useModMapMods") ) {
          advance();
          expect('=');
          entry.first = current.is("level1") || current.is("Level1");
          advance();
          expect(';');
        } else if ( current.is("repeat") ) {
          advance();
          expect('=');
          entry.repeat = current.is("True") || current.is("true") || current.is("Yes");
          advance();
          expect(';');
        } else
          skip();
      }
      expect(';');
      if ( out.interpretations.size() >= 4096 ) {
        error = -micron::error::overflow;
        return;
      }
      out.interpretations.push_back(entry);
    }
  }

  void
  symbols_for(key &entry)
  {
    expect('{');
    u32 implicit_group = 0;
    while ( !error && current.kind != token_kind::end && !take('}') ) {
      const bool bare = current.punctuation == '[';
      const bool syms = current.is("symbols");
      const bool type = current.is("type");
      if ( bare || syms || type ) {
        u32 group = bare ? implicit_group++ : 0;
        if ( !bare ) {
          advance();
          if ( take('[') ) {
            group = index("Group", 4);
            expect(']');
          }
          expect('=');
        }
        if ( group >= 4 ) {
          error = -micron::error::overflow;
          return;
        }
        if ( type )
          name(entry.groups[group].type);
        else {
          expect('[');
          auto &values = entry.groups[group].symbols;
          values.clear();
          while ( !error && current.kind != token_kind::end && !take(']') ) {
            if ( values.size() >= 32 ) {
              error = -micron::error::overflow;
              return;
            }
            values.push_back(symbol());
            if ( current.punctuation != ']' && !expect(',') ) return;
          }
          if ( entry.group_count <= group ) entry.group_count = static_cast<u8>(group + 1);
        }
      } else if ( current.is("virtualMods") || current.is("virtualModifiers") ) {
        advance();
        expect('=');
        entry.virtuals = mask();
      } else if ( current.is("repeat") ) {
        advance();
        expect('=');
        entry.repeat = current.is("True") || current.is("true") || current.is("Yes");
        entry.repeat_explicit = true;
        advance();
      } else if ( current.is("groupsWrap") || current.is("groupsClamp") || current.is("groupsRedirect") ) {
        entry.policy = current.is("groupsWrap") ? 0 : current.is("groupsClamp") ? 1 : 2;
        advance();
        expect('=');
        if ( entry.policy == 2 )
          entry.redirect = static_cast<u8>(index("Group", 4));
        else
          advance();
      } else {
        // Actions/geometry do not execute in a Wayland client; the compositor supplies modifier state.
        u32 depth = 0;
        while ( !error && current.kind != token_kind::end ) {
          const char c = current.punctuation;
          if ( !depth && (c == ',' || c == '}') ) break;
          if ( c == '[' || c == '(' || c == '{' ) {
            if ( ++depth > 32 ) {
              error = -micron::error::overflow;
              return;
            }
          } else if ( c == ']' || c == ')' || c == '}' ) {
            if ( !depth ) {
              error = -micron::error::invalid_arg;
              return;
            }
            --depth;
          }
          advance();
        }
      }
      if ( current.punctuation != '}' ) expect(',');
    }
    expect(';');
  }

  void
  symbols()
  {
    while ( !error && current.kind != token_kind::end && !take('}') ) {
      if ( current.is("virtual_modifiers") ) {
        virtual_modifiers();
        continue;
      }
      if ( current.is("modifier_map") ) {
        advance();
        const u32 bits = modifier(current.text);
        advance();
        expect('{');
        while ( !error && current.kind != token_kind::end && !take('}') ) {
          if ( out.modmap.size() >= 8192 ) {
            error = -micron::error::overflow;
            return;
          }
          mod_entry entry;
          entry.mask = bits;
          entry.by_name = current.kind == token_kind::key;
          if ( entry.by_name )
            name(entry.name);
          else
            entry.symbol = symbol();
          out.modmap.push_back(micron::move(entry));
          if ( current.punctuation != '}' ) expect(',');
        }
        expect(';');
        continue;
      }
      if ( current.is("key") ) {
        advance();
        if ( take('.') ) {
          if ( current.is("type") ) {
            advance();
            u32 group = 0;
            if ( take('[') ) {
              group = index("Group", 4);
              expect(']');
            }
            expect('=');
            name(default_type[group]);
            expect(';');
          } else if ( current.is("repeat") ) {
            advance();
            expect('=');
            default_repeat = current.is("True") || current.is("true");
            advance();
            expect(';');
          } else
            skip();
          continue;
        }
        auto *entry = out.named(current.text);
        if ( !entry ) {
          error = -micron::error::invalid_arg;
          return;
        }
        advance();
        entry->repeat = default_repeat;
        for ( u32 g = 0; g < 4; ++g ) entry->groups[g].type = default_type[g];
        symbols_for(*entry);
      } else if ( current.is("include") ) {
        error = -micron::error::op_not_supported;
        return;
      } else
        skip();
    }
  }

  i32
  parse()
  {
    if ( !current.is("xkb_keymap") ) return -micron::error::invalid_arg;
    advance();
    if ( current.kind == token_kind::string ) advance();
    expect('{');
    while ( !error && current.kind != token_kind::end && !take('}') ) {
      const bool codes = current.is("xkb_keycodes"), type = current.is("xkb_types"),
                 compat = current.is("xkb_compatibility") || current.is("xkb_compat"), syms = current.is("xkb_symbols");
      if ( !(codes || type || compat || syms || current.is("xkb_geometry")) ) return -micron::error::op_not_supported;
      advance();
      if ( current.kind == token_kind::string ) advance();
      expect('{');
      if ( codes )
        keycodes();
      else if ( type )
        types();
      else if ( compat )
        compatibility();
      else if ( syms )
        symbols();
      else {
        while ( !error && current.kind != token_kind::end && !take('}') ) skip();
      }
      expect(';');
    }
    expect(';');
    if ( current.kind != token_kind::end || out.keys.empty() ) return -micron::error::invalid_arg;
    if ( error ) return error;
    for ( const auto &binding : out.modmap ) {
      if ( binding.by_name ) {
        auto *entry = out.named({ binding.name.data(), binding.name.size() });
        if ( !entry ) return -micron::error::invalid_arg;
        entry->real_mods |= binding.mask;
      } else
        for ( auto &entry : out.keys )
          for ( u32 g = 0; g < entry.group_count; ++g )
            for ( const auto sym : entry.groups[g].symbols )
              if ( sym == binding.symbol ) entry.real_mods |= binding.mask;
    }
    for ( auto &entry : out.keys ) {
      for ( u32 g = 0; g < entry.group_count; ++g ) {
        auto &group = entry.groups[g];
        if ( group.type.empty() ) {
          const char *name = inferred_type(group);
          for ( const auto &type : out.types )
            if ( type.name == name ) {
              group.type = name;
              break;
            }
        }
      }
      u32 virtuals = entry.virtuals;
      for ( const auto &rule : out.interpretations ) {
        bool matches = false;
        for ( u32 g = 0; g < entry.group_count; ++g )
          for ( usize l = 0; l < entry.groups[g].symbols.size(); ++l ) {
            if ( rule.first && (g || l) ) continue;
            if ( !rule.any && entry.groups[g].symbols[l] != rule.symbol ) continue;
            const auto mods = entry.real_mods & rule.mods;
            if ( (rule.match == 1 && !mods) || (rule.match == 2 && entry.real_mods != rule.mods) || (rule.match == 3 && mods != rule.mods)
                 || (rule.match == 4 && mods) || (rule.match == 5 && entry.real_mods && !mods) )
              continue;
            matches = true;
          }
        if ( matches ) {
          virtuals |= rule.virtuals;
          if ( !entry.repeat_explicit ) entry.repeat = rule.repeat;
        }
      }
      for ( usize v = 0; v < out.virtuals.size(); ++v )
        if ( virtuals & (1u << (8 + v)) ) out.virtuals[v].bindings |= entry.real_mods;
      for ( u32 g = 0; g < entry.group_count; ++g )
        if ( !entry.groups[g].type.empty() ) {
          bool found = false;
          for ( const auto &type : out.types )
            if ( type.name == entry.groups[g].type ) found = true;
          if ( !found ) return -micron::error::invalid_arg;
        }
    }
    out.code_index.reserve(out.keys.size());
    for ( u32 i = 0; i < out.keys.size(); ++i ) out.code_index.push_back(i);
    micron::sort::sort(out.code_index, [&](u32 a, u32 b) { return out.keys[a].code < out.keys[b].code; });
    return 0;
  }
};
}      // namespace detail

struct translation {
  u32 symbol = 0, codepoint = 0, consumed = 0;
  bool repeat = false, supported = true;
};

template<bool Accelerated = true> class basic_state
{
  detail::map map_{};
  u32 modifiers_ = 0, group_ = 0;
  bool valid_ = false;

public:
  basic_state() = default;
  basic_state(const basic_state &) = delete;
  basic_state &operator=(const basic_state &) = delete;
  basic_state(basic_state &&) = default;
  basic_state &operator=(basic_state &&) = default;

  bool
  prepare() const noexcept
  {
    return true;
  }

  i32
  load_map(raw_slice<const char> bytes) noexcept
  {
    if ( bytes.len > 4 * 1024 * 1024 ) return -micron::error::message_too_long;
    try {
      detail::map candidate;
      detail::parser<Accelerated> parser{ bytes, candidate };
      const i32 error = parser.parse();
      if ( error ) return error;
      map_ = micron::move(candidate);
      modifiers_ = group_ = 0;
      valid_ = true;
      return 0;
    } catch ( ... ) {
      return -micron::error::out_of_memory;
    }
  }

  void
  reset() noexcept
  {
    map_ = {};
    modifiers_ = group_ = 0;
    valid_ = false;
  }

  bool
  valid() const noexcept
  {
    return valid_;
  }

  void
  update_key(u32, bool) noexcept
  {
  }

  void
  update_mods(u32 depressed, u32 latched, u32 locked, u32 group) noexcept
  {
    modifiers_ = depressed | latched | locked;
    group_ = group;
  }

  u32
  modifiers() const noexcept
  {
    return map_.expanded(modifiers_);
  }

  u32
  modifier_mask(const char *name) const noexcept
  {
    constexpr const char *real[] = { "Shift", "Lock", "Control", "Mod1", "Mod2", "Mod3", "Mod4", "Mod5" };
    const raw_slice<const char> value{ name, micron::strlen(name) };
    for ( u32 i = 0; i < 8; ++i )
      if ( same(value, real[i]) ) return 1u << i;
    for ( usize i = 0; i < map_.virtuals.size(); ++i )
      if ( same(value, map_.virtuals[i].name.c_str()) ) return (1u << (8 + i)) | map_.virtuals[i].bindings;
    return 0;
  }

  translation
  translate(u32 evdev_code) const noexcept
  {
    if ( !valid_ || evdev_code > ~0u - 8 ) return {};
    const auto *entry = map_.code(evdev_code + 8);
    if ( !entry || !entry->group_count ) return {};
    u32 selected = group_;
    if ( selected >= entry->group_count )
      selected = entry->policy == 1   ? entry->group_count - 1
                 : entry->policy == 2 ? (entry->redirect < entry->group_count ? entry->redirect : 0)
                                      : selected % entry->group_count;
    const auto &group = entry->groups[selected];
    u32 level = 0, consumed = 0;
    if ( !group.type.empty() ) {
      for ( const auto &type : map_.types )
        if ( type.name == group.type ) {
          const auto actual = modifiers() & type.modifiers;
          consumed = type.modifiers;
          for ( const auto &mapping : type.entries )
            if ( mapping.mask == actual ) {
              level = mapping.level;
              consumed &= ~mapping.preserve;
              break;
            }
          break;
        }
    } else if ( group.symbols.size() >= 2 ) {
      const auto lower = symbol_unicode(group.symbols[0]), upper = symbol_unicode(group.symbols[1]);
      const bool alphabetic = detail::alphabetic_pair(lower, upper);
      level = (modifiers_ & 1u) ? 1 : 0;
      consumed = alphabetic ? 3u : 1u;
      if ( alphabetic && (modifiers_ & 2u) ) level ^= 1;
    }
    for ( usize v = 0; v < map_.virtuals.size(); ++v )
      if ( consumed & (1u << (8 + v)) ) consumed |= map_.virtuals[v].bindings;
    if ( level >= group.symbols.size() ) return { .consumed = consumed, .repeat = entry->repeat };
    const u32 symbol = group.symbols[level];
    const u32 codepoint = symbol_unicode(symbol);
    const bool supported
        = symbol != unknown_symbol && (codepoint || !symbol || symbol == 0xffffff || (symbol >= 0xfe00 && symbol <= 0xffff));
    return { symbol == unknown_symbol ? 0u : symbol, codepoint, consumed, entry->repeat, supported };
  }

  u32
  translate_key(u32 code) const noexcept
  {
    return translate(code).symbol;
  }

  u32
  translate_utf32(u32 code) const noexcept
  {
    return translate(code).codepoint;
  }
};

using state = basic_state<true>;
using scalar_state = basic_state<false>;
};      // namespace micron::uxin::keymap
