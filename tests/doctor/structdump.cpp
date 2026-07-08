
#include "../../src/std.hpp"

static bool
eq(const char *a, const char *b)
{
  while ( *a && *b ) {
    if ( *a != *b ) return false;
    ++a;
    ++b;
  }
  return *a == *b;
}

int
main(int argc, char **argv)
{
  if ( argc > 1 && eq(argv[1], "hw") ) {
    byte *q = abc::alloc(8192);
    micron::syscall(SYS_mprotect, q + 4096, 4096, 0 /*PROT_NONE*/);
    volatile byte *g = q + 4096;
    return static_cast<int>(*g);
  }

  byte *keep[3];
  for ( int i = 0; i < 3; ++i ) keep[i] = abc::alloc(48u + 16u * static_cast<usize>(i));
  (void)keep;

  {
    byte *p = abc::alloc(88);
    byte *h = p - 32;
    byte saved[4];
    for ( int i = 0; i < 4; ++i ) saved[i] = h[i];
    h[0] = static_cast<byte>(0x44);
    h[1] = static_cast<byte>(0x33);
    h[2] = static_cast<byte>(0x22);
    h[3] = static_cast<byte>(0x11);
    abc::doctor::on_bad_free(p, 0, "structdump rig: tlsf bsize clobbered", __FILE__, __LINE__);
    for ( int i = 0; i < 4; ++i ) h[i] = saved[i];
    abc::dealloc(p);
  }

  {
    byte *q = abc::alloc(8192);
    byte *h = q + abc::query_size(q);
    byte saved[4];
    for ( int i = 0; i < 4; ++i ) saved[i] = h[i];
    for ( int i = 0; i < 4; ++i ) h[i] = static_cast<byte>(0xEE);
    abc::doctor::on_bad_free(q, 0, "structdump rig: buddy order clobbered", __FILE__, __LINE__);
    for ( int i = 0; i < 4; ++i ) h[i] = saved[i];
    abc::dealloc(q);
  }
  return 0;
}
