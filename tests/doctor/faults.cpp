

#include "../../src/std.hpp"

static byte g_static = 7;

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
  const char *what = (argc > 1) ? argv[1] : "";

  if ( eq(what, "double") ) {
    byte *p = abc::alloc(64);
    abc::dealloc(p);
    abc::dealloc(p);
  } else if ( eq(what, "wrongsize") ) {
    byte *p = abc::alloc(100);
    abc::dealloc(p, 4096);
  } else if ( eq(what, "foreign") ) {
    abc::dealloc(&g_static);
  } else if ( eq(what, "corrupt") ) {
    byte *p = abc::alloc(24);
    for ( int i = 24; i < 40; ++i ) p[i] = static_cast<byte>(0xEE);
    abc::doctor::fsck();
  }
  return 0;
}
