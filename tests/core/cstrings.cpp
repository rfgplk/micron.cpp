#include "../../src/memory/memory.hpp"
#include "../../src/string/unistring.hpp"
#include "../../src/io/console.hpp"

#include "../../src/string/istring.hpp"
#include "../../src/std.h"
int
main(void) {
  const unicode8* a8 = u8"fdgjfidgiodfg";
  const unicode16* a16 = u"fdgjfidgiodfg";
  const unicode32* a32 = U"fdgjfidgiodfg";
  micron::console(micron::u8_check(a8, micron::strlen(a8)) == nullptr);
  micron::console(micron::u16_check(a16, micron::u16strlen(a16)) == nullptr);
  micron::console(micron::u32_check(a32, micron::ustrlen(a32)) == nullptr);
  const char* a = "fdgjfidgiodfg";
  const char* b = "fdgjfidgiodfg";
  const char* c = "dfg";
  micron::console(strcmp(a, b));
  micron::console(strcmp(b, a));
  micron::console(strcmp(b, c));
  
  mc::istring ic = "Hello!";
  auto id = ic.append("hi");
  auto ie = id += "bye";
  mc::console(ic.find('H'));
  mc::console(ic);
  mc::console(id);
  mc::console(ie);
}
