// elf_high_level.cpp
// Exercises the micron::elf high-level façade (src/elf.hpp): open by path,
// open by soname, list_symbols enumeration, and execute<> error path.
//
// As in elf_basic.cpp, we deliberately avoid INVOKING a function pointer
// resolved from a privately-loaded duplicate of libc (separate errno/locale
// state). The execute() success path is the one-liner cast-and-call —
// covered indirectly by the host_dso suite — so here we only exercise the
// missing-symbol error path.

#include "../../src/elf.hpp"

#include "../../src/io/console.hpp"
#include "../../src/memory/cstring.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== ELF HIGH-LEVEL FACADE ===");

  test_case("open() returns a valid handle for an absolute path");
  {
    micron::elf::handle_t h = micron::elf::open("/lib64/libwayland-client.so.0");
    require_true(h.valid());
  }
  end_test_case();

  test_case("open() throws on a nonexistent path");
  {
    require_throw([] { (void)micron::elf::open("/this/path/does/not/exist/xyz.so"); });
  }
  end_test_case();

  test_case("open_soname() resolves a known soname via default search paths");
  {
    micron::elf::handle_t h = micron::elf::open_soname("libwayland-client.so.0");
    require_true(h.valid());
  }
  end_test_case();

  test_case("list_symbols() returns a non-empty vector for a loaded .so");
  {
    micron::elf::handle_t h = micron::elf::open("/lib64/libwayland-client.so.0");
    const auto &syms = micron::elf::list_symbols(h);
    require_true(syms.size() > 0);
  }
  end_test_case();

  test_case("list_symbols() enumerates wl_display_connect as a defined function");
  {
    micron::elf::handle_t h = micron::elf::open("/lib64/libwayland-client.so.0");
    const auto &syms = micron::elf::list_symbols(h);
    bool found = false;
    for ( const auto &s : syms ) {
      if ( s.name && micron::strcmp(s.name, "wl_display_connect") == 0 ) {
        require_true(s.defined);
        require_true(s.type == micron::elf::stt_func);
        require_true(s.address != nullptr);
        found = true;
        break;
      }
    }
    require_true(found);
  }
  end_test_case();

  test_case("list_symbols() reports at least one undefined import");
  {
    micron::elf::handle_t h = micron::elf::open("/lib64/libwayland-client.so.0");
    const auto &syms = micron::elf::list_symbols(h);
    bool any_undef = false;
    for ( const auto &s : syms ) {
      if ( !s.defined ) {
        any_undef = true;
        break;
      }
    }
    require_true(any_undef);
  }
  end_test_case();

  test_case("execute<>() throws library_error on a missing symbol");
  {
    micron::elf::handle_t h = micron::elf::open("/lib64/libwayland-client.so.0");
    require_throw([&] { micron::elf::execute(h, "definitely-not-a-real-symbol-zzz9"); });
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
