
#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"



int
main()
{
  mc::console("Total usage: ", abc::musage());
  mc::console("Usage of __class_precise: ", abc::musage<abc::__class_precise>());
  mc::console("Usage of __class_small: ", abc::musage<abc::__class_small>());
  mc::console("Usage of __class_medium: ", abc::musage<abc::__class_medium>());
  mc::console("Usage of __class_large: ", abc::musage<abc::__class_large>());
  mc::console("Usage of __class_huge: ", abc::musage<abc::__class_huge>());
  return 0;
}
