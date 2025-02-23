#pragma once

#include "../../except.hpp"
#include "../../string/strings.h"
#include "../../tuple.hpp"
#include "../../vector/vector.hpp"
#include "utils.hpp"

#include <cstdio>
#include <dirent.h>     // for dir funcs
#include <linux/limits.h>
#include <sys/types.h>     // for dir funcs
#include <type_traits>

// functions for handling posix/linux dirs
namespace micron
{
namespace io
{

constexpr static const int max_name = (NAME_MAX + 1) * 2;

struct dir {

  typedef micron::vector<micron::pair<struct dirent, struct stat>> dir_container;
  micron::sstr<max_name> dname;
  DIR *dp;
  dir_container dd;
  ~dir()
  {
    if ( dp == NULL )
      return;
    closedir(dp);
  }
  dir(void) = default;
  dir(const char *str)
  {
    if ( !verify(str) )
      throw except::filesystem_error("error in creating micron::dir, malformed string.");
    if ( !exists(str) )
      throw except::filesystem_error("micron::dir dir doesn't exist");
    if ( !is_dir(str) )
      throw except::filesystem_error("micron::dir dir isn't a dir (check type)");
    dp = ::opendir(str);
    if ( dp == NULL )
      throw except::filesystem_error("micron::dir failed to open");
    list();     // init base structure
    dname = str;
  }
  dir(const micron::sstr<max_name> &str)
  {
    if ( !verify(str) )
      throw except::filesystem_error("error in creating micron::dir, malformed string.");
    if ( !exists(str.c_str()) )
      throw except::filesystem_error("micron::dir dir doesn't exist");
    if ( !is_dir(str.c_str()) )
      throw except::filesystem_error("micron::dir dir isn't a dir (check type)");
    dp = ::opendir(str.c_str());
    if ( dp == NULL )
      throw except::filesystem_error("micron::dir failed to open");
    list();     // init base structure
    dname = str;
  }
  dir(const micron::string &str)
  {
    if ( !verify(str) )
      throw except::filesystem_error("error in creating micron::dir, malformed string.");
    if ( !exists(str.c_str()) )
      throw except::filesystem_error("micron::dir dir doesn't exist");
    if ( !is_dir(str.c_str()) )
      throw except::filesystem_error("micron::dir dir isn't a dir (check type)");
    dp = ::opendir(str.c_str());
    if ( dp == NULL )
      throw except::filesystem_error("micron::dir failed to open");
    list();     // init base structure
    dname = str;
  }
  dir(const dir &o) : dname(o.dname), dp(o.dp), dd() {}
  dir(dir &&o) : dname(micron::move(o.dname)), dp(o.dp), dd(micron::move(o.dd)) { o.dp = nullptr; }
  dir &
  operator=(const dir &o)
  {
    dname = o.dname;
    dp = o.dp;
    dd = dir_container();
    return *this;
  }
  dir &
  operator=(dir &&o)
  {
    dname = micron::move(o.dname);
    dp = o.dp;
    o.dp = nullptr;
    dd = micron::move(o.dd);
    return *this;
  }
  inline auto
  get() const
  {
    return dp;
  }
  inline auto &
  get_children()
  {
    if ( !dd )
      list();
    return dd;
  }
  inline auto
  name() const
  {
    return dname;
  }
  template <typename F>
  inline dir &
  until(const char *str, F (dir::*f)())
  {
    while ( dname != str ) {
      (this->*f)();
    }
    return *this;
  }
  template <typename F, typename... Args>
  inline dir &
  until(const char *str, F (dir::*f)(Args...), Args &&...args)
  {
    while ( dname != str ) {
      (this->*f)(micron::forward(args)...);
      if ( dname == "/" )
        break;
    }
    return *this;
  }
  inline dir &
  parent(void)
  {
    if ( dd.empty() )
      list();
    if ( dp )
      closedir(dp);
    micron::sstr<(max_name) * 2> pstr(dname);
    pstr += "/..";

    dp = ::opendir(pstr.c_str());
    if ( dp == NULL )
      throw except::filesystem_error("micron::dir failed to open");
    list();
    dname = ::realpath(pstr.c_str(), NULL);
    return *this;
  }
  dir &
  up(void)
  {
    return parent();
  }     // shorthand
  dir &
  to(const char *str)
  {
    if ( dp )
      closedir(dp);
    dp = ::opendir(str);
    if ( dp == NULL )
      throw except::filesystem_error("micron::dir failed to open");
    list();
    dname = ::realpath(str, NULL);
    return *this;
  }
  int
  fd(void) const
  {
    if ( dp )
      return dirfd(dp);
    return -1;
  }
  // NOTE: takes in a str which _must_ correlate to a child
  dir &
  change(const char *str)
  {
    // change dir. NOTE: this doesn't actually change the working path of the bin, only modifies *dp
    for ( auto &n : dd ) {
      if ( !micron::strcmp(n.a.d_name, str) ) {
        closedir(dp);
        micron::sstr<(max_name) * 2> pstr(dname);
        pstr += "/";
        pstr += n.a.d_name;
        dp = ::opendir(str);
        if ( dp == NULL )
          throw except::filesystem_error("micron::dir failed to open");
        list();     // init base structure
        dname = str;
        return *this;
      }
    }
    return *this;
  }
  void
  list(void)
  {
    // NOTE: there's ways to speed this up, and it really isn't all too efficient, but you generally don't want to call
    // this repeatedly either; furthermore, we have to do it the C way (wrapping readdir)
    alive();
    if ( dd.size() )
      dd.clear();
    struct dirent *e;
    while ( (e = readdir(dp)) != NULL ) {     // TODO: implement readdir from 0
      struct stat sd;
      micron::sstr<(max_name) * 2> pstr(dname);
      pstr += "/";
      pstr += e->d_name;
      if ( stat(pstr.c_str(), &sd) != 0 )
        throw except::filesystem_error("micron::dir failed to stat dir");
      dd.emplace_back(
          micron::tie(micron::move(*e),
                      micron::move(sd)));     // try to avoid senseless copying, replace with preallocated eventually
    }
  }

private:
  inline bool
  alive(void) const
  {
    if ( dp == NULL ) {
      throw except::filesystem_error("micron::linux_dir, dp isn't open.");
      return false;
    }
    return true;
  }
};
};
};
