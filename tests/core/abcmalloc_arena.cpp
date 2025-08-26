#include "../../src/allocation/abcmalloc/arena.hpp"
#include "../../src/allocation/abcmalloc/book.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.h"

int
main()
{
  if constexpr ( true ) {
    mc::console(sizeof(abc::sheet<abc::__class_arena_internal, abc::__default_arena_page_buf>));
    mc::console(sizeof(abc::sheet<abc::__class_small>));
    mc::console(sizeof(abc::sheet<abc::__class_medium>));
    mc::console(sizeof(abc::sheet<abc::__class_large>));
    mc::console(sizeof(abc::sheet<abc::__class_huge>));
    abc::__arena arena{};
  }

  return 0;
}
