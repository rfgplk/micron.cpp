

#include "../../src/dynamic.hpp"
#include "../../src/linux/elf/host_modules.hpp"
#include "../../src/linux/elf/read.hpp"

#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace mc = micron;
namespace dl = micron::elf::dl;

namespace
{

bool
any_valid_non_exec()
{
  for ( usize k = 0; k < mc::elf::__host_module_count; ++k ) {
    const auto &m = mc::elf::__host_modules[k];
    if ( !m.exec && m.valid ) return true;
  }
  return false;
}

const mc::elf::host_module_t *
first_valid()
{
  for ( usize k = 0; k < mc::elf::__host_module_count; ++k )
    if ( mc::elf::__host_modules[k].valid ) return &mc::elf::__host_modules[k];
  return nullptr;
}

};      // namespace

int
main()
{
  sb::print("=== DL SECURE-EXEC + HOST-MODULE TRUST ===");

  test_case("an ordinary test binary is not AT_SECURE");
  {

    require_false(dl::is_secure_exec());
  }
  end_test_case();

  test_case("__segment_is_trusted accepts only absolute $-free directories");
  {
    require_true(dl::__segment_is_trusted("/usr/lib64", 10));
    require_false(dl::__segment_is_trusted("usr/lib64", 9));
    require_false(dl::__segment_is_trusted("./lib", 5));
    require_false(dl::__segment_is_trusted("/opt/$ORIGIN/l", 14));
    require_false(dl::__segment_is_trusted("/a/${LIB}/b", 11));
    require_false(dl::__segment_is_trusted("", 0));
  }
  end_test_case();

  test_case("ordinary exec honours a path list, AT_SECURE drops the untrusted forms");
  {

    const char *soname = "libc.so.6";
    auto sys = dl::resolve_dependency(soname, nullptr, nullptr, nullptr);
    if ( sys.empty() ) {
      sb::print("no libc.so.6 in the default search paths -- path-list cases skipped");
    } else {

      auto dir = dl::__dirname_of(sys.c_str());
      require_true(!dir.empty());

      dl::__secure_exec_state = 1u;
      require_true(!dl::__try_path_list(dir.c_str(), soname, nullptr, false).empty());
      require_true(!dl::__try_path_list("$ORIGIN", soname, dir.c_str(), false).empty());

      dl::__secure_exec_state = 2u;
      require_true(!dl::__try_path_list(dir.c_str(), soname, nullptr, true).empty());
      require_true(dl::__try_path_list("$ORIGIN", soname, dir.c_str(), true).empty());
      require_true(dl::__try_path_list("relative/dir", soname, nullptr, true).empty());

      dl::__secure_exec_state = 1u;
    }
  }
  end_test_case();

  test_case("AT_SECURE refuses a relative explicit path, keeps an absolute one");
  {
    dl::__secure_exec_state = 2u;
    require_true(dl::resolve_dependency("./nope/libx.so", nullptr, nullptr, nullptr).empty());
    require_true(dl::resolve_dependency("../nope/libx.so", nullptr, nullptr, nullptr).empty());
    dl::__secure_exec_state = 1u;
  }
  end_test_case();

  mc::elf::init_host_modules();

  test_case("the host table is populated and every entry carries an extent");
  {
    require_true(mc::elf::host_count() > 0);
    for ( usize k = 0; k < mc::elf::__host_module_count; ++k ) {
      const auto &m = mc::elf::__host_modules[k];
      require_true(m.base != nullptr);
      require_true(m.span > 0);
    }
  }
  end_test_case();

  test_case("no entry is valid without an executable mapping");
  {
    require_false(any_valid_non_exec());
    const auto *m = first_valid();
    if ( m ) require_true(m->exec);
  }
  end_test_case();

  test_case("a valid module's dyn pointers lie inside its own mapping");
  {
    const auto *m = first_valid();
    if ( m ) {
      require_true(mc::elf::__host_in_span(*m, reinterpret_cast<uintptr_t>(m->dyn.strtab), 1));
      require_true(mc::elf::__host_in_span(*m, reinterpret_cast<uintptr_t>(m->dyn.symtab), 1));

      require_true(m->dyn.strsz <= static_cast<u64>(m->span));
    }
  }
  end_test_case();

  test_case("__build_host_dyn refuses a mapping with no executable segment");
  {

    const auto *donor = first_valid();
    if ( donor == nullptr ) {
      sb::print("no loaded module to re-describe -- exec-gate case skipped");
    } else {
      mc::elf::host_module_t probe;
      probe.base = donor->base;
      probe.span = donor->span;
      probe.path = donor->path;

      probe.exec = false;
      mc::elf::__build_host_dyn(probe);
      require_false(probe.valid);

      mc::elf::host_module_t accept;
      accept.base = donor->base;
      accept.span = donor->span;
      accept.path = donor->path;
      accept.exec = true;
      mc::elf::__build_host_dyn(accept);
      require_true(accept.valid);
    }
  }
  end_test_case();

  test_case("a live read-only source mapping adds no non-exec symbol source");
  {
    const auto *donor = first_valid();
    if ( donor == nullptr || donor->path.empty() ) {
      sb::print("no donor module -- read-only mapping case skipped");
    } else {
      mc::sstring<384> donor_path = donor->path;

      mc::elf::invalidate_host_modules();
      auto src = mc::elf::read::source::open(donor_path.c_str());
      require_true(src.is_first());

      mc::elf::init_host_modules();
      require_false(any_valid_non_exec());

      src.cast<mc::elf::read::source>().reset();
      require_false(mc::elf::__host_initialized);

      mc::elf::init_host_modules();
      require_true(mc::elf::host_count() > 0);
      require_false(any_valid_non_exec());
    }
  }
  end_test_case();

  test_case("unmapping through elf::read::source invalidates the host snapshot");
  {
    mc::elf::init_host_modules();
    require_true(mc::elf::__host_initialized);
    {
      auto src = mc::elf::read::source::open("/proc/self/exe");
      if ( src.is_first() ) {
      }
    }
    require_false(mc::elf::__host_initialized);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
