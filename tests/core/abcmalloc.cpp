#include "../../src/allocation/abcmalloc/arena.hpp"
#include "../../src/allocation/abcmalloc/book.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include <random>

int
main()
{
  mc::infolog(sizeof(abc::sheet<abc::__class_small>));
  mc::infolog(sizeof(abc::sheet<abc::__class_medium>));
  mc::infolog(sizeof(abc::sheet<abc::__class_large>));
  mc::infolog(sizeof(abc::sheet<abc::__class_huge>));
  if constexpr ( false ) {
    auto sheet = abc::make_sheet<abc::__class_small>(2 << 15);
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(394);
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  // should fire exactly 32 times
  if constexpr ( false ) {
    auto sheet = abc::make_sheet<abc::__class_small>(2 << 12);
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(256);
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  if constexpr ( false ) {
    auto sheet = abc::make_sheet<abc::__class_small>(abc::__default_arena_page_buf * abc::__system_pagesize);
    for ( size_t n = 0; n < 64; n++ ) {
      sheet.try_mark(1024);
    }
  }
  if constexpr ( false ) {
    auto sheet = abc::make_sheet<abc::__class_small>(abc::__default_arena_page_buf * abc::__system_pagesize);
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(sizeof(abc::sheet<abc::__class_small>));
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  // should fire exactly 128 times
  if constexpr ( false ) {
    auto sheet = abc::make_sheet<abc::__class_medium>(2 << 12);
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(1024);
      mc::console(n++, " ", mem.ptr, " of size ", mem.len);
      if ( mem.zero() ) {
        break;
      }
    }
  }
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    auto sheet = abc::make_sheet<abc::__class_small>(2 << 24);
    for ( size_t n = 0;; ) {
      auto mem = sheet.try_mark(dist(gen));
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
