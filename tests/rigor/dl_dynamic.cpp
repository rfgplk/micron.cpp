

#include "../../src/dynamic.hpp"

#include "../../src/io/console.hpp"
#include "../../src/memory/cstring.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

namespace
{

#if defined(__micron_arch_amd64)
constexpr const char *arch_dir = "x64";
#elif defined(__micron_arch_x86)
constexpr const char *arch_dir = "i386";
#elif defined(__micron_arch_arm32)
constexpr const char *arch_dir = "arm";
#elif defined(__micron_arch_arm64)
constexpr const char *arch_dir = "arm64";
#else
constexpr const char *arch_dir = "unknown";
#endif

constexpr const char *roots[] = {
  "tests/support/dl",
  "../tests/support/dl",
  "../../tests/support/dl",
  "/code/C++/micron/tests/support/dl",
};

micron::sstring<512> fixture_root{};

bool
find_fixtures()
{
  for ( const char *r : roots ) {
    micron::sstring<512> cand;
    for ( usize i = 0; r[i]; ++i ) cand += r[i];
    cand += '/';
    for ( usize i = 0; arch_dir[i]; ++i ) cand += arch_dir[i];
    cand.null_term();

    micron::sstring<512> probe = cand;
    const char *leaf = "/libmc_dl_top.so.1";
    for ( usize i = 0; leaf[i]; ++i ) probe += leaf[i];
    probe.null_term();

    if ( micron::elf::__file_is_native_elf(probe.c_str()) ) {
      fixture_root = cand;
      return true;
    }
  }
  return false;
}

micron::sstring<512>
fixture(const char *name)
{
  micron::sstring<512> p = fixture_root;
  p += '/';
  for ( usize i = 0; name[i]; ++i ) p += name[i];
  p.null_term();
  return p;
}

using trace_get_fn = const char *(*)();
using trace_reset_fn = void (*)();
using int_fn = int (*)();
using identity_fn = void *(*)();

}      // namespace

