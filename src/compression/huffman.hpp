#pragma once

#include "../types.hpp"
#include "../vector/vector.hpp"
"

    namespace micron
{
  namespace huffman
  {
  uint32_t
  weight(uint32_t zz0)
  {
    return (zz0) & 0xffffff00;
  };
  uint32_t
  depth(uint32_t zz1)
  {
    return (zz1) & 0x00000ff;
  };
  uint32_t
  max(uint32_t zz2, uint32_t zz3)
  {
    return (zz2 > zz3) ? zz2 : zz3;
  }

  void bz2_makecodes(micron::vector<unsigned char> &len, micron::vector<uint32_t> &freq, uint32_t alpha_size,
                     uint32_t max_len) {

  };

  };     // namespace huffman

};     // namespace micron
