// filesystem.cpp
// Tour of micron's I/O / filesystem layers (src/io/, src/io/file.hpp,
// src/io/fsys.hpp, src/io/filesystem.hpp).
//
// The layout from low to high level:
//
//   src/io/posix/        — thin POSIX-call wrappers (open/read/write/...).
//   src/io/io.hpp        — safe read/write helpers around fd_t / i32 fds.
//   src/io/file.hpp      — fsys::file<T>, an OWNING file handle bound to
//                          a string buffer T (default micron::string).
//   src/io/fsys.hpp      — free functions: exists, rename, copy, move,
//                          make, file_type_at.
//   src/io/filesystem.hpp — fsys::system<modes, N>, an in-memory cache
//                          of opened files (advanced).
//   src/io/paths.hpp     — path_t (sstring<path_max>) + path class with
//                          basename / extension / stem helpers.
//
// This example writes to a temp file under /tmp, reads it back, then
// cleans up. It uses the high-level fsys::file<> for the read/write and
// fsys:: free functions for the metadata operations.
//
// STL deltas:
//   - file<T> is generic over the buffer string type (string, ustr8, ...).
//   - The mode is io::modes — one enum, no flag math.
//   - Path manipulation is sstring-backed, no std::filesystem::path.
//   - operator<< / operator>> on file<T> push and pull a whole string.

#include "../src/io/console.hpp"
#include "../src/io/file.hpp"
#include "../src/io/fsys.hpp"
#include "../src/io/paths.hpp"
#include "../src/string/strings.hpp"

int
main()
{
  // ================================================================
  // 1. Path manipulation — io::path_t
  // ----------------------------------------------------------------
  // path_t = sstring<path_max>, a stack-allocated path buffer. The
  // higher-level io::path class (with basename / extension / stem
  // member methods) exists too, but its constructor calls resolve()
  // which requires the path to exist on disk — so we use path_t
  // here for purely-string manipulation.
  //
  // io::path also exposes a static prune() that squashes "//" and
  // trailing slashes; src/io/format.hpp provides free function
  // versions of basename/extension/stem.
  // ================================================================
  micron::io::println("-- 1. paths --");

  micron::io::path_t messy("/tmp//foo/bar/");
  auto cleaned = micron::io::path::prune(messy);
  micron::io::println("prune('/tmp//foo/bar/') = '", cleaned, "'");

  // ================================================================
  // 2. fsys functions — quick checks without opening
  // ================================================================
  micron::io::println("-- 2. fsys metadata --");

  micron::io::path_t target("/tmp/micron_fs_example.txt");

  // exists / readable_at / writeable_at / executable_at all wrap access(2)
  micron::io::println("exists before = ", micron::fsys::exists(target));

  // ================================================================
  // 3. fsys::file<T> — write, then read back
  // ----------------------------------------------------------------
  // open with io::modes::readwritecreate to create-or-truncate. The
  // operator= overload assigns a string and immediately flushes it.
  // ================================================================
  micron::io::println("-- 3. write / read --");

  {
    micron::fsys::file<micron::string> f(target.c_str(), micron::io::modes::readwritecreate);
    micron::string payload("hello from micron!\nsecond line\n");
    f = payload;     // operator=(const T&) writes the buffer to the fd
  }     // f closes here

  micron::io::println("exists after write = ", micron::fsys::exists(target));

  // Read the file back: open in read mode, load(), pull() the buffer.
  {
    micron::fsys::file<micron::string> f(target.c_str(), micron::io::modes::read);
    f.load();
    micron::io::println("read-back size = ", f.count());
    micron::io::println("contents:");
    micron::io::print(f.get());
  }

  // ================================================================
  // 4. fsys::copy / move / rename
  // ----------------------------------------------------------------
  // Convenience wrappers around posix::rename and a buffered
  // copy. fsys::move is an alias for fsys::rename.
  // ================================================================
  micron::io::println("-- 4. copy / rename --");

  micron::io::path_t copy_target("/tmp/micron_fs_example_copy.txt");

  micron::fsys::copy(target, copy_target);
  micron::io::println("copy exists = ", micron::fsys::exists(copy_target));

  micron::io::path_t renamed("/tmp/micron_fs_example_renamed.txt");
  micron::fsys::rename(copy_target, renamed);
  micron::io::println("renamed exists  = ", micron::fsys::exists(renamed));
  micron::io::println("old name exists = ", micron::fsys::exists(copy_target));

  // ================================================================
  // 5. Cleanup
  // ----------------------------------------------------------------
  // micron does not yet ship an "unlink" wrapper at fsys::, so we
  // just leave temp files in /tmp for the test runner. Real code
  // would call posix::unlink directly.
  // ================================================================
  micron::io::println("-- 5. done; check /tmp/micron_fs_example*.txt --");

  // ================================================================
  // 6. Beyond this example
  // ----------------------------------------------------------------
  //   src/io/filesystem.hpp  — fsys::system<> in-memory file cache
  //   src/io/ftw.hpp         — file-tree walk / dir traversal
  //   src/io/concurrent_filesystem.hpp — locked variants
  //   src/io/serial.hpp      — text/binary serialisation helpers
  //   src/io/stream.hpp      — buffered stream wrapper
  //   src/io/term/           — TTY raw / cooked mode helpers
  // ================================================================
  return 0;
}