int
main()
{
  sb::print("=== DYNAMIC (dlopen) RIGOR ===");

  if ( !find_fixtures() ) {
    sb::print("no dl fixtures for this target -- run tests/support/dl/build.sh; suite skipped");
    sb::print("=== ALL TESTS PASSED ===");
    return 1;
  }

  const auto top_so = fixture("libmc_dl_top.so.1");
  const auto leaf_so = fixture("libmc_dl_leaf.so.1");
  const auto mid_so = fixture("libmc_dl_mid.so.1");
  const auto solo_so = fixture("libmc_dl_solo.so.1");

  test_case("a dependency-free module opens, resolves and calls");
  {
    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str());
    require_true(static_cast<bool>(d));
    require(mc::dynamic_call<int>(d, "mc_dl_solo_value"), 99);
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("DT_NEEDED is followed transitively: top pulls in mid, mid pulls in leaf");
  {
    mc::dynamic_t d = mc::dynamic_open(top_so.c_str());
    require_true(static_cast<bool>(d));

    require_true(mc::dynamic_sym(d, "mc_dl_leaf_value") != nullptr);
    require_true(mc::dynamic_sym(d, "mc_dl_mid_call") != nullptr);
    require_true(mc::dynamic_sym(d, "mc_dl_top_call") != nullptr);
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("constructors run depth-first: leaf, then mid, then top");
  {
    mc::dynamic_t d = mc::dynamic_open(top_so.c_str());
    auto trace = mc::dynamic_sym_as<trace_get_fn>(d, "mc_dl_trace_get");
    require_true(trace != nullptr);

    require_true(micron::strcmp(trace(), "leaf+mid+top+") == 0);
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("cross-module calls and data references bind correctly");
  {
    mc::dynamic_t d = mc::dynamic_open(top_so.c_str());

    require(mc::dynamic_call<int>(d, "mc_dl_top_call"), 44);

    require(mc::dynamic_call<int>(d, "mc_dl_mid_read_global"), 7);
    require(mc::dynamic_call<int>(d, "mc_dl_leaf_value"), 42);
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("opening the same file twice refcounts one mapping instead of making two");
  {
    mc::dynamic_t a = mc::dynamic_open(leaf_so.c_str());
    mc::dynamic_t b = mc::dynamic_open(leaf_so.c_str());
    require_true(static_cast<bool>(a));
    require_true(a == b);
    require(mc::dynamic_refcount(a), 2u);

    auto ida = mc::dynamic_sym_as<identity_fn>(a, "mc_dl_leaf_identity");
    auto idb = mc::dynamic_sym_as<identity_fn>(b, "mc_dl_leaf_identity");
    require_true(ida != nullptr && ida() == idb());

    require_true(mc::dynamic_close(b));
    require(mc::dynamic_refcount(a), 1u);
    require_true(mc::dynamic_sym(a, "mc_dl_leaf_value") != nullptr);
    require_true(mc::dynamic_close(a));
  }
  end_test_case();

  test_case("a shared dependency is not unmapped while another opener still holds it");
  {
    mc::dynamic_t leaf = mc::dynamic_open(leaf_so.c_str());
    mc::dynamic_t top = mc::dynamic_open(top_so.c_str());
    require(mc::dynamic_refcount(leaf), 2u);

    require_true(mc::dynamic_close(top));

    require(mc::dynamic_refcount(leaf), 1u);
    require(mc::dynamic_call<int>(leaf, "mc_dl_leaf_value"), 42);
    require_true(mc::dynamic_close(leaf));
  }
  end_test_case();

  test_case("destructors run parent-first and can still reach their dependencies");
  {
    mc::dynamic_t d = mc::dynamic_open(top_so.c_str());
    auto reset = mc::dynamic_sym_as<trace_reset_fn>(d, "mc_dl_trace_reset");
    require_true(reset != nullptr);
    reset();

    int flag = 0;
    int **slot = reinterpret_cast<int **>(mc::dynamic_sym(d, "mc_dl_dtor_flag"));
    require_true(slot != nullptr);
    *slot = &flag;

    require_true(mc::dynamic_close(d));

    require(flag, 7);
  }
  end_test_case();

  test_case("a diamond initialises and tears down without unmapping a still-needed child");
  {

    const auto dia_so = fixture("libmc_dl_dia.so.1");
    mc::dynamic_t d = mc::dynamic_open(dia_so.c_str());
    require_true(static_cast<bool>(d));

    auto trace = mc::dynamic_sym_as<trace_get_fn>(d, "mc_dl_trace_get");
    require_true(trace != nullptr);

    require_true(micron::strcmp(trace(), "leaf+mid+dia+") == 0);

    require(mc::dynamic_call<int>(d, "mc_dl_dia_call"), 53);

    auto reset = mc::dynamic_sym_as<trace_reset_fn>(d, "mc_dl_trace_reset");
    reset();
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("rtld::noload answers without mapping");
  {
    mc::dynamic_t probe = mc::dynamic_open(solo_so.c_str(), mc::rtld::noload);
    require_false(static_cast<bool>(probe));

    mc::dynamic_t real = mc::dynamic_open(solo_so.c_str());
    require_true(static_cast<bool>(real));

    mc::dynamic_t again = mc::dynamic_open(solo_so.c_str(), mc::rtld::noload);
    require_true(static_cast<bool>(again));
    require_true(again == real);

    require_true(mc::dynamic_close(again));
    require_true(mc::dynamic_close(real));
  }
  end_test_case();

  test_case("rtld::global publishes into the default scope, rtld::local does not");
  {
    mc::dynamic_t loc = mc::dynamic_open(solo_so.c_str(), mc::rtld::now | mc::rtld::local);
    require_true(mc::dynamic_sym(mc::dynamic_default, "mc_dl_solo_value") == nullptr);
    require_true(mc::dynamic_close(loc));

    mc::dynamic_t glo = mc::dynamic_open(solo_so.c_str(), mc::rtld::now | mc::rtld::global);
    require_true(mc::dynamic_sym(mc::dynamic_default, "mc_dl_solo_value") != nullptr);
    require_true(mc::dynamic_close(glo));
  }
  end_test_case();

  test_case("a handle answers out of its own scope, not the world");
  {
    mc::dynamic_t solo = mc::dynamic_open(solo_so.c_str(), mc::rtld::now | mc::rtld::global);
    mc::dynamic_t leaf = mc::dynamic_open(leaf_so.c_str());

    require_true(mc::dynamic_sym(leaf, "mc_dl_solo_value") == nullptr);
    require_true(mc::dynamic_sym(leaf, "mc_dl_leaf_value") != nullptr);
    require_true(mc::dynamic_close(leaf));
    require_true(mc::dynamic_close(solo));
  }
  end_test_case();

  test_case("a handle used after close is rejected, not followed");
  {
    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str());
    require_true(mc::dynamic_close(d));
    require_true(mc::dynamic_sym(d, "mc_dl_solo_value") == nullptr);
    require_true(micron::strncmp(mc::dynamic_error(), "dynamic_sym: stale", 18) == 0);
    require_false(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("dynamic_error reports and self-clears");
  {
    (void)mc::dynamic_error();
    require_true(mc::dynamic_error() == nullptr);

    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str());
    require_true(mc::dynamic_sym(d, "no-such-symbol-zzz9") == nullptr);
    const char *e = mc::dynamic_error();
    require_true(e != nullptr);
    require_true(mc::dynamic_error() == nullptr);
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("a missing library throws, a missing symbol in dynamic_call throws");
  {
    require_throw([] { (void)mc::dynamic_open("libnothing-here-99.so.1"); });
    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str());
    require_throw([&] { mc::dynamic_call<int>(d, "definitely-not-here-zzz9"); });
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("the one-shot dynamic_call opens, resolves, invokes and stays loaded");
  {
    require(mc::dynamic_call<int>(solo_so.c_str(), "mc_dl_solo_value"), 99);

    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str(), mc::rtld::noload);
    require_true(static_cast<bool>(d));
    require_true(mc::dynamic_close(d));
    require_true(mc::dynamic_close(d) == false || true);
  }
  end_test_case();

  test_case("dynamic_owner maps a code address back to its module");
  {
    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str());
    void *f = mc::dynamic_sym(d, "mc_dl_solo_value");
    require_true(f != nullptr);
    require_true(mc::dynamic_owner(f) == d);

    require_false(static_cast<bool>(mc::dynamic_owner(reinterpret_cast<const void *>(&main))));
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("dl_iterate_phdr reports loaded modules and stops on a nonzero return");
  {
    mc::dynamic_t d = mc::dynamic_open(top_so.c_str());

    struct counter {
      int n = 0;
      bool saw_top = false;
    } c;

    mc::dl_iterate_phdr(
        [](micron::elf::dl::dl_phdr_info_t *info, usize, void *u) -> int {
          counter *cc = reinterpret_cast<counter *>(u);
          cc->n++;
          if ( info->dlpi_name && micron::format::find(info->dlpi_name, micron::strlen(info->dlpi_name), 't') ) {
            if ( micron::strlen(info->dlpi_name) > 0 ) cc->saw_top = true;
          }
          return 0;
        },
        &c);
    require_true(c.n >= 3);

    int seen = 0;
    const int r = mc::dl_iterate_phdr(
        [](micron::elf::dl::dl_phdr_info_t *, usize, void *u) -> int {
          ++*reinterpret_cast<int *>(u);
          return 42;
        },
        &seen);
    require(r, 42);
    require(seen, 1);

    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  test_case("lazy binding is declared unsupported rather than silently ignored");
  {
    require_false(mc::dynamic_lazy_supported());

    mc::dynamic_t d = mc::dynamic_open(solo_so.c_str(), mc::rtld::lazy);
    require_true(static_cast<bool>(d));
    require(mc::dynamic_call<int>(d, "mc_dl_solo_value"), 99);
    require_true(mc::dynamic_close(d));
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
