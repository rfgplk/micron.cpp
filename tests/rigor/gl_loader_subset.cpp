

#include "../../src/gfx/gl/loader.hpp"
#include "../../src/gfx/gl/opengl.hpp"

#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{

void *
stub_get_proc(const char *name)
{

  for ( usize i = 0; i < micron::gl::gl_table_size; ++i ) {
    const char *a = micron::gl::gl_table[i].name;
    const char *b = name;
    while ( *a && *a == *b ) {
      ++a;
      ++b;
    }
    if ( !*a && !*b ) return reinterpret_cast<void *>(i + 1);
  }
  return nullptr;
}

}      // namespace

int
main()
{
  sb::print("=== GL LOADER SUBSET ===");

  test_case("constexpr sym_index_of finds known names");
  {
    constexpr auto i_clear = micron::gl::sym_index_of("glClear");
    constexpr auto i_draw = micron::gl::sym_index_of("glDrawArrays");
    constexpr auto i_miss = micron::gl::sym_index_of("glNotARealFunction");

    static_assert(i_clear != u16(-1));
    static_assert(i_draw != u16(-1));
    static_assert(i_miss == u16(-1));
    require_true(true);
  }
  end_test_case();

  test_case("MICRON_GL_SYM macro resolves at compile time");
  {
    constexpr auto i = MICRON_GL_SYM(glClear);
    static_assert(i != u16(-1));
    require_true(i != u16(-1));
  }
  end_test_case();

  test_case("globals are null before __load_core_4_6");
  {
    require_true(micron::gl::glClear == nullptr);
    require_true(micron::gl::glDrawArrays == nullptr);
  }
  end_test_case();

  test_case("__load_core_4_6 populates every slot in gl_table");
  {
    micron::gl::__load_core_4_6(&stub_get_proc);

    constexpr auto i_clear = micron::gl::sym_index_of("glClear");
    require_true(reinterpret_cast<void *>(micron::gl::glClear) == reinterpret_cast<void *>(i_clear + 1));

    constexpr auto idx = micron::gl::sym_index_of("glDrawArrays");
    require_true(reinterpret_cast<void *>(micron::gl::glDrawArrays) == reinterpret_cast<void *>(idx + 1));
  }
  end_test_case();

  test_case("micron:: scope forwards see the same pointer");
  {
    require_true(reinterpret_cast<void *>(micron::glClear) == reinterpret_cast<void *>(micron::gl::glClear));
  }
  end_test_case();

  test_case("subset<...> packs only the named symbols");
  {
    micron::gl::subset<MICRON_GL_SYM(glClear), MICRON_GL_SYM(glDrawArrays), MICRON_GL_SYM(glViewport)> sub;
    static_assert(sub.count == 3);
    static_assert(sizeof(sub) == 3 * sizeof(void *));

    sub.load(&stub_get_proc);

    constexpr auto i_clear = MICRON_GL_SYM(glClear);
    constexpr auto i_draw = MICRON_GL_SYM(glDrawArrays);
    constexpr auto i_view = MICRON_GL_SYM(glViewport);

    require_true(sub.raw<i_clear>() == reinterpret_cast<void *>(i_clear + 1));
    require_true(sub.raw<i_draw>() == reinterpret_cast<void *>(i_draw + 1));
    require_true(sub.raw<i_view>() == reinterpret_cast<void *>(i_view + 1));
  }
  end_test_case();

  test_case("subset position_of is comptime-derived");
  {
    using s_t = micron::gl::subset<MICRON_GL_SYM(glClear), MICRON_GL_SYM(glDrawArrays), MICRON_GL_SYM(glViewport)>;
    static_assert(s_t::template position_of<MICRON_GL_SYM(glClear)>() == 0);
    static_assert(s_t::template position_of<MICRON_GL_SYM(glDrawArrays)>() == 1);
    static_assert(s_t::template position_of<MICRON_GL_SYM(glViewport)>() == 2);
    require_true(true);
  }
  end_test_case();

  test_case("subset::as<...,Fn> reinterprets correctly");
  {
    micron::gl::subset<MICRON_GL_SYM(glClear)> sub;
    sub.load(&stub_get_proc);
    auto clear = sub.as<MICRON_GL_SYM(glClear), micron::gl::PFNGLCLEARPROC>();
    constexpr auto i_clear = MICRON_GL_SYM(glClear);
    require_true(reinterpret_cast<void *>(clear) == reinterpret_cast<void *>(i_clear + 1));
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
