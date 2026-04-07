#pragma once
// dfgdf
#include "io/stdio.hpp"

#include "linux/std.hpp"
#include "std.hpp"

// TODO: finish

// hardcoded for now
static constexpr const char *__include_search_paths[] = { "./src" };
static constexpr usize __include_search_paths_count = 1;

inline mc::string
__dir_of(const mc::string &path)
{
  auto itr = mc::fmt::find_reverse(path, path.last(), '/');
  if ( itr == nullptr )
    return mc::string("./");
  return path.substr(path.begin(), itr + 1);
}

inline mc::string
__resolve_include(const mc::string &raw_token, const mc::string &dir_hint, bool quoted)
{
  // strip surrounding quotes or angle brackets that may still be present
  mc::string token(raw_token);
  mc::fmt::strip_chars(token, "\"<> \t");

  if ( quoted ) {
    mc::string candidate = dir_hint + token;
    mc::fmt::replace_all(candidate, "//", "/");
    if ( mc::posix::exists(candidate) )
      return candidate;
  }

  for ( usize i = 0; i < __include_search_paths_count; ++i ) {
    mc::string candidate(__include_search_paths[i]);
    candidate += "/";
    candidate += token;
    mc::fmt::replace_all(candidate, "//", "/");
    if ( mc::posix::exists(candidate) )
      return candidate;
  }

  return mc::string();
}

template <mc::is_string S>
bool
verify_compileability(mc::posix::time_t last_time, const S &path)
{
  if ( !mc::posix::exists(path) ) {
    mc::console("verify_compileability(): file doesn't exist");
    return false;
  }

  mc::string file;
  auto __f = mc::io::open_file(path);
  mc::io::read(__f, file);
  mc::posix::time_t mtime = __f.mtime();
  const mc::posix::time_t root_time = last_time ? last_time : mtime;
  
  if ( mtime > root_time )
    return true;

  mc::string dir = __dir_of(mc::string(path));
  auto lines = mc::fmt::splitlines(file);
  for ( auto &line : lines ) {
    mc::fmt::strip(line);     // trim leading/trailing whitespace in-place

    if ( mc::fmt::starts_with(line, "#include \"") ) {
      auto open = mc::fmt::find(line, line.begin(), '"');
      if ( open == nullptr )
        continue;
      auto close = mc::fmt::find(line, open + 1, '"');
      if ( close == nullptr )
        continue;

      mc::string token = line.substr(open + 1, close);

      mc::string resolved = __resolve_include(token, dir, true);
      if ( resolved.empty() )
        continue;     // effectively system header, skip
                      // NOTE: system headers could technically update, but treat them as static for now

      if ( verify_compileability(root_time, resolved) )
        return true;
    } else if ( mc::fmt::starts_with(line, "#include <") ) {
      auto open = mc::fmt::find(line, line.begin(), '<');
      if ( open == nullptr )
        continue;
      auto close = mc::fmt::find(line, open + 1, '>');
      if ( close == nullptr )
        continue;

      mc::string token = line.substr(open + 1, close);

      mc::string resolved = __resolve_include(token, dir, false);
      if ( resolved.empty() )
        continue;     // LITERAL system header

      if ( verify_compileability(root_time, resolved) )
        return true;
    }
  }

  return false;
}
