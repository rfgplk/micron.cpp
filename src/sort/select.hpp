#pragma once
#include "../algorithm/mem.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{

template <typename T>
void
selection(T &arr)
{
  i64 n = arr.size();
  for ( i64 i = 0; i < n - 1; i++ ) {
    i64 m = i;
    for ( i64 j = m + 1; j < n; j++ ) {
      if ( arr[j] < arr[m] )
        m = j;
    }
    if ( m != i )
      swap(arr[i], arr[m]);
  }
}
};     // namespace sort

};     // namespace micron
