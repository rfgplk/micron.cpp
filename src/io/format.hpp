//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../string/format.hpp"
#include "../string/strings.hpp"
#include "io.hpp"
#include "paths.hpp"
#include "posix/file.hpp"
#include "realpath.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// functions for formatting paths (and other types)

namespace micron
{
namespace io
{

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pruning

inline path_t
prune(path_t &&str)
{
  if ( micron::format::find(str, "//") ) micron::format::replace_all(str, "//", "/");

  if ( micron::format::find(str, "...") ) micron::format::replace_all(str, "...", "..");

  while ( str.size() > 1 && str[str.size() - 1] == '/' ) str.erase(str.last());

  return str;
}

inline path_t
prune(const path_t &str)
{
  path_t copy(str);
  return prune(micron::move(copy));
}

inline path_t
prune(const char *str)
{
  return prune(path_t(str));
}

template <is_string T>
inline path_t
prune(const T &str)
{
  return prune(path_t(str.c_str()));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trims

inline path_t
trim(const char *str)
{
  if ( !str || str[0] == '\0' ) return path_t();

  usize start = 0;
  while ( str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n' ) ++start;

  usize end = start;
  while ( str[end] ) ++end;

  while ( end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\r' || str[end - 1] == '\n') ) --end;

  if ( end <= start ) return path_t();

  path_t out;
  for ( usize i = start; i < end; ++i ) out += str[i];

  return prune(micron::move(out));
}

inline path_t
trim(const path_t &str)
{
  if ( str.empty() ) return path_t();

  const char first = str[0];
  const char last = str[str.size() - 1];
  if ( first != ' ' && first != '\t' && first != '\r' && first != '\n' && last != ' ' && last != '\t' && last != '\r' && last != '\n' )
    return prune(str);
  return trim(str.c_str());
}

template <is_string T>
inline path_t
trim(const T &str)
{
  return trim(str.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// collapses

inline path_t
collapse(const char *str)
{
  if ( !str || str[0] == '\0' ) return path_t();

  path_t in = prune(path_t(str));
  bool abs = (in.size() > 0 && in[0] == '/');

  // Fixed-depth component stack.  PATH_MAX / 2 is a safe upper bound on the

  constexpr usize max_depth = posix::path_max / 2;
  path_t stack[max_depth];
  usize depth = 0;

  const char *s = in.c_str();
  usize n = in.size();
  usize start = abs ? 1u : 0u;

  for ( usize i = start; i <= n; ++i ) {
    if ( s[i] == '/' || s[i] == '\0' ) {
      usize len = i - start;

      if ( len == 0 || (len == 1 && s[start] == '.') ) {
        start = i + 1;
        continue;
      }

      if ( len == 2 && s[start] == '.' && s[start + 1] == '.' ) {
        if ( depth > 0 ) --depth;
        start = i + 1;
        continue;
      }
      if ( depth < max_depth ) {
        path_t comp;
        for ( usize j = start; j < i; ++j ) comp += s[j];
        stack[depth++] = micron::move(comp);
      }
      start = i + 1;
    }
  }

  path_t out;
  if ( abs ) out += '/';
  for ( usize i = 0; i < depth; ++i ) {
    if ( i ) out += '/';
    out += stack[i];
  }
  if ( out.empty() ) out += (abs ? '/' : '.');
  return out;
}

inline path_t
collapse(const path_t &str)
{
  if ( !micron::format::find(str, ".") ) return prune(str);
  return collapse(str.c_str());
}

template <is_string T>
inline path_t
collapse(const T &str)
{
  return collapse(str.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// valids

inline bool
valid(const char *s) noexcept
{
  return posix::verify(s);
}

inline bool
valid(const path_t &s) noexcept
{
  return posix::verify(s.c_str());
}

template <is_string T>
inline bool
valid(const T &s) noexcept
{
  return posix::verify(s.c_str());
}

inline path_t
validate(const char *str)
{
  if ( !posix::verify(str) ) exc<except::filesystem_error>("io::validate — path string is not valid.");
  return prune(path_t(str));
}

inline path_t
validate(const path_t &str)
{
  if ( !posix::verify(str.c_str()) ) exc<except::filesystem_error>("io::validate — path string is not valid.");
  return prune(str);
}

template <is_string T>
inline path_t
validate(const T &str)
{
  return validate(str.c_str());
}

inline bool
has_nul(const char *str, usize len) noexcept
{
  for ( usize i = 0; i < len; ++i )
    if ( str[i] == '\0' ) return true;
  return false;
}

inline bool
has_nul(const path_t &s) noexcept
{
  return has_nul(s.c_str(), s.size());
}

template <is_string T>
inline bool
has_nul(const T &s) noexcept
{
  return has_nul(s.c_str(), s.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// well formeds

inline bool
well_formed(const char *str) noexcept
{
  if ( !posix::verify(str) ) return false;
  usize total = 0;
  usize comp = 0;
  for ( ; *str; ++str, ++total ) {
    if ( total >= static_cast<usize>(posix::path_max) ) return false;
    if ( *str == '/' ) {
      comp = 0;
    } else if ( ++comp > static_cast<usize>(posix::name_max) )
      return false;
  }
  return true;
}

inline bool
well_formed(const path_t &s) noexcept
{
  return well_formed(s.c_str());
}

template <is_string T>
inline bool
well_formed(const T &s) noexcept
{
  return well_formed(s.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exists

inline bool
exists(const char *s) noexcept
{
  return posix::exists(s);
}

inline bool
exists(const path_t &s) noexcept
{
  return posix::exists(s.c_str());
}

template <is_string T>
inline bool
exists(const T &s) noexcept
{
  return posix::exists(s.c_str());
}

inline bool
lexists(const char *s) noexcept
{
  return posix::lexists(s);
}

inline bool
lexists(const path_t &s) noexcept
{
  return posix::lexists(s.c_str());
}

template <is_string T>
inline bool
lexists(const T &s) noexcept
{
  return posix::lexists(s.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// accesses

inline bool
accessible(const char *s) noexcept
{
  return posix::access(s, posix::read_ok | posix::execute_ok) == 0;
}

inline bool
accessible(const path_t &s) noexcept
{
  return posix::access(s.c_str(), posix::read_ok | posix::execute_ok) == 0;
}

template <is_string T>
inline bool
accessible(const T &s) noexcept
{
  return posix::access(s.c_str(), posix::read_ok | posix::execute_ok) == 0;
}

inline bool
readable(const char *s) noexcept
{
  return posix::is_readable(s);
}

inline bool
readable(const path_t &s) noexcept
{
  return posix::is_readable(s.c_str());
}

template <is_string T>
inline bool
readable(const T &s) noexcept
{
  return posix::is_readable(s.c_str());
}

inline bool
writable(const char *s) noexcept
{
  return posix::is_writable(s);
}

inline bool
writable(const path_t &s) noexcept
{
  return posix::is_writable(s.c_str());
}

template <is_string T>
inline bool
writable(const T &s) noexcept
{
  return posix::is_writable(s.c_str());
}

inline bool
executable(const char *s) noexcept
{
  return posix::is_executable(s);
}

inline bool
executable(const path_t &s) noexcept
{
  return posix::is_executable(s.c_str());
}

template <is_string T>
inline bool
executable(const T &s) noexcept
{
  return posix::is_executable(s.c_str());
}

inline bool
access(const char *s, int m) noexcept
{
  return posix::access(s, m) == 0;
}

inline bool
access(const path_t &s, int m) noexcept
{
  return posix::access(s.c_str(), m) == 0;
}

template <is_string T>
inline bool
access(const T &s, int m) noexcept
{
  return posix::access(s.c_str(), m) == 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ownerships

inline bool
owned_by_me(const char *s) noexcept
{
  return posix::is_owned_by_me(s);
}

inline bool
owned_by_me(const path_t &s) noexcept
{
  return posix::is_owned_by_me(s.c_str());
}

template <is_string T>
inline bool
owned_by_me(const T &s) noexcept
{
  return posix::is_owned_by_me(s.c_str());
}

inline bool
owned_by(const char *s, posix::uid_t u) noexcept
{
  return posix::is_owned_by(s, u);
}

inline bool
owned_by(const path_t &s, posix::uid_t u) noexcept
{
  return posix::is_owned_by(s.c_str(), u);
}

template <is_string T>
inline bool
owned_by(const T &s, posix::uid_t u) noexcept
{
  return posix::is_owned_by(s.c_str(), u);
}

inline bool
in_group(const char *s, posix::gid_t g) noexcept
{
  return posix::is_in_group(s, g);
}

inline bool
in_group(const path_t &s, posix::gid_t g) noexcept
{
  return posix::is_in_group(s.c_str(), g);
}

template <is_string T>
inline bool
in_group(const T &s, posix::gid_t g) noexcept
{
  return posix::is_in_group(s.c_str(), g);
}

inline linux_permissions
permissions(const char *s)
{
  return posix::get_permissions(s);
}

inline linux_permissions
permissions(const path_t &s)
{
  return posix::get_permissions(s.c_str());
}

template <is_string T>
inline linux_permissions
permissions(const T &s)
{
  return posix::get_permissions(s.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// nodes

#define __MICRON_PATH_PRED(name_, posix_fn_)                                                                                               \
  inline bool name_(const char *s) noexcept { return posix_fn_(s); }                                                                       \
  inline bool name_(const path_t &s) noexcept { return posix_fn_(s.c_str()); }                                                             \
  template <is_string T> inline bool name_(const T &s) noexcept { return posix_fn_(s.c_str()); }

__MICRON_PATH_PRED(is_file, posix::is_file)
__MICRON_PATH_PRED(is_dir, posix::is_dir)
__MICRON_PATH_PRED(is_symlink, posix::is_symlink)
__MICRON_PATH_PRED(is_socket, posix::is_socket)
__MICRON_PATH_PRED(is_fifo, posix::is_fifo)
__MICRON_PATH_PRED(is_block_device, posix::is_block_device)
__MICRON_PATH_PRED(is_char_device, posix::is_char_device)
__MICRON_PATH_PRED(is_virtual, posix::is_virtual_file)

#undef __MICRON_PATH_PRED

inline bool
is_pipe(const char *s) noexcept
{
  return posix::is_fifo(s);
}

inline bool
is_pipe(const path_t &s) noexcept
{
  return posix::is_fifo(s.c_str());
}

template <is_string T>
inline bool
is_pipe(const T &s) noexcept
{
  return posix::is_fifo(s.c_str());
}

inline bool
is_device(const char *s) noexcept
{
  return posix::is_block_device(s) || posix::is_char_device(s);
}

inline bool
is_device(const path_t &s) noexcept
{
  return posix::is_block_device(s.c_str()) || posix::is_char_device(s.c_str());
}

template <is_string T>
inline bool
is_device(const T &s) noexcept
{
  return posix::is_block_device(s.c_str()) || posix::is_char_device(s.c_str());
}

inline bool
is_absolute(const char *s) noexcept
{
  return posix::is_absolute(s);
}

inline bool
is_absolute(const path_t &s) noexcept
{
  return posix::is_absolute(s);
}

template <is_string T>
inline bool
is_absolute(const T &s) noexcept
{
  return s.size() > 0 && s[0] == '/';
}

inline bool
is_relative(const char *s) noexcept
{
  return posix::is_relative(s);
}

inline bool
is_relative(const path_t &s) noexcept
{
  return posix::is_relative(s);
}

template <is_string T>
inline bool
is_relative(const T &s) noexcept
{
  return !is_absolute(s);
}

inline bool
is_root(const char *s) noexcept
{
  return s[0] == '/' && s[1] == '\0';
}

inline bool
is_root(const path_t &s) noexcept
{
  return s.size() == 1 && s[0] == '/';
}

template <is_string T>
inline bool
is_root(const T &s) noexcept
{
  return s.size() == 1 && s[0] == '/';
}

inline bool
is_mountpoint(const char *s) noexcept
{
  return posix::is_mountpoint(s);
}

inline bool
is_mountpoint(const path_t &s) noexcept
{
  return posix::is_mountpoint(s.c_str());
}

template <is_string T>
inline bool
is_mountpoint(const T &s) noexcept
{
  return posix::is_mountpoint(s.c_str());
}

inline bool
is_hidden(const char *str) noexcept
{
  if ( !str || !*str ) return false;
  const char *last = str;
  for ( const char *p = str; *p; ++p )
    if ( *p == '/' && *(p + 1) ) last = p + 1;
  if ( last[0] != '.' ) return false;
  if ( last[1] == '\0' || (last[1] == '.' && last[2] == '\0') ) return false;
  return true;
}

inline bool
is_hidden(const path_t &str) noexcept
{
  if ( str.empty() ) return false;
  if ( !micron::format::find(str, "/.") && str[0] != '.' ) return false;
  return is_hidden(str.c_str());
}

template <is_string T>
inline bool
is_hidden(const T &str) noexcept
{
  return is_hidden(str.c_str());
}

inline bool
same_file(const char *a, const char *b) noexcept
{
  return posix::is_same_file(a, b);
}

inline bool
same_file(const path_t &a, const path_t &b) noexcept
{
  return posix::is_same_file(a.c_str(), b.c_str());
}

inline bool
same_file(const path_t &a, const char *b) noexcept
{
  return posix::is_same_file(a.c_str(), b);
}

inline bool
same_file(const char *a, const path_t &b) noexcept
{
  return posix::is_same_file(a, b.c_str());
}

template <is_string A, is_string B>
inline bool
same_file(const A &a, const B &b) noexcept
{
  return posix::is_same_file(a.c_str(), b.c_str());
}

inline path_t
basename(const char *str) noexcept
{
  if ( !str || !*str ) return path_t();

  usize n = 0;
  while ( str[n] ) ++n;
  while ( n > 1 && str[n - 1] == '/' ) --n;

  usize sep = n;
  while ( sep > 0 && str[sep - 1] != '/' ) --sep;

  path_t out;
  for ( usize i = sep; i < n; ++i ) out += str[i];
  return out.empty() ? path_t("/") : out;
}

inline path_t
basename(const path_t &s) noexcept
{
  return basename(s.c_str());
}

template <is_string T>
inline path_t
basename(const T &s) noexcept
{
  return basename(s.c_str());
}

inline path_t
dirname(const char *str) noexcept
{
  if ( !str || !*str ) return path_t(".");

  usize n = 0;
  while ( str[n] ) ++n;
  while ( n > 1 && str[n - 1] == '/' ) --n;

  usize sep = n;
  while ( sep > 0 && str[sep - 1] != '/' ) --sep;

  if ( sep == 0 ) return path_t(".");
  if ( sep == 1 ) return path_t("/");

  path_t out;
  for ( usize i = 0; i < sep - 1; ++i ) out += str[i];
  return out;
}

inline path_t
dirname(const path_t &s) noexcept
{
  return dirname(s.c_str());
}

template <is_string T>
inline path_t
dirname(const T &s) noexcept
{
  return dirname(s.c_str());
}

inline path_t
extension(const char *str) noexcept
{
  path_t base = basename(str);
  const char *s = base.c_str();
  usize l = base.size();

  if ( l == 0 || s[0] == '.' ) return path_t();
  if ( !micron::format::find(base, ".") ) return path_t();

  usize dot = l;
  while ( dot > 0 && s[dot - 1] != '.' ) --dot;
  if ( dot == 0 ) return path_t();

  path_t out;
  for ( usize i = dot - 1; i < l; ++i ) out += s[i];
  return out;
}

inline path_t
extension(const path_t &s) noexcept
{
  return extension(s.c_str());
}

template <is_string T>
inline path_t
extension(const T &s) noexcept
{
  return extension(s.c_str());
}

inline path_t
stem(const char *str) noexcept
{
  path_t base = basename(str);
  path_t ext = extension(str);
  if ( ext.empty() ) return base;

  path_t out;
  usize keep = base.size() - ext.size();
  for ( usize i = 0; i < keep; ++i ) out += base[i];
  return out;
}

inline path_t
stem(const path_t &s) noexcept
{
  return stem(s.c_str());
}

template <is_string T>
inline path_t
stem(const T &s) noexcept
{
  return stem(s.c_str());
}

inline path_t
join(const char *base, const char *rel)
{
  if ( !rel || rel[0] == '\0' ) return prune(path_t(base ? base : ""));
  if ( rel[0] == '/' ) return prune(path_t(rel));
  if ( !base || base[0] == '\0' ) return prune(path_t(rel));

  usize bl = 0;
  while ( base[bl] ) ++bl;

  if ( bl > 0 && base[bl - 1] == '/' ) {

    return prune(micron::format::concat<path_t>(base, rel));
  }

  path_t sep_rel;
  sep_rel += '/';
  sep_rel += rel;
  return prune(micron::format::concat<path_t>(base, sep_rel.c_str()));
}

inline path_t
join(const path_t &base, const path_t &rel)
{
  return join(base.c_str(), rel.c_str());
}

inline path_t
join(const path_t &base, const char *rel)
{
  return join(base.c_str(), rel);
}

inline path_t
join(const char *base, const path_t &rel)
{
  return join(base, rel.c_str());
}

template <is_string A, is_string B>
inline path_t
join(const A &base, const B &rel)
{
  return join(base.c_str(), rel.c_str());
}

inline path_t
operator/(const path_t &base, const char *rel)
{
  return join(base, rel);
}

inline path_t
operator/(const path_t &base, const path_t &rel)
{
  return join(base, rel);
}

template <is_string T>
inline path_t
operator/(const path_t &base, const T &rel)
{
  return join(base, rel.c_str());
}

inline path_t
canonical(const char *str)
{
  path_t out;
  if ( micron::realpath(str, &out[0]) == nullptr ) return path_t();
  out.adjust_size();
  return out;
}

inline path_t
canonical(const path_t &s)
{
  return canonical(s.c_str());
}

template <is_string T>
inline path_t
canonical(const T &s)
{
  return canonical(s.c_str());
}

inline path_t
relative_to(const char *path_str, const char *base_str)
{
  path_t ps(path_str), bs(base_str);

  if ( ps == bs ) return path_t(".");

  usize pl = ps.size();
  usize bl = bs.size();

  usize common = 0;
  usize i = 0;
  while ( i < pl && i < bl && path_str[i] == base_str[i] ) {
    if ( path_str[i] == '/' ) common = i + 1;
    ++i;
  }
  if ( i == bl && (path_str[i] == '/' || path_str[i] == '\0') ) common = i + (path_str[i] == '/' ? 1u : 0u);

  usize up = 0;
  for ( usize j = common; j < bl; ++j )
    if ( base_str[j] == '/' ) ++up;
  if ( common < bl ) ++up;

  path_t out;
  for ( usize j = 0; j < up; ++j ) {
    if ( !out.empty() ) out += '/';
    out += "..";
  }
  if ( common < pl ) {
    if ( !out.empty() ) out += '/';
    for ( usize j = common; j < pl; ++j ) out += path_str[j];
  }
  if ( out.empty() ) out += '.';
  return out;
}

inline path_t
relative_to(const path_t &p, const path_t &base)
{
  return relative_to(p.c_str(), base.c_str());
}

inline path_t
relative_to(const path_t &p, const char *base)
{
  return relative_to(p.c_str(), base);
}

inline path_t
relative_to(const char *p, const path_t &base)
{
  return relative_to(p, base.c_str());
}

template <is_string A, is_string B>
inline path_t
relative_to(const A &p, const B &base)
{
  return relative_to(p.c_str(), base.c_str());
}

inline path_t
with_extension(const char *str, const char *ext)
{
  path_t dir_ = dirname(str);
  path_t st = stem(str);

  path_t dot_ext;
  if ( ext && ext[0] != '\0' ) {
    if ( ext[0] != '.' ) dot_ext += '.';
    dot_ext += ext;
  }

  path_t new_base = micron::format::concat<path_t>(st.c_str(), dot_ext.c_str());

  if ( dir_ == "." ) return new_base;
  return join(dir_, new_base);
}

inline path_t
with_extension(const path_t &s, const path_t &ext)
{
  return with_extension(s.c_str(), ext.c_str());
}

inline path_t
with_extension(const path_t &s, const char *ext)
{
  return with_extension(s.c_str(), ext);
}

template <is_string A, is_string B>
inline path_t
with_extension(const A &s, const B &ext)
{
  return with_extension(s.c_str(), ext.c_str());
}

};     // namespace io
};     // namespace micron
