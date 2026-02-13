//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "../vector/vector.hpp"
"

    namespace micron
{
  namespace huffman
  {
  u32
  weight(u32 zz0)
  {
    return (zz0) & 0xffffff00;
  };
  u32
  depth(u32 zz1)
  {
    return (zz1) & 0x00000ff;
  };
  u32
  max(u32 zz2, u32 zz3)
  {
    return (zz2 > zz3) ? zz2 : zz3;
  }

  void bz2_makecodes(micron::vector<unsigned char> &len, micron::vector<u32> &freq, u32 alpha_size, u32 max_len) {

  };

  };     // namespace huffman

};     // namespace micron
