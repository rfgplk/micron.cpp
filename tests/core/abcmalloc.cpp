#include "../../src/allocation/abcmalloc/arena.hpp"
#include "../../src/allocation/abcmalloc/book.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.h"

#include <random>

int
main()
{
  // should fire exactly 64 times
  if constexpr ( false ) {
    abc::sheet<abc::__class_medium> sheet;
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(512);
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  // should fire exactly 512 times
  if constexpr ( true ) {
    abc::sheet<abc::__class_medium> sheet;
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(1024);
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  // should fire exactly 512 times
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(900, 1200);
    abc::sheet<abc::__class_small> sheet;
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(1024);
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  if constexpr ( false ) {
  }
  return 0;
}
