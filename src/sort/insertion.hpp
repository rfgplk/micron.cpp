#pragma once

#include "../algorithm/mem.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template <typename T>
void
insertion(T &arr)
{
  i64 i = 0;
  i64 n = arr.size();
  while ( i < n ) {
    auto x = arr[i];
    i64 j = i;
    for ( ; j > 0 and arr[j - 1] > x; j-- )
      arr[j] = arr[j - 1];
    arr[j] = x;
    i++;
  }
}
};

};
