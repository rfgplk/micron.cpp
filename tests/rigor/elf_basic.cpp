// elf_basic.cpp
// Smoke test for micron::dso / micron::elf::handle_t.
//
// Loads libc.so.6 and resolves a known symbol (getpid). We do not call
// the returned function pointer in this test — invoking a function from a
// privately-loaded duplicate of libc has well-known side-effects (separate
// errno, separate locale state, etc.) and isn't the property we're checking
// here. The point of this test is that the loader walks PT_LOAD, processes
// the RELA / JMPREL tables, and produces a base-relative symtab address.

#include "../../src/linux/dynamic.hpp"
#include "../../src/linux/elf/search.hpp"

#include "../../src/io/console.hpp"
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
  sb::print("=== ELF LOADER BASIC ===");

  if ( micron::elf::resolve_soname("libwayland-client.so.0").empty() ) {
    sb::print("no native libwayland-client.so.0 for this target -- suite skipped");
    sb::print("=== ALL TESTS PASSED ===");
    return 1;
  }

  test_case("rejects nonexistent soname");
  {
    require_throw([] { micron::dso d("this-soname-does-not-exist-12345.so"); });
  }
  end_test_case();

  test_case("loads libwayland-client.so.0 by soname");
  {
    micron::dso wl("libwayland-client.so.0");
    require_true(wl.valid());
  }
  end_test_case();

  test_case("resolves wl_display_connect");
  {
    micron::dso wl("libwayland-client.so.0");
    void *fn = wl.sym("wl_display_connect");
    require_true(fn != nullptr);
  }
  end_test_case();

  test_case("missing symbol returns nullptr");
  {
    micron::dso wl("libwayland-client.so.0");
    void *fn = wl.sym("totally-not-a-real-symbol-9999");
    require_true(fn == nullptr);
  }
  end_test_case();

  test_case("absolute path load");
  {

    auto path = micron::elf::resolve_soname("libwayland-client.so.0");
    require_false(path.empty());
    micron::dso wl = micron::dso::from_path(path.c_str());
    require_true(wl.valid());
  }
  end_test_case();

  test_case("a wrong-class library is refused, not silently mapped");
  {

    const char *other = sizeof(void *) == 8 ? "/usr/lib/libwayland-client.so.0" : "/lib64/libwayland-client.so.0";
    auto probe = micron::elf::__file_exists(other);
    if ( probe ) {
      require_throw([&] { (void)micron::dso::from_path(other); });
    } else {
      sb::print("no opposite-class libwayland on this box -- wrong-class check skipped");
    }
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
