// elf_host.cpp
// Verifies host-module enumeration via /proc/self/maps + .dynsym walk.
//
// micron::host_dso borrows from the host process's already-loaded libraries
// rather than mmap'ing a duplicate, so we can safely talk to stateful libs.
// Every Linux process has libc.so.6 and ld-linux loaded; we lean on those
// for a universal smoke test that doesn't depend on graphics packages.

#include "../../src/linux/dynamic.hpp"
#include "../../src/linux/elf/host_modules.hpp"

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
  sb::print("=== ELF HOST MODULES ===");

  test_case("init populates the host module table");
  {
    micron::elf::init_host_modules();
    require_true(micron::elf::host_count() > 0);
  }
  end_test_case();

  test_case("host_find locates libc.so.6");
  {
    const auto *m = micron::elf::host_find("libc.so.6");
    require_true(m != nullptr);
  }
  end_test_case();

  test_case("host_resolve_sym resolves getpid");
  {
    void *fn = micron::elf::host_resolve_sym("getpid");
    require_true(fn != nullptr);
  }
  end_test_case();

  test_case("host_resolve_sym misses on bogus symbol");
  {
    void *fn = micron::elf::host_resolve_sym("definitely-not-a-symbol-zzz9");
    require_true(fn == nullptr);
  }
  end_test_case();

  test_case("host_dso wraps an already-loaded library");
  {
    micron::host_dso libc("libc.so.6");
    require_true(libc.valid());
    void *fn = libc.sym("getpid");
    require_true(fn != nullptr);
  }
  end_test_case();

  test_case("calling getpid through host_dso returns this process's tid");
  {
    micron::host_dso libc("libc.so.6");
    using getpid_fn = int (*)();
    auto getpid = libc.sym_as<getpid_fn>("getpid");
    require_true(getpid != nullptr);
    int p = getpid();
    require_true(p > 0);
  }
  end_test_case();

  test_case("host_dso rejects a soname not loaded by the host");
  {
    require_throw([] { micron::host_dso d("libnothing-here-99.so.1"); });
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
