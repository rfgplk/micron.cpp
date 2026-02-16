//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../tuple.hpp"
#include "console.hpp"
#include "paths.hpp"
#include "posix/iosys.hpp"

namespace micron
{
namespace io
{
// walk the file tree entirely, from the given path
// directories before files
// NOTE: from cwd
auto
ftw(path &&p)
{
  micron::fvector<path_t> rslt;
  // look how much prettier this is compared to nftw
  micron::fvector<path_t> dirs = p.dirs();
  if ( dirs.empty() or dirs.size() == 2 )
    return rslt;
  // rslt.append(dirs);
  //  micron::string cur_path(4096);
  //  if ( posix::getcwd(cur_path.data(), 4096) == NULL )
  //    return;
  //  cur_path.adjust_size();
  path_t total_path;
  for ( auto &n : dirs ) {
    if ( n == "." or n == ".." )
      continue;
    total_path = p.get();
    total_path.adjust_size();     // TODO: fix up len's and remove this eventually
    total_path.insert(total_path.end(), '/');
    total_path.insert(total_path.end(), n);
    // total_path += n;
    rslt.push_back(total_path);
    // console(total_path);
    try {
      rslt.append(ftw(path(total_path.c_str())));
    } catch ( except::filesystem_error &e ) {
    }
  }
  return rslt;
}
auto
ftw_all(path &&p)
{
  micron::fvector<path_t> rslt;
  // look how much prettier this is compared to nftw
  micron::fvector<path_t> all = p.all();
  if ( all.empty() or all.size() == 2 )
    return rslt;
  // rslt.append(dirs);
  //  micron::string cur_path(4096);
  //  if ( posix::getcwd(cur_path.data(), 4096) == NULL )
  //    return;
  //  cur_path.adjust_size();
  path_t total_path;
  for ( auto &n : all ) {
    if ( n == "." or n == ".." )
      continue;
    total_path = p.get();
    total_path.adjust_size();
    total_path.insert(total_path.end(), '/');
    total_path.insert(total_path.end(), n);
    // total_path += n;
    rslt.push_back(total_path);
    // console(total_path);
    try {
      rslt.append(ftw_all(path(total_path.c_str())));
    } catch ( except::filesystem_error &e ) {
    }
  }
  return rslt;
}

auto
ftw_files(path &&p)
{
  micron::fvector<path_t> rslt;
  // look how much prettier this is compared to nftw
  micron::fvector<path_t> all = p.files();
  if ( all.empty() or all.size() == 2 )
    return rslt;
  // rslt.append(dirs);
  //  micron::string cur_path(4096);
  //  if ( posix::getcwd(cur_path.data(), 4096) == NULL )
  //    return;
  //  cur_path.adjust_size();
  path_t total_path;
  for ( auto &n : all ) {
    if ( n == "." or n == ".." )
      continue;
    total_path = p.get();
    total_path.adjust_size();
    total_path.insert(total_path.end(), '/');
    total_path.insert(total_path.end(), n);
    // total_path += n;
    rslt.push_back(total_path);
    // console(total_path);
    try {
      rslt.append(ftw_all(path(total_path.c_str())));
    } catch ( except::filesystem_error &e ) {
    }
  }
  return rslt;
}
};
};
