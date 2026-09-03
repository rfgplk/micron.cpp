//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ATTACH_MODULE 1
#define MICRON_MX_CONTINUATION 1

#include "../src/attach/continue.hpp"

#include "snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::test_case;

namespace
{

i64 g_seen_user_len = 0;
u64 g_seen_self_base = 0;
const micron_cont_args *g_seen_published = nullptr;
u32 g_calls = 0;

}      // namespace

extern "C" i64
mx_continue_main(const micron_cont_args *a) noexcept
{
  ++g_calls;
  g_seen_user_len = static_cast<i64>(a->user_len);
  g_seen_self_base = a->self_base;
  g_seen_published = micron::__micron_cont_args;
  return static_cast<i64>(a->user_len) * 5;
}

namespace
{

micron_cont_args
good(void) noexcept
{
  micron_cont_args a{};
  a.abi = micron_cont_args_abi;
  a.self_base = 0x40000;
  a.self_span = 0x1000;
  a.host_base = 0x80000;
  a.host_span = 0x2000;
  a.user_len = 9;
  return a;
}

micron_attach_info g_fake_info{};

}      // namespace

int
main()
{
  sb::print("=== MICRON _CONTINUE ===");

  micron::__micron_attach_info = nullptr;

  test_case("a null argument is refused before anything is read");
  {
    require(__micron_continuec(nullptr) == cont_efault, true);
    require(g_calls == 0u, true);
  }
  end_test_case();

  test_case("the direct form entered before _attach is refused");
  {
    micron_cont_args a = good();
    require(__micron_continuec(&a) == cont_enotattached, true);
    require(g_calls == 0u, true);
  }
  end_test_case();

  test_case("a wrong abi is refused, and it is refused before the attach check");
  {
    micron_cont_args a = good();
    a.abi = micron_cont_args_abi + 1u;
    require(__micron_continuec(&a) == cont_ebadargs, true);
  }
  end_test_case();

  test_case("an unknown flag bit is refused rather than ignored");
  {
    micron_cont_args a = good();
    a.flags = ~micron_cont_args_flag_all;
    require(__micron_continuec(&a) == cont_ebadargs, true);
  }
  end_test_case();

  test_case("flag_graph with a null ops table is refused");
  {
    micron_cont_args a = good();
    a.flags = micron_cont_args_flag_graph;
    a.ops = 0;
    require(__micron_continuec(&a) == cont_ebadargs, true);
  }
  end_test_case();

  test_case("a zero span is refused on either side");
  {
    micron_cont_args a = good();
    a.self_span = 0;
    require(__micron_continuec(&a) == cont_ebadargs, true);
    micron_cont_args b = good();
    b.host_span = 0;
    require(__micron_continuec(&b) == cont_ebadargs, true);
    require(g_calls == 0u, true);
  }
  end_test_case();

  test_case("the loaded form runs without an attach, because it boots no runtime of its own");
  {
    micron_cont_args a = good();
    a.flags = micron_cont_args_flag_replace;
    require(__micron_continuec(&a) == 45, true);
    require(g_calls == 1u, true);
  }
  end_test_case();

  test_case("the direct form runs once _attach has been through");
  {
    micron::__micron_attach_info = &g_fake_info;
    micron_cont_args a = good();
    a.user_len = 3;
    require(__micron_continuec(&a) == 15, true);
    require(g_calls == 2u, true);
    require(g_seen_user_len == 3, true);
    require(g_seen_self_base == 0x40000u, true);
  }
  end_test_case();

  test_case("the argument is published before the guest is entered, and it is the same pointer");
  {
    micron_cont_args a = good();
    require(__micron_continuec(&a) == 45, true);
    require(g_seen_published == &a, true);
    require(micron::__micron_cont_args == &a, true);
  }
  end_test_case();

  test_case("a run may repeat: data persists across it, which is what a resident continuation is");
  {
    const u32 before = g_calls;
    micron_cont_args a = good();
    require(__micron_continuec(&a) == 45, true);
    require(__micron_continuec(&a) == 45, true);
    require(g_calls == before + 2u, true);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
