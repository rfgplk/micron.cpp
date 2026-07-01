#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

int
main(void)
{

  byte *p = abc::alloc(24);
  if ( p )
    for ( int i = 24; i < 40; ++i ) p[i] = static_cast<byte>(0xEE);
  mc::console(">>> H5 fsck:");
  abc::doctor::fsck();

  byte *q = abc::alloc(200);
  if ( q ) {
    unsigned bad = 0x11223344u;
    __builtin_memcpy(q - 32, &bad, 4);
  }
  mc::console(">>> H4 fsck:");
  abc::doctor::fsck();
  return 0;
}
