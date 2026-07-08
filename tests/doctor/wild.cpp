
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
  const char *what = (argc > 1) ? argv[1] : "guard";

  byte *keep[4];
  for ( int i = 0; i < 4; ++i ) keep[i] = abc::alloc(64u + 64u * static_cast<usize>(i));
  (void)keep;

  micron::posix::sigset_t blk{};
  micron::posix::sigemptyset(blk);
  micron::posix::sigaddset(blk, micron::posix::sig_usr1);
  micron::posix::sigaddset(blk, micron::posix::sig_usr2);
  micron::posix::sigprocmask(micron::posix::sig_block, blk, nullptr);
  const long pid = static_cast<long>(micron::syscall(SYS_getpid));
  const long tid = static_cast<long>(micron::syscall(SYS_gettid));
  micron::syscall(SYS_tgkill, pid, tid, micron::posix::sig_usr2);

  if ( eq(what, "null") ) {
    volatile int *p = reinterpret_cast<volatile int *>(0x10);
    return *p;
  }
  if ( eq(what, "noncanon") ) {
    volatile int *p = reinterpret_cast<volatile int *>(0xdead000000000000ULL);
    return *p;
  }

  byte *m = reinterpret_cast<byte *>(micron::syscall(SYS_mmap, nullptr, 8192, 3 /*R|W*/, 0x22 /*PRIVATE|ANON*/, -1, 0));
  for ( int i = 0; i < 4096; ++i ) m[i] = 0xa5;
  micron::syscall(SYS_mprotect, m + 4096, 4096, 0 /*PROT_NONE*/);
  volatile byte *g = m + 4096;
  *g = 1;
  return static_cast<int>(*g);
}
