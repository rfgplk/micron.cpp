#pragma once

#include "../algorithm/mem.hpp"
#include "../slice.hpp"
#include "../string/strings.h"
#include "../types.hpp"
#include "io.hpp"
#include "print.hpp"

namespace micron
{
namespace io
{

template <char C = '\n'>
inline void
__read_stdin(micron::string &bufout)
{
  micron::slice<char> buf(4096);
  size_t needle = 0;
  io::fget_byte(&buf[needle], stdin);
  while ( buf[needle] != C )     // stop char to look for, if any
  {
    if ( needle >= 4095 ) {
      bufout += buf;
      buf.reset();
      needle = 0;
    }
    io::fget_byte(&buf[++needle], stdin);
  }
  bufout.append(buf, needle);
}

//redirect from stdin to some other arbitrary FILE*
void
copy_to()
{
}


// functions intended for reading from the terminal and retrieving the data
template <bool Echo = false>
void
terminal(micron::string& bf)
{
  __read_stdin<'\n'>(bf);
  if constexpr(Echo)
    println(bf);
}

template <bool Echo = false>
micron::string
terminal(void)
{
  micron::string bf;
  __read_stdin<'\n'>(bf);
  if constexpr(Echo)
    println(bf);
  return bf;
}

};
};
