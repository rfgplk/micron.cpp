#include "../src/io/filesystem.hpp"
#include "../src/io/fsys.hpp"
#include "../src/io/paths.hpp"
#include "../src/io/serial.hpp"
#include "../src/std.hpp"

#include "../src/io/stream.hpp"

#include "../src/control.hpp"
#include "../src/errno.hpp"
#include "../src/io/console.hpp"

#include "snowball/snowball.hpp"

int
main()
{
  mc::fvector<int> vec = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  sb::require(vec.size(), 10);
  mc::ustr8 contents = "Contents of a file..";
  mc::io::stream<> strm;
  mc::io::fd_t fl
      = (i32)mc::posix::open("/tmp/test_0", mc::o_rdwr | mc::o_create, mc::S_IRUSR | mc::S_IWUSR | mc::S_IRGRP | mc::S_IROTH);
  mc::io::fd_t fl_2
      = (i32)mc::posix::open("/tmp/test_1", mc::o_rdwr | mc::o_create, mc::S_IRUSR | mc::S_IWUSR | mc::S_IRGRP | mc::S_IROTH);
  mc::io::fd_t fl_3
      = (i32)mc::posix::open("/tmp/test_2", mc::o_rdwr | mc::o_create, mc::S_IRUSR | mc::S_IWUSR | mc::S_IRGRP | mc::S_IROTH);
  sb::require(mc::access("/tmp/test_0", mc::f_ok), 0);
  sb::require(mc::access("/tmp/test_1", mc::f_ok), 0);
  sb::require(mc::access("/tmp/test_2", mc::f_ok), 0);

  strm << contents;
  strm >> fl;
  strm << contents;
  strm >> fl_2;
  strm << vec;
  strm >> fl_2;
  strm << vec;
  strm >> fl_3;
  mc::lseek(fl.fd, 0, mc::seek_set);
  strm << fl;
  const char *arr = "Contents of a file..";
  sb::require(mc::bytecmp(strm.data(), reinterpret_cast<const byte *>(arr), strm.size()), 0);

  return 0;
}
