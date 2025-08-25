#include "../src/type_traits.hpp"
#include "../src/std.h"
#include "../src/io/console.hpp"

int main() {
  mc::console("is_null_pointer_v [int]: ", mc::is_null_pointer_v<int>);
  mc::console("is_null_pointer_v [nullptr]: ", mc::is_null_pointer_v<nullptr_t>);
  return 0;
}
