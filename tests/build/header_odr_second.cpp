//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "header_odr_surface.hpp"

void *
header_odr_arena()
{
#ifdef MICRON_ODR_NEGATIVE_CONTROL
  return nullptr;
#else
  return abc::__current_arena();
#endif
}

void *
header_odr_stream()
{
  return micron::io::__global_buffer_stdout;
}

i32 *
header_odr_errno()
{
  return __micron_errno_location();
}

void
header_odr_release(byte *p)
{
  abc::dealloc(p);
}
